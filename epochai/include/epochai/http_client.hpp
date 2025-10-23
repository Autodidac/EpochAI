#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace epochai {

struct HttpRequest {
    std::string method;
    std::string url;
    std::string body;
    std::string content_type = "application/json";
};

struct HttpResponse {
    int status = 0;
    std::string body;
    std::string headers;
};

struct HttpResult {
    bool success = false;
    HttpResponse response;
    std::string error_message;
    std::chrono::milliseconds latency{0};
};

class HttpClient {
public:
    HttpClient();
    HttpResult perform(const HttpRequest& request, int timeout_ms, int retries) const;
private:
    HttpResult perform_once(const HttpRequest& request, int timeout_ms) const;
};

}
