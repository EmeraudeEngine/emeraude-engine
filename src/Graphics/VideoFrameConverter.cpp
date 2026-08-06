/*
 * src/Graphics/VideoFrameConverter.cpp
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

#include "VideoFrameConverter.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstring>
#include <string>

/* Local inclusions. */
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "VideoColorConversion.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"

namespace EmEn::Graphics
{
	using namespace Base;

	/* Structured test pattern shared by the GLSL and C++ generators (gradients +
	 * checkerboard: corruption is visually obvious, bytes stay deterministic). */
	static void
	testPatternBGR (uint32_t x, uint32_t y, uint32_t width, uint32_t height, int32_t & blue, int32_t & green, int32_t & red) noexcept
	{
		blue = static_cast< int32_t >(x * 255U / std::max(width - 1U, 1U));
		green = static_cast< int32_t >(y * 255U / std::max(height - 1U, 1U));
		red = static_cast< int32_t >((((x / 64U) + (y / 64U)) % 2U) * 255U);
	}

	/* The conversion kernel: one invocation handles a 2x2 block (4 luma texels +
	 * 1 interleaved chroma pair), the exact structure of the CPU converters.
	 * TEST_PATTERN builds the source from the shared integer hash; the production
	 * variant (M4) will fetch the grabbed BGRA image instead. */
	constexpr auto ConversionShaderBody = R"GLSL(
layout(local_size_x = 8, local_size_y = 8) in;

layout(binding = 0, r8) writeonly uniform image2D lumaOut;
layout(binding = 1, rg8) writeonly uniform image2D chromaOut;

/* Structured test pattern (gradients + checkerboard): corruption is obvious to
   the eye AND the bytes stay deterministic for the CPU comparison. */
ivec3 sourceBGR (ivec2 coord, ivec2 size)
{
	const int blue = coord.x * 255 / max(size.x - 1, 1);
	const int green = coord.y * 255 / max(size.y - 1, 1);
	const int red = (((coord.x / 64) + (coord.y / 64)) % 2) * 255;

	return ivec3(blue, green, red);
}

void main ()
{
	const ivec2 blockCoord = ivec2(gl_GlobalInvocationID.xy);
	const ivec2 lumaSize = imageSize(lumaOut);
	const ivec2 base = blockCoord * 2;

	if ( base.x >= lumaSize.x || base.y >= lumaSize.y )
	{
		return;
	}

	int sumB = 0;
	int sumG = 0;
	int sumR = 0;

	for ( int dy = 0; dy < 2; dy++ )
	{
		for ( int dx = 0; dx < 2; dx++ )
		{
			const ivec2 coord = base + ivec2(dx, dy);
			const ivec3 bgr = sourceBGR(coord, lumaSize);
			const int luma = clamp(((Y_COEF_R * bgr.z + Y_COEF_G * bgr.y + Y_COEF_B * bgr.x + 128) >> 8) + 16, 0, 255);

			imageStore(lumaOut, coord, vec4(float(luma) / 255.0));

			sumB += bgr.x;
			sumG += bgr.y;
			sumR += bgr.z;
		}
	}

	const int avgB = sumB >> 2;
	const int avgG = sumG >> 2;
	const int avgR = sumR >> 2;

	const int chromaU = clamp(((U_COEF_R * avgR + U_COEF_G * avgG + U_COEF_B * avgB + 128) >> 8) + 128, 0, 255);
	const int chromaV = clamp(((V_COEF_R * avgR + V_COEF_G * avgG + V_COEF_B * avgB + 128) >> 8) + 128, 0, 255);

	imageStore(chromaOut, blockCoord, vec4(float(chromaU) / 255.0, float(chromaV) / 255.0, 0.0, 0.0));
}
)GLSL";

	/* Production variant: same kernel fed by the grabbed BGRA frame (nearest sampler,
	 * integer math identical to the CPU path). */
	constexpr auto ProductionShaderBody = R"GLSL(
layout(local_size_x = 8, local_size_y = 8) in;

layout(binding = 0, r8) writeonly uniform image2D lumaOut;
layout(binding = 1, rg8) writeonly uniform image2D chromaOut;
layout(binding = 2) uniform sampler2D srcFrame;

void main ()
{
	const ivec2 blockCoord = ivec2(gl_GlobalInvocationID.xy);
	const ivec2 lumaSize = imageSize(lumaOut);
	const ivec2 base = blockCoord * 2;

	if ( base.x >= lumaSize.x || base.y >= lumaSize.y )
	{
		return;
	}

	int sumB = 0;
	int sumG = 0;
	int sumR = 0;

	for ( int dy = 0; dy < 2; dy++ )
	{
		for ( int dx = 0; dx < 2; dx++ )
		{
			const ivec2 coord = base + ivec2(dx, dy);
			const vec4 texel = texelFetch(srcFrame, coord, 0);
			const int red = int(round(texel.r * 255.0));
			const int green = int(round(texel.g * 255.0));
			const int blue = int(round(texel.b * 255.0));
			const int luma = clamp(((Y_COEF_R * red + Y_COEF_G * green + Y_COEF_B * blue + 128) >> 8) + 16, 0, 255);

			imageStore(lumaOut, coord, vec4(float(luma) / 255.0));

			sumB += blue;
			sumG += green;
			sumR += red;
		}
	}

	const int avgB = sumB >> 2;
	const int avgG = sumG >> 2;
	const int avgR = sumR >> 2;

	const int chromaU = clamp(((U_COEF_R * avgR + U_COEF_G * avgG + U_COEF_B * avgB + 128) >> 8) + 128, 0, 255);
	const int chromaV = clamp(((V_COEF_R * avgR + V_COEF_G * avgG + V_COEF_B * avgB + 128) >> 8) + 128, 0, 255);

	imageStore(chromaOut, blockCoord, vec4(float(chromaU) / 255.0, float(chromaV) / 255.0, 0.0, 0.0));
}
)GLSL";

	VideoFrameConverter::VideoFrameConverter (const std::shared_ptr< Vulkan::Device > & device, Saphir::ShaderManager & shaderManager) noexcept
		: m_device{device},
		m_shaderManager{&shaderManager}
	{

	}

	VideoFrameConverter::~VideoFrameConverter ()
	{
		this->destroy();
	}

	bool
	VideoFrameConverter::create (uint32_t width, uint32_t height) noexcept
	{
		if ( width == 0 || height == 0 || width % 2 != 0 || height % 2 != 0 )
		{
			TraceError{ClassId} << "Invalid frame dimensions " << width << "x" << height << " (must be non-zero and even) !";

			return false;
		}

		m_width = width;
		m_height = height;

		/* Plane images: R8 luma (full res) + R8G8 chroma (half res, NV12 layout). */
		m_lumaImage = std::make_shared< Vulkan::Image >(
			m_device, VK_IMAGE_TYPE_2D, VK_FORMAT_R8_UNORM,
			VkExtent3D{width, height, 1},
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		);
		m_lumaImage->setIdentifier(ClassId, "LumaPlane", "Image");

		m_chromaImage = std::make_shared< Vulkan::Image >(
			m_device, VK_IMAGE_TYPE_2D, VK_FORMAT_R8G8_UNORM,
			VkExtent3D{width / 2, height / 2, 1},
			VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
		);
		m_chromaImage->setIdentifier(ClassId, "ChromaPlane", "Image");

		if ( !m_lumaImage->createOnHardware() || !m_chromaImage->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the plane images !");

			this->destroy();

			return false;
		}

		constexpr VkImageSubresourceRange fullRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

		m_lumaView = std::make_shared< Vulkan::ImageView >(m_lumaImage, VK_IMAGE_VIEW_TYPE_2D, fullRange);
		m_lumaView->setIdentifier(ClassId, "LumaPlane", "ImageView");

		m_chromaView = std::make_shared< Vulkan::ImageView >(m_chromaImage, VK_IMAGE_VIEW_TYPE_2D, fullRange);
		m_chromaView->setIdentifier(ClassId, "ChromaPlane", "ImageView");

		if ( !m_lumaView->createOnHardware() || !m_chromaView->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the plane image views !");

			this->destroy();

			return false;
		}

		/* Compute shader: shared BT.709 integer coefficients injected as defines. */
		const auto shaderSource = "#version 450\n" + VideoColor::glslDefines() + ConversionShaderBody;
		const auto shaderModule = m_shaderManager->getShaderModuleFromSourceCode(m_device, "VideoFrameConverterCS", Saphir::ShaderType::ComputeShader, shaderSource);

		if ( shaderModule == nullptr )
		{
			Tracer::error(ClassId, "Failed to compile the conversion compute shader !");

			this->destroy();

			return false;
		}

		/* Descriptor set layout: two storage images. */
		m_descriptorSetLayout = std::make_shared< Vulkan::DescriptorSetLayout >(m_device, "VideoFrameConverterDSLayout");

		for ( uint32_t bindingIndex = 0; bindingIndex < 2; bindingIndex++ )
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding = bindingIndex;
			binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding.descriptorCount = 1;
			binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

			m_descriptorSetLayout->declare(binding);
		}

		if ( !m_descriptorSetLayout->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the descriptor set layout !");

			this->destroy();

			return false;
		}

		m_pipelineLayout = std::make_shared< Vulkan::PipelineLayout >(
			m_device, "VideoFrameConverterPipelineLayout",
			StaticVector< std::shared_ptr< Vulkan::DescriptorSetLayout >, 6 >{m_descriptorSetLayout},
			StaticVector< VkPushConstantRange, 4 >{}
		);

		if ( !m_pipelineLayout->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the pipeline layout !");

			this->destroy();

			return false;
		}

		m_computePipeline = std::make_unique< Vulkan::ComputePipeline >(m_pipelineLayout);
		m_computePipeline->setShaderModule(shaderModule->handle());

		if ( !m_computePipeline->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the compute pipeline !");

			this->destroy();

			return false;
		}

		/* Descriptor pool and set.
		 * NOTE: FREE_DESCRIPTOR_SET_BIT is mandatory, Vulkan::DescriptorSet frees
		 * its set individually in its destructor (see src/Vulkan/AGENTS.md). */
		const std::vector< VkDescriptorPoolSize > poolSizes{
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
		};

		m_descriptorPool = std::make_shared< Vulkan::DescriptorPool >(m_device, poolSizes, 2, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

		if ( !m_descriptorPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Failed to create the descriptor pool !");

			this->destroy();

			return false;
		}

		m_descriptorSet = std::make_unique< Vulkan::DescriptorSet >(m_descriptorPool, m_descriptorSetLayout);

		if ( !m_descriptorSet->create() )
		{
			Tracer::error(ClassId, "Failed to allocate the descriptor set !");

			this->destroy();

			return false;
		}

		{
			const std::array< VkDescriptorImageInfo, 2 > imageInfos{{
				{VK_NULL_HANDLE, m_lumaView->handle(), VK_IMAGE_LAYOUT_GENERAL},
				{VK_NULL_HANDLE, m_chromaView->handle(), VK_IMAGE_LAYOUT_GENERAL}
			}};

			std::array< VkWriteDescriptorSet, 2 > writes{};

			for ( uint32_t bindingIndex = 0; bindingIndex < 2; bindingIndex++ )
			{
				writes[bindingIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[bindingIndex].dstSet = m_descriptorSet->handle();
				writes[bindingIndex].dstBinding = bindingIndex;
				writes[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				writes[bindingIndex].descriptorCount = 1;
				writes[bindingIndex].pImageInfo = &imageInfos[bindingIndex];
			}

			vkUpdateDescriptorSets(m_device->handle(), static_cast< uint32_t >(writes.size()), writes.data(), 0, nullptr);
		}

		/* Production pipeline: real BGRA source through a nearest sampler. */
		{
			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_NEAREST;
			samplerInfo.minFilter = VK_FILTER_NEAREST;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

			if ( vkCreateSampler(m_device->handle(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS )
			{
				Tracer::error(ClassId, "Failed to create the source sampler !");

				this->destroy();

				return false;
			}

			const auto productionSource = "#version 450\n" + VideoColor::glslDefines() + ProductionShaderBody;
			const auto productionModule = m_shaderManager->getShaderModuleFromSourceCode(m_device, "VideoFrameConverterProductionCS", Saphir::ShaderType::ComputeShader, productionSource);

			if ( productionModule == nullptr )
			{
				Tracer::error(ClassId, "Failed to compile the production conversion shader !");

				this->destroy();

				return false;
			}

			m_productionSetLayout = std::make_shared< Vulkan::DescriptorSetLayout >(m_device, "VideoFrameConverterProdDSLayout");

			for ( uint32_t bindingIndex = 0; bindingIndex < 2; bindingIndex++ )
			{
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = bindingIndex;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

				m_productionSetLayout->declare(binding);
			}

			{
				VkDescriptorSetLayoutBinding binding{};
				binding.binding = 2;
				binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

				m_productionSetLayout->declare(binding);
			}

			if ( !m_productionSetLayout->createOnHardware() )
			{
				Tracer::error(ClassId, "Failed to create the production descriptor set layout !");

				this->destroy();

				return false;
			}

			m_productionPipelineLayout = std::make_shared< Vulkan::PipelineLayout >(
				m_device, "VideoFrameConverterProdPipelineLayout",
				StaticVector< std::shared_ptr< Vulkan::DescriptorSetLayout >, 6 >{m_productionSetLayout},
				StaticVector< VkPushConstantRange, 4 >{}
			);

			if ( !m_productionPipelineLayout->createOnHardware() )
			{
				Tracer::error(ClassId, "Failed to create the production pipeline layout !");

				this->destroy();

				return false;
			}

			m_productionPipeline = std::make_unique< Vulkan::ComputePipeline >(m_productionPipelineLayout);
			m_productionPipeline->setShaderModule(productionModule->handle());

			if ( !m_productionPipeline->createOnHardware() )
			{
				Tracer::error(ClassId, "Failed to create the production compute pipeline !");

				this->destroy();

				return false;
			}

			m_productionSet = std::make_unique< Vulkan::DescriptorSet >(m_descriptorPool, m_productionSetLayout);

			if ( !m_productionSet->create() )
			{
				Tracer::error(ClassId, "Failed to allocate the production descriptor set !");

				this->destroy();

				return false;
			}

			const std::array< VkDescriptorImageInfo, 2 > imageInfos{{
				{VK_NULL_HANDLE, m_lumaView->handle(), VK_IMAGE_LAYOUT_GENERAL},
				{VK_NULL_HANDLE, m_chromaView->handle(), VK_IMAGE_LAYOUT_GENERAL}
			}};

			std::array< VkWriteDescriptorSet, 2 > writes{};

			for ( uint32_t bindingIndex = 0; bindingIndex < 2; bindingIndex++ )
			{
				writes[bindingIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writes[bindingIndex].dstSet = m_productionSet->handle();
				writes[bindingIndex].dstBinding = bindingIndex;
				writes[bindingIndex].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				writes[bindingIndex].descriptorCount = 1;
				writes[bindingIndex].pImageInfo = &imageInfos[bindingIndex];
			}

			vkUpdateDescriptorSets(m_device->handle(), static_cast< uint32_t >(writes.size()), writes.data(), 0, nullptr);
		}

		TraceSuccess{ClassId} << "GPU BGRA->I420 converter created (" << width << "x" << height << ", BT.709 integer path).";

		return true;
	}

	void
	VideoFrameConverter::destroy () noexcept
	{
		m_productionSet.reset();
		m_productionPipeline.reset();
		m_productionPipelineLayout.reset();
		m_productionSetLayout.reset();

		if ( m_sampler != VK_NULL_HANDLE )
		{
			vkDestroySampler(m_device->handle(), m_sampler, nullptr);
			m_sampler = VK_NULL_HANDLE;
		}

		m_descriptorSet.reset();
		m_descriptorPool.reset();
		m_computePipeline.reset();
		m_pipelineLayout.reset();
		m_descriptorSetLayout.reset();
		m_chromaView.reset();
		m_chromaImage.reset();
		m_lumaView.reset();
		m_lumaImage.reset();
		m_width = 0;
		m_height = 0;
	}

	bool
	VideoFrameConverter::dispatchConversion (bool production) noexcept
	{
		const auto commandPool = std::make_shared< Vulkan::CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, true, false);

		if ( !commandPool->createOnHardware() )
		{
			return false;
		}

		const auto commandBuffer = std::make_unique< Vulkan::CommandBuffer >(commandPool, true);

		if ( !commandBuffer->isCreated() || !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		/* UNDEFINED -> GENERAL for the compute writes. */
		for ( const auto & image : {m_lumaImage, m_chromaImage} )
		{
			const Vulkan::Sync::ImageMemoryBarrier barrier{
				*image,
				0,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL
			};

			commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
		}

		if ( production )
		{
			commandBuffer->bind(*m_productionPipeline);
			commandBuffer->bind(*m_productionSet, *m_productionPipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
		}
		else
		{
			commandBuffer->bind(*m_computePipeline);
			commandBuffer->bind(*m_descriptorSet, *m_pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
		}

		/* One invocation per 2x2 block. */
		const uint32_t groupCountX = (m_width / 2 + 7) / 8;
		const uint32_t groupCountY = (m_height / 2 + 7) / 8;

		commandBuffer->dispatch(groupCountX, groupCountY, 1);

		/* GENERAL -> TRANSFER_SRC for the readback (M3 will target the plane copies instead). */
		for ( const auto & image : {m_lumaImage, m_chromaImage} )
		{
			const Vulkan::Sync::ImageMemoryBarrier barrier{
				*image,
				VK_ACCESS_SHADER_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			};

			commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		}

		if ( !commandBuffer->end() )
		{
			return false;
		}

		auto * queue = m_device->getGraphicsQueue(Vulkan::QueuePriority::High);

		return queue != nullptr && queue->submit(*commandBuffer) && queue->waitIdle();
	}

	bool
	VideoFrameConverter::convertFrom (const std::shared_ptr< Vulkan::ImageView > & sourceView) noexcept
	{
		/* Point the sampler binding at this frame's source (synchronous use only:
		 * the previous dispatch has completed before the set is rewritten). */
		VkDescriptorImageInfo sourceInfo{};
		sourceInfo.sampler = m_sampler;
		sourceInfo.imageView = sourceView->handle();
		sourceInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = m_productionSet->handle();
		write.dstBinding = 2;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.descriptorCount = 1;
		write.pImageInfo = &sourceInfo;

		vkUpdateDescriptorSets(m_device->handle(), 1, &write, 0, nullptr);

		return this->dispatchConversion(true);
	}

	bool
	VideoFrameConverter::readbackPlanes (std::vector< uint8_t > & luma, std::vector< uint8_t > & chroma) noexcept
	{
		const VkDeviceSize lumaBytes = static_cast< VkDeviceSize >(m_width) * m_height;
		const VkDeviceSize chromaBytes = lumaBytes / 2;

		auto stagingBuffer = std::make_unique< Vulkan::Buffer >(m_device, static_cast< VkBufferCreateFlags >(0), lumaBytes + chromaBytes, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
		stagingBuffer->setHostReadable(true);

		if ( !stagingBuffer->createOnHardware() )
		{
			return false;
		}

		const auto commandPool = std::make_shared< Vulkan::CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, true, false);

		if ( !commandPool->createOnHardware() )
		{
			return false;
		}

		const auto commandBuffer = std::make_unique< Vulkan::CommandBuffer >(commandPool, true);

		if ( !commandBuffer->isCreated() || !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		{
			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
			region.imageExtent = {m_width, m_height, 1};

			vkCmdCopyImageToBuffer(commandBuffer->handle(), m_lumaImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer->handle(), 1, &region);

			region.bufferOffset = lumaBytes;
			region.imageExtent = {m_width / 2, m_height / 2, 1};

			vkCmdCopyImageToBuffer(commandBuffer->handle(), m_chromaImage->handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer->handle(), 1, &region);
		}

		if ( !commandBuffer->end() )
		{
			return false;
		}

		auto * queue = m_device->getGraphicsQueue(Vulkan::QueuePriority::High);

		if ( queue == nullptr || !queue->submit(*commandBuffer) || !queue->waitIdle() )
		{
			return false;
		}

		const auto * mapped = stagingBuffer->mapMemoryAs< uint8_t >();

		if ( mapped == nullptr )
		{
			return false;
		}

		luma.resize(lumaBytes);
		chroma.resize(chromaBytes);

		std::memcpy(luma.data(), mapped, lumaBytes);
		std::memcpy(chroma.data(), mapped + lumaBytes, chromaBytes);

		stagingBuffer->unmapMemory();

		return true;
	}

	bool
	VideoFrameConverter::debugDumpPlanes (const std::filesystem::path & basePath) noexcept
	{
		std::vector< uint8_t > luma;
		std::vector< uint8_t > chroma;

		if ( !this->readbackPlanes(luma, chroma) )
		{
			return false;
		}

		const auto writePGM = [] (const std::filesystem::path & path, const std::vector< uint8_t > & data, uint32_t width, uint32_t height) {
			std::FILE * file = std::fopen(path.c_str(), "wb");

			if ( file == nullptr )
			{
				return false;
			}

			std::fprintf(file, "P5\n%u %u\n255\n", width, height);
			std::fwrite(data.data(), 1, data.size(), file);
			std::fclose(file);

			return true;
		};

		auto lumaPath = basePath;
		lumaPath += "_luma.pgm";
		auto chromaPath = basePath;
		chromaPath += "_chroma.pgm";

		/* Chroma dumped as a double-width gray strip (U,V interleaved). */
		return writePGM(lumaPath, luma, m_width, m_height) && writePGM(chromaPath, chroma, m_width, m_height / 2);
	}

	bool
	VideoFrameConverter::selfTest (uint64_t & mismatchedBytes) noexcept
	{
		mismatchedBytes = 0;

		if ( !this->dispatchConversion(false) )
		{
			Tracer::error(ClassId, "The conversion dispatch failed !");

			return false;
		}

		std::vector< uint8_t > gpuLuma;
		std::vector< uint8_t > gpuChroma;

		if ( !this->readbackPlanes(gpuLuma, gpuChroma) )
		{
			Tracer::error(ClassId, "The plane readback failed !");

			return false;
		}

		/* CPU reference: the same procedural source through the same integer math. */
		using namespace VideoColor;

		std::vector< uint8_t > cpuLuma(gpuLuma.size());
		std::vector< uint8_t > cpuChroma(gpuChroma.size());

		const auto chromaWidth = m_width / 2;

		for ( uint32_t row = 0; row < m_height; row += 2 )
		{
			for ( uint32_t col = 0; col < m_width; col += 2 )
			{
				int32_t sumB = 0;
				int32_t sumG = 0;
				int32_t sumR = 0;

				for ( uint32_t dy = 0; dy < 2; dy++ )
				{
					for ( uint32_t dx = 0; dx < 2; dx++ )
					{
					int32_t blue = 0;
					int32_t green = 0;
					int32_t red = 0;

					testPatternBGR(col + dx, row + dy, m_width, m_height, blue, green, red);

						cpuLuma[(row + dy) * m_width + (col + dx)] = static_cast< uint8_t >(std::clamp(((YCoefR * red + YCoefG * green + YCoefB * blue + 128) >> 8) + 16, 0, 255));

						sumB += blue;
						sumG += green;
						sumR += red;
					}
				}

				const auto avgB = sumB >> 2;
				const auto avgG = sumG >> 2;
				const auto avgR = sumR >> 2;
				const auto chromaIndex = ((row / 2) * chromaWidth + (col / 2)) * 2;

				cpuChroma[chromaIndex] = static_cast< uint8_t >(std::clamp(((UCoefR * avgR + UCoefG * avgG + UCoefB * avgB + 128) >> 8) + 128, 0, 255));
				cpuChroma[chromaIndex + 1] = static_cast< uint8_t >(std::clamp(((VCoefR * avgR + VCoefG * avgG + VCoefB * avgB + 128) >> 8) + 128, 0, 255));
			}
		}

		for ( size_t index = 0; index < gpuLuma.size(); index++ )
		{
			if ( gpuLuma[index] != cpuLuma[index] )
			{
				mismatchedBytes++;
			}
		}

		for ( size_t index = 0; index < gpuChroma.size(); index++ )
		{
			if ( gpuChroma[index] != cpuChroma[index] )
			{
				mismatchedBytes++;
			}
		}

		if ( mismatchedBytes > 0 )
		{
			TraceError{ClassId} << "GPU/CPU conversion mismatch: " << mismatchedBytes << " bytes differ !";

			return false;
		}

		TraceSuccess{ClassId} << "GPU conversion matches the CPU reference byte-for-byte (" << gpuLuma.size() + gpuChroma.size() << " bytes compared).";

		return true;
	}
}
