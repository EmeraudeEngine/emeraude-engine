---
description: Affiche le AGENTS.md d'un sous-système avec résumé des points critiques
---

Read and summarize the AGENTS.md file for the requested subsystem.

**Available subsystems:**
- physics, graphics, vulkan, saphir, resources
- scenes, audio, input, overlay
- net, platformspecific, libs, testing
- animations, console, tool, avconsole

**Task:**
1. If no system specified, list available systems above
2. Read @src/{SystemName}/AGENTS.md (adjust case: Physics, Graphics, etc.)
3. Display:
   - 🎯 Vue d'ensemble
   - 🚨 Points critiques (section "Points d'attention")
   - 📚 Références docs associées
   - 🔗 Systèmes liés

**Format output concisely** - focus on critical info for quick reference.

**Example:**
User: `/doc-system physics`
Output: Show Physics overview, Y-down rules, 4-entity types, critical warnings, links to physics-system.md
