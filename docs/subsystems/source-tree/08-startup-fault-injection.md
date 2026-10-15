## Startup fault injection (Ave Robustus II P3, Oct 2026)

Every failure path of the startup is run in turn: the startup of one named service is made to fail, and the run must
exit cleanly — no leak, no hang, no VUID. Owner decisions 2026-10-16: a CMake option plus an argument (never in a
release), and the measure before the fixes.

### The mechanism

- CMake `EMERAUDE_FAULT_INJECTION` (off by default; a TEST build only) → `EmEn::FaultInjectionEnabled`
  (`emeraude_config.hpp`).
- `--inject-startup-failure=<service name>` (`FaultInjection.hpp`), read once by `PrimaryServices` right after the
  arguments service (main process only; a CEF helper never injects).
- `ServiceInterface::initialize()` returns `false` for that service BEFORE its `onInitialize()`: the caller's failure
  path is what is tested. Compiled out of a normal build (`if constexpr`).
- Not injectable: the arguments service (read before the injection is configured), and the resource containers: their
  `initialize()` only reports their store and cannot fail, and the resource manager only logs it.

### The tools

- `EmeraudeStartupFaultInjection` (`tools/stress/StartupFaultInjection.cpp`, built with the option): a windowed, CEF-less
  engine that starts every service, runs one second and stops. It never shows the refusal dialog: its
  `onStartupRefused()` prints it.
- `tools/stress/startup-fault-sweep.sh <Release dir> <scratch dir> [jobs]`: one run per service of the reference run,
  each line giving the exit code, the duration, whether the injection hit, the validation messages, the ASan errors and
  the LeakSanitizer reports.
- The build (Linux): clang (GCC's `-Wmaybe-uninitialized` reports a false positive on `std::optional<std::function>`
  under ASan), ASan without UBSan (UBSan's `vptr` check needs the RTTI of the prebuilt glslang):

```bash
cmake -S . -B .claude-build-asan -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
	-DEMERAUDE_ENABLE_ASAN=On -DEMERAUDE_FAULT_INJECTION=On -DEMERAUDE_COMPILATION_DIR=$PWD/.claude-build-asan/Release
cmake --build .claude-build-asan --target EmeraudeStartupFaultInjection -j$(nproc)
dependencies/emeraude-engine/tools/stress/startup-fault-sweep.sh $PWD/.claude-build-asan/Release <scratch> 4
```

- `resources/lsan_suppressions.txt` (copied next to the binaries of an ASan / LSan build) names third-party system
  libraries only (fontconfig, dbus: global caches kept until exit). Never a frame of the cascade.
- Run on lavapipe (`VK_DRIVER_FILES`): the NVIDIA driver leaves one 56-byte allocation from a module already unloaded
  at exit ("unknown module", not suppressible by name; absent with lavapipe).

### Found and fixed by the first sweep (2026-10-16, Linux)

- A failed `ResourcesManagerService` was only logged: the startup went on, and the renderer's default resources
  dereferenced a missing container (an ASan SEGV). It is now fatal, like the console.
- A failed Vulkan instance, before `vkCreateInstance()` ran, was told to the user as "Graphics driver required". A
  renderer failing before any device selection was told as "Graphics hardware not compatible". Both runs waited on the
  modal dialog (killed at 120 s).
  - The diagnosis now needs evidence: `Instance::creationAttempted()` and `graphicsSelectionAttempted()`.
  - A real missing driver is still diagnosed: dialog, exit 97, verified.
- The Vulkan loader detected a sanitized build through `__SANITIZE_ADDRESS__` / `__SANITIZE_THREAD__`, which only GCC
  defines. Clang announces it through `__has_feature()`. A clang ASan build opened the loader with `RTLD_DEEPBIND`,
  which the ASan runtime refuses (`Vulkan/Loader.cpp`).

### Verified (Linux, clang 19 ASan, lavapipe, the validation layer, 2026-10-16)

33 services injected in turn, each run under 3 s:
- 20 essential ones refuse the startup cleanly (exit 1): the base level in the constructor, the core level in `run()`;
- 13 let the engine go on and stop normally (exit 0): settings, user, texture compressor and cache, post-processor,
  video input, recorders, physics, audio, track mixer, system notification, notifier;
- 0 validation message, 0 ASan error, 0 leak of the cascade.
- The reference run: every service up, exit 0, 0 leak of the cascade.

### Native handles in RAII holders (owner decision 2026-10-16: option A, defence in depth)

The sweep injects at the service level; a failure INSIDE an `onInitialize()` (after it acquired a native handle) is
the other half. A service whose `onInitialize()` fails is never terminated, so what it acquired must be released by its
members.
- `Audio/OpenALHandles.hpp`: `ALCDeviceHandle`, `ALCCaptureDeviceHandle`, `ALCContextHandle` (`std::unique_ptr` with a
  deleter; a context is made non-current before it is destroyed). The audio manager, the audio recorder and the
  external input hold their devices and contexts in them, each device declared before its contexts. `onTerminate()`
  still releases them in order with its checks and logs (`release()` for a close whose answer is logged).
  - Fixed through it: a failed context creation in the audio manager left the output device open. Proven with a
    test-only build: the context creation forced to fail, the holder closed the device at destruction ("closed a
    device: yes"). LeakSanitizer does not see this class: OpenAL Soft keeps its devices reachable in a global list.
  - Fixed in the audio recorder: a failure to open the output device destroyed the game context while it was the
    current one, without restoring the audio manager's context; one `releaseContexts()` now ends every failure branch
    and `onTerminate()`.
- `PlatformManager::GLFWLibrary`: owns `glfwInit()`, `glfwTerminate()` at its end, declared after the Vulkan loader
  (GLFW terminated before the loader closes). Fixed through it: a missing Vulkan support after `glfwInit()` left GLFW
  initialized.
- `Vulkan::Instance` already released in its destructor (P0, H3): unchanged.
- Verified after the change: the sweep again (33 runs, 0 issue); a projet-alpha rush with the voice-over (the output,
  loopback and capture devices closed with their logs; 181 pictures, the WAVs valid; 0 VUID).

