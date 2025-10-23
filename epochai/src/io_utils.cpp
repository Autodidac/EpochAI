#include "epochai/io_utils.hpp"

#include <algorithm>
#include <chrono>
#include <climits>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace epochai {

namespace {

void write_all(int fd, const char* data, std::size_t size) {
    std::size_t written = 0;
    while (written < size) {
#ifdef _WIN32
        int result = _write(fd, data + written, static_cast<unsigned>(std::min<std::size_t>(size - written, static_cast<std::size_t>(INT_MAX))));
#else
        ssize_t result = ::write(fd, data + written, size - written);
#endif
        if (result <= 0) {
#ifdef _WIN32
            int error_code = errno;
            throw std::system_error(error_code, std::generic_category(), "write failed");
#else
            int error_code = errno;
            throw std::system_error(error_code, std::generic_category(), "write failed");
#endif
        }
        written += static_cast<std::size_t>(result);
    }
}

void fsync_fd(int fd) {
#ifdef _WIN32
    if (_commit(fd) != 0) {
        int error_code = errno;
        throw std::system_error(error_code, std::generic_category(), "_commit failed");
    }
#else
    if (::fsync(fd) != 0) {
        int error_code = errno;
        throw std::system_error(error_code, std::generic_category(), "fsync failed");
    }
#endif
}

} // namespace

void FileIO::atomic_write(const std::filesystem::path& path, std::string_view content) {
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
    const auto temp_path = path.string() + ".tmp";
#ifdef _WIN32
    int fd = _open(temp_path.c_str(), _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE);
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "_open failed");
    }
#else
    int fd = ::open(temp_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "open failed");
    }
#endif
    try {
        write_all(fd, content.data(), content.size());
        fsync_fd(fd);
#ifdef _WIN32
        _close(fd);
#else
        ::close(fd);
#endif
        std::error_code ec;
        std::filesystem::rename(temp_path, path, ec);
        if (ec) {
            std::filesystem::remove(path, ec);
            std::filesystem::rename(temp_path, path, ec);
        }
        if (ec) {
            throw std::system_error(ec);
        }
    } catch (...) {
#ifdef _WIN32
        _close(fd);
        _unlink(temp_path.c_str());
#else
        ::close(fd);
        ::unlink(temp_path.c_str());
#endif
        throw;
    }
}

void FileIO::append_log(const std::filesystem::path& path, std::string_view content) {
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
#ifdef _WIN32
    int fd = _open(path.string().c_str(), _O_WRONLY | _O_CREAT | _O_APPEND | _O_BINARY, _S_IREAD | _S_IWRITE);
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "_open log failed");
    }
#else
    int fd = ::open(path.string().c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "open log failed");
    }
#endif
    try {
        write_all(fd, content.data(), content.size());
        fsync_fd(fd);
#ifdef _WIN32
        _close(fd);
#else
        ::close(fd);
#endif
    } catch (...) {
#ifdef _WIN32
        _close(fd);
#else
        ::close(fd);
#endif
        throw;
    }
}

std::string FileIO::read_file(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Failed to open file: " + path.string());
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

std::optional<std::string> FileIO::try_read_file(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return std::nullopt;
    }
    try {
        return read_file(path);
    } catch (...) {
        return std::nullopt;
    }
}

std::string format_utc_timestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm{};
#ifdef _WIN32
    gmtime_s(&utc_tm, &time);
#else
    gmtime_r(&time, &utc_tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string to_hex(std::uint64_t value) {
    std::ostringstream oss;
    oss << std::hex << std::nouppercase << value;
    return oss.str();
}

}
