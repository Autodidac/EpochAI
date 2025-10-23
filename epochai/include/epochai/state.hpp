#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace epochai {

/// \file state.hpp
/// Persistent training state management and high-level optimization helpers.
///
/// This header defines the canonical on-disk schema (configuration, datasets,
/// model state) alongside helper routines that update transition statistics and
/// report metrics. The `StateManager` assumes exclusive ownership over its
/// `root` directory; concurrent writers must coordinate externally.

/// Configuration required to connect to external model providers.
struct TrainingConfig {
    std::string mcp_url;
    std::string lm_studio_url;
    int request_timeout_ms = 2000;
    int retries = 2;
};

/// Markov-style model state persisted between training runs.
struct ModelState {
    int step = 0;
    std::vector<std::string> vocab;
    std::unordered_map<std::string, std::unordered_map<std::string, double>> transitions;
    std::unordered_map<std::string, double> totals;
};

/// Summary of a single training iteration.
struct TrainingStats {
    double loss_before = 0.0;
    double loss_after = 0.0;
    double perplexity = 0.0;
    std::size_t token_count = 0;
    std::size_t sequence_count = 0;
};

/// Evaluation metrics recorded after running validation sequences.
struct EvaluationStats {
    double loss = 0.0;
    double perplexity = 0.0;
};

/// Filesystem-backed accessor for EpochAI state artifacts.
class StateManager {
public:
    /// Create a manager rooted at `root`. The directory must be writable.
    explicit StateManager(std::filesystem::path root);

    const std::filesystem::path& root() const noexcept { return root_; }
    std::filesystem::path config_path() const;
    std::filesystem::path dataset_path() const;
    std::filesystem::path model_state_path() const;
    std::filesystem::path log_path() const;

    /// Load state from disk or create defaults when missing.
    TrainingConfig load_or_initialize_config();
    std::vector<std::string> load_or_initialize_dataset();
    ModelState load_or_initialize_model_state();

    /// Persist the supplied `state`, overwriting any previous version.
    void save_model_state(const ModelState& state);

private:
    std::filesystem::path root_;
};

/// Perform one training iteration, mutating `state` in-place.
TrainingStats train_one_step(ModelState& state, const std::vector<std::vector<std::string>>& sequences,
                              std::size_t vocab_size);

/// Evaluate the model using the provided sequences without mutating state.
EvaluationStats evaluate_model(const ModelState& state, const std::vector<std::vector<std::string>>& sequences,
                               std::size_t vocab_size);

/// Ensure the state contains the required special tokens.
void ensure_core_tokens(ModelState& state);

/// Merge newly observed tokens into the vocabulary and update counters.
void update_vocab(ModelState& state, const std::vector<std::string>& tokens);

}
