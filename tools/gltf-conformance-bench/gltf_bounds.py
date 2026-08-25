#!/usr/bin/env python3
"""
Computes the world-space axis-aligned bounding box of a glTF/GLB asset by walking the
node hierarchy WITH its transformations.

The mesh-local bbox lies: MetalRoughSpheresNoTextures is ONE sphere instanced many times,
whose mesh bbox is nearly zero. The scene bbox is only correct once every node transform is
composed down the hierarchy.

Since the Y-up migration the glTF frame IS the engine frame (identity import), so the box
computed here is directly the world box the engine sees. Any disagreement is a defect.
"""

import json
import math
import struct
import sys
from pathlib import Path

COMPONENT_SIZES = {5120: 1, 5121: 1, 5122: 2, 5123: 2, 5125: 4, 5126: 4}
TYPE_COUNTS = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT2": 4, "MAT3": 9, "MAT4": 16}


def identity():
    return [1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0]


def mat_mul(a, b):
    """Column-major 4x4 multiply, same layout as glTF's `matrix` array: result = a * b."""
    out = [0.0] * 16
    for col in range(4):
        for row in range(4):
            out[col * 4 + row] = sum(a[k * 4 + row] * b[col * 4 + k] for k in range(4))
    return out


def trs_matrix(node):
    """Builds the local matrix of a node, either the explicit one or T * R * S."""
    if "matrix" in node:
        return list(node["matrix"])

    t = node.get("translation", [0.0, 0.0, 0.0])
    r = node.get("rotation", [0.0, 0.0, 0.0, 1.0])  # x, y, z, w
    s = node.get("scale", [1.0, 1.0, 1.0])

    x, y, z, w = r
    rot = [
        1 - 2 * (y * y + z * z), 2 * (x * y + z * w), 2 * (x * z - y * w), 0.0,
        2 * (x * y - z * w), 1 - 2 * (x * x + z * z), 2 * (y * z + x * w), 0.0,
        2 * (x * z + y * w), 2 * (y * z - x * w), 1 - 2 * (x * x + y * y), 0.0,
        0.0, 0.0, 0.0, 1.0,
    ]

    # Scale post-multiplies (scales the rotated basis vectors), translation goes in the last column.
    out = list(rot)
    for col in range(3):
        for row in range(3):
            out[col * 4 + row] *= s[col]
    out[12], out[13], out[14] = t[0], t[1], t[2]

    return out


def transform_point(m, p):
    return (
        m[0] * p[0] + m[4] * p[1] + m[8] * p[2] + m[12],
        m[1] * p[0] + m[5] * p[1] + m[9] * p[2] + m[13],
        m[2] * p[0] + m[6] * p[1] + m[10] * p[2] + m[14],
    )


def load_asset(path):
    """Returns (gltf_json, buffers) for a .gltf or .glb file."""
    path = Path(path)
    data = path.read_bytes()

    if data[:4] == b"glTF":
        _, _, total = struct.unpack_from("<III", data, 0)
        offset = 12
        gltf = None
        bin_chunk = None
        while offset < total:
            length, kind = struct.unpack_from("<II", data, offset)
            payload = data[offset + 8: offset + 8 + length]
            if kind == 0x4E4F534A:
                gltf = json.loads(payload.decode("utf-8"))
            elif kind == 0x004E4942:
                bin_chunk = payload
            offset += 8 + length + ((4 - length % 4) % 4 if length % 4 else 0)
        buffers = [bin_chunk if bin_chunk is not None else b""]
        # A GLB may still reference external buffers past the embedded chunk.
        for index, buffer in enumerate(gltf.get("buffers", [])):
            if index == 0 and "uri" not in buffer:
                continue
            buffers = buffers[:index] + [read_buffer(path, buffer)] + buffers[index + 1:]
        return gltf, buffers

    gltf = json.loads(data.decode("utf-8"))
    return gltf, [read_buffer(path, buffer) for buffer in gltf.get("buffers", [])]


def read_buffer(gltf_path, buffer):
    uri = buffer.get("uri")
    if uri is None:
        return b""
    if uri.startswith("data:"):
        import base64
        return base64.b64decode(uri.split(",", 1)[1])
    from urllib.parse import unquote
    return (Path(gltf_path).parent / unquote(uri)).read_bytes()


def accessor_min_max(gltf, buffers, accessor_index):
    """Returns (min, max) of a POSITION accessor, reading the data when the hints are absent."""
    accessor = gltf["accessors"][accessor_index]

    if "min" in accessor and "max" in accessor:
        return list(accessor["min"][:3]), list(accessor["max"][:3])

    # No hints: decode the positions. Rare, but a missing bbox must never silently read as zero.
    view = gltf["bufferViews"][accessor["bufferView"]]
    buffer = buffers[view.get("buffer", 0)]
    comp_size = COMPONENT_SIZES[accessor["componentType"]]
    comp_count = TYPE_COUNTS[accessor["type"]]
    stride = view.get("byteStride") or comp_size * comp_count
    base = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)

    lo = [math.inf] * 3
    hi = [-math.inf] * 3
    for i in range(accessor["count"]):
        offset = base + i * stride
        values = struct.unpack_from("<3f", buffer, offset)
        for axis in range(3):
            lo[axis] = min(lo[axis], values[axis])
            hi[axis] = max(hi[axis], values[axis])
    return lo, hi


def scene_bounds(path):
    gltf, buffers = load_asset(path)
    nodes = gltf.get("nodes", [])
    meshes = gltf.get("meshes", [])

    scene_index = gltf.get("scene", 0)
    scenes = gltf.get("scenes", [{"nodes": list(range(len(nodes)))}])
    roots = scenes[scene_index].get("nodes", [])

    lo = [math.inf] * 3
    hi = [-math.inf] * 3
    mesh_count = 0

    def visit(node_index, parent):
        nonlocal mesh_count
        node = nodes[node_index]
        world = mat_mul(parent, trs_matrix(node))

        if "mesh" in node:
            for primitive in meshes[node["mesh"]].get("primitives", []):
                position = primitive.get("attributes", {}).get("POSITION")
                if position is None:
                    continue
                mesh_count += 1
                pmin, pmax = accessor_min_max(gltf, buffers, position)
                # The 8 corners of the local box, transformed: the rotated box's AABB.
                for cx in (pmin[0], pmax[0]):
                    for cy in (pmin[1], pmax[1]):
                        for cz in (pmin[2], pmax[2]):
                            wx, wy, wz = transform_point(world, (cx, cy, cz))
                            for axis, value in enumerate((wx, wy, wz)):
                                lo[axis] = min(lo[axis], value)
                                hi[axis] = max(hi[axis], value)

        for child in node.get("children", []):
            visit(child, world)

    for root in roots:
        visit(root, identity())

    if mesh_count == 0:
        raise RuntimeError(f"{path}: no mesh primitive carries a POSITION attribute")

    centre = [(lo[axis] + hi[axis]) * 0.5 for axis in range(3)]
    extent = [hi[axis] - lo[axis] for axis in range(3)]
    radius = 0.5 * max(extent)

    return {
        "min": lo,
        "max": hi,
        "centre": centre,
        "extent": extent,
        "radius": radius,
        "primitives": mesh_count,
    }


if __name__ == "__main__":
    for argument in sys.argv[1:]:
        result = scene_bounds(argument)
        print(json.dumps({"file": argument, **result}, indent=2))
