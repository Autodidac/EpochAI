# Autodidact Loop Roadmap

This roadmap outlines the major milestones required to deliver the autodidact loop for EpochAI. Each phase lists core objectives, success criteria, and status checkpoints to make progress measurable and transparent.

## Milestone 1: Tokenizer Revamp

**Objective:** Replace the legacy tokenization pipeline with a modular, language-agnostic tokenizer that can adapt to new data sources without manual intervention.

- Components to deliver:
  - Vocabulary ingestion pipeline with pluggable normalization filters.
  - Dynamic token frequency tracking for feedback-driven pruning and expansion.
  - Streaming interface for incremental encoding/decoding.
- Status checkpoints:
  1. ✅ Baseline tokenizer benchmarked against current production metrics.
  2. 🔄 Prototype modular tokenizer integrated into the preprocessing stage.
  3. ⏳ Automated regression suite validating throughput and token fidelity across corpora.
  4. ⏳ Rollout plan finalized with rollback procedures and observability hooks.

## Milestone 2: Learner Upgrade

**Objective:** Introduce an adaptive learner capable of curriculum scheduling, multi-modal ingestion, and self-evaluated reinforcement to shorten convergence cycles.

- Components to deliver:
  - Curriculum scheduler that sequences tasks based on token rarity and performance deltas.
  - Multi-modal encoder adapters for text, audio, and structured data streams.
  - Self-evaluation loop that scores model outputs and routes corrective feedback.
- Status checkpoints:
  1. ✅ Research spike documenting architecture options and trade-offs.
  2. 🔄 Curriculum scheduler connected to the new tokenizer feedback signals.
  3. ⏳ Integration tests validating end-to-end learner updates across modalities.
  4. ⏳ Deployment readiness review covering infrastructure, monitoring, and rollback.

## Milestone 3: Autonomy Layers

**Objective:** Layer autonomous orchestration capabilities on top of the learner to plan, execute, and evaluate long-horizon tasks with minimal human oversight.

- Components to deliver:
  - Goal decomposition engine that transforms objectives into executable plans.
  - Execution manager coordinating tool use, environment interaction, and state checkpoints.
  - Reflection module capturing outcomes, hypotheses, and next actions for continuous improvement.
- Status checkpoints:
  1. 🔄 Architecture blueprint accepted by stakeholders.
  2. ⏳ Goal decomposition engine delivering plans with traceable rationales.
  3. ⏳ Closed-loop execution trials demonstrating autonomous task completion.
  4. ⏳ Operational dashboards reporting autonomy metrics and safety boundaries.

## Cross-Cutting Initiatives

- **Observability:** Extend tracing, logging, and metrics capture across tokenizer, learner, and autonomy layers.
- **Safety & Alignment:** Embed guardrails, policy checks, and human-in-the-loop overrides throughout.
- **Tooling:** Provide developer tooling for rapid experimentation, reproducible evaluations, and config management.

## Reporting Cadence

- Bi-weekly status updates mapped to the checkpoints above.
- Quarterly roadmap reviews to adjust sequencing and dependencies.
- Immediate risk reports triggered by regressions or missed checkpoints.

## Next Steps

- Finalize staffing assignments for in-flight checkpoints.
  - Owners submit detailed implementation plans for remaining ⏳ items.
- Establish shared dashboard highlighting roadmap status and key risks.
- Schedule autonomy layer architecture workshop to unblock checkpoint 3.1.
