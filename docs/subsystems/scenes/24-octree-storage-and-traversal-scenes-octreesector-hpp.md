## Octree storage and traversal — `Scenes/OctreeSector.hpp` (2026-08-09)

**Storage invariant: one element, one sector — the deepest that FULLY CONTAINS it.**
`insertWithPrimitive()` descends only into a subsector that entirely contains the primitive
(`isFullyContaining()`); otherwise the element stays where it is. It replaces the former
*all-levels* storage, where an element was copied into every sector it touched at every depth —
the cause of the 104 GB above.

**Completed 2026-10-02 — every operation keeps it** (until then `expand()` still copied, see below):

| Operation | What it does |
|---|---|
| `insert()` | descends while a child contains the element entirely, stores it there |
| `expand()` | MOVES each element into the child that contains it entirely; a straddler stays in the parent |
| `collapse()` | pulls every descendant's element back up before releasing the children (they lived in ONE sector) |
| `update()` | finds the owner (first along the path the current volume would take, then the whole subtree); keeps it there while that sector still contains it whole and no child does, else erases it there and files it again from the root; an element outside the root stays AT the root |
| `erase()` | stops where it is found |
| `forEachElement()` | every element of the subtree, each ONCE — the way to list "all elements" (`elements()` is ONE sector's own set) |
| `getFirstElementNamed()` | searches the subtree, not one sector |
| auto-collapse (`isStillLeaf()`) | counts the SUBTREE (`subtreeElementCount()`), not the sector's own set |

⚠️⚠️ **What the leftover copy did (2026-10-02, citadel):** `expand()` kept a splitting sector's elements AND
inserted them in every child they touched (`insert()` tests overlap, not containment). An element filed before a
split lived in 2 to 7 sectors — about a hundred of citadel's statics (towers, merlons, stairs) were met 2 to 7 times
per physics cycle. A gate guard (a P4 kinematic character) met ITSELF: 0.91 m of penetration with its own capsule,
normal +Y, three depenetration passes pushed it 2.3 m under the one-sided ground, and it fell forever. It depended
on whether the paladin was filed before or after a split (4/4 falls polled from the first frames, 0/3 otherwise);
after: 0 duplicates and 0 falls in 4 runs, the 32-station bench bit-identical to before (no duplicate there).
`Scene::rebuildPhysicsOctree()` transferred only the ROOT's elements (the rendering rebuild had been fixed, this one
not); projet-alpha's `Act::processLogics()` walked the root of its actor octree (past 256 actors the actors added
after the split were never processed) and called `update()` inside that walk — both now use `forEachElement()`, the
Act into a reused vector before anything moves. The physics step reports a duplicated body in Debug
(`adjacent_find` after its creation-order sort). The rendering gathers' stamp
(`AbstractEntity::markCollectedByRenderingGather()`) is redundant now and stays as a cheap guard.

> [!CAUTION]
> **The invariant changes what a traversal must read, and getting that wrong is silent.**
>
> An element that straddles a boundary now sits on an INNER node. The bigger it is, the higher it
> sits: **a ground plane straddles every boundary and lives at the ROOT.** Two consequences, both
> of which shipped as bugs before being caught:
>
> 1. **Reading `sector.elements()` of leaves only misses everything large.** The physics broad
>    phase did exactly that (the former leaf-only traversal + `elements()`), so no body was ever offered the
>    ground as a collider — **you fall through the floor**, with no error anywhere.
> 2. **`m_elements.empty()` no longer proves an empty subtree.** An inner node can hold nothing
>    while its children are full, so the old early-exit prunes populated branches.
>
> **Contract (rewritten 2026-09-03):** `forEachSector()` visits EVERY sector that owns at least one
> element — inner nodes included, that is where every straddling element lives — and hands the
> callback `(sector, candidates, ownedOffset)`, where `candidates` is `elements of ALL ancestors ∪
> sector.elements()` accumulated on the way down into a single reused buffer. Never call
> `elements()` on a sector to build a query result.
>
> **The pairing rule — and the ONLY rule.** A caller that tests pairs tests, at each sector,
> `owned × owned` and `owned × inherited`, nothing else. Every pair whose bounds can overlap is then
> produced **exactly once**: two elements owned by the same sector meet there; an element owned by
> a descendant meets an element owned by an ancestor at the DESCENDANT, where the latter is
> inherited; two elements owned by disjoint subtrees have disjoint bounds by the storage invariant.
> **No cross-sector deduplication exists, and none must come back.** A caller acting per element
> acts on the OWNED range `[ownedOffset, size)` only — each element is owned by exactly one
> sector — and reaches the subtree of that sector with `forTouchedSector(aabb)`.
>
> ⚠️⚠️ **What the previous contract cost — measured (2026-09-03, projet-alpha `game-logic`, 121
> nodes, 138 static entities, RTX 3500 Ada, validation layers ON).** The former `forLeafSectors()`
> visited leaves only, so the physics phase 2 paired ALL candidates in EVERY leaf and deduplicated
> with a `std::unordered_set< uint64_t >` of pair keys. Every inherited × inherited pair — the
> player, the walls, the crate at the origin, anything straddling a split plane — was re-hashed
> once per leaf below it. The logic thread was **CPU-saturated at 22.8 ms per tick (p50; p90
> 28.2 ms)**, every cycle over the 16.7 ms budget, **99 % of it in `resolveCollisions()`** (hash
> insert ≈ 57 %, `createEntityPairKey` ≈ 8.5 %). The owner attributed the slowdown to the Aug 26
> two-state synchronisation commits; the measurement said otherwise — `publishStateForRendering()`
> and the node logic were under 1 % combined. After the rewrite the same thread spends
> **≈ 1.2 s of CPU over 15 s (≈ 1.3 ms per tick, 12× less)**, all of it in the narrow phase
> (`OrientedCuboid::set`, `detectCollisionMovableToMovable`), zero warnings, zero VUID.
> **Rule:** a "dedup set" that grows with the number of sectors is not an optimisation, it is the
> symptom of a traversal that produces duplicates. Fix the traversal.
>
> ⚠️ `ownedOffset` is not decoration. Candidates before it are INHERITED, and any per-sector
> predicate is unsound for them — `isTouchingRootBorder()` in particular: an inherited element may
> straddle the world edge while being met in a sector that does not touch it. Only elements the
> sector OWNS are fully inside it.
>
> ⚠️ The leaf-only version also had a **correctness hole**: `resolveCollisions()` phase 1 corrected
> a straddling body in the FIRST leaf that met it, against that leaf's candidates only — the statics
> owned by the sibling leaves it also straddled were never tested. Phase 1 now runs each movable
> once, at its owning sector, against the inherited statics plus `forTouchedSector(aabb)` over the
> sector's subtree (`Scene::accumulateStaticEntityCorrections()`).

**The render lists QUERY the rendering octree since 2026-09-25** (owner: "use the octree"). ⚠️⚠️ **A scene is not drawn before its first state publication** (`Scene::hasPublishedStateForRendering()`, gated in `Core::renderingTask()`): the render thread's triple-buffer slot 0 is never written before it, so the first frame read every entity at the ORIGIN — `terrain` drew 817 745 LOD-0 tree instances, 55.7 G triangles, and hung the macOS/Windows GPUs (`docs/caution-points.md` § *NEVER-WRITTEN slot*). ⚠️⚠️ **Each gather DE-DUPLICATES** (`AbstractEntity::markCollectedByRenderingGather()`, a per-gather stamp): until 2026-10-02 `OctreeSector::expand()` kept a splitting sector's elements in the parent AND filed them in the children, so an entity sat in several sectors (fixed above; the stamp stays as a guard) — the first version drew every copy and hung the macOS and Windows GPUs on `terrain` (1.09 G triangles at the spawn instead of 40.6 M; `docs/caution-points.md` § *drew an entity once per sector copy*).
`Scene::gatherRenderingCandidates(acceptsBox, candidates)` walks it with a box test and collects the
entities the volume may see — a sector is an `AACuboid`, and every entity is owned by the deepest sector
that FULLY contains its render box, so a missed sector is skipped with its subtree. The raster list uses the
frustum (the view range for a cubemap), the RT list a sphere of `TLASDistance`, the shadow lists the caster
volume (the light's range for a cubemap); the entity-level tests then run BEFORE its components are visited.
Collected under `m_renderingOctreeAccess`, processed after it — an entity's component lock is never taken
inside the octree's. Entities the octree cannot file (outside its bounds) live in `m_renderingOctreeOverflow`,
always walked; `rebuildRenderingOctree()` now transfers EVERY sector's elements (it transferred the root's,
i.e. nearly nothing). Before, the lists walked every entity and component and tested the volume last:
`terrain`'s 12 776 forest cells cost ~90 ms of CPU per frame (9 FPS for 32 ms of GPU), 22-23 ms after.
`docs/caution-points.md` § *The render lists walked EVERY entity*.
⚠️⚠️ **An entity leaves the octree ONLY through `SubNodeDeleting`** (or `removeStaticEntity()`), and the
octree holds it by `shared_ptr`: a node removed without that notification is not freed, it stays DRAWN and
TRACED for the rest of the session. `Node::destroyChild()` was a bare map erase and `destroyChildren()` never
announced the descendants — a retired explosion sprite pushed `game-logic` from 54 to 202 ms per frame (fixed
2026-09-26: every removal path now announces every node of the subtree, `trimTree()`'s teardown). Never remove a
child from a node's map by any other route. `docs/caution-points.md` § *a node removed by `destroyChild()` stayed
DRAWN and TRACED forever*.

> [!CAUTION]
> `StaticEntity::isVisibleTo()` tests the **collision model** AABB, or a bare point when there is
> none. The `renderBoundingBox()` introduced for exactly this purpose is read by no culling path
> yet — the render/collision split is only half wired.

### When the scene refiles an entity (2026-10-02)
`Scene::checkEntityLocationInOctrees()` files an entity in the rendering and physics octrees. It runs on a content
notification (any thread; `setCollisionModel()` notifies too, so a body enters the physics step at once), after the
physics step for the bodies it moved, and in the logic cycle's node crawl for a node whose `processLogics()` reports a
move OR whose world frame changed since its last logic-thread filing (`AbstractEntity::movedSinceFiled()` /
`recordFiledFrame()`, position + upward + backward). The second test catches what a node's own `processLogics()`
never reports: a child moved by its parent, a frame set by a component (a vehicle's wheels), an animated hierarchy,
any non-collidable mover — before it, such a node stayed in its first sector and was culled with it.
