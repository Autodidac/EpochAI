#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace epochai {

struct CountMetrics {
    int tokens = 0;
    int word_count = 0;
    int total_letters = 0;
    std::vector<int> letters_per_word;
    std::vector<std::string> words;
};

CountMetrics count_metrics(std::string_view text);

}
