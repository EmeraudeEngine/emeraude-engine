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
#include <utility>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/Frustum.hpp"
#include "Graphics/Geometry/Interface.hpp"
#include "Graphics/Geometry/OceanSurface.hpp"
#include "Graphics/Geometry/Types.hpp"
#include "Graphics/ImageViewTexture.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/SeaShelterMap.hpp"
#include "Graphics/Types.hpp"
#include "Saphir/Generator/OceanSurfaceHelper.hpp"
#include "Tracer.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/DeferredDestructor.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/MemoryRegion.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/TextureInterface.hpp"
#include "Vulkan/TransferManager.hpp"
#include "Vulkan/UniformBufferObject.hpp"

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

		if ( parameters.shelterMapSide == 0 || parameters.shelterMapSide > OceanSurface::MaxShelterMapSide )
		{
			TraceError{ClassId} << "Ocean '" << this->name() << "': a shelter map takes 1 to " << OceanSurface::MaxShelterMapSide << " texels a side, not " << parameters.shelterMapSide << " !";

			return this->setLoadSuccess(false);
		}

		m_parameters = parameters;

		this->setShelterTuning({.minimumPassage = parameters.shelterMinimumPassage, .fade = parameters.shelterFade, .waveShares = parameters.shelteredWaveShares});
		this->setCausticTuning({.strength = parameters.causticStrength, .softening = parameters.causticSoftening, .fadeDepth = parameters.causticFadeDepth});

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

		if ( !this->createShelterResources() )
		{
			return false;
		}

		m_shelterStaging.clear();
		m_shelterStaging.resize(framesInFlight);
		m_uploadedShelterGeneration = 0;
		m_shelterRectangle = {};

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
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5U * framesInFlight},
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

		/* Displacement, slopes, uniforms, whitecap foam, previous displacement and shelter map (Geometry::OceanSurface). */
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
				!descriptorSet->writeCombinedImageSampler(OceanSurface::ShelterBinding, *m_shelterView, *m_shelterSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ||
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

		/* The curvature cascades are low-passed by their mips: REPEAT, trilinear. */
		{
			VkSamplerCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.maxAnisotropy = 1.0F;
			createInfo.maxLod = static_cast< float >(OceanWaves::CurvatureMipLevels - 1U);
			createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

			m_curvatureSampler = std::make_shared< Sampler >(device, createInfo);
			m_curvatureSampler->setIdentifier(ClassId, this->name(), "CurvatureSampler");

			if ( !m_curvatureSampler->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the curvature sampler !");

				return false;
			}
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

		/* What the underwater view binds to stand where this mesh does: the very resources this surface's stages read
		 * (the uniforms' offset is each frame's, updateSurfaceVideoMemory()). */
		m_surfaceWaves = {};
		m_surfaceWaves.displacement = m_waves->displacementView();
		m_surfaceWaves.sampler = m_sampler.get();
		m_surfaceWaves.shelter = m_shelterView.get();
		m_surfaceWaves.shelterSampler = m_shelterSampler.get();
		m_surfaceWaves.uniforms = m_uniformBuffer.get();
		m_surfaceWaves.uniformsRange = sizeof(OceanSurface::Uniforms);
		m_surfaceWaves.reach = m_parameters.waveReach;

		/* What a light draws this surface's caustics with: each cascade's curvature and the shelter map, as textures a
		 * table can hold (they share the objects above; nothing is created). */
		m_caustics = {};

		{
			uint32_t cascade = 0;

			for ( auto & curvature : m_caustics.curvature )
			{
				curvature = std::make_shared< ImageViewTexture >(m_waves->curvatureImage(), m_waves->curvatureView(cascade), m_curvatureSampler);

				++cascade;
			}

			cascade = 0;

			for ( auto & displacement : m_caustics.displacement )
			{
				displacement = std::make_shared< ImageViewTexture >(m_waves->displacementImage(), m_waves->displacementLayerView(cascade), m_sampler);

				++cascade;
			}

			static_assert(SeaCaustics::CascadeCount == OceanWaves::CascadeCount, "A sea's caustics are made of the ocean's cascades.");

			m_caustics.cascadeSizes = m_parameters.waves.cascadeSizes;
		}

		m_caustics.reach = std::isfinite(m_parameters.waveReach) ? std::max(m_parameters.waveReach, 0.0F) : 0.0F;

		m_caustics.shelter = std::make_shared< ImageViewTexture >(m_shelterImage, m_shelterView, m_shelterSampler);

		return true;
	}

	bool
	OceanSurfaceResource::createShelterResources () noexcept
	{
		const auto & device = this->serviceProvider().graphicsRenderer().device();
		const auto side = m_parameters.shelterMapSide;

		/* NOTE: R8_UNORM is sampled, filtered and written by a transfer on every Vulkan device (a mandatory format). */
		m_shelterImage = std::make_shared< Image >(device, VK_IMAGE_TYPE_2D, VK_FORMAT_R8_UNORM, VkExtent3D{side, side, 1}, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		m_shelterImage->setIdentifier(ClassId, this->name(), "ShelterMap");

		if ( !m_shelterImage->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the shelter map !");

			return false;
		}

		m_shelterView = std::make_shared< ImageView >(m_shelterImage, VK_IMAGE_VIEW_TYPE_2D, VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
		m_shelterView->setIdentifier(ClassId, this->name(), "ShelterMapView");

		if ( !m_shelterView->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the shelter map's view !");

			return false;
		}

		/* CLAMP TO EDGE: beyond its square the map's edge holds — the sea around the scene. */
		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.magFilter = VK_FILTER_LINEAR;
		createInfo.minFilter = VK_FILTER_LINEAR;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		createInfo.maxAnisotropy = 1.0F;
		createInfo.maxLod = 0.0F;
		createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		m_shelterSampler = std::make_shared< Sampler >(device, createInfo);
		m_shelterSampler->setIdentifier(ClassId, this->name(), "ShelterMapSampler");

		if ( !m_shelterSampler->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the shelter map's sampler !");

			return false;
		}

		/* What a sampler finds it in: every upload leaves it there (recordShelterUpload()), and the first one runs ahead
		 * of the first frame that reads it. A texture table writes its descriptor with this layout. */
		m_shelterImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		return true;
	}

	void
	OceanSurfaceResource::setShelterMap (std::shared_ptr< const SeaShelterMap > map) noexcept
	{
		if ( map != nullptr )
		{
			const auto side = static_cast< size_t >(m_parameters.shelterMapSide);

			if ( map->side != m_parameters.shelterMapSide || map->texels.size() != side * side || !std::isfinite(map->originX) || !std::isfinite(map->originZ) || !std::isfinite(map->size) || map->size <= 0.0F )
			{
				TraceError{ClassId} <<
					"Ocean surface '" << this->name() << "': a shelter map of " << map->side << " texels a side (" << map->texels.size() << " values) over " << map->size << " m is refused; "
					"this surface takes " << m_parameters.shelterMapSide << " texels a side over a square that has an extent.";

				return;
			}
		}

		const Base::MutexLock lock{m_shelterAccess};

		m_shelterMap = std::move(map);
		++m_shelterGeneration;
	}

	OceanSurfaceResource::ShelterTuning
	OceanSurfaceResource::shelterTuning () const noexcept
	{
		const Base::MutexLock lock{m_shelterAccess};

		return m_shelterTuning;
	}

	void
	OceanSurfaceResource::setShelterTuning (const ShelterTuning & tuning) noexcept
	{
		const Base::MutexLock lock{m_shelterAccess};

		m_shelterTuning = tuning;
	}

	OceanSurfaceResource::CausticTuning
	OceanSurfaceResource::causticTuning () const noexcept
	{
		const Base::MutexLock lock{m_shelterAccess};

		return m_causticTuning;
	}

	void
	OceanSurfaceResource::setCausticTuning (const CausticTuning & tuning) noexcept
	{
		const Base::MutexLock lock{m_shelterAccess};

		m_causticTuning = tuning;
	}

	bool
	OceanSurfaceResource::stageShelterMap (uint32_t frameIndex, const SeaShelterMap * map) noexcept
	{
		const auto side = static_cast< size_t >(m_parameters.shelterMapSide);
		const auto bytes = side * side;
		auto & staging = m_shelterStaging[frameIndex];

		/* This frame's staging buffer was last read by this frame index's previous cycle, which the in-flight fence
		 * already waited for (as the terrain's clipmap). */
		if ( staging == nullptr )
		{
			staging = std::make_unique< Buffer >(this->serviceProvider().graphicsRenderer().device(), 0, bytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
			staging->setIdentifier(ClassId, this->name(), "ShelterMapStaging");

			if ( !staging->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the shelter map's staging buffer !");

				staging.reset();

				return false;
			}
		}

		/* No map: the open sea everywhere, and a rectangle without extent (every read lands on one texel). */
		const std::vector< uint8_t > openSea(map != nullptr ? size_t{0} : bytes, uint8_t{255});
		const auto & texels = map != nullptr ? map->texels : openSea;

		if ( !staging->writeData(MemoryRegion{texels.data(), bytes}) )
		{
			Tracer::error(ClassId, "Unable to write the shelter map's staging buffer !");

			return false;
		}

		m_shelterRectangle = map != nullptr
			? std::array< float, 4 >{map->originX, map->originZ, 1.0F / map->size, 1.0F / map->size}
			: std::array< float, 4 >{};

		return true;
	}

	void
	OceanSurfaceResource::recordShelterUpload (const CommandBuffer & commandBuffer, uint32_t frameIndex) const noexcept
	{
		constexpr VkImageSubresourceRange wholeImage{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
		constexpr VkPipelineStageFlags readers = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

		auto * const handle = commandBuffer.handle();
		const bool first = m_uploadedShelterGeneration == 0;

		/* To transfer. The barrier's first scope is every earlier command of the graphics queue: the frames still in
		 * flight have finished reading the map this frame rewrites. */
		VkImageMemoryBarrier toTransfer{};
		toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		toTransfer.srcAccessMask = first ? 0 : VK_ACCESS_SHADER_READ_BIT;
		toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		toTransfer.oldLayout = first ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		toTransfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toTransfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toTransfer.image = m_shelterImage->handle();
		toTransfer.subresourceRange = wholeImage;

		const VkPipelineStageFlags before = first ? VkPipelineStageFlags{VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT} : readers;

		vkCmdPipelineBarrier(handle, before, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &toTransfer);

		VkBufferImageCopy copy{};
		copy.imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1};
		copy.imageExtent = {.width = m_parameters.shelterMapSide, .height = m_parameters.shelterMapSide, .depth = 1};

		vkCmdCopyBufferToImage(handle, m_shelterStaging[frameIndex]->handle(), m_shelterImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		VkImageMemoryBarrier toRead{};
		toRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		toRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		toRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		toRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		toRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		toRead.image = m_shelterImage->handle();
		toRead.subresourceRange = wholeImage;

		vkCmdPipelineBarrier(handle, VK_PIPELINE_STAGE_TRANSFER_BIT, readers, 0, 0, nullptr, 0, nullptr, 1, &toRead);
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
		/* The textures a table may still hold share the objects retired here: they outlive them unharmed. */
		m_caustics = {};
		deferredDestructor.retireObject(std::move(m_curvatureSampler));
		deferredDestructor.retireObject(std::move(m_waves));
		m_surfaceWaves = {};

		for ( auto & staging : m_shelterStaging )
		{
			deferredDestructor.retireObject(std::move(staging));
		}

		m_shelterStaging.clear();
		deferredDestructor.retireObject(std::move(m_shelterSampler));
		deferredDestructor.retireObject(std::move(m_shelterView));
		deferredDestructor.retireObject(std::move(m_shelterImage));
		m_uploadedShelterGeneration = 0;
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
		m_surfaceWaves.uniformsOffset = m_uniformSectionSize * frameIndex;

		/* The shelter map of this frame: a new one (or the first, the open sea everywhere when none was given) is
		 * staged now and uploaded ahead of the FFT, below. */
		std::shared_ptr< const SeaShelterMap > shelterMap;
		uint64_t shelterGeneration = 0;
		std::array< float, OceanWaves::CascadeCount > shelteredWaveShares{};
		CausticTuning causticTuning{};

		{
			const Base::MutexLock lock{m_shelterAccess};

			shelterMap = m_shelterMap;
			shelterGeneration = m_shelterGeneration;
			shelteredWaveShares = m_shelterTuning.waveShares;
			causticTuning = m_causticTuning;
		}

		/* The caustics' tuning of this frame, bounded: nothing for a value that is not a number. */
		m_caustics.strength = std::isfinite(causticTuning.strength) ? std::clamp(causticTuning.strength, 0.0F, 1.0F) : 0.0F;
		m_caustics.softening = std::isfinite(causticTuning.softening) ? std::max(causticTuning.softening, 0.0F) : 0.0F;
		m_caustics.fadeDepth = std::isfinite(causticTuning.fadeDepth) ? std::max(causticTuning.fadeDepth, 0.01F) : 0.01F;

		const bool uploadShelter = shelterGeneration != m_uploadedShelterGeneration;

		if ( uploadShelter && !this->stageShelterMap(frameIndex, shelterMap.get()) )
		{
			return false;
		}

		/* The uniforms of this frame. */
		{
			OceanSurface::Uniforms uniforms{};
			uniforms.grid = {m_parameters.finestCell, 0.0F, static_cast< float >(m_parameters.levelCount), static_cast< float >(m_parameters.patchQuads)};
			uniforms.level = {m_parameters.seaLevel, 0.0F, 0.0F, 0.0F};
			uniforms.cascades = {m_parameters.waves.cascadeSizes[0], m_parameters.waves.cascadeSizes[1], m_parameters.waves.cascadeSizes[2], static_cast< float >(OceanWaves::CascadeCount)};
			uniforms.textureCoordinates = {m_parameters.textureRepeatsPerMetre, m_parameters.textureRepeatsPerMetre, 0.0F, 0.0F};
			uniforms.shelter = m_shelterRectangle;
			/* A share is a part of the waves: 0 to 1, and nothing for a value that is not a number. */
			const auto share = [] (float value) noexcept {
				return std::isfinite(value) ? std::clamp(value, 0.0F, 1.0F) : 0.0F;
			};

			uniforms.shelteredShares = {share(std::get< 0 >(shelteredWaveShares)), share(std::get< 1 >(shelteredWaveShares)), share(std::get< 2 >(shelteredWaveShares)), 0.0F};

			/* The caustics follow the surface: the same shares, the same map. */
			m_caustics.shelteredShares = {uniforms.shelteredShares[0], uniforms.shelteredShares[1], uniforms.shelteredShares[2]};
			m_caustics.shelterRectangle = m_shelterRectangle;

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

		if ( uploadShelter )
		{
			this->recordShelterUpload(*commandBuffer, frameIndex);
		}

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

		/* The upload is on the queue: the image holds this map from this frame on. */
		if ( uploadShelter )
		{
			m_uploadedShelterGeneration = shelterGeneration;
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
