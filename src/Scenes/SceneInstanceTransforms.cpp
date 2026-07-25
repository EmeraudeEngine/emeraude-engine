/*
 * src/Scenes/SceneInstanceTransforms.cpp
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

#include "SceneInstanceTransforms.hpp"

/* STL inclusions. */
#include <cstring>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/DeferredDestructor.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/LayoutManager.hpp"

namespace EmEn::Scenes
{
	using namespace Vulkan;

	SceneInstanceTransforms::SceneInstanceTransforms (const std::shared_ptr< Device > & device, DeferredDestructor * deferredDestructor) noexcept
		: m_device{device},
		m_deferredDestructor{deferredDestructor}
	{

	}

	SceneInstanceTransforms::~SceneInstanceTransforms ()
	{
		/* NOTE: A scene can be destroyed at runtime (scene switch) while frames are
		 * still in flight: route GPU-visible objects through the deferred destructor. */
		if ( m_deferredDestructor != nullptr )
		{
			for ( auto & descriptorSet : m_descriptorSets )
			{
				m_deferredDestructor->retireObject(std::move(descriptorSet));
			}

			for ( auto & buffer : m_buffers )
			{
				m_deferredDestructor->retireObject(std::move(buffer));
			}
		}

		m_descriptorSets.clear();
		m_buffers.clear();
	}

	std::shared_ptr< DescriptorSetLayout >
	SceneInstanceTransforms::getDescriptorSetLayout (LayoutManager & layoutManager) noexcept
	{
		static constexpr auto UUID{"InstanceTransformsSSBO"};

		auto descriptorSetLayout = layoutManager.getDescriptorSetLayout(UUID);

		if ( descriptorSetLayout == nullptr )
		{
			descriptorSetLayout = layoutManager.prepareNewDescriptorSetLayout(UUID);
			descriptorSetLayout->setIdentifier(ClassId, "InstanceTransforms", "DescriptorSetLayout");

			/* Binding 0: InstanceTransforms SSBO (host-visible, staged per frame). */
			descriptorSetLayout->declareStorageBuffer(0, VK_SHADER_STAGE_VERTEX_BIT);

			if ( !layoutManager.createDescriptorSetLayout(descriptorSetLayout) )
			{
				return nullptr;
			}
		}

		return descriptorSetLayout;
	}

	bool
	SceneInstanceTransforms::initializePerFrameBuffers (Graphics::Renderer & renderer) noexcept
	{
		constexpr VkDeviceSize initialBytes = sizeof(Header) + InitialEntryCapacity * sizeof(Entry);

		const auto descriptorSetLayout = SceneInstanceTransforms::getDescriptorSetLayout(renderer.layoutManager());

		if ( descriptorSetLayout == nullptr )
		{
			Tracer::error(ClassId, "Failed to get the instance transforms descriptor set layout, per-instance transforms will be unavailable.");

			return false;
		}

		const auto frameCount = renderer.framesInFlight();

		m_buffers.resize(frameCount);
		m_descriptorSets.resize(frameCount);

		for ( uint32_t index = 0; index < frameCount; ++index )
		{
			m_buffers[index] = std::make_unique< ShaderStorageBufferObject >(m_device, initialBytes);

			if ( !m_buffers[index]->createOnHardware() )
			{
				Tracer::error(ClassId, "Failed to create the instance transforms SSBOs for frames-in-flight, per-instance transforms will be unavailable.");

				m_descriptorSets.clear();
				m_buffers.clear();

				return false;
			}

			/* Allocate and point the frame's descriptor set at the frame's SSBO. */
			m_descriptorSets[index] = std::make_unique< DescriptorSet >(renderer.descriptorPool(), descriptorSetLayout);

			if ( !m_descriptorSets[index]->create() || !this->writeBufferToDescriptorSet(index) )
			{
				Tracer::error(ClassId, "Failed to create the instance transforms descriptor sets for frames-in-flight, per-instance transforms will be unavailable.");

				m_descriptorSets.clear();
				m_buffers.clear();

				return false;
			}
		}

		m_stagedEntries.reserve(InitialEntryCapacity);

		return true;
	}

	bool
	SceneInstanceTransforms::writeBufferToDescriptorSet (uint32_t frameIndex) noexcept
	{
		const VkDescriptorBufferInfo bufferInfo{
			.buffer = m_buffers[frameIndex]->handle(),
			.offset = 0,
			.range = VK_WHOLE_SIZE
		};

		return m_descriptorSets[frameIndex]->writeStorageBuffer(0, bufferInfo);
	}

	bool
	SceneInstanceTransforms::updateVideoMemory () noexcept
	{
		/* NOTE: Inert when the per-frame buffers were never created. */
		if ( m_buffers.empty() )
		{
			return true;
		}

		if ( m_stagedFrameIndex >= m_buffers.size() )
		{
			Tracer::error(ClassId, "The staged frame index is out of the per-frame buffer range !");

			return false;
		}

		auto & buffer = m_buffers[m_stagedFrameIndex];

		const VkDeviceSize requiredBytes = sizeof(Header) + m_stagedEntries.size() * sizeof(Entry);

		/* NOTE: Grow the current frame buffer when the staged range exceeds its capacity.
		 * The previous buffer is retired through the deferred destructor: even though the
		 * frame-in-flight fence guarantees the GPU is done with it at this point, retirement
		 * keeps the destruction path uniform with the scene-switch case. */
		if ( buffer == nullptr || buffer->bytes() < requiredBytes )
		{
			VkDeviceSize newBytes = buffer != nullptr ? buffer->bytes() : sizeof(Header) + InitialEntryCapacity * sizeof(Entry);

			while ( newBytes < requiredBytes )
			{
				newBytes *= 2;
			}

			auto newBuffer = std::make_unique< ShaderStorageBufferObject >(m_device, newBytes);

			if ( !newBuffer->createOnHardware() )
			{
				Tracer::error(ClassId, "Failed to grow the instance transforms SSBO !");

				return false;
			}

			if ( m_deferredDestructor != nullptr )
			{
				m_deferredDestructor->retireObject(std::move(buffer));
			}

			buffer = std::move(newBuffer);

			/* Repoint the frame's descriptor set at the new buffer. Legal here: the
			 * frame-in-flight fence guarantees no in-flight command buffer references it. */
			if ( m_stagedFrameIndex < m_descriptorSets.size() && m_descriptorSets[m_stagedFrameIndex] != nullptr && !this->writeBufferToDescriptorSet(m_stagedFrameIndex) )
			{
				Tracer::error(ClassId, "Unable to rewrite the instance transforms descriptor set after buffer growth !");

				return false;
			}
		}

		auto * destination = buffer->mapMemoryAs< uint8_t >();

		if ( destination == nullptr )
		{
			Tracer::error(ClassId, "Unable to map the instance transforms SSBO !");

			return false;
		}

		std::memcpy(destination, &m_stagedHeader, sizeof(Header));

		if ( !m_stagedEntries.empty() )
		{
			std::memcpy(destination + sizeof(Header), m_stagedEntries.data(), m_stagedEntries.size() * sizeof(Entry));
		}

		buffer->unmapMemory();

		return true;
	}
}
