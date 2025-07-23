// TcpServer.cpp
#include "TcpServer.h"
#include <fstream>


// 링크: 윈속 라이브러리
#pragma comment(lib, "Ws2_32.lib")

// =======================================================================
// [생성자] TcpServer
// - 포트 번호를 받아서 서버 소켓 초기값 설정
// =======================================================================
TcpServer::TcpServer(int port)
    : port_(port), listenSocket_(INVALID_SOCKET), wsaData_{} {
}

// =======================================================================
// [소멸자] TcpServer
// - 리스닝 소켓이 유효하면 닫고, Winsock 종료
// =======================================================================
TcpServer::~TcpServer() {
    if (listenSocket_ != INVALID_SOCKET) closesocket(listenSocket_);
    WSACleanup();
}

// =======================================================================
// [start] - 서버 오픈을 위한 소켓 생성 단계 (1)
// - 서버 초기화 (Winsock 시작, 소켓 생성, 바인딩, 리슨까지)
// - 성공 시 true, 실패 시 false 반환
// =======================================================================
bool TcpServer::start() {
    // Winsock 라이브러리 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsaData_) != 0) {
        cerr << "WSAStartup 실패\n";
        return false;
    }

    // 서버 소켓 생성 (IPv4, TCP)
    listenSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket_ == INVALID_SOCKET) {
        cerr << "소켓 생성 실패\n";
        return false;
    }

    // 서버 주소 구조체 설정
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;    // 모든 IP에서 접속 허용
    serverAddr.sin_port = htons(port_);         // 포트 설정

    // 소켓 바인딩 (주소와 연결)
    if (::bind(listenSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "바인딩 실패\n";
        closesocket(listenSocket_);
        return false;
    }

    // 리슨 상태 진입 (클라이언트 연결 대기)
    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "리스닝 실패\n";
        closesocket(listenSocket_);
        return false;
    }

    cout << u8"서버가 포트 " << port_ << u8"에서 대기 중입니다...\n";
    return true;
}

// =======================================================================
// [acceptClient] - 클라이언트 연결 감지 단계 (2)
// - 클라이언트의 연결 요청을 수락하고, 클라이언트 소켓 반환
// - 실패 시 INVALID_SOCKET 반환
// =======================================================================
SOCKET TcpServer::acceptClient() {
    sockaddr_in clientAddr{};
    int clientSize = sizeof(clientAddr);

    // 클라이언트 수락 (연결 요청 받아들이기)
    SOCKET clientSocket = accept(listenSocket_, (sockaddr*)&clientAddr, &clientSize);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "클라이언트 수락 실패\n";
    }
    //cout << "[클라이언트] 연결 성공\n";

    return clientSocket;
}


// =======================================================================
// [readExact]
// - 주어진 바이트 수(totalBytes)만큼 정확히 읽을 때까지 반복 수신
// - recv()는 TCP 스트림 특성상 일부만 수신될 수 있으므로 반드시 반복 호출
// - 실패하거나 연결이 끊기면 false 반환
// =======================================================================
bool TcpServer::readExact(SOCKET sock, char* buffer, int totalBytes) {
    int received = 0;
    while (received < totalBytes) {
        int len = recv(sock, buffer + received, totalBytes - received, 0);
        cout << "[readExact] recv returned: " << len << "\n";
        if (len <= 0) return false;  // 연결 끊김 or 오류
        received += len;
    }
    return true;
}

// =======================================================================
// [receivePacket]
// - 클라이언트로부터 다음 순서로 수신:
//   [1] 8바이트 헤더: totalSize (4바이트) + jsonSize (4바이트)
//   [2] JSON 문자열 (jsonSize 바이트)
//   [3] payload (totalSize - jsonSize 바이트, 있을 경우)
//
// - 받은 데이터를 out_json, out_payload로 분리해서 반환
// =======================================================================
bool TcpServer::receivePacket(SOCKET clientSocket, string& out_json, vector<char>& out_payload) {
    //cout << u8"[DEBUG] receivePacket 진입\n";

    // 1. 8바이트 헤더 수신
    char header[8];
    if (!readExact(clientSocket, header, 8)) {
        cerr << u8"[ERROR] 헤더 수신 실패\n";
        closesocket(clientSocket); //  반드시 소켓 닫아야 함
        return false;
    }

    // 2. 헤더 파싱
    uint32_t totalSize = 0, jsonSize = 0;
    memcpy(&totalSize, header, 4);
    memcpy(&jsonSize, header + 4, 4);

    cout << u8"[DEBUG] totalSize: " << totalSize << u8", jsonSize: " << jsonSize << "\n";

    // 3. 헤더 유효성 검증
    if (jsonSize == 0 || jsonSize > totalSize || totalSize > 10 * 1024 * 1024) {
        cerr << u8"[ERROR] 헤더 정보 비정상: jsonSize=" << jsonSize << u8", totalSize=" << totalSize << "\n";
        return false;
    }

    // 4. 바디 수신
    vector<char> buffer(totalSize);
    if (!readExact(clientSocket, buffer.data(), totalSize)) {
        cerr << "[ERROR] 바디 수신 실패\n";
        return false;
    }

// 5. JSON 분리 및 BOM 제거
    try {
        out_json = string(buffer.begin(), buffer.begin() + jsonSize);

        //// 덤프 확인
        //cout << u8"[DEBUG] JSON 첫 바이트 HEX: ";
        //for (int i = 0; i < 10 && i < out_json.size(); ++i)
        //    printf("%02X ", (unsigned char)out_json[i]);
        //printf("\n");

        //// 1. BOM 제거
        //if (out_json.size() >= 3 &&
        //    (unsigned char)out_json[0] == 0xEF &&
        //    (unsigned char)out_json[1] == 0xBB &&
        //    (unsigned char)out_json[2] == 0xBF) {
        //    out_json = out_json.substr(3);
        //}

        //cout << u8"[DEBUG] out_json 전체 HEX: ";
        //for (int i = 0; i < out_json.size(); ++i)
        //    printf("%02X ", (unsigned char)out_json[i]);
        //printf("\n");

        // 2. UTF-8 비정상 바이트 제거
        while (!out_json.empty() && ((unsigned char)out_json[0] == 0xC0 || (unsigned char)out_json[0] == 0xC1 || (unsigned char)out_json[0] < 0x20)) {
            cerr << "[경고] JSON 앞에 비정상 바이트(0x" << hex << (int)(unsigned char)out_json[0] << ") 발견 → 제거\n";
            out_json = out_json.substr(1);
        }
    }
    catch (const exception& e) {
        cerr << u8"[ERROR] JSON 문자열 처리 중 예외 발생: " << e.what() << "\n";
        return false;
    }

    // 6. payload 분리
    if (jsonSize < totalSize)
        out_payload.assign(buffer.begin() + jsonSize, buffer.end());
    else
        out_payload.clear();

    return true;
}

// =======================================================================
// [sendJsonResponse]
// - 클라이언트에게 보낼 json 생성
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

// =======================================================================
// [sendBinary]
// - 클라이언트에게 보낼 바이너리 파일 생성
// =======================================================================

//void TcpServer::sendBinary(SOCKET sock, const std::string& filePath) {
//    std::ifstream file(filePath, std::ios::binary);
//    if (!file.is_open()) {
//        std::cerr << u8"→ [sendBinary] 바이너리 파일 열기 실패: " << filePath << std::endl;
//        return;
//    }
//
//    std::vector<char> buffer((std::istreambuf_iterator<char>(file)),
//        std::istreambuf_iterator<char>());
//
//    send(sock, buffer.data(), buffer.size(), 0);
//    std::cout << u8"→ [sendBinary] 바이너리 전송 완료 (" << buffer.size() << " bytes)\n";
//}
//
// =======================================================================
// [sendPacket]
// - 클라이언트에게 보낼 바이너리 파일 생성
// =======================================================================
void TcpServer::sendPacket(SOCKET sock, const std::string& jsonStr, const std::vector<char>& payload) {
    uint32_t jsonSize = static_cast<uint32_t>(jsonStr.size());
    uint32_t payloadSize = static_cast<uint32_t>(payload.size());
    uint32_t totalSize = jsonSize + payloadSize;

    std::vector<char> packet(totalSize);
    packet.resize(8 + totalSize);  // 반드시 헤더 포함 크기

    // [0~3] 전체 크기
    memcpy(packet.data(), &totalSize, 4);

    // [4~7] JSON 크기
    memcpy(packet.data() + 4, &jsonSize, 4);

    // [8 ~ 8+jsonSize-1] JSON
    memcpy(packet.data() + 8, jsonStr.data(), jsonSize);

    // [8+jsonSize ~ end] payload
    if (payloadSize > 0) {
        memcpy(packet.data() + 8 + jsonSize, payload.data(), payloadSize);
    }

    //  send() 한번으로 전송
    send(sock, packet.data(), packet.size(), 0);

    //  디버그 출력
    std::cout << u8"\n===== [sendPacket] 패킷 전송 디버그 정보 =====\n";
    std::cout << "Total Size : " << totalSize << " bytes\n";
    std::cout << "JSON Size  : " << jsonSize << " bytes\n";
    std::cout << "Payload Size: " << payloadSize << " bytes\n";
    cout << u8" → [sendPacket]  전송 완료 ";
}
