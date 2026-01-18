#pragma once

#include <string>
#include <map>

namespace minidfs::core {

    struct HttpResponse {
        int status_code;
        std::string body;
        std::map<std::string, std::string> headers;
    };

    class HttpClient {
    public:
        static HttpClient& get();

        HttpResponse get(const std::string& url, const std::map<std::string, std::string>& headers = {});
        HttpResponse post(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers = {});
        HttpResponse put(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers = {});
        HttpResponse del(const std::string& url, const std::map<std::string, std::string>& headers = {});

    private:
        HttpClient() = default;
        ~HttpClient() = default;
        HttpClient(const HttpClient&) = delete;
        HttpClient& operator=(const HttpClient&) = delete;

        HttpResponse perform_request(const std::string& method, const std::string& url, const std::string& body = "", const std::map<std::string, std::string>& headers = {});
    };

}
