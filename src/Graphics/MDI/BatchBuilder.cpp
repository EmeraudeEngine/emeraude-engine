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

/* Local inclusions. */
#include "Graphics/Geometry/Interface.hpp"
#include "Graphics/Material/Interface.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Graphics/RenderableInstance/RenderStateTracker.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Tracer.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/IndirectBuffer.hpp"
#include "Vulkan/IndexBufferObject.hpp"
#include "Vulkan/ShaderStorageBufferObject.hpp"
#include "Vulkan/VertexBufferObject.hpp"

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

		const VkDeviceSize perDrawSSBOSize = MaxMDIDraws * sizeof(PerDrawData);
		const VkDeviceSize indirectBufferSize = MaxMDIDraws * sizeof(VkDrawIndexedIndirectCommand);

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

		for ( const auto & [sortKey, renderBatch] : renderList )
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

			const auto * geometry = renderable->geometry();

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
				batch.representativeBatch = &renderBatch;

				m_batches.push_back(batch);

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

			/* Fill indirect draw command. */
			if ( geometry->useIndexBuffer() && geometry->indexBufferObject() != nullptr )
			{
				const auto * ibo = geometry->indexBufferObject();

				indirectCmds[totalDrawIndex].indexCount = ibo->indexCount();
				indirectCmds[totalDrawIndex].instanceCount = 1;
				indirectCmds[totalDrawIndex].firstIndex = 0;
				indirectCmds[totalDrawIndex].vertexOffset = 0;
				indirectCmds[totalDrawIndex].firstInstance = 0;
			}
			else
			{
				/* Non-indexed geometry: use vertexCount as indexCount (fallback). */
				const auto * vbo = geometry->vertexBufferObject();

				indirectCmds[totalDrawIndex].indexCount = vbo != nullptr ? vbo->vertexCount() : 0;
				indirectCmds[totalDrawIndex].instanceCount = 1;
				indirectCmds[totalDrawIndex].firstIndex = 0;
				indirectCmds[totalDrawIndex].vertexOffset = 0;
				indirectCmds[totalDrawIndex].firstInstance = 0;
			}

			/* Increment draw count for current batch. */
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

		/* Use state tracker for the dispatch — MDI batches are already state-sorted,
		 * so the tracker eliminates redundant binds between consecutive single-draw batches
		 * and between the bind-once phase of multi-draw batches. */
		RenderableInstance::RenderStateTracker tracker{};

		for ( const auto & batch : m_batches )
		{
			if ( batch.representativeBatch == nullptr )
			{
				continue;
			}

			const auto & instance = batch.representativeBatch->renderableInstance();
			const auto layerIndex = batch.representativeBatch->subGeometryIndex();

			/* TODO: Remove this forced fallback once MDI indirect draw is validated. */
			if ( batch.drawCount >= 1 )
			{
				/* Fallback to individual tracked render for all batches. */
				instance->render(readStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, layerIndex, batch.representativeBatch->worldCoordinates(), commandBuffer, tracker, bindlessTexturesManager);
			}
			else
			{
				/* Multi-draw: for now, fall back to individual draws per batch entry.
				 * True MDI dispatch (vkCmdDrawIndexedIndirect) requires MDI-enabled shaders
				 * that read modelMatrix from SSBO via gl_DrawID instead of push constants.
				 * This will be activated once shader generation is updated (steps 6-8). */
				/* TODO: Replace with vkCmdDrawIndexedIndirect when MDI shaders are ready. */
				instance->render(readStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, layerIndex, batch.representativeBatch->worldCoordinates(), commandBuffer, tracker, bindlessTexturesManager);
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