## Critical Points

- **Smart pointers**: shared_ptr and weak_ptr for automatic hierarchy management
- **Manager and Scene**: Handle fail-safe construction/destruction (in development)
- **Root Node**: Immutable, cannot move nor receive Components
- **NodeCrawler**: the iteration NEVER yields the base node; `currentNode()` is the base node before the first `fetchNextNode()` and `nullptr` after the last. Process the base node BEFORE the loop when you need it
- **Y-up convention**: CartesianFrame uses Y-up everywhere — `localYAxis()` for any STRUCTURAL read (the basis Y column), `upwardVector()`/`downwardVector()` reserved for code actually talking about gravity
- **No world cache**: On-demand recalculation (future optimization planned)
- **Observers**: Automatic registration, do not register manually
- **Suspend/Wakeup**: Every new Component MUST implement `onSuspend()`/`onWakeup()` (pure virtual)
- **Friend class**: `AbstractEntity` is friend of `Component::Abstract` to access protected hooks
- **Auto collision models**: Visual components auto-generate collision models - disable for gizmos!

### ⚠️ Every displacement notifies — `setLocalCoordinates()` included (2026-11-26)

- **The contract**: every mutator of `LocatableInterface` on a `Node` or a `StaticEntity` ends with
  `onLocationDataUpdate()`: `move()` on each component of the entity, then the same down every sub-node, then the
  node's physics simulation is woken. A component that keeps a position of its own (the lights' buffers, a sound
  emitter's source, the push modifiers, `MultipleVisuals`) follows through `move()` ONLY; the drawn objects read the
  node's frame every frame, so a missing notification is invisible on the picture.
- **The defect it had**: `setLocalCoordinates()` (the whole frame at once) wrote the frame and notified nobody.
  Measured before the fix, projet-alpha `water-world`: after `Act.setPosition` (which uses it) the microphone kept
  the listener at the old place, the head's sound emitter stayed 18 m behind (−25 dB), and the player's flashlight
  (a spot light under the head) received no `move()` over two placements 400 m apart, until the head turned. After:
  one `move()` per placement at the right position; the emitter within 0.4 dB of a placement followed by a head turn.
- **Who calls it every cycle**: `NodeAnimation::evaluateAtTime()` (each node its clip drives) and
  `Vehicle` (each wheel). Cost measured on the animation's write loop (Release, mean of 600 cycles, `asset-loader`
  with ChronographWatch, one animated node): 60 to 180 ns a cycle before, 1.0 to 1.5 µs notifying; a logic cycle is
  16.7 ms. ⚠️ Not measured: a clip driving many nodes of ONE hierarchy — each written node walks its whole subtree
  (engine item `node-animation-notifies-per-written-node`).
- Verified after the fix (Linux, validation layer loaded): `asset-loader` (the watch's clip), `city` (the cars) and
  `water-world`, 0 VUID, no root-node warning, 0 frame reading a state being rewritten.
- macOS (engine 182fd2bb, Apple M6, MoltenVK, 2026-11-26): the build without a warning (AppleClang,
  `-Wdocumentation -Werror`), the same three demos with 0 VUID and 0 frame reading a state being rewritten; the
  emitter placed without a head turn at −37.8 dB against −38.6 dB with one (Linux: −38.6 / −39.0).
- Windows (engine 182fd2bb, RTX 3500 Ada, MSVC 14.44, 2026-11-26): Release and Debug without a warning, the same
  three demos with 0 VUID and 0 frame reading a state being rewritten; the emitter at −38.0 dB without a head turn
  against −37.8 dB with one. AMD: not tested.
- `Physics::Particle::setLocalCoordinates()` is not an entity: it has nobody to notify.

### ⚠️ The scene graph has a DEPTH CAP: `Node::MaxDepth` = 256 (triad, 2026-09-30)

- **Why**: a child's notification climbs the tree HOP BY HOP (`Node::onUnhandledNotification()` → `notify()` → the
  parent … → the scene), and every graph walk recurses (`destroyTree`, `trimTree`, `destroyChildren`,
  `onLocationDataUpdate`, ModelViewer's framing). A dropped 200 000-level glTF with a node animation (so ModelViewer's
  NODE mode) overflowed the stack in `Scene::onNotification()` — reproduced.
- **The rule** (owner decision): `Node` stores its `depth()` (the root is 0; fixed at construction, a node is never
  re-parented); `createChild()` REFUSES a node deeper than `MaxDepth` (an error, `nullptr`). 256 = 8× the deepest
  real asset measured (30 levels, 349 assets: `RecursiveSkeletons`; `Dragon.glb` 22).
- `createChild()` also returns `nullptr` for a DUPLICATE name at the same level. `SceneDataConsumer` renames the
  duplicates before it creates a child (`<name>-<nodeIndex>`, the first holder verbatim, 2026-11-14: a node name is
  not unique in glTF or FBX — `docs/caution-points.md` § *A duplicate ENTITY NAME cost the whole node*); a refusal
  that remains (deeper than `MaxDepth`, or a renamed name that still collides) skips the subtree and `build()` returns
  false; it used to dereference the null node.
- `forEachComponent()` / `forEachModifiers()` call their callable as an LVALUE: it is invoked once per element, a
  `std::forward` inside the loop would reuse a moved-from rvalue callable (clang-tidy bugprone-use-after-move).

