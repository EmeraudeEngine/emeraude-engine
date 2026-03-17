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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "GrabPass.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	bool
	GrabPass::create (Renderer & renderer, uint32_t width, uint32_t height, VkFormat colorFormat, VkFormat depthFormat, VkFormat normalsFormat, VkFormat materialPropertiesFormat) noexcept
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

			TraceSuccess{ClassId} << "Grab pass textures created (" << width << "x" << height << ") with depth.";
		}
		else
		{
			TraceSuccess{ClassId} << "Grab pass texture created (" << width << "x" << height << ") without depth.";
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

		return true;
	}

	void
	GrabPass::destroy () noexcept
	{
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
	GrabPass::recreate (Renderer & renderer, uint32_t width, uint32_t height, VkFormat colorFormat, VkFormat depthFormat, VkFormat normalsFormat, VkFormat materialPropertiesFormat) noexcept
	{
		this->destroy();

		return this->create(renderer, width, height, colorFormat, depthFormat, normalsFormat, materialPropertiesFormat);
	}

	void
	GrabPass::recordBlit (const CommandBuffer & commandBuffer, const Image & srcColorImage, const Image * srcDepthImage, const Image * srcNormalsImage, const Image * srcMaterialPropertiesImage) const noexcept
	{
		if ( !this->isCreated() )
		{
			return;
		}

		/* === Color blit === */

		/* 1. Barrier: swapchain color COLOR_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL */
		{
			const Sync::ImageMemoryBarrier srcBarrier{
				srcColorImage,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				srcBarrier,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT
			);
		}

		/* 2. Barrier: grab color texture SHADER_READ_ONLY_OPTIMAL -> TRANSFER_DST_OPTIMAL */
		{
			const Sync::ImageMemoryBarrier dstBarrier{
				*m_image,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				dstBarrier,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT
			);
		}

		/* 3. Blit: swapchain color -> grab color texture */
		commandBuffer.blitImage(
			srcColorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		/* 4. Barrier: grab color texture TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL */
		{
			const Sync::ImageMemoryBarrier dstBarrier{
				*m_image,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				dstBarrier,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			);
		}

		/* 5. Barrier: swapchain color TRANSFER_SRC_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL
		 * (ready for the post-process render pass). */
		{
			const Sync::ImageMemoryBarrier srcBarrier{
				srcColorImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				srcBarrier,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			);
		}

		/* === Depth copy === */
		if ( srcDepthImage != nullptr && this->hasDepth() )
		{
			/* 6. Barrier: swapchain depth DEPTH_STENCIL_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL */
			{
				const Sync::ImageMemoryBarrier srcBarrier{
					*srcDepthImage,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_ASPECT_DEPTH_BIT
				};

				commandBuffer.pipelineBarrier(
					srcBarrier,
					VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 7. Barrier: grab depth texture SHADER_READ_ONLY_OPTIMAL -> TRANSFER_DST_OPTIMAL */
			{
				const Sync::ImageMemoryBarrier dstBarrier{
					*m_depthImage,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_ASPECT_DEPTH_BIT
				};

				commandBuffer.pipelineBarrier(
					dstBarrier,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 8. Copy: swapchain depth -> grab depth texture (vkCmdCopyImage, no filtering).
			 * Depth formats may not support vkCmdBlitImage on all GPUs. */
			commandBuffer.copyImage(
				*srcDepthImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*m_depthImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_DEPTH_BIT
			);

			/* 9. Barrier: grab depth texture TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL */
			{
				const Sync::ImageMemoryBarrier dstBarrier{
					*m_depthImage,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_ASPECT_DEPTH_BIT
				};

				commandBuffer.pipelineBarrier(
					dstBarrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				);
			}

			/* 10. Barrier: swapchain depth TRANSFER_SRC_OPTIMAL -> DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			 * (ready for the post-process render pass). */
			{
				const Sync::ImageMemoryBarrier srcBarrier{
					*srcDepthImage,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
					VK_IMAGE_ASPECT_DEPTH_BIT
				};

				commandBuffer.pipelineBarrier(
					srcBarrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
				);
			}
		}

		/* === Normals copy === */
		if ( srcNormalsImage != nullptr && this->hasNormals() )
		{
			/* 11. Barrier: source normals COLOR_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL */
			{
				const Sync::ImageMemoryBarrier srcBarrier{
					*srcNormalsImage,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					srcBarrier,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 12. Barrier: grab normals texture SHADER_READ_ONLY_OPTIMAL -> TRANSFER_DST_OPTIMAL */
			{
				const Sync::ImageMemoryBarrier dstBarrier{
					*m_normalsImage,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					dstBarrier,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 13. Copy: source normals -> grab normals texture (same format, use vkCmdCopyImage). */
			commandBuffer.copyImage(
				*srcNormalsImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*m_normalsImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			/* 14. Barrier: grab normals texture TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL */
			{
				const Sync::ImageMemoryBarrier dstBarrier{
					*m_normalsImage,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					dstBarrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				);
			}

			/* 15. Barrier: source normals TRANSFER_SRC_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL
			 * (ready for the post-process render pass). */
			{
				const Sync::ImageMemoryBarrier srcBarrier{
					*srcNormalsImage,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					srcBarrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				);
			}
		}

		/* === Material properties copy === */
		if ( srcMaterialPropertiesImage != nullptr && this->hasMaterialProperties() )
		{
			/* 16. Barrier: source material properties COLOR_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL */
			{
				const Sync::ImageMemoryBarrier srcBarrier{
					*srcMaterialPropertiesImage,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					srcBarrier,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 17. Barrier: grab material properties texture SHADER_READ_ONLY_OPTIMAL -> TRANSFER_DST_OPTIMAL */
			{
				const Sync::ImageMemoryBarrier dstBarrier{
					*m_materialPropertiesImage,
					VK_ACCESS_SHADER_READ_BIT,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					dstBarrier,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VK_PIPELINE_STAGE_TRANSFER_BIT
				);
			}

			/* 18. Copy: source material properties -> grab material properties texture (same format, use vkCmdCopyImage). */
			commandBuffer.copyImage(
				*srcMaterialPropertiesImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				*m_materialPropertiesImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			);

			/* 19. Barrier: grab material properties texture TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL */
			{
				const Sync::ImageMemoryBarrier dstBarrier{
					*m_materialPropertiesImage,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					dstBarrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
				);
			}

			/* 20. Barrier: source material properties TRANSFER_SRC_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL
			 * (ready for the post-process render pass). */
			{
				const Sync::ImageMemoryBarrier srcBarrier{
					*srcMaterialPropertiesImage,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				};

				commandBuffer.pipelineBarrier(
					srcBarrier,
					VK_PIPELINE_STAGE_TRANSFER_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				);
			}
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