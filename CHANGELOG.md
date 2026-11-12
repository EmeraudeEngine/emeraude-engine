# Development History Log

## Version 1.3.0 (2026-11-12)
 - Requires emeraude-base 1.6.0.
 - **Swap-chain failure paths.** A refused frame submission recovers; a degraded headless (window-less) swap-chain is recreated without querying a surface (it dereferenced the null window surface); a driver that creates more swap-chain images than the engine handles (5) is refused with an error naming the count, and the image count is checked again at its second query; a degraded swap-chain counts as a renderer debt, so an application rendering on demand keeps retrying it; a given-up swap-chain is no longer retried. libwayland's log goes to the Tracer.
 - **Exit code 95 when the swap-chain is given up for lack of video memory.** `Vulkan::Device::noteAllocationResult()` records the allocations refused with `VK_ERROR_OUT_OF_DEVICE_MEMORY`; `Renderer::isSwapChainGivenUpForVideoMemory()`; `Core::OutOfVideoMemoryExitCode` (95), `GraphicsFailure` (title, message, GPU and its memory), `Core::stopOnVideoMemoryExhaustion()`. The user is told before `onBeforeCoreStop()`, by the application or else by a native dialog. An integrated GPU's memory is described as shared with the system (`GraphicsFailure::sharedMemory`).
 - **Every runtime stop gives the application its last chance.** Video memory exhausted (95), device lost (98) and display lost (96) all call the new `Core::onGraphicsFailure()` hook after `setAppReadyToQuit()` and before `onBeforeCoreStop()`, so the application can save its data. `GraphicsFailure::Cause` gains `DeviceLost` and `DisplayLost`; `diagnoseGraphicsFailure(cause)` fills their texts.
 - Fault injection (test builds only): `--inject-frame-submit-failure`, `--inject-swap-chain-image-count=N`, `--inject-swap-chain-out-of-video-memory`, `--inject-device-loss`, `--inject-display-loss`, `--inject-runtime-failure-delay=N` (`Core::injectRuntimeFailure()`).
 - **GPU ordering of uploads.** An upload is ordered on the GPU before its readers and after the previous writes of the same object: `SynchInfo::waitsFor()` with timeline `SubmissionPoint`s, `Buffer::pendingWrites()` / `Image::pendingWrites()`, `Device::publishUploadPoint()`, `Device::uploadFrontier()`. Fixes a read-after-write hazard on single-queue devices (rejected submit, then a crash) and write-racing-write hazards between overlay region uploads.
 - GPU checkpoints (`VK_NV_device_diagnostic_checkpoints`) through the new `CommandBuffer::setCheckpoint()` on the frame stages, the sky, shadow maps, render-to-textures, every transfer and the overlay copies: a device-loss dump now localizes the fault.
 - Per-frame-in-flight resources: one material uniform-block region and descriptor set per frame in flight (properties under a lock), one RT light SSBO per frame in flight, one bindless IBL group per frame in flight.
 - Device selection: a translation layer (Mesa Dozen, Vulkan on Direct3D 12) never wins over the native driver of the same GPU (`PhysicalDevice::isTranslationLayer()`); the preferred-device match goes from the best-ranked device.
 - Window: a stored size with a zero side falls back to the default size with a warning instead of failing the startup; `Window::wasStoredSizeReplaced()` tells the application.
 - Child processes inherit nothing but their standard streams: the console and `TCPServer` sockets are non-inheritable (base `Network::preventSocketInheritance()`); `runProcessAndWait()` and the new `runProcessCapturingOutput()` close every other descriptor in the child; `runProcessCapturingOutput()` replaces `popen()` in `executeCommand()` and the Wi-Fi scan.
 - Windows: a `vulkan-1.dll` locked by another process is loaded again for 2 s, then the error names the lock.
 - Vulkan instance: a failed instance service terminates itself while the loader is still open; the unused present-rectangles query is removed (MoltenVK loader errors); the core-feature scan is one Info line.
 - Input: GLFW is woken only while it is initialized (`PlatformManager::postEmptyEvent()`). GLFW fork pinned at 27737a13 (Wayland HiDPI cursor frame index bounded).
 - Tracer: no direct `std::cout` / `std::cerr` left in the engine; VMA asserts and the Vulkan debug messenger go through the Tracer, with the message severity mapped (a loader warning is no longer an error).
 - **Temporal discontinuity contract.** TAA's and the GI anti-lag's large-change tests run only on the frames where the lighting stepped; the sky's IBL cycle runs in one frame on a large change and the renderer adopts it in the frame that recorded it; a light's enabled state is published per render state slot; the GI anti-lag is pooled over 5 x 5 and discards its history on a very large step. A time-of-day jump now changes the sky, the ambient and the lights in the same frame. `temporalCapture`'s JSON carries each frame's discontinuity mark.
 - Core: the application's logic (`onCoreProcessLogics()`) runs between the scene's logic and its publication, so its changes render from the same slot; `Scenes::Manager::withLogicCycle()` makes the cycle one unit against exclusive accesses. An exclusive access asked inside the cycle on its own thread is a contract fault (aborts).
 - Shutdown: the stop token goes through the resource completion chain (`ResourceTrait::LoadStopScope`); `Resources::Manager` requests every container's stop before waiting; a texture compression stops per block row; `AnimatedTexture2D` encodes its frames in parallel.
 - Lighting: translucent passes skip the lights the opaque resolve skipped; `RenderableInstance::Abstract::setPunctualLightingEnabled()` and `Scene::setSeaLevelPunctualLighting()`; a disabled main light no longer lights the atmospheric fog.
 - Physical sky: the ground albedo is live (`SkyAtmosphere::setGroundAlbedo()`, console `setDynamicSkyGroundAlbedo(r, g, b)`).
 - Caches keyed exactly through base `ExactKey`: the program cache, the pipeline cache, the samplers and the combine / denoise variants (a hash collision served a spot-shadow program to the sun's cascaded pass).
 - Post-process: the view block's fast forms for marching loops (SSGI builds both variants); `PostProcess.getStatus` names the view-reconstruction variant the frame recorded.
 - Physics: explicit centre of mass; a braked wheel and the tyre friction ellipse fixed; traction control and anti-lock brakes. The world boundaries hold kinematic characters.
 - Audio: follows base's `Frequency` strong type; the playback and capture frequency settings accept any rate in [1 kHz, 384 kHz] (a 22255 Hz file loads and is resampled once); a rate outside it keeps the 48 kHz default with a warning.
 - Net: `APIClient` notifications can be recorded from the console (`watchEvents()`, `events()`); `APIRequestStatus::Unknown` for a ticket never issued, released or evicted (it answered `Error`).
 - CMake: `APP_OSX_DEPLOYMENT_TARGET` defaults to 13.0 (was 12.0).
 - Overlay: a resident surface image is refreshed through the region upload only.
 - Saphir: the TBN debug view projects through the vertex stage's own MVP.

## Version 1.2.1 (2026-11-04)
 - Every error code's text goes through `Base::IO::systemErrorText()` (UTF-8 on Windows). Requires emeraude-base 1.5.4.
 - Windows: `StorageInfo` skips removable drives without media (an empty card reader made every probe log an error).

## Version 1.2.0 (2026-10-29)
 - **`CelestialClock` replaces `SunCourse` (API change).** `Scenes::Component::CelestialClock` places the sun and the moons from a planet, a latitude, a date and a time of day; `CelestialClock::simpleCourse()` reproduces the former `SunCourse`; `Toolkit::generateCelestialClock()` replaces `generateSunCourse()`; `SkyFollowsSun` binds a clock; the console's `CelestialClock` commands replace `SunCourse`'s.
 - **Real sky at a real instant.** `TimeModel::Calendar`: the Sun, the Moon (topocentric) and the precessed star frame at a UTC instant and place; `setInstant()`, `julianDay()`; console `setCalendarDate()`, `setNow()`, `setLongitude()`, `useSystemLocation`, `findObservationWindows`. `PlatformSpecific::approximateLocation()` derives the machine's place from its time zone. A negative time scale runs the sky backwards.
 - **Physical sky.** `Graphics::SkyAtmosphere` (Hillaire 2020 tables, planet-parametric `AtmosphereParameters` with Earth and Mars presets) drawn as the background of `DynamicSkyResource`; the sun's light through the sky's own atmosphere; aerial perspective; the sky's own IBL amortised over frames; tables in RGBA32F so the night no longer flushes to zero.
 - **Sky bodies.** Moons drawn with their light integrated over the disc; stars from the new `StarCatalogueResource` (data store `StarCatalogues`, format `EmeraudeStarCatalogue` v1) drawn as points; planets placed by the calendar, drawn as points, then as lit discs past 2 px.
 - **Scene-colour pre-exposure.** Every scene program stores radiance x the view's pre-exposure; the tone mapper and the luminance metering divide by it; histories rescale; setting `Core/Graphics/PostProcessing/PreExposure/Override` and `Core.RendererService.setPreExposureOverride()`.
 - GI anti-lag in `GIDenoiser`: a lighting change converges in a few frames; squared quantities stored as their root (no pre-exposure fireflies).
 - **Lights at city scale.** `AbstractLightEmitter::setDrawDistance(distance, fadeRange)`; visible lights beyond `MaxLights` are dropped, the farthest first, with a warning, instead of going to the forward passes; `SharedUniformBuffer::MaxBankCount` 64 -> 256; a light without a seat is refused alone at initialization instead of failing the whole scene.
 - Light glints: `AbstractLightEmitter::setVisibleSourceArea()` and the `LightGlints` effect draw a light's visible source as a point that never flickers (setting `Core/Graphics/PostProcessing/LightGlints/Enabled`).
 - HLOD: `Toolkit::generateHLODProxy()` builds one proxy for a multi-layer static mesh, drawn beyond a switch distance; shadow casters honour the draw distance range.
 - Render thread: per-instance readiness without locks (`Renderable::Abstract::programCacheGeneration()`); a draw of 0 instances is skipped; physics skips pairs of two static-world members.
 - Emission in the indirect diffuse: `StandardResource::setLightFace()` (JSON `LightFace`) marks the visible face of a light source, which never re-emits; RTGI adds a free emitter's emission. Material UBO 140 -> 148 floats with the AO response below.
 - SSAO: a depth-precision bias, interleaved gradient noise and `StandardResource::setAOResponse()` (JSON `AOResponse`): the open sea is no longer darkened.
 - Post-process: one view reconstruction for the whole chain (`ViewReconstructionGLSL`, `FrameContext::view`) replaces the scalar depth linearisation, whose clip planes were frozen at 0.1 / 1000.
 - Geometric specular antialiasing for the direct lights, the IBL and the clear coat (`LightGenerator::specularAntialiasedRoughness()`).
 - Decal layers lit by the light passes; a switched-off glow keeps its indirect light. Depth of field puts a sky pixel at infinity; a disabled main light casts no light shaft.
 - glTF: `LoaderOptions::nonEmissiveMaterialNames` drops a material's emission.
 - **TileWorld**, a new optional module (`src/TileWorld`): square tiles with a full autotile, a slope-bounded corner relief, a plinth, arc zones, `TileContext::neighbours`, drawn-only surface slots (`SurfaceSlot::collidable`), `scatter()` over the zones of a kind, `Relief::surfaceHeight()`, public `cellSeed()`.
 - `TerrainResource::loadFractalNoise()`: a seeded fBm terrain sunk toward its border.
 - **Audio.** `SoundEmitter::playOneShot()` (up to 4 voices over the emitter's own sound), `SoundEmitter::setPitch()`, `Scene::playSoundAt()` (world sounds owned by the scene, up to 16), `Audio::Source::isFinished()`, console `getWorldSoundCount()`. The source pool resets a returned source to OpenAL's defaults (it kept its last user's state).
 - Physics: Ackermann steering (`VehicleSettings::ackermann`, 0 by default, which changes nothing); an impact is measured per entity pair along the resultant impulse.
 - Vulkan: a drawn instance's or geometry's buffers are retired through the deferred destructor, never destroyed in place (`DeferredDestructor::close()`); a grown instance-transforms SSBO gets a new descriptor set.
 - Particles: an emitter is culled on its particles' world box, not on its node.
 - Input: `mousePress` / `mouseRelease` also hold the polled button state (`PointerController::injectButtonState()`); `Input::Manager::injectMouseWheelEvent()` and console `mouseWheel()`. A console argument with an exponent is a float.
 - Resources: no false "loaded, but still have N dependencies" error.

## Version 1.1.1 (2026-10-26)
 - **`FileSystem::resourcesDirectory()`: one answer to "where are the shipped resources"** — static and path-only: beside the binary, or `Contents/Resources` inside a macOS bundle, the OUTER bundle's for a binary of a bundle embedded in `Contents/Frameworks` (a CEF helper used to resolve its own empty `Resources`). The `data/` search, the Vulkan layer lookup and the MoltenVK driver pinning use it; `pinVulkanLoaderToBundledDriver()` now takes the resources directory (API change).

## Version 1.1.0 (2026-10-23)
 - **Material texture packing.** A material binds one descriptor per distinct texture; every scalar component honours `SourceChannel`; one shared `TexturePacker` (`TexturePacker::plan()`) for the runtime and the bake tool. Console `Core.RendererService.getTextureMemory(top)` reports every texture's VRAM.
 - **Block format per texture kind.** BC5 for normal maps, BC4 for opaque grey linear textures, BC7 otherwise (`TextureCompressor::selectFormat()`); the normal map is part of a texture's identity (`Usage::normalMap`, `enableNormalMap()`, the `#normal` store variant, the loaders' `-normal` slot); Z is reconstructed in every normal decode. `TextureCompressor::compressedSize()` removed (API change). Needs external dependencies v019.
 - **One mip chain for both texture paths** (`Graphics/MipChain::build()`): Kaiser filter, sRGB filtered in linear light, colour dilated under transparent texels, coverage of a binary mask held, normal maps renormalized. The uncompressed path uploads the CPU chain instead of a GPU box blit. `Vulkan::Image::createFromCompressed()` becomes `createFromMipChain()` (API change). New setting `Core/Graphics/Texture/BlockCompression` (default true).
 - Optional GPU footprint per texture kind: settings `Core/Graphics/Texture/AlbedoSkippedMips`, `NormalSkippedMips`, `DataSkippedMips` (default 0); `Usage::fullResolution` / `enableFullResolution()` (the `#full` variant) exempts sprites, the image viewer and light projections.
 - Animated textures in BC7 (the ocean normal map 640 -> 160 MiB); a grey-scale image keeps one channel.
 - **Baked GPU textures.** `Graphics/KTX2Writer` (BC7 / BC5 / BC4 into zstd-supercompressed KTX2); the `EmeraudeTextureBake` tool (`tools/texture-bake`, material mode `--material` / `--materials`); `tools/gltf-bake-textures.py`, with in-place glTF bake and recorded alpha coverage.
 - **The `GPUTextures` store.** `CompressedImageResource` reads it, with names checked against the `<TextureName>.<Purpose>` convention (base `TextureName`); `ImageResource` refuses a GPU container (`.ktx2`, `.basis`, `.dds`) and points to `GPUTextures`; a JSON material reads its texture set (`TextureSet` key, `StandardResource::planGPUTextures()`).
 - **Stop requests.** `Core::cancelStopRequest()`: the user's Cancel wins over a system stop (`Window::cancelQuitRequest()` answers a suspended macOS "quit" Apple Event "user canceled"); `Core::isSystemStopRequested()`; the force-quit dialog opens only after `MaxStopVetoCount` vetoes and 10 s (`ForceQuitMinimumDelay`) since the first one.
 - macOS: a Dock or AppleScript "quit" is a normal stop (vetoes, force-quit dialog, no deadline); only the end of the session is a system stop; every Dock "Quit" counts as a new attempt.
 - **Exit code 96 for a lost display connection.** `Window::isDisplayConnectionLost()` (Wayland) notifies `OSLostDisplayConnection`, and `Core::stopOnDisplayLoss()` stops without a dialog with `DisplayLostExitCode` (96). `StartupRefusal` exposes the parts of its message for translation; `Core::userExitCode()` reads the exit code a stop will end with.
 - Tracer levels: a failure the engine recovers from (device creation fallback, swap-chain recreation, physics manager) is an Error, not a Fatal.
 - Windows: `Window::getBackOnDesktop()` restores a minimised window.
 - USDLoader: small fix.

## Version 1.0.1 (2026-10-19)
 - **Startup refusals told to the user, exit code 97.** `Core::diagnoseStartupRefusal()` classifies NoVulkanDriver / OutdatedDriver / IncompatibleHardware; the `Core::onStartupRefused()` hook lets the application show its own message, else a native dialog names the GPU, its driver and the vendor's driver page; the process exits with `Core::IncompatibleSystemExitCode` (97). `SystemInfo::fetchGPUInformation()` lists the GPUs through the operating system, independently of Vulkan; `GPUDevice` gains the driver name and version and a software flag.
 - **Vulkan startup.** `Vulkan::enumerate()` treats `VK_INCOMPLETE` as a success; a device below `Instance::MinimumDeviceAPIVersion` (1.3; 1.2 on macOS) is refused with a log line; the required features are checked before `vkCreateDevice()`; when the selected device cannot be created, every other compatible device is tried; `Instance::deviceRefusals()`; one unconditional line names the graphics device.
 - **Target Vulkan API version.** `Core::targetVulkanAPIVersion()` (virtual, default 1.3, lowest 1.1 with `VK_KHR_timeline_semaphore`) and the switch `--vulkan-api-target=X.Y`; below 1.2 the engine draws the overlay only and `newScene()` refuses.
 - Swap-chain: a failed recreation is retried with a limit, then the engine stops; a resize during a recreation is no longer lost; a 0x0 surface (minimised window) is survived.
 - Core pacing: without an active scene the Continuous loop is paced; no sleep after a late frame; the pacing stands down while the present blocks (FIFO). The logic runs at 60 Hz on an absolute deadline; logic clocks count cycles exactly.
 - **Ownership and lifetime (Ave Robustus II).** Every thread-pool job is owned by a task handle or group, and `Core` no longer cancels nor drains the pool; every dedicated thread stops through its stop token; input listeners and shared uniform buffer seats are tokens; no global console instance; OpenAL, GLFW, COM and BSTR handles in RAII holders; Vulkan and OpenAL handle owners are neither copyable nor movable; an entity's component mutex is no longer recursive.
 - Thread-safety annotations on the mutex-protected members of Vulkan, Resources, Scenes, Graphics, Overlay, Console and Net; the races they revealed are fixed (render / logic, the recorder's shutdown, a console use-after-free at exit).
 - Startup fault injection (CMake `EMERAUDE_FAULT_INJECTION`, switch `--inject-startup-failure=NAME`): every Core startup step proven to exit cleanly.
 - Sanitizers: the engine's own block is removed; emeraude-base's switches (`EMERAUDE_ENABLE_ASAN`, `LSAN`, `TSAN`, `UBSAN`, `MSAN`) instrument the whole cascade, on Linux, macOS and MSVC.
 - Loaders: the WAD, FBX, USD and glTF loaders are split into an analysis and a materialization, each with a fuzz target; the engine's readers go through `Base::IO`, and the loaders check a path before their library opens it.
 - Static analysis: cppcheck 2.22.0 and include-what-you-use configured and swept; a template-instantiation guard (it found `Texture3D` dereferencing an unregistered container); clang-tidy with four more checks and `WarningsAsErrors '*'`; MSVC /analyze findings fixed.
 - Windows paths: every path conversion in UTF-8; the hard aborts on a path outside the ANSI code page are gone; a startup warning when the process code page is not UTF-8; `Console::replaceInvalidUTF8()` keeps console and MCP replies valid.
 - Settings: a settings file named on the command line must exist (else the engine stops); the autosave switch holds on every path; the settings version guard writes the fresh file at once.
 - Overlay: geometry setters thread-safe; notifications after the lock; surface buffers retired through the deferred destructor; a size change undone before its commit re-syncs the content provider.
 - Resources: a load cancelled by the stop fails silently, with one summary line per container. Staging buffers are released after the upload (large ones at once, idle ones after 5 s); the pipeline cache is restored on a worker.
 - Textures: the colour space and the normal map flip are part of a texture's identity.
 - Audio: every OpenAL object released while its context and device exist. `MusicResource`: no tag extraction from a script music, a missing tag is no error.
 - Video: the hardware H.265 encode is withheld on AMD (software VP9 is used); H.265 encoder fixes (SPS, encode status, encode-only queue family).
 - macOS: RTTI across images (default visibility for the export macros); the bundled MoltenVK manifest is found from the resolved binary directory; `VK_KHR_portability_subset` enabled on every device, compute included.
 - Paths: a point-count change marks the old shape's screen region reactive (TAA ghost). The TBN display is refused on a device without a geometry stage.
 - `SystemInfo`: the GPU available memory is reported in MiB; the GPU inventory runs in the main process only.

## Version 1.0.0 (2026-10-11)
 - **Physics overhaul.** One contact pipeline with a soft-step solver; oriented boxes, centre of mass, derived inertia, rotation by default; a kinematic character controller; continuous collision of fast bodies, dynamic against dynamic, with exact box sweeps; simulation islands and static triangle meshes; a one-sided, solid-below ground; impacts emitted outside the physics lock.
 - **Wheeled vehicles.** A dynamic chassis on sphere-cast wheels (`VehicleSettings`); a braked wheel locks inside the solve; a wheel ignores a contact steeper than `maxSlopeAngle`.
 - **Deferred punctual lights.** One resolve instead of a forward pass per light, with tiled light culling (16 x 16 tiles) and `MaxLights` 128 -> 1024; console `setDeferredLightTileCulling(0|1)`. The frame's lights are selected by visibility and distance; a light can drive its panel; a switched-off light stops rendering its shadow map.
 - **CPU copies released after the GPU upload.** `ResourceTrait` leases (`LocalDataLease`, `retainLocalData()`, `markLocalDataReleasable()`, `releaseLocalDataIfIdle()`); a released copy comes back from its store source or from its GPU copy. Settings `Core/Resources/ReleaseLocalData` (on by default) and `Core/Resources/LocalDataReleaseDelay` (5 s); console `releaseLocalData()` and `memoryCensus`.
 - **Resource sharing** (off by default). `Resources::SharingServer` serves an engine's data stores over HTTP; `Resources::PeerStore` fetches what a peer lacks, each file SHA-256 verified; `Net::Manager::registerPeer()`; console / MCP `sharingStatus`, `fetchFromPeer`, `peerFetchStatus`, `cancelPeerFetch`. The MCP server runs on base `Network::HTTPServer`.
 - Loaders: identical geometry shared by content (`GeometryDeduplicator`); USD parsed-layer cache, level-of-detail chains, arcs composited to a fixpoint, `.usdz` read into memory when it cannot be mapped, an inventory mode; tinyusdz moves to its `LightUSD` name; `LoaderOptions::cutoutMaterialNames`; glTF `TEXCOORD_1` per map, the meshopt colour filter, skins keep their non-joint ancestors.
 - GPU memory: a budget warning at 98 % with the largest allocation groups; `Vulkan::Device::memoryBudgets()`; console `Core.RendererService.getGPUMemory(top)` and `writeGPUMemoryReport()`.
 - **System stop and device loss.** `Core::requestSystemStop()`: the application's veto lasts `SystemStopGracePeriod` (3 s), then the stop proceeds with exit code 0; `PlatformSpecific::TerminationSignalCatcher` for SIGTERM / SIGHUP / SIGINT; Windows session end (`Core::onCoreSessionEnding()`). Device loss fails fast: `Vulkan::Device::isLost()`, `Core::stopOnDeviceLoss()`, `DeviceLostExitCode` (98), never `std::abort()`.
 - **The engine's own Vulkan loader.** It opens the loader of the external dependencies by explicit path through volk (`Vulkan::Loader`) and links no Vulkan library, while CEF keeps its own. The Vulkan headers, link library and validation layer come from the external dependencies only; the layer ships next to the binary and is put first in `VK_ADD_LAYER_PATH`. A validation layer that cannot be loaded never stops the launch.
 - Device selection: `DeviceAutoSelectMode::RawPerformance`; equal scores no longer drop a device; `Instance::preferGraphicsDevice(DeviceIdentity)`; `Vulkan::probePhysicalDevices()` lists the GPUs before the engine exists.
 - Overlay: `Surface::uploadAcceleratedFrameFromCPU()`, `Surface::fitPopupOrigin()`; partial image updates run on the graphics queue family.
 - Core: a failed start is recorded (`Core::startupFailed()`), never a process exit from the constructor; `Core::run()` returns a failure when `onCoreStarted()` fails; `--help` prints again.
 - Display sleep: setting `Core/Video/Window/PreventDisplaySleep` (default false, opt-in); macOS wakes a sleeping display to start and disables window state restoration.
 - Tracer: the previous journal is kept (`Tracer::previousLogFilepath()`, `*.previous.log`); `Tracer::setLoggerEnabledByDefault()`; `Tracer::setLoggerForcingSettingKey(key, default)`; the log file is closed before exit.
 - **The Observer payload is a `Base::Any` (API change).** `onNotification()` and `notify()` take `const Base::Any &` instead of `const std::any &`.
 - `Vulkan::Device::lockAccess()` returns a guard; the `lock()` / `unlock()` pair is gone (API change).
 - `UserInfo::preferredLanguage()`: the user's interface language as a BCP 47 tag.
 - Net: every URL is traced through `URI::redacted()`; `TCPClient::connect()` uses Happy Eyeballs (RFC 8305); a console client's last line ends with a FIN.
 - Every engine thread starts through `Base::Thread`; caches commit through `IO::renameFile()` (long Windows paths).
 - The paranoid warning set at zero warning on GCC, Clang, AppleClang and MSVC; every lifetime ends in its owner, failure paths included.
 - Automatic LOD jobs never outlive their mesh and are interrupted at shutdown.
 - Vulkan: a buffer or image released during its upload is destroyed once the upload completes; a non-indexed triangle list gets an identity RT index list (device loss); the bindless layout is checked with `vkGetDescriptorSetLayoutSupport`; no power-of-two texture requirement.
 - `Dialog::Message`: Windows boxes come to the foreground; an optional default answer.
 - Materials: JSON `AlphaThreshold` in the Albedo block; the emissive texture takes its factor; the ambient diffuse takes (1 - metalness).

## Beta versions 0.9.52 → 0.9.82 (2026-08-05 – 2026-10-01)
 - **World coordinates are Y-up (API change).** The whole engine moves from the Y-down to the Y-up convention.
 - **One lit material (API change).** The legacy Blinn-Phong `StandardResource` and `BasicResource` are deleted; the Cook-Torrance `PBRResource` is renamed `StandardResource` (ClassId `MaterialStandardResource`). It gains an unlit flag, vertex colours, a per-component UV transform, source channels, an opacity contract and a uniform JSON format. `BlendingMode::Screen` is removed.
 - **glTF.** Compressed glTF (KTX2, meshopt, quantization), `KHR_draco_mesh_compression`, `EXT_texture_webp`, `KHR_texture_transform`, `KHR_materials_specular`, `ior`, `volume`, `clearcoat`, `sheen`, `iridescence`, `anisotropy`, `COLOR_0`, authored tangents, sampler wrap modes; a refused file names the extensions the loader lacks. A glTF conformance bench in `tools/`.
 - **Scene loaders.** An OpenUSD loader (USD `PointInstancer`s become native instance sets), and a non-blocking way to use any loader. `AssetLoaders` became `SceneLoaders`, then moved to `Scenes/Loaders/` (API change); a scene loader registry; mirroring at import (`swapX` / `swapY` / `swapZ`).
 - **Two lighting lanes.** The screen-space and ray-traced lanes are both resident and selectable at runtime (`PostProcessStack::installLightingFamily()`, `setLightingMode()`), under one settings tree; post-process effects split by domain; `PostProcessStack::hasAnyEffect()`.
 - Post-process pipeline restructured: overlay protocol, a shared combine pass, display effects folded into the final shader, bloom folded into tone mapping, batched grab-pass barriers, a shared denoise pass.
 - Global illumination: the shared `GIDenoiser` (SVGF) for RTGI, SSGI and RTR, with albedo demodulation and variance clipping; a GTAO horizon search in SSGI; DDGI probes feed RTR and RTGI.
 - Reflections: Hi-Z SSR at full resolution, glossy cone tracing, RTR normal mapping and ray-cone texture LOD, skinned geometry in the TLAS through a per-frame BLAS refit, GGX-convolved probes, HDR probes, on-demand render targets with per-target exclusion lists.
 - Shadows: cascades fitted by bounding spheres and snapped to the texel grid, a per-cascade bias in metres, cascade cross-fade, each cascade in its own pass culled by receivers, coarser LODs for shadows, alpha-tested shadows.
 - Volumetrics and sky: volumetric clouds placed as scene entities, with their shadows; world-space single scattering through a scene-level participating medium; atmospheric fog in nits; lens flare rewritten; an animated sun (`SunCourse`, `SkyFollowsSun`).
 - **Terrain and water.** CDLOD terrain over a height clipmap; optional task and mesh shaders for displaced grids and the near terrain; parallax occlusion mapping fixed; an FFT ocean on a camera-following CDLOD grid, with whitecaps, shore foam and velocities for TAA.
 - Vegetation: generated trees as renderables with their LOD chain, continuous wind sway (shadows included), octahedral imposters for distant trees, hashed alpha test.
 - New drawables: line lights (LTC), beams (lasers, electric arcs) along curves, paths drawn by vertex pulling, a selection outline and a multi-entity editor panel.
 - **Frame synchronization.** A triple-buffered logic-to-render state (the logic thread no longer laps the renderer); render lists culled from the published state through the rendering octree; the read state latched per frame and uploads behind the fence; the active scene lock gives writers preference.
 - **Load times.** The engine owns its Vulkan pipeline cache (5.7 s of load stall becomes 31 ms); the SPIR-V cache is on by default and validated against its toolchain; a built-in per-pass GPU profiler (`Vulkan::GPUProfiler`, setting `Core/Graphics/GPUProfiler/Enabled`); descriptor pools grow by pages.
 - **Typed console and MCP server (API change).** Every console command has a typed signature and is served as an MCP tool (default port 17778); the untyped command form is deleted; every request is answered with one JSON line; every component type is driven from the console, addressed by the shortest unique node-path suffix.
 - The remote console listener is off by default and binds to loopback (setting `Core/Console/EnableRemoteListener`); Shift+F10 opens or closes it live.
 - Window-less runs render the real frame into a headless swap-chain that can be captured.
 - **Net.** `Net::APIClient` calls a web API without CEF; the download manager downloads over HTTPS with a URL-keyed cache, retries and progress; UDP multicast and interface listing (IPv4 and IPv6); `TCPServer`, `UDPClient` and `SerialPort` fixes.
 - RushMaker overhaul: BT.709, a single studio constant-frame-rate mode, hardware H.265 through Vulkan Video, setting `Core/RushMaker/ForceCPUEncoding`.
 - Robustness: C++ exceptions removed from the engine; the triad pass (robustness, allocations, performance) over every subsystem, with trust boundaries checked and non-throwing `std::filesystem` calls. A settings reset keeps the entries declared in `SettingsKeyRestoration` (an optional Core constructor argument).
 - Build: `EMEN_LEAN_API` with `EMERAUDE_USE_FULL_EXPORTS` / `EMERAUDE_USE_LEAN_EXPORTS`; `EMERAUDE_USE_EXPLICIT_EXPORTS` reverted to Off, then back to On under MSVC; RenderDoc optional and fetched on demand; GLFW 3.5.1 fork pinned; third-party archives kept out of the export table; headers trimmed by forward declarations.
 - **Revert `EMERAUDE_USE_EXPLICIT_EXPORTS` to Off by default** — `On` makes the consuming application's link much longer, on every link (explicit `dllimport`/`dllexport` gives the consumer's linker a far larger import-resolution surface than the compact export-all `.def`), on top of the standing duty to annotate every new consumer-referenced public symbol. Also move the MSVC "export-all excludes the PCH" guard out of emeraude-base's shared helper into the engine's own PCH call site. The engine target is the only one in the cascade using `WINDOWS_EXPORT_ALL_SYMBOLS`, so it is now the only one that loses its precompiled header — every other target keeps it. The explicit-export migration itself stays in the tree and can be turned back on with one line. (0.9.52)
 - **`EMERAUDE_USE_EXPLICIT_EXPORTS` defaults back to On, on MSVC only** — the engine's symbol surface crossed the hard PE limit of 65535 exported ordinals per DLL, so `WINDOWS_EXPORT_ALL_SYMBOLS` can no longer link at all (`LNK1189`, exports.def at ~65.8k symbols). The July revert below is therefore void on Windows: explicit `EMEN_API` exports are the only viable mode there. The option stays Off on non-MSVC platforms, where it is inert (no `.def`; ELF/Mach-O export via symbol visibility). The engine target gets its MSVC PCH back as a side effect. (0.9.53)
 - macOS: portability subset features requested, a dedicated memory block for the skinning SSBO on MoltenVK, an explicit barrier between post-process passes, the Vulkan loader pinned to the bundle's own driver.

## Beta version 0.9.51 (2026-07-30)
 - Fix Vulkan validation errors.
 - Normalise the legacy specular in energy, and read `Shininess` as a glossiness.
 - Add a state reading probe to the input manager.
 - Set the Vulkan SDK to 1.4.357.0.
 - macOS: minor fixes.
 - Docs: the macOS/MoltenVK section (bindless samplers require MoltenVK 1.4+), and what not to re-investigate.
 - Rewrite the README as a layered, non-redundant set across the family: emeraude-base owns the toolchain, the compile policy and all external-dependency detail; the engine owns the Vulkan runtime, the SDK, its submodules and options; the applications own their own demonstration.

## Beta version 0.9.5 (2026-07-30)
 - Release of the photometric lighting + physical camera cycle (see 0.9.42).
 - Cross-platform fixes from macOS and MSVC.

## Beta version 0.9.42 (2026-07-22)
 - **Photometric lighting, phase 1.** The unit vocabulary for lights: a directional light takes an illuminance in **lux**, a point/spot light a luminous power in **lumens**. Physical inverse-square attenuation for point and spot lights, light generators take photometric units, and the legacy compensation is dropped.
 - **Photometric lighting, phase 2.** The ambient term becomes a photometric illuminance; middle grey is 0.18, not 0.5; the auto-exposure gets a photometric clamp range; materials carry an emissive strength and unlit materials apply their emission.
 - **A sky is a light source.** A sky declares its luminance, and that luminance lights the scene. Photometric background contract, **sky-driven lighting**, and removal of the pre-photometry `StaticLighting` shortcut — `SimplePass` is now strictly unlit. Thread-safe `applyBackgroundLighting()`, poll-based background lighting, safe entity removal and sky re-derivation; the default ambient illuminance is **measured** on the sky texels.
 - **Image-based lighting (4 lots).** GPU foundations (BRDF LUT, bakeable IBL textures, cubemap mips, Y convention), per-environment bake (GGX prefilter + irradiance, scene trigger, publication), shading (diffuse irradiance in the ambient pass, split-sum specular), and an SSR environment fallback on the bindless prefiltered slot. HDR environment cubemaps (Radiance RGBE, RGBA16F, D6 calibration).
 - **Physical camera system.** Absolute APEX exposure metered on ISO from the camera triad; the focal length **is** the field of view (derived, not stored) and reframes; the sensor format is a constant-lens change; a style declares a format; camera presets keep the sensor (tone mapping is not a style); the photographic authority is a weak reference that heals to null when a camera dies. A physical camera panel on **Shift+F2**.
 - **Lens effects are the camera's.** Bloom is materialized from the camera with a threshold in nits; depth of field uses the aperture diameter, not the f-number; motion blur joins the camera (McGuire reconstruction driven by the shutter speed). Scoped lens-effect locks, and the camera gets its chain whether or not the scene built one.
 - **Motion vectors (4 steps) and TAA.** Per-instance transforms SSBO replaces the scene-pass matrix push constants, real previous model matrices, an RG16F velocity MRT attachment, and RTGI temporal reprojection by motion vectors; double skinning gives limb-level velocity for skinned meshes; the sky needs the previous INFINITY view-projection. Temporal anti-aliasing with per-draw sub-pixel jitter and filtered source reconstruction.
 - Ray tracing: RTGI temporal accumulation & multi-bounce, RTR shadow rays at reflection hits ("reflections match the raster"), RTAO double-intensity fix with softened defaults, SSGI parameters exposed as settings keys.
 - AssetLoaders: **WADLoader** — a classic Doom-engine map materializer.
 - Lighting: the legacy specular becomes what its name already claimed, Blinn-Phong.
 - Window: the title bar follows the OS appearance, on the only platform that allows it.

## Beta version 0.9.41 (2026-07-22)
 - Core: opt-in settings reset when the settings file predates the current build.

## Beta version 0.9.40 (2026-07-16)
 - macOS: implement `SystemInfo::getFreeMemory()` via `host_statistics64`.
 - Linux: use `MemAvailable` for the free-memory probe.
 - Fix the per-axis scale in `Window::getFramebufferSize()`.
 - Overlay: scale-change notification + async-provider properties latch.
 - Update the clang-tidy rules.

## Beta version 0.9.39 (2026-07-13)
 - Add `Core::scheduleMainLoopCycle()`, a thread-safe main-loop cycle scheduling contract.
 - Linux: portable `INSTALL_RPATH` (`$ORIGIN`) on the shared library.
 - Update the GLFW source.
 - MSVC fixes; remove an excessive log.

## Beta version 0.9.38 (2026-07-08)
 - **Complete the explicit-export migration and enable the STL precompiled header on MSVC.** The public surface consumed by an application carries `EMERAUDE_API`; `EMERAUDE_USE_EXPLICIT_EXPORTS` becomes the default, which resolves the old `WINDOWS_EXPORT_ALL_SYMBOLS` + PCH incompatibility.
 - Add the missing STL includes so the engine still builds with the PCH disabled.
 - CI: add a PCH-OFF Linux lane to catch missing includes, trimmed to the compile-only dependency set, and clone emeraude-base from a configurable branch.
 - Docs: note the macOS Objective-C++ PCH auto-skip.

## Beta version 0.9.37 (2026-06-30)
 - Add an **on-demand rendering mode** to Core (and fix it not redrawing on window resize).
 - Vulkan: a central `DeferredDestructor` for the runtime destruction of GPU-visible objects.
 - Fix BLAS builds racing buffer uploads across transfer queues.
 - Fix the per-light binding: include the dynamic offset in the redundancy check.
 - Two-sided lighting: orient the shading normal with `dot(N,V)`, not `gl_FrontFacing`.
 - Light culling: test the light against the instance bounding sphere.
 - Settings: per-effect ray-tracing sub-groups exposing all RTGI/RTAO parameters, plus a PixelDoubling option for RTR and RTAO; RTGI default sample count 16 → 8 (measured).
 - Enable the overlay surface to use an external API.
 - Windows: redefine the file dialog box ownership (TECH-1927), add DNS hostname retrieval, and fix desktop commands failing on paths containing spaces.
 - Deliver the button release to press-observer surfaces under pointer capture; update the Linux dialog box and the settings key descriptions.

## Beta version 0.9.36 (2026-06-23)
 - Enable the configuration of the main loop update frequency.
 - Change the `Core/Video/VulkanDevice/EnableFailSafe` setting default to `false`.
 - Fix the pointer coordinate space on Wayland and on HiDPI monitor switches.
 - Windows: fix the file dialog box centering (TECH-1838).
 - Hotfix: mouse move tapping between surfaces.

## Beta version 0.9.35 (2026-06-18)
 - **Multi-scene robustness.** Fix the crash on unloading a scene and three render-resource lifetime bugs exposed by scene unload/switch; make the acceleration structure builder a single Renderer-owned instance; make scene switching hitch-free; fix the Overlay shared-sampler destruction; fix the device loss when deleting a scene that uses post-processing; stop the device-lost spam from the acceleration structure builder.
 - **Vulkan debug object naming** (`vkSetDebugUtilsObjectNameEXT`) on every remaining Vulkan object, for validation and GPU captures; restore leak detection for `DescriptorSetLayout`.
 - Include the pipeline layout in the graphics pipeline cache key.
 - MDI: fix the shader generation plus draw-time descriptor and push-constant bugs.
 - Fix `IntermediateRenderTarget` by-region dependency causing stale-frame block corruption.
 - Honor the double-sided material flag in the glTF and FBX loaders, add `LoaderOptions::forceDoubleSided`, and flip the shading normal on back-facing fragments.
 - Wire the `GISampleCount` setting to the RTGI trace (it was a dead key); replace the RTGI `sin()` noise hash with a PCG integer hash.
 - Diamond-square ground/terrain: snap a non-power-of-two division instead of failing.
 - Console: add the `triggerRenderDocCapture` command; fix Settings serialization emitting invalid JSON for empty stores.

## Beta version 0.9.34 (2026-06-17)
 - Update the identification usage.

## Beta version 0.9.33 (2026-06-11)
 - Fix the ImGui integration.
 - Add a manual request to resize a Surface from an external source (such as CEF), a safe-guard for invalid overlay surfaces, and a new overlay surface transfer strategy.
 - Windows: fix the dialog box latency (TECH-1838).
 - Fixes for the PCH option; remove useless default parameters on Surface; overlay and CMake cleanup.

## Beta version 0.9.32 (2026-06-02)
 - **Enable precompiled headers** (the shared STL hot-set from emeraude-base).
 - Uniformize `AARectangle` / `AACuboid`.
 - CMakeLists cleanup; bug fixes.

## Beta version 0.9.31 (2026-05-27)
 - **Split the engine's foundation layer into the standalone [emeraude-base](https://github.com/EmeraudeEngine/emeraude-base) library** — `src/Libs/` (`EmEn::Libs`) becomes `EmEn::Base`, with its own repository, versioning and roadmap. The engine clones it (`cmake/InstallEmeraudeBase.cmake`) and consumes a pinned package; emeraude-base becomes the single source of truth for external dependencies and the compile policy. (Briefly numbered 0.9.4 before the version settled back on the 0.9.3x line.)
 - "Ave robustus A.0": re-export `Severity` from emeraude-base in `CoreTypes`, and route the emeraude-base diagnostics through the Tracer.
 - Improve mouse event tracking on a single UI surface.
 - Remove `LOCAL_LIB_DIR`; CMakeLists cleanup.

## Beta version 0.9.3 (2026-05-19)
 - **Improve compilation time**: includes cleanup, and drop the usage of `__PRETTY_FUNCTION__`.
 - **Large clang-tidy campaign**: tier 1 cleanup + math-parens auto-fix, redundant member-initializers removed, positional → designated initializers, tightened underlying types on bounded-value enums (bitmask `FlagBits` stay `uint32_t` via NOLINT), `OBJVertex` encapsulated, 24 const getters marked `[[nodiscard]]`, and west-const local variables.
 - Improve the user application close policy.
 - Fix the MSVC build: include `<Windows.h>` before other Windows headers.

## Beta version 0.9.26 (2026-05-18)
 - Ultimate fix for `TCPClient`; minor Linux fix.

## Beta version 0.9.25 (2026-05-13)
 - **Automatic download of the external dependencies from GitHub** at configure time, with the build type taken into account when selecting them.
 - Unify the G-buffer.
 - Ray-traced reflections: fix multi-layer objects, sprite display and alpha.
 - Docs: caution-points entries for multi-geometry BLAS and the sprite RT pipeline.

## Beta version 0.9.24 (2026-05-12)
 - Clean up the external libraries to be compiled.

## Beta version 0.9.23 (2026-04-23)
 - Update the FBX loader with animations, fix its UVs, and add a renderable customization option.
 - Network: add `TCPClient` and `TCPServer` — ASIO is now mandatory.
 - OverlayManager: `UIScreen` can now sort surfaces naturally.
 - Update the keyboard/pointer input debug; fix the config file in release mode; update the hash tools and the clang-tidy rules.

## Beta version 0.9.22 (2026-04-21)
 - Support RMID-wrapped MIDI files (Microsoft RIFF MIDI container); WaveFactory MIDI reader cleanup and documented quirks.
 - Console: mandatory help, recursive help dump, MDI telemetry.
 - Fix the Observer/Observable traits for a data race, and a crash in the overlay screen destructor.

## Beta version 0.9.21 (2026-04-17)
 - Add `PlaylistResource` and enhance the TrackMixer remote control; expose playlist management for a runtime UI (`currentPlaylist` + `PlaylistSwapped`).
 - Fix lossy audio loading: VBR frame-count mismatch and int16 peak wraparound.
 - Add a Win32 API fallback for user dialogs.

## Beta version 0.9.2 (2026-04-17)
 - Minor fixes.

## Beta version 0.9.11 (2026-04-08)
 - **Scene Editor system**: CPU picking, standalone gizmo rendering and remote input injection. Interactive **translate** gizmo (drag, hover highlight, local/world space), **rotate** gizmo (3 RGB torus rings, interactive drag), and **scale** gizmo (per-axis and uniform, `setScalingFactor` on `LocatableInterface`); all gizmos pre-created at editor activation, with `Shift+T` to switch back to translation.
 - Geometry: add loading from raw buffers, with interleaved or separate attributes.
 - Promote `SSDPClient` to a general `UDPClient`.
 - Fix crashes caused by incorrect input manager usage.

## Beta version 0.9.1 (2026-03-30)
 - Network: add `SSDPClient`, serial port listening and Wi-Fi scanning (Windows switched from `netsh` to the native API).
 - System: improve the CPU info, add storage info (`DiskInfo` renamed `StorageInfo`), and convert `UserInfo`/`SystemInfo` into services.
 - Window: add monitor info. Core: add a way to specify the user exit code.
 - Fix the TLAS instance transform to include the renderable instance scale.
 - Add a setting to silence HWLOC; add colors in the Windows terminal.

## Beta version 0.9.0 (2026-03-30)
 - Milestone closing the 0.8.6x cycle: cross-platform compilation fixes.

## Beta version 0.8.64 (2026-03-25)
 - **Skeletal animation with GPU skinning**: animation data structures and loader integration, runtime and resource management, bone vertex attributes enabled in the VBO, VBO format bone handling, a `PerModel` descriptor set for the bone matrix SSBO, and generated vertex-shader skinning code.
 - GLTFLoader: new node-mode API, normal generation, hierarchy flattening and crash fixes.
 - MD5 loader rewrite: shared vertices, correct TBN, descriptor pool fix.
 - LOD settings, ThreadPool integration, read-only skinning SSBO fix.
 - Fix the `CartesianFrame` constructor ambiguity with `Quaternion`.

## Beta version 0.8.63 (2026-03-17)
 - **Enable the G-buffer** in the renderer.
 - **Unified console command system** with clean TCP responses: scene creation with camera, node manipulation, window control, `createScene`/`deleteScene`/`setBackground`/`setGround`/`addMesh`, and JSON scene loading through the TCP `RemoteListener` (`SceneDefinition`, a complete JSON scene description format).
 - Vulkan compute pipeline + GPU X-Ray volumetric scanner; fix the remote listener and add an `XRayAnalyzer` unpack option.
 - Rendering: add LOD capability. VertexFactory: add `ShapeSplitter`.
 - Resources: `ControllableTrait`, terrain types, and `ClassId` container identification in the console.
 - Docs: the AI Runtime Control guide, promoted to a GOLD RULE.

## Beta version 0.8.621 (2026-03-13)
 - **Runtime BC7 texture compression**: the `bc7enc_rdo` library, a `TextureCompressor` for block compression, a compressed-texture upload pipeline for multi-mip data, wiring into the `Texture2D` pipeline, and a disk cache for instant loading on subsequent launches.
 - Add sRGB texture format support for correct PBR color handling.
 - **Indirect rendering (MDI)**: phase 1A, then phase 1B activating the dispatch with a full batch fallback.
 - Ray tracing: eliminate the per-frame CPU stall from the synchronous TLAS rebuild, fix the pipeline barrier and TLAS/buffer lifetime for the async build, remove the legacy synchronous `buildTLAS` path, and add a pixel doubling option for RTGI.
 - Renderable: an instance-local program cache eliminates per-draw hash lookups.
 - Fix a race condition in the lazy `PostProcessStack` scene target creation, and a `GrabPass` use-after-free on lazy reconfiguration.
 - Bloom: add a Karis anti-firefly filter and NaN protection.
 - glTF: fix incorrect UV loading, fix scaling when loading as a `StaticEntity`, and defer `Texture2D` creation to material loading. ShadowMapping: fix loading with a mask.

## Beta version 0.8.62 (2026-03-10)
 - **Ray tracing enabled.**
 - GLTFLoader completed.
 - Minor fix for macOS when ray tracing is unavailable.

## Beta version 0.8.611 (2026-03-05)
 - Versioning scheme restructured (0.8.61 → 0.8.6xx) for the new development cycle.
 - macOS: fix the dock icon disappearing when showing message dialogs, fix the duplicate file extension in the save dialog, and only strip an extension when it matches a declared filter.
 - Manage the `not_allowed` cursor; fix a bug on enabling/disabling the post-processor.
 - IO: file/stream decoupling for media formats.

## Beta version 0.8.61 (2026-03-04)
 - New development cycle structuration.
 - Replace some `std::any` usage with `std::variant`.
 - Add a dialog box for data input.
 - Fix `Libs::IO::zipWriter` for MSVC and for multiple files.

## Beta version 0.8.6 "Push It To The Limit 🤟" (2026-02-20)
 - **New framebuffer post-processing family**, replacing the old `Scenes/Effect` lens effects: `ToneMapping`, `Bloom`, `SSR`, `SSAO`, `DepthOfField`, `FXAA` (+ `FXAASharpen`) and `AtmosphericFog`, with a reworked `PostProcessor` and a new `SceneRenderTarget`.
 - **RenderDoc integration**: `SetupRenderDoc.cmake`, `BuildRenderDocPython.cmake` and the RenderDoc submodule, for in-application GPU frame capture and programmatic `.rdc` analysis.
 - Massive `ShapeGenerator` expansion, and a reworked geometry `ResourceGenerator`.
 - Add `CubemapMovieResource` and `DummyColorProjectionTexture`.
 - Add `AI-COLLABORATION.md`.

## Beta version 0.8.52 (2026-02-12)
 - Recorder and `GrabPass` hardening; Vulkan `RenderPass` work.
 - PBR material additions; audio `Recorder` and `ExternalInput` improvements; IO fixes.
 - Add the `update-glfw.sh` helper.

## Beta version 0.8.51 (2026-02-05)
 - **Video capture devices** on Linux, macOS and Windows (`VideoCaptureDevice`), with libVPX and `SetupVideoDeviceCapture.cmake`.
 - **Screen capture / video recording**: a `Graphics::Recorder` and a `GrabPass`.
 - Audio: add a `Listener` and an `ExternalInput`; `AudioRecorder` becomes `Recorder`.
 - Extend the PBR material resource and its light generator.
 - New documentation: `caution-points.md`, `development-patterns.md`, `troubleshooting.md`, `trueglass-screen-capture-implementation.md`.

## Beta version 0.8.5 (2026-01-20)
 - **Bindless texture manager.**
 - **Cascaded shadow maps**: `ViewMatricesCascadedUBO`, an extended `ShadowMap` render target, a shadow-map light generator and a dummy shadow texture — documented in `docs/shadow-mapping.md`.
 - VertexFactory: split the mesh file formats into their own headers (MDx, STL) and rewrite the OBJ reader.

## Beta version 0.8.46b (2026-01-19)
 - Consolidate the platform-specific helpers: the Linux dialogs share a single implementation (~900 lines removed).
 - macOS dialog fixes; update the `PlatformSpecific` documentation.

## Beta version 0.8.46 (2026-01-15)
 - **PBR material**: a full `Material::PBRResource` and its Saphir PBR light generator.
 - Custom message dialogs on Linux, macOS and Windows.
 - Document the pipeline caching system.

## Beta version 0.8.45 (2026-01-12)
 - **Desktop notifications** on Linux, macOS and Windows, behind a `SystemNotification` service.
 - Rework the native dialogs (message, open file, save file), mainly on Linux.
 - Add the project tooling: `build.py`, `format-code.py` (+ `format-code.json`) and `run_unit_tests.py`.

## Beta version 0.8.44 (2026-01-08)
 - Add swap-chain present mode selection.
 - Add software FPS limiter.
 - Add soundfonts (SF2) support for MIDI playback.
 - Fix bug when unloading wave resources.
 - Fix instruction order in CMakeLists.txt.
 - Print compiler flags and definitions in CMake setup output.
 - Fix overlay Surface buffer selection when transition is enabled.
 - Fix unit test code on Linux, macOS and Windows.
 - Update documentation.

## Beta version 0.8.43 (2026-01-08)
 - Move jsonCpp and fastGLTF to pre-compiled binaries.

## Beta version 0.8.42 (2025-12-21)
 - Upgrade physics engine.
 - Add Capsule primitive for collision detection.
 - Add collision detection model interface.
 - Fix AACuboid validity test.
 - Complete modifiers influence with new collision model.
 - Enable "grounded" feature in node physics.
 - Enable sleep/wake for physics simulation.
 - Add adaptive terrain LOD.
 - Enable sea level in scenes.
 - Fix opacity computation in shaders.
 - Add floating-point optimization flags option.
 - Add sRGB swap-chain option.
 - Remove color-space conversion from OverlayManager.
 - Add pre-multiplied alpha and color format (RGBA/BGRA) options to OverlayManager.
 - macOS: Remove redundant NSApplication initialization from dialogs.
 - Update dependencies.
 - Code cleanup.

## Beta version 0.8.41 (2025-12-17)
 - Enable -Werror/-WX flag on GCC, Clang and MSVC to treat warnings as errors.
 - Add C++ exceptions as a CMake option.
 - Add RTTI and exceptions options.
 - macOS: Fix deprecation warnings in dialog file type filters.

## Beta version 0.8.40 (2025-12-08)
 - Remove legacy collision physics system (WIP).
 - Add retro synthesizer for MIDI files and sound effects.
 - Upgrade IO functions.
 - Various minor fixes.
 - Windows: Add terminal wait to read logs after runtime.

## Beta version 0.8.39 (2025-12-08)
 - Unify "showInformation" settings with command-line argument support.
 - Add color-space conversion option per overlay UIScreen.
 - Minor fixes.

## Beta version 0.8.38 (2025-12-03)
 - Update README.md and AI documentation.
 - Improve device detection for PowerSaving mode.
 - Fix Tracer instantiation across multiple processes.
 - Update observable identification.
 - Fix missing default key for Window class.
 - Simplify shader source code generation logs.
 - Add TokenFormatter class for flexible token display.
 - Upgrade SourceCodeParser to control line number display and comment removal.
 - Fix getClassUID() removing unnecessary static local variable.
 - Fix light multi-pass rendering artifact.
 - Complete Program cache in Renderer.
 - Refactor: move program cache from RenderableInstance to Renderable.
 - Make RenderableInstance lightweight (no dynamic allocations).
 - Add ProgramCacheKey for cache lookups.
 - Remove RenderTargetPrograms classes (no longer needed).
 - Upgrade Octree system.
 - Add height shifting to Grid class.
 - Upgrade ThreadPool class.
 - macOS: Fix BlobTrait to use std::string_view.
 - Windows: Force CMake build type variable.
 - Windows: Add attachToParentConsole() to attach to parent console.

## Beta version 0.8.37b (2025-12-03)
 - Add FailSafe GPU selection mode for Nvidia Optimus workaround.
 - Implement automatic detection of Nvidia Optimus configurations (iGPU + dGPU).
 - Add Failsafe mode excluding discrete Nvidia GPU to avoid WSI swapchain deadlock.
 - Add VendorID namespace with known GPU vendor constants.
 - Add HybridGPUConfig struct for hybrid GPU detection results.
 - Distinguish laptop Optimus from desktop hybrid GPU configurations.
 - Add isLikelyMobileGPU() to detect mobile Nvidia GPUs by name patterns.
 - Change default AutoSelectMode from "Performance" to "Failsafe".

## Beta version 0.8.37 (2025-12-02)
 - Remove unintended mutex unlock() on scoped lock.
 - Use std::source_location everywhere and remove workaround include.
 - Fix Settings isArrayEmpty() method.
 - Add Vulkan device extension when necessary.
 - Add user resizing tracking for Windows.
 - Make swap-chain status change atomic.
 - Add atomic operations on Core flags.
 - Add constexpr option to select swap-chain recreation thread.

## Beta version 0.8.36 (2025-12-01)
 - Add option to disable Notifier creation on startup.
 - Fix Settings root variables on file rewrite.

## Beta version 0.8.35 (2025-12-01)
 - Update AI documentation and create Claude agents.
 - Optimize rendering to eliminate std::vector (zero heap allocation).
 - Update push_constant interface in RenderableInstance concept.
 - Enable suspend/wakeup layer.
 - Fix statistics display in terminal with setting option.
 - Add quick unit testing scripts.
 - Add Texture1D, Texture3D and VolumetricImageResource.
 - Refactor resource management.
 - Fix ASIO exception warning and enable ASIO on macOS.
 - Remove PNG longjmp usage.
 - Revert OpenAL to version 1.1 (system) on macOS.
 - Add proper hash for graphics pipeline reusability.
 - Reduce Core methods requiring user-side implementation.
 - Refactor window resize management to avoid graphics pipeline recreation.
 - Implement smooth overlay resizing.
 - Add direct GPU texture mapping for Overlay Surface.
 - Add core settings key sorting.
 - Update dependencies.

## Beta version 0.8.34 (2025-11-24)
 - Improve cross-platform builds.
 - Add ability for entities to query their scene.
 - Remove static variables from Scene layer.
 - Remove static virtual device counter.
 - Suppress spurious macOS extension usage warnings.
 - Fix std::any_cast bug on macOS.
 - Fix resource name extraction from path on Windows.
 - Fix default ImageResource loading.

## Beta version 0.8.33 (2025-11-24)
 - Add FileTimestamps class.
 - Add dynamic resource scanning.
 - Add improved default cubemap in release mode.
 - Add screenshot capture with Shift+F12.
 - Restructure AGENTS.md following Claude Code best practices.
 - Optimize Renderer cache lookups with unordered_map (O(1) average).
 - Continue render-targets implementation (WIP).
 - Fix shader code generation for uniform blocks using structures.
 - Fix missing attributes in geometry primitive generation.
 - Update StaticVector class.
 - Add Entity builder pattern.

## Beta version 0.8.32 (2025-11-19)
 - Update project information.

## Beta version 0.8.31 (2025-11-19)
 - Enable MSAA for swap-chain.
 - Enable frustum culling.
 - Update README.md.

## Beta version 0.8.3 (2025-11-18)
 - Update texture and render-target interfaces.
 - Add AGENTS.md for AI coding agent compatibility.
 - Add RLE support for TARGA file format.
 - Update unit tests.
 - Add robust physics simulation with rotation (WIP).

## Beta version 0.8.2 (2025-10-31)
 - Remove std::array for flags.
 - Update thread throttle management in Core.
 - Add pinch gesture support on macOS.

## Beta version 0.8.1 (2025-10-29)
 - Add windowless mode.
 - Update logging system.
 - Fix light direction to match camera orientation.
 - Fix external command execution on Windows.
 - Add openTextFile() alongside OpenFile() in desktop commands.

## Beta version 0.8.0 (2025-10-22)
 - Stable on all platforms.
 - Improve offscreen rendering (WIP).
 - Fix OpenAL initialization.
 - Split push constants between rendering and shadow casting.
 - Fix GPU freezing issue on Linux.
 - Implement new framebuffer system using Vulkan swap chain only.

## Beta version 0.7.54 (2025-10-19)
 - Improve command pool and buffer usage in renderer.
 - Improve sampler creation by purpose.
 - Update ShadowMap render target.
 - Simplify namespace usage.
 - Add Vulkan Memory Allocator.
 - Update project information.
 - Update external dependencies.

## Beta version 0.7.53 (2025-10-14)
 - Update CommandPool and CommandBuffer usage, removing throwaway command buffers.
 - Fix premature GPU resource cleanup on exit.
 - Standardize ServiceInterface initialization state handling.

## Beta version 0.7.52 (2025-10-13)
 - Rewrite TransferManager for GPU parallel strategy.
 - Rewrite frame rendering code.
 - Rewrite CPU/GPU synchronization.
 - Improve external library selection between Release and Debug.

## Beta version 0.7.51 (2025-09-30)
 - Improve dynamic scene loading.
 - Fix static lighting.
 - Fix render target resizing.
 - Major Settings service improvement.
 - Improve Arguments service.
 - Remove singleton usage.
 - Improve resource manager.
 - Cross-platform fixes.
 - Major CMakeLists.txt update for MSVC.
 - Replace googletest submodule with FetchContent.
 - Unify compilation options.
 - Add observable identification method.
 - Fix MoltenVK binary integration.
 - Add primary services provider build mode.
 - Add windowless mode.
 - Update CMakeLists.txt and scripts.
 - Enable OpenMP for MSVC.
 - Improve synchronization mechanism.
 - Fix TransferManager staging-buffer count bug.
 - Fix MultipleInstance VBO update deadlock.
 - Update VkBuffer.

## Beta version 0.7.5 (2025-09-06)
 - Major multi-threading improvement with extensive thread-safety.
 - Add Scene State Double Buffering to fix visual artifacts.
 - Add fastgltf and magic_enum libraries.
 - Add Libs::StaticVector class (std::vector/std::array hybrid for small structures).
 - Improve FastJSON utility functions reliability.
 - Vulkan: update queue synchronization.
 - Vulkan: replace std::vector with StaticVector at object creation.
 - Vulkan: update object identifier for debugging.
 - Vulkan: fix offscreen-rendering synchronization.
 - Vulkan: fix swap-chain creation with stencil buffer.
 - Vulkan: fix crash on termination.
 - AVConsole: move render target creation to scene.
 - OpenAL: fix initialization on faulty audio system.
 - Attempt to enable TSAN (Vulkan compatibility issues).

## Beta version 0.7.44 (2025-08-07)
 - Set Vulkan 1.1 API as minimum requirement.

## Beta version 0.7.43 (2025-08-07)
 - Clean up code for Clang and MSVC compilation.
 - Add user service layer.
 - Add option to disable default key behavior.
 - Disable notifier surface rendering.
 - Make Tracer standalone (no longer uses service interface).
 - Refactor Tracer initialization to use std::source_location.
 - Update primary services identification.
 - Improve Window fullscreen management.
 - Replace macros with C++ constants.
 - Add per-surface visibility check for input in OverlayManager.
 - Enable Wayland support.
 - Update constexpr usage.
 - Remove GTK3 dependency.
 - Add cancelable state to dialog boxes.
 - Fix various crash scenarios.
 - Improve shader generator flexibility.
 - Improve renderable instance code.
 - Improve semaphores and fences in render service.
 - Add PCH compiler support.
 - Improve thread sleep code.
 - Add dynamic resource loading memory statistics.
 - Make resource manager thread-safe.
 - Update resource manager verbosity and allow running without stores.
 - Remove Core/Window/InputManager singletons.
 - Remove most Renderer singleton usage (two calls remaining).
 - Improve ThreadPool usage.
 - Inline trivial constructors.
 - Remove ObservableTrait from ServiceInterface.
 - Update dependency versions.

## Beta version 0.7.42 (2025-05-22)
 - Add reproc/reproc++ library for external command execution.
 - Add basic throttler to reduce main thread CPU usage.
 - Add sRGB/linear color conversion to OverlayManager rendering.
 - Fix OverlayManager surface resizing (crashes and desync).
 - Fix OverlayManager service termination crashes.
 - Fix event blocking in OverlayManager.
 - Various MSVC and macOS compilation fixes.

## Beta version 0.7.41 (2025-05-11)
 - Enable overlay surface double-buffering for smooth resizing.
 - Fix CMake scripts to prevent full project re-scan.
 - Improve PixelFactory library.
 - Add cross-platform utilities.
 - Fix major Notifier slowdown from thread creation.

## Beta version 0.7.4 (2025-04-13)
 - Improve Libs classes code.
 - Remove all dynamic_cast/dynamic_pointer_cast usage.
 - Update all submodules to release versions.

## Beta version 0.7.36 (2025-03-31)
 - Fix window framebuffer for macOS Retina display.
 - Fix default font resource.
 - Improve logging and debug switches and settings.
 - Sort compilation condition macros.

## Beta version 0.7.35 (2025-03-27)
 - Fix GTK3 tokens with native dialogs.
 - Windows version fixes.
 - Refactor overlay surface (remove pixel buffer abstraction).
 - Fix concurrency crash during window resize and OverlayManager update.

## Beta version 0.7.31 (2025-03-22)
 - Update OverlayManager with pixel buffer surface and resize events.
 - Remove unused Eigen library submodule.

## Beta version 0.7.3 (2025-03-18)
 - Improve engine for use as multimedia framework.
 - Improve CMake scripts for precompiled binary archives.
 - Rename 'Emeraude' namespace to 'EmEn'.
 - Rename 'Libraries' namespace to 'Libs' and include in 'EmEn'.
 - Rename 'MasterControl' to 'AVConsole'.
 - Code cleanup.

## Beta version 0.7.2 (2025-03-11)
 - Switch to LGPLv3 license.
 - Remove embedded precompiled dependencies.
 - Update CMake external library handling.
 - Add local lib directory check.

## Beta version 0.7.12 (2025-02-21)
 - Prepare for Vulkan 1.4 API.
 - Add CursorAtlas for standard and custom cursors.
 - Update window management (centering, monitor selection, gamma).
 - Update OverlayManager resize handling.

## Beta version 0.7.1 (2025-01-16)
 - Enable C++20 features extensively.
 - Enable unit tests.
 - Cross-platform support (Linux, macOS, Windows).
 - Compile external libraries from git sources.
 - Upgrade physics engine.
 - Upgrade settings management.
 - Upgrade filesystem management using std::filesystem.
 - Upgrade input system.
 - Upgrade entity node tree and static entities.
 - Add instancing-based particle system.
 - Fix Euler coordinates with scaling.
 - Make sprite rendering fully functional.
 - Add faulty shader source display.
 - Prepare for skeletal animations.


# Previous Major Iterations

Here is the history of the engine's major refactorizations until finding the right structure.
In short, how an immature video game idea with OpenGL became a game engine with Vulkan, with separate user-application code for testing the engine.

## Alpha version 0.7 (C++20, ~2023)

This is the version that is mostly equivalent to the OpenGL version.
New features, new techniques, and adoption of C++20.

## Alpha version 0.6 [NON-EXISTENT] (C++17, ~2021)

This is a transitional version started in 2020 to rewrite the graphics engine for using Vulkan instead of OpenGL and the first version to draw the minimum on the screen.
This change was a significant decision and a huge step backward in development with many feature losses.
It was a deliberate choice to drop OpenGL support instead of adding a new rendering backend to take full advantage of Vulkan's benefits with multithreading.
From this version, the idea was to recover everything that had been done with the OpenGL version.

## Alpha version 0.5 [ARCHIVED] (C++14, ~2019)

This is a minor rewrite of the engine itself.
The idea behind this change was to make better use of large project management tools like git, CMake, and external libraries.
This is the latest archived version and the very last version of the engine that uses OpenGL.

## Alpha version 0.4 [ARCHIVED] (C++11, ~2015)

This is the third complete rewrite of the engine in C++. It is mainly a new empty structure from scratch, retaining what was good from the previous version.
By learning advanced techniques in C++ and OOP, it led to a new way of thinking about the engine structure.
This version was the last where the entire code structure would be reworked.

## Alpha version 0.3 [ARCHIVED] (C++11, ~2013)

This is the second attempt to rewrite the engine in C++ from scratch.
Learning C++ is challenging and leads to bad decisions... Alpha 2 was a big failure!

## Alpha version 0.2 [ARCHIVED] (C++03/C++0x, ~2012)

This is the first complete rewrite of the engine but using C++.
C++ provides a better way to express abstract concepts for an engine and was the first attempt at engine standardization.

## Alpha version 0.1 [ARCHIVED] (C, ~2010)

This is the first attempt to create an engine with OpenGL using C.
The idea was mainly to automate OpenGL rendering with a scene containing a sky, a ground, and entities.
