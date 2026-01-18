#include "http_client.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>

namespace minidfs::core {

    struct CurlData {
        std::string response_body;
        std::map<std::string, std::string> response_headers;
    };

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        size_t total_size = size * nmemb;
        CurlData* data = static_cast<CurlData*>(userp);
        data->response_body.append(static_cast<char*>(contents), total_size);
        return total_size;
    }

    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userp) {
        size_t total_size = size * nitems;
        CurlData* data = static_cast<CurlData*>(userp);
        
        std::string header_line(buffer, total_size);
        
        // Remove trailing \r\n
        if (header_line.length() >= 2) {
            header_line = header_line.substr(0, header_line.length() - 2);
        }
        
        // Parse header (format: "Key: Value")
        size_t colon_pos = header_line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = header_line.substr(0, colon_pos);
            std::string value = header_line.substr(colon_pos + 1);
            
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            data->response_headers[key] = value;
        }
        
        return total_size;
    }

    HttpClient& HttpClient::get() {
        static HttpClient instance;
        return instance;
    }

    HttpResponse HttpClient::get(const std::string& url, const std::map<std::string, std::string>& headers) {
        return perform_request("GET", url, "", headers);
    }

    HttpResponse HttpClient::post(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers) {
        return perform_request("POST", url, body, headers);
    }

    HttpResponse HttpClient::put(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers) {
        return perform_request("PUT", url, body, headers);
    }

    HttpResponse HttpClient::del(const std::string& url, const std::map<std::string, std::string>& headers) {
        return perform_request("DELETE", url, "", headers);
    }

    HttpResponse HttpClient::perform_request(const std::string& method, const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers) {
        HttpResponse response;
        response.status_code = 0;

        CURL* curl = curl_easy_init();
        if (!curl) {
            std::cerr << "Failed to initialize CURL" << std::endl;
            return response;
        }

        CurlData data;

        // Set URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Set callback functions
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &data);

        // Set HTTP method
        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        } else if (method == "PUT") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        } else if (method == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }

        // Set headers
        struct curl_slist* header_list = nullptr;
        for (const auto& [key, value] : headers) {
            std::string header = key + ": " + value;
            header_list = curl_slist_append(header_list, header.c_str());
        }
        if (header_list) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
        }

        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        // Perform request
        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK) {
            long response_code;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            response.status_code = static_cast<int>(response_code);
            response.body = data.response_body;
            response.headers = data.response_headers;
        } else {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        }

        // Cleanup
        if (header_list) {
            curl_slist_free_all(header_list);
        }
        curl_easy_cleanup(curl);

        return response;
    }

}
