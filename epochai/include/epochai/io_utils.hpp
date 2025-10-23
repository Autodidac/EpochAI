#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace epochai {

/// \file io_utils.hpp
/// File system and formatting helpers used throughout EpochAI for
/// deterministic state persistence and logging.
///
/// All functions in this header assume callers provide canonicalized paths and
/// have the necessary permissions. Writes are atomic and leave no partially
/// written files behind even in the presence of process failures.

/// Atomic file writing and log utilities.
class FileIO {
public:
    /// Write `content` to `path`, guaranteeing atomic replacement semantics.
    static void atomic_write(const std::filesystem::path& path, std::string_view content);

    /// Append a log line to `path`, creating the file if it does not exist.
    static void append_log(const std::filesystem::path& path, std::string_view content);

    /// Read an entire file and throw on failure.
    static std::string read_file(const std::filesystem::path& path);

    /// Attempt to read a file, returning `std::nullopt` when it does not exist.
    static std::optional<std::string> try_read_file(const std::filesystem::path& path);
};

/// Format the current UTC timestamp in ISO-8601 form.
std::string format_utc_timestamp();

/// Convert an integer to a zero-padded hexadecimal string.
std::string to_hex(std::uint64_t value);

}
