#include "epochai/count_metrics.hpp"

#include "epochai/tokenizer.hpp"

#include <cctype>

namespace epochai {

CountMetrics count_metrics(std::string_view text) {
    CountMetrics metrics;
    const auto tokens = tokenize(text);
    metrics.tokens = static_cast<int>(tokens.size());

    for (const auto& token : tokens) {
        bool is_word = true;
        int letter_count = 0;
        for (char ch : token) {
            if (std::isalpha(static_cast<unsigned char>(ch))) {
                ++letter_count;
            } else {
                is_word = false;
            }
        }
        if (is_word && !token.empty()) {
            metrics.word_count += 1;
            metrics.total_letters += letter_count;
            metrics.letters_per_word.push_back(letter_count);
            metrics.words.push_back(token);
        }
    }

    return metrics;
}

}
