/*
 * src/Graphics/GrabPass.cpp
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

#include "GrabPass.hpp"

/* STL inclusions. */
#include <vector>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	bool
	GrabPass::create (Renderer & renderer, uint32_t width, uint32_t height, VkFormat colorFormat, VkFormat depthFormat, VkFormat normalsFormat, VkFormat materialPropertiesFormat, VkFormat albedoFormat, VkFormat velocityFormat) noexcept
	{
		if ( this->isCreated() )
		{
			return true;
		}

		const auto device = renderer.device();

		/* Create the color grab pass image with transfer destination and sampled usage. */
		m_image = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D,
			colorFormat,
			VkExtent3D{width, height, 1},
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		);
		m_image->setIdentifier(ClassId, "Color", "Image");

		if ( !m_image->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the grab pass color image !";

			return false;
		}

		/* Transition color to shader read layout. */
		{
			const auto & transferManager = renderer.transferManager();

			if ( !transferManager.transitionImageLayout(
				*m_image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			) )
			{
				TraceError{ClassId} << "Unable to transition grab pass color image to shader read layout !";

				return false;
			}
		}

		m_image->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		/* Create the color image view. */
		m_imageView = std::make_shared< ImageView >(
			m_image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		);
		m_imageView->setIdentifier(ClassId, "Color", "ImageView");

		if ( !m_imageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the grab pass color image view !";

			return false;
		}

		/* Get or create the color sampler: linear filtering, clamp-to-edge. */
		m_sampler = renderer.getSampler("GrabPass", [] (Settings &, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.compareEnable = VK_FALSE;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = 1.0F;
		});

		if ( m_sampler == nullptr )
		{
			TraceError{ClassId} << "Unable to get the sampler for grab pass color !";

			return false;
		}

		/* Create the depth grab pass image if a depth format is specified. */
		if ( depthFormat != VK_FORMAT_UNDEFINED )
		{
			m_depthImage = std::make_shared< Image >(
				device,
				VK_IMAGE_TYPE_2D,
				depthFormat,
				VkExtent3D{width, height, 1},
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			);
			m_depthImage->setIdentifier(ClassId, "Depth", "Image");

			if ( !m_depthImage->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the grab pass depth image !";

				return false;
			}

			/* Transition depth to shader read layout. */
			{
				const auto & transferManager = renderer.transferManager();

				if ( !transferManager.transitionImageLayout(
					*m_depthImage,
					VK_IMAGE_ASPECT_DEPTH_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				) )
				{
					TraceError{ClassId} << "Unable to transition grab pass depth image to shader read layout !";

					return false;
				}
			}

			m_depthImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			/* Create the depth image view. */
			m_depthImageView = std::make_shared< ImageView >(
				m_depthImage,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			);
			m_depthImageView->setIdentifier(ClassId, "Depth", "ImageView");

			if ( !m_depthImageView->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the grab pass depth image view !";

				return false;
			}

			/* Get or create the depth sampler: nearest filtering, clamp-to-edge. */
			m_depthSampler = renderer.getSampler("GrabPassDepth", [] (Settings &, VkSamplerCreateInfo & createInfo) {
				createInfo.magFilter = VK_FILTER_NEAREST;
				createInfo.minFilter = VK_FILTER_NEAREST;
				createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
				createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.compareEnable = VK_FALSE;
				createInfo.minLod = 0.0F;
				createInfo.maxLod = 1.0F;
			});

			if ( m_depthSampler == nullptr )
			{
				TraceError{ClassId} << "Unable to get the sampler for grab pass depth !";

				return false;
			}

			TraceDebug{ClassId} << "Grab pass textures created (" << width << "x" << height << ") with depth.";
		}
		else
		{
			TraceDebug{ClassId} << "Grab pass texture created (" << width << "x" << height << ") without depth.";
		}

		/* Create the normals grab pass image if a normals format is specified. */
		if ( normalsFormat != VK_FORMAT_UNDEFINED )
		{
			m_normalsImage = std::make_shared< Image >(
				device,
				VK_IMAGE_TYPE_2D,
				normalsFormat,
				VkExtent3D{width, height, 1},
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			);
			m_normalsImage->setIdentifier(ClassId, "Normals", "Image");

			if ( !m_normalsImage->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the grab pass normals image !";

				return false;
			}

			/* Transition normals to shader read layout. */
			{
				const auto & transferManager = renderer.transferManager();

				if ( !transferManager.transitionImageLayout(
					*m_normalsImage,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				) )
				{
					TraceError{ClassId} << "Unable to transition grab pass normals image to shader read layout !";

					return false;
				}
			}

			m_normalsImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			/* Create the normals image view. */
			m_normalsImageView = std::make_shared< ImageView >(
				m_normalsImage,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			);
			m_normalsImageView->setIdentifier(ClassId, "Normals", "ImageView");

			if ( !m_normalsImageView->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the grab pass normals image view !";

				return false;
			}

			/* Get or create the normals sampler: nearest filtering, clamp-to-edge. */
			m_normalsSampler = renderer.getSampler("GrabPassNormals", [] (Settings &, VkSamplerCreateInfo & createInfo) {
				createInfo.magFilter = VK_FILTER_NEAREST;
				createInfo.minFilter = VK_FILTER_NEAREST;
				createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
				createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.compareEnable = VK_FALSE;
				createInfo.minLod = 0.0F;
				createInfo.maxLod = 1.0F;
			});

			if ( m_normalsSampler == nullptr )
			{
				TraceError{ClassId} << "Unable to get the sampler for grab pass normals !";

				return false;
			}
		}

		/* Create the material properties grab pass image if a format is specified. */
		if ( materialPropertiesFormat != VK_FORMAT_UNDEFINED )
		{
			m_materialPropertiesImage = std::make_shared< Image >(
				device,
				VK_IMAGE_TYPE_2D,
				materialPropertiesFormat,
				VkExtent3D{width, height, 1},
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			);
			m_materialPropertiesImage->setIdentifier(ClassId, "MaterialProperties", "Image");

			if ( !m_materialPropertiesImage->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the grab pass material properties image !";

				return false;
			}

			/* Transition material properties to shader read layout. */
			{
				const auto & transferManager = renderer.transferManager();

				if ( !transferManager.transitionImageLayout(
					*m_materialPropertiesImage,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				) )
				{
					TraceError{ClassId} << "Unable to transition grab pass material properties image to shader read layout !";

					return false;
				}
			}

			m_materialPropertiesImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			/* Create the material properties image view. */
			m_materialPropertiesImageView = std::make_shared< ImageView >(
				m_materialPropertiesImage,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			);
			m_materialPropertiesImageView->setIdentifier(ClassId, "MaterialProperties", "ImageView");

			if ( !m_materialPropertiesImageView->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the grab pass material properties image view !";

				return false;
			}

			/* Get or create the material properties sampler: nearest filtering, clamp-to-edge. */
			m_materialPropertiesSampler = renderer.getSampler("GrabPassMaterialProperties", [] (Settings &, VkSamplerCreateInfo & createInfo) {
				createInfo.magFilter = VK_FILTER_NEAREST;
				createInfo.minFilter = VK_FILTER_NEAREST;
				createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
				createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.compareEnable = VK_FALSE;
				createInfo.minLod = 0.0F;
				createInfo.maxLod = 1.0F;
			});

			if ( m_materialPropertiesSampler == nullptr )
			{
				TraceError{ClassId} << "Unable to get the sampler for grab pass material properties !";

				return false;
			}
		}

		/* Create the albedo grab pass image (optional). */
		if ( albedoFormat != VK_FORMAT_UNDEFINED )
		{
			m_albedoImage = std::make_shared< Image >(
				device,
				VK_IMAGE_TYPE_2D,
				albedoFormat,
				VkExtent3D{width, height, 1},
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			);
			m_albedoImage->setIdentifier(ClassId, "Albedo", "Image");

			if ( !m_albedoImage->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the grab pass albedo image !";

				return false;
			}

			/* Transition albedo to shader read layout. */
			{
				const auto & transferManager = renderer.transferManager();

				if ( !transferManager.transitionImageLayout(
					*m_albedoImage,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				) )
				{
					TraceError{ClassId} << "Unable to transition grab pass albedo image to shader read layout !";

					return false;
				}
			}

			m_albedoImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			/* Create the albedo image view. */
			m_albedoImageView = std::make_shared< ImageView >(
				m_albedoImage,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			);
			m_albedoImageView->setIdentifier(ClassId, "Albedo", "ImageView");

			if ( !m_albedoImageView->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the grab pass albedo image view !";

				return false;
			}

			/* Get or create the albedo sampler: nearest filtering, clamp-to-edge. */
			m_albedoSampler = renderer.getSampler("GrabPassAlbedo", [] (Settings &, VkSamplerCreateInfo & createInfo) {
				createInfo.magFilter = VK_FILTER_NEAREST;
				createInfo.minFilter = VK_FILTER_NEAREST;
				createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
				createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.compareEnable = VK_FALSE;
				createInfo.minLod = 0.0F;
				createInfo.maxLod = 1.0F;
			});

			if ( m_albedoSampler == nullptr )
			{
				TraceError{ClassId} << "Unable to get the sampler for grab pass albedo !";

				return false;
			}
		}

		/* Create the velocity grab pass image (optional, motion vectors). */
		if ( velocityFormat != VK_FORMAT_UNDEFINED )
		{
			m_velocityImage = std::make_shared< Image >(
				device,
				VK_IMAGE_TYPE_2D,
				velocityFormat,
				VkExtent3D{width, height, 1},
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			);
			m_velocityImage->setIdentifier(ClassId, "Velocity", "Image");

			if ( !m_velocityImage->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the grab pass velocity image !";

				return false;
			}

			/* Transition velocity to shader read layout. */
			{
				const auto & transferManager = renderer.transferManager();

				if ( !transferManager.transitionImageLayout(
					*m_velocityImage,
					VK_IMAGE_ASPECT_COLOR_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				) )
				{
					TraceError{ClassId} << "Unable to transition grab pass velocity image to shader read layout !";

					return false;
				}
			}

			m_velocityImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

			/* Create the velocity image view. */
			m_velocityImageView = std::make_shared< ImageView >(
				m_velocityImage,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			);
			m_velocityImageView->setIdentifier(ClassId, "Velocity", "ImageView");

			if ( !m_velocityImageView->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the grab pass velocity image view !";

				return false;
			}

			/* Get or create the velocity sampler: nearest filtering, clamp-to-edge
			 * (motion vectors must never be interpolated across geometry edges). */
			m_velocitySampler = renderer.getSampler("GrabPassVelocity", [] (Settings &, VkSamplerCreateInfo & createInfo) {
				createInfo.magFilter = VK_FILTER_NEAREST;
				createInfo.minFilter = VK_FILTER_NEAREST;
				createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
				createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				createInfo.compareEnable = VK_FALSE;
				createInfo.minLod = 0.0F;
				createInfo.maxLod = 1.0F;
			});

			if ( m_velocitySampler == nullptr )
			{
				TraceError{ClassId} << "Unable to get the sampler for grab pass velocity !";

				return false;
			}
		}

		return true;
	}

	void
	GrabPass::destroy () noexcept
	{
		m_velocitySampler.reset();
		m_velocityImageView.reset();
		m_velocityImage.reset();
		m_albedoSampler.reset();
		m_albedoImageView.reset();
		m_albedoImage.reset();
		m_materialPropertiesSampler.reset();
		m_materialPropertiesImageView.reset();
		m_materialPropertiesImage.reset();
		m_normalsSampler.reset();
		m_normalsImageView.reset();
		m_normalsImage.reset();
		m_depthSampler.reset();
		m_depthImageView.reset();
		m_depthImage.reset();
		m_sampler.reset();
		m_imageView.reset();
		m_image.reset();
	}

	bool
	GrabPass::recreate (Renderer & renderer, uint32_t width, uint32_t height, VkFormat colorFormat, VkFormat depthFormat, VkFormat normalsFormat, VkFormat materialPropertiesFormat, VkFormat albedoFormat, VkFormat velocityFormat) noexcept
	{
		this->destroy();

		return this->create(renderer, width, height, colorFormat, depthFormat, normalsFormat, materialPropertiesFormat, albedoFormat, velocityFormat);
	}

	void
	GrabPass::recordBlit (const CommandBuffer & commandBuffer, const Image & srcColorImage, const Image * srcDepthImage, const Image * srcNormalsImage, const Image * srcMaterialPropertiesImage) const noexcept
	{
		if ( !this->isCreated() )
		{
			return;
		}

		const bool copyDepth = srcDepthImage != nullptr && this->hasDepth();
		const bool copyNormals = srcNormalsImage != nullptr && this->hasNormals();
		const bool copyMaterialProperties = srcMaterialPropertiesImage != nullptr && this->hasMaterialProperties();

		/* The grab is expressed as TWO batched barriers around the copies instead of one
		 * pipelineBarrier() per transition (which serialized the GPU up to ~20 times):
		 * every source goes to TRANSFER_SRC and every destination to TRANSFER_DST in a
		 * single call, then the copies run back-to-back, then a single call restores
		 * everything. The stage masks are the union of the per-image stages — per-image
		 * precision is preserved by the access masks carried by each VkImageMemoryBarrier. */
		std::vector< VkImageMemoryBarrier > barriers;
		barriers.reserve(8);

		/* === Pre-copy batch: sources -> TRANSFER_SRC, destinations -> TRANSFER_DST. === */

		barriers.push_back(Sync::ImageMemoryBarrier{
			srcColorImage,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		}.get());

		barriers.push_back(Sync::ImageMemoryBarrier{
			*m_image,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		}.get());

		if ( copyDepth )
		{
			barriers.push_back(Sync::ImageMemoryBarrier{
				*srcDepthImage,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			}.get());

			barriers.push_back(Sync::ImageMemoryBarrier{
				*m_depthImage,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			}.get());
		}

		if ( copyNormals )
		{
			barriers.push_back(Sync::ImageMemoryBarrier{
				*srcNormalsImage,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			}.get());

			barriers.push_back(Sync::ImageMemoryBarrier{
				*m_normalsImage,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			}.get());
		}

		if ( copyMaterialProperties )
		{
			barriers.push_back(Sync::ImageMemoryBarrier{
				*srcMaterialPropertiesImage,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			}.get());

			barriers.push_back(Sync::ImageMemoryBarrier{
				*m_materialPropertiesImage,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			}.get());
		}

		{
			/* Destinations were last sampled by fragment shaders; color sources were last
			 * written as color attachments; the depth source by the late fragment tests. */
			VkPipelineStageFlags preSrcStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

			if ( copyDepth )
			{
				preSrcStages |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			}

			commandBuffer.pipelineBarrier(barriers, preSrcStages, VK_PIPELINE_STAGE_TRANSFER_BIT);
		}

		/* === Copies, back-to-back (no barrier needed between independent transfers). === */

		/* Color uses vkCmdBlitImage (formats may differ); depth uses vkCmdCopyImage as
		 * depth formats may not support blit on all GPUs; the rest are same-format copies. */
		commandBuffer.blitImage(
			srcColorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		if ( copyDepth )
		{
			commandBuffer.copyImage(
				*srcDepthImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*m_depthImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			);
		}

		if ( copyNormals )
		{
			commandBuffer.copyImage(
				*srcNormalsImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*m_normalsImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		if ( copyMaterialProperties )
		{
			commandBuffer.copyImage(
				*srcMaterialPropertiesImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*m_materialPropertiesImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);
		}

		/* === Post-copy batch: destinations -> SHADER_READ, sources restored for the
		 * post-process render pass. === */

		barriers.clear();

		barriers.push_back(Sync::ImageMemoryBarrier{
			*m_image,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		}.get());

		barriers.push_back(Sync::ImageMemoryBarrier{
			srcColorImage,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		}.get());

		if ( copyDepth )
		{
			barriers.push_back(Sync::ImageMemoryBarrier{
				*m_depthImage,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			}.get());

			barriers.push_back(Sync::ImageMemoryBarrier{
				*srcDepthImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			}.get());
		}

		if ( copyNormals )
		{
			barriers.push_back(Sync::ImageMemoryBarrier{
				*m_normalsImage,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			}.get());

			barriers.push_back(Sync::ImageMemoryBarrier{
				*srcNormalsImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}.get());
		}

		if ( copyMaterialProperties )
		{
			barriers.push_back(Sync::ImageMemoryBarrier{
				*m_materialPropertiesImage,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			}.get());

			barriers.push_back(Sync::ImageMemoryBarrier{
				*srcMaterialPropertiesImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			}.get());
		}

		{
			/* Destinations are sampled by fragment shaders; color sources resume as color
			 * attachments; the depth source resumes at the early fragment tests. */
			VkPipelineStageFlags postDstStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			if ( copyDepth )
			{
				postDstStages |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			}

			commandBuffer.pipelineBarrier(barriers, VK_PIPELINE_STAGE_TRANSFER_BIT, postDstStages);
		}
	}

	VkDescriptorImageInfo
	GrabPass::normalsDescriptorInfo () const noexcept
	{
		VkDescriptorImageInfo info{};
		info.sampler = m_normalsSampler ? m_normalsSampler->handle() : VK_NULL_HANDLE;
		info.imageView = m_normalsImageView ? m_normalsImageView->handle() : VK_NULL_HANDLE;
		info.imageLayout = m_normalsImage ? m_normalsImage->currentImageLayout() : VK_IMAGE_LAYOUT_UNDEFINED;

		return info;
	}

	VkDescriptorImageInfo
	GrabPass::materialPropertiesDescriptorInfo () const noexcept
	{
		VkDescriptorImageInfo info{};
		info.sampler = m_materialPropertiesSampler ? m_materialPropertiesSampler->handle() : VK_NULL_HANDLE;
		info.imageView = m_materialPropertiesImageView ? m_materialPropertiesImageView->handle() : VK_NULL_HANDLE;
		info.imageLayout = m_materialPropertiesImage ? m_materialPropertiesImage->currentImageLayout() : VK_IMAGE_LAYOUT_UNDEFINED;

		return info;
	}

	VkDescriptorImageInfo
	GrabPass::albedoDescriptorInfo () const noexcept
	{
		VkDescriptorImageInfo info{};
		info.sampler = m_albedoSampler ? m_albedoSampler->handle() : VK_NULL_HANDLE;
		info.imageView = m_albedoImageView ? m_albedoImageView->handle() : VK_NULL_HANDLE;
		info.imageLayout = m_albedoImage ? m_albedoImage->currentImageLayout() : VK_IMAGE_LAYOUT_UNDEFINED;

		return info;
	}

	VkDescriptorImageInfo
	GrabPass::velocityDescriptorInfo () const noexcept
	{
		VkDescriptorImageInfo info{};
		info.sampler = m_velocitySampler ? m_velocitySampler->handle() : VK_NULL_HANDLE;
		info.imageView = m_velocityImageView ? m_velocityImageView->handle() : VK_NULL_HANDLE;
		info.imageLayout = m_velocityImage ? m_velocityImage->currentImageLayout() : VK_IMAGE_LAYOUT_UNDEFINED;

		return info;
	}

	VkDescriptorImageInfo
	GrabPass::depthDescriptorInfo () const noexcept
	{
		VkDescriptorImageInfo info{};
		info.sampler = m_depthSampler ? m_depthSampler->handle() : VK_NULL_HANDLE;
		info.imageView = m_depthImageView ? m_depthImageView->handle() : VK_NULL_HANDLE;
		info.imageLayout = m_depthImage ? m_depthImage->currentImageLayout() : VK_IMAGE_LAYOUT_UNDEFINED;

		return info;
	}

	bool
	GrabPass::isCreated () const noexcept
	{
		if ( m_image == nullptr || !m_image->isCreated() )
		{
			return false;
		}

		if ( m_imageView == nullptr || !m_imageView->isCreated() )
		{
			return false;
		}

		if ( m_sampler == nullptr || !m_sampler->isCreated() )
		{
			return false;
		}

		return true;
	}

	TextureType
	GrabPass::type () const noexcept
	{
		return TextureType::Texture2D;
	}

	uint32_t
	GrabPass::dimensions () const noexcept
	{
		return 2;
	}

	bool
	GrabPass::isCubemapTexture () const noexcept
	{
		return false;
	}

	std::shared_ptr< Image >
	GrabPass::image () const noexcept
	{
		return m_image;
	}

	std::shared_ptr< ImageView >
	GrabPass::imageView () const noexcept
	{
		return m_imageView;
	}

	std::shared_ptr< Sampler >
	GrabPass::sampler () const noexcept
	{
		return m_sampler;
	}

	bool
	GrabPass::request3DTextureCoordinates () const noexcept
	{
		return false;
	}
}
