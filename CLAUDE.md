# Claude Code - Emeraude Engine Quick Start

Welcome to Emeraude Engine! This guide helps AI agents quickly understand the project structure and find relevant information.

## 🎯 What is Emeraude Engine?

A modern **3D graphics engine** built with **Vulkan + C++20** providing:
- High-level graphics abstraction (OpenGL-style API over Vulkan)
- Automatic shader generation (Saphir system eliminates manual shader variants)
- Fail-safe resource management (never crashes on missing assets)
- Integrated physics, audio, scene graph, and UI systems

**License:** LGPLv3 | **Platforms:** Windows 11, Linux, macOS

---

## 🚀 First Steps for New AI Agents

### 1. Read the Main Documentation
Start here: **@AGENTS.md** - Complete project overview, conventions, and critical rules

### 2. Understand Critical Concepts
These are **non-negotiable** rules that affect all development:

- **Y-down coordinate system** (@docs/coordinate-system.md)
  - Gravity = +9.81 along Y-axis
  - Used in Physics, Graphics, Audio, Scenes
  - **NEVER flip Y coordinates** - breaks everything

- **Fail-safe resource management** (@docs/resource-management.md)
  - Resources never return nullptr
  - Missing/broken assets → neutral fallback resources
  - Application never crashes on asset errors

- **Vulkan abstraction** (@src/Vulkan/AGENTS.md + @src/Graphics/AGENTS.md)
  - **NEVER call Vulkan directly** from user code
  - Use Graphics abstractions (Geometry, Material, Renderable)
  - Vulkan complexity hidden behind declarative interface

- **Saphir shader system** (@docs/saphir-shader-system.md)
  - Shaders generated automatically from Material + Geometry
  - No manual shader files (eliminates combinatorial explosion)
  - Strict compatibility checking (Material requirements ↔ Geometry attributes)

### 3. Navigate by Task

**"I'm adding a new visual effect"**
→ @src/Graphics/AGENTS.md → @src/Saphir/AGENTS.md → @docs/graphics-system.md

**"I'm debugging physics collisions"**
→ @src/Physics/AGENTS.md → @docs/physics-system.md → @docs/coordinate-system.md

**"I'm loading a new resource type"**
→ @src/Resources/AGENTS.md → @docs/resource-management.md

**"I'm working on scene hierarchy"**
→ @src/Scenes/AGENTS.md → @docs/scene-graph-architecture.md

**"I'm adding 3D audio"**
→ @src/Audio/AGENTS.md → @docs/coordinate-system.md

**"I need to understand the architecture"**
→ @AGENTS.md (overview) → Subsystem AGENTS.md files → @docs/ (deep dives)

---

## 📂 Documentation Hierarchy

```
CLAUDE.md (you are here)
    ↓
AGENTS.md - Project overview, conventions, all critical rules
    ↓
Subsystem AGENTS.md files - Contextual details per system
    ↓
@docs/*.md - Deep technical documentation
```

---

## 🗂️ Subsystem Quick Reference

### Framework Core
| System | Purpose | Link |
|--------|---------|------|
| **Core Components** | Core, Tracer, Window, Settings, Arguments, Services | @src/AGENTS.md |

### Core Rendering
| System | Purpose | Link |
|--------|---------|------|
| **Vulkan** | Graphics API abstraction (Device, Buffer, Pipeline, etc.) | @src/Vulkan/AGENTS.md |
| **Graphics** | High-level rendering (Geometry, Material, Renderable, Renderer) | @src/Graphics/AGENTS.md |
| **Saphir** | Automatic GLSL shader generation (eliminates manual shaders) | @src/Saphir/AGENTS.md |

### Scene & Entities
| System | Purpose | Link |
|--------|---------|------|
| **Scenes** | Scene graph (Nodes, StaticEntity, Components, Observer pattern) | @src/Scenes/AGENTS.md |
| **AVConsole** | Audio-Video console (Camera/Microphone to render targets) | @src/Scenes/AVConsole/AGENTS.md |

### Simulation & Interaction
| System | Purpose | Link |
|--------|---------|------|
| **Physics** | Physics simulation (4-entity system: Boundaries, Ground, Static, Dynamic) | @src/Physics/AGENTS.md |
| **Audio** | 3D spatial audio (OpenAL-based, Y-down positioned) | @src/Audio/AGENTS.md |
| **Input** | Input management (keyboard, mouse, gamepads - polling + events) | @src/Input/AGENTS.md |

### Assets & Resources
| System | Purpose | Link |
|--------|---------|------|
| **Resources** | Async loading with fail-safe fallbacks (never returns nullptr) | @src/Resources/AGENTS.md |
| **Net** | Network resource downloading (integrates with Resources system) | @src/Net/AGENTS.md |

### UI & Platform
| System | Purpose | Link |
|--------|---------|------|
| **Overlay** | 2D overlay system (ImGui, HUD, debug tools) | @src/Overlay/AGENTS.md |
| **PlatformSpecific** | OS-specific code isolation (Windows, Linux, macOS) | @src/PlatformSpecific/AGENTS.md |

### Foundation
| System | Purpose | Link |
|--------|---------|------|
| **Libs** | Foundational libraries (Math, IO, ThreadPool, JSON, Compression, Hash) | @src/Libs/AGENTS.md |

### Development Tools
| System | Purpose | Link |
|--------|---------|------|
| **Testing** | Unit tests (Google Test framework, 100% Libs coverage target) | @src/Testing/AGENTS.md |
| **Animations** | Skeletal animations (in development) | @src/Animations/AGENTS.md |
| **Console** | Command console (in development) | @src/Console/AGENTS.md |
| **Tool** | Utility tools (rarely used) | @src/Tool/AGENTS.md |

---

## 📚 Deep Technical Documentation

Located in `@docs/` directory:

| Document | Topic | When to Read |
|----------|-------|--------------|
| **coordinate-system.md** | Y-down coordinate system (CRITICAL) | Before any graphics/physics/audio work |
| **graphics-system.md** | Graphics architecture (instancing, Renderer, subsystems) | When working on rendering |
| **saphir-shader-system.md** | Shader generation pipeline | When adding materials or visual effects |
| **physics-system.md** | 4-entity physics architecture | When implementing physics features |
| **resource-management.md** | Async loading and fail-safe fallbacks | When adding new resource types |
| **scene-graph-architecture.md** | Entity-component scene organization | When working with scene hierarchy |

---

## 🛠️ Build & Test Quick Commands

```bash
# Initial setup
git submodule update --init --recursive

# Build (Debug with tests)
mkdir -p .claude-build-debug && cd .claude-build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug -DEMERAUDE_ENABLE_TESTS:BOOL=ON
cmake --build . --parallel $(nproc)

# Build (Release without tests)
mkdir -p .claude-build-release && cd .claude-build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --parallel $(nproc)

# Run tests
cd .claude-build-debug
ctest --parallel $(nproc) --output-on-failure          # All tests
ctest --parallel $(nproc) --output-on-failure -R Math  # Math tests only

# Custom slash commands (if in .claude/commands/)
/build-test           # Build Debug + run tests
/quick-test Physics   # Incremental build + Physics tests
/full-test            # Clean rebuild + all tests
/build-release        # Build Release library
/build-only           # Build without running tests
```

See `.claude/commands/README.md` for all available slash commands.

---

## ⚠️ Common Pitfalls for New Agents

### 1. Coordinate System Confusion
❌ **Wrong:** Flipping Y coordinates to "fix" upside-down rendering
✅ **Correct:** Engine is Y-down everywhere; check projection matrices and viewport setup

### 2. Calling Vulkan Directly
❌ **Wrong:** `vkCmdBindPipeline(commandBuffer, ...);`
✅ **Correct:** Use `Renderable::create(geometry, material);` (Graphics handles Vulkan)

### 3. Expecting nullptr on Failure
❌ **Wrong:** `if (resource == nullptr) { /* handle error */ }`
✅ **Correct:** Resources never null; failure → neutral fallback (check logs for diagnostics)

### 4. Material/Geometry Incompatibility
❌ **Wrong:** Using normal map material with geometry that has no tangents
✅ **Correct:** Material requirements MUST match Geometry attributes (Saphir checks this)

### 5. Loading Resources in Render Loop
❌ **Wrong:** `for (int i = 0; i < 1000; ++i) { auto r = resources.get("tree"); }`
✅ **Correct:** Load once, reuse: `auto r = resources.get("tree"); for (...) { use(r); }`

---

## 🎓 Learning Path Recommendations

**Beginner (First 30 minutes):**
1. Read @AGENTS.md completely
2. Skim @docs/coordinate-system.md (understand Y-down)
3. Skim @docs/resource-management.md (understand fail-safe)

**Intermediate (Working on specific system):**
1. Read relevant subsystem @src/[system]/AGENTS.md
2. Read referenced @docs/ files for deep dives
3. Check code examples in AGENTS.md files

**Advanced (Architecting new features):**
1. Read multiple subsystem AGENTS.md files (understand integration)
2. Deep dive into @docs/ for architectural details
3. Cross-reference between systems (e.g., Graphics ↔ Saphir ↔ Resources)

---

## 🔍 Quick Search Strategies

**Looking for a concept:** Use Grep tool with pattern matching across AGENTS.md files
**Looking for implementation:** Check subsystem AGENTS.md → referenced source files
**Looking for examples:** AGENTS.md files contain pattern examples for each system
**Understanding dependencies:** Each AGENTS.md lists "Systèmes liés" (related systems)

---

**Always start with @AGENTS.md for the complete picture!**
