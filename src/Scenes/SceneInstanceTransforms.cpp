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

			for ( auto * buffers : {&m_buffers, &m_pathDirectoryBuffers, &m_pathPointBuffers} )
			{
				for ( auto & buffer : *buffers )
				{
					m_deferredDestructor->retireObject(std::move(buffer));
				}
			}
		}

		m_descriptorSets.clear();
		m_buffers.clear();
		m_pathDirectoryBuffers.clear();
		m_pathPointBuffers.clear();
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
			descriptorSetLayout->declareStorageBuffer(0, VK_SHADER_STAGE_VERTEX_BIT | layoutManager.device()->meshShadingStages());
			/* Bindings 1-2: the path directory and points (vertex pulling, Graphics::RenderableInstance::Path). In the
			 * same set on purpose: same per-scene, per-frame lifecycle, same staging in prepareRender(). */
			descriptorSetLayout->declareStorageBuffer(1, VK_SHADER_STAGE_VERTEX_BIT);
			descriptorSetLayout->declareStorageBuffer(2, VK_SHADER_STAGE_VERTEX_BIT);

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
		constexpr VkDeviceSize initialBytes = sizeof(Header) + (InitialEntryCapacity * sizeof(Entry));

		const auto descriptorSetLayout = SceneInstanceTransforms::getDescriptorSetLayout(renderer.layoutManager());

		if ( descriptorSetLayout == nullptr )
		{
			Tracer::error(ClassId, "Failed to get the instance transforms descriptor set layout, per-instance transforms will be unavailable.");

			return false;
		}

		const auto frameCount = renderer.framesInFlight();

		m_buffers.resize(frameCount);
		m_pathDirectoryBuffers.resize(frameCount);
		m_pathPointBuffers.resize(frameCount);
		m_descriptorSets.resize(frameCount);

		for ( uint32_t index = 0; index < frameCount; ++index )
		{
			m_buffers[index] = std::make_unique< ShaderStorageBufferObject >(m_device, initialBytes);
			/* Never empty: the descriptors must stay valid even in a scene without any path. */
			m_pathDirectoryBuffers[index] = std::make_unique< ShaderStorageBufferObject >(m_device, InitialEntryCapacity * sizeof(PathSpan));
			m_pathPointBuffers[index] = std::make_unique< ShaderStorageBufferObject >(m_device, InitialPathPointCapacity * sizeof(PathPoint));

			if ( !m_buffers[index]->createOnHardware() || !m_pathDirectoryBuffers[index]->createOnHardware() || !m_pathPointBuffers[index]->createOnHardware() )
			{
				Tracer::error(ClassId, "Failed to create the instance transforms SSBOs for frames-in-flight, per-instance transforms will be unavailable.");

				m_descriptorSets.clear();
				m_buffers.clear();
				m_pathDirectoryBuffers.clear();
				m_pathPointBuffers.clear();

				return false;
			}

			/* Allocate and point the frame's descriptor set at the frame's SSBO. */
			m_descriptorSets[index] = std::make_unique< DescriptorSet >(renderer.descriptorPool(), descriptorSetLayout);

			if ( !m_descriptorSets[index]->create() || !this->writeBufferToDescriptorSet(index) )
			{
				Tracer::error(ClassId, "Failed to create the instance transforms descriptor sets for frames-in-flight, per-instance transforms will be unavailable.");

				m_descriptorSets.clear();
				m_buffers.clear();
				m_pathDirectoryBuffers.clear();
				m_pathPointBuffers.clear();

				return false;
			}
		}

		m_stagedEntries.reserve(InitialEntryCapacity);
		m_stagedPathPoints.reserve(InitialPathPointCapacity);

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

		const VkDescriptorBufferInfo pathDirectoryInfo{
			.buffer = m_pathDirectoryBuffers[frameIndex]->handle(),
			.offset = 0,
			.range = VK_WHOLE_SIZE
		};

		const VkDescriptorBufferInfo pathPointInfo{
			.buffer = m_pathPointBuffers[frameIndex]->handle(),
			.offset = 0,
			.range = VK_WHOLE_SIZE
		};

		return m_descriptorSets[frameIndex]->writeStorageBuffer(0, bufferInfo) &&
			m_descriptorSets[frameIndex]->writeStorageBuffer(1, pathDirectoryInfo) &&
			m_descriptorSets[frameIndex]->writeStorageBuffer(2, pathPointInfo);
	}

	void
	SceneInstanceTransforms::stagePath (uint32_t slot, std::span< const Base::Math::Vector< 4, float > > current, std::span< const Base::Math::Vector< 4, float > > previous) noexcept
	{
		/* The directory is indexed by the entry slot: any slot below it that is not a path holds zeros. ⚠️ A path with no
		 * point still gets its (empty) entry: its vertex stage reads pathSpans[slot] whatever, and a slot past the end of
		 * the directory would be an out-of-bounds read. */
		if ( m_stagedPathDirectory.size() <= slot )
		{
			m_stagedPathDirectory.resize(slot + 1, PathSpan{.firstPoint = 0, .pointCount = 0, .reserved0 = 0, .reserved1 = 0});
		}

		m_stagedPathDirectory[slot] = PathSpan{.firstPoint = static_cast< uint32_t >(m_stagedPathPoints.size()), .pointCount = static_cast< uint32_t >(current.size()), .reserved0 = 0, .reserved1 = 0};

		const bool paired = previous.size() == current.size();

		for ( size_t index = 0; index < current.size(); ++index )
		{
			m_stagedPathPoints.push_back({current[index], paired ? previous[index] : current[index]});
		}
	}

	void
	SceneInstanceTransforms::stageDebugPath (const Base::Math::Matrix< 4, float > & modelMatrix, std::span< const Base::Math::Vector< 4, float > > points, const Base::Math::Vector< 4, float > & color, const Base::Math::Vector< 4, float > & style) noexcept
	{
		if ( points.size() < 2 )
		{
			return;
		}

		m_stagedDebugPaths.push_back({static_cast< uint32_t >(m_stagedDebugPoints.size()), static_cast< uint32_t >(points.size()), color, style});

		for ( const auto & point : points )
		{
			const auto world = modelMatrix * Base::Math::Vector< 4, float >{point[Base::Math::X], point[Base::Math::Y], point[Base::Math::Z], 1.0F};

			m_stagedDebugPoints.emplace_back(world[Base::Math::X], world[Base::Math::Y], world[Base::Math::Z], point[Base::Math::W]);
		}
	}

	bool
	SceneInstanceTransforms::ensureCapacity (std::unique_ptr< ShaderStorageBufferObject > & buffer, VkDeviceSize requiredBytes, VkDeviceSize initialBytes, bool & grown) noexcept
	{
		if ( buffer != nullptr && buffer->bytes() >= requiredBytes )
		{
			return true;
		}

		VkDeviceSize newBytes = buffer != nullptr ? buffer->bytes() : initialBytes;

		while ( newBytes < requiredBytes )
		{
			newBytes *= 2;
		}

		auto newBuffer = std::make_unique< ShaderStorageBufferObject >(m_device, newBytes);

		if ( !newBuffer->createOnHardware() )
		{
			return false;
		}

		/* Even though the frame-in-flight fence guarantees the GPU is done with it at this point, retirement keeps the
		 * destruction path uniform with the scene-switch case. */
		if ( m_deferredDestructor != nullptr )
		{
			m_deferredDestructor->retireObject(std::move(buffer));
		}

		buffer = std::move(newBuffer);
		grown = true;

		return true;
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
		auto & pathDirectoryBuffer = m_pathDirectoryBuffers[m_stagedFrameIndex];
		auto & pathPointBuffer = m_pathPointBuffers[m_stagedFrameIndex];

		const VkDeviceSize requiredBytes = sizeof(Header) + (m_stagedEntries.size() * sizeof(Entry));

		/* NOTE: Grow the current frame buffers when the staged ranges exceed their capacity, then repoint the frame's
		 * descriptor set at the new ones. Legal here: the frame-in-flight fence guarantees no in-flight command buffer
		 * references them. */
		bool grown = false;

		if ( !this->ensureCapacity(buffer, requiredBytes, sizeof(Header) + (InitialEntryCapacity * sizeof(Entry)), grown) ||
			 !this->ensureCapacity(pathDirectoryBuffer, m_stagedPathDirectory.size() * sizeof(PathSpan), InitialEntryCapacity * sizeof(PathSpan), grown) ||
			 !this->ensureCapacity(pathPointBuffer, m_stagedPathPoints.size() * sizeof(PathPoint), InitialPathPointCapacity * sizeof(PathPoint), grown) )
		{
			Tracer::error(ClassId, "Failed to grow the instance transforms SSBOs !");

			return false;
		}

		if ( grown && m_stagedFrameIndex < m_descriptorSets.size() && m_descriptorSets[m_stagedFrameIndex] != nullptr && !this->writeBufferToDescriptorSet(m_stagedFrameIndex) )
		{
			Tracer::error(ClassId, "Unable to rewrite the instance transforms descriptor set after buffer growth !");

			return false;
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

		/* The paths: only what was staged (a scene without any path uploads nothing more). */
		const auto upload = [] (ShaderStorageBufferObject & destinationBuffer, const void * source, size_t bytes) noexcept {
			if ( bytes == 0 )
			{
				return true;
			}

			auto * mapped = destinationBuffer.mapMemoryAs< uint8_t >();

			if ( mapped == nullptr )
			{
				return false;
			}

			std::memcpy(mapped, source, bytes);
			destinationBuffer.unmapMemory();

			return true;
		};

		if ( !upload(*pathDirectoryBuffer, m_stagedPathDirectory.data(), m_stagedPathDirectory.size() * sizeof(PathSpan)) ||
			!upload(*pathPointBuffer, m_stagedPathPoints.data(), m_stagedPathPoints.size() * sizeof(PathPoint)) )
		{
			Tracer::error(ClassId, "Unable to map the path SSBOs !");

			return false;
		}

		return true;
	}
}
