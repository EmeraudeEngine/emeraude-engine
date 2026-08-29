#!/usr/bin/env python3
"""
Generates VolumeAbsorptionProbe.glb — the bench's ONLY asset that exercises
KHR_materials_volume's absorption, because no Khronos sample model does.

Why this exists
---------------
Across the 60 glTF files reachable from this repository (the vendored Khronos set plus
projet-alpha's own), 15 materials declare KHR_materials_volume, 2 declare `attenuationColor`
and ZERO declare `attenuationDistance`. Since the extension's default distance is +INFINITY,
the absorption of every one of them is the identity: the whole feature is invisible on the
available content, before OR after being implemented. So it cannot be regression-tested with
what Khronos ships, and a regression would pass unnoticed.

The three balls are identical transmissive glass except for the volume they declare, which
makes the capture self-controlling — the comparison lives inside ONE frame rather than
between two runs, so it is immune to any temporal variation:

  left    NoVolume           no extension at all          -> no absorption
  centre  ColourNoDistance   colour, NO distance          -> STILL no absorption
  right   ColourAndDistance  colour AND distance 0.25 m   -> visibly green

⚠️ The MIDDLE ball is the point of the whole asset. A reader's instinct is that declaring an
attenuation colour tints the glass; per spec it does NOT, because the distance defaults to
+infinity and the transmittance is exp(log(colour) / distance * thickness). That subtlety is
exactly what a well-meaning "fix" would break, and it is why the neutral middle ball must
stay neutral.

Expected reading (measured 2026-08-29, at the bench's own framing). The backdrop reads
(201, 200, 196); the criterion is the G/R ratio at each ball's centre:

  ball                 centre RGB               G/R
  NoVolume             (198.5, 197.1, 194.0)    0.993
  ColourNoDistance     (198.5, 197.1, 194.0)    0.993
  ColourAndDistance    (  4.6,  67.7,   7.5)   14.717

The two neutral balls transmit the panel essentially intact and are IDENTICAL to each other; the
third keeps ~13 % of the green and almost none of the red or blue, which is 0.6^4 — exactly what the
medium prescribes. A regression shows as the third ratio collapsing toward 1, or as the middle ball
drifting away from the first.

⚠️⚠️ THE SPHERE WINDING IS LOAD-BEARING. These triangles were wound the wrong way round until
2026-08-29 and the asset looked PERFECTLY NORMAL: a closed convex mesh is identical under a winding
flip, back-face culling simply keeps the far side. What it broke was the shading — the outward normal
faced away from the camera, NdotV clamped to 0, and the Fresnel term pinned at 1: total reflection,
zero transmission, no matter what the transmission path did. Three engine hypotheses were burned
before the geometry was suspected. If these balls ever go dark again, output `1.0 - F` before
touching the engine.

⚠️ Restart the engine after regenerating this file. The resource manager serves a CACHED material
for an asset already loaded in the process, and a recapture then comes back byte-identical, which
reads exactly like "the change did nothing".

⚠️ GLB packing trap, paid once: the JSON chunk must be padded with SPACES (0x20) and the BIN
chunk with zeros. Padding the JSON with NULs yields "An error occurred while parsing the
JSON" and no hint as to why.

Run:  ./make-volume-probe.py        (writes into assets/, next to this script)
"""

import json
import math
import struct
from pathlib import Path

RINGS = 24
SECTORS = 48
RADIUS = 0.5

# A green medium, chosen dark enough in red and blue that the absorption is unmistakable
# rather than a subtle tint the eye could argue with.
ATTENUATION_COLOUR = [0.05, 0.6, 0.25]
#
# ⚠️⚠️ THICKNESS and DISTANCE are chosen as a PAIR. Beer-Lambert here is
# `exp(log(colour) / distance * thickness)`, so only their RATIO sets the absorption: 0.2 / 0.05 is
# the same medium as the 1.0 / 0.25 this probe used until Aug 2026. What changed is that thickness
# is ALSO the LENGTH of the refraction ray since the screen-space refraction was rewritten, and at
# 1.0 — the ball's own diameter — the refracted sample landed a whole world unit away, off the
# backdrop and onto the sky. Shrinking thickness 5x keeps the absorption identical and pulls the
# sample back onto the panel. ⚠️ Change one without the other and you change the medium.
ATTENUATION_DISTANCE = 0.05
THICKNESS = 0.2

# ⚠️⚠️ THE BACKDROP IS PART OF THE INSTRUMENT, not decoration. Absorption is a MULTIPLICATION:
# with nothing bright behind the balls it multiplies near-black by near-black and measures nothing.
# The probe floated against the viewer's dark trees from Aug 2026 and stopped discriminating the
# moment the additive light-pass transmission term — which it had been reading absorption THROUGH —
# was removed. A white, opaque, fully lit panel is what gives the transmitted light something to
# absorb.
#
# It is deliberately UNIFORM rather than a checker like TransmissionTest's cloth: this probe
# measures ABSORPTION, and a flat field removes the refraction displacement as a confound — a
# refracted sample of a uniform panel is the same colour wherever it lands. Isolate one variable.
#
# ⚠️ It sits at NEGATIVE Z because the bench's `front` view puts the camera at +Z
# (bench.py::camera_position, azimuth 0 -> centre + (0, 0, distance)).
BACKDROP_Z = -0.9
BACKDROP_HALF_WIDTH = 2.1
BACKDROP_HALF_HEIGHT = 0.9


def uv_sphere():
    """A plain UV sphere: positions, normals and a triangle index list."""
    positions, normals, indices = [], [], []

    for ring in range(RINGS + 1):
        phi = math.pi * ring / RINGS

        for sector in range(SECTORS + 1):
            theta = 2.0 * math.pi * sector / SECTORS
            x = math.sin(phi) * math.cos(theta)
            y = math.cos(phi)
            z = math.sin(phi) * math.sin(theta)
            normals.append((x, y, z))
            positions.append((x * RADIUS, y * RADIUS, z * RADIUS))

    for ring in range(RINGS):
        for sector in range(SECTORS):
            a = ring * (SECTORS + 1) + sector
            b = a + SECTORS + 1
            # ⚠️ Winding matters and is invisible: see the module docstring.
            indices += [a, a + 1, b, a + 1, b + 1, b]

    return positions, normals, indices


def pad_zero(blob: bytes) -> bytes:
    return blob + b"\x00" * ((4 - len(blob) % 4) % 4)


def glass(name: str, volume: dict | None) -> dict:
    """Smooth, fully transmissive glass at IOR 1.5 — the only difference is the volume."""
    material = {
        "name": name,
        "pbrMetallicRoughness": {
            "baseColorFactor": [1, 1, 1, 1],
            "metallicFactor": 0,
            "roughnessFactor": 0,
        },
        "extensions": {
            "KHR_materials_transmission": {"transmissionFactor": 1.0},
            "KHR_materials_ior": {"ior": 1.5},
        },
    }

    if volume is not None:
        material["extensions"]["KHR_materials_volume"] = volume

    return material


def backdrop_quad():
    """A quad facing the camera (+Z normal), sized to sit behind all three balls."""
    x, y, z = BACKDROP_HALF_WIDTH, BACKDROP_HALF_HEIGHT, BACKDROP_Z

    positions = [(-x, -y, z), (x, -y, z), (x, y, z), (-x, y, z)]
    normals = [(0.0, 0.0, 1.0)] * 4
    indices = [0, 1, 2, 0, 2, 3]

    return positions, normals, indices


def build() -> bytes:
    positions, normals, indices = uv_sphere()
    quadPositions, quadNormals, quadIndices = backdrop_quad()

    position_bytes = b"".join(struct.pack("<3f", *v) for v in positions)
    normal_bytes = b"".join(struct.pack("<3f", *n) for n in normals)
    index_bytes = b"".join(struct.pack("<I", i) for i in indices)
    quad_position_bytes = b"".join(struct.pack("<3f", *v) for v in quadPositions)
    quad_normal_bytes = b"".join(struct.pack("<3f", *n) for n in quadNormals)
    quad_index_bytes = b"".join(struct.pack("<I", i) for i in quadIndices)

    binary = pad_zero(
        pad_zero(position_bytes) + pad_zero(normal_bytes) + pad_zero(index_bytes) +
        pad_zero(quad_position_bytes) + pad_zero(quad_normal_bytes) + pad_zero(quad_index_bytes)
    )
    normal_offset = len(pad_zero(position_bytes))
    index_offset = normal_offset + len(pad_zero(normal_bytes))
    quad_position_offset = index_offset + len(pad_zero(index_bytes))
    quad_normal_offset = quad_position_offset + len(pad_zero(quad_position_bytes))
    quad_index_offset = quad_normal_offset + len(pad_zero(quad_normal_bytes))

    minimum = [min(v[axis] for v in positions) for axis in range(3)]
    maximum = [max(v[axis] for v in positions) for axis in range(3)]
    quadMinimum = [min(v[axis] for v in quadPositions) for axis in range(3)]
    quadMaximum = [max(v[axis] for v in quadPositions) for axis in range(3)]

    materials = [
        glass("NoVolume", None),
        glass("ColourNoDistance", {"thicknessFactor": THICKNESS, "attenuationColor": ATTENUATION_COLOUR}),
        glass("ColourAndDistance", {
            "thicknessFactor": THICKNESS,
            "attenuationColor": ATTENUATION_COLOUR,
            "attenuationDistance": ATTENUATION_DISTANCE,
        }),
        {
            # ⚠️ Opaque, white, fully rough and with NO extension: the panel must contribute a
            # bright neutral field and nothing else. Any tint here would be read as absorption.
            "name": "Backdrop",
            "pbrMetallicRoughness": {
                "baseColorFactor": [1, 1, 1, 1],
                "metallicFactor": 0,
                "roughnessFactor": 1,
            },
        },
    ]

    document = {
        "asset": {"version": "2.0", "generator": "emeraude-engine tools/gltf-conformance-bench/make-volume-probe.py"},
        "extensionsUsed": ["KHR_materials_transmission", "KHR_materials_volume", "KHR_materials_ior"],
        "scene": 0,
        "scenes": [{"nodes": [0, 1, 2, 3]}],
        "nodes": [
            {"mesh": 0, "translation": [-1.3, 0, 0], "name": "NoVolume"},
            {"mesh": 1, "translation": [0.0, 0, 0], "name": "ColourNoDistance"},
            {"mesh": 2, "translation": [1.3, 0, 0], "name": "ColourAndDistance"},
            {"mesh": 3, "name": "Backdrop"},
        ],
        "meshes": [
            {"name": f"Ball{index}", "primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1}, "indices": 2, "material": index}]}
            for index in range(3)
        ] + [
            {"name": "Backdrop", "primitives": [{"attributes": {"POSITION": 3, "NORMAL": 4}, "indices": 5, "material": 3}]}
        ],
        "materials": materials,
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": len(positions), "type": "VEC3", "min": minimum, "max": maximum},
            {"bufferView": 1, "componentType": 5126, "count": len(normals), "type": "VEC3"},
            {"bufferView": 2, "componentType": 5125, "count": len(indices), "type": "SCALAR"},
            {"bufferView": 3, "componentType": 5126, "count": len(quadPositions), "type": "VEC3", "min": quadMinimum, "max": quadMaximum},
            {"bufferView": 4, "componentType": 5126, "count": len(quadNormals), "type": "VEC3"},
            {"bufferView": 5, "componentType": 5125, "count": len(quadIndices), "type": "SCALAR"},
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": len(position_bytes)},
            {"buffer": 0, "byteOffset": normal_offset, "byteLength": len(normal_bytes)},
            {"buffer": 0, "byteOffset": index_offset, "byteLength": len(index_bytes)},
            {"buffer": 0, "byteOffset": quad_position_offset, "byteLength": len(quad_position_bytes)},
            {"buffer": 0, "byteOffset": quad_normal_offset, "byteLength": len(quad_normal_bytes)},
            {"buffer": 0, "byteOffset": quad_index_offset, "byteLength": len(quad_index_bytes)},
        ],
        "buffers": [{"byteLength": len(binary)}],
    }

    encoded = json.dumps(document).encode("utf-8")
    # ⚠️ SPACES, not NULs — see the module docstring.
    encoded += b" " * ((4 - len(encoded) % 4) % 4)

    out = b"glTF" + struct.pack("<II", 2, 12 + 8 + len(encoded) + 8 + len(binary))
    out += struct.pack("<II", len(encoded), 0x4E4F534A) + encoded
    out += struct.pack("<II", len(binary), 0x004E4942) + binary

    return out


def main() -> None:
    payload = build()

    target = Path(__file__).resolve().parent / "assets" / "VolumeAbsorptionProbe" / "glTF-Binary" / "VolumeAbsorptionProbe.glb"
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(payload)

    # Read the JSON chunk back: the padding trap is silent otherwise.
    offset = 12
    while offset < len(payload):
        length, kind = struct.unpack_from("<II", payload, offset)
        offset += 8
        if kind == 0x4E4F534A:
            json.loads(payload[offset:offset + length].decode("utf-8"))
        offset += length

    print(f"{target} — {len(payload)} bytes, JSON chunk parses back")


if __name__ == "__main__":
    main()
