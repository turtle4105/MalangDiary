#include <iostream>
#include <fstream>
#include "HttpClient.hpp"



int main() {
    HttpClient client;
    std::string response = client.sendFile("../output.wav", "http://localhost:8000/transcribe");

    // 1) �ܼ� ���
    std::cout << "Response from server:\n" << response << std::endl;

    // 2) ���Ϸ� ����
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
