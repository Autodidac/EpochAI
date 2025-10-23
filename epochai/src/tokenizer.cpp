#include "epochai/tokenizer.hpp"

#include <cctype>

namespace epochai {

std::vector<std::string> tokenize(std::string_view text) {
    std::vector<std::string> tokens;
    std::string current;

    auto flush_current = [&]() {
        if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    };

    for (char ch : text) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            flush_current();
            continue;
        }
        if (std::ispunct(static_cast<unsigned char>(ch))) {
            flush_current();
            tokens.emplace_back(1, ch);
            continue;
        }
        current.push_back(ch);
    }
    flush_current();
    return tokens;
}

}
