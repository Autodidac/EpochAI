#pragma once

#include <string>

namespace epochai {

class Application {
public:
    explicit Application(std::string state_directory = "state");
    int run();
private:
    std::string state_directory_;
};

}
