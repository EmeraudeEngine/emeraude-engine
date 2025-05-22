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
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Graphics/Renderable/Interface.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Matrix.hpp"
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
			 * @param renderable A reference to a smart pointer of a renderable object.
			 * @param instanceLocations A reference to a vector of coordinates. The max location count will be extracted from size().
			 * @param flagBits The multiple renderable instance level flags. Default 0.
			 */
			Multiple (const std::shared_ptr< Renderable::Interface > & renderable, const std::vector< Libs::Math::CartesianFrame< float > > & instanceLocations, uint32_t flagBits = 0) noexcept;

			/**
			 * @brief Constructs a renderable instance.
			 * @param renderable A reference to a smart pointer of a renderable object.
			 * @param instanceCount The maximum of number of locations hold by this instance.
			 * @param flagBits The multiple renderable instance level flags. Default 0.
			 */
			Multiple (const std::shared_ptr< Renderable::Interface > & renderable, uint32_t instanceCount, uint32_t flagBits = 0) noexcept;

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

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::resetModelMatrices() */
			void
			resetModelMatrices () noexcept override
			{
				this->resetLocalData();

				this->updateVideoMemory();
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

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::worldPosition() */
			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			worldPosition () const noexcept override
			{
				// TODO: This should have the average location ...
				return {};
			}

			/**
			 * @brief Sets the active instance count to draw.
			 * @param count The number of instance.
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
			 * @param instanceOffset The instance index from which the update take places.
			 * @return bool
			 */
			[[nodiscard]]
			bool updateLocalData (const std::vector< Libs::Math::CartesianFrame< float > > & instanceLocations, uint32_t instanceOffset = 0) noexcept;

			/**
			 * @brief Copies local data to video memory.
			 * @return bool
			 */
			bool updateVideoMemory () noexcept;

		private:

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::pushMatrices() */
			void pushMatrices (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::PipelineLayout & pipelineLayout, const ViewMatricesInterface & viewMatrices, const Saphir::Program & program) const noexcept override;

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
			 * @return bool
			 */
			[[nodiscard]]
			bool createModelMatrices () noexcept;

			/**
			 * @brief Resets local data to identity matrices.
			 * @return void
			 */
			void resetLocalData () noexcept;

			/**
			 * @brief Converts a list of coordinates to a list of model matrices.
			 * @param coordinates A reference to a list of coordinates.
			 * @param modelMatrices A reference to a list of model matrices.
			 * @param strict Ensure the two vectors are same size. Default no.
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
