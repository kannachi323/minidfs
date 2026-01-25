#include "http_client.h"
#include <curl/curl.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

namespace minidfs::core {

    // Align chunk size to 320KB boundary (required by Microsoft Graph)
    static size_t align_chunk_size(size_t requested_size) {
        const size_t alignment = 320 * 1024;  // 320 KB
        if (requested_size < alignment) {
            return alignment;
        }
        return (requested_size / alignment) * alignment;
    }

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

        // Disable SSL verification for localhost (needed for self-signed certificates)
        if (url.find("localhost") != std::string::npos || url.find("127.0.0.1") != std::string::npos) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

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

    ChunkedUploadResult HttpClient::chunked_upload(
        const std::string& upload_url,
        const std::string& file_path,
        size_t chunk_size,
        UploadProgressCallback progress_cb,
        std::atomic<bool>* cancel_flag
    ) {
        ChunkedUploadResult result;

        // Open file and get size
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            result.error_message = "Failed to open file: " + file_path;
            return result;
        }

        size_t file_size = static_cast<size_t>(file.tellg());
        file.seekg(0, std::ios::beg);

        if (file_size == 0) {
            result.error_message = "File is empty";
            return result;
        }

        // Align chunk size to 320KB boundary
        size_t aligned_chunk_size = align_chunk_size(chunk_size);

        // Allocate buffer for chunks
        std::vector<char> buffer(aligned_chunk_size);

        size_t bytes_uploaded = 0;

        while (bytes_uploaded < file_size) {
            // Check for cancellation
            if (cancel_flag && cancel_flag->load()) {
                result.error_message = "Upload cancelled";
                return result;
            }

            // Calculate this chunk's size
            size_t remaining = file_size - bytes_uploaded;
            size_t current_chunk_size = std::min(aligned_chunk_size, remaining);

            // Read chunk from file
            file.read(buffer.data(), current_chunk_size);
            if (!file && !file.eof()) {
                result.error_message = "Failed to read file at offset " + std::to_string(bytes_uploaded);
                return result;
            }

            size_t bytes_read = static_cast<size_t>(file.gcount());
            if (bytes_read == 0) {
                break;
            }

            // Calculate byte range for Content-Range header
            size_t range_start = bytes_uploaded;
            size_t range_end = bytes_uploaded + bytes_read - 1;

            // Format: "bytes start-end/total"
            std::string content_range = "bytes " + std::to_string(range_start) + "-"
                                       + std::to_string(range_end) + "/" + std::to_string(file_size);

            // Initialize curl for this chunk
            CURL* curl = curl_easy_init();
            if (!curl) {
                result.error_message = "Failed to initialize CURL";
                return result;
            }

            CurlData response_data;

            curl_easy_setopt(curl, CURLOPT_URL, upload_url.c_str());
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buffer.data());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, bytes_read);

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_data);

            // Set headers
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, ("Content-Length: " + std::to_string(bytes_read)).c_str());
            headers = curl_slist_append(headers, ("Content-Range: " + content_range).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            // Follow redirects
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

            // Perform the request
            CURLcode res = curl_easy_perform(curl);

            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK) {
                result.error_message = "CURL error: " + std::string(curl_easy_strerror(res));
                return result;
            }

            // Check response status
            // 202 Accepted = more chunks expected
            // 200/201 = upload complete
            // 4xx/5xx = error
            if (http_code >= 400) {
                result.final_status_code = static_cast<int>(http_code);
                result.error_message = "Upload failed with status " + std::to_string(http_code) + ": " + response_data.response_body;
                result.response_body = response_data.response_body;
                return result;
            }

            bytes_uploaded += bytes_read;

            // Call progress callback
            if (progress_cb) {
                if (!progress_cb(bytes_uploaded, file_size)) {
                    result.error_message = "Upload cancelled by callback";
                    return result;
                }
            }

            // Check if upload is complete (200 or 201 response)
            if (http_code == 200 || http_code == 201) {
                result.success = true;
                result.final_status_code = static_cast<int>(http_code);
                result.response_body = response_data.response_body;
                return result;
            }
        }

        // If we get here without a 200/201, something unexpected happened
        if (!result.success) {
            result.error_message = "Upload ended unexpectedly without completion response";
        }

        return result;
    }

}
