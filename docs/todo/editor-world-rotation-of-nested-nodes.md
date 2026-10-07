---
id: editor-world-rotation-of-nested-nodes
title: Scenes — Node::rotate(TransformSpace::World) rotates in the parent's space
status: open
priority: high
scope: Scenes / Node
opened: 2026-09-29
tags: [scenes, node, transforms, editor]
---

# Scenes — Node::rotate(TransformSpace::World) rotates in the parent's space

## Why

`Node::rotate(radian, axis, TransformSpace::World)` calls `m_logicStateCoordinates.rotate(radian, axis, false)` on
the node's LOCAL frame (relative to its parent): the axis is taken in the PARENT's space and the position orbits the
PARENT's origin. That is the world only for a child of the root node. Every caller asking for World on a deeper node
of a rotated parent gets a different rotation.

Found while writing the editor's group rotation (2026-09-29): the editor turns each selected entity with
`rotate(World)` + position restored, then turns its offset from the pivot itself. Exact for children of the root
(every case measured), wrong for a nested node under a rotated parent. A single entity in Local space keeps the
exact local path and is not affected.

## What remains

- Decide the contract with the owner: World = true world axis (convert the axis into the parent's space, rotate the
  position around the WORLD origin through the parent), or rename the enum value's meaning.
- Then check the other `TransformSpace::World` paths of `Node` (`pitch`, `yaw`, `roll`, `translate`…) the same way.
- A unit test on a two-level hierarchy with a rotated parent.

## ⚠️ Traps

- `StaticEntity` has no parent: unaffected.
- The editor's rotation code comments point here (`Scenes/Editor/Manager.cpp`, `onPointerMove`).

## References

- `src/Scenes/Node.cpp` `Node::rotate()`; `CartesianFrame::rotate(radian, axis, bool local)`.
- `docs/subsystems/scenes/25-editor-selection-states-group-transform-panel.md` § Group transformation.
