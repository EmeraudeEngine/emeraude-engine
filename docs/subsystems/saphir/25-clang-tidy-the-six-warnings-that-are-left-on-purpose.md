## clang-tidy — the six warnings that are LEFT ON PURPOSE

Saphir is scanned with the project `.clang-tidy` (`run-clang-tidy -p .claude-build-release
"dependencies/emeraude-engine/src/Saphir/.*\.cpp"`). It went from 12 warnings to 6 in Aug 2026;
the remaining six are deliberate, so **do not "fix" them**:

- **`Declaration/Types.cpp` × 2 — `bugprone-branch-clone`.** The std140 alignment switch returns
  the same number for several type families, but each family carries its own comment stating the
  rule that justifies it (`vec2` → 8, `dvec2` → 16, `vec3`/`vec4` → 16…). Merging the branches
  would delete the explanation, which is the only reason the table is readable. The duplication
  is didactic.
⚠️ Everything else is fixed, never silenced — no NOLINT, no check disabled.

### The shader includer: there is none, on purpose

Saphir **generates** its GLSL: no source handed to glslang ever carries an `#include`, and no
include directory was ever registered (the single `pushExternalLocalDirectory()` call had been
commented out for as long as it existed). glslang nevertheless requires an `Includer` argument
for `preprocess()` and `parse()`, so `ShaderManager` passes glslang's own no-op
`glslang::TShader::ForbidIncluder`.

That replaced a copied `DirStackFileIncluder` (deleted Aug 2026) whose class name and comment
came verbatim from glslang's `StandAlone/` sample while the file carried only the Emeraude
header — a licensing loose end, four unfixable `cppcoreguidelines-owning-memory` warnings (the
Includer contract is raw-pointer based: glslang frees the `IncludeResult *` itself), and code
that was never once executed.

`ForbidIncluder` is also the safer behaviour: an `#include` appearing by accident now FAILS
instead of being silently resolved against an empty search stack.

⚠️ **The day hand-written GLSL sources become a thing** (see [`docs/todo/manual-glsl-sources.md`](../../todo/manual-glsl-sources.md), "Prepare a way to use
manual GLSL sources"), a real includer plugs in exactly there — written against an actual
specification (which directories, which search policy, what caching), not copied from a sample.
