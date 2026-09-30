# clang-tidy ledger — emeraude-engine

> Owner decision 2026-09-30, plan Ave Robustus § 3 (projet-alpha `docs/plans/ave-robustus.md`). The configuration is
> this repository's `.clang-tidy`. Gate on every change: ZERO NEW finding on the touched translation units; a finding
> is fixed, never silenced. A FULL run is a long process ordered by the owner; its results are recorded here.

## How to run

```bash
# One translation unit, with the compile database of a Claude build directory (never cmake-build-*):
clang-tidy-19 -p /mnt/bunker/studio/dev/ln-isle/projet-alpha/.claude-build-release <file.cpp> > tidy.log 2>&1; echo EXIT=$?
```

Redirect, never pipe. Count the findings by check (`[check-name]` at the end of each warning line).

## Last full run per module

| Module | Date | Findings by check | Notes |
|---|---|---|---|
| — | — | no full run recorded yet under this ledger | |

## Findings kept ON PURPOSE

Each with its file:line, its check and the reason (an owner decision).

Known on purpose before this ledger: Saphir — the six warnings left on purpose
(`docs/subsystems/saphir/25-clang-tidy-the-six-warnings-that-are-left-on-purpose.md`).

_None recorded yet._
