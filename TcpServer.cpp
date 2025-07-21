// TcpServer.cpp
#include "TcpServer.h"
#include <fstream>


// ��ũ: ���� ���̺귯��
#pragma comment(lib, "Ws2_32.lib")

// =======================================================================
// [������] TcpServer
// - ��Ʈ ��ȣ�� �޾Ƽ� ���� ���� �ʱⰪ ����
// =======================================================================
TcpServer::TcpServer(int port)
    : port_(port), listenSocket_(INVALID_SOCKET), wsaData_{} {
}

// =======================================================================
// [�Ҹ���] TcpServer
// - ������ ������ ��ȿ�ϸ� �ݰ�, Winsock ����
// =======================================================================
TcpServer::~TcpServer() {
    if (listenSocket_ != INVALID_SOCKET) closesocket(listenSocket_);
    WSACleanup();
}

// =======================================================================
// [start] - ���� ������ ���� ���� ���� �ܰ� (1)
// - ���� �ʱ�ȭ (Winsock ����, ���� ����, ���ε�, ��������)
// - ���� �� true, ���� �� false ��ȯ
// =======================================================================
bool TcpServer::start() {
    // Winsock ���̺귯�� �ʱ�ȭ
    if (WSAStartup(MAKEWORD(2, 2), &wsaData_) != 0) {
        cerr << "WSAStartup ����\n";
        return false;
    }

    // ���� ���� ���� (IPv4, TCP)
    listenSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket_ == INVALID_SOCKET) {
        cerr << "���� ���� ����\n";
        return false;
    }

    // ���� �ּ� ����ü ����
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;    // ��� IP���� ���� ���
    serverAddr.sin_port = htons(port_);         // ��Ʈ ����

    // ���� ���ε� (�ּҿ� ����)
    if (::bind(listenSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "���ε� ����\n";
        closesocket(listenSocket_);
        return false;
    }

    // ���� ���� ���� (Ŭ���̾�Ʈ ���� ���)
    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "������ ����\n";
        closesocket(listenSocket_);
        return false;
    }

    cout << u8"������ ��Ʈ " << port_ << u8"���� ��� ���Դϴ�...\n";
    return true;
}

// =======================================================================
// [acceptClient] - Ŭ���̾�Ʈ ���� ���� �ܰ� (2)
// - Ŭ���̾�Ʈ�� ���� ��û�� �����ϰ�, Ŭ���̾�Ʈ ���� ��ȯ
// - ���� �� INVALID_SOCKET ��ȯ
// =======================================================================
SOCKET TcpServer::acceptClient() {
    sockaddr_in clientAddr{};
    int clientSize = sizeof(clientAddr);

    // Ŭ���̾�Ʈ ���� (���� ��û �޾Ƶ��̱�)
    SOCKET clientSocket = accept(listenSocket_, (sockaddr*)&clientAddr, &clientSize);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Ŭ���̾�Ʈ ���� ����\n";
    }
    //cout << "[Ŭ���̾�Ʈ] ���� ����\n";

    return clientSocket;
}


// =======================================================================
// [readExact]
// - �־��� ����Ʈ ��(totalBytes)��ŭ ��Ȯ�� ���� ������ �ݺ� ����
// - recv()�� TCP ��Ʈ�� Ư���� �Ϻθ� ���ŵ� �� �����Ƿ� �ݵ�� �ݺ� ȣ��
// - �����ϰų� ������ ����� false ��ȯ
// =======================================================================
bool TcpServer::readExact(SOCKET sock, char* buffer, int totalBytes) {
    int received = 0;
    while (received < totalBytes) {
        int len = recv(sock, buffer + received, totalBytes - received, 0);
        cout << "[readExact] recv returned: " << len << "\n";
        if (len <= 0) return false;  // ���� ���� or ����
        received += len;
    }
    return true;
}

// =======================================================================
// [receivePacket]
// - Ŭ���̾�Ʈ�κ��� ���� ������ ����:
//   [1] 8����Ʈ ���: totalSize (4����Ʈ) + jsonSize (4����Ʈ)
//   [2] JSON ���ڿ� (jsonSize ����Ʈ)
//   [3] payload (totalSize - jsonSize ����Ʈ, ���� ���)
//
// - ���� �����͸� out_json, out_payload�� �и��ؼ� ��ȯ
// =======================================================================
bool TcpServer::receivePacket(SOCKET clientSocket, string& out_json, vector<char>& out_payload) {
    //cout << u8"[DEBUG] receivePacket ����\n";

    // 1. 8����Ʈ ��� ����
    char header[8];
    if (!readExact(clientSocket, header, 8)) {
        cerr << u8"[ERROR] ��� ���� ����\n";
        closesocket(clientSocket); //  �ݵ�� ���� �ݾƾ� ��
        return false;
    }

    // 2. ��� �Ľ�
    uint32_t totalSize = 0, jsonSize = 0;
    memcpy(&totalSize, header, 4);
    memcpy(&jsonSize, header + 4, 4);

    cout << u8"[DEBUG] totalSize: " << totalSize << u8", jsonSize: " << jsonSize << "\n";

    // 3. ��� ��ȿ�� ����
    if (jsonSize == 0 || jsonSize > totalSize || totalSize > 10 * 1024 * 1024) {
        cerr << u8"[ERROR] ��� ���� ������: jsonSize=" << jsonSize << u8", totalSize=" << totalSize << "\n";
        return false;
    }

    // 4. �ٵ� ����
    vector<char> buffer(totalSize);
    if (!readExact(clientSocket, buffer.data(), totalSize)) {
        cerr << "[ERROR] �ٵ� ���� ����\n";
        return false;
    }

// 5. JSON �и� �� BOM ����
    try {
        out_json = string(buffer.begin(), buffer.begin() + jsonSize);

        //// ���� Ȯ��
        //cout << u8"[DEBUG] JSON ù ����Ʈ HEX: ";
        //for (int i = 0; i < 10 && i < out_json.size(); ++i)
        //    printf("%02X ", (unsigned char)out_json[i]);
        //printf("\n");

        //// 1. BOM ����
        //if (out_json.size() >= 3 &&
        //    (unsigned char)out_json[0] == 0xEF &&
        //    (unsigned char)out_json[1] == 0xBB &&
        //    (unsigned char)out_json[2] == 0xBF) {
        //    out_json = out_json.substr(3);
        //}

        //cout << u8"[DEBUG] out_json ��ü HEX: ";
        //for (int i = 0; i < out_json.size(); ++i)
        //    printf("%02X ", (unsigned char)out_json[i]);
        //printf("\n");

        // 2. UTF-8 ������ ����Ʈ ����
        while (!out_json.empty() && ((unsigned char)out_json[0] == 0xC0 || (unsigned char)out_json[0] == 0xC1 || (unsigned char)out_json[0] < 0x20)) {
            cerr << "[���] JSON �տ� ������ ����Ʈ(0x" << hex << (int)(unsigned char)out_json[0] << ") �߰� �� ����\n";
            out_json = out_json.substr(1);
        }
    }
    catch (const exception& e) {
        cerr << u8"[ERROR] JSON ���ڿ� ó�� �� ���� �߻�: " << e.what() << "\n";
        return false;
    }

    // 6. payload �и�
    if (jsonSize < totalSize)
        out_payload.assign(buffer.begin() + jsonSize, buffer.end());
    else
        out_payload.clear();

    return true;
}

// =======================================================================
// [sendJsonResponse]
// - Ŭ���̾�Ʈ���� ���� json ����
// =======================================================================

void TcpServer::sendJsonResponse(SOCKET sock, const string& jsonStr) {
    uint32_t totalSize = static_cast<uint32_t>(jsonStr.size());
    uint32_t jsonSize = totalSize;

    char header[8];
    memcpy(header, &totalSize, 4);
    memcpy(header + 4, &jsonSize, 4);

    send(sock, header, 8, 0);
    send(sock, jsonStr.c_str(), jsonStr.size(), 0);
}
