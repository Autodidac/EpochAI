#include "epochai/http_client.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace epochai {
namespace {

struct ParsedUrl {
    std::string host;
    std::string port;
    std::string path;
};

bool parse_url(const std::string& url, ParsedUrl& out) {
    constexpr std::string_view prefix = "http://";
    if (!url.starts_with(prefix)) {
        return false;
    }
    std::string remainder = url.substr(prefix.size());
    auto slash_pos = remainder.find('/');
    std::string host_port;
    if (slash_pos == std::string::npos) {
        host_port = remainder;
        out.path = "/";
    } else {
        host_port = remainder.substr(0, slash_pos);
        out.path = remainder.substr(slash_pos);
        if (out.path.empty()) {
            out.path = "/";
        }
    }
    if (host_port.empty()) {
        return false;
    }
    auto colon_pos = host_port.find(':');
    if (colon_pos == std::string::npos) {
        out.host = host_port;
        out.port = "80";
    } else {
        out.host = host_port.substr(0, colon_pos);
        out.port = host_port.substr(colon_pos + 1);
        if (out.port.empty()) {
            out.port = "80";
        }
    }
    return true;
}

#ifdef _WIN32
struct WinsockInitializer {
    WinsockInitializer() {
        WSADATA data;
        WSAStartup(MAKEWORD(2, 2), &data);
    }
    ~WinsockInitializer() {
        WSACleanup();
    }
};
#endif

void set_socket_timeout(
#ifdef _WIN32
    SOCKET socket_fd,
#else
    int socket_fd,
#endif
    int timeout_ms) {
#ifdef _WIN32
    const DWORD timeout = static_cast<DWORD>(timeout_ms);
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
#else
    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

bool send_all(
#ifdef _WIN32
    SOCKET socket_fd,
#else
    int socket_fd,
#endif
    const char* data,
    std::size_t length) {
    std::size_t sent_total = 0;
    while (sent_total < length) {
        int sent =
#ifdef _WIN32
            ::send(socket_fd, data + sent_total, static_cast<int>(length - sent_total), 0);
#else
            static_cast<int>(::send(socket_fd, data + sent_total, length - sent_total, 0));
#endif
        if (sent <= 0) {
            return false;
        }
        sent_total += static_cast<std::size_t>(sent);
    }
    return true;
}

std::string receive_all(
#ifdef _WIN32
    SOCKET socket_fd,
#else
    int socket_fd,
#endif
    bool& success) {
    std::string data;
    std::vector<char> buffer(4096);
    success = true;
    for (;;) {
        int received =
#ifdef _WIN32
            ::recv(socket_fd, buffer.data(), static_cast<int>(buffer.size()), 0);
#else
            static_cast<int>(::recv(socket_fd, buffer.data(), buffer.size(), 0));
#endif
        if (received > 0) {
            data.append(buffer.data(), received);
        } else if (received == 0) {
            break;
        } else {
#ifdef _WIN32
            int error_code = WSAGetLastError();
            if (error_code == WSAETIMEDOUT) {
                success = false;
            }
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                success = false;
            }
#endif
            break;
        }
    }
    return data;
}

} // namespace

HttpClient::HttpClient() {
#ifdef _WIN32
    static WinsockInitializer initializer;
#endif
}

HttpResult HttpClient::perform(const HttpRequest& request, int timeout_ms, int retries) const {
    HttpResult final_result;
    for (int attempt = 0; attempt <= std::max(0, retries); ++attempt) {
        auto result = perform_once(request, timeout_ms);
        if (result.success) {
            return result;
        }
        final_result = result;
    }
    return final_result;
}

HttpResult HttpClient::perform_once(const HttpRequest& request, int timeout_ms) const {
    HttpResult result;
    ParsedUrl parsed;
    if (!parse_url(request.url, parsed)) {
        result.error_message = "Unsupported URL";
        return result;
    }

    addrinfo hints{};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    addrinfo* info = nullptr;
    const auto lookup_status = getaddrinfo(parsed.host.c_str(), parsed.port.c_str(), &hints, &info);
    if (lookup_status != 0) {
        result.error_message = "DNS failure";
        return result;
    }

    auto cleanup_info = [&]() {
        if (info) {
            freeaddrinfo(info);
            info = nullptr;
        }
    };

    auto start_time = std::chrono::steady_clock::now();

#ifdef _WIN32
    SOCKET socket_fd = INVALID_SOCKET;
#else
    int socket_fd = -1;
#endif

    for (addrinfo* current = info; current != nullptr; current = current->ai_next) {
#ifdef _WIN32
        socket_fd = ::socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (socket_fd == INVALID_SOCKET) {
            continue;
        }
        set_socket_timeout(socket_fd, timeout_ms);
        if (::connect(socket_fd, current->ai_addr, static_cast<int>(current->ai_addrlen)) == 0) {
            break;
        }
        closesocket(socket_fd);
        socket_fd = INVALID_SOCKET;
#else
        socket_fd = ::socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (socket_fd < 0) {
            continue;
        }
        set_socket_timeout(socket_fd, timeout_ms);
        if (::connect(socket_fd, current->ai_addr, current->ai_addrlen) == 0) {
            break;
        }
        ::close(socket_fd);
        socket_fd = -1;
#endif
    }

    cleanup_info();

#ifdef _WIN32
    if (socket_fd == INVALID_SOCKET) {
        result.error_message = "Connection failed";
        return result;
    }
#else
    if (socket_fd < 0) {
        result.error_message = "Connection failed";
        return result;
    }
#endif

    std::ostringstream request_stream;
    request_stream << request.method << ' ' << parsed.path << " HTTP/1.1\r\n";
    request_stream << "Host: " << parsed.host << "\r\n";
    request_stream << "Content-Type: " << request.content_type << "\r\n";
    request_stream << "Accept: application/json\r\n";
    request_stream << "Connection: close\r\n";
    request_stream << "Content-Length: " << request.body.size() << "\r\n\r\n";
    request_stream << request.body;
    const auto request_text = request_stream.str();

    if (!send_all(socket_fd, request_text.c_str(), request_text.size())) {
#ifdef _WIN32
        closesocket(socket_fd);
#else
        ::close(socket_fd);
#endif
        result.error_message = "Send failed";
        return result;
    }

    bool receive_success = true;
    const auto raw_response = receive_all(socket_fd, receive_success);

#ifdef _WIN32
    closesocket(socket_fd);
#else
    ::close(socket_fd);
#endif

    result.latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);

    if (!receive_success && raw_response.empty()) {
        result.error_message = "Receive timeout";
        return result;
    }

    auto header_end = raw_response.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        result.error_message = "Malformed HTTP response";
        return result;
    }
    const std::string header_text = raw_response.substr(0, header_end);
    result.response.headers = header_text;
    result.response.body = raw_response.substr(header_end + 4);

    auto line_end = header_text.find("\r\n");
    std::string status_line = header_text.substr(0, line_end);
    std::istringstream status_stream(status_line);
    std::string http_version;
    int status_code = 0;
    status_stream >> http_version >> status_code;
    result.response.status = status_code;
    result.success = status_code >= 200 && status_code < 300;
    if (!result.success && result.error_message.empty()) {
        result.error_message = "HTTP status " + std::to_string(status_code);
    }
    return result;
}

} // namespace epochai
