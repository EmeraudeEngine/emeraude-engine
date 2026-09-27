## CRITICAL Attention Points

| Point | Importance | Description |
|-------|------------|-------------|
| **Thread safety** | CRITICAL | Atomic status + mutex on lists |
| **Deadlock prevention** | CRITICAL | Virtual calls OUTSIDE lock |
| **Async capture safety** | CRITICAL | No `this` capture in getOrCreateResource lambdas |
| **Cycle detection** | HIGH | Automatic DFS in addDependency() |
| **Memory management** | HIGH | `shared_ptr` for reference counting |
| **Status tracking** | MEDIUM | State machine: Unloaded → Loading → Loaded/Failed |
| **Cache efficiency** | MEDIUM | Key by resource name for reuse |
