## 8. Code Generation Directives

> [!CRITICAL]
> These rules are **NON-NEGOTIABLE** for any AI-generated code in this engine.

### Professional Standard
This engine targets **production-grade real-time visual quality** as its benchmark.
Every generated implementation must meet professional-grade quality. No shortcuts,
no "good enough" approximations, no deferred-quality patterns. The AI is the primary
implementor — the human architect directs, the AI executes and measures.

### Architectural Contract Compliance
All generated code must comply with established contracts:
- **POLA / Pit of Success / No Gulf** design axioms
- **Service interfaces** for engine subsystems
- **Observer/Observable** for loose coupling
- **RAII** for all resource lifetimes
- **Vulkan abstraction** (never direct Vulkan calls)

When a contract is missing or insufficient, **the contract must be created or
improved** as part of the implementation — not bypassed.

### No Workarounds — Industrial-Grade Integration Only
Emeraude Engine is held to an **industrial standard**. It must be implementable
as a Khronos Group showcase application. This means **zero tolerance for
hacks, workarounds, or ad-hoc solutions**.

When integrating with professional tools (RenderDoc, Vulkan Validation Layers,
profilers, etc.), **use their official integration path** — Vulkan layers,
documented APIs, proper manifests. Never resort to `LD_PRELOAD` injection,
manual symbol lookups, environment variable tricks, or any approach that a
Khronos engineer would reject in a code review.

When something doesn't work, **diagnose the real cause** (missing layer
manifest, wrong library path, incorrect API usage) instead of piling on
workarounds. Read the tool's documentation and understand its architecture
before writing a single line of integration code.

### Mandatory Decision Escalation
When facing architectural choices (multiple valid approaches, unclear trade-offs,
or potential contract changes), **ALWAYS escalate to the user** with:
1. Clear description of each option
2. Constraints and trade-offs per option
3. Recommendation with justification

Never make autonomous architectural decisions. The project owner decides.

### Documentation First — No Silent Changes

> [!CRITICAL]
> **AI documentation maintenance is MORE IMPORTANT than the code.**

Every code change **MUST** be accompanied by its documentation update **in the same
work session**. This is not optional, not "nice to have" — it is the **highest priority
deliverable**.

**The rule:**
- Before reporting a task as complete, verify that all affected `AGENTS.md` and `docs/`
  files reflect the change.
- **Explicitly tell the user** what documentation was updated: file paths, what changed, why.
- If a change affects the Architecture Map, Link Index, or cross-references — update them.
- If a new subsystem, pattern, or API is introduced — document it immediately.
- If an existing pattern changes — update every doc that references the old pattern.
- **Never assume** the user will remember to ask for doc updates. Signal proactively.

**Active enforcement — the AI MUST remind the user:**
If the user tries to move on to the next task without documentation being updated,
the AI **MUST push back** and insist. The argument: *"Without this doc update, the
next AI session will misunderstand this area of the project, make wrong assumptions,
and waste your time. Updating now takes 2 minutes. Reverse-engineering later costs hours."*
Do not be passive about this. Actively remind, actively propose doc updates, actively
flag when documentation is stale or missing after a change.

**Why this is non-negotiable:**
The `AGENTS.md` network is the **only persistent context** an AI has across sessions.
Code without documentation forces every future AI session to reverse-engineer intent,
architecture, and constraints from scratch. This wastes the user's time, introduces
regression risk, and degrades the quality of AI assistance over time. The documentation
IS the engine's memory.

> **Since 2026-09-27 every `AGENTS.md` is a ROUTER**: "update the AGENTS.md" means update the `docs/` file the
> router points to (`docs/agents/`, `docs/subsystems/<area>/`, or a topic file) and add a router row only when a
> docs file is created. Knowledge never goes back into an `AGENTS.md`.
