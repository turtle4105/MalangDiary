#include "HttpClient.hpp"
#include <curl/curl.h>
#include <iostream>
#include <sstream>


static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = reinterpret_cast<std::string*>(userp);
    response->append(reinterpret_cast<char*>(contents), totalSize);
    return totalSize;
}


HttpClient::HttpClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}


HttpClient::~HttpClient() {
    curl_global_cleanup();
}


std::string HttpClient::sendFile(const std::string& filePath, const std::string& url) {
    CURL* curl = curl_easy_init();
    std::string response;

    if (curl) {
        struct curl_httppost* formpost = NULL;
        struct curl_httppost* lastptr = NULL;

        
        curl_formadd(&formpost, &lastptr,
            CURLFORM_COPYNAME, "file",
            CURLFORM_FILE, filePath.c_str(),
            CURLFORM_FILENAME, "output.wav",           // ������ ������ ���ϸ� (���ϴ� �̸�����)
            CURLFORM_CONTENTTYPE, "audio/wav",          // �߿�: MIME Ÿ�� ����
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
