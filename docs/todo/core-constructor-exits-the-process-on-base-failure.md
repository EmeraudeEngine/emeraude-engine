---
id: core-constructor-exits-the-process-on-base-failure
title: Core's constructor ends the process (quick_exit(2), std::terminate() on macOS) when the base level fails
status: open
priority: high
scope: src/Core.cpp (Core::Core, initializeBaseLevel())
opened: 2026-10-11
tags: [ave-robustus-ii, lifetime, defect]
---

# Core's constructor ends the process when the base level fails

## Why

`Core::Core()` calls `initializeBaseLevel()` and, when it returns false, `std::quick_exit(2)` — `std::terminate()` on
macOS ("clang 15 failure" comment). Every destructor is skipped (primary services, open files, the settings save),
the consumer's `main()` never gets control back, and on macOS the process ABORTS (a crash report for a refused
start). Ave Robustus II: a constructor yields a valid object, and every failure path exits cleanly.

Found 2026-10-11 through `--help`, which returned false from `initializeBaseLevel()`: the help was never printed and the
process exited with code 2 and no output (stdout lost by `quick_exit`). That case is fixed (help is a successful
start, `main()` prints it); the mechanism remains for real base-level failures (primary services, …).

## What remains — owner decision

Options: (1) the constructor records the failure (`m_startupMode = StartupMode::Error`, `willNotRun()` true) and the
consumer's `main()` returns a failure code — no process exit inside the engine; (2) a factory returning
`std::optional< Core >` / a status (a larger API change for every consumer); (3) keep an exit but a normal `std::exit`
after flushing, never `std::terminate()`.
Recommendation: (1) — `main()` already checks `willNotRun()` before anything else.

## References

- `src/Core.cpp` `Core::Core()` (both constructors), `initializeBaseLevel()`; projet-alpha `src/Boot/*/main.*`.
