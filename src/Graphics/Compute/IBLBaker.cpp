/*
 * src/Graphics/Compute/IBLBaker.cpp
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

/* NOTE: Define EMERAUDE_DEBUG_IBL_FACES (before the header: the dump method declaration
 * is gated by the same macro) to write tonemapped PNGs of every baked face to /tmp. */

#include "IBLBaker.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <chrono>
#include <string>
#include <vector>

/* Local inclusions. */
#include "Graphics/IBLTexture.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"

#ifdef EMERAUDE_DEBUG_IBL_FACES
#include <cmath>
#include <filesystem>
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "Vulkan/Buffer.hpp"
#endif

namespace EmEn::Graphics::Compute
{
	/* ---- Compute shader sources ----
	 * Split-sum BRDF LUT integration (Karis, "Real Shading in Unreal Engine 4", SIGGRAPH 2013).
	 * X axis = NdotV, Y axis = roughness. Outputs (scale, bias) on F0:
	 * specular = prefilteredColor * (F0 * lut.x + lut.y).
	 * The Smith visibility uses the IBL k remap (k = a^2 / 2), NOT the analytic-light
	 * Disney remap — Karis: applying the latter to IBL is "much too dark" at glancing angles. */

	static const std::string BRDFLutShaderSource = R"(
#version 450

layout(local_size_x = 8, local_size_y = 8) in;

layout(set = 0, binding = 0) uniform writeonly image2D brdfLut;

const uint SampleCount = 1024u;
const float PI = 3.14159265359;

/* Van der Corput radical inverse for the Hammersley low-discrepancy sequence. */
float radicalInverseVdC (uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

	return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley (uint i, uint n)
{
	return vec2(float(i) / float(n), radicalInverseVdC(i));
}

/* GGX importance sample around N = +Z (tangent space). */
vec3 importanceSampleGGX (vec2 Xi, float roughness)
{
	const float a = roughness * roughness;
	const float phi = 2.0 * PI * Xi.x;
	const float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

float geometrySchlickGGX (float NdotX, float roughness)
{
	/* IBL remap: k = a^2 / 2. */
	const float a = roughness * roughness;
	const float k = a / 2.0;

	return NdotX / (NdotX * (1.0 - k) + k);
}

float geometrySmith (float NdotV, float NdotL, float roughness)
{
	return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

void main ()
{
	const ivec2 lutSize = imageSize(brdfLut);
	const ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);

	if ( texelCoord.x >= lutSize.x || texelCoord.y >= lutSize.y )
	{
		return;
	}

	/* Half-texel offset: NdotV never reaches 0 (division below) nor exactly 1. */
	const float NdotV = (float(texelCoord.x) + 0.5) / float(lutSize.x);
	const float roughness = (float(texelCoord.y) + 0.5) / float(lutSize.y);

	const vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);

	float A = 0.0;
	float B = 0.0;

	for ( uint i = 0u; i < SampleCount; ++i )
	{
		const vec2 Xi = hammersley(i, SampleCount);
		const vec3 H = importanceSampleGGX(Xi, roughness);
		const vec3 L = normalize(2.0 * dot(V, H) * H - V);

		const float NdotL = max(L.z, 0.0);
		const float NdotH = max(H.z, 0.0);
		const float VdotH = max(dot(V, H), 0.0);

		if ( NdotL > 0.0 )
		{
			const float G = geometrySmith(NdotV, NdotL, roughness);
			const float GVis = (G * VdotH) / (NdotH * NdotV);
			const float Fc = pow(1.0 - VdotH, 5.0);

			A += (1.0 - Fc) * GVis;
			B += Fc * GVis;
		}
	}

	A /= float(SampleCount);
	B /= float(SampleCount);

	imageStore(brdfLut, texelCoord, vec4(A, B, 0.0, 1.0));
}
)";

	/* Shared GLSL between the two environment passes: face direction (CUBEMAP space —
	 * the baker never applies the world-to-cubemap Y negation, that is the consumer's
	 * contract), Hammersley sequence and tangent frame. The filtered importance sampling
	 * (Krivanek & Colbert, GPU Gems 3 ch. 20) reads the SOURCE mip chain by the ratio of
	 * the sample solid angle to the texel solid angle — this is what makes 32-128 samples
	 * per texel enough where brute force needs 1024+. */

	static const std::string EnvironmentCommonGLSL = R"(
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(set = 0, binding = 0) uniform samplerCube sourceEnv;
layout(set = 0, binding = 1) uniform writeonly image2DArray destFaces;

layout(push_constant) uniform PushConstants
{
	uint sourceSize;
	uint destSize;
	uint sampleCount;
	float roughness;
} pc;

const float PI = 3.14159265359;

/* Standard cube face layout (+X,-X,+Y,-Y,+Z,-Z), st in [-1,1]. CUBEMAP space. */
vec3 faceDirection (uint face, vec2 st)
{
	switch ( face )
	{
		case 0u: return vec3( 1.0, -st.y, -st.x);
		case 1u: return vec3(-1.0, -st.y,  st.x);
		case 2u: return vec3( st.x,  1.0,  st.y);
		case 3u: return vec3( st.x, -1.0, -st.y);
		case 4u: return vec3( st.x, -st.y,  1.0);
		default: return vec3(-st.x, -st.y, -1.0);
	}
}

float radicalInverseVdC (uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

	return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley (uint i, uint n)
{
	return vec2(float(i) / float(n), radicalInverseVdC(i));
}

void tangentFrame (vec3 N, out vec3 T, out vec3 B)
{
	const vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	T = normalize(cross(up, N));
	B = cross(N, T);
}

/* Solid angle of one texel of the SOURCE cubemap (mip 0). */
float sourceTexelSolidAngle ()
{
	return 4.0 * PI / (6.0 * float(pc.sourceSize * pc.sourceSize));
}
)";

	static const std::string PrefilterShaderSource = "#version 450\n" + EnvironmentCommonGLSL + R"(
float distributionGGX (float NdotH, float roughness)
{
	const float a = roughness * roughness;
	const float a2 = a * a;
	const float d = NdotH * NdotH * (a2 - 1.0) + 1.0;

	return a2 / (PI * d * d);
}

vec3 importanceSampleGGX (vec2 Xi, float roughness)
{
	const float a = roughness * roughness;
	const float phi = 2.0 * PI * Xi.x;
	const float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	const float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

	return vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}

void main ()
{
	const ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
	const uint face = gl_GlobalInvocationID.z;

	if ( texelCoord.x >= int(pc.destSize) || texelCoord.y >= int(pc.destSize) )
	{
		return;
	}

	const vec2 st = 2.0 * (vec2(texelCoord) + 0.5) / float(pc.destSize) - 1.0;
	const vec3 N = normalize(faceDirection(face, st));

	/* Mip 0 = perfect mirror: a straight copy of the source (Karis N=V=R with
	 * roughness 0 degenerates to exactly this — skip the sampling loop). */
	if ( pc.roughness <= 0.0 )
	{
		imageStore(destFaces, ivec3(texelCoord, int(face)), vec4(textureLod(sourceEnv, N, 0.0).rgb, 1.0));

		return;
	}

	vec3 T;
	vec3 B;
	tangentFrame(N, T, B);

	const float saTexel = sourceTexelSolidAngle();

	vec3 accumulated = vec3(0.0);
	float totalWeight = 0.0;

	for ( uint i = 0u; i < pc.sampleCount; ++i )
	{
		const vec2 Xi = hammersley(i, pc.sampleCount);
		const vec3 Ht = importanceSampleGGX(Xi, pc.roughness);
		const vec3 H = T * Ht.x + B * Ht.y + N * Ht.z;
		/* N = V = R (Karis isotropic assumption). */
		const vec3 L = normalize(2.0 * dot(N, H) * H - N);

		const float NdotL = dot(N, L);

		if ( NdotL > 0.0 )
		{
			/* Filtered importance sampling: read the source mip whose texel solid
			 * angle matches the sample solid angle. */
			const float NdotH = max(dot(N, H), 0.0);
			const float pdf = distributionGGX(NdotH, pc.roughness) * 0.25 + 0.0001;
			const float saSample = 1.0 / (float(pc.sampleCount) * pdf);
			const float mip = 0.5 * log2(saSample / saTexel);

			/* Cosine weighting: not in the split-sum derivation, but "achieves better
			 * results" (Karis, note 1). */
			accumulated += textureLod(sourceEnv, L, max(mip, 0.0)).rgb * NdotL;
			totalWeight += NdotL;
		}
	}

	imageStore(destFaces, ivec3(texelCoord, int(face)), vec4(accumulated / max(totalWeight, 0.0001), 1.0));
}
)";

	/* Diffuse irradiance, cosine importance sampling. The stored value is E/pi (the
	 * cosine-weighted MEAN radiance): the ambient shading term is then simply
	 * `albedo * texture(irradiance, N) * environmentLuminance`, which matches the
	 * scalar path (albedo/pi * averageColor * ambientIlluminance) on a uniform sky. */

	static const std::string IrradianceShaderSource = "#version 450\n" + EnvironmentCommonGLSL + R"(
void main ()
{
	const ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
	const uint face = gl_GlobalInvocationID.z;

	if ( texelCoord.x >= int(pc.destSize) || texelCoord.y >= int(pc.destSize) )
	{
		return;
	}

	const vec2 st = 2.0 * (vec2(texelCoord) + 0.5) / float(pc.destSize) - 1.0;
	const vec3 N = normalize(faceDirection(face, st));

	vec3 T;
	vec3 B;
	tangentFrame(N, T, B);

	const float saTexel = sourceTexelSolidAngle();

	vec3 accumulated = vec3(0.0);

	for ( uint i = 0u; i < pc.sampleCount; ++i )
	{
		const vec2 Xi = hammersley(i, pc.sampleCount);

		/* Cosine-weighted hemisphere sample (pdf = cosTheta / pi). */
		const float phi = 2.0 * PI * Xi.x;
		const float cosTheta = sqrt(1.0 - Xi.y);
		const float sinTheta = sqrt(Xi.y);

		const vec3 Lt = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
		const vec3 L = T * Lt.x + B * Lt.y + N * Lt.z;

		/* Filtered importance sampling against the source mip chain. The +1 mip bias
		 * trades irrelevant high frequencies for variance: near-normal samples would
		 * otherwise read the detailed mips (a sun disc there prints a star-shaped
		 * pattern of the fixed Hammersley sequence into the irradiance). */
		const float pdf = cosTheta / PI + 0.0001;
		const float saSample = 1.0 / (float(pc.sampleCount) * pdf);
		const float mip = 0.5 * log2(saSample / saTexel) + 1.0;

		/* The cosine cancels with the pdf: the estimator of E/pi is the plain mean. */
		accumulated += textureLod(sourceEnv, L, max(mip, 0.0)).rgb;
	}

	imageStore(destFaces, ivec3(texelCoord, int(face)), vec4(accumulated / float(pc.sampleCount), 1.0));
}
)";

	/* Push constant block shared by the two environment pipelines. */
	struct EnvironmentPushConstants
	{
		uint32_t sourceSize;
		uint32_t destSize;
		uint32_t sampleCount;
		float roughness;
	};

	/* Per-texel sample counts (FIS makes these enough — see class note). The prefilter
	 * count grows with the mip level: the GGX lobe widens with roughness and the texel
	 * count shrinks just as fast, so the extra samples are virtually free. */
	constexpr uint32_t PrefilterBaseSampleCount{64};
	constexpr uint32_t PrefilterSampleCountPerMip{32};
	constexpr uint32_t IrradianceSampleCount{512};

	IBLBaker::IBLBaker (const std::shared_ptr< Vulkan::Device > & device, Saphir::ShaderManager & shaderManager) noexcept
		: m_device{device},
		m_shaderManager{&shaderManager}
	{

	}

	IBLBaker::~IBLBaker () = default;

	bool
	IBLBaker::generateBRDFLut (IBLTexture & lut) const noexcept
	{
		if ( !lut.isCreated() || lut.role() != IBLTexture::Role::BRDFLut )
		{
			Tracer::error(ClassId, "The destination texture is not a created BRDF LUT !");

			return false;
		}

		/* Step 1: Compile the compute shader. */
		const auto shaderModule = m_shaderManager->getShaderModuleFromSourceCode(m_device, "IBLBRDFLutCS", Saphir::ShaderType::ComputeShader, BRDFLutShaderSource);

		if ( shaderModule == nullptr )
		{
			Tracer::error(ClassId, "Failed to compile the BRDF LUT compute shader !");

			return false;
		}

		/* Step 2: Descriptor set layout (binding 0: storage image). */
		const auto descriptorSetLayout = std::make_shared< Vulkan::DescriptorSetLayout >(m_device, "IBLBRDFLutDSLayout");

		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = 0;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			descriptorSetLayout->declare(binding);
		}

		if ( !descriptorSetLayout->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the descriptor set layout !");

			return false;
		}

		/* Step 3: Pipeline layout and compute pipeline. */
		const auto pipelineLayout = std::make_shared< Vulkan::PipelineLayout >(
			m_device, "IBLBRDFLutPipelineLayout",
			Base::StaticVector< std::shared_ptr< Vulkan::DescriptorSetLayout >, 6 >{descriptorSetLayout},
			Base::StaticVector< VkPushConstantRange, 4 >{}
		);

		if ( !pipelineLayout->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the pipeline layout !");

			return false;
		}

		const auto computePipeline = std::make_unique< Vulkan::ComputePipeline >(pipelineLayout);
		computePipeline->setShaderModule(shaderModule->handle());

		if ( !computePipeline->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the compute pipeline !");

			return false;
		}

		/* Step 4: Descriptor pool and set. */
		const std::vector< VkDescriptorPoolSize > poolSizes{
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}
		};

		const auto descriptorPool = std::make_shared< Vulkan::DescriptorPool >(m_device, poolSizes, 1);

		if ( !descriptorPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the descriptor pool !");

			return false;
		}

		const auto descriptorSet = std::make_unique< Vulkan::DescriptorSet >(descriptorPool, descriptorSetLayout);

		if ( !descriptorSet->create() )
		{
			Tracer::error(ClassId, "Failed to allocate the descriptor set !");

			return false;
		}

		{
			VkDescriptorImageInfo imageInfo{};
			imageInfo.sampler = VK_NULL_HANDLE;
			imageInfo.imageView = lut.storageView(0)->handle();
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			VkWriteDescriptorSet write{};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.dstSet = descriptorSet->handle();
			write.dstBinding = 0;
			write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			write.descriptorCount = 1;
			write.pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(m_device->handle(), 1, &write, 0, nullptr);
		}

		/* Step 5: Record and submit on the graphics queue (see class note). */
		const auto commandPool = std::make_shared< Vulkan::CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, true, false);

		if ( !commandPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the command pool !");

			return false;
		}

		const auto commandBuffer = std::make_unique< Vulkan::CommandBuffer >(commandPool, true);

		if ( !commandBuffer->isCreated() || !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			Tracer::error(ClassId, "Failed to create/begin the command buffer !");

			return false;
		}

		const auto & image = *lut.image();

		/* UNDEFINED -> GENERAL for the compute write. */
		{
			Vulkan::Sync::ImageMemoryBarrier barrier{
				image,
				0,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL
			};

			commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		commandBuffer->bind(*computePipeline);
		commandBuffer->bind(*descriptorSet, *pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);

		const uint32_t groupCount = (IBLTexture::BRDFLutSize + 7) / 8;
		commandBuffer->dispatch(groupCount, groupCount, 1);

		/* GENERAL -> SHADER_READ_ONLY for fragment sampling. */
		{
			Vulkan::Sync::ImageMemoryBarrier barrier{
				image,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		if ( !commandBuffer->end() )
		{
			Tracer::error(ClassId, "Failed to end the command buffer !");

			return false;
		}

		auto * queue = m_device->getGraphicsQueue(Vulkan::QueuePriority::High);

		if ( queue == nullptr || !queue->submit(*commandBuffer) || !queue->waitIdle() )
		{
			Tracer::error(ClassId, "Failed to submit the BRDF LUT bake !");

			return false;
		}

		lut.image()->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		Tracer::success(ClassId, "BRDF LUT (128px², RGBA16F, 1024 samples) baked successfully.");

		return true;
	}

	bool
	IBLBaker::ensureEnvironmentPipelines () noexcept
	{
		if ( m_prefilterPipeline != nullptr && m_irradiancePipeline != nullptr )
		{
			return true;
		}

		const auto prefilterModule = m_shaderManager->getShaderModuleFromSourceCode(m_device, "IBLPrefilterCS", Saphir::ShaderType::ComputeShader, PrefilterShaderSource);
		const auto irradianceModule = m_shaderManager->getShaderModuleFromSourceCode(m_device, "IBLIrradianceCS", Saphir::ShaderType::ComputeShader, IrradianceShaderSource);

		if ( prefilterModule == nullptr || irradianceModule == nullptr )
		{
			Tracer::error(ClassId, "Failed to compile the environment IBL compute shaders !");

			return false;
		}

		/* Shared descriptor set layout: binding 0 = source cubemap, binding 1 = dest storage. */
		m_environmentDSLayout = std::make_shared< Vulkan::DescriptorSetLayout >(m_device, "IBLEnvironmentDSLayout");

		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = 0;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			m_environmentDSLayout->declare(binding);
		}

		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = 1;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			m_environmentDSLayout->declare(binding);
		}

		if ( !m_environmentDSLayout->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the environment descriptor set layout !");

			return false;
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(EnvironmentPushConstants);

		m_environmentPipelineLayout = std::make_shared< Vulkan::PipelineLayout >(
			m_device, "IBLEnvironmentPipelineLayout",
			Base::StaticVector< std::shared_ptr< Vulkan::DescriptorSetLayout >, 6 >{m_environmentDSLayout},
			Base::StaticVector< VkPushConstantRange, 4 >{pushConstantRange}
		);

		if ( !m_environmentPipelineLayout->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the environment pipeline layout !");

			return false;
		}

		m_prefilterPipeline = std::make_unique< Vulkan::ComputePipeline >(m_environmentPipelineLayout);
		m_prefilterPipeline->setShaderModule(prefilterModule->handle());

		m_irradiancePipeline = std::make_unique< Vulkan::ComputePipeline >(m_environmentPipelineLayout);
		m_irradiancePipeline->setShaderModule(irradianceModule->handle());

		if ( !m_prefilterPipeline->createOnHardware() || !m_irradiancePipeline->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the environment compute pipelines !");

			return false;
		}

		m_commandPool = std::make_shared< Vulkan::CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, true, false);

		if ( !m_commandPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the environment command pool !");

			return false;
		}

		m_commandBuffer = std::make_unique< Vulkan::CommandBuffer >(m_commandPool, true);

		if ( !m_commandBuffer->isCreated() )
		{
			Tracer::error(ClassId, "Failed to create the environment command buffer !");

			return false;
		}

		return true;
	}

	bool
	IBLBaker::bakeEnvironment (const Vulkan::TextureInterface & source, IBLTexture & irradiance, IBLTexture & prefiltered) noexcept
	{
		if ( !source.isCreated() || !source.isCubemapTexture() )
		{
			Tracer::error(ClassId, "The source environment texture is not a created cubemap !");

			return false;
		}

		if ( !irradiance.isCreated() || irradiance.role() != IBLTexture::Role::IrradianceCubemap )
		{
			Tracer::error(ClassId, "The irradiance destination is not a created IrradianceCubemap texture !");

			return false;
		}

		if ( !prefiltered.isCreated() || prefiltered.role() != IBLTexture::Role::PrefilteredCubemap )
		{
			Tracer::error(ClassId, "The prefiltered destination is not a created PrefilteredCubemap texture !");

			return false;
		}

		if ( !this->ensureEnvironmentPipelines() )
		{
			return false;
		}

		const auto sourceSize = source.image()->width();
		const uint32_t prefilteredMipLevels = prefiltered.mipLevels();
		const uint32_t dispatchCount = prefilteredMipLevels + 1;

		/* Transient descriptor pool + sets: one set per dispatch (6 prefilter mips + 1
		 * irradiance). Rebuilt at every bake — the pool is tiny and a sky change is rare. */
		const std::vector< VkDescriptorPoolSize > poolSizes{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, dispatchCount},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, dispatchCount}
		};

		const auto descriptorPool = std::make_shared< Vulkan::DescriptorPool >(m_device, poolSizes, dispatchCount);

		if ( !descriptorPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the environment descriptor pool !");

			return false;
		}

		std::vector< std::unique_ptr< Vulkan::DescriptorSet > > descriptorSets;
		descriptorSets.reserve(dispatchCount);

		const auto writeDescriptorSet = [&] (const Vulkan::DescriptorSet & descriptorSet, const std::shared_ptr< Vulkan::ImageView > & destView) {
			VkDescriptorImageInfo sourceInfo{};
			sourceInfo.sampler = source.sampler()->handle();
			sourceInfo.imageView = source.imageView()->handle();
			sourceInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorImageInfo destInfo{};
			destInfo.sampler = VK_NULL_HANDLE;
			destInfo.imageView = destView->handle();
			destInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			std::array< VkWriteDescriptorSet, 2 > writes{};

			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet = descriptorSet.handle();
			writes[0].dstBinding = 0;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].descriptorCount = 1;
			writes[0].pImageInfo = &sourceInfo;

			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstSet = descriptorSet.handle();
			writes[1].dstBinding = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writes[1].descriptorCount = 1;
			writes[1].pImageInfo = &destInfo;

			vkUpdateDescriptorSets(m_device->handle(), static_cast< uint32_t >(writes.size()), writes.data(), 0, nullptr);
		};

		for ( uint32_t dispatchIndex = 0; dispatchIndex < dispatchCount; dispatchIndex++ )
		{
			auto descriptorSet = std::make_unique< Vulkan::DescriptorSet >(descriptorPool, m_environmentDSLayout);

			if ( !descriptorSet->create() )
			{
				Tracer::error(ClassId, "Failed to allocate an environment descriptor set !");

				return false;
			}

			const auto destView = dispatchIndex < prefilteredMipLevels
				? prefiltered.storageView(dispatchIndex)
				: irradiance.storageView(0);

			writeDescriptorSet(*descriptorSet, destView);

			descriptorSets.emplace_back(std::move(descriptorSet));
		}

		/* Record the whole bake (both destination images) in one submission. */
		if ( !m_commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			Tracer::error(ClassId, "Failed to begin the environment command buffer !");

			return false;
		}

		/* Both destinations: UNDEFINED -> GENERAL (previous content discarded — the
		 * ping-pong pair never republishes an old bake). */
		for ( const auto * texture : {&prefiltered, &irradiance} )
		{
			Vulkan::Sync::ImageMemoryBarrier barrier{
				*texture->image(),
				0,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL
			};

			m_commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		/* Prefiltered chain: one dispatch per mip, roughness 0 -> 1 across the chain. */
		m_commandBuffer->bind(*m_prefilterPipeline);

		for ( uint32_t mipLevel = 0; mipLevel < prefilteredMipLevels; mipLevel++ )
		{
			const uint32_t destSize = std::max(1U, IBLTexture::PrefilteredSize >> mipLevel);

			EnvironmentPushConstants pushConstants{};
			pushConstants.sourceSize = sourceSize;
			pushConstants.destSize = destSize;
			pushConstants.sampleCount = PrefilterBaseSampleCount + PrefilterSampleCountPerMip * mipLevel;
			pushConstants.roughness = static_cast< float >(mipLevel) / static_cast< float >(prefilteredMipLevels - 1);

			m_commandBuffer->bind(*descriptorSets[mipLevel], *m_environmentPipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);

			vkCmdPushConstants(m_commandBuffer->handle(), m_environmentPipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(EnvironmentPushConstants), &pushConstants);

			const uint32_t groupCount = (destSize + 7) / 8;
			m_commandBuffer->dispatch(groupCount, groupCount, 6);
		}

		/* Irradiance. */
		{
			m_commandBuffer->bind(*m_irradiancePipeline);

			EnvironmentPushConstants pushConstants{};
			pushConstants.sourceSize = sourceSize;
			pushConstants.destSize = IBLTexture::IrradianceSize;
			pushConstants.sampleCount = IrradianceSampleCount;
			pushConstants.roughness = 1.0F;

			m_commandBuffer->bind(*descriptorSets[prefilteredMipLevels], *m_environmentPipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);

			vkCmdPushConstants(m_commandBuffer->handle(), m_environmentPipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(EnvironmentPushConstants), &pushConstants);

			const uint32_t groupCount = (IBLTexture::IrradianceSize + 7) / 8;
			m_commandBuffer->dispatch(groupCount, groupCount, 6);
		}

		/* Both destinations: GENERAL -> SHADER_READ_ONLY for fragment sampling. */
		for ( const auto * texture : {&prefiltered, &irradiance} )
		{
			Vulkan::Sync::ImageMemoryBarrier barrier{
				*texture->image(),
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			m_commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		if ( !m_commandBuffer->end() )
		{
			Tracer::error(ClassId, "Failed to end the environment command buffer !");

			return false;
		}

		/* Blocking submit (v1). Upgrade path when a per-frame dynamic sky lands: submit
		 * with a fence, poll it from the logic thread, publish on completion. */
		const auto start = std::chrono::steady_clock::now();

		auto * queue = m_device->getGraphicsQueue(Vulkan::QueuePriority::High);

		if ( queue == nullptr || !queue->submit(*m_commandBuffer) || !queue->waitIdle() )
		{
			Tracer::error(ClassId, "Failed to submit the environment IBL bake !");

			return false;
		}

		const auto durationUS = std::chrono::duration_cast< std::chrono::microseconds >(std::chrono::steady_clock::now() - start).count();

		static_cast< void >(m_commandBuffer->reset());

		irradiance.image()->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		prefiltered.image()->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		TraceInfo{ClassId} <<
			"Environment IBL baked from a " << sourceSize << "px² source: prefiltered " <<
			IBLTexture::PrefilteredSize << "px²x" << prefilteredMipLevels << " mips (" << PrefilterBaseSampleCount <<
			"+ samples/texel), irradiance " << IBLTexture::IrradianceSize << "px² (" << IrradianceSampleCount <<
			" samples/texel) in " << durationUS << " us (submit+wait).";

#ifdef EMERAUDE_DEBUG_IBL_FACES
		this->dumpTextureFaces(irradiance, "irradiance");
		this->dumpTextureFaces(prefiltered, "prefiltered");
#endif

		return true;
	}

#ifdef EMERAUDE_DEBUG_IBL_FACES
	void
	IBLBaker::dumpTextureFaces (const IBLTexture & texture, const char * label) const noexcept
	{
		using namespace Base::PixelFactory;

		const auto & image = *texture.image();
		const uint32_t mipLevels = image.createInfo().mipLevels;

		for ( uint32_t mipLevel = 0; mipLevel < mipLevels; mipLevel++ )
		{
			const uint32_t size = std::max(1U, image.width() >> mipLevel);
			const VkDeviceSize faceBytes = static_cast< VkDeviceSize >(size) * size * 4 * sizeof(uint16_t);

			Vulkan::Buffer stagingBuffer{m_device, 0, faceBytes * 6, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true};

			if ( !stagingBuffer.createOnHardware() )
			{
				return;
			}

			const auto commandPool = std::make_shared< Vulkan::CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, true, false);

			if ( !commandPool->createOnHardware() )
			{
				return;
			}

			Vulkan::CommandBuffer commandBuffer{commandPool, true};

			if ( !commandBuffer.isCreated() || !commandBuffer.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
			{
				return;
			}

			{
				Vulkan::Sync::ImageMemoryBarrier barrier{image, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};
				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			}

			std::array< VkBufferImageCopy, 6 > regions{};

			for ( uint32_t faceIndex = 0; faceIndex < 6; faceIndex++ )
			{
				regions[faceIndex].bufferOffset = faceBytes * faceIndex;
				regions[faceIndex].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				regions[faceIndex].imageSubresource.mipLevel = mipLevel;
				regions[faceIndex].imageSubresource.baseArrayLayer = faceIndex;
				regions[faceIndex].imageSubresource.layerCount = 1;
				regions[faceIndex].imageExtent = {size, size, 1};
			}

			vkCmdCopyImageToBuffer(commandBuffer.handle(), image.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.handle(), static_cast< uint32_t >(regions.size()), regions.data());

			{
				Vulkan::Sync::ImageMemoryBarrier barrier{image, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			}

			if ( !commandBuffer.end() )
			{
				return;
			}

			auto * queue = m_device->getGraphicsQueue(Vulkan::QueuePriority::High);

			if ( queue == nullptr || !queue->submit(commandBuffer) || !queue->waitIdle() )
			{
				return;
			}

			const auto * mapped = static_cast< const uint16_t * >(stagingBuffer.mapMemory());

			if ( mapped == nullptr )
			{
				return;
			}

			const auto halfToFloat = [] (uint16_t half) {
				const auto exponent = static_cast< int32_t >((half >> 10) & 0x1FU);
				const auto mantissa = half & 0x3FFU;
				if ( exponent == 0 ) { return 0.0F; }
				return std::ldexp(1.0F + static_cast< float >(mantissa) / 1024.0F, exponent - 15);
			};

			for ( uint32_t faceIndex = 0; faceIndex < 6; faceIndex++ )
			{
				Pixmap< uint8_t > debugFace;

				if ( !debugFace.initialize(size, size, ChannelMode::RGB) )
				{
					continue;
				}

				auto * out = debugFace.data().data();
				const auto * face = mapped + static_cast< size_t >(faceIndex) * size * size * 4;

				for ( size_t index = 0; index < static_cast< size_t >(size) * size; ++index )
				{
					for ( size_t channel = 0; channel < 3; ++channel )
					{
						const auto value = halfToFloat(face[index * 4 + channel]);
						out[index * 3 + channel] = static_cast< uint8_t >(std::pow(value / (1.0F + value), 1.0F / 2.2F) * 255.0F);
					}
				}

				std::ignore = FileIO::write(debugFace, std::filesystem::path{std::string{"/tmp/ibl-"} + label + "-mip" + std::to_string(mipLevel) + "-face" + std::to_string(faceIndex) + ".png"}, true);
			}

			stagingBuffer.unmapMemory();
		}
	}
#endif
}
