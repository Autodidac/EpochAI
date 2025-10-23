#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace epochai {

/// \file tokenizer.hpp
/// Tokenization utilities converting raw strings into vocabulary-aligned
/// tokens.
///
/// Tokenization is deterministic and whitespace-delimited; callers should
/// normalize input prior to invoking `tokenize` to ensure consistent results.

/// Split text into tokens compatible with `ModelState::vocab`.
std::vector<std::string> tokenize(std::string_view text);

}
