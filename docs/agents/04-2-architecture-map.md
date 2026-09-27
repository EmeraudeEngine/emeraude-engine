## 2. Architecture Map

| Category | System | Path | Context |
|---|---|---|---|
| **Framework** | Core / Tracer | [`src/AGENTS.md`](../../src/AGENTS.md) | App lifecycle, Logging. |
| | **Foundation** (`EmEn::Base`) | [`dependencies/emeraude-base/AGENTS.md`](../../dependencies/emeraude-base/AGENTS.md) | **Extracted to the standalone [emeraude-base](https://github.com/EmeraudeEngine/emeraude-base) library** — math, factories (Pixel/Wave/Vertex), traits, I/O, hashing, threading, skeletal-animation data types. Formerly `src/Libs/` (`EmEn::Libs`). |
| | Platform | [`src/PlatformSpecific/AGENTS.md`](../../src/PlatformSpecific/AGENTS.md) | OS implementations. |
| | Testing | [`dependencies/emeraude-base/src/Testing/AGENTS.md`](../../dependencies/emeraude-base/src/Testing/AGENTS.md) | Unit tests live in emeraude-base (`EmeraudeBaseUnitTests`). |
| **Graphics** | **Graphics Layer** | [`src/Graphics/AGENTS.md`](../../src/Graphics/AGENTS.md) | **Start Here**. High-level. |
| | Vulkan Layer | [`src/Vulkan/AGENTS.md`](../../src/Vulkan/AGENTS.md) | Low-level abstraction. |
| | Saphir (Shader) | [`src/Saphir/AGENTS.md`](../../src/Saphir/AGENTS.md) | Shader generation. |
| **Sim** | **Physics** | [`src/Physics/AGENTS.md`](../../src/Physics/AGENTS.md) | Physics system. |
| | Audio | [`src/Audio/AGENTS.md`](../../src/Audio/AGENTS.md) | OpenAL spatial audio. |
| | Input | [`src/Input/AGENTS.md`](../../src/Input/AGENTS.md) | Keyboard/Mouse/Pad. |
| **Data** | Resources | [`src/Resources/AGENTS.md`](../../src/Resources/AGENTS.md) | Async loading. |
| | Scenes::Loaders | [`src/Scenes/Loaders/AGENTS.md`](../../src/Scenes/Loaders/AGENTS.md) | Composite format loaders (glTF, FBX stub). |
| | Scenes | [`src/Scenes/AGENTS.md`](../../src/Scenes/AGENTS.md) | Scene graph. |
| | Animations | [`src/Animations/AGENTS.md`](../../src/Animations/AGENTS.md) | *In Dev*. Skeletal data types in emeraude-base (`Base::Animation`), runtime eval here. |
| **Tools/UI** | Overlay (ImGui) | [`src/Overlay/AGENTS.md`](../../src/Overlay/AGENTS.md) | UI & Debug. |
| | Console | [`src/Console/AGENTS.md`](../../src/Console/AGENTS.md) | Runtime control (TCP + in-game). |
| | AVConsole | [`src/Scenes/AVConsole/AGENTS.md`](../../src/Scenes/AVConsole/AGENTS.md) | Virtual devices. |
| | Scene Editor | [`src/Scenes/Editor/AGENTS.md`](../../src/Scenes/Editor/AGENTS.md) | Picking, gizmos, entity manipulation. |
| | Tool | [`src/Tool/AGENTS.md`](../../src/Tool/AGENTS.md) | Editor tools. |
| **Net** | Networking | [`src/Net/AGENTS.md`](../../src/Net/AGENTS.md) | HTTPS download manager (URL-keyed cache, `ExternalData` resources), UDP/multicast/SSDP, interface enumeration, TCP client/server, Serial, WiFi. |
