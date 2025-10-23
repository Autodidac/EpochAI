#pragma once

#include <string>

namespace epochai {

/// \file app.hpp
/// High-level application entry point responsible for bootstrapping EpochAI's
/// orchestration loop.
///
/// The application coordinates initialization of the persistent state
/// directory, loads configuration/state through the `StateManager`, and then
/// executes the main training and evaluation workflow. The `run` method returns
/// an integer process exit code (0 on success) and is expected to be invoked
/// from `main`. The `state_directory` constructor argument must point to a
/// writable location on disk where state and logs are stored.

/// Executes the primary EpochAI workflow.
///
/// The `Application` owns the lifecycle of the state directory provided at
/// construction time. The directory path is treated as immutable once the
/// instance is created.
class Application {
public:
    /// Create an application that persists all files beneath `state_directory`.
    explicit Application(std::string state_directory = "state");

    /// Start the training/evaluation loop.
    ///
    /// @returns 0 on success, or a non-zero process exit code describing an
    ///          unrecoverable failure.
    int run();

private:
    std::string state_directory_;
};

}
