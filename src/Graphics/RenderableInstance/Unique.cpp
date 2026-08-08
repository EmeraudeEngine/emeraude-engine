/*
 * src/Graphics/RenderableInstance/Unique.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
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
#include "Vulkan/PipelineLayout.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	void
	Unique::pushMatricesForShadowCasting (const RenderPassContext & passContext, const PushConstantContext & pushContext, const CartesianFrame< float > * worldCoordinates) const noexcept
	{
		/* Prepare the model matrix (M). */
		Matrix< 4, float > modelMatrix;

		/* NOTE: If world coordinates are a nullptr, we assume to render the object at the origin. */
		if ( worldCoordinates != nullptr )
		{
			modelMatrix = this->renderable()->isSprite() ?
				worldCoordinates->getSpriteModelMatrix(passContext.viewMatrices->position(passContext.readStateIndex)) :
				worldCoordinates->getModelMatrix();
		}

		if ( this->isFlagEnabled(ApplyTransformationMatrix) )
		{
			modelMatrix *= this->transformationMatrix();
		}

		/* For cubemap/CSM rendering, View/Projection matrices are in UBO indexed by gl_ViewIndex.
		 * We only push the Model matrix. */
		if ( passContext.isCubemap || passContext.isCSM )
		{
			vkCmdPushConstants(
				passContext.commandBuffer->handle(),
				pushContext.pipelineLayout->handle(),
				pushContext.stageFlags,
				0,
				MatrixBytes,
				modelMatrix.data()
			);
		}
		else
		{
			/* Classic 2D rendering: compute and push MVP. */
			const auto & viewMatrix = passContext.viewMatrices->viewMatrix(passContext.readStateIndex, this->isUsingInfinityView(), 0);
			const auto & projectionMatrix = passContext.viewMatrices->projectionMatrix(passContext.readStateIndex);
			const auto modelViewProjectionMatrix = projectionMatrix * viewMatrix * modelMatrix;

			vkCmdPushConstants(
				passContext.commandBuffer->handle(),
				pushContext.pipelineLayout->handle(),
				pushContext.stageFlags,
				0,
				MatrixBytes,
				modelViewProjectionMatrix.data()
			);
		}
	}

	void
	Unique::pushMatricesForRendering (const RenderPassContext & passContext, const PushConstantContext & pushContext, const CartesianFrame< float > * worldCoordinates) const noexcept
	{
		/* InstanceTransforms SSBO paths: the model matrix is read from the per-instance
		 * SSBO entry selected by the firstInstance draw parameter (the slot staged at
		 * Scene::prepareRender()), so its CPU-side computation is skipped entirely.
		 * Classic: push VP + jitter + frameIndex (76 B). Advanced: push V + jitter + frameIndex
		 * (76 B, the projection comes from the view UBO) — this replaces the V+M+frameIndex
		 * fallback (132 B) that violates the 128 B Vulkan minimum guarantee.
		 * The pushed matrices are UNJITTERED: the sub-pixel TAA offset is applied to
		 * gl_Position by the vertex shader from the jitter member, so the velocity clip
		 * positions stay jitter-free. Layout MUST match
		 * Saphir::Generator::Abstract::declareMatrixPushConstantBlock().
		 * NOTE: The generator only sets this flag on non-cubemap programs. */
		if ( pushContext.useInstanceTransforms )
		{
			const auto & viewMatrix = passContext.viewMatrices->viewMatrix(passContext.readStateIndex, this->isUsingInfinityView(), 0);

			std::array< float, Matrix4Alignment + 3 > buffer{};

			if ( pushContext.useAdvancedMatrices )
			{
				std::memcpy(buffer.data(), viewMatrix.data(), MatrixBytes);
			}
			else
			{
				const auto & projectionMatrix = passContext.viewMatrices->unjitteredProjectionMatrix(passContext.readStateIndex);
				const auto viewProjectionMatrix = projectionMatrix * viewMatrix;

				std::memcpy(buffer.data(), viewProjectionMatrix.data(), MatrixBytes);
			}

			const auto & projectionJitter = passContext.viewMatrices->projectionJitter();

			buffer[Matrix4Alignment] = projectionJitter.x();
			buffer[Matrix4Alignment + 1] = projectionJitter.y();
			buffer[Matrix4Alignment + 2] = static_cast< float >(this->frameIndexFor(pushContext.layerIndex));

			vkCmdPushConstants(passContext.commandBuffer->handle(), pushContext.pipelineLayout->handle(), pushContext.stageFlags, 0, MatrixBytes + (3 * sizeof(float)), buffer.data());

			return;
		}

		/* Prepare the model matrix (M). */
		Matrix< 4, float > modelMatrix;

		/* NOTE: If world coordinates are a nullptr, we assume to render the object at the origin. */
		if ( worldCoordinates != nullptr )
		{
			modelMatrix = this->renderable()->isSprite() ?
				worldCoordinates->getSpriteModelMatrix(passContext.viewMatrices->position(passContext.readStateIndex)) :
				worldCoordinates->getModelMatrix();
		}

		if ( this->isFlagEnabled(ApplyTransformationMatrix) )
		{
			modelMatrix *= this->transformationMatrix();
		}

		const auto handle = passContext.commandBuffer->handle();
		const auto layout = pushContext.pipelineLayout->handle();
		const auto flags = pushContext.stageFlags;
		const auto frameIndex = static_cast< float >(this->frameIndexFor(pushContext.layerIndex));

		/* For cubemap rendering, View/Projection matrices are in UBO indexed by gl_ViewIndex.
		 * We only push the Model matrix (and optionally normal matrix for lighting). */
		if ( passContext.isCubemap )
		{
			/* Push the model matrix (M) + frameIndex. */
			std::array< float, Matrix4Alignment + 1 > buffer{};
			std::memcpy(buffer.data(), modelMatrix.data(), MatrixBytes);
			buffer[Matrix4Alignment] = frameIndex;

			vkCmdPushConstants(handle, layout, flags, 0, MatrixBytes + sizeof(float), buffer.data());
		}
		else if ( pushContext.useAdvancedMatrices )
		{
			/* Classic 2D with advanced matrices: push View, Model, and frameIndex.
			 * ASSUMED LIMIT: this fallback (reachable only when the scene provides no
			 * InstanceTransforms SSBO) carries NO jitter member — the block is already at 132 B,
			 * above the 128 B Vulkan minimum guarantee, and must not grow. Its projection comes
			 * from the view UBO, which is now always clean, so these renderables are rasterized
			 * unjittered while TAA is active: they simply do not benefit from the temporal AA
			 * (stable, no artifact — their velocity output is zero anyway). */
			const auto & viewMatrix = passContext.viewMatrices->viewMatrix(passContext.readStateIndex, this->isUsingInfinityView(), 0);

			std::array< float, (Matrix4Alignment * 2) + 1 > buffer{};
			std::memcpy(buffer.data(), viewMatrix.data(), MatrixBytes);
			std::memcpy(&buffer[Matrix4Alignment], modelMatrix.data(), MatrixBytes);
			buffer[Matrix4Alignment * 2] = frameIndex;

			vkCmdPushConstants(handle, layout, flags, 0, (MatrixBytes * 2UL) + sizeof(float), buffer.data());
		}
		else
		{
			/* Classic 2D simple: compute and push MVP + frameIndex.
			 * NOTE: projectionMatrix() serves the JITTERED matrix while TAA is active — wanted
			 * here: a CPU-computed MVP is this path's only way to jitter, and it emits no
			 * velocity (no previous-matrix source), so nothing has to undo the offset. */
			const auto & viewMatrix = passContext.viewMatrices->viewMatrix(passContext.readStateIndex, this->isUsingInfinityView(), 0);
			const auto & projectionMatrix = passContext.viewMatrices->projectionMatrix(passContext.readStateIndex);
			const auto modelViewProjectionMatrix = projectionMatrix * viewMatrix * modelMatrix;

			std::array< float, Matrix4Alignment + 1 > buffer{};
			std::memcpy(buffer.data(), modelViewProjectionMatrix.data(), MatrixBytes);
			buffer[Matrix4Alignment] = frameIndex;

			vkCmdPushConstants(handle, layout, flags, 0, MatrixBytes + sizeof(float), buffer.data());
		}
	}

	void
	Unique::bindInstanceModelLayer (const CommandBuffer & commandBuffer, uint32_t layerIndex, uint32_t LODLevel) const noexcept
	{
		/* Bind the geometry VBO and the optional IBO. */
		commandBuffer.bind(*this->renderable()->geometry(LODLevel), layerIndex);
	}
}
