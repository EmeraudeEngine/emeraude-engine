/*
 * src/Overlay/Surface.cpp
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

#include "Surface.hpp"

/* Local inclusions. */
#include "Vulkan/Device.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/MemoryRegion.hpp"
#include "Graphics/Renderer.hpp"
#include "Libs/PixelFactory/Processor.hpp"
#include "Manager.hpp"
#include "Tracer.hpp"

namespace EmEn::Overlay
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Graphics;
	using namespace Vulkan;

	bool
	Surface::isBelowPoint (float positionX, float positionY) const noexcept
	{
		/* NOTE: Check on X axis */
		{
			const auto screenWidth = static_cast< float >(m_framebufferProperties.width());

			const auto positionXa = screenWidth * m_rectangle.left();

			if ( positionX < positionXa )
			{
				return false;
			}

			const auto positionXb = screenWidth * m_rectangle.right();

			if ( positionX > positionXb )
			{
				return false;
			}
		}

		/* NOTE: Check on Y axis */
		{
			const auto screenHeight = static_cast< float >(m_framebufferProperties.height());

			const auto positionYa = screenHeight * m_rectangle.top();

			if ( positionY < positionYa )
			{
				return false;
			}

			const auto positionYb = screenHeight * m_rectangle.bottom();

			if ( positionY > positionYb )
			{
				return false;
			}
		}

		return true;
	}

	bool
	Surface::isEventBlocked (float screenX, float screenY) const noexcept
	{
		/* The test is not required at all. */
		if ( !this->isBlockingEvent() )
		{
			return false;
		}

		/* NOTE: The alpha testing is disabled, so whatever the position is, it's blocked. */
		if ( !this->isBlockingEventWithAlphaTest() )
		{
			return true;
		}

		/* Get the pixel coordinates on the surface. */
		const auto surfaceX = static_cast< uint32_t >(screenX - (static_cast< float >(m_framebufferProperties.width()) * m_rectangle.left()));
		const auto surfaceY = static_cast< uint32_t >(screenY - (static_cast< float >(m_framebufferProperties.height()) * m_rectangle.top()));

		/* Get that pixel color from the pixmap. */
		const auto pixelColor = m_activeBuffer.pixmap.safePixel(surfaceX, surfaceY);
		const auto blocked = pixelColor.alpha() > m_alphaThreshold;

		return blocked;
	}

	void
	Surface::updateModelMatrix () noexcept
	{
		const auto xPosition = (-1.0F + m_rectangle.width()) + (m_rectangle.left() * 2.0F);
		const auto yPosition = (-1.0F + m_rectangle.height()) + (m_rectangle.top() * 2.0F);

		m_modelMatrix.reset();
		m_modelMatrix *= Matrix< 4, float >::translation(xPosition, yPosition, m_depth);
		m_modelMatrix *= Matrix< 4, float >::scaling(m_rectangle.width(), m_rectangle.height(), 1.0F);
	}

	bool
	Surface::createOnHardware (Renderer & renderer) noexcept
	{
		const auto & framebuffer = this->framebufferProperties();
		const auto & geometry = this->geometry();

		const auto textureWidth = framebuffer.getSurfaceWidth(geometry.width());
		const auto textureHeight = framebuffer.getSurfaceHeight(geometry.height());

		/* NOTE: When memory mapping is enabled, we skip the local pixmap entirely.
		 * The caller will write directly to the GPU-mapped memory. */
		if ( !m_memoryMappingEnabled )
		{
			if ( !m_activeBuffer.pixmap.initialize(textureWidth, textureHeight, ChannelMode::RGBA) )
			{
				TraceError{ClassId} << "Unable to initialize a " << textureWidth << "x" << textureHeight << "px pixmap for the surface '" << this->name() << "' !";

				return false;
			}
		}

		if ( m_sampler == nullptr || !m_sampler->isCreated() )
		{
			if ( !this->getSampler(renderer) )
			{
				return false;
			}
		}

		if ( !this->createFramebufferResources(m_activeBuffer, renderer, textureWidth, textureHeight) )
		{
			m_activeBuffer.destroy();

			return false;
		}

		m_videoMemorySizeValid = true;
		m_videoMemoryUpToDate = true;

		this->onActiveBufferReady(m_activeBuffer);

		return true;
	}

	bool
	Surface::destroyFromHardware () noexcept
	{
		/* NOTE: Cleaning both buffers. */
		m_transitionBuffer.destroy();
		m_activeBuffer.destroy();

		if ( m_sampler != nullptr )
		{
			m_sampler->destroyFromHardware();
			m_sampler.reset();
		}

		return true;
	}

	bool
	Surface::processUpdates (Renderer & renderer) noexcept
	{
		if ( !m_framebufferAccess.try_lock() )
		{
			return true;
		}

		/* Step 1: Handle size changes.
		 * This is triggered by invalidate() from window resize or manual setSize()/setGeometry(). */
		if ( !this->isVideoMemorySizeValid() )
		{
			this->updateModelMatrix();

			if ( !this->updatePhysicalRepresentation(renderer) )
			{
				TraceError{ClassId} << "Unable to update the physical representation of surface '" << this->name() << "' !";

				m_framebufferAccess.unlock();

				return false;
			}

			m_videoMemorySizeValid = true;
			m_videoMemoryUpToDate = false;
		}

		/* Step 2: Upload active buffer content to GPU.
		 * This uploads the active pixmap data to the GPU when setVideoMemoryOutdated() was called.
		 * NOTE: When memory mapping is enabled, the caller writes directly to the GPU, so we skip this step. */
		if ( !m_memoryMappingEnabled && m_activeBuffer.image != nullptr && !this->isVideoMemoryUpToDate() )
		{
			const MemoryRegion memoryRegion{
				m_activeBuffer.pixmap.data().data(),
				m_activeBuffer.pixmap.bytes()
			};

			if ( !m_activeBuffer.image->writeData(renderer.transferManager(), memoryRegion) )
			{
				TraceError{ClassId} << "Unable to update the content of surface '" << this->name() << "' !";

				m_framebufferAccess.unlock();

				return false;
			}

			m_videoMemoryUpToDate = true;
		}

		m_framebufferAccess.unlock();

		return true;
	}

	bool
	Surface::getSampler (Renderer & renderer) noexcept
	{
		m_sampler = renderer.getSampler("OverlaySurface", [] (Settings &, VkSamplerCreateInfo & createInfo) {
			//createInfo.flags = 0;
			//createInfo.magFilter = VK_FILTER_NEAREST;
			//createInfo.minFilter = VK_FILTER_NEAREST;
			//createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			//createInfo.mipLodBias = 0.0F;
			//createInfo.anisotropyEnable = VK_FALSE;
			//createInfo.maxAnisotropy = 1.0F;
			//createInfo.compareEnable = VK_FALSE;
			//createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			//createInfo.minLod = 0.0F;
			//createInfo.maxLod = VK_LOD_CLAMP_NONE;
			createInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
			//createInfo.unnormalizedCoordinates = VK_FALSE;
		});

		if ( m_sampler == nullptr )
		{
			TraceError{ClassId} << "Unable to get a sampler for the surface '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	Surface::createFramebufferResources (Framebuffer & buffer, Renderer & renderer, uint32_t width, uint32_t height) const noexcept
	{
		/* NOTE: When memory mapping is disabled, the pixmap is required.
		 * When memory mapping is enabled, we skip the pixmap entirely. */
		if ( !m_memoryMappingEnabled && !buffer.pixmap.isValid() )
		{
			TraceError{ClassId} << "The framebuffer local pixmap is invalid for the surface '" << this->name() << "' ! Unable to create the image for the GPU.";

			return false;
		}

		if ( buffer.image != nullptr && buffer.image->isCreated() )
		{
			TraceError{ClassId} << "The framebuffer image is already created for the surface '" << this->name() << "' ! Destroy it before.";

			return false;
		}

		/* Create the Vulkan image.
		 * NOTE: When memory mapping is enabled, we use LINEAR tiling to allow direct CPU access.
		 * This trades some GPU sampling performance for zero-copy writes from CPU. */
		const auto imageTiling = m_memoryMappingEnabled ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
		const auto imageUsage = m_memoryMappingEnabled ?
			VK_IMAGE_USAGE_SAMPLED_BIT : /* NOTE: No transfer needed when mapping directly. */
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		/* NOTE: RGBA format (4 channels) is always used for overlay surfaces. */
		constexpr uint32_t colorCount = 4;

		buffer.image = std::make_shared< Vulkan::Image >(
			renderer.device(),
			VK_IMAGE_TYPE_2D,
			Image::getFormat< uint8_t >(colorCount),
			VkExtent3D{width, height, 1U},
			imageUsage,
			0, /* createFlags */
			1, /* mipLevels */
			1, /* arrayLayers */
			VK_SAMPLE_COUNT_1_BIT,
			imageTiling,
			m_memoryMappingEnabled /* hostVisible */
		);
		buffer.image->setIdentifier(ClassId, this->name(), "Image");

		if ( m_memoryMappingEnabled )
		{
			/* NOTE: When memory mapping is enabled, just create the image on hardware.
			 * The caller will write directly to the mapped memory. */
			if ( !buffer.image->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the framebuffer image for the surface '" << this->name() << "' !";

				buffer.image.reset();

				return false;
			}

			/* NOTE: Transition the image layout to SHADER_READ_ONLY_OPTIMAL so it can be sampled.
			 * Unlike the staging buffer path, we don't go through transfer operations. */
			if ( !renderer.transferManager().transitionImageLayout(
				*buffer.image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) )
			{
				TraceError{ClassId} << "Unable to transition the image layout for the surface '" << this->name() << "' !";

				buffer.image.reset();

				return false;
			}
		}
		else
		{
			/* NOTE: Standard path: create image and upload pixmap data. */
			if ( !buffer.image->create(renderer.transferManager(), buffer.pixmap) )
			{
				TraceError{ClassId} << "Unable to create the framebuffer image for the surface '" << this->name() << "' !";

				buffer.image.reset();

				return false;
			}
		}

		/* Create the Vulkan image view. */
		buffer.imageView = std::make_shared< ImageView >(
			buffer.image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = buffer.image->createInfo().mipLevels,
				.baseArrayLayer = 0,
				.layerCount = buffer.image->createInfo().arrayLayers
			}
		);
		buffer.imageView->setIdentifier(ClassId, this->name(), "ImageView");

		if ( !buffer.imageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the framebuffer image view for the surface '" << this->name() << "' !";

			return false;
		}

		/* Create the descriptor set. */
		const auto descriptorSetLayout = Manager::getDescriptorSetLayout(renderer.layoutManager());

		if ( descriptorSetLayout == nullptr )
		{
			TraceError{ClassId} << "Unable to get the overlay descriptor set layout for the surface '" << this->name() << "' !";

			return false;
		}

		buffer.descriptorSet = std::make_unique< DescriptorSet >(renderer.descriptorPool(), descriptorSetLayout);
		buffer.descriptorSet->setIdentifier(ClassId, this->name(), "DescriptorSet");

		if ( !buffer.descriptorSet->create() )
		{
			buffer.descriptorSet.reset();

			TraceError{ClassId} << "Unable to create the surface descriptor set for the surface '" << this->name() << "' !";

			return false;
		}

		if ( !buffer.descriptorSet->writeCombinedImageSampler(0, *buffer.image, *buffer.imageView, *m_sampler) )
		{
			TraceError{ClassId} << "Unable to write to the surface descriptor set of the surface '" << this->name() << "' !";

			return false;
		}

		return true;
	}

	bool
	Surface::isTransitionBufferReady () const noexcept
	{
		if ( !m_transitionBufferEnabled )
		{
			TraceWarning{ClassId} << "The surface '" << this->name() << "' is not using the transition buffer mode !";

			return false;
		}

		return m_transitionBuffer.isValid() && m_transitionBufferStatus != TransitionBufferStatus::Resizing;
	}

	bool
	Surface::commitTransitionBuffer () noexcept
	{
		if ( !m_transitionBufferEnabled )
		{
			TraceWarning{ClassId} << "The surface '" << this->name() << "' is not using the transition buffer mode !";

			return false;
		}

		const std::lock_guard< std::mutex > lock{m_framebufferAccess};

		if ( !m_transitionBuffer.isValid() )
		{
			TraceError{ClassId} << "The surface '" << this->name() << "' transition buffer is invalid !";

			return false;
		}

		/* NOTE: Swap the buffer structures (transition becomes active, active becomes transition). */
		std::swap(m_transitionBuffer, m_activeBuffer);

		/* NOTE: After commit, the transition buffer status returns to Ready for next resize. */
		m_transitionBufferStatus = TransitionBufferStatus::Ready;

		/* NOTE: When memory mapping is enabled, the caller writes directly to GPU memory,
		 * so the video memory is already up to date. When disabled, mark as outdated
		 * so the pixmap gets uploaded via staging buffer. */
		if ( !m_memoryMappingEnabled )
		{
			m_videoMemoryUpToDate = false;
		}

		return true;
	}

	bool
	Surface::updatePhysicalRepresentation (Renderer & renderer) noexcept
	{
		const auto & framebuffer = this->framebufferProperties();
		const auto & geometry = this->geometry();

		const auto textureWidth = framebuffer.getSurfaceWidth(geometry.width());
		const auto textureHeight = framebuffer.getSurfaceHeight(geometry.height());

		/* NOTE: Check if resize is actually needed. */
		if ( m_activeBuffer.matchesSize(textureWidth, textureHeight) )
		{
			return true;
		}

		if ( m_transitionBufferEnabled )
		{
			/* DOUBLE BUFFER MODE: Prepare transition buffer with new size while
			 * active buffer continues to be used for rendering. */

			/* NOTE: Signal that resize is in progress (drawing not allowed during recreation). */
			m_transitionBufferStatus = TransitionBufferStatus::Resizing;

			/* NOTE: When memory mapping is enabled, skip the pixmap entirely.
			 * When disabled, copy and resize the active buffer content to the transition buffer
			 * to have a placeholder image while waiting for new content. */
			if ( !m_memoryMappingEnabled )
			{
				if ( !m_disablePixmapCopyInTransitionBuffer && m_activeBuffer.pixmap.isValid() )
				{
					m_transitionBuffer.pixmap = Processor< uint8_t >::resize(
						m_activeBuffer.pixmap,
						textureWidth,
						textureHeight,
						FilteringMode::Linear
					);

					if ( !m_transitionBuffer.pixmap.isValid() )
					{
						TraceWarning{ClassId} << "Unable to resize the active pixmap to transition buffer for the surface '" << this->name() << "'. Initializing empty.";

						if ( !m_transitionBuffer.pixmap.initialize(textureWidth, textureHeight, ChannelMode::RGBA) )
						{
							TraceError{ClassId} << "Unable to initialize the transition pixmap for the surface '" << this->name() << "' !";

							return false;
						}
					}
				}
				else
				{
					if ( !m_transitionBuffer.pixmap.initialize(textureWidth, textureHeight, ChannelMode::RGBA) )
					{
						TraceError{ClassId} << "Unable to initialize the transition pixmap for the surface '" << this->name() << "' !";

						return false;
					}
				}
			}

			/* NOTE: Wait for GPU to finish using the old transition resources before destroying them. */
			renderer.device()->waitIdle("Surface::updatePhysicalRepresentation() - transition buffer");

			m_transitionBuffer.destroy();

			if ( !this->createFramebufferResources(m_transitionBuffer, renderer, textureWidth, textureHeight) )
			{
				m_transitionBuffer.destroy();

				return false;
			}

			/* NOTE: Set status based on whether we have placeholder content or not.
			 * If pixmap copy is disabled or memory mapping is enabled, the buffer is empty and waiting for content.
			 * If pixmap copy is enabled, the buffer has a resized placeholder and is ready. */
			if ( m_disablePixmapCopyInTransitionBuffer || m_memoryMappingEnabled )
			{
				m_transitionBufferStatus = TransitionBufferStatus::WaitingForContent;
			}
			else
			{
				m_transitionBufferStatus = TransitionBufferStatus::Ready;
			}

			/* NOTE: Notify derived classes that the transition buffer is ready for content. */
			this->onTransitionBufferReady(m_transitionBuffer);

			return true;
		}

		/* SINGLE BUFFER MODE: Recreate active buffer directly (blocking). */
		if ( !m_memoryMappingEnabled )
		{
			if ( !m_activeBuffer.pixmap.initialize(textureWidth, textureHeight, ChannelMode::RGBA) )
			{
				TraceError{ClassId} << "Unable to resize the active pixmap for the surface '" << this->name() << "' !";

				return false;
			}
		}

		renderer.device()->waitIdle("Surface::updatePhysicalRepresentation() - active buffer");

		m_activeBuffer.destroy();

		if ( !this->createFramebufferResources(m_activeBuffer, renderer, textureWidth, textureHeight) )
		{
			m_activeBuffer.destroy();

			return false;
		}

		this->onActiveBufferReady(m_activeBuffer);

		return true;
	}

	std::ostream &
	operator<< (std::ostream & out, const Surface & obj)
	{
		return out << "Surface '" << obj.name() << "' [depth:" << obj.depth() << "] " << obj.geometry() << "Model matrix : " << obj.modelMatrix();
	}

	std::string
	to_string (const Surface & obj)
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
