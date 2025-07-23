#define WIN32_LEAN_AND_MEAN     // ������ ��� �淮ȭ (���ʿ��� macro ����)
#include "TcpServer.h"          // TCP ���� Ŭ���� ���
#include "DBManager.h"          // DB ���� Ŭ���� ���
#include "ProtocolHandler.h"    // Protocol �Ľ� Ŭ���� ��� 
#include "FileUtils.h"          // ���丮 ���� Ŭ���� ���
#include <curl/curl.h>
#include <iostream>
#include <thread>
#include <string>
#include <codecvt> 
#include <locale>
#include <fstream>
#include <nlohmann/json.hpp>
#include <fcntl.h>     // _O_U8TEXT
#include <io.h>        // _setmode, _fileno
#include "httplib.h"

#define LINE_LABEL(label) cout << "=======================[" << label << "]=======================\n";
#define LINE cout << "=====================================================\n";
const std::string BASE_DIR = "user_data"; // ��Ʈ ��� (Visual Studio ������Ʈ �������� ��� ���)


using json = nlohmann::json;
using namespace std;

bool CreateUserDirectory(const string& parentId, const int& uid);

// UTF-8 -> UTF-16 ��ȯ �� ���
//static std::wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;

// UTF-8 ���ڿ� �� UTF-16 (wstring)
wstring Utf8ToWstring(const string& utf8) {
    if (utf8.empty()) return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len == 0) {
        // ��ȯ ���� �� ���� ��� �Ǵ� �⺻��
        return L"[��ȯ����]";
    }

    wstring wide(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], len);
    wide.pop_back();  // ������ null ���� ����
    return wide;
}

// =======================================================================
// [�Լ�] Ŭ���̾�Ʈ �޽�����
// UTF-8 �� �����ڵ�(wstring) ��ȯ �Լ�
// - Ŭ���̾�Ʈ�κ��� ������ ���ڿ��� UTF-8�� ��� �ѱ� ���� ������ ���� ���
// =======================================================================
// �׽�Ʈ
bool ReadExactBytes(SOCKET sock, char* buffer, int totalBytes) {
    int received = 0;
    while (received < totalBytes) {
        int len = recv(sock, buffer + received, totalBytes - received, 0);
        if (len <= 0) return false;
        received += len;
    }
    return true;
}

// =======================================================================
// [�Լ�] Ŭ���̾�Ʈ ��û ó�� �Լ� (������� ����)
// - Ŭ���̾�Ʈ�� ���� �޽����� �����ϰ�, ������ ��ȯ�ϴ� ����
// - �� Ŭ���̾�Ʈ���� �� �Լ��� ���������� �����
// =======================================================================
void handleClient(SOCKET clientSocket) {

    //cout << u8"[DEBUG] handleClient ����\n";

    while (1) {
        DBManager db;
        string jsonStr;
        vector<char> payload;

        // [1] JSON + payload ���� (��� ����)
        if (!TcpServer::receivePacket(clientSocket, jsonStr, payload)) {
            cerr << u8"[��Ŷ ���� ����] JSON ���� ����\n";
            TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"UNKNOWN","RESP":"FAIL","MESSAGE":"RECEIVE_FAIL"})");
            closesocket(clientSocket);
            return;
        }
          // [2] JSON ��ȿ�� �˻�
        if (!nlohmann::json::accept(jsonStr)) {
            cerr << u8"[JSON ���� ����] ��ȿ���� ���� JSON�Դϴ�.\n";
            TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"UNKNOWN","RESP":"FAIL","MESSAGE":"INVALID_JSON"})");
            closesocket(clientSocket);
            return;
        }

        try {
            //cout << u8"[STEP 1] JSON �Ľ� ����" << endl;
            nlohmann::json json = nlohmann::json::parse(jsonStr);
            //cout << u8"[STEP 1-OK] JSON �Ľ� ����" << endl;

            string protocol = json.value("PROTOCOL", "");
            //cout << u8"[STEP 2] ��������: " << protocol << endl;

            if (protocol == u8"REGISTER_PARENTS") {
                LINE_LABEL("REGISTER_PARENTS")
                db.connect();

                //cout << u8"[STEP 4] ProtocolHandler ����" << endl;
                nlohmann::json response = ProtocolHandler::handle_RegisterParents(json, db);

                //cout << u8"[STEP 5] ���� ���� ����" << endl;
                TcpServer::sendJsonResponse(clientSocket, response.dump());

                // ���� ���丮 ���⼭ ���ܿ� ~! 
                if (response.value("RESP", "") == "SUCCESS") {
                    string id = json.value("ID", "");
                    int uid = response.value("PARENTS_UID", -1);
                    CreateUserDirectory(id, uid);
                    LINE
                }
            }

            else if (protocol == u8"LOGIN") {
                LINE_LABEL("LOGIN")
                db.connect();
                nlohmann::json response = ProtocolHandler::handle_Login(json, db);
                TcpServer::sendJsonResponse(clientSocket, response.dump());
                LINE
            }

            else if (protocol == u8"REGISTER_CHILD") {
                LINE_LABEL("REGISTER_CHILD")
                db.connect();
                nlohmann::json response = ProtocolHandler::handle_RegisterChild(json, db);
                TcpServer::sendJsonResponse(clientSocket, response.dump());
                LINE
            }

            else if (protocol == u8"GET_LATEST_DIARY") {
                LINE_LABEL("GET_LATEST_DIARY")
                    db.connect();

                // [1] �ϱ� + ���� ��Ÿ ���� ��ȸ
                nlohmann::json response = ProtocolHandler::handle_LatestDiary(json, db);

                // [2] ���� ���̳ʸ� �ε�
                std::vector<char> payload;
                if (response.value("HAS_PHOTO", false)) {
                    std::string abs_path = "user_data" + response.value("PHOTO_PATH", "");
                    std::replace(abs_path.begin(), abs_path.end(), '/', '\\');

                    std::ifstream file(abs_path, std::ios::binary);
                    if (file) {
                        payload = std::vector<char>((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
                    }
                }

                // [3] JSON + payload ���� ���� (1ȸ ����!)
                TcpServer::sendPacket(clientSocket, response.dump(), payload);

                LINE
            }

            else if (protocol == u8"SETTING_VOICE") {
                LINE_LABEL("SETTING_VOICE")
                db.connect();
                nlohmann::json response = ProtocolHandler::handle_SettingVoice(json, payload, db);
                TcpServer::sendJsonResponse(clientSocket, response.dump());
                LINE
            }

            else if (protocol == u8"GEN_DIARY") {
                LINE_LABEL("GEN_DIARY")
                    db.connect();
                cout << u8"[GEN_DIARY] ��û:\n" << json.dump(2) << endl;

                string child_name = json.value("CHILD_NAME", "���ο�");
                int child_uid = json.value("CHILD_UID", -1);

                // 1. payload ����
                if (payload.size() < 44) { // �ּ� WAV ��� ũ��
                    TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"INVALID_AUDIO_SIZE"})");
                    LINE
                        return;
                }
                cout << u8"[GEN_DIARY] Audio payload size: " << payload.size() << " bytes" << endl;

                // 2. ���� ���� ����
                std::string audio_path = "C:/python_workspace/MalangDiary/data/inwoo_diary.wav";
                std::ofstream ofs(audio_path, std::ios::binary);
                if (!ofs.is_open()) {
                    TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"AUDIO_FILE_WRITE_ERROR"})");
                    LINE
                        return;
                }
                ofs.write(payload.data(), payload.size());
                ofs.close();

                // 3. �Ӻ��� ���ڿ� ����
                std::string embedding_data = json.value("EMBEDDING_DATA", "[0.10536525398492813,0.0,0.0,0.0,0.0,0.0,0.0,0.14744506776332855,0.004937579855322838,0.17479261755943298,0.06373615562915802,0.08198851346969604,0.03966839984059334,0.025140749290585518,0.0,0.0,0.11556734889745712,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.018912233412265778,0.0,0.0,0.050996165722608566,0.0,0.08332962542772293,0.06788799166679382,0.11879827082157135,0.013113993220031261,0.0532664991915226,0.05336317420005798,0.0,0.0,0.0,0.09670987725257874,0.010388411581516266,0.09378775954246521,0.06565140932798386,0.0,0.09055177867412567,0.0,0.1321539431810379,0.0,0.0069200024008750916,0.0,0.0,0.0,0.0,0.015766184777021408,0.0,0.008977294899523258,0.0,0.11799708753824234,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0767320841550827,0.0,0.0,0.0,0.0,0.06016342714428902,0.0,0.06941312551498413,0.0,0.0,0.0,0.056130532175302505,0.0,0.0,0.07815210521221161,0.0,0.0,0.015127808786928654,0.12008466571569443,0.13539718091487885,0.12517991662025452,0.06313008815050125,0.0,0.09056569635868073,0.0,0.0,0.0,0.0,0.053303103893995285,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.010747555643320084,0.0,0.09393686056137085,0.0,0.05816495046019554,0.0,0.011810600757598877,0.08441205322742462,0.0,0.02782626636326313,0.04225461557507515,0.046907346695661545,0.03502822667360306,0.0,0.048499107360839844,0.0,0.052725281566381454,0.0,0.0,0.056104060262441635,0.17207135260105133,0.059082791209220886,0.0,0.09805073589086533,0.0,0.1121729239821434,0.027458028867840767,0.0,0.0,0.0,0.0,0.12860001623630524,0.0013112802989780903,0.0,0.07360716909170151,0.0,0.12624073028564453,0.0,0.0,0.0,0.0,0.0,0.09217185527086258,0.0,0.0,0.0,0.09635742753744125,0.0,0.15457244217395782,0.08135044574737549,0.0,0.0,0.07077725231647491,0.05613444000482559,0.0,0.029485011473298073,0.0,0.14895856380462646,0.14272215962409973,0.0,0.0,0.004072296433150768,0.0486181378364563,0.028519731014966965,0.06104172766208649,0.05260615423321724,0.08927661925554276,0.08164189010858536,0.0,0.12847894430160522,0.03976507857441902,0.03530411794781685,0.0,0.005394948646426201,0.025604968890547752,0.0974198505282402,0.13515551388263702,0.03497400879859924,0.002324869157746434,0.0,0.0,0.07337424159049988,0.0,0.0,0.0,0.05793168023228645,0.05520328879356384,0.05291927978396416,0.06275950372219086,0.007361725438386202,0.0766606256365776,0.14770032465457916,0.0,0.11581330746412277,0.0,0.06596610695123672,0.09065289050340652,0.06525104492902756,0.03704148530960083,0.059521496295928955,0.0,0.0,0.0,0.0,0.177495539188385,0.071068674325943,0.0,0.0,0.0,0.0,0.0859004482626915,0.05506306514143944,0.0,0.15635564923286438,0.0,0.05706821009516716,0.1485048234462738,0.0013909117551520467,0.04134749621152878,0.0,0.0,0.0,0.0,0.04491094872355461,0.0,0.18181727826595306,0.0,0.0,0.0807722955942154,0.03402213007211685,0.0,0.0,0.0,0.1514233946800232,0.007192510180175304,0.04689515382051468,0.08952917158603668,0.07451057434082031,0.15092289447784424,0.025175033137202263,0.0,0.05732634291052818,0.273547500371933,0.10064282268285751,0.14913064241409302,0.06763939559459686,0.1062425822019577,0.048018503934144974,0.0,0.0,0.10734295099973679,0.0,0.0,0.0]");
                if (embedding_data.empty()) {
                    TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"NO_EMBEDDING_DATA"})");
                    LINE
                        return;
                }

                // 4. libcurl ����
                CURL* curl = curl_easy_init();
                std::string response_str;

                if (curl) {
                    struct curl_httppost* formpost = NULL;
                    struct curl_httppost* lastptr = NULL;

                    curl_formadd(&formpost, &lastptr,
                        CURLFORM_COPYNAME, "child_name",
                        CURLFORM_COPYCONTENTS, child_name.c_str(),
                        CURLFORM_END);

                    curl_formadd(&formpost, &lastptr,
                        CURLFORM_COPYNAME, "file",
                        CURLFORM_FILE, audio_path.c_str(),
                        CURLFORM_FILENAME, "audio.wav",
                        CURLFORM_CONTENTTYPE, "audio/wav",
                        CURLFORM_END);

                    curl_formadd(&formpost, &lastptr,
                        CURLFORM_COPYNAME, "embedding_file",
                        CURLFORM_COPYCONTENTS, embedding_data.c_str(),
                        CURLFORM_CONTENTTYPE, "application/json",
                        CURLFORM_END);

                    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/transcribe");
                    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
                        size_t totalSize = size * nmemb;
                        std::string* str = static_cast<std::string*>(userp);
                        str->append((char*)contents, totalSize);
                        return totalSize;
                        });
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_str);

                    CURLcode res = curl_easy_perform(curl);
                    if (res == CURLE_OK) {
                        cout << u8"[GEN_DIARY] ���� ����: " << response_str << endl;
                        try {
                            nlohmann::json py_response = nlohmann::json::parse(response_str);

                            nlohmann::json response;
                            response["PROTOCOL"] = "GEN_DIARY";
                            response["RESP"] = "SUCCESS";
                            response["DIARY_DATA"] = py_response;
                            TcpServer::sendJsonResponse(clientSocket, response.dump());
                        }
                        catch (...) {
                            TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"JSON_PARSE_ERROR"})");
                        }
                    }
                    else {
                        TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"PYTHON_SERVER_ERROR"})");
                    }
                    curl_formfree(formpost);
                    curl_easy_cleanup(curl);
                }
                else {
                    TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"CURL_INIT_FAIL"})");
                }
                LINE
            }

            else {
                cerr << u8"[����] �� �� ���� ��������: " << protocol << endl;
                TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"UNKNOWN","RESP":"FAIL","MESSAGE":"UNSUPPORTED_PROTOCOL"})");
            }
        }
        catch (const nlohmann::json::parse_error& e) {
            cerr << u8"[����] JSON �Ľ� ���� (parse_error): " << e.what() << endl;
        }
        catch (const nlohmann::json::type_error& e) {
            cerr << u8"[����] JSON Ÿ�� ���� (type_error): " << e.what() << endl;
        }
        catch (const exception& e) {
            cerr << u8"[����] ��Ÿ ����: " << e.what() << endl;
        }
    }
}

// =======================================================================
// [main �Լ�] 
// - TcpServer ��ü�� �����ϰ�, ���� Ŭ���̾�Ʈ�� �񵿱������� ó��
// =======================================================================

int main() {
    // �ܼ� �ѱ� ���� ���� ����
    SetConsoleOutputCP(CP_UTF8);                // UTF-8 ��� ���ڵ� ����
    TcpServer server(5556);                     // ��Ʈ 5556���� ���� ����
    DBManager db;

    // DB ���� �õ�
    if (!db.connect()) {
        cout << u8"[����] DB ���� ����!" << endl;
        return 1;
    }

    // ���� ���ε� �� ����
    if (!server.start()) {
        cout << u8"[����] ���� ���ε� �Ǵ� ���� ����!" << endl;
        return 1;
    }

    cout << u8"[����] Ŭ���̾�Ʈ ���� ��� ����..." << endl;

    // Ŭ���̾�Ʈ ���� ����
    while (true) {
        SOCKET clientSocket = server.acceptClient();

        if (clientSocket != INVALID_SOCKET) {
            cout << u8"[����] �� Ŭ���̾�Ʈ �����!" << endl;
            thread th(handleClient, clientSocket);
            th.detach();  // ���������� ����
        }
    }
    return 0;
}
