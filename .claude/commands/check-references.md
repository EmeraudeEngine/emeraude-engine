---
description: Vérifie que toutes les références @docs/ et @src/ dans AGENTS.md sont valides
---

Verify that all documentation cross-references in AGENTS.md files are valid and up-to-date.

**Task:**

1. **Find all AGENTS.md files:**
   - Root AGENTS.md
   - All src/*/AGENTS.md files

2. **Extract all references:**
   - `@docs/[filename].md` patterns
   - `@src/[path]/AGENTS.md` patterns

3. **Verify each reference:**
   - Check file exists at specified path
   - For @docs/ refs: verify file in ./docs/
   - For @src/ refs: verify AGENTS.md exists

4. **Check for orphaned docs:**
   - List all .md files in ./docs/
   - Verify each is referenced at least once in an AGENTS.md
   - Flag unreferenced docs

5. **Check for broken links:**
   - Verify all subsystem cross-references are bidirectional when appropriate
   - E.g., if Graphics references Saphir, does Saphir reference Graphics back?

**Output format:**
```
📚 Reference Check Results

✅ Valid references: [N]
⚠️ Broken references: [M]
  - src/Graphics/AGENTS.md:110 - @docs/graphics-system.md (NOT FOUND - marked "à créer")
  - src/Foo/AGENTS.md:50 - @src/Bar/AGENTS.md (NOT FOUND)

📄 Documentation coverage:
✅ All docs/ files referenced
OR
⚠️ Orphaned docs (not referenced):
  - docs/old-system.md

🔗 Cross-reference completeness:
✅ Bidirectional links complete
OR
⚠️ Missing reciprocal references:
  - Graphics → Saphir (exists)
  - Saphir → Graphics (missing)

Summary: [status]
```

Use Read and Glob tools to verify file existence. Be thorough but concise in output.
