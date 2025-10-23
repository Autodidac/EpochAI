# EpochAI C++ API Overview

This document summarizes the responsibilities, inputs, outputs, and invariants of
core EpochAI headers. Each section mirrors the high-level documentation comments
embedded in the header files under `epochai/include/epochai/` and adds quick
usage snippets for common entry points.

## `app.hpp` — Application Lifecycle
- **Responsibilities:** Bootstraps the orchestration loop, coordinates
  initialization of the persistent state directory, and invokes the main
  training/evaluation workflow.
- **Inputs:** Optional `state_directory` string provided at construction time.
- **Outputs:** `int Application::run()` returns `0` on success or a non-zero
  exit code when an unrecoverable failure occurs.
- **Invariants:** The state directory path is immutable for the lifetime of the
  `Application` instance and must be writable by the process.

```cpp
#include "epochai/app.hpp"

int main() {
    epochai::Application app{"/var/lib/epochai"};
    return app.run();
}
```

## `count_metrics.hpp` — Text Statistics
- **Responsibilities:** Produce aggregate lexical metrics for diagnostics based
  on raw text input.
- **Inputs:** `std::string_view text` passed to `count_metrics`.
- **Outputs:** `CountMetrics` structure containing token count, word count,
  total letters, per-word letter counts, and the extracted words.
- **Invariants:**
  - All counts are non-negative integers.
  - `letters_per_word.size()` matches `word_count`.
  - `tokens` mirrors the number of entries returned by the tokenizer.

```cpp
#include "epochai/count_metrics.hpp"

void print_counts(std::string_view text) {
    const epochai::CountMetrics metrics = epochai::count_metrics(text);
    std::cout << "Tokens: " << metrics.tokens << "\n";
}
```

## `http_client.hpp` — HTTP Transport
- **Responsibilities:** Provide a synchronous HTTP client for interacting with
  external services (e.g., MCP, LM Studio).
- **Inputs:** `HttpRequest` describing method, URL, body, and content type; per
  request timeout and retry counts.
- **Outputs:** `HttpResult` containing success flag, `HttpResponse`, error
  message, and measured latency.
- **Invariants:** Requests are immutable after construction; no persistent
  connections are held between calls; latency is always non-negative.

```cpp
#include "epochai/http_client.hpp"

void ping_service() {
    epochai::HttpClient client;
    epochai::HttpRequest request{
        .method = "GET",
        .url = "https://localhost:8080/health",
    };
    const epochai::HttpResult result = client.perform(request, /*timeout_ms=*/2000, /*retries=*/1);
    if (!result.success) {
        throw std::runtime_error(result.error_message);
    }
}
```

## `io_utils.hpp` — File & Formatting Utilities
- **Responsibilities:** Deliver deterministic filesystem operations and helper
  formatting routines.
- **Inputs:** Canonical filesystem paths and string views for content; integral
  values for `to_hex`.
- **Outputs:** Written files, read buffers, formatted timestamps, and hexadecimal
  strings.
- **Invariants:**
  - Writes are atomic and do not leave partial files behind.
  - Append operations preserve existing data.
  - `format_utc_timestamp` returns ISO-8601 UTC strings.

```cpp
#include "epochai/io_utils.hpp"

void write_state(std::filesystem::path root, std::string_view snapshot) {
    const auto path = root / "model.json";
    epochai::FileIO::atomic_write(path, snapshot);
}
```

## `logger.hpp` — Event Logging
- **Responsibilities:** Append structured textual events to log files.
- **Inputs:** Destination log path provided to the constructor and log lines
  passed to `log_line`.
- **Outputs:** Append-only log files updated atomically per line.
- **Invariants:** Each call to `log_line` produces a single line in the output;
  parent directories must exist before constructing an `EventLogger`.

```cpp
#include "epochai/logger.hpp"

void record_training_step(const std::filesystem::path& log_path, int step) {
    epochai::EventLogger logger{log_path};
    logger.log_line(std::format("step={} status=ok", step));
}
```

## `state.hpp` — Persistent State & Training Helpers
- **Responsibilities:** Define the persistent configuration/state schema and
  expose routines for training, evaluation, and vocabulary management.
- **Inputs:** Writable root directory passed to `StateManager`; training/eval
  sequences provided as tokenized vectors; vocabulary size for metrics.
- **Outputs:** Loaded/saved model state, configuration, datasets, and
  `TrainingStats` / `EvaluationStats` summaries.
- **Invariants:**
  - `StateManager` assumes exclusive access to its root directory.
  - `ModelState::transitions` and `totals` remain synchronized by helper
    functions.
  - Training helpers mutate the supplied state in place.

```cpp
#include "epochai/state.hpp"

void ensure_initialized(const std::filesystem::path& root) {
    epochai::StateManager manager{root};
    auto config = manager.load_or_initialize_config();
    auto dataset = manager.load_or_initialize_dataset();
    auto model = manager.load_or_initialize_model_state();
    manager.save_model_state(model);
}
```

## `tokenizer.hpp` — Tokenization Helpers
- **Responsibilities:** Convert raw user strings into deterministic tokens aligned
  with the vocabulary maintained in `ModelState`.
- **Inputs:** `std::string_view` text.
- **Outputs:** `std::vector<std::string>` tokens.
- **Invariants:** Tokenization is whitespace-delimited and deterministic for a
  given input string.

```cpp
#include "epochai/tokenizer.hpp"

std::vector<std::string> to_tokens(std::string_view text) {
    return epochai::tokenize(text);
}
```

## Critical Entry Points at a Glance

### `Application::run`
```cpp
#include "epochai/app.hpp"

int main(int argc, char** argv) {
    const std::string state_dir = argc > 1 ? argv[1] : "state";
    epochai::Application app{state_dir};
    return app.run();
}
```

### `StateManager`
```cpp
#include "epochai/state.hpp"

void update_vocab_from_text(const std::filesystem::path& root, std::string_view text) {
    epochai::StateManager manager{root};
    auto state = manager.load_or_initialize_model_state();
    const auto tokens = epochai::tokenize(text);
    epochai::update_vocab(state, tokens);
    manager.save_model_state(state);
}
```

### Tokenizer utilities
```cpp
#include "epochai/tokenizer.hpp"

std::vector<std::string> normalize_and_tokenize(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return epochai::tokenize(text);
}
```
