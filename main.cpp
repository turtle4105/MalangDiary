#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include "HttpClient.hpp"

// ================== 설정 구역 ==================
// 아이 이름 설정 - 필요시 여기서 수정하세요
const std::string CHILD_NAME = "전인우";

// 서버 URL 설정
const std::string SERVER_URL = "http://localhost:8000/transcribe";

// 오디오 파일 경로 설정
const std::string AUDIO_FILE_PATH = "../output.wav";
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
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

int main() {
    std::cout << "=== 음성 일기 생성 클라이언트 ===" << std::endl;
    std::cout << "설정된 아이 이름: " << CHILD_NAME << std::endl;
    std::cout << "오디오 파일: " << AUDIO_FILE_PATH << std::endl;
    std::cout << "서버 URL: " << SERVER_URL << std::endl;
    
    // URL에 아이 이름을 Query Parameter로 추가 (URL 인코딩 적용)
    std::string url_with_params;
    
    if (!CHILD_NAME.empty()) {
        std::string encoded_name = url_encode(CHILD_NAME);
        url_with_params = SERVER_URL + "?child_name=" + encoded_name;
        std::cout << "요청 URL: " << url_with_params << std::endl;
        std::cout << "아이 이름 '" << CHILD_NAME << "'로 음성 분석을 시작합니다..." << std::endl;
    } else {
        url_with_params = SERVER_URL;
        std::cout << "아이 이름 없이 요청합니다." << std::endl;
    }
    
    HttpClient client;
    std::string response = client.sendFile(AUDIO_FILE_PATH, url_with_params);

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
