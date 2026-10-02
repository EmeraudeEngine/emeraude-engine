---
id: rtti-key-function-audit
title: Audit the engine's polymorphic classes for a key function (macOS dynamic_cast across images)
status: open
priority: unranked
scope: every EMEN_API polymorphic class the application may construct, every dynamic_cast / dynamic_pointer_cast in the engine
opened: 2026-10-02
tags: [macos, rtti, abi]
---

# Audit the engine's polymorphic classes for a key function (macOS dynamic_cast across images)

## Why

Found by the macOS peer on 2026-10-02 (physics overhaul P4): `Component::CharacterController` had every virtual
inline, so the application that built it held its own hidden typeinfo, and the engine's
`std::dynamic_pointer_cast< CharacterController >` answered nullptr on macOS (libc++ compares type_info by address).
Fixed for that class by an out-of-line destructor (`docs/caution-points.md`, the macOS dynamic_cast entry). The same
pattern may hide elsewhere: the peer counted about 40 dynamic_cast / dynamic_pointer_cast sites in the engine.

## What remains

- [ ] List the engine's dynamic casts; for each target class, check it has an out-of-line virtual function (or is
  never constructed outside the engine). Give the missing ones an out-of-line destructor.
- [ ] Verify on macOS: `nm -m` on the framework and the app shows ONE external typeinfo per audited class.
- [ ] Consider a clang-tidy / compiler check (`-Wweak-vtables`) to keep it from coming back.

## References

- The Itanium C++ ABI "key function" rule; libc++ `_LIBCPP_TYPEINFO_COMPARISON_IMPLEMENTATION` (unique RTTI on Apple).
