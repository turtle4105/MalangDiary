#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include "HttpClient.hpp"
#include <filesystem>

// ================== 설정 구역 ==================

// 서버 URL 설정
const std::string SERVER_URL = "http://localhost:8000/transcribe";

// ==============================================

// URL 인코딩 함수 (한글 이름 처리용)
std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        // ASCII 알파벳, 숫자, 일부 특수문자는 그대로 유지
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }

        // 나머지는 퍼센트 인코딩
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char)c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

// 음성 일기 생성 함수
std::string generateVoiceDiary(const std::string& childName,
    const std::string& audioFilePath,
    const std::string& embedding_json) {
    std::cout << "=== 음성 일기 생성 클라이언트 ===" << std::endl;
    std::cout << "설정된 아이 이름: " << childName << std::endl;
    std::cout << "오디오 파일: " << audioFilePath << std::endl;
    std::cout << "임베딩 파일: " << embedding_json << std::endl;
    std::cout << "서버 URL: " << SERVER_URL << std::endl;

    HttpClient client;
    std::cout << "아이 이름 '" << childName << "'로 음성 분석을 시작합니다..." << std::endl;
    std::string response = client.sendMultipleFiles(childName, audioFilePath, embedding_json, SERVER_URL);

    return response;
}

int main() {


    // 기본값으로 테스트
    std::string childName = "전인우";
    std::string audioFilePath = "../../data/inwoo_diary.wav";
    //std::string embedding_json = "./test_embedding.txt"; 
    std::string embedding_json = "[0.11101, 0, 0, 0, 0.111]"; // 문자열 데이터 직접 사용

    std::string response = generateVoiceDiary(childName, audioFilePath, embedding_json);

    // 1) 콘솔 출력
    std::cout << "\n=== 서버 응답 ===" << std::endl;
    std::cout << response << std::endl;

    // 2) 파일로 저장
    std::ofstream outFile("result.json");
    if (outFile.is_open()) {
        outFile << response;
        outFile.close();
        std::cout << "\n=== 결과 저장 완료 ===" << std::endl;
        std::cout << "결과가 result.json 파일에 저장되었습니다." << std::endl;
    }
    else {
        std::cerr << "ERROR: result.json 파일 저장에 실패했습니다." << std::endl;
    }

    return 0;
}
