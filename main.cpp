﻿#define WIN32_LEAN_AND_MEAN     // 윈도우 헤더 경량화 (불필요한 macro 제거)
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
                ProtocolHandler::handle_SendDiaryDetail(json, db, clientSocket); 
                //TcpServer::sendJsonResponse(clientSocket, response.dump());
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
    CURLcode global_res = curl_global_init(CURL_GLOBAL_ALL);
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
    curl_global_cleanup();
    return 0;
}
