#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace epochai {

/// \file logger.hpp
/// Structured logging helpers focused on append-only event streams.
///
/// The logger operates on plain-text lines, guaranteeing that each call to
/// `log_line` produces a single atomic append to the underlying file. Callers
/// are responsible for formatting messages prior to logging.

/// Minimal append-only logger used for audit trails and debugging.
class EventLogger {
public:
    /// Create a logger that writes to `log_path`. Parent directories must exist.
    explicit EventLogger(std::filesystem::path log_path);

    /// Append a single log line terminated with a newline if needed.
    void log_line(std::string_view line);

private:
    std::filesystem::path log_path_;
};

}
