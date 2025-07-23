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
