/*
 * src/Graphics/RenderableInstance/Unique.cpp
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

#include "Unique.hpp"

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>

/* Local inclusions. */
#include "Abstract.hpp"
#include "Graphics/Renderable/Interface.hpp"
#include "Graphics/Types.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Saphir/Program.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/PipelineLayout.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Vulkan;

	Unique::Unique (const std::shared_ptr< Renderable::Interface > & renderable, const CartesianFrame< float > & location, uint32_t flagBits) noexcept
		: Abstract(renderable, flagBits), m_cartesianFrame(location)
	{
		this->observe(renderable.get());
	}

	void
	Unique::pushMatrices (const CommandBuffer & commandBuffer, const PipelineLayout & pipelineLayout, const ViewMatricesInterface & viewMatrices, const Saphir::Program & program) const noexcept
	{
		constexpr uint32_t MatrixBytes{Matrix4Alignment * sizeof(float)};

		const VkShaderStageFlags stageFlags = program.hasGeometryShader() ?
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT :
			VK_SHADER_STAGE_VERTEX_BIT;

		const auto & viewMatrix = viewMatrices.viewMatrix(this->isUsingInfinityView(), 0);

		auto modelMatrix = this->isFacingCamera() ?
			m_cartesianFrame.getSpriteModelMatrix(viewMatrices.position()) :
			m_cartesianFrame.getModelMatrix();

		if ( this->isFlagEnabled(ApplyTransformationMatrix) )
		{
			modelMatrix *= this->transformationMatrix();
		}

		/* [VULKAN-PUSH-CONSTANT:4] Push camera related matrices. */
		if ( program.wasAdvancedMatricesEnabled() )
		{
			if constexpr ( MergePushConstants )
			{
				/* NOTE: Create a single buffer for 2x mat4x4. */
				std::array< float, 32 > buffer{};
				std::memcpy(buffer.data(), viewMatrix.data(), MatrixBytes);
				std::memcpy(&buffer[Matrix4Alignment], modelMatrix.data(), MatrixBytes);

				/* NOTE: Push the view matrix (V) and the model matrix (M). */
				vkCmdPushConstants(
					commandBuffer.handle(),
					pipelineLayout.handle(),
					stageFlags,
					0,
					MatrixBytes * 2,
					buffer.data()
				);
			}
			else
			{
				/* NOTE: Push the view matrix (V). */
				vkCmdPushConstants(
					commandBuffer.handle(),
					pipelineLayout.handle(),
					stageFlags,
					0,
					MatrixBytes,
					viewMatrix.data()
				);

				/* NOTE: Push the model matrix (M). */
				vkCmdPushConstants(
					commandBuffer.handle(),
					pipelineLayout.handle(),
					stageFlags,
					MatrixBytes,
					MatrixBytes,
					modelMatrix.data()
				);
			}
		}
		else
		{
			const auto modelViewProjectionMatrix = viewMatrices.projectionMatrix() * viewMatrix * modelMatrix;

			/* NOTE: Push the model view projection matrix (MVP). */
			vkCmdPushConstants(
				commandBuffer.handle(),
				pipelineLayout.handle(),
				stageFlags,
				0,
				MatrixBytes,
				modelViewProjectionMatrix.data()
			);
		}
	}

	void
	Unique::bindInstanceModelLayer (const CommandBuffer & commandBuffer, uint32_t layerIndex) const noexcept
	{
		/* Bind the geometry VBO and the optional IBO. */
		commandBuffer.bind(*this->renderable()->geometry(), layerIndex);
	}
}
