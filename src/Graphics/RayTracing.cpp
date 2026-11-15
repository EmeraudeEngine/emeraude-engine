/*
 * src/Graphics/RayTracing.cpp
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

#include "RayTracing.hpp"

/* STL inclusions. */
#include <array>
#include <string>
#include <utility>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/IrradianceProbeVolume.hpp"
#include "Graphics/RayTracingFrame.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/SkinnedGeometryProcessor.hpp"
#include "Scenes/LightSet.hpp"
#include "Scenes/SceneMetaData.hpp"
#include "Tracer.hpp"
#include "Vulkan/AccelerationStructure.hpp"
#include "Vulkan/AccelerationStructureBuilder.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/MemoryRegion.hpp"
#include "Vulkan/ShaderStorageBufferObject.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	RayTracing::RayTracing (Renderer & renderer) noexcept
		: ServiceInterface{ClassId},
		m_renderer{renderer}
	{

	}

	RayTracing::~RayTracing () = default;

	bool
	RayTracing::onInitialize () noexcept
	{
		/* The single acceleration structure builder. It is shared by every geometry (BLAS, for SHARED geometries that
		 * outlive any scene) and every scene (TLAS): it must NOT be owned per scene — deleting a scene would otherwise
		 * destroy the builder the active scene still needs and leave geometries unable to build their BLAS. */
		auto accelerationStructureBuilder = std::make_unique< AccelerationStructureBuilder >(m_renderer.device());

		if ( !accelerationStructureBuilder->initialize() )
		{
			TraceError{ClassId} << "Unable to initialize the acceleration structure builder !";

			return false;
		}

		m_accelerationStructureBuilder = std::move(accelerationStructureBuilder);

		/* What the traced passes bind: the set layout and one set per frame in flight. BEFORE the irradiance probe
		 * volume below, whose trace declares that layout. */
		if ( !this->createDescriptorSets() )
		{
			static_cast< void >(this->onTerminate());

			return false;
		}

		/* Skinned geometry support: a compute pass skinning vertices into per-instance mirror buffers for the per-frame
		 * BLAS refit. Optional: without it, skinned meshes are simply absent from the TLAS. */
		m_skinnedGeometryProcessor = std::make_unique< SkinnedGeometryProcessor >();

		if ( !m_skinnedGeometryProcessor->initialize(m_renderer) )
		{
			TraceWarning{ClassId} << "Unable to initialize the skinned geometry processor. Skinned meshes will be absent from the ray-traced effects.";

			m_skinnedGeometryProcessor.reset();
		}

		/* The irradiance probe volume — the radiance cache the traced effects read at their hits (Sep 2026). Optional:
		 * the effects that require it (the reflected world had no indirect light at all before it) are not created
		 * without it. */
		m_irradianceProbeVolume = std::make_unique< IrradianceProbeVolume >();

		if ( !m_irradianceProbeVolume->initialize(m_renderer) )
		{
			TraceWarning{ClassId} << "Unable to initialize the irradiance probe volume. The ray-traced reflections and global illumination will be unavailable.";

			m_irradianceProbeVolume.reset();
		}

		return true;
	}

	bool
	RayTracing::onTerminate () noexcept
	{
		/* NOTE: Everything here owns Vulkan objects (compute pipelines, layouts, images, descriptor sets, buffers): they
		 * are released now, while the device still exists, in the reverse order of their creation. */
		m_frame.reset();
		m_irradianceProbeVolume.reset();
		m_skinnedGeometryProcessor.reset();
		m_descriptorSets.clear();
		m_standInSSBO.reset();
		m_setLayout.reset();
		m_accelerationStructureBuilder.reset();

		return true;
	}

	bool
	RayTracing::createDescriptorSets () noexcept
	{
		const auto & device = m_renderer.device();
		const auto framesInFlight = m_renderer.framesInFlight();

		if ( framesInFlight == 0 || framesInFlight > MaxFramesInFlight )
		{
			TraceError{ClassId} << "The frames in flight count (" << framesInFlight << ") is outside [1, " << MaxFramesInFlight << "] !";

			return false;
		}

		/* The ray-tracing descriptor set layout:
		 * binding 0 = TLAS,
		 * binding 1 = mesh metadata SSBO,
		 * binding 2 = material data SSBO,
		 * binding 3 = light array SSBO,
		 * binding 4 = sub-geometry table SSBO. */
		auto setLayout = std::make_shared< DescriptorSetLayout >(device, "RTDescriptorSet");

		/* FRAGMENT for the post-process effects (fullscreen passes), COMPUTE for the irradiance probe
		 * volume's trace (Sep 2026): a compute pipeline may only bind a set whose bindings declare
		 * the compute stage — a fragment-only layout would fail the ray-query compute at bind time. */
		constexpr VkShaderStageFlags Stages = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

		if ( !setLayout->declareAccelerationStructureKHR(0, Stages) ||
			 !setLayout->declareStorageBuffer(1, Stages) ||
			 !setLayout->declareStorageBuffer(2, Stages) ||
			 !setLayout->declareStorageBuffer(3, Stages) ||
			 !setLayout->declareStorageBuffer(4, Stages) )
		{
			TraceError{ClassId} << "Unable to declare the ray-tracing descriptor set layout bindings !";

			return false;
		}

		if ( !setLayout->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the ray-tracing descriptor set layout !";

			return false;
		}

		/* NOTE: The stand-in of a storage binding the scene does not provide (doom-loader: a disabled light set has no RT
		 * light buffer, and 18 918 VUID-…-08114 "never been updated" followed). Every RT storage block is a runtime-sized
		 * vec4 array, so four zero vec4 read as an empty array (lights.length() / 4 = 0 lights). */
		constexpr std::array< float, 16 > Zeros{};

		auto standInSSBO = std::make_unique< ShaderStorageBufferObject >(device, sizeof(Zeros));

		if ( !standInSSBO->createOnHardware() || !standInSSBO->writeData({Zeros.data(), sizeof(Zeros)}) )
		{
			TraceError{ClassId} << "Unable to create the ray-tracing stand-in storage buffer !";

			return false;
		}

		m_setLayout = std::move(setLayout);
		m_standInSSBO = std::move(standInSSBO);

		for ( uint32_t index = 0; index < framesInFlight; ++index )
		{
			auto descriptorSet = std::make_unique< DescriptorSet >(m_renderer.descriptorPool(), m_setLayout);

			if ( !descriptorSet->create() )
			{
				TraceError{ClassId} << "Unable to create the ray-tracing descriptor set of the frame " << index << " !";

				return false;
			}

			/* NOTE: Proven in capacity: framesInFlight <= MaxFramesInFlight (tested above). */
			m_descriptorSets.push_back(std::move(descriptorSet));
		}

		return true;
	}

	const RayTracingFrame *
	RayTracing::bindFrame (uint32_t frameIndex, const Scenes::SceneMetaData & sceneMetaData, const Scenes::LightSet & lightSet) noexcept
	{
		m_frame.reset();

		/* NOTE: A session that cannot trace has no descriptor set. */
		if ( !this->usable() )
		{
			return nullptr;
		}

		if ( frameIndex >= m_descriptorSets.size() )
		{
			TraceError{ClassId} << "There is no ray-tracing descriptor set for the frame " << frameIndex << " !";

			return nullptr;
		}

		/* NOTE: A scene without ray-traced geometry, or whose first structure is not built yet: this frame does not trace. */
		const auto * TLAS = sceneMetaData.TLAS();

		if ( TLAS == nullptr || !TLAS->isCreated() )
		{
			return nullptr;
		}

		/* NOTE: Only the current frame's descriptor set is written, to avoid a write while another one is pending. */
		const auto & descriptorSet = *m_descriptorSets[frameIndex];

		/* Bindings 1-4: the scene's storage buffers (per frame where they are: no CPU/GPU race). A buffer the scene does
		 * not provide gets the stand-in: the programs statically use every binding, so none may stay unwritten. */
		const auto writeStorage = [this, &descriptorSet] (uint32_t binding, const ShaderStorageBufferObject * buffer) noexcept {
			const auto & bound = buffer != nullptr ? *buffer : *m_standInSSBO;

			const VkDescriptorBufferInfo info{
				.buffer = bound.handle(),
				.offset = 0,
				.range = bound.bytes()
			};

			return descriptorSet.writeStorageBuffer(binding, info);
		};

		/* Binding 0: the acceleration structure this frame built. */
		if ( !descriptorSet.writeAccelerationStructure(0, TLAS->handle()) ||
			 !writeStorage(1, sceneMetaData.meshMetaDataSSBO(frameIndex)) ||
			 !writeStorage(2, sceneMetaData.materialDataSSBO(frameIndex)) ||
			 !writeStorage(3, lightSet.RTLightBuffer(frameIndex)) ||
			 !writeStorage(4, sceneMetaData.subGeometryDataSSBO(frameIndex)) )
		{
			TraceError{ClassId} << "Unable to write the ray-tracing descriptor set of the frame " << frameIndex << ": this frame does not trace !";

			return nullptr;
		}

		m_frame.emplace(descriptorSet, lightSet.RTLightCount());

		return &*m_frame;
	}
}
