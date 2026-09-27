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
