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

### ⚠️ The scene graph has a DEPTH CAP: `Node::MaxDepth` = 256 (triad, 2026-09-30)

- **Why**: a child's notification climbs the tree HOP BY HOP (`Node::onUnhandledNotification()` → `notify()` → the
  parent … → the scene), and every graph walk recurses (`destroyTree`, `trimTree`, `destroyChildren`,
  `onLocationDataUpdate`, ModelViewer's framing). A dropped 200 000-level glTF with a node animation (so ModelViewer's
  NODE mode) overflowed the stack in `Scene::onNotification()` — reproduced.
- **The rule** (owner decision): `Node` stores its `depth()` (the root is 0; fixed at construction, a node is never
  re-parented); `createChild()` REFUSES a node deeper than `MaxDepth` (an error, `nullptr`). 256 = 8× the deepest
  real asset measured (30 levels, 349 assets: `RecursiveSkeletons`; `Dragon.glb` 22).
- `createChild()` also returns `nullptr` for a DUPLICATE name at the same level: `SceneDataConsumer` now handles both
  (the subtree is skipped, `build()` returns false); it used to dereference the null node.
- `forEachComponent()` / `forEachModifiers()` call their callable as an LVALUE: it is invoked once per element, a
  `std::forward` inside the loop would reuse a moved-from rvalue callable (clang-tidy bugprone-use-after-move).

