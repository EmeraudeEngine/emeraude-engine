/*
 * src/Scenes/OctreeSector.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <array>
#include <vector>
#include <unordered_set>
#include <string>
#include <memory>
#include <algorithm>
#include <limits>
#include <type_traits>

/* Local inclusions for inheritances. */
#include "Libs/Math/Space3D/AACuboid.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Space3D/Collisions/SamePrimitive.hpp"
#include "Libs/Math/Space3D/Collisions/PointCuboid.hpp"
#include "Libs/Math/Space3D/Collisions/PointSphere.hpp"
#include "Libs/Math/Space3D/Collisions/SphereCuboid.hpp"
#include "Physics/CollisionModelInterface.hpp"
#include "LocatableInterface.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	/**
	 * @class OctreeSector
	 * @brief Template class for hierarchical octree spatial partitioning.
	 *
	 * This class implements a dynamic octree spatial partitioning structure used for both
	 * rendering (frustum culling) and physics (collision broad-phase detection). The octree
	 * subdivides 3D space into eight child sectors recursively, storing elements at all levels
	 * they intersect for efficient spatial queries.
	 *
	 * The template supports two modes via the enable_volume parameter:
	 * - Point-based (enable_volume=false): Elements inserted based on their position point only.
	 *   Used for rendering octrees where each element occupies a single leaf sector.
	 * - Volume-based (enable_volume=true): Elements inserted based on their bounding volume
	 *   (AABB or Sphere). Used for physics octrees where elements can span multiple sectors.
	 *
	 * Key features:
	 * - Zero-overhead callbacks: Template methods (forTouchedSector, forSurroundingSectors)
	 *   accept callable types directly, avoiding std::function allocation overhead.
	 * - Direct slot calculation: computeSlotForPosition() enables O(1) octree traversal
	 *   by computing the child sector index from position via bit manipulation.
	 * - Combined operations: updateOrInsert() performs contains + update/insert in a single
	 *   traversal, eliminating redundant lookups.
	 * - Dynamic expansion: Sectors automatically subdivide when element count exceeds
	 *   maxElementPerSector threshold.
	 * - Optional auto-collapse: Empty leaf sectors can be automatically removed when enabled.
	 *
	 * @tparam element_t The type of elements stored in the octree. Must inherit from both
	 *				   EmEn::Scenes::LocatableInterface (provides position/volume) and
	 *				   EmEn::Libs::NameableTrait (provides name for debugging).
	 * @tparam enable_volume When true, uses element's bounding volume (AABB/Sphere) for
	 *					   insertion, allowing elements to span multiple sectors (physics mode).
	 *					   When false, uses only the element's position point (rendering mode).
	 *
	 * @note DESIGN CONSIDERATION - Storage Strategy (All-Levels vs Leaf-Only):
	 *	   Current implementation stores elements at ALL levels they touch (root to leaves).
	 *	   Alternative: Store elements only in leaf sectors.
	 *
	 *	   Current (All-Levels) - CPU optimized:
	 *	   + Fast early-exit: m_elements.empty() skips empty branches without traversal
	 *	   + Local decisions: isStillLeaf() decides expand/collapse without checking children
	 *	   + O(1) contains() at any level
	 *	   - Higher memory: elements duplicated in parent sectors (shared_ptr overhead only)
	 *
	 *	   Alternative (Leaf-Only) - Memory optimized:
	 *	   + Lower memory footprint
	 *	   + Simpler erase() logic
	 *	   - Must traverse to leaves to check emptiness
	 *	   - Breaks current expand/collapse logic based on m_elements.size()
	 *	   - No early-exit optimization
	 *
	 *	   Assessment: Current all-levels strategy prioritizes CPU performance over memory.
	 *	   The memory overhead is acceptable (shared_ptr = pointer + refcount, not entity copy).
	 *	   A compile-time template parameter could be added to switch strategies if needed.
	 *
	 * @note OPTIMIZATION CONSIDERATION - Cached Sector Reference in Entity:
	 *	   Potential optimization: store a pointer to the last known sector in the entity itself.
	 *	   This would allow fast-path updates when an entity hasn't moved out of its sector.
	 *
	 *	   For enable_volume=false (rendering octree, point-based):
	 *	   + Entity is in exactly ONE leaf sector at a time
	 *	   + Could store OctreeSector* in entity, check if still inside before full traversal
	 *	   + Already partially implemented: update() uses getDeepestSubSector() for this
	 *
	 *	   For enable_volume=true (physics octree, volume-based):
	 *	   - Entity can span MULTIPLE sectors simultaneously (AABB/Sphere overlaps)
	 *	   - Cannot store single sector reference
	 *	   - Would need std::vector or bitset of touched sectors
	 *	   - Complexity may outweigh benefits
	 *
	 *	   Implementation approaches:
	 *	   1. Add void* m_cachedSector to LocatableInterface (simple, not type-safe)
	 *	   2. Add std::unordered_map<element_t*, OctreeSector*> cache at root level
	 *	   3. Template specialization for enable_volume=false only
	 *
	 *	   Assessment: Deferred. Current update() already has fast-path for point-based case.
	 *	   Volume-based case complexity makes this optimization questionable for physics octree.
	 *
	 * @see EmEn::Scenes::LocatableInterface
	 * @see EmEn::Libs::NameableTrait
	 * @see EmEn::Libs::Math::Space3D::AACuboid
	 * @version 0.8.38
	 */
	template< typename element_t, bool enable_volume >
	requires (std::is_base_of_v< Libs::NameableTrait, element_t >, std::is_base_of_v< LocatableInterface, element_t >)
	class OctreeSector final : public std::enable_shared_from_this< OctreeSector< element_t, enable_volume > >, public Libs::Math::Space3D::AACuboid< float >
	{
		public:

			/** @brief Class identifier for tracing and debugging. */
			static constexpr auto ClassId{"OctreeSector"};

			/** @brief Number of child sectors in an octree node (always 8). */
			static constexpr auto SectorDivision{8UL};

			/** @brief Default maximum number of elements per sector before subdivision. */
			static constexpr auto DefaultSectorElementLimit{8UL};

			/** @brief Default maximum depth of octree subdivision to prevent infinite recursion. */
			static constexpr auto DefaultMaxDepth{16UL};

			/** @brief Slot index for subsector with positive X, positive Y, positive Z (slot 0). */
			static constexpr auto XPositiveYPositiveZPositive{0UL};

			/** @brief Slot index for subsector with positive X, positive Y, negative Z (slot 1). */
			static constexpr auto XPositiveYPositiveZNegative{1UL};

			/** @brief Slot index for subsector with positive X, negative Y, positive Z (slot 2). */
			static constexpr auto XPositiveYNegativeZPositive{2UL};

			/** @brief Slot index for subsector with positive X, negative Y, negative Z (slot 3). */
			static constexpr auto XPositiveYNegativeZNegative{3UL};

			/** @brief Slot index for subsector with negative X, positive Y, positive Z (slot 4). */
			static constexpr auto XNegativeYPositiveZPositive{4UL};

			/** @brief Slot index for subsector with negative X, positive Y, negative Z (slot 5). */
			static constexpr auto XNegativeYPositiveZNegative{5UL};

			/** @brief Slot index for subsector with negative X, negative Y, positive Z (slot 6). */
			static constexpr auto XNegativeYNegativeZPositive{6UL};

			/** @brief Slot index for subsector with negative X, negative Y, negative Z (slot 7). */
			static constexpr auto XNegativeYNegativeZNegative{7UL};

			/**
			 * @brief Constructs a root octree sector.
			 *
			 * Creates the top-level sector of the octree hierarchy. The root sector is initially
			 * a leaf (no subdivisions) and will automatically expand when the element count
			 * exceeds maxElementPerSector.
			 *
			 * @param maximum The maximum corner (highest X, Y, Z coordinates) of the root sector bounds.
			 * @param minimum The minimum corner (lowest X, Y, Z coordinates) of the root sector bounds.
			 * @param maxElementPerSector The threshold number of elements that triggers automatic
			 *							subdivision. Must be at least DefaultSectorElementLimit (8).
			 *							Values below this minimum are clamped upward. Default is 8.
			 * @param enableAutoCollapse When true, empty leaf sectors are automatically removed during
			 *						   erase operations to reduce memory usage. When false, sectors
			 *						   remain allocated once created. Default is false.
			 *
			 * @note The root sector initially has no parent (isRoot() returns true).
			 * @note Auto-collapse is incompatible with reserve() - pre-allocated sectors would be
			 *	   immediately removed if they're empty.
			 */
			OctreeSector (const Libs::Math::Vector< 3, float > & maximum, const Libs::Math::Vector< 3, float > & minimum, size_t maxElementPerSector = DefaultSectorElementLimit, bool enableAutoCollapse = false) noexcept
				: AACuboid{maximum, minimum},
				m_maxElementPerSector{std::max< size_t >(DefaultSectorElementLimit, maxElementPerSector)},
				m_autoCollapseEnabled{enableAutoCollapse}
			{

			}

			/**
			 * @brief Constructs a child octree sector.
			 *
			 * Creates a subsector within a parent sector during subdivision. Child sectors inherit
			 * their parent's maxElementPerSector and autoCollapseEnabled settings.
			 *
			 * @param maximum The maximum corner (highest X, Y, Z coordinates) of the child sector bounds.
			 * @param minimum The minimum corner (lowest X, Y, Z coordinates) of the child sector bounds.
			 * @param parentSector Shared pointer to the parent sector that owns this child.
			 *					 Used to traverse up the hierarchy and inherit configuration.
			 * @param slot The slot index (0-7) indicating this child's position within the parent's
			 *			 eight-way subdivision. See XPositiveYPositiveZPositive and related constants.
			 *
			 * @note This constructor is typically called internally by expand().
			 * @see expand()
			 */
			OctreeSector (const Libs::Math::Vector< 3, float > & maximum, const Libs::Math::Vector< 3, float > & minimum, const std::shared_ptr< OctreeSector > & parentSector, size_t slot) noexcept
				: AACuboid{maximum, minimum},
				m_parentSector{parentSector},
				m_slot{slot},
				m_maxElementPerSector{parentSector->m_maxElementPerSector},
				m_autoCollapseEnabled{parentSector->m_autoCollapseEnabled}
			{

			}

			/**
			 * @brief Deleted copy constructor.
			 *
			 * OctreeSector cannot be copied as it maintains hierarchical parent-child relationships
			 * via shared_ptr and weak_ptr that cannot be safely duplicated.
			 */
			OctreeSector (const OctreeSector & copy) noexcept = delete;

			/**
			 * @brief Deleted move constructor.
			 *
			 * OctreeSector cannot be moved as it maintains hierarchical parent-child relationships
			 * via shared_ptr and weak_ptr that would be invalidated by moving.
			 */
			OctreeSector (OctreeSector && copy) noexcept = delete;

			/**
			 * @brief Deleted copy assignment operator.
			 *
			 * OctreeSector cannot be copy-assigned due to its complex hierarchical structure and
			 * shared ownership semantics.
			 */
			OctreeSector & operator= (const OctreeSector & copy) noexcept = delete;

			/**
			 * @brief Deleted move assignment operator.
			 *
			 * OctreeSector cannot be move-assigned due to its complex hierarchical structure and
			 * shared ownership semantics.
			 */
			OctreeSector & operator= (OctreeSector && copy) noexcept = delete;

			/**
			 * @brief Destructs the octree sector.
			 *
			 * Automatically cleans up child sectors and releases all stored elements.
			 * If this sector has children, they will be recursively destroyed.
			 */
			~OctreeSector () = default;

			/**
			 * @brief Checks whether this sector is the root of the octree.
			 *
			 * The root sector has no parent and represents the entire spatial volume.
			 *
			 * @return True if this is the root sector (no parent), false if it's a child sector.
			 *
			 * @note Root sectors have m_slot set to std::numeric_limits<size_t>::max().
			 */
			[[nodiscard]]
			bool
			isRoot () const noexcept
			{
				return m_parentSector.expired();
			}

			/**
			 * @brief Checks whether this sector is a leaf node (has no children).
			 *
			 * Leaf sectors are the endpoints of the octree hierarchy and directly contain
			 * elements without further subdivision.
			 *
			 * @return True if this sector has no child sectors, false if it has been subdivided.
			 *
			 * @see isExpanded()
			 */
			[[nodiscard]]
			bool
			isLeaf () const noexcept
			{
				return !m_isExpanded;
			}

			/**
			 * @brief Checks whether this sector has been subdivided into child sectors.
			 *
			 * Expanded sectors have eight child sectors and were subdivided due to exceeding
			 * the maxElementPerSector threshold.
			 *
			 * @return True if this sector has child sectors, false if it's still a leaf.
			 *
			 * @see isLeaf()
			 */
			[[nodiscard]]
			bool
			isExpanded () const noexcept
			{
				return m_isExpanded;
			}

			/**
			 * @brief Checks whether this sector contains no elements.
			 *
			 * An empty sector has no elements registered at this level. Due to the all-levels
			 * storage strategy, if a sector is empty, all its descendants are guaranteed to be
			 * empty as well (enabling fast early-exit in traversal algorithms).
			 *
			 * @return True if no elements are stored in this sector, false otherwise.
			 *
			 * @warning This only checks the local element set, not whether child sectors exist.
			 *		  Use isLeaf() to check for the absence of child sectors.
			 *
			 * @note This is used for optimization - empty() allows forTouchedSector() and other
			 *	   traversal methods to skip entire branches without recursing into children.
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_elements.empty();
			}

			/**
			 * @brief Returns this sector's slot index within its parent.
			 *
			 * The slot index (0-7) indicates which of the eight child positions this sector
			 * occupies within its parent's subdivision. Slot indices follow the bit pattern:
			 * bit 2 (value 4) = X axis, bit 1 (value 2) = Y axis, bit 0 (value 1) = Z axis,
			 * where 0 = positive direction, 1 = negative direction.
			 *
			 * @return The slot index (0-7) for child sectors, or std::numeric_limits<size_t>::max()
			 *		 for the root sector.
			 *
			 * @warning Check isRoot() before using this value. Root sectors return the maximum
			 *		  size_t value, which is not a valid slot index.
			 *
			 * @see XPositiveYPositiveZPositive and related slot constants
			 * @see computeSlotForPosition()
			 */
			[[nodiscard]]
			size_t
			slot () const noexcept
			{
				return m_slot;
			}

			/**
			 * @brief Returns the element count threshold that triggers sector subdivision.
			 *
			 * When a leaf sector's element count exceeds this threshold, the sector automatically
			 * subdivides into eight child sectors.
			 *
			 * @return The maximum number of elements allowed in a sector before expansion.
			 *
			 * @note This value is set during construction and inherited by all child sectors.
			 * @note The minimum value is DefaultSectorElementLimit (8).
			 */
			[[nodiscard]]
			size_t
			maxElementPerSector () const noexcept
			{
				return m_maxElementPerSector;
			}

			/**
			 * @brief Checks whether automatic empty sector removal is enabled.
			 *
			 * When enabled, empty leaf sectors are automatically collapsed (removed) during
			 * erase operations to reduce memory usage. When disabled, sectors remain allocated
			 * once created.
			 *
			 * @return True if auto-collapse is enabled, false otherwise.
			 *
			 * @note Auto-collapse is incompatible with reserve() - pre-allocated empty sectors
			 *	   would be immediately removed.
			 * @note This setting is inherited from the root sector by all child sectors.
			 */
			[[nodiscard]]
			bool
			autoCollapseEnabled () const noexcept
			{
				return m_autoCollapseEnabled;
			}

			/**
			 * @brief Calculates the distance (level) of this sector from the root.
			 *
			 * The distance represents how many levels down this sector is in the octree hierarchy.
			 * This is useful for debugging and understanding the octree structure.
			 *
			 * @return The number of levels from the root to this sector. Returns 0 for the root
			 *		 sector, 1 for direct children of root, 2 for grandchildren, etc.
			 *
			 * @note This method traverses up the parent chain, so it has O(depth) complexity.
			 */
			[[nodiscard]]
			size_t
			getDistance () const noexcept
			{
				if ( this->isRoot() )
				{
					return 0;
				}

				size_t depth = 1;

				auto parentSector = m_parentSector.lock();

				while ( !parentSector->isRoot() )
				{
					depth++;

					parentSector = parentSector->m_parentSector.lock();
				}

				return depth;
			}

			/**
			 * @brief Calculates the maximum depth of the subtree below this sector.
			 *
			 * The depth represents the longest path from this sector to any leaf sector below it.
			 * When called on the root sector, this returns the total depth of the entire octree.
			 *
			 * @return The maximum number of levels below this sector. Returns 0 for leaf sectors,
			 *		 1 if only direct children exist, etc.
			 *
			 * @note This method recursively traverses all child sectors, so it has O(n) complexity
			 *	   where n is the number of sectors in the subtree.
			 * @note When called on root, this gives the maximum subdivision depth of the octree.
			 */
			[[nodiscard]]
			size_t
			getDepth () const noexcept
			{
				if ( this->isLeaf() )
				{
					return 0;
				}

				size_t belowDepth = 0;

				for ( const auto & subSector : m_subSectors )
				{
					const auto sectorDepth = subSector->getDepth();

					if ( sectorDepth > belowDepth )
					{
						belowDepth = sectorDepth;
					}
				}

				return 1 + belowDepth;
			}

			/**
			 * @brief Counts the total number of sectors in this subtree.
			 *
			 * Recursively counts this sector plus all its descendant sectors. Useful for
			 * analyzing memory usage and octree structure.
			 *
			 * @return The total number of sectors including this one and all descendants.
			 *		 Returns 1 for leaf sectors (just themselves).
			 *
			 * @note This method recursively traverses all child sectors, so it has O(n) complexity
			 *	   where n is the number of sectors in the subtree.
			 * @note When called on root, this gives the total sector count for the entire octree.
			 */
			[[nodiscard]]
			size_t
			getSectorCount () const noexcept
			{
				size_t count = 1;

				if ( !this->isLeaf() )
				{
					for ( const auto & subSector : m_subSectors )
					{
						count += subSector->getSectorCount();
					}
				}

				return count;
			}

			/**
			 * @brief Returns a weak pointer to the parent sector.
			 *
			 * The parent sector is the sector one level up in the octree hierarchy that
			 * contains this sector as one of its eight children.
			 *
			 * @return A weak_ptr to the parent sector. The weak_ptr will be expired (invalid)
			 *		 if this is the root sector.
			 *
			 * @warning Always check isRoot() before using the returned weak_ptr, or check if
			 *		  the weak_ptr is expired before locking it.
			 *
			 * @note A weak_ptr is used to avoid circular references between parent and child sectors.
			 *
			 * @see isRoot()
			 */
			[[nodiscard]]
			std::weak_ptr< OctreeSector >
			parentSector () noexcept
			{
				return m_parentSector;
			}

			/**
			 * @brief Returns a weak pointer to the parent sector (const version).
			 *
			 * The parent sector is the sector one level up in the octree hierarchy that
			 * contains this sector as one of its eight children.
			 *
			 * @return A weak_ptr to the const parent sector. The weak_ptr will be expired (invalid)
			 *		 if this is the root sector.
			 *
			 * @warning Always check isRoot() before using the returned weak_ptr, or check if
			 *		  the weak_ptr is expired before locking it.
			 *
			 * @note A weak_ptr is used to avoid circular references between parent and child sectors.
			 *
			 * @see isRoot()
			 */
			[[nodiscard]]
			std::weak_ptr< const OctreeSector >
			parentSector () const noexcept
			{
				return m_parentSector;
			}

			/**
			 * @brief Traverses up the hierarchy to find and return the root sector.
			 *
			 * Walks up the parent chain until reaching the root sector (the sector with no parent).
			 * Useful for accessing octree-wide configuration or performing operations from the root.
			 *
			 * @return A shared_ptr to the root sector of this octree.
			 *
			 * @note If called on the root sector itself, returns this sector.
			 * @note This method has O(depth) complexity as it traverses the parent chain.
			 */
			[[nodiscard]]
			std::shared_ptr< OctreeSector >
			getRootSector () noexcept
			{
				auto currentSector = this->shared_from_this();

				while ( !currentSector->isRoot() )
				{
					currentSector = currentSector->parentSector();
				}

				return currentSector;
			}

			/**
			 * @brief Traverses up the hierarchy to find and return the root sector (const version).
			 *
			 * Walks up the parent chain until reaching the root sector (the sector with no parent).
			 * Useful for accessing octree-wide configuration or performing operations from the root.
			 *
			 * @return A shared_ptr to the const root sector of this octree.
			 *
			 * @note If called on the root sector itself, returns this sector.
			 * @note This method has O(depth) complexity as it traverses the parent chain.
			 */
			[[nodiscard]]
			std::shared_ptr< const OctreeSector >
			getRootSector () const noexcept
			{
				auto currentSector = this->shared_from_this();

				while ( !currentSector->isRoot() )
				{
					currentSector = currentSector->parentSector();
				}

				return currentSector;
			}

			/**
			 * @brief Returns the array of eight child sectors.
			 *
			 * Provides direct access to the subsector array for manual traversal. Each element
			 * in the array corresponds to one of the eight octants defined by the slot constants.
			 *
			 * @return Reference to the array of eight child sector pointers.
			 *
			 * @warning The array may contain null pointers if this sector is a leaf (not expanded).
			 *		  Always check isExpanded() before accessing child sectors.
			 * @warning Do not modify the array contents directly unless you understand the octree
			 *		  invariants. Use insert(), erase(), expand(), and collapse() instead.
			 *
			 * @see isExpanded()
			 * @see isLeaf()
			 */
			[[nodiscard]]
			std::array< std::shared_ptr< OctreeSector >, SectorDivision > &
			subSectors () noexcept
			{
				return m_subSectors;
			}

			/**
			 * @brief Returns the array of eight child sectors (const version).
			 *
			 * Provides direct read-only access to the subsector array for manual traversal.
			 * Each element in the array corresponds to one of the eight octants defined by
			 * the slot constants.
			 *
			 * @return Const reference to the array of eight child sector pointers.
			 *
			 * @warning The array may contain null pointers if this sector is a leaf (not expanded).
			 *		  Always check isExpanded() before accessing child sectors.
			 *
			 * @see isExpanded()
			 * @see isLeaf()
			 */
			[[nodiscard]]
			const std::array< std::shared_ptr< OctreeSector >, SectorDivision > &
			subSectors () const noexcept
			{
				return m_subSectors;
			}

			/**
			 * @brief Pre-allocates octree sectors to a specified depth.
			 *
			 * Recursively subdivides this sector and all descendants to create a fixed-depth
			 * octree structure. This is useful for avoiding dynamic allocations during runtime
			 * when the spatial extent is known in advance.
			 *
			 * @param depth The number of levels to pre-allocate. 0 means no allocation,
			 *			  1 means allocate immediate children, 2 means children and grandchildren, etc.
			 *
			 * @note This method has no effect if auto-collapse is enabled, as empty pre-allocated
			 *	   sectors would be immediately removed. A warning is logged if attempted.
			 * @note Pre-allocation is useful for performance-critical scenarios where allocation
			 *	   overhead must be avoided during gameplay.
			 *
			 * @warning Incompatible with autoCollapseEnabled. Check autoCollapseEnabled() before calling.
			 *
			 * @see autoCollapseEnabled()
			 */
			void
			reserve (size_t depth) noexcept
			{
				if ( m_autoCollapseEnabled )
				{
					Tracer::warning(ClassId, "Automatic empty subsectors removal is enabled !");

					return;
				}

				if ( depth != 0 )
				{
					this->expand();

					if ( depth > 1 )
					{
						for ( auto & subSector : m_subSectors )
						{
							subSector->reserve(depth - 1);
						}
					}
				}
			}

			/**
			 * @brief Checks whether an element is present in this sector.
			 *
			 * Tests if the element is registered in this specific sector's element set.
			 * Due to the all-levels storage strategy, an element present in a child sector
			 * will also be present in all its parent sectors.
			 *
			 * @param element Shared pointer to the element to search for.
			 *
			 * @return True if the element is in this sector's element set, false otherwise.
			 *
			 * @note This is an O(1) operation using unordered_set lookup.
			 * @note This only checks the local sector, not descendants. To check if an element
			 *	   is anywhere in the octree, call contains() on the root sector.
			 */
			[[nodiscard]]
			bool
			contains (const std::shared_ptr< element_t > & element) const noexcept
			{
				return m_elements.contains(element);
			}

			/**
			 * @brief Tests collision between this sector and a geometric primitive.
			 *
			 * Determines whether the given primitive (point, sphere, AABB, frustum, etc.)
			 * intersects with this sector's bounding volume. This is the fundamental spatial
			 * query used by insert, update, and traversal methods.
			 *
			 * @tparam primitive_t The type of primitive to test (automatically deduced).
			 *					 Supported types: Vector<3,float> (point), Sphere, AACuboid, Frustum.
			 * @param primitive The primitive to test for collision.
			 *
			 * @return True if the primitive intersects this sector's bounds, false otherwise.
			 *
			 * @note This method delegates to the collision detection functions in
			 *	   EmEn::Libs::Math::Space3D::isColliding().
			 *
			 * @see EmEn::Libs::Math::Space3D::isColliding()
			 */
			template< typename primitive_t >
			[[nodiscard]]
			bool
			isCollidingWith (const primitive_t & primitive) const noexcept
			{
				return Libs::Math::Space3D::isColliding(*this, primitive);
			}

			/**
			 * @brief Inserts an element into the octree at this sector level and all descendant sectors it touches.
			 *
			 * This is the primary insertion method that dispatches to the appropriate collision primitive
			 * based on the template parameter enable_volume:
			 * - When enable_volume=false (rendering octree): Uses only the element's position point.
			 * - When enable_volume=true (physics octree): Uses the element's collision model AABB
			 *   or position for Point types (as determined by element->collisionModel()).
			 *
			 * The element is inserted at this level if it collides with this sector, then recursively
			 * inserted into all child sectors it touches (all-levels storage strategy).
			 *
			 * @param element Shared pointer to the element to insert. Must provide position and
			 *				(if enable_volume=true) collision detection model and bounding volume.
			 *
			 * @return True if the element was successfully inserted (collides with this sector),
			 *		 false if the element is outside this sector's bounds or already present.
			 *
			 * @note Automatically triggers subdivision if element count exceeds maxElementPerSector.
			 * @note For volume-based insertion, elements can span multiple sectors simultaneously.
			 * @note Duplicate insertions of the same element are ignored (idempotent operation).
			 *
			 * @see insertWithPrimitive()
			 * @see update()
			 * @see updateOrInsert()
			 */
			bool
			insert (const std::shared_ptr< element_t > & element) noexcept
			{
				if constexpr ( enable_volume )
				{
					/* If no collision model, use position as a point. */
					if ( !element->hasCollisionModel() )
					{
						return this->insertWithPrimitive(element, element->getWorldCoordinates().position());
					}

					const auto * model = element->collisionModel();

					/* Point models use position directly. */
					if ( model->modelType() == Physics::CollisionModelType::Point )
					{
						return this->insertWithPrimitive(element, element->getWorldCoordinates().position());
					}

					/* All other models (Sphere, AABB, Capsule) use their world AABB. */
					return this->insertWithPrimitive(element, model->getAABB(element->getWorldCoordinates()));
				}
				else
				{
					return this->insertWithPrimitive(element, element->getWorldCoordinates().position());
				}
			}

			/**
			 * @brief Optimized combined operation: updates element position if present, otherwise inserts it.
			 *
			 * This is a performance optimization that combines the functionality of contains() +
			 * update() or insert() into a single operation, avoiding redundant octree traversal.
			 * Use this when you're unsure whether an element is already in the octree.
			 *
			 * Algorithm:
			 * 1. Fast-path check: If element is in root's element set, call update()
			 * 2. Otherwise: Call insert() to add it to the octree
			 *
			 * @param element Shared pointer to the element to update or insert.
			 *
			 * @return True if the element is now present in the octree, false if the operation failed
			 *		 (e.g., element is outside the octree bounds).
			 *
			 * @pre This method must be called on the root sector only. Debug builds will assert this.
			 *
			 * @note This is more efficient than calling contains() + update()/insert() separately.
			 * @note Typical usage: Call this every frame for moving elements where you're unsure
			 *	   if they've been added yet.
			 *
			 * @see insert()
			 * @see update()
			 */
			bool
			updateOrInsert (const std::shared_ptr< element_t > & element) noexcept
			{
				if constexpr ( IsDebug )
				{
					if ( !this->isRoot() )
					{
						TraceError{ClassId} << "You can't call updateOrInsert() on a subsector !";

						return false;
					}
				}

				/* Fast path: element already present, just update it. */
				if ( m_elements.contains(element) )
				{
					return this->update(element);
				}

				/* Element not present, insert it. */
				return this->insert(element);
			}

			/**
			 * @brief Updates an element's position within the octree after it has moved.
			 *
			 * This method re-evaluates the element's position/volume and adjusts its placement
			 * in the octree hierarchy. If the element has moved into different sectors, it will
			 * be removed from old sectors and added to new ones.
			 *
			 * Algorithm behavior differs by template mode:
			 * - enable_volume=false (rendering): Fast-path optimization checks if element is still
			 *   in its last known leaf sector before full traversal.
			 * - enable_volume=true (physics): Full re-evaluation since elements can span multiple sectors.
			 *
			 * @param element Shared pointer to the element to update. Must already be in the octree.
			 *
			 * @return True if the element is still within the octree bounds, false if it moved
			 *		 completely outside the root sector (element will be removed).
			 *
			 * @pre This method must be called on the root sector only. Debug builds will assert this.
			 * @pre The element must already be present in the octree (use insert() for new elements).
			 *
			 * @note If the root sector is not expanded (still a leaf), this is a no-op that returns true.
			 * @note For point-based octrees, includes fast-path: checks last leaf sector first.
			 *
			 * @todo Verify the specific fast-path check for point-based mode (enable_volume=false).
			 *
			 * @see insert()
			 * @see updateOrInsert()
			 * @see erase()
			 */
			bool
			update (const std::shared_ptr< element_t > & element) noexcept
			{
				if constexpr ( IsDebug )
				{
					if ( !this->isRoot() )
					{
						TraceError{ClassId} << "You can't call update() on a subsector !";

						return false;
					}
				}

				/* NOTE: If the root sector is not split down, there is no need to check. */
				if ( !m_isExpanded )
				{
					return true;
				}

				/* TODO: Verify the specific check when not using volume. */
				if constexpr ( enable_volume )
				{
					/* If no collision model, use position as a point. */
					if ( !element->hasCollisionModel() )
					{
						return this->checkElementOverlapWithPrimitive(element, element->getWorldCoordinates().position());
					}

					const auto * model = element->collisionModel();

					/* Point models use position directly. */
					if ( model->modelType() == Physics::CollisionModelType::Point )
					{
						return this->checkElementOverlapWithPrimitive(element, element->getWorldCoordinates().position());
					}

					/* All other models (Sphere, AABB, Capsule) use their world AABB. */
					return this->checkElementOverlapWithPrimitive(element, model->getAABB(element->getWorldCoordinates()));
				}
				else
				{
					const auto position = element->getWorldCoordinates().position();

					/* NOTE: Does the element moved out the last registered subsector boundaries? */
					const auto * lastSubSector = this->getDeepestSubSector(element);

					if ( Libs::Math::Space3D::isColliding(*lastSubSector, position) )
					{
						return true;
					}

					return this->checkElementOverlapWithPrimitive(element, position);
				}
			}

			/**
			 * @brief Removes an element from the octree at this level and all descendant sectors.
			 *
			 * Recursively removes the element from this sector and all child sectors. Due to the
			 * all-levels storage strategy, the element must be removed from every sector it was
			 * inserted into.
			 *
			 * If auto-collapse is enabled, this operation may trigger sector collapse if removing
			 * the element reduces the sector's element count below the collapse threshold.
			 *
			 * @param element Shared pointer to the element to remove.
			 *
			 * @return True if the element was found and removed, false if it wasn't present in
			 *		 this sector.
			 *
			 * @note When called on root, logs a warning if the element isn't in the octree.
			 * @note This method automatically triggers collapse if autoCollapseEnabled and element
			 *	   count drops below maxElementPerSector / 2.
			 * @note Safe to call even if element is not present (idempotent operation).
			 *
			 * @see insert()
			 * @see autoCollapseEnabled()
			 */
			bool
			erase (const std::shared_ptr< element_t > & element) noexcept
			{
				/* The node is not present in this sector. */
				if ( !m_elements.contains(element) )
				{
					if ( this->isRoot() )
					{
						TraceWarning{ClassId} << "Element '" << element->name() << "' is not part of the octree !";
					}

					return false;
				}

				/* Removes the node from the set. */
				m_elements.erase(element);

				/* If this sector is a leaf, we are done. */
				if ( !this->isStillLeaf() )
				{
					for ( const auto & subSector : m_subSectors )
					{
						subSector->erase(element);
					}
				}

				return true;
			}

			/**
			 * @brief Returns the number of elements stored in this sector.
			 *
			 * Counts only elements at this specific sector level, not including descendants.
			 * Due to the all-levels storage strategy, elements present in child sectors are
			 * also counted in their parent sectors.
			 *
			 * @return The number of elements in this sector's element set.
			 *
			 * @note This is an O(1) operation.
			 * @note To count total unique elements in the entire octree, call this on leaf
			 *	   sectors only and aggregate the results.
			 */
			[[nodiscard]]
			size_t
			elementCount () const noexcept
			{
				return m_elements.size();
			}

			/**
			 * @brief Returns read-only access to the element set for this sector.
			 *
			 * Provides direct access to the unordered_set containing all elements registered
			 * at this sector level. Useful for iteration and queries.
			 *
			 * @return Const reference to the unordered_set of element shared pointers.
			 *
			 * @note Elements in this set are also present in all child sectors they intersect.
			 * @note Do not modify the returned set. Use insert() and erase() to modify octree contents.
			 *
			 * @see elementCount()
			 */
			[[nodiscard]]
			const std::unordered_set< std::shared_ptr< element_t > > &
			elements () const noexcept
			{
				return m_elements;
			}

			/**
			 * @brief Searches for the first element with a specific name in this sector.
			 *
			 * Performs a linear search through this sector's element set to find an element
			 * matching the given name. Only searches the local sector, not descendants.
			 *
			 * @param name The name to search for (compared via element->name()).
			 *
			 * @return Shared pointer to the first matching element, or nullptr if no element
			 *		 with that name exists in this sector.
			 *
			 * @note This searches only the current sector. To search the entire octree, call
			 *	   this on the root sector (which contains all elements due to all-levels storage).
			 * @note Has O(n) complexity where n is the number of elements in this sector.
			 * @note If multiple elements share the same name, only the first one found is returned
			 *	   (iteration order is unspecified due to unordered_set).
			 */
			[[nodiscard]]
			std::shared_ptr< element_t >
			getFirstElementNamed (const std::string & name) const noexcept
			{
				for ( const auto & element : m_elements )
				{
					if ( element->name() == name )
					{
						return element;
					}
				}

				return nullptr;
			}

			/**
			 * @brief Executes a callback function on surrounding leaf sectors (Moore neighborhood).
			 *
			 * Invokes the provided callable on up to 26 neighboring leaf sectors surrounding this
			 * sector, plus optionally this sector itself. The Moore neighborhood includes all
			 * sectors that share a face, edge, or corner with this sector.
			 *
			 * This is a zero-overhead callback mechanism - the function parameter is a template
			 * type that avoids std::function allocation overhead. Perfect for hot-path collision
			 * detection and spatial queries.
			 *
			 * @tparam function_t The callable type (lambda, function pointer, functor). Automatically
			 *					deduced. Must be invocable with signature: void(const OctreeSector&).
			 * @param includeThisSector If true, the callback is first invoked on this sector before
			 *						  checking neighbors.
			 * @param function The callable to execute on each neighbor. Receives a const reference
			 *				 to each neighboring sector.
			 *
			 * @note Neighbors that don't exist (outside octree bounds) or aren't subdivided to the
			 *	   same level are automatically skipped.
			 * @note This is useful for broad-phase collision detection: find elements in neighboring
			 *	   sectors that might collide with elements in this sector.
			 * @note Prefer this over getSurroundingSectors() to avoid vector allocation.
			 *
			 * @see getSurroundingSectors()
			 * @see getNeighbor()
			 */
			template< typename function_t >
			void
			forSurroundingSectors (bool includeThisSector, function_t && function) const noexcept
			{
				if ( includeThisSector )
				{
					function(*this);
				}

				/* Iterate through the 26 directions of the Moore neighborhood. */
				for ( int x = -1; x <= 1; ++x )
				{
					for ( int y = -1; y <= 1; ++y )
					{
						for ( int z = -1; z <= 1; ++z )
						{
							/* Skip the center (0, 0, 0), which is the current sector itself. */
							if ( x == 0 && y == 0 && z == 0 )
							{
								continue;
							}

							if ( const auto neighbor = this->getNeighbor(x, y, z) )
							{
								function(*neighbor);
							}
						}
					}
				}
			}

			/**
			 * @brief Returns a vector of surrounding leaf sectors (Moore neighborhood).
			 *
			 * Collects up to 26 neighboring leaf sectors surrounding this sector, plus optionally
			 * this sector itself, and returns them in a vector. The Moore neighborhood includes
			 * all sectors that share a face, edge, or corner with this sector.
			 *
			 * @param includeThisSector If true, this sector is included as the first element of
			 *						  the returned vector.
			 *
			 * @return Vector of shared pointers to neighboring const sectors. May contain fewer
			 *		 than 26 neighbors if some don't exist or aren't subdivided to the same level.
			 *
			 * @deprecated Prefer forSurroundingSectors() to avoid vector allocation overhead.
			 *			 This method allocates a vector on each call, while forSurroundingSectors()
			 *			 uses zero-overhead callbacks.
			 *
			 * @see forSurroundingSectors()
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< const OctreeSector > >
			getSurroundingSectors (bool includeThisSector) const noexcept
			{
				std::vector< std::shared_ptr< const OctreeSector > > sectors;
				/* Reserve space for 26 neighbors and itself */
				sectors.reserve(27);

				if ( includeThisSector )
				{
					sectors.emplace_back(this->shared_from_this());
				}

				/* Iterate through the 26 directions of the Moore neighborhood. */
				for ( int x = -1; x <= 1; ++x )
				{
					for ( int y = -1; y <= 1; ++y )
					{
						for ( int z = -1; z <= 1; ++z )
						{
							/* Skip the center (0, 0, 0), which is the current sector itself. */
							if ( x == 0 && y == 0 && z == 0 )
							{
								continue;
							}

							if ( auto neighbor = this->getNeighbor(x, y, z) )
							{
								sectors.emplace_back(std::move(neighbor));
							}
						}
					}
				}

				return sectors;
			}

			/**
			 * @brief Executes a callback function on every leaf sector that intersects a primitive.
			 *
			 * Recursively traverses the octree and invokes the callback only on leaf sectors that
			 * collide with the provided geometric primitive (frustum, sphere, AABB, etc.). This is
			 * the primary spatial query method for frustum culling, range queries, and collision detection.
			 *
			 * The traversal is optimized via early-exit: if a sector is empty (m_elements.empty()),
			 * the entire subtree is skipped without recursion, thanks to the all-levels storage strategy.
			 *
			 * @tparam primitive_t The primitive type for collision testing (automatically deduced).
			 *					 Supported: Vector<3,float> (point), Sphere, AACuboid, Frustum.
			 * @tparam function_t The callable type (automatically deduced). Must be invocable with
			 *					signature: void(const OctreeSector&).
			 * @param primitive The geometric primitive to test for collision. Only sectors intersecting
			 *				  this primitive will have the callback invoked.
			 * @param function The callable to execute on each leaf sector that intersects the primitive.
			 *
			 * @note Zero-overhead callbacks: template type avoids std::function allocation.
			 * @note Fast early-exit: empty sectors are skipped without descending to children.
			 * @note Only leaf sectors invoke the callback - intermediate nodes are just traversed.
			 * @note Common usage: Frustum culling for rendering, range queries for AI, broad-phase collision.
			 *
			 * @see forLeafSectors()
			 * @see isCollidingWith()
			 */
			template< typename primitive_t, typename function_t >
			void
			forTouchedSector (const primitive_t & primitive, function_t && function) const noexcept
			{
				/* NOTE: Sector empty or out of bound. */
				if ( m_elements.empty() || !this->isCollidingWith(primitive) )
				{
					return;
				}

				/* NOTE: This is a final sector, we can execute the function here. */
				if ( this->isLeaf() )
				{
					function(*this);

					return;
				}

				/* NOTE: Go deeper in the tree before executing the function. */
				for ( const auto & subSector : m_subSectors )
				{
					subSector->forTouchedSector(primitive, function);
				}
			}

			/**
			 * @brief Executes a callback function on every non-empty leaf sector in the subtree.
			 *
			 * Recursively traverses the octree rooted at this sector and invokes the callback on
			 * all leaf sectors that contain at least one element. Empty sectors and their entire
			 * subtrees are skipped via early-exit optimization.
			 *
			 * @tparam function_t The callable type (automatically deduced). Must be invocable with
			 *					signature: void(const OctreeSector&).
			 * @param function The callable to execute on each non-empty leaf sector.
			 *
			 * @note Zero-overhead callbacks: template type avoids std::function allocation.
			 * @note Fast early-exit: empty sectors are skipped without descending to children.
			 * @note Only leaf sectors invoke the callback - intermediate nodes are just traversed.
			 * @note Common usage: Iterate over all elements in the octree, broad-phase collision
			 *	   detection (check all pairs within each leaf), serialization.
			 *
			 * @see forTouchedSector()
			 */
			template< typename function_t >
			void
			forLeafSectors (function_t && function) const noexcept
			{
				/* NOTE: Sector empty, skip entirely. */
				if ( m_elements.empty() )
				{
					return;
				}

				/* NOTE: This is a leaf sector, execute the function here. */
				if ( this->isLeaf() )
				{
					function(*this);

					return;
				}

				/* NOTE: Go deeper in the tree. */
				for ( const auto & subSector : m_subSectors )
				{
					subSector->forLeafSectors(function);
				}
			}

			/**
			 * @brief Finds the deepest (smallest) leaf sector containing an element.
			 *
			 * Recursively traverses the octree downward to find the most specific (deepest) leaf
			 * sector that contains the given element. This is useful for localized spatial queries
			 * and update operations.
			 *
			 * Algorithm: For point-based octrees (enable_volume=false), the element exists in exactly
			 * one leaf sector. For volume-based octrees (enable_volume=true), this returns the first
			 * matching leaf found (elements may span multiple sectors).
			 *
			 * @param element Shared pointer to the element to locate.
			 *
			 * @return Raw pointer to the deepest leaf sector containing the element. Returns this
			 *		 sector itself if it's a leaf. Never returns nullptr if the element is present.
			 *
			 * @pre The element must be present in this sector (call contains() first or ensure
			 *	  this is called from the root sector).
			 *
			 * @note For point-based octrees, this provides the exact leaf sector for fast updates.
			 * @note Has O(depth) complexity as it traverses down the hierarchy.
			 * @note Linear search through 8 child sectors at each level. For position-only queries,
			 *	   prefer getDeepestSubSectorForPosition() which uses O(1) slot calculation.
			 *
			 * @see getDeepestSubSectorForPosition()
			 * @see contains()
			 */
			[[nodiscard]]
			const OctreeSector *
			getDeepestSubSector (const std::shared_ptr< element_t > & element) const noexcept
			{
				/* NOTE: If there is no subsector below this one. */
				if ( !m_isExpanded )
				{
					return this;
				}

				const OctreeSector * deepestSubSector = this;

				for ( const auto & subSector : m_subSectors )
				{
					if ( subSector->contains(element) )
					{
						deepestSubSector = subSector->getDeepestSubSector(element);

						break;
					}
				}

				return deepestSubSector;
			}

			/**
			 * @brief Finds the deepest leaf sector containing a position via direct slot calculation.
			 *
			 * Recursively traverses the octree downward using O(1) slot calculation at each level
			 * to find the leaf sector containing the given position. This is significantly faster
			 * than getDeepestSubSector() as it avoids linear search through child sectors.
			 *
			 * The slot index is computed directly from the position relative to each sector's center
			 * using bit manipulation, enabling immediate selection of the correct child sector.
			 *
			 * @param position The 3D position to locate within the octree.
			 *
			 * @return Raw pointer to the deepest leaf sector containing the position. Returns this
			 *		 sector itself if it's a leaf. Never returns nullptr if position is within bounds.
			 *
			 * @note This is the fastest way to locate a leaf sector for a position (O(depth) with
			 *	   O(1) per level, no search overhead).
			 * @note Used internally by update() for fast-path optimization in point-based octrees.
			 * @note Assumes position is within this sector's bounds (no bounds checking performed).
			 *
			 * @see getDeepestSubSector()
			 * @see computeSlotForPosition()
			 */
			[[nodiscard]]
			const OctreeSector *
			getDeepestSubSectorForPosition (const Libs::Math::Vector< 3, float > & position) const noexcept
			{
				/* NOTE: If there is no subsector below this one. */
				if ( !m_isExpanded )
				{
					return this;
				}

				/* Calculate the slot directly from position relative to sector center. */
				const auto center = this->center();
				const size_t slot = computeSlotForPosition(position, center);

				return m_subSectors[slot]->getDeepestSubSectorForPosition(position);
			}

			/**
			 * @brief Checks whether this sector touches any face of the root octree boundary.
			 *
			 * Determines if this sector is located at the outer edge of the entire octree structure
			 * by analyzing the slot path from this sector up to the root. A sector touches the root
			 * boundary on a given axis if all slots in the parent chain have consistent values for
			 * that axis (all positive or all negative direction).
			 *
			 * Algorithm: For each axis (X, Y, Z), we check if all bits in the slot path are
			 * consistently 0 (positive side) or consistently 1 (negative side). If so, this sector
			 * touches the root boundary on that axis's corresponding face.
			 *
			 * Performance: O(depth) where depth is the distance from this sector to the root.
			 * Uses only integer bit operations (no floating-point comparisons).
			 *
			 * @return True if this sector touches any face of the root octree boundary, false if
			 *		 it is entirely internal (surrounded by other sectors on all sides).
			 *
			 * @note The root sector itself always returns true (it IS the boundary).
			 * @note This is useful for spatial queries that need to handle edge cases differently,
			 *	   such as neighbor finding or boundary condition handling.
			 *
			 * @see isTouchingRootBorderOnAxis()
			 * @see getNeighbor()
			 * @version 0.8.38
			 */
			[[nodiscard]]
			bool
			isTouchingRootBorder () const noexcept
			{
				/* The root sector is the boundary itself. */
				if ( this->isRoot() )
				{
					return true;
				}

				/* Track which faces we might be touching.
				 * For each axis: bit 0 = might touch positive face, bit 1 = might touch negative face.
				 * Initialize to 0b11 (might touch both) for each axis. */
				unsigned int possibleX = 0b11;
				unsigned int possibleY = 0b11;
				unsigned int possibleZ = 0b11;

				auto currentSector = this;

				while ( !currentSector->isRoot() )
				{
					const size_t slot = currentSector->m_slot;

					/* X-axis: bit 2 (value 4). If set = negative X, if clear = positive X.
					 * If we see a positive X slot, we can't be at negative X boundary (clear bit 1).
					 * If we see a negative X slot, we can't be at positive X boundary (clear bit 0). */
					if ( slot & 4 )
					{
						possibleX &= 0b10; /* Clear positive possibility. */
					}
					else
					{
						possibleX &= 0b01; /* Clear negative possibility. */
					}

					/* Y-axis: bit 1 (value 2). */
					if ( slot & 2 )
					{
						possibleY &= 0b10;
					}
					else
					{
						possibleY &= 0b01;
					}

					/* Z-axis: bit 0 (value 1). */
					if ( slot & 1 )
					{
						possibleZ &= 0b10;
					}
					else
					{
						possibleZ &= 0b01;
					}

					/* Early exit: if no face is possible anymore, we're internal. */
					if ( possibleX == 0 && possibleY == 0 && possibleZ == 0 )
					{
						return false;
					}

					currentSector = currentSector->m_parentSector.lock().get();
				}

				/* If any axis still has a possible face, we're touching the boundary. */
				return (possibleX != 0) || (possibleY != 0) || (possibleZ != 0);
			}

			/**
			 * @brief Checks whether this sector touches a specific face of the root octree boundary.
			 *
			 * Determines if this sector is located at the outer edge of the entire octree structure
			 * on a specific axis and direction. This is more specific than isTouchingRootBorder()
			 * and allows querying individual faces.
			 *
			 * @param axis The axis to check (0 = X, 1 = Y, 2 = Z).
			 * @param negative If true, check the negative face (min); if false, check the positive face (max).
			 *
			 * @return True if this sector touches the specified face of the root boundary.
			 *
			 * @note The root sector always returns true for any face.
			 *
			 * @see isTouchingRootBorder()
			 * @version 0.8.38
			 */
			[[nodiscard]]
			bool
			isTouchingRootBorderOnAxis (size_t axis, bool negative) const noexcept
			{
				if ( this->isRoot() )
				{
					return true;
				}

				/* Bit position for this axis in the slot (X=2, Y=1, Z=0). */
				const size_t bitMask = 4 >> axis;

				auto currentSector = this;

				while ( !currentSector->isRoot() )
				{
					const size_t slot = currentSector->m_slot;
					const bool slotIsNegative = (slot & bitMask) != 0;

					/* If the slot direction doesn't match what we're looking for, we're not at that boundary. */
					if ( slotIsNegative != negative )
					{
						return false;
					}

					currentSector = currentSector->m_parentSector.lock().get();
				}

				return true;
			}

		private:

			/**
			 * @brief Evaluates sector state and triggers expansion or collapse as needed.
			 *
			 * This method is called after element insertion or removal to determine whether
			 * the sector should be subdivided (expand) or merged (collapse) based on element count.
			 * It enforces the octree's adaptive subdivision strategy.
			 *
			 * Expansion trigger: If this is a leaf with more than maxElementPerSector elements,
			 * the sector is subdivided into 8 child sectors.
			 *
			 * Collapse trigger (if autoCollapseEnabled): If this is expanded but has fewer than
			 * maxElementPerSector / 2 elements, child sectors are merged back into this sector.
			 *
			 * @return True if the sector is still (or has become) a leaf, false if it's expanded.
			 *
			 * @note This method modifies the sector's structure (may create or destroy child sectors).
			 * @note Called internally by insert() and erase() operations.
			 *
			 * @see expand()
			 * @see collapse()
			 * @see autoCollapseEnabled()
			 */
			[[nodiscard]]
			bool
			isStillLeaf () noexcept
			{
				/* If the number of elements exceeds the sector limit, we split down the sector.
				 * But only if we haven't reached the maximum depth to prevent infinite recursion
				 * when all elements are at the same position. */
				if ( !m_isExpanded && m_elements.size() > m_maxElementPerSector )
				{
					if ( this->getDistance() < DefaultMaxDepth )
					{
						this->expand();

						return false;
					}
					/* else: We've hit max depth, stay as a leaf with many elements. */
				}

				if ( m_autoCollapseEnabled )
				{
					/* If the number of elements is below the sector limit, we merge the subsectors. */
					if ( m_isExpanded && m_elements.size() < m_maxElementPerSector / 2 )
					{
						this->collapse();

						return true;
					}
				}

				return this->isLeaf();
			}

			/**
			 * @brief Internal insertion implementation using a specific collision primitive.
			 *
			 * This is the core insertion algorithm that recursively inserts an element into
			 * this sector and all descendant sectors it collides with. It handles collision
			 * detection, duplicate checking, and automatic subdivision.
			 *
			 * Algorithm:
			 * 1. Test if primitive collides with this sector's bounds
			 * 2. If yes, add element to this sector's element set (ignoring duplicates)
			 * 3. Check if sector should expand due to element count exceeding threshold
			 * 4. If expanded, recursively insert into all 8 child sectors
			 *
			 * @tparam primitive_t The primitive type for collision testing (automatically deduced).
			 *					 Supported: Vector<3,float> (point), Sphere, AACuboid.
			 * @param element Shared pointer to the element to insert.
			 * @param primitive The collision primitive representing the element's spatial extent.
			 *
			 * @return True if insertion succeeded (element collides with this sector), false if
			 *		 element is outside bounds or already present.
			 *
			 * @note This is called by the public insert() method after determining the appropriate
			 *	   primitive based on the element's collision detection model.
			 * @note Duplicate insertions are silently ignored (unordered_set::emplace returns false).
			 *
			 * @see insert()
			 * @see isStillLeaf()
			 */
			template< typename primitive_t >
			bool
			insertWithPrimitive (const std::shared_ptr< element_t > & element, const primitive_t & primitive) noexcept
			{
				if ( !this->isCollidingWith(primitive) )
				{
					return false;
				}

				if ( !m_elements.emplace(element).second )
				{
					return false;
				}

				if ( !this->isStillLeaf() )
				{
					for ( auto & subSector: m_subSectors )
					{
						subSector->insertWithPrimitive(element, primitive);
					}
				}

				return true;
			}

			/**
			 * @brief Internal update implementation that validates and adjusts element placement.
			 *
			 * This method recursively checks whether an element's primitive still intersects
			 * the correct sectors after a position/volume change. It removes the element from
			 * sectors it no longer touches and adds it to new sectors it now intersects.
			 *
			 * Algorithm at each sector:
			 * 1. Test if primitive still collides with this sector
			 * 2. If no collision and not root: remove element from this sector and descendants
			 * 3. If collision but element missing: insert element (moved into this branch)
			 * 4. If collision and element present: recursively check all child sectors
			 *
			 * @tparam primitive_t The primitive type for collision testing (automatically deduced).
			 *					 Supported: Vector<3,float> (point), Sphere, AACuboid.
			 * @param element Shared pointer to the element to update.
			 * @param primitive The collision primitive representing the element's current spatial extent.
			 *
			 * @return True if the element still intersects the octree (remains in the root sector),
			 *		 false if it moved completely outside the root bounds.
			 *
			 * @note This is called by the public update() method after determining the appropriate
			 *	   primitive based on the element's collision detection model.
			 * @note The root sector never removes the element, allowing it to be re-inserted if
			 *	   it moves back into bounds.
			 *
			 * @see update()
			 * @see insertWithPrimitive()
			 */
			template< typename primitive_t >
			bool
			checkElementOverlapWithPrimitive (const std::shared_ptr< element_t > & element, const primitive_t & primitive) noexcept
			{
				if ( !this->isCollidingWith(primitive) )
				{
					/* If this sector is not the root, we remove the element. */
					if ( !this->isRoot() )
					{
						this->erase(element);
					}

					return false;
				}

				/* If the element is not present to this sector.
				 * We let the insertion algorithm do the work. */
				if ( !m_elements.contains(element) )
				{
					return this->insertWithPrimitive(element, primitive);
				}

				/* It this sector is not a leaf, we propagate the current test below. */
				if ( !this->isLeaf() )
				{
					for ( const auto & subSector : m_subSectors )
					{
						subSector->checkElementOverlapWithPrimitive(element, primitive);
					}
				}

				return true;
			}

			/**
			 * @brief Subdivides this sector into eight child sectors.
			 *
			 * Creates 8 child sectors by splitting this sector's volume along each axis at its center point.
			 * Each child sector has half the width/height/depth of the parent. All elements currently in
			 * this sector are redistributed to the appropriate child sectors based on their spatial extent.
			 *
			 * The eight child sectors are arranged according to the octant constants (slot indices 0-7).
			 * This method allocates 8 std::make_shared<OctreeSector> and redistributes all elements
			 * via recursive insert() calls.
			 *
			 * @note OPTIMIZATION CONSIDERATION: This method performs 8 std::make_shared allocations.
			 *	   Potential improvements:
			 *	   - Pool allocator: Pre-allocate sector blocks to reduce allocation overhead.
			 *	   - std::unique_ptr: Lower overhead but breaks enable_shared_from_this and weak_ptr for parent.
			 *	   Current assessment: Expansion is infrequent (only when exceeding m_maxElementPerSector),
			 *	   so the optimization gain would be marginal compared to the implementation complexity.
			 *
			 * @note Element redistribution: All elements in this sector are re-inserted into child sectors.
			 *	   Elements may be inserted into multiple child sectors if their volume spans boundaries.
			 *
			 * @note Called automatically by isStillLeaf() when element count exceeds maxElementPerSector.
			 *
			 * @see collapse()
			 * @see isStillLeaf()
			 */
			void
			expand () noexcept
			{
				using namespace Libs::Math;

				const auto size = this->width() * 0.5F;
				const auto max = this->maximum();

				const auto minX = max[X] - size;
				const auto minY = max[Y] - size;
				const auto minZ = max[Z] - size;

				const auto parent = this->shared_from_this();

				/* Init to max. */
				auto tmp = max;

				//tmp[X] = max[X];
				//tmp[Y] = max[Y];
				//tmp[Z] = max[Z];

				m_subSectors[XPositiveYPositiveZPositive] = std::make_shared< OctreeSector >(tmp, tmp - size, parent, XPositiveYPositiveZPositive);

				//tmp[X] = max[X];
				//tmp[Y] = max[Y];
				tmp[Z] = minZ;

				m_subSectors[XPositiveYPositiveZNegative] = std::make_shared< OctreeSector >(tmp, tmp - size, parent, XPositiveYPositiveZNegative);

				/* Reset only the Z axis to max. */
				//tmp[X] = max[X];
				tmp[Y] = minY;
				tmp[Z] = max[Z];

				m_subSectors[XPositiveYNegativeZPositive] = std::make_shared< OctreeSector >(tmp, tmp - size, parent, XPositiveYNegativeZPositive);

				/* Y-axis is already to min. */
				//tmp[X] = max[X];
				//tmp[Y] = minY;
				tmp[Z] = minZ;

				m_subSectors[XPositiveYNegativeZNegative] = std::make_shared< OctreeSector >(tmp, tmp - size, parent, XPositiveYNegativeZNegative);

				/* Reset to max except for X axis */
				tmp[X] = minX;
				tmp[Y] = max[Y];
				tmp[Z] = max[Z];

				m_subSectors[XNegativeYPositiveZPositive] = std::make_shared< OctreeSector >(tmp, tmp - size, parent, XNegativeYPositiveZPositive);

				//tmp[X] = minX;
				//tmp[Y] = max[Y];
				tmp[Z] = minZ;

				m_subSectors[XNegativeYPositiveZNegative] = std::make_shared< OctreeSector >(tmp, tmp - size, parent, XNegativeYPositiveZNegative);

				/* Reset only the Z axis to max. */
				//tmp[X] = minX;
				tmp[Y] = minY;
				tmp[Z] = max[Z];

				m_subSectors[XNegativeYNegativeZPositive] = std::make_shared< OctreeSector >(tmp, tmp - size, parent, XNegativeYNegativeZPositive);

				/* Y-axis is already to min. */
				//tmp[X] = minX;
				//tmp[Y] = minY;
				tmp[Z] = minZ;

				m_subSectors[XNegativeYNegativeZNegative] = std::make_shared< OctreeSector >(tmp, tmp - size, parent, XNegativeYNegativeZNegative);

				/* Now, we redistribute the sector elements to the subsectors. */
				for ( const auto & element : m_elements )
				{
					for ( const auto & subSector : m_subSectors )
					{
						subSector->insert(element);
					}
				}

				m_isExpanded = true;
			}

			/**
			 * @brief Merges child sectors back into this sector (removes subdivision).
			 *
			 * Destroys all 8 child sectors by resetting their shared pointers, deallocating them
			 * and transitioning this sector back to a leaf state. All elements remain in this
			 * sector's element set (they were already present due to all-levels storage).
			 *
			 * This operation is triggered automatically when auto-collapse is enabled and the
			 * element count drops below maxElementPerSector / 2.
			 *
			 * @note Elements are not removed or re-inserted - they remain in this sector's set.
			 * @note Child sectors are deallocated, freeing memory.
			 * @note Only called when autoCollapseEnabled is true.
			 *
			 * @see expand()
			 * @see isStillLeaf()
			 * @see autoCollapseEnabled()
			 */
			void
			collapse () noexcept
			{
				for ( auto & subSector : m_subSectors )
				{
					subSector.reset();
				}

				m_isExpanded = false;
			}

			/**
			 * @brief Computes the octree slot index for a position using bit manipulation.
			 *
			 * Determines which of the 8 child sectors (octants) a position falls into by comparing
			 * the position to the center point along each axis. The result is a 3-bit index where
			 * each bit corresponds to an axis:
			 * - Bit 2 (value 4): X axis - set if position.x < center.x (negative X half)
			 * - Bit 1 (value 2): Y axis - set if position.y < center.y (negative Y half)
			 * - Bit 0 (value 1): Z axis - set if position.z < center.z (negative Z half)
			 *
			 * Examples:
			 * - (+X, +Y, +Z): slot 0 (binary 000) = XPositiveYPositiveZPositive
			 * - (+X, +Y, -Z): slot 1 (binary 001) = XPositiveYPositiveZNegative
			 * - (-X, -Y, -Z): slot 7 (binary 111) = XNegativeYNegativeZNegative
			 *
			 * @param position The 3D position to locate within the octree.
			 * @param center The center point of the sector for comparison.
			 *
			 * @return The slot index (0-7) indicating which octant contains the position.
			 *
			 * @note This is a static method and can be called without a sector instance.
			 * @note O(1) operation using only 3 comparisons and bit operations.
			 * @note Used by getDeepestSubSectorForPosition() for fast octree traversal.
			 *
			 * @see getDeepestSubSectorForPosition()
			 */
			[[nodiscard]]
			static size_t
			computeSlotForPosition (const Libs::Math::Vector< 3, float > & position, const Libs::Math::Vector< 3, float > & center) noexcept
			{
				using namespace Libs::Math;

				size_t slot = 0;

				/* X-axis: bit 2 (value 4). Negative X sets the bit. */
				if ( position[X] < center[X] )
				{
					slot |= 4;
				}

				/* Y-axis: bit 1 (value 2). Negative Y sets the bit. */
				if ( position[Y] < center[Y] )
				{
					slot |= 2;
				}

				/* Z-axis: bit 0 (value 1). Negative Z sets the bit. */
				if ( position[Z] < center[Z] )
				{
					slot |= 1;
				}

				return slot;
			}

			/**
			 * @brief Recursively finds a neighbor sector in a given direction.
			 * @param dirX Direction on X axis (-1, 0, or 1).
			 * @param dirY Direction on Y axis (-1, 0, or 1).
			 * @param dirZ Direction on Z axis (-1, 0, or 1).
			 * @return std::shared_ptr< const OctreeSector > A pointer to the neighbor, or nullptr if none exists.
			 */
			[[nodiscard]]
			std::shared_ptr< const OctreeSector >
			getNeighbor (int dirX, int dirY, int dirZ) const noexcept
			{
				/* If this is the root, there are no neighbors at this level. */
				if ( this->isRoot() )
				{
					return nullptr;
				}

				const auto parent = m_parentSector.lock();
				const size_t mySlot = this->m_slot;

				/* Determine the target slot based on a direction. */
				size_t targetSlot = mySlot;

				/* Check for crossing parent boundaries on each axis. */
				const bool crossX = (dirX > 0 && (mySlot & 4) == 0) || (dirX < 0 && (mySlot & 4) != 0);
				const bool crossY = (dirY > 0 && (mySlot & 2) == 0) || (dirY < 0 && (mySlot & 2) != 0);
				const bool crossZ = (dirZ > 0 && (mySlot & 1) == 0) || (dirZ < 0 && (mySlot & 1) != 0);

				/* If we don't cross any boundary, the neighbor is a sibling. */
				if ( !crossX && !crossY && !crossZ )
				{
					/* Flip X bit. */
					if ( dirX != 0 )
					{
						targetSlot ^= 4;
					}

					/* Flip Y bit. */
					if ( dirY != 0 )
					{
						targetSlot ^= 2;
					}

					/* Flip Z bit */
					if ( dirZ != 0 )
					{
						targetSlot ^= 1;
					}

					return parent->m_subSectors[targetSlot];
				}

				/* If we cross a boundary, we must ask the parent for its neighbor. */
				auto parentNeighbor = parent->getNeighbor(dirX, dirY, dirZ);

				if ( !parentNeighbor || parentNeighbor->isLeaf() )
				{
					/* The "uncle" sector doesn't exist or isn't subdivided, so no neighbor. */
					return nullptr;
				}

				/* We have the uncle sector. Now we need to find the correct child within it.
				 * The target slot is our own slot, with the axe bits flipped for the directions we did NOT cross. */
				size_t descendantSlot = mySlot;

				/* Flip if not crossing X. */
				if ( dirX == 0 )
				{
					descendantSlot ^= 4;
				}

				/* Flip if not crossing Y. */
				if ( dirY == 0 )
				{
					descendantSlot ^= 2;
				}

				/* Flip if not crossing Z. */
				if ( dirZ == 0 )
				{
					descendantSlot ^= 1;
				}

				return parentNeighbor->subSectors()[descendantSlot];
			}

			/* Flag names. */
			static constexpr auto IsExpanded{0UL};
			static constexpr auto AutoCollapseEnabled{1UL};

			std::weak_ptr< OctreeSector > m_parentSector;
			std::array< std::shared_ptr< OctreeSector >, SectorDivision > m_subSectors;
			/**
			 * @brief Set of elements in this sector.
			 * @note OPTIMIZATION CONSIDERATION: Using std::shared_ptr adds atomic refcount overhead on insert/erase.
			 *	   Alternative: std::unordered_set<element_t*> with external lifetime management.
			 *	   Risk: If an entity is destroyed without being removed from the octree, it causes undefined behavior.
			 *	   Current assessment: The shared_ptr provides safety as the Scene manages entity lifetime.
			 *	   The refcount overhead is acceptable given the protection it provides against dangling pointers.
			 */
			std::unordered_set< std::shared_ptr< element_t > > m_elements;
			size_t m_slot{std::numeric_limits< size_t >::max()};
			size_t m_maxElementPerSector;
			const bool m_autoCollapseEnabled{false};
			bool m_isExpanded{false};
	};
}
