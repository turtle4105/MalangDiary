#include <iostream>
#include <fstream>
#include <string>
#include "HttpClient.hpp"

// 음성 일기 생성 함수 (main.cpp에서 복사)
std::string generateVoiceDiary(const std::string& childName, 
                              const std::string& audioFilePath, 
                              const std::string& embeddingFilePath) {
    std::cout << "=== 음성 일기 생성 클라이언트 테스트 ===" << std::endl;
    std::cout << "설정된 아이 이름: " << childName << std::endl;
    std::cout << "오디오 파일: " << audioFilePath << std::endl;
    std::cout << "임베딩 파일: " << embeddingFilePath << std::endl;
    
    const std::string SERVER_URL = "http://localhost:8000/transcribe";
    std::cout << "서버 URL: " << SERVER_URL << std::endl;
    
    HttpClient client;
    std::cout << "아이 이름 '" << childName << "'로 음성 분석을 시작합니다..." << std::endl;
    std::string response = client.sendMultipleFiles(childName, audioFilePath, embeddingFilePath, SERVER_URL);
    
    return response;
}

int main() {
    std::cout << "=== 테스트 매개변수 설정 ===" << std::endl;
    
    // 테스트 매개변수
    std::string childName = "전인우";
    std::string audioFilePath = "data/inwoo_diary.wav";
    std::string embeddingFilePath = "data/embedding_inwoo.json";
    
    std::cout << "테스트 매개변수:" << std::endl;
    std::cout << "1. 아이 이름: " << childName << std::endl;
    std::cout << "2. 오디오 파일: " << audioFilePath << std::endl;
    std::cout << "3. 임베딩 파일: " << embeddingFilePath << std::endl;
    std::cout << std::endl;
    
    // 파일 존재 확인
    std::cout << "=== 파일 존재 확인 ===" << std::endl;
    std::ifstream audioCheck(audioFilePath);
    if (audioCheck.good()) {
        std::cout << "✓ 오디오 파일 존재: " << audioFilePath << std::endl;
    } else {
        std::cout << "✗ 오디오 파일 없음: " << audioFilePath << std::endl;
        std::cout << "테스트를 계속 진행합니다 (서버에서 오류 처리 확인용)" << std::endl;
    }
    audioCheck.close();
    
    std::ifstream embeddingCheck(embeddingFilePath);
    if (embeddingCheck.good()) {
        std::cout << "✓ 임베딩 파일 존재: " << embeddingFilePath << std::endl;
    } else {
        std::cout << "✗ 임베딩 파일 없음: " << embeddingFilePath << std::endl;
        std::cout << "테스트를 계속 진행합니다 (서버에서 오류 처리 확인용)" << std::endl;
    }
    embeddingCheck.close();
    std::cout << std::endl;
    
    // 함수 호출 테스트
    try {
        std::string response = generateVoiceDiary(childName, audioFilePath, embeddingFilePath);
        
        // 결과 출력
        std::cout << "\n=== 서버 응답 ===" << std::endl;
        std::cout << response << std::endl;
        
        // 결과를 파일로 저장
        std::ofstream outFile("test_result.json");
        if (outFile.is_open()) {
            outFile << response;
            outFile.close();
            std::cout << "\n=== 테스트 결과 저장 완료 ===" << std::endl;
            std::cout << "결과가 test_result.json 파일에 저장되었습니다." << std::endl;
        } else {
            std::cerr << "ERROR: test_result.json 파일 저장에 실패했습니다." << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "테스트 중 예외 발생: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n=== 테스트 완료 ===" << std::endl;
    return 0;
}