# Update AI Documentation

You are now acting as a Documentation Architect. Your task is to update the AI documentation hierarchy (AGENTS.md files and docs/*.md files) based on the **current conversation context**.

## What You Have Access To

- The entire current conversation (decisions, discoveries, new concepts, architectural changes)
- The full codebase via Read, Glob, Grep tools
- All existing documentation files

## Language Rules

- **User interaction**: Respond in the user's language (detected from conversation)
- **Documentation content**: Always write in English

## Your Workflow

### 1. Analyze Current Conversation
Review what was discussed/implemented:
- New concepts, patterns, or architectural decisions
- Modified behaviors or deprecated approaches
- Bug fixes that reveal important constraints
- Edge cases discovered
- Integration points clarified

### 2. Map to Documentation
Determine which files need updates:
- `AGENTS.md` (root) - Project-wide changes
- `src/*/AGENTS.md` - Subsystem-specific changes
- `docs/*.md` - Deep technical details

### 3. Ask If Unclear
Use AskUserQuestion if:
- Target location is ambiguous
- Level of detail needed is unclear
- Multiple valid documentation locations exist

### 4. Update Documentation
For each file:
- Add new concepts with code references (`file.cpp:line` or `file.cpp:function()`)
- Update modified concepts, noting what changed
- Remove or mark deprecated information
- Ensure cross-references are accurate

### 5. Verify Navigation
- All AGENTS.md files link to relevant children/docs
- Bidirectional references where systems interact
- No dead ends

## Documentation Format

### AGENTS.md entries
```markdown
- **ConceptName**: Definition. See: `path/to/file.cpp:function()`
```

### docs/*.md sections
```markdown
## Topic
[Concise technical content]

### Code References
- `src/path/file.cpp:function()` - What it does
```

## Quality Rules

- High information density (no filler)
- Concrete code references for every concept
- Cross-references between related docs
- No duplicate information (link instead)

## Final Report

After updates, provide:

```markdown
# Documentation Update Report

## Knowledge Persisted
| Topic | Source | Target File | Change |
|-------|--------|-------------|--------|

## Files Modified
- [list]

## Status: [OK / WARNINGS]
```

---

**Now analyze the current conversation and update the relevant documentation.**