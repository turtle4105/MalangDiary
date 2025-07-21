#define WIN32_LEAN_AND_MEAN     // ������ ��� �淮ȭ (���ʿ��� macro ����)
#include "TcpServer.h"          // TCP ���� Ŭ���� ���
#include "DBManager.h"          // DB ���� Ŭ���� ���
#include "ProtocolHandler.h"    // Protocol �Ľ� Ŭ���� ��� 
#include <iostream>
#include <thread>
#include <string>
#include <codecvt> 
#include <locale>
#include <nlohmann/json.hpp>
#include <fcntl.h>     // _O_U8TEXT
#include <io.h>        // _setmode, _fileno
#define LINE_LABEL(label) cout << "=======================[" << label << "] =======================\n";
#define LINE cout << "=====================================================\n";

using json = nlohmann::json;
using namespace std;

// UTF-8 -> UTF-16 ��ȯ �� ���
static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

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

    //std::cout << u8"[DEBUG] handleClient ����\n";

    DBManager db;
    std::string jsonStr;
    std::vector<char> payload;

    // [1] JSON + payload ���� (��� ����)
    if (!TcpServer::receivePacket(clientSocket, jsonStr, payload)) {
        std::cerr << u8"[��Ŷ ���� ����] JSON ���� ����\n";
        TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"UNKNOWN","RESP":"FAIL","MESSAGE":"RECEIVE_FAIL"})");
        closesocket(clientSocket);
        return;
    }

  /* [2] ���� �� UTF - 8 ���͸� �߰�(0xC0, BOM �� ����� ����)
    //while (!jsonStr.empty() &&
    //    ((unsigned char)jsonStr[0] == 0xC0 ||
    //        (unsigned char)jsonStr[0] == 0xEF)) {
    //    if ((unsigned char)jsonStr[0] == 0xC0) {
    //        std::cerr << "[DEBUG] handleClient���� 0xC0 ����\n";
    //        jsonStr = jsonStr.substr(1);
    //    }
    //    else if (jsonStr.size() >= 3 &&
    //        (unsigned char)jsonStr[0] == 0xEF &&
    //        (unsigned char)jsonStr[1] == 0xBB &&
    //        (unsigned char)jsonStr[2] == 0xBF) {
    //        std::cerr << "[DEBUG] handleClient���� BOM ����\n";
    //        jsonStr = jsonStr.substr(3);
    //    }
    //    else {
    //        break;
    //    }
    //}
    */
    // [2] JSON ��ȿ�� �˻�
    if (!nlohmann::json::accept(jsonStr)) {
        std::cerr << u8"[JSON ���� ����] ��ȿ���� ���� JSON�Դϴ�.\n";
        TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"UNKNOWN","RESP":"FAIL","MESSAGE":"INVALID_JSON"})");
        closesocket(clientSocket);
        return;
    }

    try {
        //std::cout << u8"[STEP 1] JSON �Ľ� ����" << std::endl;
        nlohmann::json json = nlohmann::json::parse(jsonStr);
        //std::cout << u8"[STEP 1-OK] JSON �Ľ� ����" << std::endl;

        std::string protocol = json.value("PROTOCOL", "");
        //std::cout << u8"[STEP 2] ��������: " << protocol << std::endl;

        if (protocol == u8"REGISTER_PARENTS") {
            LINE_LABEL("REGISTER_PARENTS")
            db.connect();

            //if (!db.connect()) {
            //    TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"REGISTER_PARENTS","RESP":"FAIL","MESSAGE":"DB_CONNECT_FAIL"})");
            //    closesocket(clientSocket);
            //    return;
            //}
            //cout << u8"[STEP 3-OK] DB ���� ����" << endl;

            //cout << u8"[STEP 4] ProtocolHandler ����" << endl;
            nlohmann::json response = ProtocolHandler::handle_RegisterParents(json, db);
            //cout << u8"[STEP 4-OK] ���� JSON ������:\n" << response.dump(2) << endl;

            //cout << u8"[STEP 5] ���� ���� ����" << endl;
            TcpServer::sendJsonResponse(clientSocket, response.dump());
            //cout << u8"[STEP 5-OK] ���� ���� �Ϸ�" << endl;
            LINE
        }

    else if (protocol == u8"LOGIN") {
        LINE_LABEL("LOGIN")
        db.connect();

        nlohmann::json response = ProtocolHandler::handle_Login(json, db);
        TcpServer::sendJsonResponse(clientSocket, response.dump());
        LINE
    }

        //else if (protocol == u8"REGISTER_CHILD") {
        //    cout << u8"[���� ��� ��û]" << endl;
        //    db.connect();

        //    nlohmann::json response = ProtocolHandler::handle_RegisterChild(json, db);


        //}

        else {
            std::cerr << u8"[����] �� �� ���� ��������: " << protocol << std::endl;
            TcpServer::sendJsonResponse(clientSocket, R"({"PROTOCOL":"UNKNOWN","RESP":"FAIL","MESSAGE":"UNSUPPORTED_PROTOCOL"})");
        }

    }
    catch (const nlohmann::json::parse_error& e) {
        std::cerr << u8"[����] JSON �Ľ� ���� (parse_error): " << e.what() << std::endl;
    }
    catch (const nlohmann::json::type_error& e) {
        std::cerr << u8"[����] JSON Ÿ�� ���� (type_error): " << e.what() << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << u8"[����] ��Ÿ ����: " << e.what() << std::endl;
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
