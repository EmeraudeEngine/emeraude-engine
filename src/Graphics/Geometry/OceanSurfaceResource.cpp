/*
 * src/Graphics/Geometry/OceanSurfaceResource.cpp
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

#include "OceanSurfaceResource.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <limits>

/* Local inclusions. */
#include "Graphics/Frustum.hpp"
#include "Graphics/Renderer.hpp"
#include "Vulkan/DeferredDestructor.hpp"
#include "Saphir/Generator/OceanSurfaceHelper.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/IndexBufferObject.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/TransferManager.hpp"
#include "Vulkan/UniformBufferObject.hpp"
#include "Vulkan/VertexBufferObject.hpp"

namespace EmEn::Graphics::Geometry
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	namespace
	{
		/** @brief The animation clock wraps after an hour: the phase ω t stays well inside float precision. */
		constexpr double ClockWrapSeconds{3600.0};
	}

	OceanSurfaceResource::OceanSurfaceResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name) noexcept
		: Interface{serviceProvider, name, EnableHeightfieldSurface | EnableOceanSurface}
	{

	}

	OceanSurfaceResource::~OceanSurfaceResource ()
	{
		this->destroyFromHardware(true);
	}

	size_t
	OceanSurfaceResource::classUID () const noexcept
	{
		return getClassUID();
	}

	bool
	OceanSurfaceResource::is (size_t classUID) const noexcept
	{
		return classUID == getClassUID();
	}

	const char *
	OceanSurfaceResource::classLabel () const noexcept
	{
		return ClassId;
	}

	bool
	OceanSurfaceResource::load () noexcept
	{
		return this->load(OceanSurfaceParameters{});
	}

	bool
	OceanSurfaceResource::load (const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "This resource is not intended to be loaded by a JSON file!");

		return this->setLoadSuccess(false);
	}

	bool
	OceanSurfaceResource::load (const OceanSurfaceParameters & parameters) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( parameters.patchQuads < 2 || parameters.patchQuads % 2 != 0 || parameters.levelCount == 0 || parameters.levelCount > OceanSurface::MaxLevelsOfDetail || parameters.rootsPerSide == 0 || parameters.finestCell <= 0.0F )
		{
			TraceError{ClassId} << "Ocean '" << this->name() << "': invalid parameters (even patch quads, 1-" << OceanSurface::MaxLevelsOfDetail << " levels, a positive cell) !";

			return this->setLoadSuccess(false);
		}

		m_parameters = parameters;

		/* The level ranges and the geomorph table, as the terrain's (CDLODTerrainResource::load()). */
		const auto ratio = std::clamp(m_parameters.morphStartRatio, 0.0F, 0.95F);

		for ( uint32_t level = 0; level < OceanSurface::MaxLevelsOfDetail; ++level )
		{
			m_lodRanges[level] = m_parameters.detailDistance * static_cast< float >(1U << std::min(level, 30U));
		}

		m_lodRanges[m_parameters.levelCount - 1] = std::numeric_limits< float >::max();

		for ( uint32_t level = 0; level < OceanSurface::MaxLevelsOfDetail; ++level )
		{
			if ( level + 1 >= m_parameters.levelCount )
			{
				m_morphTable[level] = {1.0e30F, 0.0F};

				continue;
			}

			const auto previous = level == 0 ? 0.0F : m_lodRanges[level - 1];
			const auto morphStart = previous + ((m_lodRanges[level] - previous) * ratio);
			const auto morphEnd = m_lodRanges[level] - ((m_lodRanges[level] - morphStart) * 0.01F);

			m_morphTable[level] = {morphStart, 1.0F / (morphEnd - morphStart)};
		}

		/* The sea may be anywhere under the camera: the box spans the root square at its widest. */
		const auto rootSize = static_cast< float >(m_parameters.patchQuads) * m_parameters.finestCell * static_cast< float >(1U << (m_parameters.levelCount - 1));
		const auto halfExtent = rootSize * static_cast< float >(m_parameters.rootsPerSide) * 0.5F;

		m_boundingBox.set({halfExtent, m_parameters.seaLevel + m_parameters.waveReach, halfExtent}, {-halfExtent, m_parameters.seaLevel - m_parameters.waveReach, -halfExtent});
		m_boundingSphere.setRadius(m_boundingBox.highestLength() * 0.5F);

		return this->setLoadSuccess(true);
	}

	size_t
	OceanSurfaceResource::memoryOccupied () const noexcept
	{
		size_t bytes = 0;

		if ( m_vertexBufferObject != nullptr )
		{
			bytes += m_vertexBufferObject->bytes();
		}

		if ( m_indexBufferObject != nullptr )
		{
			bytes += m_indexBufferObject->bytes();
		}

		return bytes;
	}

	bool
	OceanSurfaceResource::isCreated () const noexcept
	{
		return m_vertexBufferObject != nullptr && m_vertexBufferObject->isCreated() && m_indexBufferObject != nullptr && m_indexBufferObject->isCreated() && !m_descriptorSets.empty();
	}

	Topology
	OceanSurfaceResource::topology () const noexcept
	{
		return Topology::TriangleList;
	}

	uint32_t
	OceanSurfaceResource::subGeometryCount () const noexcept
	{
		return 1;
	}

	std::array< uint32_t, 2 >
	OceanSurfaceResource::subGeometryRange (uint32_t /*subGeometryIndex*/) const noexcept
	{
		return {0, m_patchIndexCount};
	}

	const Space3D::AACuboid< float > &
	OceanSurfaceResource::boundingBox () const noexcept
	{
		return m_boundingBox;
	}

	const Space3D::Sphere< float > &
	OceanSurfaceResource::boundingSphere () const noexcept
	{
		return m_boundingSphere;
	}

	const VertexBufferObject *
	OceanSurfaceResource::vertexBufferObject () const noexcept
	{
		return m_vertexBufferObject.get();
	}

	const IndexBufferObject *
	OceanSurfaceResource::indexBufferObject () const noexcept
	{
		return m_indexBufferObject.get();
	}

	bool
	OceanSurfaceResource::useIndexBuffer () const noexcept
	{
		return true;
	}

	bool
	OceanSurfaceResource::isAdaptiveLOD () const noexcept
	{
		return true;
	}

	uint32_t
	OceanSurfaceResource::getAdaptiveDrawCallCount () const noexcept
	{
		return static_cast< uint32_t >(m_selection.size());
	}

	std::array< uint32_t, 2 >
	OceanSurfaceResource::getAdaptiveDrawCallRange (uint32_t drawCallIndex) const noexcept
	{
		return m_selection[drawCallIndex].range;
	}

	std::array< float, 4 >
	OceanSurfaceResource::getAdaptiveDrawCallConstants (uint32_t drawCallIndex) const noexcept
	{
		return m_selection[drawCallIndex].node;
	}

	bool
	OceanSurfaceResource::createOnHardware (TransferManager & transferManager) noexcept
	{
		if ( this->isCreated() )
		{
			Tracer::warning(ClassId, "The ocean is already in video memory !");

			return true;
		}

		const auto & device = transferManager.device();
		const auto patchQuads = m_parameters.patchQuads;
		const auto patchPoints = patchQuads + 1;

		/* The shared patch: integer grid coordinates in X and Z, the vertex stage does the rest (as the terrain's). */
		{
			std::vector< float > positions;
			positions.reserve(static_cast< size_t >(patchPoints) * patchPoints * 3);

			for ( uint32_t z = 0; z < patchPoints; ++z )
			{
				for ( uint32_t x = 0; x < patchPoints; ++x )
				{
					positions.push_back(static_cast< float >(x));
					positions.push_back(0.0F);
					positions.push_back(static_cast< float >(z));
				}
			}

			m_vertexBufferObject = std::make_unique< VertexBufferObject >(device, patchPoints * patchPoints, getElementCountFromFlags(this->flags()), false);
			m_vertexBufferObject->setIdentifier(ClassId, this->name(), "PatchVertexBufferObject");

			if ( !m_vertexBufferObject->createOnHardware() || !m_vertexBufferObject->transferData(transferManager, positions) )
			{
				Tracer::error(ClassId, "Unable to create the patch vertex buffer object (VBO) !");

				m_vertexBufferObject.reset();

				return false;
			}
		}

		/* The patch triangles sorted by QUARTER, split along one diagonal (the geomorph collapses the odd vertices
		 * onto the even ones), as the terrain's. */
		{
			std::vector< uint32_t > indices;
			indices.reserve(static_cast< size_t >(patchQuads) * patchQuads * 6);

			const auto half = patchQuads / 2;
			const auto index = [patchPoints] (uint32_t x, uint32_t z) {
				return (z * patchPoints) + x;
			};

			for ( uint32_t quarter = 0; quarter < 4; ++quarter )
			{
				const auto startX = (quarter % 2) * half;
				const auto startZ = (quarter / 2) * half;

				for ( uint32_t z = startZ; z < startZ + half; ++z )
				{
					for ( uint32_t x = startX; x < startX + half; ++x )
					{
						indices.push_back(index(x, z));
						indices.push_back(index(x, z + 1));
						indices.push_back(index(x + 1, z + 1));

						indices.push_back(index(x, z));
						indices.push_back(index(x + 1, z + 1));
						indices.push_back(index(x + 1, z));
					}
				}
			}

			m_patchIndexCount = static_cast< uint32_t >(indices.size());
			m_indexBufferObject = std::make_unique< IndexBufferObject >(device, m_patchIndexCount);
			m_indexBufferObject->setIdentifier(ClassId, this->name(), "PatchIndexBufferObject");

			if ( !m_indexBufferObject->createOnHardware() || !m_indexBufferObject->transferData(transferManager, indices) )
			{
				Tracer::error(ClassId, "Unable to create the patch index buffer object (IBO) !");

				m_vertexBufferObject.reset();
				m_indexBufferObject.reset();

				return false;
			}
		}

		if ( !this->createSurfaceResources() )
		{
			this->destroyFromHardware(false);

			return false;
		}

		m_startTime = std::chrono::steady_clock::now();

		/* The renderer runs the FFT every frame from now on. */
		this->serviceProvider().graphicsRenderer().registerSurfaceGeometry(std::static_pointer_cast< Interface >(this->shared_from_this()));

		return true;
	}

	bool
	OceanSurfaceResource::createSurfaceResources () noexcept
	{
		auto & renderer = this->serviceProvider().graphicsRenderer();
		const auto & device = renderer.device();
		const auto framesInFlight = std::max(1U, renderer.framesInFlight());

		m_waves = std::make_unique< OceanWaves >(device, renderer.shaderManager());

		if ( !m_waves->create(m_parameters.waves) )
		{
			Tracer::error(ClassId, "Unable to create the FFT wave cascades !");

			return false;
		}

		/* REPEAT: a cascade tiles the whole sea. Linear, no mipmap (the far pixels fade the small cascades instead). */
		{
			VkSamplerCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.maxAnisotropy = 1.0F;
			createInfo.maxLod = 0.0F;
			createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

			m_sampler = std::make_shared< Sampler >(device, createInfo);
			m_sampler->setIdentifier(ClassId, this->name(), "CascadeSampler");

			if ( !m_sampler->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the cascade sampler !");

				return false;
			}
		}

		/* One uniform section per frame in flight. */
		{
			const auto alignment = std::max< VkDeviceSize >(device->physicalDevice()->propertiesVK10().limits.minUniformBufferOffsetAlignment, 1);

			m_uniformSectionSize = ((sizeof(OceanSurface::Uniforms) + alignment - 1) / alignment) * alignment;
			m_uniformBuffer = std::make_unique< UniformBufferObject >(device, m_uniformSectionSize * framesInFlight);
			m_uniformBuffer->setIdentifier(ClassId, this->name(), "SurfaceUniforms");

			if ( !m_uniformBuffer->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the surface uniform buffer !");

				return false;
			}
		}

		m_descriptorPool = std::make_shared< DescriptorPool >(
			device,
			std::vector< VkDescriptorPoolSize >{
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4U * framesInFlight},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framesInFlight}
			},
			framesInFlight,
			VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
		);
		m_descriptorPool->setIdentifier(ClassId, this->name(), "DescriptorPool");

		if ( !m_descriptorPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the surface descriptor pool !");

			return false;
		}

		/* Displacement, slopes, uniforms, whitecap foam and previous displacement (Geometry::OceanSurface). */
		const auto surfaceLayout = Saphir::Generator::getOceanSurfaceDescriptorSetLayout(renderer.layoutManager());

		if ( surfaceLayout == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the surface descriptor set layout !");

			return false;
		}

		m_descriptorSets.clear();

		for ( uint32_t frame = 0; frame < framesInFlight; ++frame )
		{
			auto descriptorSet = std::make_unique< DescriptorSet >(m_descriptorPool, surfaceLayout);

			const VkDescriptorBufferInfo uniforms{
				.buffer = m_uniformBuffer->handle(),
				.offset = m_uniformSectionSize * frame,
				.range = sizeof(OceanSurface::Uniforms)
			};

			if (
				!descriptorSet->create() ||
				!descriptorSet->writeCombinedImageSampler(OceanSurface::DisplacementBinding, *m_waves->displacementView(), *m_sampler, VK_IMAGE_LAYOUT_GENERAL) ||
				!descriptorSet->writeCombinedImageSampler(OceanSurface::SlopesBinding, *m_waves->slopeView(), *m_sampler, VK_IMAGE_LAYOUT_GENERAL) ||
				!descriptorSet->writeCombinedImageSampler(OceanSurface::FoamBinding, *m_waves->foamView(), *m_sampler, VK_IMAGE_LAYOUT_GENERAL) ||
				!descriptorSet->writeCombinedImageSampler(OceanSurface::PreviousDisplacementBinding, *m_waves->previousDisplacementView(), *m_sampler, VK_IMAGE_LAYOUT_GENERAL) ||
				!descriptorSet->writeUniformBuffer(OceanSurface::UniformsBinding, uniforms)
			)
			{
				Tracer::error(ClassId, "Unable to create a surface descriptor set !");

				m_descriptorSets.clear();

				return false;
			}

			m_descriptorSets.emplace_back(std::move(descriptorSet));
		}

		/* The per-frame command buffers: graphics queue, so queue order puts the FFT ahead of every pass of the frame. */
		m_commandPool = std::make_shared< CommandPool >(device, device->getGraphicsFamilyIndex(), false, true, false);
		m_commandPool->setIdentifier(ClassId, this->name(), "CommandPool");

		if ( !m_commandPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the FFT command pool !");

			return false;
		}

		m_commandBuffers.clear();

		for ( uint32_t frame = 0; frame < framesInFlight; ++frame )
		{
			auto commandBuffer = std::make_shared< CommandBuffer >(m_commandPool, true);

			if ( !commandBuffer->isCreated() )
			{
				Tracer::error(ClassId, "Unable to create an FFT command buffer !");

				return false;
			}

			m_commandBuffers.emplace_back(std::move(commandBuffer));
		}

		return true;
	}

	void
	OceanSurfaceResource::destroyFromHardware (bool clearLocalData) noexcept
	{
		/* NOTE: The FFT is submitted every frame (Renderer::updateSurfaceGeometries()) and the surface is drawn by the
		 * frames in flight, with no fence of its own: its GPU objects are RETIRED, destroyed once those frames are done,
		 * never freed here. Measured 2026-09-30: an inactive scene's ocean destroyed while another scene rendered —
		 * 20 to 31 "in use" VUIDs on Linux, macOS and Windows (item scene-destroyed-outside-manager-without-gpu-drain).
		 * FIFO order: the command buffers leave before their pool, the descriptor sets before theirs. */
		auto & deferredDestructor = this->serviceProvider().graphicsRenderer().deferredDestructor();

		for ( auto & commandBuffer : m_commandBuffers )
		{
			deferredDestructor.retireObject(std::move(commandBuffer));
		}

		m_commandBuffers.clear();
		deferredDestructor.retireObject(std::move(m_commandPool));

		for ( auto & descriptorSet : m_descriptorSets )
		{
			deferredDestructor.retireObject(std::move(descriptorSet));
		}

		m_descriptorSets.clear();
		deferredDestructor.retireObject(std::move(m_descriptorPool));
		deferredDestructor.retireObject(std::move(m_uniformBuffer));
		deferredDestructor.retireObject(std::move(m_sampler));
		deferredDestructor.retireObject(std::move(m_waves));
		deferredDestructor.retireObject(std::move(m_indexBufferObject));
		deferredDestructor.retireObject(std::move(m_vertexBufferObject));
		m_surfaceUpdated = false;
		m_previousTime = -1.0;

		if ( clearLocalData )
		{
			m_selection.clear();
		}
	}

	bool
	OceanSurfaceResource::updateVideoMemory () noexcept
	{
		return this->isCreated();
	}

	const DescriptorSet *
	OceanSurfaceResource::surfaceDescriptorSet () const noexcept
	{
		/* Nothing is sampled before the first FFT: the draw skips the frame. */
		if ( !m_surfaceUpdated || m_descriptorSets.empty() )
		{
			return nullptr;
		}

		return m_descriptorSets[m_currentFrameIndex].get();
	}

	bool
	OceanSurfaceResource::updateSurfaceVideoMemory (const Vector< 3, float > & /*lodViewPosition*/, uint32_t frameIndex) noexcept
	{
		if ( m_descriptorSets.empty() || m_waves == nullptr )
		{
			return false;
		}

		frameIndex %= static_cast< uint32_t >(m_descriptorSets.size());
		m_currentFrameIndex = frameIndex;

		/* The uniforms of this frame. */
		{
			OceanSurface::Uniforms uniforms{};
			uniforms.grid = {m_parameters.finestCell, 0.0F, static_cast< float >(m_parameters.levelCount), static_cast< float >(m_parameters.patchQuads)};
			uniforms.level = {m_parameters.seaLevel, 0.0F, 0.0F, 0.0F};
			uniforms.cascades = {m_parameters.waves.cascadeSizes[0], m_parameters.waves.cascadeSizes[1], m_parameters.waves.cascadeSizes[2], static_cast< float >(OceanWaves::CascadeCount)};
			uniforms.textureCoordinates = {m_parameters.textureRepeatsPerMetre, m_parameters.textureRepeatsPerMetre, 0.0F, 0.0F};

			for ( uint32_t level = 0; level < OceanSurface::MaxLevelsOfDetail; ++level )
			{
				uniforms.levelsOfDetail[level] = {m_morphTable[level][0], m_morphTable[level][1], m_lodRanges[level], 0.0F};
			}

			if ( !m_uniformBuffer->writeData(MemoryRegion{&uniforms, sizeof(uniforms), static_cast< size_t >(m_uniformSectionSize * frameIndex)}) )
			{
				Tracer::error(ClassId, "Unable to write the surface uniforms !");

				return false;
			}
		}

		/* The FFT of this frame's time. */
		const auto elapsed = std::chrono::duration< double >(std::chrono::steady_clock::now() - m_startTime).count();
		const auto time = static_cast< float >(std::fmod(elapsed, ClockWrapSeconds));
		const auto & commandBuffer = m_commandBuffers[frameIndex];

		if ( !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			Tracer::error(ClassId, "Unable to begin the FFT command buffer !");

			return false;
		}

		/* The whitecaps decay over the real step: none on the first update, and none across the clock wrap. */
		const auto deltaTime = m_previousTime >= 0.0 && elapsed >= m_previousTime ? static_cast< float >(std::min(elapsed - m_previousTime, 0.25)) : 0.0F;

		m_previousTime = elapsed;

		m_waves->recordUpdate(*commandBuffer, time, deltaTime);

		if ( !commandBuffer->end() )
		{
			Tracer::error(ClassId, "Unable to end the FFT command buffer !");

			return false;
		}

		/* No semaphore, no fence: queue order runs this before every pass of the frame, the frame's own fence covers it,
		 * and recordUpdate()'s first barrier waits for the previous frame's readers (as the terrain's clipmap). */
		if ( !this->serviceProvider().graphicsRenderer().graphicsQueue()->submit(*commandBuffer) )
		{
			Tracer::error(ClassId, "Unable to submit the FFT !");

			return false;
		}

		m_surfaceUpdated = true;

		return true;
	}

	bool
	OceanSurfaceResource::selectNode (uint32_t levelOfDetail, double originX, double originZ, const Vector< 3, float > & eye, const Frustum * frustum) const noexcept
	{
		const auto size = static_cast< double >(m_parameters.patchQuads) * static_cast< double >(m_parameters.finestCell) * static_cast< double >(1U << levelOfDetail);
		const Space3D::AACuboid< float > box{
			{static_cast< float >(originX + size), m_parameters.seaLevel + m_parameters.waveReach, static_cast< float >(originZ + size)},
			{static_cast< float >(originX), m_parameters.seaLevel - m_parameters.waveReach, static_cast< float >(originZ)}
		};
		const auto distance = box.distanceTo(eye);

		/* Beyond this level's range: the parent draws this area, coarser. */
		if ( distance > m_lodRanges[levelOfDetail] )
		{
			return false;
		}

		/* Culled: handled, nothing to draw. */
		if ( frustum != nullptr && !frustum->isSeeing(box) )
		{
			return true;
		}

		const std::array< float, 4 > node{static_cast< float >(originX), static_cast< float >(originZ), static_cast< float >(size), static_cast< float >(levelOfDetail)};

		if ( levelOfDetail == 0 || distance > m_lodRanges[levelOfDetail - 1] )
		{
			m_selection.push_back({{0, m_patchIndexCount}, node});

			return true;
		}

		/* Some part is close enough for the finer level: the children decide, and THIS level draws the quarters none of
		 * them took (the patch index buffer is sorted by quarter). */
		const auto quarterIndexCount = m_patchIndexCount / 4U;
		const auto half = size * 0.5;

		for ( uint32_t quarter = 0; quarter < 4; ++quarter )
		{
			const auto childX = originX + (static_cast< double >(quarter % 2U) * half);
			const auto childZ = originZ + (static_cast< double >(quarter / 2U) * half);

			if ( !this->selectNode(levelOfDetail - 1, childX, childZ, eye, frustum) )
			{
				m_selection.push_back({{quarter * quarterIndexCount, quarterIndexCount}, node});
			}
		}

		return true;
	}

	void
	OceanSurfaceResource::prepareAdaptiveRendering (const Vector< 3, float > & lodViewPosition, const Frustum * cullingFrustum, const CartesianFrame< float > * /*worldCoordinates*/) const noexcept
	{
		m_selection.clear();

		if ( m_patchIndexCount == 0 )
		{
			return;
		}

		/* A square of WORLD-ALIGNED root nodes around the camera: a node never moves in the world, so nothing swims; the
		 * square follows the camera by whole roots, far beyond the finest levels. Object space IS the world. */
		const auto top = m_parameters.levelCount - 1;
		const auto rootSize = static_cast< double >(m_parameters.patchQuads) * static_cast< double >(m_parameters.finestCell) * static_cast< double >(1U << top);
		const auto half = static_cast< int64_t >(m_parameters.rootsPerSide / 2);
		const auto centerX = static_cast< int64_t >(std::floor((static_cast< double >(lodViewPosition[X]) / rootSize) + 0.5));
		const auto centerZ = static_cast< int64_t >(std::floor((static_cast< double >(lodViewPosition[Z]) / rootSize) + 0.5));

		for ( int64_t rootZ = centerZ - half; rootZ < centerZ + half; ++rootZ )
		{
			for ( int64_t rootX = centerX - half; rootX < centerX + half; ++rootX )
			{
				this->selectNode(top, static_cast< double >(rootX) * rootSize, static_cast< double >(rootZ) * rootSize, lodViewPosition, cullingFrustum);
			}
		}
	}
}
