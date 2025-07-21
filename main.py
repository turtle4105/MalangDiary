 #include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include "HttpClient.hpp"

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

int main(int argc, char* argv[]) {
    // 아이 이름 설정 (여러 방법 지원)
    std::string child_name;
    
    // 1. 커맨드라인 인수에서 아이 이름 받기
    if (argc > 1) {
        child_name = argv[1];
        std::cout << "커맨드라인에서 아이 이름 설정: " << child_name << std::endl;
    } else {
        // 2. 기본값 사용 (필요에 따라 수정)
        child_name = "인우";  // 기본 아이 이름
        std::cout << "기본 아이 이름 사용: " << child_name << std::endl;
        std::cout << "사용법: ./client [아이이름]" << std::endl;
    }
    
    // URL에 아이 이름을 Query Parameter로 추가 (URL 인코딩 적용)
    std::string base_url = "http://localhost:8000/transcribe";
    std::string url_with_params;
    
    if (!child_name.empty()) {
        std::string encoded_name = url_encode(child_name);
        url_with_params = base_url + "?child_name=" + encoded_name;
        std::cout << "아이 이름 '" << child_name << "' (인코딩: " << encoded_name << ")로 요청합니다." << std::endl;
    } else {
        url_with_params = base_url;
        std::cout << "아이 이름 없이 요청합니다." << std::endl;
    }
    
    HttpClient client;
    std::string response = client.sendFile("../output.wav", url_with_params);

    // 1) 콘솔 출력
    std::cout << "Response from server:\n" << response << std::endl;

    // 2) 파일로 저장
    std::ofstream outFile("result.json");
    if (outFile.is_open()) {
        outFile << response;
        outFile.close();
        std::cout << "Response saved to result.json\n";
    }
    else {
        std::cerr << "Failed to open result.json for writing\n";
    }

    return 0;
}
