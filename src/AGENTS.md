# Core Framework Components


Context for the base components at the root of src/ in Emeraude Engine.


## Where the content is (router)

> ⚠️ This file is a ROUTER since 2026-09-27: its sections were moved VERBATIM to
> `docs/subsystems/source-tree/`, one file per topic (a very large section is a folder, one file per
> sub-topic). **Read only the file(s) matching the task** — never the whole folder. A reference
> elsewhere to "`src/AGENTS.md` § <Section>" means the row of that title below.

| Section | File | Size |
|---|---|---|
| Overview | [`docs/subsystems/source-tree/01-overview.md`](../docs/subsystems/source-tree/01-overview.md) | 1 KB |
| Main Components | [`docs/subsystems/source-tree/02-main-components.md`](../docs/subsystems/source-tree/02-main-components.md) | 35 KB |
| Development Patterns | [`docs/subsystems/source-tree/03-development-patterns.md`](../docs/subsystems/source-tree/03-development-patterns.md) | 4 KB |
| Critical Points | [`docs/subsystems/source-tree/04-critical-points.md`](../docs/subsystems/source-tree/04-critical-points.md) | 1 KB |
| Subsystem Integration | [`docs/subsystems/source-tree/05-subsystem-integration.md`](../docs/subsystems/source-tree/05-subsystem-integration.md) | 1 KB |
| Additional Documentation | [`docs/subsystems/source-tree/06-additional-documentation.md`](../docs/subsystems/source-tree/06-additional-documentation.md) | 1 KB |
| Dedicated threads stop through a `std::stop_token` (every `Base::Thread` owner, the `Core` loops' `m_loopsStop`, the defects found; Oct 2026) | [`docs/subsystems/source-tree/07-dedicated-threads-stop-tokens.md`](../docs/subsystems/source-tree/07-dedicated-threads-stop-tokens.md) | 5 KB |
| Startup fault injection (`EMERAUDE_FAULT_INJECTION`, `--inject-startup-failure=<service>`, `EmeraudeStartupFaultInjection`, `tools/stress/startup-fault-sweep.sh`; the defects the first sweep found; Oct 2026) | [`docs/subsystems/source-tree/08-startup-fault-injection.md`](../docs/subsystems/source-tree/08-startup-fault-injection.md) | 5 KB |
| The shutdown sequence (the stop requests and the veto, the joins, `terminate()`'s order, `~Core()`; why `ThreadPool::wait()` is not a shutdown tool; Oct 2026) | [`docs/subsystems/source-tree/09-shutdown-sequence.md`](../docs/subsystems/source-tree/09-shutdown-sequence.md) | 4 KB |
