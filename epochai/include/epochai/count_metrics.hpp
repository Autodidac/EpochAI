#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace epochai {

/// \file count_metrics.hpp
/// Token and character counting utilities used by training diagnostics.
///
/// Functions in this header accept raw text inputs and derive aggregate
/// statistics about tokenization and lexical structure. The invariants around
/// the returned metrics are intentionally simple: lengths are non-negative,
/// `letters_per_word.size()` matches `word_count`, and `tokens` mirrors the
/// tokenizer output size.

/// Aggregated lexical statistics generated from `count_metrics`.
struct CountMetrics {
    int tokens = 0;
    int word_count = 0;
    int total_letters = 0;
    std::vector<int> letters_per_word;
    std::vector<std::string> words;
};

/// Count lexical metrics for an arbitrary UTF-8 text buffer.
///
/// @param text The raw user text to analyze. The function does not take
///             ownership of the buffer.
/// @returns A `CountMetrics` structure satisfying the invariants documented
///          above.
CountMetrics count_metrics(std::string_view text);

}
