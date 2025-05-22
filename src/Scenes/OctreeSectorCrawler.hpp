/*
 * src/Scenes/OctreeSectorCrawler.hpp
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
#include <memory>
#include <stack>

namespace EmEn::Scenes
{
	/**
	 * @brief Utility class to crawl down the octree sector from a base sector.
	 * @tparam octree_t The type of octree. FIXME: Find the right syntax to use "enable_if" with a template...
	 */
	template< typename octree_t >
	class OctreeSectorCrawler final
	{
		public:

			/**
			 * @brief Constructs a crawler from a sector.
			 * @param baseSector From which sector to execute the crawling.
			 */
			explicit
			OctreeSectorCrawler (const std::shared_ptr< octree_t > & baseSector) noexcept
			{
				this->populateStack(baseSector);
			}

			/**
			 * @brief Returns whether a next sector exists.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasNextSector () const noexcept
			{
				return !m_sectors.empty();
			}

			/**
			 * @brief Returns the next populated leaf sector.
			 * @return std::shared_ptr< octree_t >
			 */
			[[nodiscard]]
			std::shared_ptr< octree_t >
			nextSector () noexcept
			{
				if ( m_sectors.empty() )
				{
					return nullptr;
				}

				/* Gets the next sector */
				auto nextSector = m_sectors.top();

				/* Removes it from the stack. */
				m_sectors.pop();

				/* Prepares the next sectors. */
				this->populateStack(nextSector);

				return nextSector;
			}

		private:

			/**
			 * @brief Populates the stacks with sub sectors.
			 * @param currentSector A reference to the smart pointer of the current sector.
			 * @return void
			 */
			void
			populateStack (const std::shared_ptr< octree_t > & currentSector) noexcept
			{
				if ( currentSector->empty() || currentSector->isLeaf() )
				{
					return;
				}

				for ( const auto & sector : currentSector->subSectors() )
				{
					m_sectors.emplace(sector);
				}
			}

			std::stack< std::shared_ptr< octree_t > > m_sectors;
	};
}
