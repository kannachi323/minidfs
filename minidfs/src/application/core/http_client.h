#pragma once

#include <string>
#include <map>
#include <functional>
#include <atomic>

namespace minidfs::core {

    struct HttpResponse {
        int status_code;
        std::string body;
        std::map<std::string, std::string> headers;
    };

    // Progress callback: (bytes_uploaded, total_bytes) -> bool (return false to cancel)
    using UploadProgressCallback = std::function<bool(size_t bytes_uploaded, size_t total_bytes)>;

    // Result of a chunked upload operation
    struct ChunkedUploadResult {
        bool success = false;
        int final_status_code = 0;
        std::string error_message;
        std::string response_body;  // Final response from server (contains file metadata on success)
    };

    class HttpClient {
    public:
        static HttpClient& get();

        HttpResponse get(const std::string& url, const std::map<std::string, std::string>& headers = {});
        HttpResponse post(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers = {});
        HttpResponse put(const std::string& url, const std::string& body, const std::map<std::string, std::string>& headers = {});
        HttpResponse del(const std::string& url, const std::map<std::string, std::string>& headers = {});

        // Chunked upload for large files (Microsoft Graph upload session)
        // upload_url: The uploadUrl from createUploadSession response
        // file_path: Local file path to upload
        // chunk_size: Size of each chunk in bytes (default 10MB, must be multiple of 320KB)
        // progress_cb: Optional callback for progress updates
        // cancel_flag: Optional atomic bool to signal cancellation
        ChunkedUploadResult chunked_upload(
            const std::string& upload_url,
            const std::string& file_path,
            size_t chunk_size = 10 * 1024 * 1024,  // 10MB default
            UploadProgressCallback progress_cb = nullptr,
            std::atomic<bool>* cancel_flag = nullptr
        );

    private:
        HttpClient() = default;
        ~HttpClient() = default;
        HttpClient(const HttpClient&) = delete;
        HttpClient& operator=(const HttpClient&) = delete;

        HttpResponse perform_request(const std::string& method, const std::string& url, const std::string& body = "", const std::map<std::string, std::string>& headers = {});
    };

}
