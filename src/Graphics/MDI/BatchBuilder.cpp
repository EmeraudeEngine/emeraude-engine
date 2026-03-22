/*
 * src/Graphics/MDI/BatchBuilder.cpp
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

#include "BatchBuilder.hpp"

/* STL inclusions. */
#include <cstring>

/* Local inclusions. */
#include "Graphics/Geometry/Interface.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Graphics/RenderableInstance/RenderStateTracker.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Saphir/Program.hpp"
#include "Tracer.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/IndirectBuffer.hpp"
#include "Vulkan/PipelineLayout.hpp"

namespace EmEn::Graphics::MDI
{
	using namespace Libs::Math;

	BatchBuilder::BatchBuilder (const std::shared_ptr< Vulkan::Device > & device, uint32_t framesInFlight) noexcept
		: m_device{device},
		m_framesInFlight{framesInFlight}
	{
		m_batches.reserve(256);
	}

	bool
	BatchBuilder::createResources () noexcept
	{
		if ( m_device == nullptr )
		{
			Tracer::error(ClassId, "No Vulkan device provided !");

			return false;
		}

		constexpr VkDeviceSize perDrawSSBOSize = MaxMDIDraws * sizeof(PerDrawData);
		constexpr VkDeviceSize indirectBufferSize = MaxMDIDraws * sizeof(VkDrawIndexedIndirectCommand);

		m_perDrawSSBOs.resize(m_framesInFlight);
		m_indirectBuffers.resize(m_framesInFlight);

		for ( uint32_t frame = 0; frame < m_framesInFlight; ++frame )
		{
			/* Create per-draw SSBO with BDA support (STORAGE + SHADER_DEVICE_ADDRESS).
			 * Host-visible for CPU-side writes of model matrices each frame. */
			m_perDrawSSBOs[frame] = std::make_unique< Vulkan::Buffer >(
				m_device, 0, perDrawSSBOSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				true
			);

			if ( !m_perDrawSSBOs[frame]->createOnHardware() )
			{
				Tracer::error(ClassId, "Failed to create per-draw SSBO !");

				return false;
			}

			/* Create indirect command buffer. */
			m_indirectBuffers[frame] = std::make_unique< Vulkan::IndirectBuffer >(m_device, indirectBufferSize);

			if ( !m_indirectBuffers[frame]->createOnHardware() )
			{
				Tracer::error(ClassId, "Failed to create indirect buffer !");

				return false;
			}
		}

		m_isReady = true;

		Tracer::success(ClassId, "MDI resources created successfully (per-draw SSBO + indirect buffers).");

		return true;
	}

	uint32_t
	BatchBuilder::buildBatches (const Scenes::RenderBatch::List & renderList, uint32_t frameIndex, uint32_t /*readStateIndex*/) noexcept
	{
		if ( !m_isReady || renderList.empty() )
		{
			return 0;
		}

		m_batches.clear();
		m_totalDrawsBatched = 0;
		m_totalFallbackDraws = 0;
		m_skippedCount = 0;

		auto * perDrawData = m_perDrawSSBOs[frameIndex]->mapMemoryAs< PerDrawData >();
		auto * indirectCmds = m_indirectBuffers[frameIndex]->mapMemoryAs< VkDrawIndexedIndirectCommand >();

		if ( perDrawData == nullptr || indirectCmds == nullptr )
		{
			Tracer::error(ClassId, "Failed to map MDI buffers !");

			return 0;
		}

		uint32_t totalDrawIndex = 0;

		/* Track the current batch state to detect group boundaries. */
		const Renderable::Abstract * currentRenderable = nullptr;
		const Material::Interface * currentMaterial = nullptr;
		const Geometry::Interface * currentGeometry = nullptr;
		uint32_t currentLayerIndex = UINT32_MAX;

		for ( const auto & renderBatch : renderList | std::views::values )
		{
			if ( totalDrawIndex >= MaxMDIDraws )
			{
				break;
			}

			const auto & instance = renderBatch.renderableInstance();
			const auto * renderable = instance->renderable();

			if ( renderable == nullptr )
			{
				continue;
			}

			/* Skip objects that require per-object special handling incompatible with MDI. */
			if ( renderable->isSprite() || instance->isUsingInfinityView() )
			{
				m_skippedCount++;
				continue;
			}

			const auto * geometry = renderable->geometry(0);

			if ( geometry != nullptr && geometry->isAdaptiveLOD() )
			{
				m_skippedCount++;
				continue;
			}

			const auto layerIndex = renderBatch.subGeometryIndex();
			const auto * material = renderable->material(layerIndex);

			if ( material == nullptr || geometry == nullptr )
			{
				continue;
			}

			/* Detect batch boundary: new group when renderable, material, geometry, or layer changes. */
			if ( renderable != currentRenderable || material != currentMaterial ||
				geometry != currentGeometry || layerIndex != currentLayerIndex )
			{
				/* Start a new batch. */
				MDIBatch batch{};
				batch.indirectOffset = totalDrawIndex * sizeof(VkDrawIndexedIndirectCommand);
				batch.firstDrawIndex = totalDrawIndex;
				batch.drawCount = 0;

				m_batches.push_back(std::move(batch));

				currentRenderable = renderable;
				currentMaterial = material;
				currentGeometry = geometry;
				currentLayerIndex = layerIndex;
			}

			/* Fill per-draw data (model matrix). */
			const auto * worldCoordinates = renderBatch.worldCoordinates();

			if ( worldCoordinates != nullptr )
			{
				perDrawData[totalDrawIndex].modelMatrix = worldCoordinates->getModelMatrix();
			}
			else
			{
				/* Identity matrix for objects at origin. */
				perDrawData[totalDrawIndex].modelMatrix = Matrix< 4, float >::identity();
			}

			perDrawData[totalDrawIndex].frameIndex = 0; /* TODO: animated materials */

			/* Fill indirect draw command using sub-geometry range for multi-layer support. */
			const auto subRange = geometry->subGeometryRange(layerIndex);

			indirectCmds[totalDrawIndex].indexCount = subRange[1];
			indirectCmds[totalDrawIndex].instanceCount = 1;
			indirectCmds[totalDrawIndex].firstIndex = subRange[0];
			indirectCmds[totalDrawIndex].vertexOffset = 0;
			indirectCmds[totalDrawIndex].firstInstance = 0;

			/* Add this render batch to the group and increment draw count. */
			m_batches.back().renderBatches.push_back(&renderBatch);
			m_batches.back().drawCount++;
			totalDrawIndex++;
		}

		/* Unmap buffers. */
		m_perDrawSSBOs[frameIndex]->unmapMemory();
		m_indirectBuffers[frameIndex]->unmapMemory();

		/* Count batched vs fallback draws. */
		for ( const auto & batch : m_batches )
		{
			if ( batch.drawCount > 1 )
			{
				m_totalDrawsBatched += batch.drawCount;
			}
			else
			{
				m_totalFallbackDraws += batch.drawCount;
			}
		}

		return static_cast< uint32_t >(m_batches.size());
	}

	void
	BatchBuilder::dispatch (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer, uint32_t frameIndex, uint32_t readStateIndex, const BindlessTextureManager * bindlessTexturesManager) const noexcept
	{
		if ( !m_isReady || m_batches.empty() )
		{
			return;
		}

		const auto bdaAddress = this->perDrawSSBOAddress(frameIndex);

		RenderableInstance::RenderStateTracker tracker{};

		for ( const auto & batch : m_batches )
		{
			if ( batch.renderBatches.empty() )
			{
				continue;
			}

			const auto & firstBatch = *batch.renderBatches[0];
			const auto & instance = firstBatch.renderableInstance();
			const auto layerIndex = firstBatch.subGeometryIndex();

			/* Multi-draw batch with valid MDI program: use vkCmdDrawIndexedIndirect. */
			if ( batch.drawCount > 1 && bdaAddress != 0 )
			{
				const auto program = instance->resolveMDIProgram(renderTarget, layerIndex);

				if ( program != nullptr )
				{
					const auto * geometry = instance->renderable()->geometry(0);
					const auto pipelineLayout = program->pipelineLayout();
					const auto pipelineHandle = program->graphicsPipeline()->handle();

					/* Bind pipeline. */
					if ( tracker.lastPipeline != pipelineHandle )
					{
						commandBuffer.bind(*program->graphicsPipeline());
						tracker.lastPipeline = pipelineHandle;
						tracker.invalidateDescriptorSets();
					}

					/* Viewport. */
					if ( !tracker.viewportSet )
					{
						renderTarget->setViewport(commandBuffer);
						tracker.viewportSet = true;
					}

					/* Geometry. */
					if ( tracker.lastGeometry != static_cast< const void * >(geometry) || tracker.lastLayerIndex != layerIndex )
					{
						commandBuffer.bind(*geometry, layerIndex);
						tracker.lastGeometry = geometry;
						tracker.lastLayerIndex = layerIndex;
					}

					/* Push MDI constants: BDA(8) + VP(64) + frameIndex(4) = 76 bytes. */
					{
						const auto & viewMatrix = renderTarget->viewMatrices().viewMatrix(readStateIndex, false, 0);
						const auto & projectionMatrix = renderTarget->viewMatrices().projectionMatrix(readStateIndex);
						const auto viewProjectionMatrix = projectionMatrix * viewMatrix;

						const auto bdaLo = static_cast< uint32_t >(bdaAddress & 0xFFFFFFFFUL);
						const auto bdaHi = static_cast< uint32_t >((bdaAddress >> 32) & 0xFFFFFFFFUL);

						std::array< float, 19 > buffer{};
						std::memcpy(&buffer[0], &bdaLo, sizeof(uint32_t));
						std::memcpy(&buffer[1], &bdaHi, sizeof(uint32_t));
						std::memcpy(&buffer[2], viewProjectionMatrix.data(), 64);
						buffer[18] = 0.0F;

						vkCmdPushConstants(commandBuffer.handle(), pipelineLayout->handle(), VK_SHADER_STAGE_VERTEX_BIT, 0, 76, buffer.data());
					}

					/* Descriptor sets. */
					uint32_t setOffset = 0;

					const auto * viewDS = renderTarget->viewMatrices().descriptorSet();

					if ( tracker.lastViewDS != viewDS->handle() )
					{
						commandBuffer.bind(*viewDS, *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setOffset);
						tracker.lastViewDS = viewDS->handle();
					}

					setOffset++;

					const auto * material = instance->renderable()->material(layerIndex);

					if ( material != nullptr )
					{
						const auto * materialDS = material->descriptorSet();

						if ( tracker.lastMaterialDS != materialDS->handle() )
						{
							commandBuffer.bind(*materialDS, *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setOffset);
							tracker.lastMaterialDS = materialDS->handle();
						}

						setOffset++;
					}

					/* THE indirect draw call — N objects in one Vulkan command. */
					commandBuffer.drawIndexedIndirect(
						m_indirectBuffers[frameIndex]->handle(),
						batch.indirectOffset,
						batch.drawCount,
						sizeof(VkDrawIndexedIndirectCommand)
					);

					continue;
				}
			}

			/* Fallback: render ALL objects in the batch individually. */
			for ( const auto * rb : batch.renderBatches )
			{
				rb->renderableInstance()->render(readStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, rb->subGeometryIndex(), rb->worldCoordinates(), commandBuffer, tracker, rb->LODLevel(), bindlessTexturesManager);
			}
		}
	}

	VkDeviceAddress
	BatchBuilder::perDrawSSBOAddress (uint32_t frameIndex) const noexcept
	{
		if ( !m_isReady || frameIndex >= m_framesInFlight )
		{
			return 0;
		}

		VkBufferDeviceAddressInfo addressInfo{};
		addressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressInfo.pNext = nullptr;
		addressInfo.buffer = m_perDrawSSBOs[frameIndex]->handle();

		return vkGetBufferDeviceAddress(m_device->handle(), &addressInfo);
	}
}