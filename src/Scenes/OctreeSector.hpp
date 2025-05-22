/*
 * src/Scenes/OctreeSector.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include "Libs/Math/Space3D/Collisions/SphereCuboid.hpp"
#include "LocatableInterface.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The octree sector class.
	 * @tparam element_t The type of inserted element, it must inherit from EmEn::Scenes::LocatableInterface.
	 * @tparam enable_volume Enable the use of the element volume instead of their position. This implies multiple insertions at the same depth level.
	 * @extends std::enable_shared_from_this A sector must be able to give his own smart pointer.
	 * @extends EmEn::Libs::Math::Space3D::AACuboid A sector is a cube in the 3D space and thus provides intersection detection with primitives.
	 */
	template< typename element_t, bool enable_volume >
	requires (std::is_base_of_v< Libs::NameableTrait, element_t >, std::is_base_of_v< LocatableInterface, element_t >)
	class OctreeSector final : public std::enable_shared_from_this< OctreeSector< element_t, enable_volume > >, public Libs::Math::Space3D::AACuboid< float >
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"OctreeSector"};

			static constexpr auto SectorDivision{8UL};
			static constexpr auto DefaultSectorElementLimit{8UL};

			static constexpr auto XPositiveYPositiveZPositive{0UL};
			static constexpr auto XPositiveYPositiveZNegative{1UL};
			static constexpr auto XPositiveYNegativeZPositive{2UL};
			static constexpr auto XPositiveYNegativeZNegative{3UL};
			static constexpr auto XNegativeYPositiveZPositive{4UL};
			static constexpr auto XNegativeYPositiveZNegative{5UL};
			static constexpr auto XNegativeYNegativeZPositive{6UL};
			static constexpr auto XNegativeYNegativeZNegative{7UL};

			/**
			 * @brief Constructs a root octree sector.
			 * @note The root sector is a leaf when empty.
			 * @param maximum The highest limit of the root sector.
			 * @param minimum The lowest limit of the root sector.
			 * @param maxElementPerSector The threshold number of elements to trigger a new sector subdivision. Default 8.
			 * @param enableAutoCollapse Enable a leaf sector to be automatically removed if empty. Default false.
			 */
			OctreeSector (const Libs::Math::Vector< 3, float > & maximum, const Libs::Math::Vector< 3, float > & minimum, size_t maxElementPerSector = DefaultSectorElementLimit, bool enableAutoCollapse = false) noexcept
				: AACuboid{maximum, minimum},
				m_maxElementPerSector{std::max< size_t >(DefaultSectorElementLimit, maxElementPerSector)}
			{
				m_flags[AutoCollapseEnabled] = enableAutoCollapse;
			}

			/**
			 * @brief Constructs a child octree sector.
			 * @param maximum The highest limit of the child sector.
			 * @param minimum The lowest limit of the child sector.
			 * @param parentSector A reference to the parent sector smart pointer.
			 * @param slot The slot where the subsector is built.
			 */
			OctreeSector (const Libs::Math::Vector< 3, float > & maximum, const Libs::Math::Vector< 3, float > & minimum, const std::shared_ptr< OctreeSector > & parentSector, size_t slot) noexcept
				: AACuboid{maximum, minimum},
				m_parentSector{parentSector},
				m_slot{slot},
				m_maxElementPerSector{parentSector->m_maxElementPerSector}
			{
				m_flags[AutoCollapseEnabled] = parentSector->m_flags[AutoCollapseEnabled];
			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			OctreeSector (const OctreeSector & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			OctreeSector (OctreeSector && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return OctreeSector &
			 */
			OctreeSector & operator= (const OctreeSector & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return OctreeSector &
			 */
			OctreeSector & operator= (OctreeSector && copy) noexcept = delete;

			/**
			 * @brief Destructs the octree sector.
			 */
			~OctreeSector () = default;

			/**
			 * @brief Returns true if the sector is the top of the tree.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRoot () const noexcept
			{
				return m_parentSector.expired();
			}

			/**
			 * @brief Returns whether the sector is an endpoint in the tree.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLeaf () const noexcept
			{
				return !m_flags[IsExpanded];
			}

			/**
			 * @brief Returns whether the sector has subsectors.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isExpanded () const noexcept
			{
				return m_flags[IsExpanded];
			}

			/**
			 * @brief Returns whether this sector has no element registered in it.
			 * @warning This method does not indicate whether there are no more subsectors below.
			 * But it is guaranteed that there can only be empty subsectors underneath.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_elements.empty();
			}

			/**
			 * @brief Returns the slot of a subsector.
			 * @warning It this sector is root, the value will be the max for a size_t.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			slot () const noexcept
			{
				return m_slot;
			}

			/**
			 * @brief Returns the number of elements hold in an octree before expanding.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			maxElementPerSector () const noexcept
			{
				return m_maxElementPerSector;
			}

			/**
			 * @brief Returns whether the automatic empty leaf sector removal is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			autoCollapseEnabled () const noexcept
			{
				return m_flags[AutoCollapseEnabled];
			}

			/**
			 * @brief Returns the sector distance from the root sector.
			 * @note If the sector is root, the method will return 0.
			 * @return size_t
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
			 * @brief Returns the depth below this one.
			 * @note From the root sector, this method will give the depth of the whole octree.
			 * @return size_t
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
			 * @brief Returns the sectors count below this one (included).
			 * @return size_t
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
			 * @brief Returns the parent sector.
			 * @warning This can be nullptr. Check if the sector is root before.
			 * @return std::weak_ptr< OctreeSector >
			 */
			[[nodiscard]]
			std::weak_ptr< OctreeSector >
			parentSector () noexcept
			{
				return m_parentSector;
			}

			/**
			 * @brief Returns the parent sector.
			 * @warning This can be nullptr. Check if the sector is root before.
			 * @return std::weak_ptr< const OctreeSector >
			 */
			[[nodiscard]]
			std::weak_ptr< const OctreeSector >
			parentSector () const noexcept
			{
				return m_parentSector;
			}

			/**
			 * @brief Returns the root sector.
			 * @return std::shared_ptr< OctreeSector >.
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
			 * @brief Returns the root sector.
			 * @return std::shared_ptr< const OctreeSector >.
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
			 * @brief Returns the subsector list.
			 * @warning This can be a list of empty sector. Check if the sector is leaf before.
			 * @return std::array< std::shared_ptr< OctreeSector >, 8 > &
			 */
			[[nodiscard]]
			std::array< std::shared_ptr< OctreeSector >, SectorDivision > &
			subSectors () noexcept
			{
				return m_subSectors;
			}

			/**
			 * @brief Returns the subsector list.
			 * @warning This can be a list of empty sector. Check if the sector is leaf before.
			 * @return const std::array< std::shared_ptr< OctreeSector >, 8 > &
			 */
			[[nodiscard]]
			const std::array< std::shared_ptr< OctreeSector >, SectorDivision > &
			subSectors () const noexcept
			{
				return m_subSectors;
			}

			/**
			 * @brief Reserves subsectors by specifying the desired depth.
			 * @note This won't have any effect with automatic collapse enabled.
			 * @param depth
			 * @return void
			 */
			void
			reserve (size_t depth) noexcept
			{
				if ( m_flags[AutoCollapseEnabled] )
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
			 * @brief Returns whether the entity is present in the sector.
			 * @param element A reference to an element smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			contains (const std::shared_ptr< element_t > & element) const noexcept
			{
				return m_elements.contains(element);
			}

			/**
			 * @brief Returns whether a primitive is colling with the octree sector.
			 * @tparam primitive_t The type of primitive.
			 * @param primitive A reference to a primitive.
			 * @return bool
			 */
			template< typename primitive_t >
			[[nodiscard]]
			bool
			isCollidingWith (const primitive_t & primitive) const noexcept
			{
				return Libs::Math::Space3D::isColliding(*this, primitive);
			}

			/**
			 * @brief Adds an element to the tree at the level of this sector and below.
			 * @note This is the main function determining the collision primitive to use.
			 * @param element A reference to an element smart pointer.
			 * @return bool
			 */
			bool
			insert (const std::shared_ptr< element_t > & element) noexcept
			{
				if constexpr ( enable_volume )
				{
					if ( element->sphereCollisionIsEnabled() )
					{
						return this->insertWithPrimitive(element, element->getWorldBoundingSphere());
					}

					return this->insertWithPrimitive(element, element->getWorldBoundingBox());
				}
				else
				{
					return this->insertWithPrimitive(element, element->getWorldCoordinates().position());
				}
			}

			/**
			 * @brief Checks whether an element still in this sector and below.
			 * @note This is the main function determining the collision primitive to use.
			 * @param element A reference to an element smart pointer.
			 * @return bool
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
				if ( !m_flags[IsExpanded] )
				{
					return true;
				}

				if constexpr ( enable_volume )
				{
					if ( element->sphereCollisionIsEnabled() )
					{
						return this->checkElementOverlapWithPrimitive(element, element->getWorldBoundingSphere());
					}

					return this->checkElementOverlapWithPrimitive(element, element->getWorldBoundingBox());
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
			 * @brief Removes an element from the tree this level and below.
			 * @param element A reference to an element smart pointer.
			 * @return bool
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
			 * @brief Returns the number of elements present in this sector.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			elementCount () const noexcept
			{
				return m_elements.size();
			}

			/**
			 * @brief Returns the element list of the sector.
			 * @return const std::unordered_set< std::shared_ptr< element_t > > &
			 */
			[[nodiscard]]
			const std::unordered_set< std::shared_ptr< element_t > > &
			elements () const noexcept
			{
				return m_elements;
			}

			/**
			 * @brief Tries to find the first named element in the octree.
			 * @param name A reference to a string
			 * @return std::shared_ptr< element_t >
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
			 * @brief Returns a list of surrounding leaf sectors (Moore neighborhood, 26 sectors).
			 * @param includeThisSector Include the current sector in the list.
			 * @return std::vector< std::shared_ptr< const OctreeSector > >
			 */
			[[nodiscard]]
			std::vector< std::shared_ptr< const OctreeSector > >
			getSurroundingSectors (bool includeThisSector) const noexcept
			{
				std::vector< std::shared_ptr< const OctreeSector > > sectors;
				/* Reserve space for 26 neighbors + self */
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
							/* Skip the center (0,0,0) which is the current sector itself. */
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
			 * @brief Executes a function on every subsector inside the area.
			 * @tparam primitive_t The type of primitive for collision detection.
			 * @param primitive A reference to the primitive.
			 * @param function A reference to lambda.
			 * @return void
			 */
			template< typename primitive_t >
			void
			forTouchedSector (const primitive_t & primitive, const std::function< void (const OctreeSector &) > & function) const noexcept
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
			 * @brief Returns a pointer to a subsector when an element appears.
			 * @note This assumes the element is part of this sector.
			 * @param element A reference to an element smart pointer.
			 * @return const OctreeSector *
			 */
			[[nodiscard]]
			const OctreeSector *
			getDeepestSubSector (const std::shared_ptr< element_t > & element) const noexcept
			{
				/* NOTE: If there is no subsector below this one. */
				if ( !m_flags[IsExpanded] )
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

		private:

			/**
			 * @brief Checks the sector content to expand or collapse it and returns if the sector is a leaf or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isStillLeaf () noexcept
			{
				/* If the number of elements exceeds the sector limit, we split down the sector. */
				if ( !m_flags[IsExpanded] && m_elements.size() > m_maxElementPerSector )
				{
					this->expand();

					return false;
				}

				if ( m_flags[AutoCollapseEnabled] )
				{
					/* If the number of elements is below the sector limit, we merge the subsectors. */
					if ( m_flags[IsExpanded] && m_elements.size() < m_maxElementPerSector / 2 )
					{
						this->collapse();

						return true;
					}
				}

				return this->isLeaf();
			}

			/**
			 * @brief Adds an element to the tree at the level of this sector and below.
			 * @tparam primitive_t The type of primitive for collision detection.
			 * @param element A reference to the element smart pointer.
			 * @param primitive A reference to the primitive.
			 * @return bool
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
			 * @brief Checks whether an element still in this sector and below.
			 * @tparam primitive_t The type of primitive for collision detection.
			 * @param element A reference to the element smart pointer.
			 * @param primitive A reference to the primitive.
			 * @return bool
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
			 * @brief Expands this sector to eight new ones.
			 * @return void.
			 */
			void
			expand () noexcept
			{
				using namespace EmEn::Libs::Math;

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

				m_flags[IsExpanded] = true;
			}

			/**
			 * @brief Merges the sectors underlying this one.
			 * @return void
			 */
			void
			collapse () noexcept
			{
				for ( auto & subSector : m_subSectors )
				{
					subSector.reset();
				}

				m_flags[IsExpanded] = false;
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
			std::unordered_set< std::shared_ptr< element_t > > m_elements;
			size_t m_slot{std::numeric_limits< size_t >::max()};
			size_t m_maxElementPerSector;
			std::array< bool, 8 > m_flags{
				false/*IsExpanded*/,
				false/*AutoCollapseEnabled*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
