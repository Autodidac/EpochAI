#include "epochai/state.hpp"

#include "epochai/io_utils.hpp"
#include "epochai/tokenizer.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace epochai {
namespace {

constexpr std::string_view kPadToken = "<pad>";
constexpr std::string_view kEosToken = "<eos>";

struct LossComputationResult {
    double loss_sum = 0.0;
    std::size_t count = 0;
};

LossComputationResult compute_loss_internal(const ModelState& state,
                                            const std::vector<std::vector<std::string>>& sequences,
                                            std::size_t vocab_size) {
    LossComputationResult result;
    if (vocab_size == 0) {
        return result;
    }
    for (const auto& seq : sequences) {
        if (seq.size() < 2) {
            continue;
        }
        for (std::size_t i = 0; i + 1 < seq.size(); ++i) {
            const auto& current = seq[i];
            const auto& next = seq[i + 1];
            if (current == kPadToken || next == kPadToken) {
                continue;
            }
            double matched = 1.0; // Laplace smoothing
            double total = static_cast<double>(vocab_size);
            if (const auto total_it = state.totals.find(current); total_it != state.totals.end()) {
                total += total_it->second;
            }
            if (const auto map_it = state.transitions.find(current); map_it != state.transitions.end()) {
                if (const auto next_it = map_it->second.find(next); next_it != map_it->second.end()) {
                    matched += next_it->second;
                }
            }
            double probability = matched / total;
            result.loss_sum -= std::log(probability);
            result.count += 1;
        }
    }
    return result;
}

void append_default_config(const std::filesystem::path& path) {
    std::string content;
    content += "# EpochAI autodidact configuration\n";
    content += "mcp_url=http://127.0.0.1:3333/jsonrpc\n";
    content += "lm_studio_url=http://127.0.0.1:1234/v1/chat/completions\n";
    content += "request_timeout_ms=2000\n";
    content += "retries=2\n";
    FileIO::atomic_write(path, content);
}

void append_default_dataset(const std::filesystem::path& path) {
    std::string content;
    content += "The curious mind observes the world with patience and care.";
    content += "\n";
    content += "Practice each day and skill will grow stronger.";
    content += "\n";
    content += "Knowledge shared kindly becomes wisdom for everyone.";
    content += "\n";
    FileIO::atomic_write(path, content);
}

std::string_view trim(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.remove_suffix(1);
    }
    return text;
}

} // namespace

StateManager::StateManager(std::filesystem::path root)
    : root_(std::move(root)) {}

std::filesystem::path StateManager::config_path() const {
    return root_ / "config.txt";
}

std::filesystem::path StateManager::dataset_path() const {
    return root_ / "dataset.txt";
}

std::filesystem::path StateManager::model_state_path() const {
    return root_ / "model_state.txt";
}

std::filesystem::path StateManager::log_path() const {
    return root_ / "events.log";
}

TrainingConfig StateManager::load_or_initialize_config() {
    const auto path = config_path();
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(root_);
        append_default_config(path);
    }
    TrainingConfig config;
    config.mcp_url = "http://127.0.0.1:3333/jsonrpc";
    config.lm_studio_url = "http://127.0.0.1:1234/v1/chat/completions";
    config.request_timeout_ms = 2000;
    config.retries = 2;

    const auto content = FileIO::read_file(path);
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        auto view = trim(line);
        if (view.empty() || view.front() == '#') {
            continue;
        }
        const auto delimiter = view.find('=');
        if (delimiter == std::string_view::npos) {
            continue;
        }
        const auto key = view.substr(0, delimiter);
        const auto value = view.substr(delimiter + 1);
        if (key == "mcp_url") {
            config.mcp_url = std::string(value);
        } else if (key == "lm_studio_url") {
            config.lm_studio_url = std::string(value);
        } else if (key == "request_timeout_ms") {
            int parsed = config.request_timeout_ms;
            std::from_chars(value.data(), value.data() + value.size(), parsed);
            config.request_timeout_ms = parsed;
        } else if (key == "retries") {
            int parsed = config.retries;
            std::from_chars(value.data(), value.data() + value.size(), parsed);
            config.retries = parsed;
        }
    }
    return config;
}

std::vector<std::string> StateManager::load_or_initialize_dataset() {
    const auto path = dataset_path();
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(root_);
        append_default_dataset(path);
    }
    const auto content = FileIO::read_file(path);
    std::istringstream stream(content);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && (line.back() == '\r')) {
            line.pop_back();
        }
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.push_back("Learning thrives when curiosity meets practice.");
    }
    return lines;
}

ModelState StateManager::load_or_initialize_model_state() {
    const auto path = model_state_path();
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(root_);
        ModelState state;
        ensure_core_tokens(state);
        save_model_state(state);
        return state;
    }
    const auto content = FileIO::read_file(path);
    std::istringstream stream(content);
    ModelState state;
    std::string line;

    if (!std::getline(stream, line)) {
        throw std::runtime_error("Model state file is empty");
    }
    {
        auto view = trim(line);
        if (!view.starts_with("STEP ")) {
            throw std::runtime_error("Expected STEP line in model state");
        }
        int value = 0;
        const auto number_view = view.substr(5);
        auto [ptr, ec] = std::from_chars(number_view.data(), number_view.data() + number_view.size(), value);
        if (ec != std::errc()) {
            throw std::runtime_error("Failed to parse STEP value");
        }
        state.step = value;
    }

    if (!std::getline(stream, line)) {
        throw std::runtime_error("Model state missing VOCAB");
    }
    {
        auto view = trim(line);
        if (!view.starts_with("VOCAB ")) {
            throw std::runtime_error("Expected VOCAB line in model state");
        }
        int count = 0;
        const auto number_view = view.substr(6);
        auto [ptr, ec] = std::from_chars(number_view.data(), number_view.data() + number_view.size(), count);
        if (ec != std::errc()) {
            throw std::runtime_error("Failed to parse VOCAB count");
        }
        for (int i = 0; i < count; ++i) {
            if (!std::getline(stream, line)) {
                throw std::runtime_error("Unexpected end of vocab entries");
            }
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            state.vocab.push_back(line);
        }
    }

    if (!std::getline(stream, line)) {
        throw std::runtime_error("Model state missing TRANSITIONS header");
    }
    {
        auto view = trim(line);
        if (!view.starts_with("TRANSITIONS ")) {
            throw std::runtime_error("Expected TRANSITIONS line");
        }
        int count = 0;
        const auto number_view = view.substr(12);
        auto [ptr, ec] = std::from_chars(number_view.data(), number_view.data() + number_view.size(), count);
        if (ec != std::errc()) {
            throw std::runtime_error("Failed to parse transition count");
        }
        for (int i = 0; i < count; ++i) {
            if (!std::getline(stream, line)) {
                throw std::runtime_error("Unexpected end of transitions");
            }
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            std::istringstream row(line);
            std::string current;
            std::string next;
            double value = 0.0;
            if (!std::getline(row, current, '\t')) {
                throw std::runtime_error("Malformed transition line");
            }
            if (!std::getline(row, next, '\t')) {
                throw std::runtime_error("Malformed transition line");
            }
            std::string value_text;
            if (!std::getline(row, value_text)) {
                throw std::runtime_error("Malformed transition value");
            }
            try {
                value = std::stod(value_text);
            } catch (...) {
                throw std::runtime_error("Failed to parse transition value");
            }
            state.transitions[current][next] = value;
        }
    }

    if (!std::getline(stream, line)) {
        throw std::runtime_error("Model state missing TOTALS header");
    }
    {
        auto view = trim(line);
        if (!view.starts_with("TOTALS ")) {
            throw std::runtime_error("Expected TOTALS line");
        }
        int count = 0;
        const auto number_view = view.substr(7);
        auto [ptr, ec] = std::from_chars(number_view.data(), number_view.data() + number_view.size(), count);
        if (ec != std::errc()) {
            throw std::runtime_error("Failed to parse totals count");
        }
        for (int i = 0; i < count; ++i) {
            if (!std::getline(stream, line)) {
                throw std::runtime_error("Unexpected end of totals");
            }
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            std::istringstream row(line);
            std::string token;
            std::string value_text;
            if (!std::getline(row, token, '\t')) {
                throw std::runtime_error("Malformed totals line");
            }
            if (!std::getline(row, value_text)) {
                throw std::runtime_error("Malformed totals line");
            }
            double total = 0.0;
            try {
                total = std::stod(value_text);
            } catch (...) {
                throw std::runtime_error("Failed to parse totals value");
            }
            state.totals[token] = total;
        }
    }

    ensure_core_tokens(state);
    return state;
}

void StateManager::save_model_state(const ModelState& state) {
    std::ostringstream oss;
    oss << "STEP " << state.step << "\n";
    oss << "VOCAB " << state.vocab.size() << "\n";
    for (const auto& token : state.vocab) {
        oss << token << "\n";
    }
    std::size_t transition_count = 0;
    for (const auto& [_, inner] : state.transitions) {
        transition_count += inner.size();
    }
    oss << "TRANSITIONS " << transition_count << "\n";
    for (const auto& [current, inner] : state.transitions) {
        for (const auto& [next, value] : inner) {
            oss << current << '\t' << next << '\t' << value << "\n";
        }
    }
    oss << "TOTALS " << state.totals.size() << "\n";
    for (const auto& [token, value] : state.totals) {
        oss << token << '\t' << value << "\n";
    }
    FileIO::atomic_write(model_state_path(), oss.str());
}

TrainingStats train_one_step(ModelState& state, const std::vector<std::vector<std::string>>& sequences,
                              std::size_t vocab_size) {
    TrainingStats stats;
    stats.sequence_count = sequences.size();
    const auto before = compute_loss_internal(state, sequences, vocab_size);
    if (before.count > 0) {
        stats.loss_before = before.loss_sum / static_cast<double>(before.count);
    }
    stats.token_count = before.count;

    for (const auto& seq : sequences) {
        if (seq.size() < 2) {
            continue;
        }
        for (std::size_t i = 0; i + 1 < seq.size(); ++i) {
            const auto& current = seq[i];
            const auto& next = seq[i + 1];
            if (current == kPadToken || next == kPadToken) {
                continue;
            }
            state.transitions[current][next] += 1.0;
            state.totals[current] += 1.0;
        }
    }

    state.step += 1;

    const auto after = compute_loss_internal(state, sequences, vocab_size);
    if (after.count > 0) {
        stats.loss_after = after.loss_sum / static_cast<double>(after.count);
        stats.perplexity = std::exp(stats.loss_after);
    }
    return stats;
}

EvaluationStats evaluate_model(const ModelState& state, const std::vector<std::vector<std::string>>& sequences,
                               std::size_t vocab_size) {
    EvaluationStats stats;
    const auto result = compute_loss_internal(state, sequences, vocab_size);
    if (result.count > 0) {
        stats.loss = result.loss_sum / static_cast<double>(result.count);
        stats.perplexity = std::exp(stats.loss);
    }
    return stats;
}

void ensure_core_tokens(ModelState& state) {
    if (std::find(state.vocab.begin(), state.vocab.end(), kPadToken) == state.vocab.end()) {
        state.vocab.emplace_back(kPadToken);
    }
    if (std::find(state.vocab.begin(), state.vocab.end(), kEosToken) == state.vocab.end()) {
        state.vocab.emplace_back(kEosToken);
    }
}

void update_vocab(ModelState& state, const std::vector<std::string>& tokens) {
    std::unordered_set<std::string> existing(state.vocab.begin(), state.vocab.end());
    for (const auto& token : tokens) {
        if (existing.insert(token).second) {
            state.vocab.push_back(token);
        }
    }
}

}
