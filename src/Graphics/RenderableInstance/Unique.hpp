/*
 * src/Graphics/RenderableInstance/Unique.hpp
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

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Graphics/Renderable/Interface.hpp"
#include "Libs/Math/CartesianFrame.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	/**
	 * @brief This is a renderable object that use an UBO to determine the location of the renderable object.
	 * @extends EmEn::Graphics::RenderableInstance::Abstract It needs the base of a renderable instance.
	 */
	class Unique final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RenderableInstanceUnique"};

			/**
			 * @brief Constructs a renderable instance.
			 * @param renderable A reference to a smart pointer of a renderable object.
			 * @param location A reference to a coordinates for the initial location. Default origin.
			 * @param flagBits The multiple renderable instance level flags. Default 0.
			 */
			explicit Unique (const std::shared_ptr< Renderable::Interface > & renderable, const Libs::Math::CartesianFrame< float > & location = {}, uint32_t flagBits = 0) noexcept;

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::isModelMatricesCreated() const */
			[[nodiscard]]
			bool
			isModelMatricesCreated () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::resetModelMatrices() */
			void
			resetModelMatrices () noexcept override
			{
				m_cartesianFrame.reset();
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::useModelUniformBufferObject() */
			[[nodiscard]]
			bool
			useModelUniformBufferObject () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::useModelVertexBufferObject() */
			[[nodiscard]]
			bool
			useModelVertexBufferObject () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::worldPosition() */
			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			worldPosition () const noexcept override
			{
				return m_cartesianFrame.position();
			}

			/**
			 * @brief Updates the renderable instance cartesian frame.
			 * @note The coordinates of the frame expected to be absolute.
			 * @param worldCartesianFrame A reference to a cartesian frame.
			 * @return void
			 */
			void
			updateModelMatrix (const Libs::Math::CartesianFrame< float > & worldCartesianFrame) noexcept
			{
				m_cartesianFrame = worldCartesianFrame;
			}

		private:

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::pushMatrices() */
			void pushMatrices (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::PipelineLayout & pipelineLayout, const ViewMatricesInterface & viewMatrices, const Saphir::Program & program) const noexcept override;

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::instanceCount() */
			[[nodiscard]]
			uint32_t
			instanceCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::bindInstanceModelLayer() */
			void bindInstanceModelLayer (const Vulkan::CommandBuffer & commandBuffer, uint32_t layerIndex) const noexcept override;

			Libs::Math::CartesianFrame< float > m_cartesianFrame;
	};
}
