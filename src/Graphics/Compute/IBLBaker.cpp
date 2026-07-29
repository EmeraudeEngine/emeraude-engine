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

#include "IBLBaker.hpp"

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
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"

namespace EmEn::Graphics::Compute
{
	/* ---- Compute shader source ----
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
}
