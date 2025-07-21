#pragma once

#include <winsock2.h>
#include <string>
#include <vector>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace std;

class TcpServer {
public:
    TcpServer(int port);
    ~TcpServer();

    SOCKET acceptClient();

    bool start();
    static bool readExact(SOCKET sock, char* buffer, int totalBytes);
    static bool receivePacket(SOCKET clientSocket, string& out_json, vector<char>& out_payload);
    static void sendJsonResponse(SOCKET sock, const string& jsonStr);

private:
    int port_;
    SOCKET listenSocket_;
    WSADATA wsaData_;
};
