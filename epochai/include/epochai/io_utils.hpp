#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace epochai {

class FileIO {
public:
    static void atomic_write(const std::filesystem::path& path, std::string_view content);
    static void append_log(const std::filesystem::path& path, std::string_view content);
    static std::string read_file(const std::filesystem::path& path);
    static std::optional<std::string> try_read_file(const std::filesystem::path& path);
};

std::string format_utc_timestamp();
std::string to_hex(std::uint64_t value);

}
