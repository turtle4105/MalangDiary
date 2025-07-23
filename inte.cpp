#define WIN32_LEAN_AND_MEAN     // ������ ��� �淮ȭ (���ʿ��� macro ����)
#include "TcpServer.h"          // TCP ���� Ŭ���� ���
#include "DBManager.h"          // DB ���� Ŭ���� ���
#include "ProtocolHandler.h"    // Protocol �Ľ� Ŭ���� ��� 
#include "FileUtils.h"          // ���丮 ���� Ŭ���� ���

#include <iostream>
#include <thread>
#include <string>
#include <codecvt> 
#include <locale>
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

                if (response.value("RESP", "") == "SUCCESS") {
                    int parent_uid = json.value("PARENTS_UID", -1);
                    string parent_id;

                    if (db.getParentIdByUID(parent_uid, parent_id)) {
                        string child_name = json.value("NAME", "");
                        int child_uid = response.value("CHILD_UID", -1);
                        CreateChildDirectory(parent_id, parent_uid, child_uid);
                    }
                }
                LINE
            }

            else if (protocol == u8"GET_LATEST_DIARY") {
                LINE_LABEL("GET_LATEST_DIARY")
                db.connect();
                nlohmann::json response = ProtocolHandler::handle_LatestDiary(json, db);
                TcpServer::sendJsonResponse(clientSocket, response.dump());
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
                LINE_LABEL("SETTING_VOICE")
                db.connect();

                // [1] JSON에서 필요한 데이터 추출 (이름, etc.)
                string child_name = json.value("CHILD_NAME", "전인우");  // 아이이름 
                int child_uid = json.value("CHILD_UID", -1);       // 아이의 uid

                // [2] Python 서버로 HTTP POST 요청 (multipart/form-data)
                httplib::Client cli("http://localhost:8000");  // Python 서버 URL (포트 확인)
                httplib::MultipartFormDataItems items = {
                    {"child_name", child_name, "", ""},
                    {"file", string(payload.begin(), payload.end()), "audio.wav", "audio/wav"},  // 음성 파일 (payload를 바이너리로)
                    {"embedding_file", "임베딩 문자열 또는 파일 바이너리", "embedding.json", "application/json"}  // 임베딩 데이터 (문자열 or payload 일부)
                };

                auto res = cli.Post("/transcribe", items);  // 엔드포인트 맞춤
                if (res && res->status == 200) {
                    nlohmann::json py_response = nlohmann::json::parse(res->body);
                    
                    // [4] DB에 저장 

                    // [5] 클라이언트에 성공 응답
                    nlohmann::json response;
                    response["PROTOCOL"] = "PROCESS_VOICE";
                    response["RESP"] = "SUCCESS";
                    response["DIARY_DATA"] = py_response;  // Python 응답 포함
                    TcpServer::sendJsonResponse(clientSocket, response.dump());
                }
                else{
                    // 실패 응답
                    TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"PROCESS_VOICE","RESP":"FAIL","MESSAGE":"PYTHON_SERVER_ERROR"})");
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


/*
서현님 코드에 내가 만든 서버와 연결하는 코드를 통합한 내용입니다.*/

