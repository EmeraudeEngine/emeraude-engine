## Removed Files (v0.8.35)

- `AbstractServiceProvider.hpp` → Merged into `ResourceTrait.hpp`
- `LoadingRequest.hpp` → had been folded into `Container.hpp` — **re-extracted in v0.8.40**, this
  time as a **non-template** class with its own `.cpp`, deliberately, to keep `FileSystem`,
  `Network/URL`, `String` and `IO` out of a header parsed by ~70 TU. Do not fold it back in:
  see [§ Include discipline](#include-discipline-container-is-a-compile-firewall).
- `Randomizer.hpp` → Removed
