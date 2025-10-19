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
#include <cstdint>
#include <cstring>
#include <array>

/* Local inclusions. */
#include "Graphics/ViewMatricesInterface.hpp"
#include "Vulkan/CommandBuffer.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Vulkan;

	void
	Unique::pushMatricesForShadowCasting (const CommandBuffer & commandBuffer, const PipelineLayout & pipelineLayout, const Saphir::Program & program, uint32_t readStateIndex, const ViewMatricesInterface & viewMatrices, const CartesianFrame< float > * worldCoordinates) const noexcept
	{
		const VkShaderStageFlags stageFlags = program.hasGeometryShader() ?
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT :
			VK_SHADER_STAGE_VERTEX_BIT;

		/* Prepare the model matrix (M). */
		Matrix< 4, float > modelMatrix;

		/* NOTE: If world coordinates are a nullptr, we assume to render the object at the origin. */
		if ( worldCoordinates != nullptr )
		{
			modelMatrix = this->isFacingCamera() ?
				worldCoordinates->getSpriteModelMatrix(viewMatrices.position(readStateIndex)) :
				worldCoordinates->getModelMatrix();
		}

		if ( this->isFlagEnabled(ApplyTransformationMatrix) )
		{
			modelMatrix *= this->transformationMatrix();
		}

		/* Prepare the view matrix (V). */
		const auto & viewMatrix = viewMatrices.viewMatrix(readStateIndex, this->isUsingInfinityView(), 0);

		/* Prepare the projection matrix (P). */
		const auto & projectionMatrix = viewMatrices.projectionMatrix(readStateIndex);

		/* Compute the model view projection matrix (MVP). */
		const auto modelViewProjectionMatrix = projectionMatrix * viewMatrix * modelMatrix;

		vkCmdPushConstants(
			commandBuffer.handle(),
			pipelineLayout.handle(),
			stageFlags,
			0,
			MatrixBytes,
			modelViewProjectionMatrix.data()
		);
	}

	void
	Unique::pushMatricesForRendering (const CommandBuffer & commandBuffer, const PipelineLayout & pipelineLayout, const Saphir::Program & program, uint32_t readStateIndex, const ViewMatricesInterface & viewMatrices, const CartesianFrame< float > * worldCoordinates) const noexcept
	{
		const VkShaderStageFlags stageFlags = program.hasGeometryShader() ?
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT :
			VK_SHADER_STAGE_VERTEX_BIT;

		/* Prepare the model matrix (M). */
		Matrix< 4, float > modelMatrix;

		/* NOTE: If world coordinates are a nullptr, we assume to render the object at the origin. */
		if ( worldCoordinates != nullptr )
		{
			modelMatrix = this->isFacingCamera() ?
				worldCoordinates->getSpriteModelMatrix(viewMatrices.position(readStateIndex)) :
				worldCoordinates->getModelMatrix();
		}

		if ( this->isFlagEnabled(ApplyTransformationMatrix) )
		{
			modelMatrix *= this->transformationMatrix();
		}

		/* Prepare the view matrix (V). */
		const auto & viewMatrix = viewMatrices.viewMatrix(readStateIndex, this->isUsingInfinityView(), 0);

		/* Prepare the projection matrix (P). */
		const auto & projectionMatrix = viewMatrices.projectionMatrix(readStateIndex);

		if ( program.wasAdvancedMatricesEnabled() )
		{
			if constexpr ( MergePushConstants )
			{
				/* Create a single buffer for 2x mat4x4. */
				std::array< float, 32 > buffer{};
				std::memcpy(buffer.data(), viewMatrix.data(), MatrixBytes);
				std::memcpy(&buffer[Matrix4Alignment], modelMatrix.data(), MatrixBytes);

				/* Push the view matrix (V) and the model matrix (M) in a single call. */
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
				/* Push the view matrix (V). */
				vkCmdPushConstants(
					commandBuffer.handle(),
					pipelineLayout.handle(),
					stageFlags,
					0,
					MatrixBytes,
					viewMatrix.data()
				);

				/* Push the model matrix (M). */
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
			/* Compute the model view projection matrix (MVP). */
			const auto modelViewProjectionMatrix = projectionMatrix * viewMatrix * modelMatrix;

			/* Push the model view projection matrix (MVP). */
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
