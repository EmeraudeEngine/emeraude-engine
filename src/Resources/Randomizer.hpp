/*
 * src/Resources/Randomizer.hpp
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
#include <vector>
#include <string>
#include <type_traits>

/* Local inclusions for usage. */
#include "Container.hpp"
#include "ResourceTrait.hpp"
#include "Tracer.hpp"

namespace EmEn::Resources
{
	/**
	 * @brief Resource collection to ease a random pick.
	 * @tparam resource_t The resource type.
	 */
	template< typename resource_t >
	requires (std::is_base_of_v< ResourceTrait, resource_t >)
	class Randomizer final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Randomizer"};

			/**
			 * @brief Constructs a randomizer.
			 * @param container A reference to the resource container.
			 */
			explicit
			Randomizer (const Container< resource_t > & container) noexcept
				: m_container{&container}
			{

			}

			/**
			 * @brief Adds resources by their names.
			 * @param resourceNames A reference to a string vector.
			 * @return void
			 */
			void
			set (const std::vector< std::string > & resourceNames) noexcept
			{
				if ( m_container == nullptr )
				{
					Tracer::error(ClassId, "The container pointer is null !");

					return;
				}

				for ( const auto & resourceName : resourceNames )
				{
					if ( !m_container->isResourceExists(resourceName) )
					{
						return;
					}

					m_collection.emplace_back(m_container->getResource(resourceName));
				}
			}

			/**
			 * @brief Returns one resource randomly.
			 * @return std::shared_ptr< resource_t >
			 */
			std::shared_ptr< resource_t >
			pickOne () noexcept
			{
				if ( this->empty() )
				{
					return nullptr;
				}

				if ( m_collection.size() == 1 )
				{
					return m_collection.at(0);
				}

				const auto index = Libs::Utility::quickRandom(0UL, m_collection.size() - 1);

				return m_collection.at(index);
			}

			/**
			 * @brief Returns whether the collection is empty.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			empty () const noexcept
			{
				return m_collection.empty();
			}

			/**
			 * @brief clear
			 */
			void
			clear () noexcept
			{
				m_collection.clear();
			}

		private:

			Container< resource_t > * m_container;
			std::vector< std::shared_ptr< resource_t > > m_collection;
	};
}
