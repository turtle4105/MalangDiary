#ifndef HTTPCLIENT_HPP
#define HTTPCLIENT_HPP

#include <string>

class HttpClient {
public:
    HttpClient();  // �⺻ ������ ����
    ~HttpClient();

    std::string sendFile(const std::string& filePath, const std::string& url);
    std::string sendMultipleFiles(const std::string& childName, 
                                 const std::string& audioFilePath, 
                                 const std::string& embeddingFilePath, 
                                 const std::string& url);
};

#endif
