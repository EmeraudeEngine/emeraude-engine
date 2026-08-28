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

Expected reading, at the bench's own framing (measured 2026-08-28). The decisive figure is the
LAST column — the share of disc pixels where green exceeds red by more than 25/255 — because it
is binary rather than a ratio to argue about:

  ball                 body G/R   rim G/R   peak G-R   green pixels
  NoVolume                1.004     1.003        9/255        0.0 %
  ColourNoDistance        1.005     1.003       13/255        0.0 %
  ColourAndDistance       1.087     1.381      171/255       34.2 %

The absorbing ball's body stays far less green than its cap: Beer-Lambert over a short optical
path face-on and a long one at the grazing edge. A regression that half-works will show up as a
green share drifting away from 34 %, or — much worse — as the middle ball ceasing to be 0.0 %.

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
ATTENUATION_DISTANCE = 0.25
THICKNESS = 1.0


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
            indices += [a, b, a + 1, a + 1, b, b + 1]

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


def build() -> bytes:
    positions, normals, indices = uv_sphere()

    position_bytes = b"".join(struct.pack("<3f", *v) for v in positions)
    normal_bytes = b"".join(struct.pack("<3f", *n) for n in normals)
    index_bytes = b"".join(struct.pack("<I", i) for i in indices)

    binary = pad_zero(pad_zero(position_bytes) + pad_zero(normal_bytes) + pad_zero(index_bytes))
    normal_offset = len(pad_zero(position_bytes))
    index_offset = normal_offset + len(pad_zero(normal_bytes))

    minimum = [min(v[axis] for v in positions) for axis in range(3)]
    maximum = [max(v[axis] for v in positions) for axis in range(3)]

    materials = [
        glass("NoVolume", None),
        glass("ColourNoDistance", {"thicknessFactor": THICKNESS, "attenuationColor": ATTENUATION_COLOUR}),
        glass("ColourAndDistance", {
            "thicknessFactor": THICKNESS,
            "attenuationColor": ATTENUATION_COLOUR,
            "attenuationDistance": ATTENUATION_DISTANCE,
        }),
    ]

    document = {
        "asset": {"version": "2.0", "generator": "emeraude-engine tools/gltf-conformance-bench/make-volume-probe.py"},
        "extensionsUsed": ["KHR_materials_transmission", "KHR_materials_volume", "KHR_materials_ior"],
        "scene": 0,
        "scenes": [{"nodes": [0, 1, 2]}],
        "nodes": [
            {"mesh": 0, "translation": [-1.3, 0, 0], "name": "NoVolume"},
            {"mesh": 1, "translation": [0.0, 0, 0], "name": "ColourNoDistance"},
            {"mesh": 2, "translation": [1.3, 0, 0], "name": "ColourAndDistance"},
        ],
        "meshes": [
            {"name": f"Ball{index}", "primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1}, "indices": 2, "material": index}]}
            for index in range(3)
        ],
        "materials": materials,
        "accessors": [
            {"bufferView": 0, "componentType": 5126, "count": len(positions), "type": "VEC3", "min": minimum, "max": maximum},
            {"bufferView": 1, "componentType": 5126, "count": len(normals), "type": "VEC3"},
            {"bufferView": 2, "componentType": 5125, "count": len(indices), "type": "SCALAR"},
        ],
        "bufferViews": [
            {"buffer": 0, "byteOffset": 0, "byteLength": len(position_bytes)},
            {"buffer": 0, "byteOffset": normal_offset, "byteLength": len(normal_bytes)},
            {"buffer": 0, "byteOffset": index_offset, "byteLength": len(index_bytes)},
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
