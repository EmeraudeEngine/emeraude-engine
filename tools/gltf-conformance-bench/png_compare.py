"""
Dependency-free PNG reading and per-pixel comparison for the glTF conformance bench.

Exists for the compressed-variant A/B: a codec test is not "did it render something" but "does it
render the SAME thing as the uncompressed source". That question is answered in pixels, at an
identical framing, or it is not answered at all.

⚠️ No hard third-party dependency, on purpose — the bench runs on a bare checkout. numpy is used
when it happens to be importable, purely as a speed-up; the pure-Python path produces the same
numbers, only slower (a few seconds per 1920x1080 pair).
"""

import struct
import zlib

try:
    import numpy as _np
except ImportError:                                                     # pragma: no cover
    _np = None


def read_png_size(path) -> tuple:
    """
    Returns (width, height) from the IHDR alone, without decompressing the image.

    The bench needs the frame size to place a comparison box, and decoding two megapixels to learn
    it would double the cost of every row.
    """
    with open(path, "rb") as file:
        header = file.read(24)

    if header[:8] != b"\x89PNG\r\n\x1a\x0a" or header[12:16] != b"IHDR":
        raise ValueError(f"{path}: not a PNG")

    return struct.unpack(">II", header[16:24])


def read_png(path) -> tuple:
    """
    Decodes a non-interlaced 8-bit PNG into (width, height, channels, bytes).

    Handles the five PNG filter types and nothing else: the engine's screenshots are plain
    8-bit RGB/RGBA, and a silent wrong answer on an exotic variant would be worse than a refusal.
    """
    data = open(path, "rb").read()

    if data[:8] != b"\x89PNG\r\n\x1a\x0a":
        raise ValueError(f"{path}: not a PNG")

    offset = 8
    payload = b""
    width = height = depth = colour = None
    interlace = 0

    while offset < len(data):
        length = struct.unpack(">I", data[offset:offset + 4])[0]
        kind = data[offset + 4:offset + 8]
        chunk = data[offset + 8:offset + 8 + length]

        if kind == b"IHDR":
            width, height, depth, colour, _, _, interlace = struct.unpack(">IIBBBBB", chunk[:13])
        elif kind == b"IDAT":
            payload += chunk
        elif kind == b"IEND":
            break

        offset += 12 + length

    if depth != 8:
        raise ValueError(f"{path}: only 8-bit PNGs are handled (got {depth})")

    if interlace:
        raise ValueError(f"{path}: interlaced PNGs are not handled")

    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}.get(colour)

    if channels is None:
        raise ValueError(f"{path}: unsupported colour type {colour}")

    raw = zlib.decompress(payload)
    stride = width * channels
    out = bytearray(stride * height)
    previous = bytearray(stride)
    position = 0

    for row in range(height):
        method = raw[position]
        position += 1
        line = bytearray(raw[position:position + stride])
        position += stride

        if method == 1:
            for x in range(channels, stride):
                line[x] = (line[x] + line[x - channels]) & 0xFF
        elif method == 2:
            for x in range(stride):
                line[x] = (line[x] + previous[x]) & 0xFF
        elif method == 3:
            for x in range(stride):
                left = line[x - channels] if x >= channels else 0
                line[x] = (line[x] + ((left + previous[x]) >> 1)) & 0xFF
        elif method == 4:
            for x in range(stride):
                left = line[x - channels] if x >= channels else 0
                upleft = previous[x - channels] if x >= channels else 0
                estimate = left + previous[x] - upleft
                da, db, dc = abs(estimate - left), abs(estimate - previous[x]), abs(estimate - upleft)
                nearest = left if (da <= db and da <= dc) else (previous[x] if db <= dc else upleft)
                line[x] = (line[x] + nearest) & 0xFF
        elif method != 0:
            raise ValueError(f"{path}: unknown PNG filter {method} on row {row}")

        out[row * stride:(row + 1) * stride] = line
        previous = line

    return width, height, channels, bytes(out)


def compare(first, second, box=None) -> dict:
    """
    Per-pixel comparison of two captures, as the bench reports it.

    The delta of a pixel is the largest absolute difference over its colour channels (alpha is
    ignored: the captures are opaque and an alpha difference would not be visible). Returns the
    share of pixels that differ at all, the mean delta over every compared pixel, and the worst one.

    ⚠️ Read the three together. A lossy codec gives a tiny mean with a high maximum confined to
    silhouettes; a defect gives a mean that moves, or a maximum spread over a region. Neither
    number alone separates the two cases.

    @param box Optional (x0, y0, x1, y1) in pixels, x1/y1 exclusive: compare only that rectangle.
    The bench passes the subject's projected bounding box, because the BACKGROUND is not
    deterministic — the viewer's environment cubemap can still be streaming when a frame is drawn,
    and a capture that caught the default instead moved a whole row by tens of units while the model
    itself was pixel-identical. Cropping removes that failure mode at the source instead of
    re-running until it goes away.

    ⚠️⚠️ The percentages and the mean are then relative to the BOX, not to the frame, so they are
    NOT comparable with numbers produced without one: the background used to dilute the mean over
    two million mostly-identical pixels. Expect every figure to rise.
    """
    w1, h1, c1, a = read_png(first)
    w2, h2, c2, b = read_png(second)

    if (w1, h1) != (w2, h2):
        raise ValueError(f"size mismatch: {w1}x{h1} vs {w2}x{h2}")

    if c1 != c2:
        raise ValueError(f"channel mismatch: {c1} vs {c2}")

    colour_channels = min(c1, 3)

    if box is None:
        x0, y0, x1, y1 = 0, 0, w1, h1
    else:
        x0, y0, x1, y1 = (max(0, int(box[0])), max(0, int(box[1])),
                          min(w1, int(box[2])), min(h1, int(box[3])))

        if x1 <= x0 or y1 <= y0:
            raise ValueError(f"empty comparison box: {box} in a {w1}x{h1} image")

    pixels = (x1 - x0) * (y1 - y0)

    if _np is not None:
        shape = (h1, w1, c1)
        first_image = _np.frombuffer(a, dtype=_np.uint8).reshape(shape)[y0:y1, x0:x1, :colour_channels]
        second_image = _np.frombuffer(b, dtype=_np.uint8).reshape(shape)[y0:y1, x0:x1, :colour_channels]
        deltas = _np.abs(first_image.astype(_np.int16) - second_image.astype(_np.int16)).max(axis=2)

        differing = int((deltas > 0).sum())
        total = int(deltas.sum())
        worst = int(deltas.max())
    else:                                                               # pragma: no cover
        differing = total = worst = 0

        for y in range(y0, y1):
            row = y * w1 * c1

            for x in range(x0, x1):
                index = row + x * c1
                delta = max(abs(a[index + k] - b[index + k]) for k in range(colour_channels))

                if delta:
                    differing += 1

                total += delta
                worst = max(worst, delta)

    result = {
        "pixels": pixels,
        "differingPixels": differing,
        "differingPercent": 100.0 * differing / pixels,
        "meanAbsDelta": total / pixels,
        "maxAbsDelta": worst,
    }

    if box is not None:
        result["box"] = [x0, y0, x1, y1]

    return result
