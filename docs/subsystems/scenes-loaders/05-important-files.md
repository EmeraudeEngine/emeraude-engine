## Important Files

- `Scenes/Loaders/SceneData.hpp` — Common intermediate format (NodeDescriptor, MeshDescriptor, SceneData)
- `Scenes/Loaders/Interface.hpp` — Loader interface + LoaderOptions
- `Scenes/Loaders/GLTFLoader.hpp/.cpp` — glTF/GLB implementation. Also hosts `MeshoptBufferCache` +
  `MeshoptBufferAdapter` (`EXT_meshopt_compression` and its ratified successor `KHR_meshopt_compression`, the same
  bitstream plus the Color filter; fastgltf fills `BufferView::meshoptCompression` for both), defined in the **.cpp** because their interface
  speaks fastgltf types, which must never leak into a public engine header — hence the forward
  declaration in the .hpp and the out-of-line constructor **and** destructor.
  `DracoPrimitiveCache` (`KHR_draco_mesh_compression`) lives in the same .cpp for the same reason,
  but needs **no** forward declaration in the .hpp: unlike the meshopt cache it is not a member of
  the loader — it is a local of `loadMeshes()`, because Draco never reaches skins, animations or
  nodes. Its two read helpers `readDracoAwareAttribute()` / `readDracoAwareIndices()` are the single
  gate every attribute and index read goes through.
- `Graphics/KTX2Decoder.hpp/.cpp` — KTX2 container + Basis transcoder (`KHR_texture_basisu`)
- `Graphics/CompressedImageResource.hpp/.cpp` — the block-compressed counterpart of `ImageResource`
- `Scenes/Loaders/FBXLoader.hpp/.cpp` — FBX implementation (ufbx): opens the file, materializes the analysis
- `Scenes/Loaders/USDParser.hpp/.cpp` — the USD loader's analysis half: `USD::composeStage()`, `USD::analyse()`,
  `USD::forEachMesh()` (plain data, fuzzed by `src/Fuzzing/fuzz_usd.cpp`)
- `Scenes/Loaders/USDZArchive.hpp/.cpp` — a USDZ mapped (or held in memory) and its asset resolution
- `Scenes/Loaders/FBXParser.hpp/.cpp` — its analysis half, `FBX::analyse()` → `FBX::Asset` and `FBX::analyseClipsOnly()`
  (plain data, fuzzed by `src/Fuzzing/fuzz_fbx.cpp`)
- `Scenes/Loaders/WADLoader.hpp/.cpp` — Doom WAD level materializer (`FullBrightLuminance` lives in the header)
- `Scenes/Loaders/WADParser.hpp/.cpp` — its analysis half, `WAD::parse()` → `WAD::Level` (plain data, fuzzed by
  `src/Fuzzing/fuzz_wad.cpp`)
- `src/Fuzzing/build-fuzzers.sh` — the engine's libFuzzer targets (one per loader analysis)
- `Scenes/SceneDataConsumer.hpp/.cpp` — Scene-side consumer (Node/StaticEntity builder, honors `lightingEnabled`)
