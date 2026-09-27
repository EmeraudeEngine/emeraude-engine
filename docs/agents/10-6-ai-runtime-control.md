## 6. AI Runtime Control

The engine exposes a **Remote Console** (TCP port 7777) that enables AI agents to control
a running application autonomously. This is the foundation for AI-driven 3D content creation.

**Capabilities:**
- **Resource discovery** — query available skyboxes, meshes, materials
- **Scene creation** — via live commands or JSON scene descriptions
- **Camera control** — position, orientation, visual verification via screenshots
- **Audio control** — full TrackMixer playback management
- **Settings/window** — runtime configuration changes

**Reference:** [`docs/ai-runtime-control.md`](../ai-runtime-control.md) — **THE complete guide** for
any AI operating the engine at runtime. Contains the full command reference, JSON scene format
specification, spatial orientation tutorial, and visual feedback loop methodology.

**Console system:** [`src/Console/AGENTS.md`](../../src/Console/AGENTS.md) — Architecture, command flow,
how to add new commands.
