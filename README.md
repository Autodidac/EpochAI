# EpochAI

EpochAI is an open research stack for building persistent, self-improving autonomous agents. Our goal is to help developers ship AI copilots that can remember context across sessions, learn from new data, and remain transparent for human operators. The project combines fast inference runtimes with a modular knowledge system so contributors can focus on novel behaviors instead of scaffolding.

## Mission
- **Human-aligned autonomy.** Keep a human in the loop with auditable plans, explainable decisions, and role-based access control.
- **Continuous learning.** Provide primitives for long-term memory, retrieval-augmented reasoning, and safe policy updates.
- **Composable tooling.** Integrate cleanly with local-first developer workflows (Model Context Protocol, LM Studio) while staying cloud-ready.

## Architecture at a Glance
EpochAI is organized as a C++20 core with extension points that can be bound from Python, MCP tools, or other host applications.

- **Runtime kernel.** `epochai/` contains the application core, scheduling loop, and state manager. See [Architecture Overview](docs/architecture.md) for detailed module diagrams.
- **Memory graph.** Pluggable vector and symbolic memories synchronize via the state manager so agents can mix fast retrieval with structured planning.
- **Tooling bridge.** The MCP adapter exposes tool schemas and capability discovery for IDEs and orchestrators.
- **Client SDKs.** Thin bindings expose agent services (chat, plan execution, telemetry) to downstream applications and LM Studio workspaces.

## Getting Started
1. **Clone and initialize submodules** (if any):
   ```bash
   git clone https://github.com/<org>/EpochAI.git
   cd EpochAI
   git submodule update --init --recursive
   ```
2. **Install toolchain requirements:** a C++20 compiler, CMake ≥ 3.25, and [vcpkg](https://github.com/microsoft/vcpkg) for dependency resolution.
3. **Bootstrap dependencies:**
   ```bash
   ./install.sh clang Release    # or gcc/Debug as needed
   ```

## Build & Run
EpochAI ships portable scripts and presets for Linux, macOS, and Windows.

### Using the helper scripts
```bash
./build.sh clang Debug   # Configure & compile
./run.sh                 # Launch the local agent host
```

### With CMake directly
```bash
cmake --preset=clang-debug
cmake --build --preset=clang-debug
./build/clang-debug/epochai
```
Use the `gcc-*` presets for GCC and the `x64-windows-*` presets with MSVC.

### IDE support
- **Visual Studio Code / CLion:** open the folder and select the matching CMake preset. Tasks are preconfigured in `.vscode/tasks.json`.
- **Visual Studio (MSVC):** load the generated solution from the `out/x64-windows` build tree.

## Integrations
EpochAI focuses on local-first development and ships adapters that keep agents connected to your favorite tooling.

- **Model Context Protocol (MCP):** The `mcp/` bridge registers EpochAI tools, memories, and telemetry streams. Configure the endpoint in your MCP host to expose the agent’s capabilities.
- **LM Studio:** Point LM Studio to the EpochAI runtime using the HTTP API documented below to enable local experimentation with custom models and memories.
- **Observability:** Structured logs and metrics can be exported to OpenTelemetry-compatible backends.

## Documentation
- [Architecture Overview](docs/architecture.md) – Runtime modules, message flows, and extension interfaces.
- [Roadmap](docs/roadmap.md) – Near-term milestones across inference, memory, and orchestration layers.
- [API Reference](docs/api.md) – REST and MCP schemas for embedding EpochAI into external systems.

## Contributing
We welcome issues, discussions, and pull requests. Please review `docs/roadmap.md` for active projects and comment on tickets before starting large changes. Run the provided formatting and linting tools before submitting a PR.

## License
EpochAI is released under the terms specified in [LICENSE](LICENSE).
