#ifndef HTTPCLIENT_HPP
#define HTTPCLIENT_HPP

#include <string>

class HttpClient {
public:
    HttpClient();  // 기본 생성자 선언
    ~HttpClient();

    std::string sendFile(const std::string& filePath, const std::string& url);
};

#endif
