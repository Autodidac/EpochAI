#include "epochai/app.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        epochai::Application app;
        return app.run();
    } catch (const std::exception& ex) {
        std::cerr << "EpochAI fatal error: " << ex.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "EpochAI fatal error: unknown exception" << std::endl;
        return 1;
    }
}
