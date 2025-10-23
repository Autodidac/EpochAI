#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace epochai {

class EventLogger {
public:
    explicit EventLogger(std::filesystem::path log_path);
    void log_line(std::string_view line);
private:
    std::filesystem::path log_path_;
};

}
