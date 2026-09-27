## Attention Points

- **STRICT isolation**: Windows code NEVER touches Linux/macOS and vice-versa
- **No platform macros in .cpp/.mm files**: CMake selects files per OS
- **Cross-platform testing**: Test on all 3 OS before commit
- **Abstract API**: Common `.hpp` interface, separate implementations
- **CMake correctly configured**: Verify file selection per OS
- **Explicit warnings**: If feature unsupported, log clear warning
- **UTF-8 everywhere**: Convert at platform boundaries only

---
