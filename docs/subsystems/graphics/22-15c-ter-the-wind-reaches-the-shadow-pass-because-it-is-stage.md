## 15c-ter. The wind reaches the shadow pass because it is staged before it (Sept 2026)

⚠⚠ `Renderer::renderShadowMaps()` is recorded **before every `Scene::prepareRender()` of the
frame**. The vegetation wind is therefore staged and uploaded in `Scene::updateVideoMemory()`,
which runs immediately before it, and `Scene::castShadows()` fetches the CURRENT frame's
`InstanceTransforms` descriptor set rather than the one `prepareRender()` prepares.

Staged in `prepareRender()` as it used to be, the wind reached the shadow one frame late and the
shadow pass bound another frame-in-flight slot entirely — outside its own fence. Full account and
the misleading error message it produced: `docs/caution-points.md` § *The shadow pass is recorded
BEFORE every prepareRender()*.

⚠️ The previous view-projection half of that same header stays in `prepareRender()`: it needs the
primary view target. The wind is a scene property and needs none — that is why only one of the two
moved.
