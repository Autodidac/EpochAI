#include "epochai/logger.hpp"

#include "epochai/io_utils.hpp"

namespace epochai {

EventLogger::EventLogger(std::filesystem::path log_path)
    : log_path_(std::move(log_path)) {}

void EventLogger::log_line(std::string_view line) {
    std::string payload(line);
    payload.push_back('\n');
    FileIO::append_log(log_path_, payload);
}

}
