## Node Tree Iteration — NodeCrawler Contract

`NodeCrawler< node_t >` (`Scenes/NodeCrawler.hpp`) is the only way the Scene walks the node tree —
**12 call sites**: `Scene.cpp` ×2, `Scene.entities.cpp` ×5, `Scene.rendering.cpp` ×5. The API is
`bool fetchNextNode()` plus the accessor `const std::shared_ptr< node_t > & currentNode()`; it
replaced `std::shared_ptr< node_t > nextNode()`.

> [!CAUTION]
> **⚠️⚠️ The iteration NEVER yields the base node.**
> - **Before** the first `fetchNextNode()`, `currentNode()` **is the base node** — the constructor
>   seats it through `populateStack()`.
> - **After** `fetchNextNode()` returns `false`, `currentNode()` is `nullptr` — so a while-loop body
>   never sees a null node.
>
> Consequence for callers, and this is the whole point of the contract:
> ```cpp
> while ( crawler.fetchNextNode() ) { use(crawler.currentNode()); }   // DESCENDANTS ONLY
> ```
> A caller that must also process the base node handles it **before** the loop. Two sites in
> `Scene.entities.cpp` do exactly that: **`getNodeStatistics()`** (the root's children count and
> depth) and the `showTree` dump in **`getNodeSystemStatistics()`** (the root's own line). Both were
> `do/while` loops and are now "process the base node, then `while (...)`", with the body factored
> into a lambda so nothing is duplicated, each carrying a ⚠️ comment on the pre-loop call. The
> conversion was forced by clang-tidy's `cppcoreguidelines-avoid-do-while`; the contract itself did
> not change, only the shape of those two call sites.

### ⚠️⚠️ Caution: The Stale-Local Bug (IT COMPILED CLEANLY)

When the crawler lost its return value, all 12 call sites kept the local they used to assign:

```cpp
// BROKEN — compiles, zero warnings, wrong on every iteration
const auto currentNode = m_rootNode;          // leftover of the old form:
while ( crawler.fetchNextNode() )             //   while ( (currentNode = crawler.nextNode()) != nullptr )
{
    currentNode->doSomething();               // ALWAYS THE ROOT NODE
}
```

The loops ran the correct NUMBER of times but operated **on the root node every iteration**.
Per-site effect:

| Site | Effect of the stale local |
|------|---------------------------|
| `Scene::processLogics()` | ran N times on the root, never on any other node |
| `Scene::findNode()` | could only ever match the root |
| `getNodeStatistics()` / tree dump | counted the root's children N times |
| Camera / microphone detection | inspected the root only |
| The 5 rendering crawlers (`Scene.rendering.cpp`) | would have rendered nothing node-attached |

**Why it was nearly invisible on `doom-loader`:** the Doom level is a `StaticEntity` — the
`m_staticEntities` path, which does not use the crawler at all — and the only `Node` is the player
actor, which carries no visible mesh. `doom-loader` is therefore a **weak target** for this and must
not be used as the witness; a demo with node-attached visuals is the real test.

**Validated at runtime on the `animation-debug` demo (Aug 2026):** node-attached visuals render and
animate correctly. That witness is decisive, not merely reassuring — with the stale local the
crawlers only ever saw the ROOT node, so node-attached meshes rendered **not at all** (last row of
the table above). Seeing them render *and* animate proves the five rendering crawlers
(`Scene.rendering.cpp`) **and** the `Scene::processLogics()` crawler are repaired.

### ⚠️ Dead End: Never Make `m_currentNode` a Reference Member

An intermediate attempt declared the member as a reference to the caller's smart pointer. Two
reasons never to retry it:

1. **It does not compile.** Binding `shared_ptr< const Node > &` / `shared_ptr< Node > &` to const
   lvalues → *"discards qualifiers"*, 6 instantiations.
2. **It would have been wrong anyway.** Assigning through the reference writes into the **CALLER's**
   variable, and two sites pass `m_rootNode` directly — the terminal `m_currentNode = nullptr` at
   the end of the iteration would have **nulled the scene's root node**.

**Residual nits deliberately left alone:** `populateStack()` also assigns `m_currentNode` (the
constructor DEPENDS on that side effect to seat the base node — an implicit coupling), and the class
constrains `requires std::is_class_v< Node >` instead of `node_t` (inert — true either way).
