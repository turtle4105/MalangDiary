#include "HttpClient.hpp"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

// libcurl 응답 데이터를 받기 위한 콜백 함수
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = reinterpret_cast<std::string*>(userp);
    response->append(reinterpret_cast<char*>(contents), totalSize);
    return totalSize;
}

// HttpClient 생성자: libcurl 전역 초기화 수행
HttpClient::HttpClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

// HttpClient 소멸자: libcurl 전역 정리 수행
HttpClient::~HttpClient() {
    curl_global_cleanup();
}

// 파일을 multipart/form-data로 POST 전송하는 함수
std::string HttpClient::sendFile(const std::string& filePath, const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        struct curl_httppost* formpost = NULL;
        struct curl_httppost* lastptr = NULL;

        // 파일 필드 추가: MIME 타입 반드시 지정!!
        curl_formadd(&formpost, &lastptr,
            CURLFORM_COPYNAME, "file",
            CURLFORM_FILE, filePath.c_str(),
            CURLFORM_FILENAME, "output.wav",           // 서버에 전달할 파일명 (원하는 이름으로)
            CURLFORM_CONTENTTYPE, "audio/wav",          // 중요: MIME 타입 명시
            CURLFORM_END);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        curl_formfree(formpost);
        curl_easy_cleanup(curl);
    }
    else {
        std::cerr << "curl_easy_init() failed" << std::endl;
    }

    return response;
}
