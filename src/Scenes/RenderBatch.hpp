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
#include <algorithm>
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
			 * @param LODLevel The geometry LOD level for this batch.
			 */
			explicit
			RenderBatch (const std::shared_ptr< const Graphics::RenderableInstance::Abstract > & renderableInstance, const Libs::Math::CartesianFrame< float > * worldCoordinates, uint32_t subGeometryIndex, uint32_t LODLevel = 0) noexcept
				: m_renderableInstance{renderableInstance},
				m_worldCoordinates{worldCoordinates},
				m_subGeometryIndex{subGeometryIndex},
				m_LODLevel{LODLevel}
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
			 * @brief Returns the geometry LOD level for this batch.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			LODLevel () const noexcept
			{
				return m_LODLevel;
			}

			/**
			 * @brief Static method to instantiate a render batch and register it to a render list.
			 * @param renderList A referencer to a render list.
			 * @param distance The distance of the renderable from the camera.
			 * @param renderableInstance A pointer to the renderable instance.
			 * @param worldCoordinates A pointer to the cartesian frame.
			 * @param subGeometryIndex The layer index of the renderable.
			 * @param LODLevel The geometry LOD level for this batch.
			 * @return void
			 */
			static
			void
			create (List & renderList, float distance, const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance, const Libs::Math::CartesianFrame< float > * worldCoordinates, uint32_t subGeometryIndex, uint32_t LODLevel = 0) noexcept
			{
				renderList.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(static_cast< uint64_t >(distance * DistanceMultiplier)),
					std::forward_as_tuple(renderableInstance, worldCoordinates, subGeometryIndex, LODLevel)
				);
			}

		/**
		 * @brief Creates a render batch with a state-sorted composite key for opaque rendering.
		 *
		 * The 64-bit key is structured as:
		 * - Bits 63-48: Pipeline identity (hash of instance flags affecting shader selection)
		 * - Bits 47-32: Material identity (low bits of material pointer address)
		 * - Bits 31-16: Geometry identity (low bits of geometry pointer address)
		 * - Bits 15-0 : Quantized distance (rough front-to-back for early-Z benefit)
		 *
		 * This ordering minimizes Vulkan state changes: pipeline binds are most expensive,
		 * followed by descriptor set binds, then geometry (VBO/IBO) binds.
		 *
		 * @param renderList A reference to a render list.
		 * @param distance The distance of the renderable from the camera.
		 * @param renderableInstance A pointer to the renderable instance.
		 * @param worldCoordinates A pointer to the cartesian frame.
		 * @param subGeometryIndex The layer index of the renderable.
		 * @param LODLevel The geometry LOD level for this batch.
		 */
		static
		void
		createStateSorted (List & renderList, float distance, const std::shared_ptr< Graphics::RenderableInstance::Abstract > & renderableInstance, const Libs::Math::CartesianFrame< float > * worldCoordinates, uint32_t subGeometryIndex, uint32_t LODLevel = 0) noexcept
		{
			const auto * renderable = renderableInstance->renderable();

			/* Pipeline identity: hash instance flags that affect shader/pipeline selection. */
			uint64_t pipelineHash = 0;
			uint64_t materialId = 0;
			uint64_t geometryId = 0;

			if ( renderable != nullptr )
			{
				/* Combine flags that differentiate pipelines. */
				pipelineHash = static_cast< uint64_t >(renderableInstance->useModelVertexBufferObject()) << 4
					| static_cast< uint64_t >(renderableInstance->isLightingEnabled()) << 3
					| static_cast< uint64_t >(renderableInstance->isDepthTestDisabled()) << 2
					| static_cast< uint64_t >(renderableInstance->isDepthWriteDisabled()) << 1;

				/* Material identity: low 16 bits of material object address. */
				if ( const auto * material = renderable->material(subGeometryIndex); material != nullptr )
				{
					/* Mix in the material layout hash for pipeline layout discrimination. */
					if ( const auto layout = material->descriptorSetLayout(); layout != nullptr )
					{
						pipelineHash ^= static_cast< uint64_t >(layout->getHash() & 0xFFFF);
					}

					materialId = reinterpret_cast< uintptr_t >(material) & 0xFFFF;
				}

				/* Geometry identity: low 16 bits of geometry object address. */
				if ( const auto * geometryAddr = renderable->geometry(LODLevel); geometryAddr != nullptr )
				{
					geometryId = reinterpret_cast< uintptr_t >(geometryAddr) & 0xFFFF;
				}
			}

			/* Quantized distance: front-to-back for early-Z, clamped to 16 bits. */
			const auto quantizedDistance = static_cast< uint64_t >(std::min(distance * DistanceMultiplier, static_cast< float >(UINT16_MAX)));

			/* Compose the 64-bit sort key. */
			const uint64_t sortKey =
				((pipelineHash & 0xFFFF) << 48) |
				((materialId & 0xFFFF) << 32) |
				((geometryId & 0xFFFF) << 16) |
				(quantizedDistance & 0xFFFF);

			renderList.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(sortKey),
				std::forward_as_tuple(renderableInstance, worldCoordinates, subGeometryIndex, LODLevel)
			);
		}

	private :

		static constexpr auto DistanceMultiplier{1000.0F};

			const std::shared_ptr< const Graphics::RenderableInstance::Abstract > m_renderableInstance;
			const Libs::Math::CartesianFrame< float > * m_worldCoordinates{nullptr};
			const uint32_t m_subGeometryIndex;
			const uint32_t m_LODLevel{0};
	};
}
