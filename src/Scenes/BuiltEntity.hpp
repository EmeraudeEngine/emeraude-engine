/*
 * src/Scenes/BuiltEntity.hpp
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
#include <utility>
#include <type_traits>

/* Local inclusions for usage. */
#include "AbstractEntity.hpp"
#include "Component/Abstract.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief Class representing temporarily the link between a node and a new component.
	 * @tparam entity_t The type of entity.
	 * @tparam component_t The type of component.
	 */
	template< typename entity_t, typename component_t >
	requires (std::is_base_of_v< AbstractEntity, entity_t >, std::is_base_of_v< Component::Abstract, component_t >)
	class BuiltEntity final
	{
		public:

		/** @brief Class identifier. */
		static constexpr auto ClassId{"ScenesToolkit"};

		/**
		 * @brief Constructs a default built entity.
		 */
		BuiltEntity () = default;

		/**
		 * @brief Constructs a built entity.
		 * @param entity A smart pointer over a scene entity.
		 * @param component A smart pointer over the build component.
		 */
		BuiltEntity (std::shared_ptr< entity_t > entity, std::shared_ptr< component_t > component) noexcept
			: m_entity{std::move(entity)},
			m_component{std::move(component)}
		{

		}

		/**
		 * @brief Returns whether the built entity is valid.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isValid () const noexcept
		{
			return !static_cast< bool >(m_entity == nullptr || m_component == nullptr);
		}

		/**
		 * @brief Returns the node or static entity smart pointer.
		 * @return std::shared_ptr< entity_t >
		 */
		[[nodiscard]]
		std::shared_ptr< entity_t >
		entity () const noexcept
		{
			return m_entity;
		}

		/**
		 * @brief Returns the component smart pointer.
		 * @return std::shared_ptr< component_t >
		 */
		[[nodiscard]]
		std::shared_ptr< component_t >
		component () const noexcept
		{
			return m_component;
		}

		private:

		std::shared_ptr< entity_t > m_entity;
		std::shared_ptr< component_t > m_component;
	};
}
