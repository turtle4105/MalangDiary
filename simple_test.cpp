#include <iostream>
#include <string>
#include "HttpClient.hpp"

int main() {
    std::cout << "=== 문자열 임베딩 테스트 ===" << std::endl;
    
    HttpClient client;
    std::string childName = "전인우";
    std::string audioFilePath = "./data/inwoo_diary.wav";
    std::string embedding_data = "[0.11101, 0, 0, 0, 0.111]"; // 문자열 데이터 직접 사용
    std::string serverUrl = "http://localhost:8000/transcribe";
    
    std::cout << "아이 이름: " << childName << std::endl;
    std::cout << "오디오 파일: " << audioFilePath << std::endl;
    std::cout << "임베딩 데이터: " << embedding_data << std::endl;
    std::cout << "서버 URL: " << serverUrl << std::endl;
    
    std::cout << "\n서버 요청 시작..." << std::endl;
    std::string response = client.sendMultipleFiles(childName, audioFilePath, embedding_data, serverUrl);
    
    std::cout << "\n=== 서버 응답 ===" << std::endl;
    std::cout << response << std::endl;
    
    return 0;
}