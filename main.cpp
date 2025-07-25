#define WIN32_LEAN_AND_MEAN     // 윈도우 헤더 경량화 (불필요한 macro 제거)
#include "TcpServer.h"          // TCP 서버 클래스 헤더
#include "DBManager.h"          // DB 실행 클래스 헤더
#include "ProtocolHandler.h"    // Protocol 파싱 클래스 헤더 
#include "FileUtils.h"          // 디렉토리 생성 클래스 헤더
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
#include <windows.h>
#include <wininet.h>

#define LINE_LABEL(label) cout << "=======================[" << label << "]=======================\n";
#define LINE cout << "=====================================================\n";
const std::string BASE_DIR = "user_data"; // 루트 경로 (Visual Studio 프로젝트 기준으로 상대 경로)


using json = nlohmann::json;
using namespace std;

bool CreateUserDirectory(const string& parentId, const int& uid);

// UTF-8 -> UTF-16 변환 및 출력
//static std::wstring_convert<codecvt_utf8_utf16<wchar_t>> converter;

// UTF-8 문자열 → UTF-16 (wstring)
wstring Utf8ToWstring(const string& utf8) {
    if (utf8.empty()) return L"";

    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (len == 0) {
        // 변환 실패 → 에러 출력 또는 기본값
        return L"[변환실패]";
    }

    wstring wide(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], len);
    wide.pop_back();  // 마지막 null 문자 제거
    return wide;
}

// =======================================================================
// [함수] 클라이언트 메시지용
// UTF-8 → 유니코드(wstring) 변환 함수
// - 클라이언트로부터 수신한 문자열이 UTF-8인 경우 한글 깨짐 방지를 위해 사용
// =======================================================================
// 테스트
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
// [함수] 클라이언트 요청 처리 함수 (쓰레드로 실행)
// - 클라이언트가 보낸 메시지를 수신하고, 응답을 반환하는 역할
// - 각 클라이언트마다 이 함수가 독립적으로 실행됨
// =======================================================================
void handleClient(SOCKET clientSocket) {

    //cout << u8"[DEBUG] handleClient 진입\n";

    while (1) {
        DBManager db;
        string jsonStr;
        vector<char> payload;

        // [1] JSON + payload 수신 (헤더 포함)
        if (!TcpServer::receivePacket(clientSocket, jsonStr, payload)) {
            cerr << u8"[패킷 수신 실패] JSON 수신 실패\n";
            TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"UNKNOWN","RESP":"FAIL","MESSAGE":"RECEIVE_FAIL"})");
            closesocket(clientSocket);
            return;
        }
          // [2] JSON 유효성 검사
        if (!nlohmann::json::accept(jsonStr)) {
            cerr << u8"[JSON 검증 실패] 유효하지 않은 JSON입니다.\n";
            TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"UNKNOWN","RESP":"FAIL","MESSAGE":"INVALID_JSON"})");
            closesocket(clientSocket);
            return;
        }

        try {
            //cout << u8"[STEP 1] JSON 파싱 시작" << endl;
            nlohmann::json json = nlohmann::json::parse(jsonStr);
            //cout << u8"[STEP 1-OK] JSON 파싱 성공" << endl;

            string protocol = json.value("PROTOCOL", "");
            //cout << u8"[STEP 2] 프로토콜: " << protocol << endl;

            if (protocol == u8"REGISTER_PARENTS") {
                LINE_LABEL("REGISTER_PARENTS")
                db.connect();

                //cout << u8"[STEP 4] ProtocolHandler 실행" << endl;
                nlohmann::json response = ProtocolHandler::handle_RegisterParents(json, db);

                //cout << u8"[STEP 5] 응답 전송 시작" << endl;
                TcpServer::sendJsonResponse(clientSocket, response.dump());

                // 계정 디렉토리 여기서 생겨요 ~! 
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

                // [1] 일기 + 사진 메타 정보 조회
                nlohmann::json response = ProtocolHandler::handle_LatestDiary(json, db);

                // [2] 사진 바이너리 로딩
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

                // [3] JSON + payload 통합 전송 (1회 전송!)
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
                nlohmann::json response = ProtocolHandler::handle_GenDiary(json, payload, db);
                TcpServer::sendJsonResponse(clientSocket, response.dump());
                cout << response.dump(2) << endl;
                LINE
            }

            else if (protocol == u8"SEND_DIARY_DETAIL") {
                LINE_LABEL("SEND_DIARY_DETAIL")
                db.connect();
                ProtocolHandler::handle_SendDiaryDetail(json, db, clientSocket);
                //이 함수는 핸들러 내부에서 샌드 두번 하기 위해서 전송이 핸들러에 있습니당
                LINE
            }

            else if (protocol == u8"DIARY_DEL") {
                LINE_LABEL("DIARY_DEL")
                db.connect();
                nlohmann::json response = ProtocolHandler::handle_DiaryDel(json, db);
                TcpServer::sendJsonResponse(clientSocket, response.dump());
                LINE
            }

            else if (protocol == u8"UPDATE_DIARY_LIKE") {
                LINE_LABEL("UPDATE_DIARY_LIKE")
                db.connect();
                nlohmann::json response = ProtocolHandler::handle_UpdateLike(json, db);
                TcpServer::sendJsonResponse(clientSocket, response.dump());
                LINE
            }

            else if (protocol == u8"GET_DIARY_DETAIL") {
                LINE_LABEL("GET_DIARY_DETAIL")
                db.connect();
                nlohmann::json response = ProtocolHandler::handle_SendDiaryDetail(json, db, clientSocket);
                TcpServer::sendJsonResponse(clientSocket, response.dump());
                LINE
            }

            else if (protocol == u8"MODIFY_DIARY") {
                LINE_LABEL("MODIFY_DIARY")
                db.connect();
                nlohmann::json response = ProtocolHandler::handle_ModifyDiary(json, payload, db);
                TcpServer::sendJsonResponse(clientSocket, response.dump());
                LINE
            }

            else if (protocol == u8"GET_CALENDAR") {
                LINE_LABEL("SEND_CALENDAR_REQ")
                db.connect();
                nlohmann::json response = ProtocolHandler::handle_Calendar(json, db);
                TcpServer::sendJsonResponse(clientSocket, response.dump());
                LINE
            }



            //else if (protocol == u8"GEN_DIARY") {
            //   

            //    LINE_LABEL("GEN_DIARY")
            //        cout << u8"[GEN_DIARY] 요청:\n" << json.dump(2) << endl;

            //    try {
            //        // 로컬 테스트 데이터
            //        string child_name = "전인우";
            //        int child_uid = 1001;
            //        string embedding_data = "[0.10536525398492813,0.0,0.0,0.0,0.0,0.0,0.0,0.14744506776332855,0.004937579855322838,0.17479261755943298,0.06373615562915802,0.08198851346969604,0.03966839984059334,0.025140749290585518,0.0,0.0,0.11556734889745712,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.018912233412265778,0.0,0.0,0.050996165722608566,0.0,0.08332962542772293,0.06788799166679382,0.11879827082157135,0.013113993220031261,0.0532664991915226,0.05336317420005798,0.0,0.0,0.0,0.09670987725257874,0.010388411581516266,0.09378775954246521,0.06565140932798386,0.0,0.09055177867412567,0.0,0.1321539431810379,0.0,0.0069200024008750916,0.0,0.0,0.0,0.0,0.015766184777021408,0.0,0.008977294899523258,0.0,0.11799708753824234,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0767320841550827,0.0,0.0,0.0,0.0,0.06016342714428902,0.0,0.06941312551498413,0.0,0.0,0.0,0.056130532175302505,0.0,0.0,0.07815210521221161,0.0,0.0,0.015127808786928654,0.12008466571569443,0.13539718091487885,0.12517991662025452,0.06313008815050125,0.0,0.09056569635868073,0.0,0.0,0.0,0.0,0.053303103893995285,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.010747555643320084,0.0,0.09393686056137085,0.0,0.05816495046019554,0.0,0.011810600757598877,0.08441205322742462,0.0,0.02782626636326313,0.04225461557507515,0.046907346695661545,0.03502822667360306,0.0,0.048499107360839844,0.0,0.052725281566381454,0.0,0.0,0.056104060262441635,0.17207135260105133,0.059082791209220886,0.0,0.09805073589086533,0.0,0.1121729239821434,0.027458028867840767,0.0,0.0,0.0,0.0,0.12860001623630524,0.0013112802989780903,0.0,0.07360716909170151,0.0,0.12624073028564453,0.0,0.0,0.0,0.0,0.0,0.09217185527086258,0.0,0.0,0.0,0.09635742753744125,0.0,0.15457244217395782,0.08135044574737549,0.0,0.0,0.07077725231647491,0.05613444000482559,0.0,0.029485011473298073,0.0,0.14895856380462646,0.14272215962409973,0.0,0.0,0.004072296433150768,0.0486181378364563,0.028519731014966965,0.06104172766208649,0.05260615423321724,0.08927661925554276,0.08164189010858536,0.0,0.12847894430160522,0.03976507857441902,0.03530411794781685,0.0,0.005394948646426201,0.025604968890547752,0.0974198505282402,0.13515551388263702,0.03497400879859924,0.002324869157746434,0.0,0.0,0.07337424159049988,0.0,0.0,0.0,0.05793168023228645,0.05520328879356384,0.05291927978396416,0.06275950372219086,0.007361725438386202,0.0766606256365776,0.14770032465457916,0.0,0.11581330746412277,0.0,0.06596610695123672,0.09065289050340652,0.06525104492902756,0.03704148530960083,0.059521496295928955,0.0,0.0,0.0,0.0,0.177495539188385,0.071068674325943,0.0,0.0,0.0,0.0,0.0859004482626915,0.05506306514143944,0.0,0.15635564923286438,0.0,0.05706821009516716,0.1485048234462738,0.0013909117551520467,0.04134749621152878,0.0,0.0,0.0,0.0,0.04491094872355461,0.0,0.18181727826595306,0.0,0.0,0.0807722955942154,0.03402213007211685,0.0,0.0,0.0,0.1514233946800232,0.007192510180175304,0.04689515382051468,0.08952917158603668,0.07451057434082031,0.15092289447784424,0.025175033137202263,0.0,0.05732634291052818,0.273547500371933,0.10064282268285751,0.14913064241409302,0.06763939559459686,0.1062425822019577,0.048018503934144974,0.0,0.0,0.10734295099973679,0.0,0.0,0.0]";

            //        std::string audio_path = "C:/data/diary_inwoo.wav";

            //        cout << u8"[GEN_DIARY] 테스트 데이터 사용 - child_name: " << child_name << ", child_uid: " << child_uid << endl;
            //        cout << u8"[GEN_DIARY] 오디오 파일: " << audio_path << endl;

            //        // 파일 존재 확인
            //        std::ifstream test_file(audio_path, std::ios::binary);
            //        if (!test_file.is_open()) {
            //            cout << u8"[GEN_DIARY] 경고: 오디오 파일이 존재하지 않음" << endl;
            //        }
            //        else {
            //            test_file.close();
            //            cout << u8"[GEN_DIARY] 오디오 파일 확인 완료" << endl;
            //        }

            //        // Python 서버 연결 테스트 먼저
            //        cout << u8"[GEN_DIARY] Python 서버 연결 테스트 중..." << endl;

            //        CURL* curl = curl_easy_init();
            //        if (!curl) {
            //            cout << u8"[GEN_DIARY] CURL 초기화 실패" << endl;
            //            TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"CURL_INIT_FAIL"})");
            //            LINE
            //                return;
            //        }

            //        std::string response_str;

            //        // 더 짧은 타임아웃으로 연결 테스트
            //        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/");
            //        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);        // 5초 타임아웃
            //        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3L);  // 3초 연결 타임아웃
            //        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);          // HEAD 요청만

            //        CURLcode test_res = curl_easy_perform(curl);
            //        curl_easy_cleanup(curl);

            //        if (test_res != CURLE_OK) {
            //            cout << u8"[GEN_DIARY] Python 서버 연결 실패: " << curl_easy_strerror(test_res) << endl;
            //            cout << u8"[GEN_DIARY] Python 서버가 실행 중인지 확인하세요 (localhost:8000)" << endl;
            //            TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"PYTHON_SERVER_NOT_RUNNING"})");
            //            LINE
            //                return;
            //        }

            //        cout << u8"[GEN_DIARY] Python 서버 연결 테스트 성공" << endl;

            //        // 실제 요청 전송
            //        curl = curl_easy_init();
            //        if (curl) {
            //            struct curl_httppost* formpost = nullptr;
            //            struct curl_httppost* lastptr = nullptr;


            //            //
            //            std::ifstream f(audio_path, std::ios::binary);
            //            std::ostringstream oss;
            //            oss << f.rdbuf();
            //            std::string audio_data = oss.str();

            //            // Form 데이터 구성
            //            curl_formadd(&formpost, &lastptr,
            //                CURLFORM_COPYNAME, "child_name",
            //                CURLFORM_COPYCONTENTS, child_name.c_str(),
            //                CURLFORM_END);

            //            curl_formadd(&formpost, &lastptr,
            //                CURLFORM_COPYNAME, "file",
            //                CURLFORM_FILE, audio_path.c_str(),  // ✅ 반드시 FILE로 전송
            //                CURLFORM_FILENAME, "audio.wav",
            //                CURLFORM_CONTENTTYPE, "audio/wav",
            //                CURLFORM_END);
            //            /*curl_formadd(&formpost, &lastptr,
            //                CURLFORM_COPYNAME, "file",
            //                CURLFORM_FILE, audio_path.c_str(),
            //                CURLFORM_FILENAME, "audio.wav",
            //                CURLFORM_CONTENTTYPE, "audio/wav",
            //                CURLFORM_END);*/

            //            curl_formadd(&formpost, &lastptr,
            //                CURLFORM_COPYNAME, "embedding_file",
            //                CURLFORM_COPYCONTENTS, embedding_data.c_str(),
            //                CURLFORM_CONTENTTYPE, "application/json",
            //                CURLFORM_END);



            //            curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/transcribe");
            //            curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
            //            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);        // 5분으로 늘림
            //            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L); // 10초 연결 타임아웃
            //            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);         // 디버그 정보 출력

            //            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, [](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
            //                if (!contents || !userp) {
            //                    fprintf(stderr, "[GEN_DIARY] 콜백 오류: contents 또는 userp null\n");
            //                    return 0;
            //                }


            //                size_t totalSize = size * nmemb;
            //                std::string* str = static_cast<std::string*>(userp);
            //                try {
            //                    str->append(static_cast<char*>(contents), totalSize);
            //                    fprintf(stderr, "[GEN_DIARY] 콜백: %zu bytes 추가\n", totalSize);
            //                    return totalSize;
            //                }
            //                catch (const std::exception& e) {
            //                    fprintf(stderr, "[GEN_DIARY] 콜백 예외: %s\n", e.what());
            //                    return CURL_WRITEFUNC_ERROR;
            //                }
            //                });




            //            cout << u8"[GEN_DIARY] 실제 요청 전송 중... (최대 300초 대기)" << endl;
            //            CURLcode res = curl_easy_perform(curl);


            //            cout << "[GEN_DIARY] CURL 리턴 코드: " << res << " (" << curl_easy_strerror(res) << ")" << endl;

            //            if (res == CURLE_OK) {

            //                cout << u8"[GEN_DIARY] 서버 응답 (길이: " << response_str.length() << "): " << response_str.substr(0, 100) << "..." << endl;
            //                try {

            //                    nlohmann::json py_response = nlohmann::json::parse(response_str);
            //                    cout << u8"[GEN_DIARY] 응답 파싱 성공. 내용:\n" << py_response.dump(2) << endl;

            //                    nlohmann::json response;
            //                    response["PROTOCOL"] = "GEN_DIARY";
            //                    response["RESP"] = "SUCCESS";
            //                    response["DIARY_DATA"] = py_response;
            //                    TcpServer::sendJsonResponse(clientSocket, response.dump());

            //                }
            //                catch (const nlohmann::json::parse_error& e) {
            //                    cout << u8"[GEN_DIARY] JSON 파싱 오류: " << e.what() << endl;
            //                    cout << u8"[GEN_DIARY] 응답 원문: " << response_str.substr(0, 100) << "..." << endl;
            //                    TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"JSON_PARSE_ERROR"})");

            //                }
            //            }
            //            else {
            //                cout << u8"[GEN_DIARY] 요청 실패: " << curl_easy_strerror(res) << endl;
            //                TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"REQUEST_FAILED"})");
            //            }
            //            curl_formfree(formpost);
            //            curl_easy_cleanup(curl);
            //        }
            //        //
            //        else {
            //            cout << u8"[GEN_DIARY] CURL 초기화 실패" << endl;
            //            TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"CURL_INIT_FAIL"})");
            //        }
            //        // 임시 파일 정리
            //      /*  if (std::remove(audio_path.c_str()) == 0) {
            //            cout << u8"[GEN_DIARY] 임시 오디오 파일 삭제: " << audio_path << endl;
            //        }
            //        else {
            //            cout << u8"[GEN_DIARY] 임시 오디오 파일 삭제 실패: " << audio_path << endl;
            //        }*/

            //    }
            //    catch (const std::exception& e) {
            //        cout << u8"[GEN_DIARY] 예외 발생" << endl;
            //        TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"EXCEPTION"})");
            //    }
            //    catch (...) {
            //        cout << u8"[GEN_DIARY] 알 수 없는 예외 발생" << endl;
            //        TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"GEN_DIARY","RESP":"FAIL","MESSAGE":"UNKNOWN_EXCEPTION"})");
            //    }
            //    curl_global_cleanup();
            //    LINE
            //    }





            //else if (protocol == u8"REQ_GALLARY") {
            //    LINE_LABEL("REQ_GALLARY")
            //    db.connect();
            //    nlohmann::json response = ProtocolHandler::handle_ReqGallary(json, db, clientSocket);
            //    TcpServer::sendJsonResponse(clientSocket, response.dump());
            //    LINE

            //}

            else {
                cerr << u8"[에러] 알 수 없는 프로토콜: " << protocol << endl;
                TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"UNKNOWN","RESP":"FAIL","MESSAGE":"UNSUPPORTED_PROTOCOL"})");
            }
        }
        catch (const nlohmann::json::parse_error& e) {
            cerr << u8"[에러] JSON 파싱 실패 (parse_error): " << e.what() << endl;
        }
        catch (const nlohmann::json::type_error& e) {
            cerr << u8"[에러] JSON 타입 오류 (type_error): " << e.what() << endl;
        }
        catch (const exception& e) {
            cerr << u8"[에러] 기타 예외: " << e.what() << endl;
        }
    }
}

// =======================================================================
// [main 함수] 
// - TcpServer 객체를 생성하고, 다중 클라이언트를 비동기적으로 처리
// =======================================================================

int main() {
    // 콘솔 한글 깨짐 방지 설정
    SetConsoleOutputCP(CP_UTF8);                // UTF-8 출력 인코딩 설정
    TcpServer server(5556);                     // 포트 5556으로 서버 시작
    DBManager db;

    // DB 연결 시도
    if (!db.connect()) {
        cout << u8"[서버] DB 연결 실패!" << endl;
        return 1;
    }

    // 서버 바인딩 및 리슨
    if (!server.start()) {
        cout << u8"[서버] 소켓 바인딩 또는 리슨 실패!" << endl;
        return 1;
    }

    cout << u8"[서버] 클라이언트 수신 대기 시작..." << endl;

    // 클라이언트 수신 루프
    while (true) {
        SOCKET clientSocket = server.acceptClient();

        if (clientSocket != INVALID_SOCKET) {
            cout << u8"[서버] 새 클라이언트 연결됨!" << endl;
            thread th(handleClient, clientSocket);
            th.detach();  // 독립적으로 실행
        }
    }
    return 0;
}
