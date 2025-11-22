---
description: Cherche où un concept/classe est utilisé dans le codebase
---

Find all usages of a given concept, class, or pattern in the Emeraude Engine codebase.

**Task:**
1. Use Grep to search for the concept in source files (.cpp, .hpp, .md)
2. Categorize results by subsystem (Physics/, Graphics/, Scenes/, etc.)
3. Show file paths with line numbers
4. Identify key usage patterns:
   - Class inheritance
   - Member variables
   - Function calls
   - Documentation references

**Search locations:**
- `src/` - All subsystems
- `*.md` files - Documentation references
- Include both exact matches and related patterns

**Output format:**
```
Concept: [name]

Found in [N] files across [M] subsystems:

**[Subsystem]:**
- file.cpp:123 - [context]
- file.hpp:45 - [context]

**Key patterns:**
- Inheritance: [list]
- Usage: [summary]
```

**Example:**
User: `/find-usage CartesianFrame`
Output: Show usage in Scenes (transformations), Physics (positions), Audio (3D positioning), Math (definition)
