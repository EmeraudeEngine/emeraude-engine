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

### Verified on Windows (Window-PA, RTX 3500 Ada, MSVC, engine 74a1edc5, 2026-10-16)

- 34 services, the 34th being `AudioExternalInputService` (a microphone exists there; the Linux runs use
  `ALSOFT_DRIVERS=null`, no capture device):
  - 20 clean refusals (exit 1) and 14 clean degradations (exit 0);
  - every injection hit, no hang (the 120 s watchdog never fired);
  - 0 validation message, 0 VUID, 0 ASan error, under ASan and in a plain build.
  - Durations: base-level refusals < 0.3 s; core-level refusals 2-4 s plain, 6-11 s under ASan (the Vulkan instance
    and device creation with the validation layer, ~2 s, doubled by ASan); nothing waits on a timeout.
- projet-alpha (CEF), plain build with the option, 5 services (UI manager, overlay, renderer, instance, notifier):
  - exit 0 or 1, never a refusal dialog (an injected instance failure is not told as a driver refusal), 0 VUID;
  - the shutdown 1.2-1.4 s when the application came up.
- MSVC AddressSanitizer — Window-PA's build, the switch's MSVC branch since 2026-10-16 (`EMERAUDE_ENABLE_ASAN` adds
  these flags to the engine target, PUBLIC; Window-PA set them in `CMAKE_C/CXX_FLAGS`):
  - `CMAKE_C/CXX_FLAGS` add `/fsanitize=address /Z7 /D_DISABLE_VECTOR_ANNOTATION /D_DISABLE_STRING_ANNOTATION`;
  - `/DEBUG` on the executable and shared linker flags;
  - `clang_rt.asan_dynamic-x86_64.dll` (the VC bin directory) on `PATH` at run time.
  - `/Z7` is required: without debug information MSVC warns C5072, fatal under `/WX`.
  - The two defines avoid the annotation mismatch with the prebuilt, uninstrumented static libraries.
  - ⚠️ MSVC ASan has no LeakSanitizer: the leak half of the sweep has no Windows equivalent.

### Verified on macOS (macOS-PA, Mac mini M6, AppleClang ASan, engine 74a1edc5, 2026-10-16)

- 33 services, the same split as Linux: 20 clean refusals (exit 1), 13 clean degradations (exit 0); every injection
  hit; every run under 2.1 s; 0 validation message (the layer proven loaded with `VK_LOADER_DEBUG=layer` in every run
  that reaches the instance), 0 ASan error.
- projet-alpha (CEF), plain build with the option, the same 5 services as Windows: exit 0 or 1, never a refusal
  dialog, 0 VUID.
- `EMERAUDE_ENABLE_ASAN` (and the engine's other `EMERAUDE_ENABLE_*SAN` switches) was a NO-OP on macOS and Windows:
  the block sat in the Linux branch of the engine's `CMakeLists.txt`; that run instrumented by hand
  (`-fsanitize=address -fno-omit-frame-pointer -g` in the C / C++ / Objective-C flags and the linker flags). Since
  2026-10-16 (owner decision) the switches have their own section after the platform branches: macOS ASan / TSan /
  UBSan, MSan refused; MSVC ASan only (Window-PA's recipe below), the others refused. (emeraude-base's own
  `EMERAUDE_ENABLE_SANITIZERS` / `EMERAUDE_ENABLE_TSAN` always applied on macOS: the TSan runs of `source-tree/07`
  went through them.)
- ⚠️ AppleClang has NO LeakSanitizer on arm64: `ASAN_OPTIONS=detect_leaks=1` aborts every process at start (exit 134,
  "detect_leaks is not supported on this platform"). Run with `detect_leaks=0`: the leak half of the sweep needs
  Homebrew LLVM (its LeakSanitizer works) or stays a Linux check.
- ⚠️ The validation layer is shipped in the bundle (`Contents/Resources/vulkan/explicit_layer.d`): a tool built
  outside the bundle does not find it, and the engine warns "No shipped layer directory in VK_ADD_LAYER_PATH". Put that
  directory in `VK_ADD_LAYER_PATH` for such a tool, and prove the load with `VK_LOADER_DEBUG=layer`; without it,
  "validation 0" proves nothing. (The warning named only the missing call until 2026-10-16.)
- The first launch of a NEW projet-alpha binary restored the 41 MB pipeline cache in 114.5 s (a cold Metal shader
  cache), and `Core.shutdown()` at 10 s waited for it: exit 0 after 117 s. That is the uninterruptible restore of owner
  decision D3 (engine `docs/subsystems/vulkan/18-vkpipelinecache-the-driver-cache-the-engine-now-owns.md`), not a
  hang; the second launch shut down in 0.2 s. A macOS run with a watchdog under 120 s must use its own
  `--cache-directory` (no stored blob, no restore).
- Every shutdown logged `AL_INVALID_NAME` at `GlobalReleaseFlush` (Linux too): the audio sources and buffers were
  deleted after their context and device. Fixed 2026-10-16 (engine `docs/subsystems/audio/06-critical-points.md`).

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

