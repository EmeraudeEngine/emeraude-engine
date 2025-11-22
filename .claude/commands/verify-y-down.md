---
description: Scanne fichier(s) pour détecter conversions Y suspectes ou valeurs incorrectes
---

Verify Y-down coordinate system convention is respected in source files.

**Task:**

If file specified: scan that file
If no file: scan Physics/, Graphics/, Audio/, Scenes/ directories

**Look for suspicious patterns:**

1. **Incorrect gravity values:**
   - ❌ `-9.81`, `-9.8`, `gravity = -`
   - ✅ Should be `+9.81`, `gravity = 9.81f`

2. **Y-axis flips or inversions:**
   - ❌ `y = -y`, `position.y *= -1`
   - ❌ `invertY()`, `flipVertical()`
   - ❌ Comments: "flip Y", "invert Y axis"

3. **Suspicious coordinate conversions:**
   - ❌ `toOpenGL()`, `fromVulkan()`
   - ❌ `yDown ? y : -y`
   - ❌ Conditional Y negation

4. **Hardcoded Y-up assumptions:**
   - ❌ Comments mentioning "Y-up"
   - ❌ `vec3(0, 1, 0)` for "up" direction (should be `vec3(0, -1, 0)`)
   - ❌ `vec3(0, -1, 0)` for "down" direction (should be `vec3(0, 1, 0)`)

5. **Jump/impulse directions:**
   - ❌ Positive Y for jump (should be negative)
   - In Physics/, check jump impulses use negative Y

**Output format:**
```
🔍 Y-Down Verification

File: [path] OR Directory: [path]

✅ No Y-down violations found

OR

⚠️ Potential violations found:

[Category]: [N] issues
  Line 123: gravity = -9.81f
    ^ Should be +9.81f (Y-down means down is +Y)

  Line 456: position.y = -position.y
    ^ Suspicious Y inversion

  Line 789: // Convert from Y-up to Y-down
    ^ Comment suggests Y-up/down conversion (should be unnecessary)

Recommendations:
- [specific fixes]
```

**Use Grep with patterns:**
- `gravity.*-[0-9]`
- `\.y\s*\*=\s*-1`
- `invert.*[Yy]|flip.*[Yy]`
- `[Yy]-up`

Be thorough but avoid false positives. Explain WHY each flagged pattern is suspicious in Y-down context.
