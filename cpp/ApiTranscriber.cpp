#include <iostream>
#include <curl/curl.h>  // 설치 필요
/*
int main() {
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8000/transcribe");

        struct curl_httppost* form = NULL;
        struct curl_httppost* last = NULL;

        curl_formadd(&form, &last,
            CURLFORM_COPYNAME, "file",
            CURLFORM_FILE, "C:/path/to/audio.wav",
            CURLFORM_END);

        curl_easy_setopt(curl, CURLOPT_HTTPPOST, form);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            std::cerr << "curl error: " << curl_easy_strerror(res) << std::endl;

        curl_easy_cleanup(curl);
    }
    return 0;
}
*/