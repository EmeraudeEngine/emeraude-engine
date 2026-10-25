/*
 * src/Graphics/SkyEnvironment.cpp
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

#include "SkyEnvironment.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <numbers>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/Effects/Shared/AtmosphereGLSL.hpp"
#include "Graphics/IBLTexture.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/SkyAtmosphere.hpp"
#include "Graphics/StorageTexture2D.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Saphir/Types.hpp"
#include "StaticVector.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ShaderModule.hpp"

namespace
{
	constexpr uint32_t ParametersBinding{0};
	constexpr uint32_t SkyViewBinding{1};
	constexpr uint32_t FrameBinding{2};
	constexpr uint32_t CaptureBinding{3};

	/** @brief The capture pass's push constants (std430: a vec4, a float, a uint). */
	struct CapturePushConstants
	{
		std::array< float, 4 > tintIntensity;
		float saturation;
		uint32_t size;
	};

	static_assert(sizeof(CapturePushConstants) == 24, "The capture push constants must match the GLSL block.");

	/* The capture: this frame's sky-view table for every texel direction of the six faces, × the star's illuminance and
	 * colour, graded — the drawn sky without the star's disc (it lights through its directional light). The lookup and
	 * the grading are the material's own (AtmosphereGLSL::SkyViewLookup, AtmosphereGLSL::Grade). */
	constexpr auto CaptureHeaderGLSL = R"GLSL(#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
)GLSL";

	constexpr auto CaptureMainGLSL = R"GLSL(
layout(set = 0, binding = 1) uniform sampler2D skyViewLut;
/* The frame texture (SkyAtmosphere::recordFrame()): texel 0 the unit vector toward the star, texel 1 its illuminance
 * above the atmosphere × its colour (lux) and the camera's distance to the planet's centre (km). */
layout(set = 0, binding = 2) uniform sampler2D frameData;
/* RGBA16F: absolute radiance — a night sky at -18 degrees (~1e-5 nits) is still a half float. */
layout(set = 0, binding = 3, rgba16f) uniform writeonly image2DArray captureFaces;

layout(push_constant) uniform CapturePushConstants
{
	vec4 tintIntensity;
	float saturation;
	uint size;
} pc;

void main ()
{
	const ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
	const uint face = gl_GlobalInvocationID.z;

	if ( texelCoord.x >= int(pc.size) || texelCoord.y >= int(pc.size) )
	{
		return;
	}

	const vec2 st = 2.0 * (vec2(texelCoord) + 0.5) / float(pc.size) - 1.0;
	const vec3 direction = normalize(faceDirection(face, st));
	const vec4 frame0 = texelFetch(frameData, ivec2(0, 0), 0);
	const vec4 frame1 = texelFetch(frameData, ivec2(1, 0), 0);
	const vec3 lookup = atmSkyViewLookup(direction, frame0.xyz, frame1.w);
	const vec3 radiance = textureLod(skyViewLut, lookup.xy, 0.0).rgb * frame1.rgb;

	imageStore(captureFaces, ivec3(texelCoord, int(face)), vec4(atmGrade(radiance, pc.tintIntensity, pc.saturation), 1.0));
}
)GLSL";
}

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	SkyEnvironment::SkyEnvironment (std::string name) noexcept
		: m_name{std::move(name)}
	{

	}

	SkyEnvironment::~SkyEnvironment () = default;

	bool
	SkyEnvironment::create (Renderer & renderer, const SkyAtmosphere & atmosphere) noexcept
	{
		const auto & device = renderer.device();

		if ( !atmosphere.isCreated() || atmosphere.parametersBuffer() == nullptr )
		{
			TraceError{ClassId} << "The atmosphere of the sky '" << m_name << "' has no tables to capture !";

			return false;
		}

		/* ---- The capture pass. ---- */
		m_captureLayout = std::make_shared< DescriptorSetLayout >(device, m_name + "SkyEnvironmentDSLayout");

		const auto declare = [this] (uint32_t binding, VkDescriptorType type) {
			VkDescriptorSetLayoutBinding layoutBinding{};
			layoutBinding.binding = binding;
			layoutBinding.descriptorType = type;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			m_captureLayout->declare(layoutBinding);
		};

		declare(ParametersBinding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		declare(SkyViewBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		declare(FrameBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		declare(CaptureBinding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);

		if ( !m_captureLayout->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the descriptor set layout of the sky environment '" << m_name << "' !";

			return false;
		}

		m_pipelineLayout = std::make_shared< PipelineLayout >(
			device, m_name + "SkyEnvironmentPipelineLayout",
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 >{m_captureLayout},
			StaticVector< VkPushConstantRange, 4 >{VkPushConstantRange{.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = sizeof(CapturePushConstants)}}
		);

		if ( !m_pipelineLayout->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the pipeline layout of the sky environment '" << m_name << "' !";

			return false;
		}

		const auto source = std::string{CaptureHeaderGLSL} +
			AtmosphereGLSL::uniformBlockGLSL(0, ParametersBinding) +
			AtmosphereGLSL::sharedFunctionsGLSL() +
			Compute::IBLBaker::FaceDirectionGLSL +
			CaptureMainGLSL;

		const auto module = renderer.shaderManager().getShaderModuleFromSourceCode(device, "SkyEnvironmentCaptureCS", Saphir::ShaderType::ComputeShader, source);

		if ( module == nullptr )
		{
			TraceError{ClassId} << "Unable to compile the capture shader of the sky environment '" << m_name << "' !";

			return false;
		}

		m_capturePipeline = std::make_unique< ComputePipeline >(m_pipelineLayout);
		m_capturePipeline->setShaderModule(module->handle());

		if ( !m_capturePipeline->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the capture pipeline of the sky environment '" << m_name << "' !";

			return false;
		}

		/* ---- The ring: allocated once. ⚠️ FREE_DESCRIPTOR_SET_BIT: a DescriptorSet frees itself. ---- */
		const std::vector< VkDescriptorPoolSize > poolSizes{
			{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = static_cast< uint32_t >(RingSize)},
			{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = static_cast< uint32_t >(2 * RingSize)},
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = static_cast< uint32_t >(RingSize)}
		};

		m_descriptorPool = std::make_shared< DescriptorPool >(device, poolSizes, static_cast< uint32_t >(RingSize), VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

		if ( !m_descriptorPool->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the descriptor pool of the sky environment '" << m_name << "' !";

			return false;
		}

		auto & baker = renderer.IBLBaker();

		for ( auto & set : m_ring )
		{
			set.capture = std::make_shared< IBLTexture >(IBLTexture::Role::EnvironmentCapture);
			set.irradiance = std::make_shared< IBLTexture >(IBLTexture::Role::IrradianceCubemap);
			set.prefiltered = std::make_shared< IBLTexture >(IBLTexture::Role::PrefilteredCubemap);

			if ( !set.capture->create(renderer) || !set.irradiance->create(renderer) || !set.prefiltered->create(renderer) )
			{
				TraceError{ClassId} << "Unable to create the textures of the sky environment '" << m_name << "' !";

				return false;
			}

			set.captureSet = std::make_unique< DescriptorSet >(m_descriptorPool, m_captureLayout);

			const auto & skyView = atmosphere.skyView();
			const auto & frameData = atmosphere.frameData();

			const bool written =
				set.captureSet->create() &&
				set.captureSet->writeUniformBufferObject(ParametersBinding, *atmosphere.parametersBuffer()) &&
				set.captureSet->writeCombinedImageSampler(SkyViewBinding, *skyView.imageView(), *skyView.sampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) &&
				set.captureSet->writeCombinedImageSampler(FrameBinding, *frameData.imageView(), *frameData.sampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) &&
				set.captureSet->writeStorageImage(CaptureBinding, *set.capture->storageView(0));

			if ( !written )
			{
				TraceError{ClassId} << "Unable to write the capture descriptor set of the sky environment '" << m_name << "' !";

				return false;
			}

			/* The bake reads the capture through its sampled view: it must be created (it is) — the bindings only
			 * reference it, the content comes with each cycle. */
			set.bindings = baker.createEnvironmentBindings(*set.capture, *set.irradiance, *set.prefiltered);

			if ( set.bindings == nullptr )
			{
				TraceError{ClassId} << "Unable to bind the bakes of the sky environment '" << m_name << "' !";

				return false;
			}
		}

		TraceSuccess{ClassId} << "Sky environment '" << m_name << "': a ring of " << RingSize << " sets (capture " << IBLTexture::EnvironmentCaptureSize << "², irradiance " << IBLTexture::IrradianceSize << "², prefiltered " << IBLTexture::PrefilteredSize << "² × " << IBLTexture::PrefilteredMipLevels << "), one stage per frame.";

		return true;
	}

	bool
	SkyEnvironment::cycleNeeded (const Inputs & inputs) const noexcept
	{
		if ( !m_baked || m_restart )
		{
			return true;
		}

		/* The star moved. */
		const auto cosMotion = Vector< 3, float >::dotProduct(inputs.sunDirection, m_cycle.sunDirection);

		if ( cosMotion < std::cos(TriggerAngle * (std::numbers::pi_v< float > / 180.0F)) )
		{
			return true;
		}

		/* A relative change of a positive quantity (an absolute one around zero). */
		const auto changed = [] (float now, float before) {
			return std::abs(now - before) > TriggerRatio * std::max(std::abs(before), 1.0e-3F);
		};

		for ( size_t channel = 0; channel < 3; ++channel )
		{
			if ( changed(inputs.sunIlluminance[channel], m_cycle.sunIlluminance[channel]) || changed(inputs.grading.tint[channel], m_cycle.grading.tint[channel]) )
			{
				return true;
			}
		}

		return changed(inputs.grading.intensity, m_cycle.grading.intensity) || changed(inputs.grading.saturation, m_cycle.grading.saturation);
	}

	void
	SkyEnvironment::recordCapture (const CommandBuffer & commandBuffer, const RingSet & set, const Grading & grading) const noexcept
	{
		auto * const image = set.capture->image()->handle();
		const uint32_t mipLevels = set.capture->mipLevels();
		constexpr uint32_t Size = IBLTexture::EnvironmentCaptureSize;

		/* Mip 0: discarded, after every earlier read of the set (any stage), opened for the compute write. */
		Compute::IBLBaker::recordCubeBarrier(commandBuffer, image, {.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_GENERAL, .srcAccess = 0, .dstAccess = VK_ACCESS_SHADER_WRITE_BIT, .srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, .dstStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .baseMip = 0, .mipCount = 1});

		const CapturePushConstants pushConstants{
			.tintIntensity = {grading.tint[0], grading.tint[1], grading.tint[2], grading.intensity},
			.saturation = grading.saturation,
			.size = Size
		};

		commandBuffer.bind(*m_capturePipeline);
		commandBuffer.bind(*set.captureSet, *m_pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
		vkCmdPushConstants(commandBuffer.handle(), m_pipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(CapturePushConstants), &pushConstants);
		commandBuffer.dispatch((Size + 7) / 8, (Size + 7) / 8, 6);

		/* The mip chain: mip 0 the blit source, the others discarded and filled by a cascade (ProbeConvolver's). */
		Compute::IBLBaker::recordCubeBarrier(commandBuffer, image, {.oldLayout = VK_IMAGE_LAYOUT_GENERAL, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, .srcAccess = VK_ACCESS_SHADER_WRITE_BIT, .dstAccess = VK_ACCESS_TRANSFER_READ_BIT, .srcStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .baseMip = 0, .mipCount = 1});
		Compute::IBLBaker::recordCubeBarrier(commandBuffer, image, {.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, .srcAccess = 0, .dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT, .srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .baseMip = 1, .mipCount = mipLevels - 1});

		for ( uint32_t mip = 1; mip < mipLevels; mip++ )
		{
			const auto sourceSize = static_cast< int32_t >(std::max(1U, Size >> (mip - 1U)));
			const auto destSize = static_cast< int32_t >(std::max(1U, Size >> mip));

			VkImageBlit region{};
			region.srcSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, mip - 1, 0, 6};
			region.srcOffsets[1] = VkOffset3D{sourceSize, sourceSize, 1};
			region.dstSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, mip, 0, 6};
			region.dstOffsets[1] = VkOffset3D{destSize, destSize, 1};

			vkCmdBlitImage(commandBuffer.handle(), image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);

			Compute::IBLBaker::recordCubeBarrier(commandBuffer, image, {.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, .srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT, .dstAccess = VK_ACCESS_TRANSFER_READ_BIT, .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .baseMip = mip, .mipCount = 1});
		}

		/* The whole chain: sampled by the bakes (compute) and, once published, by every reader of the environment slot. */
		Compute::IBLBaker::recordCubeBarrier(commandBuffer, image, {.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, .srcAccess = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT, .dstAccess = VK_ACCESS_SHADER_READ_BIT, .srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT, .dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, .baseMip = 0, .mipCount = mipLevels});
	}

	void
	SkyEnvironment::recordStage (Renderer & renderer, const CommandBuffer & commandBuffer, const SkyAtmosphere & atmosphere, const Inputs & inputs) noexcept
	{
		if ( m_failed )
		{
			return;
		}

		if ( !m_created )
		{
			if ( !this->create(renderer, atmosphere) )
			{
				/* Once: the scene keeps the IBL it has. */
				TraceError{ClassId} << "The sky environment '" << m_name << "' is disabled: the scene keeps its previous IBL.";

				m_failed = true;

				return;
			}

			m_created = true;
		}

		if ( m_stage == Stage::Idle )
		{
			const auto lengthSquared = inputs.sunDirection.lengthSquared();

			if ( !std::isfinite(lengthSquared) || lengthSquared <= 0.0F )
			{
				return;
			}

			Inputs normalized = inputs;
			normalized.sunDirection = inputs.sunDirection.normalized();

			if ( !this->cycleNeeded(normalized) )
			{
				return;
			}

			m_cycle = normalized;
			m_restart = false;
			m_stage = Stage::Capture;
		}

		/* m_ringIndex < RingSize: it only advances modulo RingSize. */
		const auto & set = *std::next(m_ring.begin(), static_cast< std::ptrdiff_t >(m_ringIndex));
		const auto & baker = renderer.IBLBaker();
		constexpr Compute::IBLBaker::StarMask NoMask{};

		switch ( m_stage )
		{
			case Stage::Capture :
				this->recordCapture(commandBuffer, set, m_cycle.grading);

				m_stage = Stage::Irradiance;
				break;

			case Stage::Irradiance :
				baker.recordIrradiance(commandBuffer, *set.bindings, *set.irradiance, IBLTexture::EnvironmentCaptureSize, NoMask);

				m_prefilterMip = 0;
				m_stage = Stage::Prefilter;
				break;

			case Stage::Prefilter :
				baker.recordPrefilterMip(commandBuffer, *set.bindings, m_prefilterMip, *set.prefiltered, IBLTexture::EnvironmentCaptureSize, NoMask);

				if ( ++m_prefilterMip >= set.prefiltered->mipLevels() )
				{
					/* Every stage left its target in SHADER_READ_ONLY_OPTIMAL: the tracked layout the bindless table
					 * describes the textures with follows (IBLBaker::bakeEnvironment() does the same). */
					for ( const auto * texture : {set.capture.get(), set.irradiance.get(), set.prefiltered.get()} )
					{
						texture->image()->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
					}

					/* The set is complete in this command buffer: released after its submission (onSubmitted()). */
					m_completed = Publication{
						.environment = set.capture,
						.irradiance = set.irradiance,
						.prefiltered = set.prefiltered,
						.sunDirection = m_cycle.sunDirection
					};

					if ( !m_baked )
					{
						TraceInfo{ClassId} << "Sky environment '" << m_name << "': first IBL baked from the sky.";
					}

					m_baked = true;
					m_ringIndex = (m_ringIndex + 1) % RingSize;
					m_stage = Stage::Idle;
				}
				break;

			case Stage::Idle :
				break;
		}
	}

	void
	SkyEnvironment::onSubmitted (bool submitted) noexcept
	{
		if ( !submitted )
		{
			/* The stages of this cycle recorded in the lost command buffer never ran: start the cycle over (every stage
			 * discards its target first, nothing depends on the lost content). */
			if ( m_stage != Stage::Idle || m_completed.has_value() )
			{
				m_completed.reset();
				m_stage = Stage::Idle;
				m_restart = true;
			}

			return;
		}

		if ( !m_completed.has_value() )
		{
			return;
		}

		const MutexLock lock{m_publicationAccess};

		m_publication = std::exchange(m_completed, std::nullopt);
	}

	std::optional< SkyEnvironment::Publication >
	SkyEnvironment::takePublication () noexcept
	{
		const MutexLock lock{m_publicationAccess};

		return std::exchange(m_publication, std::nullopt);
	}
}
