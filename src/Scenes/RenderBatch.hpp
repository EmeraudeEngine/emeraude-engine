/*
 * src/Scenes/RenderBatch.hpp
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
#include <cstdint>
#include <memory>
#include <map>

/* Local inclusions for usages. */
#include "Graphics/RenderableInstance/Abstract.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The RenderBatch class.
	 */
	class RenderBatch final
	{
		public :

			using List = std::multimap< uint64_t, RenderBatch >;

			/**
			 * @brief Constructs a render batch.
			 * @param renderableInstance A reference to a renderable instance smart pointer.
			 * @param worldCoordinates A pointer to the cartesian frame.
			 * @param subGeometryIndex The layer index of the renderable.
			 */
			explicit
			RenderBatch (const std::shared_ptr< const Graphics::RenderableInstance::Abstract > & renderableInstance, const Libs::Math::CartesianFrame< float > * worldCoordinates, uint32_t subGeometryIndex) noexcept
				: m_renderableInstance{renderableInstance},
				m_worldCoordinates{worldCoordinates},
				m_subGeometryIndex{subGeometryIndex}
			{

			}

			/**
			 * @brief Returns the renderable instance pointer.
			 * @return std::shared_ptr< const Graphics::RenderableInstance::Abstract >
			 */
			[[nodiscard]]
			std::shared_ptr< const Graphics::RenderableInstance::Abstract >
			renderableInstance () const noexcept
			{
				return m_renderableInstance;
			}

			/**
			 * @brief Returns the world coordinates of the renderable instance.
			 * @note If nullptr, this means at the origin.
			 * @warning Do not store this pointer!
			 * @return const Libs::Math::CartesianFrame< float > *
			 */
			[[nodiscard]]
			const Libs::Math::CartesianFrame< float > *
			worldCoordinates () const noexcept
			{
				return m_worldCoordinates;
			}

			/**
			 * @brief Returns the batch index of the renderable.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			subGeometryIndex () const noexcept
			{
				return m_subGeometryIndex;
			}

			/**
			 * @brief Static method to instantiate a render batch and register it to a render list.
			 * @param renderList A referencer to a render list.
			 * @param distance The distance of the renderable from the camera.
			 * @param renderableInstance A pointer to the renderable instance.
			 * @param worldCoordinates A pointer to the cartesian frame.
			 * @param subGeometryIndex The layer index of the renderable.
			 * @return void
			 */
			static
			void
			create (List & renderList, float distance, const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance, const Libs::Math::CartesianFrame< float > * worldCoordinates, uint32_t subGeometryIndex) noexcept
			{
				renderList.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(static_cast< uint64_t >(distance * DistanceMultiplier)),
					std::forward_as_tuple(renderableInstance, worldCoordinates, subGeometryIndex)
				);
			}

		private :

			static constexpr auto DistanceMultiplier{1000.0F};

			const std::shared_ptr< const Graphics::RenderableInstance::Abstract > m_renderableInstance;
			const Libs::Math::CartesianFrame< float > * m_worldCoordinates{nullptr};
			const uint32_t m_subGeometryIndex;
	};
}
