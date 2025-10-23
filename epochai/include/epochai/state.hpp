#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace epochai {

struct TrainingConfig {
    std::string mcp_url;
    std::string lm_studio_url;
    int request_timeout_ms = 2000;
    int retries = 2;
};

struct ModelState {
    int step = 0;
    std::vector<std::string> vocab;
    std::unordered_map<std::string, std::unordered_map<std::string, double>> transitions;
    std::unordered_map<std::string, double> totals;
};

struct TrainingStats {
    double loss_before = 0.0;
    double loss_after = 0.0;
    double perplexity = 0.0;
    std::size_t token_count = 0;
    std::size_t sequence_count = 0;
};

struct EvaluationStats {
    double loss = 0.0;
    double perplexity = 0.0;
};

class StateManager {
public:
    explicit StateManager(std::filesystem::path root);

    const std::filesystem::path& root() const noexcept { return root_; }
    std::filesystem::path config_path() const;
    std::filesystem::path dataset_path() const;
    std::filesystem::path model_state_path() const;
    std::filesystem::path log_path() const;

    TrainingConfig load_or_initialize_config();
    std::vector<std::string> load_or_initialize_dataset();
    ModelState load_or_initialize_model_state();
    void save_model_state(const ModelState& state);

private:
    std::filesystem::path root_;
};

TrainingStats train_one_step(ModelState& state, const std::vector<std::vector<std::string>>& sequences,
                              std::size_t vocab_size);
EvaluationStats evaluate_model(const ModelState& state, const std::vector<std::vector<std::string>>& sequences,
                               std::size_t vocab_size);
void ensure_core_tokens(ModelState& state);
void update_vocab(ModelState& state, const std::vector<std::string>& tokens);

}
