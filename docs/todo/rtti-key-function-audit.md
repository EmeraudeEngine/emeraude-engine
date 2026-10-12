---
id: rtti-key-function-audit
title: Key-function hygiene — enforce -Wweak-vtables on clang (the macOS cross-image RTTI defect itself is fixed)
status: open
priority: medium
scope: every polymorphic class of the engine (70) and of emeraude-base (9 + 4 in Testing) without an out-of-line virtual function; the clang warning set of the cascade
opened: 2026-10-02
tags: [macos, rtti, abi, warnings]
---

# Key-function hygiene — enforce -Wweak-vtables on clang

## Why

The defect this item was opened for is FIXED (owner decision 2026-10-12, options (A) + (C)): on macOS, `EMEN_API` and
`EMEN_LEAN_API` always expand to default visibility, so an application built with `-fvisibility=hidden` (CEF's flags)
no longer holds hidden copies of the engine's typeinfo; `Renderable::Abstract`, a `dynamic_pointer_cast` target, got
its key function. Root cause, measurements and the remaining exposure (unannotated types, emeraude-base): engine
`docs/caution-points.md` § "macOS: typeid, std::any and dynamic_cast across the engine boundary".

What is left is the option (B) the owner kept for later: a key function (one out-of-line virtual function, the
destructor `= default` in the .cpp) in EVERY polymorphic class. Benefits: ONE vtable and typeinfo per class instead of
one per image and per TU (smaller binaries, faster links), and no reliance on visibility for the polymorphic types. It
does NOT cover non-polymorphic types (a struct in a `std::any`): those depend on (A).

## Census (Apple M6, Apple clang, 2026-10-12, engine 9054be48)

`-fsyntax-only -Wno-everything -Wweak-vtables` over the 600 engine + base TUs of `compile_commands.json`: 0 failures,
84 unique sites before (C), 83 after.
- Engine (70 after (C)): Vulkan 12 (Device, Queue, CommandBuffer, DescriptorSet, the buffer objects, TextureInterface,
  AbstractObject, AbstractDeviceDependentObject), Physics 7 (the collision models, BodyPhysicalProperties,
  MovableTrait, CharacterWorldInterface), Scenes 6 (GroundLevelInterface, SeaLevelInterface, LocatableInterface,
  InfluenceAreaInterface, LightSet, GroundTriangleVisitor), Saphir 8, Help 4, Graphics 4 + Renderable 2 + Material 3 +
  Geometry 2, Vulkan/Sync 3, Overlay 3 (Surface, UIScreen, ImGUIScreen), Input 3, Resources 2, Audio 2, Animations 2,
  ServiceInterface, Tool, Scenes Loaders / Component / AVConsole, Desktop Dialog, Effects/Lighting 1 each.
- emeraude-base (9): IO::ByteStream, IO::FileStream, IO::MemoryStream, NameableTrait, Network::HTTPHeaders,
  Thread::TaskBase, Time::Elapsed::Abstract, Time::Elapsed::CPUTime, Time::Statistics::Abstract; plus 4 in Testing.

## What remains

- [ ] Owner: adopt (B) or not, and its scope (engine only, or base too).
- [ ] If adopted: one out-of-line destructor per listed class (a class without a .cpp needs one, or an existing TU),
  then `-Wweak-vtables` in the clang warning set, guarded: GCC rejects the option ("unrecognized command-line
  option"), MSVC has no equivalent.
- [ ] Verify: the census tool answers 0; `nm -m <app> | grep __ZTIN4EmEn` shows no engine typeinfo defined in the
  application for a polymorphic class.

## ⚠️ Traps

- A non-polymorphic class (no virtual function, e.g. `Renderable::SkeletalDataTrait`) has no key function at all: its
  typeinfo is emitted wherever it is used. Only (A) protects it across images.
- An out-of-line destructor on a class that had none changes nothing in layout, but the class needs a TU of its own
  or of its subsystem; the explicit source lists (`src/Net`, `src/Help`, `src/PlatformSpecific`, base tests) must be
  edited by hand.

## References

- The Itanium C++ ABI "key function" rule; libc++ `_LIBCPP_TYPEINFO_COMPARISON_IMPLEMENTATION` (non-unique RTTI bit
  on Apple arm64).
- Engine `src/emeraude_export.hpp` (the macOS expansion and its comment).
