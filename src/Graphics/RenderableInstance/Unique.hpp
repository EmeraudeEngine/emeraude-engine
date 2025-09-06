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
#include "Libs/Math/CartesianFrame.hpp"
#include "Graphics/Renderable/Interface.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	/**
	 * @brief This is a renderable object that uses an UBO to determine the location of the renderable object.
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
			 * @param flagBits The multiple renderable instance level flags. Default 0.
			 */
			explicit
			Unique (const std::shared_ptr< Renderable::Interface > & renderable, uint32_t flagBits = 0) noexcept
				: Abstract{renderable, flagBits}
			{

			}

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::isModelMatricesCreated() const */
			[[nodiscard]]
			bool
			isModelMatricesCreated () const noexcept override
			{
				return true;
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

		private:

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::pushMatrices() */
			void pushMatrices (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::PipelineLayout & pipelineLayout, const Saphir::Program & program, uint32_t readStateIndex, const ViewMatricesInterface & viewMatrices, const Libs::Math::CartesianFrame< float > * worldCoordinates) const noexcept override;

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::instanceCount() */
			[[nodiscard]]
			uint32_t
			instanceCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::RenderableInstance::Abstract::bindInstanceModelLayer() */
			void bindInstanceModelLayer (const Vulkan::CommandBuffer & commandBuffer, uint32_t layerIndex) const noexcept override;
	};
}
