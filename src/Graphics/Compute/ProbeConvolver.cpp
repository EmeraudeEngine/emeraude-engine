/*
 * src/Graphics/Compute/ProbeConvolver.cpp
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

#include "ProbeConvolver.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>

/* Local inclusions. */
#include "Graphics/Compute/IBLBaker.hpp"
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/Sampler.hpp"

namespace
{
	/* Same per-texel budgets as the sky bake (IBLBaker): the filtered importance
	 * sampling reads the scratch mips by solid-angle ratio, which makes these enough. */
	constexpr uint32_t PrefilterBaseSampleCount{64};
	constexpr uint32_t PrefilterSampleCountPerMip{32};
}

namespace EmEn::Graphics::Compute
{
	using namespace Vulkan;

	ProbeConvolver::~ProbeConvolver () noexcept = default;

	bool
	ProbeConvolver::create (Renderer & renderer, const std::shared_ptr< Vulkan::Image > & probeImage) noexcept
	{
		if ( probeImage == nullptr || !probeImage->isCreated() )
		{
			Tracer::error(ClassId, "The probe image is not created !");

			return false;
		}

		const auto & createInfo = probeImage->createInfo();

		if ( createInfo.arrayLayers != 6 || createInfo.mipLevels < 2 )
		{
			Tracer::error(ClassId, "The probe image must be a cubemap with at least 2 mips !");

			return false;
		}

		auto & baker = renderer.IBLBaker();

		if ( !baker.ensureEnvironmentPipelines() )
		{
			Tracer::error(ClassId, "The IBL baker environment pipelines are unavailable !");

			return false;
		}

		m_pipelineLayout = baker.environmentPipelineLayout();
		m_prefilterPipeline = baker.prefilterPipeline();

		m_probeImage = probeImage;
		m_probeSize = createInfo.extent.width;
		m_probeMipLevels = createInfo.mipLevels;

		const auto device = probeImage->device();

		/* Scratch cubemap: half the probe size, one mip per probe UPPER mip. It receives a
		 * plain blit cascade of the probe's mirror render and serves as the SOURCE of the
		 * filtered importance sampling (reading the probe's own mips while writing them
		 * would be a hazard AND a bias — prefiltered mips are not a plain chain). */
		m_scratchSize = std::max(1U, m_probeSize / 2U);
		m_scratchMipLevels = m_probeMipLevels - 1U;

		m_scratchImage = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D,
			createInfo.format,
			VkExtent3D{m_scratchSize, m_scratchSize, 1},
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
			m_scratchMipLevels,
			6
		);
		m_scratchImage->setIdentifier(ClassId, "ConvolutionScratch", "Image");

		if ( !m_scratchImage->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the convolution scratch cubemap !");

			m_scratchImage.reset();

			return false;
		}

		m_scratchCubeView = std::make_shared< ImageView >(
			m_scratchImage,
			VK_IMAGE_VIEW_TYPE_CUBE,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = m_scratchMipLevels,
				.baseArrayLayer = 0,
				.layerCount = 6
			}
		);
		m_scratchCubeView->setIdentifier(ClassId, "ConvolutionScratch", "ImageView");

		if ( !m_scratchCubeView->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the convolution scratch cube view !");

			return false;
		}

		/* Trilinear sampler over the whole scratch chain (the FIS picks its source LOD). */
		m_scratchSampler = renderer.getSampler("ProbeConvolverScratch", [] (Settings & /*settings*/, VkSamplerCreateInfo & samplerCreateInfo) {
			samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
			samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;
		});

		if ( m_scratchSampler == nullptr )
		{
			Tracer::error(ClassId, "Unable to create the convolution scratch sampler !");

			return false;
		}

		/* Per DEST mip (probe mips 1..N-1): a storage view and a descriptor set
		 * {scratch cube sampled, probe mip storage}. */
		const uint32_t destMipCount = m_probeMipLevels - 1U;

		const std::vector< VkDescriptorPoolSize > poolSizes{
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, destMipCount},
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, destMipCount}
		};

		m_descriptorPool = std::make_shared< DescriptorPool >(device, poolSizes, destMipCount, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

		if ( !m_descriptorPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the convolution descriptor pool !");

			return false;
		}

		m_mipStorageViews.reserve(destMipCount);
		m_mipDescriptorSets.reserve(destMipCount);

		for ( uint32_t destMip = 1; destMip < m_probeMipLevels; destMip++ )
		{
			auto storageView = std::make_shared< ImageView >(
				m_probeImage,
				VK_IMAGE_VIEW_TYPE_2D_ARRAY,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = destMip,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 6
				}
			);
			storageView->setIdentifier(ClassId, "ConvolutionMip" + std::to_string(destMip), "ImageView");

			if ( !storageView->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create a probe mip storage view !");

				m_mipStorageViews.clear();
				m_mipDescriptorSets.clear();

				return false;
			}

			auto descriptorSet = std::make_unique< DescriptorSet >(m_descriptorPool, baker.environmentDSLayout());

			if ( !descriptorSet->create() )
			{
				Tracer::error(ClassId, "Unable to allocate a convolution descriptor set !");

				m_mipStorageViews.clear();
				m_mipDescriptorSets.clear();

				return false;
			}

			VkDescriptorImageInfo sourceInfo{};
			sourceInfo.sampler = m_scratchSampler->handle();
			sourceInfo.imageView = m_scratchCubeView->handle();
			sourceInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorImageInfo destInfo{};
			destInfo.sampler = VK_NULL_HANDLE;
			destInfo.imageView = storageView->handle();
			destInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

			std::array< VkWriteDescriptorSet, 2 > writes{};

			writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[0].dstSet = descriptorSet->handle();
			writes[0].dstBinding = 0;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].descriptorCount = 1;
			writes[0].pImageInfo = &sourceInfo;

			writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writes[1].dstSet = descriptorSet->handle();
			writes[1].dstBinding = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			writes[1].descriptorCount = 1;
			writes[1].pImageInfo = &destInfo;

			vkUpdateDescriptorSets(device->handle(), static_cast< uint32_t >(writes.size()), writes.data(), 0, nullptr);

			m_mipStorageViews.emplace_back(std::move(storageView));
			m_mipDescriptorSets.emplace_back(std::move(descriptorSet));
		}

		TraceInfo{ClassId} <<
			"Probe GGX convolution ready: probe " << m_probeSize << "px² x" << m_probeMipLevels << " mips, "
			"scratch " << m_scratchSize << "px² x" << m_scratchMipLevels << " mips.";

		return true;
	}

	void
	ProbeConvolver::record (const Vulkan::CommandBuffer & commandBuffer) const noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		const auto cmdBuf = commandBuffer.handle();
		const auto probeHandle = m_probeImage->handle();
		const auto scratchHandle = m_scratchImage->handle();

		const auto imageBarrier = [&] (VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, uint32_t baseMip, uint32_t mipCount) {
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.srcAccessMask = srcAccess;
			barrier.dstAccessMask = dstAccess;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange = VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = baseMip,
				.levelCount = mipCount,
				.baseArrayLayer = 0,
				.layerCount = 6
			};

			vkCmdPipelineBarrier(cmdBuf, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		};

		const auto blit = [&] (VkImage sourceImage, uint32_t sourceMip, uint32_t sourceSize, VkImage destImage, uint32_t destMip, uint32_t destSize) {
			VkImageBlit region{};
			region.srcSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, sourceMip, 0, 6};
			region.srcOffsets[1] = VkOffset3D{static_cast< int32_t >(sourceSize), static_cast< int32_t >(sourceSize), 1};
			region.dstSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, destMip, 0, 6};
			region.dstOffsets[1] = VkOffset3D{static_cast< int32_t >(destSize), static_cast< int32_t >(destSize), 1};

			vkCmdBlitImage(cmdBuf, sourceImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);
		};

		/* 1. Probe mip 0 (fresh render, SHADER_READ_ONLY per the render pass) -> blit source. */
		imageBarrier(
			probeHandle, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 1
		);

		/* 2. Scratch: discard previous content, receive the cascade. The UNDEFINED source
		 * layout is deliberate — the whole chain is rewritten below. */
		imageBarrier(
			scratchHandle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			0, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, m_scratchMipLevels
		);

		/* 3. Blit cascade: probe mirror -> scratch mip 0 -> ... -> scratch last mip. */
		blit(probeHandle, 0, m_probeSize, scratchHandle, 0, m_scratchSize);

		for ( uint32_t mip = 1; mip < m_scratchMipLevels; mip++ )
		{
			imageBarrier(
				scratchHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				mip - 1, 1
			);

			blit(scratchHandle, mip - 1, std::max(1U, m_scratchSize >> (mip - 1U)), scratchHandle, mip, std::max(1U, m_scratchSize >> mip));
		}

		/* 4. Scratch chain -> sampled source for the prefilter dispatches. */
		if ( m_scratchMipLevels > 1 )
		{
			imageBarrier(
				scratchHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				0, m_scratchMipLevels - 1
			);
		}

		imageBarrier(
			scratchHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			m_scratchMipLevels - 1, 1
		);

		/* 5. Probe mip 0 back to sampled (materials read the mirror level directly). */
		imageBarrier(
			probeHandle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 1
		);

		/* 6. Probe upper mips: discard and open for compute writes. */
		imageBarrier(
			probeHandle, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
			0, VK_ACCESS_SHADER_WRITE_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			1, m_probeMipLevels - 1
		);

		/* 7. GGX prefilter: one dispatch per upper mip, roughness k/(N-1) — the exact chain
		 * semantics of the sky IBL, sampled with roughness x (N-1) by the materials. */
		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_prefilterPipeline->handle());

		for ( uint32_t destMip = 1; destMip < m_probeMipLevels; destMip++ )
		{
			const uint32_t destSize = std::max(1U, m_probeSize >> destMip);

			IBLBaker::EnvironmentPushConstants pushConstants{};
			pushConstants.sourceSize = m_scratchSize;
			pushConstants.destSize = destSize;
			pushConstants.sampleCount = PrefilterBaseSampleCount + (PrefilterSampleCountPerMip * destMip);
			pushConstants.roughness = static_cast< float >(destMip) / static_cast< float >(m_probeMipLevels - 1);

			const auto descriptorSetHandle = m_mipDescriptorSets[destMip - 1]->handle();

			vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout->handle(), 0, 1, &descriptorSetHandle, 0, nullptr);
			vkCmdPushConstants(cmdBuf, m_pipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(IBLBaker::EnvironmentPushConstants), &pushConstants);

			const uint32_t groupCount = (destSize + 7U) / 8U;
			vkCmdDispatch(cmdBuf, groupCount, groupCount, 6);
		}

		/* 8. Probe upper mips -> sampled: the whole image is SHADER_READ_ONLY again, the
		 * layout the render pass and the material descriptors expect. */
		imageBarrier(
			probeHandle, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			1, m_probeMipLevels - 1
		);
	}
}
