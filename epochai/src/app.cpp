#include "epochai/app.hpp"

#include "epochai/count_metrics.hpp"
#include "epochai/http_client.hpp"
#include "epochai/io_utils.hpp"
#include "epochai/logger.hpp"
#include "epochai/state.hpp"
#include "epochai/tokenizer.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

namespace epochai {
namespace {

constexpr std::string_view kPadToken = "<pad>";
constexpr std::string_view kEosToken = "<eos>";

std::string join_lines(const std::vector<std::string>& lines) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) {
            oss << '\n';
        }
        oss << lines[i];
    }
    return oss.str();
}

std::string escape_json(std::string_view text) {
    std::ostringstream oss;
    for (char ch : text) {
        switch (ch) {
        case '\\':
            oss << "\\\\";
            break;
        case '"':
            oss << "\\\"";
            break;
        case '\n':
            oss << "\\n";
            break;
        case '\r':
            oss << "\\r";
            break;
        case '\t':
            oss << "\\t";
            break;
        default:
            oss << ch;
            break;
        }
    }
    return oss.str();
}

std::string hash_string(const std::string& value) {
    return to_hex(static_cast<std::uint64_t>(std::hash<std::string>{}(value)));
}

} // namespace

Application::Application(std::string state_directory)
    : state_directory_(std::move(state_directory)) {}

int Application::run() {
    StateManager manager(state_directory_);
    std::filesystem::create_directories(manager.root());
    EventLogger logger(manager.log_path());

    const auto start_timestamp = format_utc_timestamp();
    logger.log_line(std::string("{\"timestamp\":\"") + start_timestamp + "\",\"action\":\"startup\",\"version\":\"" + EPOCHAI_VERSION + "\"}");

    const auto config = manager.load_or_initialize_config();
    auto dataset_lines = manager.load_or_initialize_dataset();
    auto state = manager.load_or_initialize_model_state();
    ensure_core_tokens(state);

    std::vector<std::vector<std::string>> raw_sequences;
    raw_sequences.reserve(dataset_lines.size());
    for (const auto& line : dataset_lines) {
        auto tokens = tokenize(line);
        update_vocab(state, tokens);
        tokens.push_back(std::string(kEosToken));
        raw_sequences.push_back(tokens);
    }
    const std::size_t max_length = std::accumulate(raw_sequences.begin(), raw_sequences.end(), std::size_t{0},
                                                   [](std::size_t acc, const auto& seq) { return std::max(acc, seq.size()); });
    std::vector<std::vector<std::string>> sequences;
    sequences.reserve(raw_sequences.size());
    for (auto& seq : raw_sequences) {
        std::vector<std::string> padded(std::max<std::size_t>(1, max_length), std::string(kPadToken));
        for (std::size_t i = 0; i < seq.size(); ++i) {
            padded[i] = std::move(seq[i]);
        }
        sequences.push_back(std::move(padded));
    }

    const auto dataset_blob = join_lines(dataset_lines);
    const auto metrics = count_metrics(dataset_blob);
    std::ostringstream metrics_log;
    metrics_log << "{\"timestamp\":\"" << format_utc_timestamp() << "\",";
    metrics_log << "\"action\":\"dataset_metrics\",";
    metrics_log << "\"tokens\":" << metrics.tokens << ",";
    metrics_log << "\"words\":" << metrics.word_count << ",";
    metrics_log << "\"total_letters\":" << metrics.total_letters << ",";
    metrics_log << "\"hash\":\"" << hash_string(dataset_blob) << "\"}";
    logger.log_line(metrics_log.str());

    const std::size_t vocab_size = state.vocab.size();
    const auto train_start = std::chrono::steady_clock::now();
    auto stats = train_one_step(state, sequences, vocab_size);
    const auto train_latency = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - train_start);

    manager.save_model_state(state);

    std::ostringstream train_log;
    train_log << "{\"timestamp\":\"" << format_utc_timestamp() << "\",";
    train_log << "\"action\":\"train\",";
    train_log << "\"step\":" << state.step << ",";
    train_log << "\"loss_before\":" << stats.loss_before << ",";
    train_log << "\"loss_after\":" << stats.loss_after << ",";
    train_log << "\"perplexity\":" << stats.perplexity << ",";
    train_log << "\"tokens\":" << stats.token_count << ",";
    train_log << "\"sequences\":" << stats.sequence_count << ",";
    train_log << "\"latency_ms\":" << train_latency.count() << ",";
    train_log << "\"dataset_hash\":\"" << hash_string(dataset_blob) << "\"}";
    logger.log_line(train_log.str());

    const auto eval_stats = evaluate_model(state, sequences, state.vocab.size());
    std::ostringstream eval_log;
    eval_log << "{\"timestamp\":\"" << format_utc_timestamp() << "\",";
    eval_log << "\"action\":\"evaluation\",";
    eval_log << "\"loss\":" << eval_stats.loss << ",";
    eval_log << "\"perplexity\":" << eval_stats.perplexity << ",";
    eval_log << "\"step\":" << state.step << "}";
    logger.log_line(eval_log.str());

    HttpClient client;

    const std::string health_request =
        "{\"jsonrpc\":\"2.0\",\"id\":\"health\",\"method\":\"health\",\"params\":{}}";
    HttpRequest mcp_health{.method = "POST", .url = config.mcp_url, .body = health_request};
    const auto mcp_health_result = client.perform(mcp_health, config.request_timeout_ms, config.retries);

    std::ostringstream mcp_health_log;
    mcp_health_log << "{\"timestamp\":\"" << format_utc_timestamp() << "\",";
    mcp_health_log << "\"action\":\"mcp_health\",";
    mcp_health_log << "\"request_hash\":\"" << hash_string(health_request) << "\",";
    if (mcp_health_result.success) {
        mcp_health_log << "\"status\":" << mcp_health_result.response.status << ",";
        mcp_health_log << "\"latency_ms\":" << mcp_health_result.latency.count() << ",";
        mcp_health_log << "\"response_hash\":\"" << hash_string(mcp_health_result.response.body) << "\"";
    } else {
        mcp_health_log << "\"error\":\"" << escape_json(mcp_health_result.error_message) << "\",";
        mcp_health_log << "\"latency_ms\":" << mcp_health_result.latency.count() << "";
    }
    mcp_health_log << "}";
    logger.log_line(mcp_health_log.str());

    const std::string call_request =
        "{\"jsonrpc\":\"2.0\",\"id\":\"call\",\"method\":\"call\",\"params\":{\"message\":\"ping\"}}";
    HttpRequest mcp_call{.method = "POST", .url = config.mcp_url, .body = call_request};
    const auto mcp_call_result = client.perform(mcp_call, config.request_timeout_ms, config.retries);

    std::ostringstream mcp_call_log;
    mcp_call_log << "{\"timestamp\":\"" << format_utc_timestamp() << "\",";
    mcp_call_log << "\"action\":\"mcp_call\",";
    mcp_call_log << "\"request_hash\":\"" << hash_string(call_request) << "\",";
    if (mcp_call_result.success) {
        mcp_call_log << "\"status\":" << mcp_call_result.response.status << ",";
        mcp_call_log << "\"latency_ms\":" << mcp_call_result.latency.count() << ",";
        mcp_call_log << "\"response_hash\":\"" << hash_string(mcp_call_result.response.body) << "\"";
    } else {
        mcp_call_log << "\"error\":\"" << escape_json(mcp_call_result.error_message) << "\",";
        mcp_call_log << "\"latency_ms\":" << mcp_call_result.latency.count() << "";
    }
    mcp_call_log << "}";
    logger.log_line(mcp_call_log.str());

    const std::string lm_request_body =
        "{\"model\":\"default\",\"messages\":[{\"role\":\"user\",\"content\":\"Hello from EpochAI.\"}]}";
    HttpRequest lm_request{.method = "POST", .url = config.lm_studio_url, .body = lm_request_body};
    const auto lm_result = client.perform(lm_request, config.request_timeout_ms, config.retries);

    std::ostringstream lm_log;
    lm_log << "{\"timestamp\":\"" << format_utc_timestamp() << "\",";
    lm_log << "\"action\":\"lm_studio_chat\",";
    lm_log << "\"request_hash\":\"" << hash_string(lm_request_body) << "\",";
    if (lm_result.success) {
        lm_log << "\"status\":" << lm_result.response.status << ",";
        lm_log << "\"latency_ms\":" << lm_result.latency.count() << ",";
        lm_log << "\"response_hash\":\"" << hash_string(lm_result.response.body) << "\"";
    } else {
        lm_log << "\"error\":\"" << escape_json(lm_result.error_message) << "\",";
        lm_log << "\"latency_ms\":" << lm_result.latency.count() << "";
    }
    lm_log << "}";
    logger.log_line(lm_log.str());

    std::cout << "EpochAI autodidact step " << state.step << " completed." << std::endl;
    std::cout << "Training loss: " << stats.loss_after << ", perplexity: " << stats.perplexity << std::endl;
    if (!mcp_health_result.success) {
        std::cout << "MCP health check failed: " << mcp_health_result.error_message << std::endl;
    }
    if (!mcp_call_result.success) {
        std::cout << "MCP call failed: " << mcp_call_result.error_message << std::endl;
    }
    if (!lm_result.success) {
        std::cout << "LM Studio request failed: " << lm_result.error_message << std::endl;
    }

    return 0;
}

} // namespace epochai
