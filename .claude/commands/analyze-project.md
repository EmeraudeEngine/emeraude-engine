---
description: Analyze the Emeraude Engine project structure and conventions
---

Read and analyze the main project documentation to understand the Emeraude Engine architecture, conventions, and development guidelines.

**When to use this command:**
- Starting a new work session on the project
- Need to understand project structure
- Before making significant changes
- When asked about project conventions or architecture

## Instructions

1. Read the main AGENTS.md file at the project root to understand:
   - Project overview and architecture
   - Critical conventions (Y-down coordinate system, fail-safe resources, Vulkan abstraction)
   - Development patterns and guidelines
   - Subsystem organization

2. Provide a concise summary of key points relevant to the current task.

## Main Documentation File

Read: `AGENTS.md`

This file contains:
- Engine architecture overview
- Critical conventions that must be followed
- Subsystem descriptions and their AGENTS.md locations
- Development patterns and best practices
- Build and test instructions

## Related Documentation

For deeper understanding of specific subsystems, the following AGENTS.md files are available:
- `src/AGENTS.md` - Core framework components
- `src/Graphics/AGENTS.md` - Graphics and rendering system
- `src/Vulkan/AGENTS.md` - Vulkan abstraction layer
- `src/Physics/AGENTS.md` - Physics system
- `src/Scenes/AGENTS.md` - Scene graph system
- `src/Resources/AGENTS.md` - Resource management
- `src/Audio/AGENTS.md` - Audio system
- `src/Input/AGENTS.md` - Input handling
- `src/Overlay/AGENTS.md` - 2D overlay system
- `src/Saphir/AGENTS.md` - Shader generation
- `src/Libs/AGENTS.md` - Utility libraries

## Critical Conventions Summary

After reading, always remember:
1. **Y-Down**: +Y is down, gravity = +9.81
2. **Fail-Safe**: Resources never return nullptr, always provide neutral fallback
3. **Vulkan Abstraction**: Never call vk* directly outside src/Vulkan/
4. **Composition**: Use components, never subclass Node/StaticEntity