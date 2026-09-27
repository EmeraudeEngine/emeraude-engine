## ShaderManager Include Contract (compile-time)

> [!IMPORTANT]
> **`ShaderManager.hpp` must never expose glslang or `Program`.** Both leaks were
> removed on 2026-07-27; re-introducing either one silently multiplies compile time
> across the whole engine.

`ShaderManager.hpp` is included by 21 translation units, one of which is
`Graphics/Renderer.hpp` — itself included by 81 more (7 of them headers that
repropagate it). Anything reachable from `ShaderManager.hpp` is therefore paid for
by most of the engine. Two rules keep it cheap:

1. **`Saphir::Program` is forward-declared, never included.** `getShaderModules()`
   only takes `const std::shared_ptr< Program > &`, which needs a declaration, not a
   definition. `Program.hpp` transitively drags in every shader class, `SetIndexes`,
   `Graphics/VertexBufferFormatManager.hpp` and `Vulkan/GraphicsPipeline.hpp`
   (~70 headers / 25.7k lines). Consumers that need the complete type include
   `Saphir/Program.hpp` themselves.
2. **The glslang state lives behind a pimpl.** `struct ShaderManager::GLSLangContext`
   (declared in the header, defined in `ShaderManager.cpp`) holds `TBuiltInResource`,
   `DirStackFileIncluder`, `EProfile`, `EShMessages` and the target version. Keeping
   them as by-value members forced `<glslang/Public/ShaderLang.h>` into every
   consumer. The Saphir→GLSLang shader-type conversion is a **file-local free
   function** in the `.cpp` for the same reason: its `EShLanguage` return type alone
   pulled the glslang public header in.

Consequences to respect when editing this class:

- The constructor and destructor are **defined out-of-line** in `ShaderManager.cpp`.
  This is mandatory: `std::unique_ptr< GLSLangContext >` needs the complete type for
  its deleter. Same pattern as `Graphics::Renderer` — see
  [`docs/windows-export-api.md`](../../windows-export-api.md) § "exported pimpl".
- Declaring the destructor suppresses the implicit move constructor; `ShaderManager`
  is a by-value member of `Renderer` and is direct-initialized, so this is fine — do
  not add a move that would require the complete type in the header.
- Measured effect: `ShaderManager.hpp` went from 75 project headers / 26 570 lines to
  **10 / 2 506**; `Graphics/Renderer.hpp` from 127 / 47 064 to **87 / 38 624**.
- The PCH masks missing includes. After touching this header, verify a TU that used
  to rely on the transitive `Program.hpp` still compiles with
  `-DEMERAUDE_ENABLE_PCH=Off`.
