#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace epochai {

/// \file http_client.hpp
/// Lightweight synchronous HTTP client responsible for communicating with
/// external services such as MCP and LM Studio.
///
/// Requests are immutable after creation, and response metadata remains valid
/// for the lifetime of the returned `HttpResult`. No persistent network
/// connections are kept across calls; each invocation of `perform` starts a
/// fresh request cycle.

/// Plain-old-data request description used by `HttpClient`.
struct HttpRequest {
    std::string method;
    std::string url;
    std::string body;
    std::string content_type = "application/json";
};

/// Resulting HTTP payload and metadata.
struct HttpResponse {
    int status = 0;
    std::string body;
    std::string headers;
};

/// Top-level result of a request, including error context and latency.
struct HttpResult {
    bool success = false;
    HttpResponse response;
    std::string error_message;
    std::chrono::milliseconds latency{0};
};

/// Synchronous HTTP transport with retry logic.
class HttpClient {
public:
    HttpClient();

    /// Perform an HTTP request with bounded timeout and retries.
    ///
    /// @param request Immutable request description to send.
    /// @param timeout_ms Per-attempt timeout in milliseconds.
    /// @param retries Number of retry attempts allowed after the first try.
    /// @returns A populated `HttpResult` describing success or failure.
    HttpResult perform(const HttpRequest& request, int timeout_ms, int retries) const;

private:
    HttpResult perform_once(const HttpRequest& request, int timeout_ms) const;
};

}
