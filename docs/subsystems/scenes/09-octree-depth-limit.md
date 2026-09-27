## Octree Depth Limit

The OctreeSector class has a maximum subdivision depth (`DefaultMaxDepth = 16`) to prevent infinite recursion when many entities occupy the same position.

**Problem solved:**
When entities cluster at the same point (e.g., physics simulation causing all balls to converge), the octree would subdivide infinitely trying to separate them.

**Solution:**
- `OctreeSector::isStillLeaf()` checks `getDistance() < DefaultMaxDepth` before calling `expand()`
- At max depth, sector remains a leaf with all elements (O(n²) collision checks, but no infinite loop)

**Code references:**
- `OctreeSector.hpp:DefaultMaxDepth` - Constant (16 levels)
- `OctreeSector.hpp:isStillLeaf()` - Depth check before expansion
- `OctreeSector.hpp:getDistance()` - Calculates current depth from root

**Performance note:**
At depth 16 with a 200-unit root sector, minimum sector size ≈ 0.003 units. This is smaller than any realistic entity radius, so the depth limit rarely triggers in normal gameplay.
