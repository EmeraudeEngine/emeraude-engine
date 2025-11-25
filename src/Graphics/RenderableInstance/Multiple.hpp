/*
 * src/Graphics/RenderableInstance/Multiple.hpp
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
#include <cstdint>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Matrix.hpp"
#include "Graphics/Renderable/Interface.hpp"
#include "Vulkan/VertexBufferObject.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	/**
	 * @brief This is a renderable object that uses a VBO to determine multiple locations for the renderable object.
	 * @note This version uses its own VBO to store locations.
	 * @extends EmEn::Graphics::RenderableInstance::Abstract It needs the base of a renderable instance.
	 */
	class Multiple final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RenderableInstanceMultiple"};

			/**
			 * @brief Constructs a renderable instance.
			 * @param device A reference to the device smart-pointer.
			 * @param renderable A reference to a smart pointer of a renderable object.
			 * @param instanceLocations A reference to a vector of coordinates. The max location count will be extracted from size().
			 * @param flagBits The multiple renderable instance level flags. Default 0.
			 */
			Multiple (const std::shared_ptr< Vulkan::Device > & device, const std::shared_ptr< Renderable::Interface > & renderable, const std::vector< Libs::Math::CartesianFrame< float > > & instanceLocations, uint32_t flagBits = 0) noexcept;

			/**
			 * @brief Constructs a renderable instance.
			 * @param device A reference to the device smart-pointer.
			 * @param renderable A reference to a smart pointer of a renderable object.
			 * @param instanceCount The maximum of number of locations holds by this instance.
			 * @param flagBits The multiple renderable instance level flags. Default 0.
			 */
			Multiple (const std::shared_ptr< Vulkan::Device > & device, const std::shared_ptr< Renderable::Interface > & renderable, uint32_t instanceCount, uint32_t flagBits = 0) noexcept;

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::isModelMatricesCreated() const */
			[[nodiscard]]
			bool
			isModelMatricesCreated () const noexcept override
			{
				if ( m_vertexBufferObject == nullptr )
				{
					return false;
				}

				return m_vertexBufferObject->isCreated();
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::useModelUniformBufferObject() */
			[[nodiscard]]
			bool
			useModelUniformBufferObject () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::useModelVertexBufferObject() */
			[[nodiscard]]
			bool
			useModelVertexBufferObject () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Sets the active instance count to draw.
			 * @param count The number of instances.
			 * @return void
			 */
			void
			setActiveInstanceCount (uint32_t count) noexcept
			{
				m_activeInstanceCount = std::min(count, m_instanceCount);
			}

			/**
			 * @brief Updates a unique instance location.
			 * @param instanceLocation A reference to a cartesian frame.
			 * @param instanceIndex The instance index.
			 * @return bool
			 */
			[[nodiscard]]
			bool updateLocalData (const Libs::Math::CartesianFrame< float > & instanceLocation, uint32_t instanceIndex) noexcept;

			/**
			 * @brief Updates instance locations from a batch.
			 * @param instanceLocations A reference to a cartesian frame vector.
			 * @param instanceOffset The instance index from which the update takes places.
			 * @return bool
			 */
			[[nodiscard]]
			bool updateLocalData (const std::vector< Libs::Math::CartesianFrame< float > > & instanceLocations, uint32_t instanceOffset = 0) noexcept;

			/**
			 * @brief Copies local data to video memory.
			 * @return bool
			 */
			bool updateVideoMemory () noexcept;

			/**
			 * @brief Reset the local data.
			 * @return void
			 */
			void
			resetModelMatrices () noexcept
			{
				this->resetLocalData();

				this->disableFlag(ArePositionsSynchronized);
			}

		private:

			/**
			 * @brief Push constant strategy for Multiple (shadow casting).
			 *
			 * @par Matrix Distribution
			 * | Mode      | Push Constants | VBO Content | UBO Content                   |
			 * |-----------|----------------|-------------|-------------------------------|
			 * | Cubemap   | (none)         | M per inst. | VP[6] indexed by gl_ViewIndex |
			 * | Billboard | V + VP         | pos + scale | -                             |
			 * | Simple    | VP only        | M per inst. | -                             |
			 *
			 * @par Key Difference from Unique
			 * Multiple stores Model matrices in a VBO (one per instance), so we never
			 * push M via push constants. For cubemap, this means NO push constants at all.
			 *
			 * @par Billboard Mode
			 * Sprites need both V (for orientation) and VP (for final transform).
			 * The VBO contains position + scale, not full matrices.
			 */
			void pushMatricesForShadowCasting (const RenderPassContext & passCtx, const PushConstantContext & pushCtx, const Libs::Math::CartesianFrame< float > * worldCoordinates) const noexcept override;

			/**
			 * @brief Push constant strategy for Multiple (scene rendering).
			 *
			 * @par Matrix Distribution
			 * | Mode              | Push Constants | VBO Content | UBO Content                   |
			 * |-------------------|----------------|-------------|-------------------------------|
			 * | Cubemap           | (none)         | M per inst. | VP[6] indexed by gl_ViewIndex |
			 * | Advanced/Billboard| V + VP         | M per inst. | -                             |
			 * | Simple            | VP only        | M per inst. | -                             |
			 *
			 * @par Key Difference from Unique
			 * Multiple stores Model matrices in a VBO (one per instance), so we never
			 * push M via push constants. For cubemap, this means NO push constants at all
			 * - the most efficient path for GPU instancing.
			 *
			 * @par Mode Selection
			 * - **Cubemap**: Zero push constants (M in VBO, VP in UBO)
			 * - **Advanced**: Lighting needs V for world-space reconstruction
			 * - **Billboard**: V needed to compute camera-facing orientation
			 * - **Simple**: Just VP, shader computes final position as VP * M * vertex
			 */
			void pushMatricesForRendering (const RenderPassContext & passCtx, const PushConstantContext & pushCtx, const Libs::Math::CartesianFrame< float > * worldCoordinates) const noexcept override;

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::instanceCount() */
			[[nodiscard]]
			uint32_t
			instanceCount () const noexcept override
			{
				return m_activeInstanceCount;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::bindInstanceModelLayer() */
			void bindInstanceModelLayer (const Vulkan::CommandBuffer & commandBuffer, uint32_t layerIndex) const noexcept override;

			/**
			 * @brief Creates the model matrices.
			 * @param device A reference to the device smart-pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createOnHardware (const std::shared_ptr< Vulkan::Device > & device) noexcept;

			/**
			 * @brief Resets local data to identity matrices.
			 * @return void
			 */
			void resetLocalData () noexcept;

			/**
			 * @brief Converts a list of coordinates to a list of model matrices.
			 * @param coordinates A reference to a list of coordinates.
			 * @param modelMatrices A reference to a list of model matrices.
			 * @param strict Ensure the two vectors are the same size. Default no.
			 * @return bool
			 */
			static bool coordinatesToModelMatrices (const std::vector< Libs::Math::CartesianFrame< float > > & coordinates, std::vector< Libs::Math::Matrix< 4, float > > & modelMatrices, bool strict = false) noexcept;

			/* Position vector (vec3 aligned to a vec4) + scale vector (vec3 aligned to a vec4) */
			//static constexpr uint32_t SpriteVBOElementCount = 4UL + 4UL;
			//static constexpr uint32_t SpriteVBOElementBytes = 16UL + 16UL;
			/* Model matrix 4x4 (4 x vec4) + normal matrix 3x3 (3 x vec3 aligned to a vec4) */
			//static constexpr uint32_t MeshVBOElementCount = 16UL + 12UL;
			//static constexpr uint32_t MeshVBOElementBytes = 64UL + 48UL;

			/* Position vector + scale vector */
			static constexpr uint32_t SpriteVBOElementCount = 3U + 3U;
			/* Model matrix 4x4 + normal matrix 3x3 */
			static constexpr uint32_t MeshVBOElementCount = 16U + 9U;

			std::unique_ptr< Vulkan::VertexBufferObject > m_vertexBufferObject;
			std::vector< float > m_localData;
			uint32_t m_instanceCount{0};
			uint32_t m_activeInstanceCount{0};
	};
}
