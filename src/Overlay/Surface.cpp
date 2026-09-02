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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Surface.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "PixelFactory/Processor.hpp"
#include "Manager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/ExternalImageDescriptor.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/MemoryRegion.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/Sync/Fence.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"
#include "magic_enum/magic_enum.hpp"

namespace EmEn::Overlay
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Graphics;
	using namespace Vulkan;

	bool
	Surface::isBelowPoint (float positionX, float positionY) const noexcept
	{
		/* NOTE: Check on X axis */
		{
			const auto screenWidth = static_cast< float >(m_framebufferProperties.width());

			if ( const auto positionXa = screenWidth * m_rectangle.left(); positionX < positionXa )
			{
				return false;
			}

			if ( const auto positionXb = screenWidth * m_rectangle.right(); positionX > positionXb )
			{
				return false;
			}
		}

		/* NOTE: Check on Y axis */
		{
			const auto screenHeight = static_cast< float >(m_framebufferProperties.height());

			if ( const auto positionYa = screenHeight * m_rectangle.top(); positionY < positionYa )
			{
				return false;
			}

			if ( const auto positionYb = screenHeight * m_rectangle.bottom(); positionY > positionYb )
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

	Surface::MemoryMappingMode
	Surface::parseMemoryMappingMode (const std::string & value) noexcept
	{
		if ( value == "direct" )
		{
			return MemoryMappingMode::Direct;
		}

		if ( value == "staging" )
		{
			return MemoryMappingMode::Staging;
		}

		/* NOTE: "auto" and any unrecognized value fall back to the device-driven decision. */
		return MemoryMappingMode::Auto;
	}

	bool
	Surface::createOnHardware (Renderer & renderer) noexcept
	{
		/* NOTE: Cached for content providers running on their own thread (importAcceleratedFrame). */
		m_renderer = &renderer;

		/* NOTE: The accelerated (zero-copy GPU) source mode is exclusive with CPU memory mapping:
		 * the image is a GPU→GPU copy target (OPTIMAL/DEVICE_LOCAL), never host-mapped. */
		if ( m_acceleratedSourceEnabled )
		{
			m_memoryMappingMode = MemoryMappingMode::Staging;
		}

		{
			/* NOTE: Resolve the CPU-to-GPU memory-mapping decision against the actual device. Auto maps
			 * only on unified-memory devices (integrated GPUs, software, full-ReBAR discrete GPUs) where
			 * a host-mapped image is not sampled across PCIe, and only when the overlay format supports
			 * LINEAR + SAMPLED. Otherwise, we fall back to a staging upload. */
			const auto & physicalDevice = *renderer.device()->physicalDevice();
			const auto overlayFormat = Image::getFormat< uint8_t >(4);
			const bool linearSampled = (physicalDevice.getFormatProperties(overlayFormat).linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0;
			const bool mappableDeviceLocal = physicalDevice.hasMappableDeviceLocalMemory();

			switch ( m_memoryMappingMode )
			{
				case MemoryMappingMode::Staging :
					m_memoryMappingEnabled = false;
					break;

				case MemoryMappingMode::Direct :
					m_memoryMappingEnabled = linearSampled;

					if ( !linearSampled )
					{
						TraceWarning{ClassId} << "Surface '" << this->name() << "': memory mapping forced ON but the device lacks LINEAR+SAMPLED for the overlay format; falling back to staging.";
					}
					break;

				case MemoryMappingMode::Auto :
					m_memoryMappingEnabled = linearSampled && mappableDeviceLocal;
					break;
			}

			TraceInfo{ClassId} <<
				"Surface '" << this->name() << "' memory mapping " << ( m_memoryMappingEnabled ? "ENABLED (direct CPU write)" : "DISABLED (staging upload)" ) << " "
				"[mode=" << magic_enum::enum_name(m_memoryMappingMode) << ", "
				"deviceLocalMappable=" << ( mappableDeviceLocal ? "yes" : "no" ) << ", "
				"linearSampled=" << ( linearSampled ? "yes" : "no" ) << "].";
		}

		const auto & framebuffer = this->framebufferProperties();
		const auto & geometry = this->geometry();

		const auto textureWidth = framebuffer.getSurfaceWidth(geometry.width());
		const auto textureHeight = framebuffer.getSurfaceHeight(geometry.height());

		/* NOTE: When memory mapping is enabled, we skip the local pixmap entirely (the caller
		 * writes directly to the GPU-mapped memory). Same for the accelerated source mode
		 * (content arrives through a GPU→GPU copy — no CPU-side pixels exist at all). */
		if ( !m_memoryMappingEnabled && !m_acceleratedSourceEnabled )
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

		TraceSuccess{ClassId} << "Surface '" << this->name() << "' (" << textureWidth << "x" << textureHeight << "px, scale:" << framebuffer.maxScreenScale() << ") created!";

		return true;
	}

	bool
	Surface::destroyFromHardware () noexcept
	{
		/* NOTE: Cleaning both buffers. */
		m_transitionBuffer.destroy();
		m_activeBuffer.destroy();

		/* NOTE: Accelerated-mode resources (popup cache + one-shot copy resources). */
		m_acceleratedPopupImage.reset();
		m_acceleratedCommandBuffer.reset();
		m_acceleratedCommandPool.reset();
		m_acceleratedFence.reset();

		/* NOTE: The sampler comes from the renderer's shared sampler cache (Renderer::getSampler,
		 * id "OverlaySurface") and is shared by every overlay surface. Only release our reference —
		 * destroying it from hardware would kill it for all other surfaces still using it. The
		 * cache owns it and destroys it at renderer shutdown. */
		m_sampler.reset();

		return true;
	}

	bool
	Surface::uploadActiveBuffer (Renderer & renderer, const Base::Math::Space2D::AARectangle< uint32_t > & touchedRegion) noexcept
	{
		auto & pixmap = m_activeBuffer.pixmap;
		auto & image = *m_activeBuffer.image;

		const auto pixmapBytes = static_cast< uint64_t >(pixmap.bytes());

		const auto fullUpload = [&] () {
			return image.writeData(renderer.transferManager(), MemoryRegion{pixmap.data().data(), pixmap.bytes()});
		};

		/* NOTE: Every condition below is a reason the partial path cannot be PROVEN safe, so the
		 * full upload stays the default. In particular the layout check is what guarantees the image
		 * already holds a complete frame - a fresh or recreated image is UNDEFINED and gets a full
		 * upload, which is also what re-arms the partial path for the frames after it. */
		const auto & createInfo = image.createInfo();

		if (
			!touchedRegion.isValid() ||
			image.currentImageLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL ||
			createInfo.arrayLayers != 1 ||
			createInfo.mipLevels != 1 ||
			createInfo.extent.width != pixmap.width() ||
			createInfo.extent.height != pixmap.height() ||
			pixmap.width() == 0 || pixmap.height() == 0
		)
		{
			return fullUpload();
		}

		/* NOTE: Full-width row band. The staging buffer layout is the linear image, so a band of
		 * rows is contiguous in both - which is the whole reason this shape was chosen over a tight
		 * 2D sub-rectangle: no row-by-row copy, no stride arithmetic. */
		const auto bandTop = std::min(touchedRegion.top(), pixmap.height() - 1);
		const auto bandHeight = std::min(touchedRegion.height(), pixmap.height() - bandTop);

		if ( bandHeight == 0 || bandHeight >= pixmap.height() )
		{
			/* NOTE: A band covering every row IS the full image - going through the partial path
			 * would only add two barriers for nothing. */
			return fullUpload();
		}

		const auto bytesPerPixel = pixmapBytes / (static_cast< uint64_t >(pixmap.width()) * pixmap.height());
		const auto rowBytes = static_cast< uint64_t >(pixmap.width()) * bytesPerPixel;
		const auto bandBytes = rowBytes * bandHeight;

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset.x = 0;
		region.imageOffset.y = static_cast< int32_t >(bandTop);
		region.imageOffset.z = 0;
		region.imageExtent.width = pixmap.width();
		region.imageExtent.height = bandHeight;
		region.imageExtent.depth = 1;

		const MemoryRegion bandMemory{pixmap.data().data() + (bandTop * rowBytes), bandBytes};

		if ( !image.writeDataRegion(renderer.transferManager(), bandMemory, region) )
		{
			TraceWarning{ClassId} << "Partial upload failed for surface '" << this->name() << "', falling back to a full one.";

			return fullUpload();
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

			/* NOTE: After buffer recreation, defer the GPU upload to the next processUpdates() call.
			 * This avoids a wasted upload when observers (e.g., Notifier via OverlayResized)
			 * rewrite the pixmap content before the render thread's next updateVideoMemory() pass. */
			m_framebufferAccess.unlock();

			return true;
		}

		/* Step 1.b: Honor a content-provider-driven transition resize.
		 * The async provider (e.g. CEF) is the source of truth for the painted pixel size. When the
		 * engine's surface-size formula and the provider's own device-scale rounding diverge by a
		 * sub-pixel (typical on fractional display scales, e.g. 125%), the painted frame matches
		 * neither buffer and the resize commit stalls (black render). The provider then requests a
		 * resize to its painted size via requestTransitionBufferResize(); we honor it here so the
		 * next identical frame can commit. This converges within one render iteration. */
		if ( m_transitionBufferEnabled && !this->recreateTransitionBufferToRequestedSize(renderer) )
		{
			TraceError{ClassId} << "Unable to honor the requested transition buffer resize for surface '" << this->name() << "' !";

			m_framebufferAccess.unlock();

			return false;
		}

		/* Step 2: Upload active buffer content to GPU.
		 * This uploads the active pixmap data to the GPU when setVideoMemoryOutdated() was called.
		 * NOTE: When memory mapping is enabled, the caller writes directly to the GPU, so we skip this
		 * step. Same for the accelerated source: there is no CPU pixmap to upload. */
		if ( !m_memoryMappingEnabled && !m_acceleratedSourceEnabled && m_activeBuffer.image != nullptr && !this->isVideoMemoryUpToDate() )
		{
			auto & pixmap = m_activeBuffer.pixmap;

			/* NOTE: The touched region must be read BEFORE the upload, since the marker is consumed
			 * right after it. */
			const auto touchedRegion = pixmap.updatedRegion();

			if ( !this->uploadActiveBuffer(renderer, touchedRegion) )
			{
				TraceError{ClassId} << "Unable to update the content of surface '" << this->name() << "' !";

				m_framebufferAccess.unlock();

				return false;
			}

			/* NOTE: Consume the marker so the next upload measures its own delta and not the union
			 * since the surface was created. Behaviour-neutral today - nothing else in the engine or
			 * in a consuming application reads updatedRegion(), it was pure write-only telemetry -
			 * and a prerequisite of any sub-region upload. */
			pixmap.resetUpdatedRegionMarker();

			m_videoMemoryUpToDate = true;

			//TraceDebug{ClassId} << "Surface '" << this->name() << "' pixmap uploaded to GPU (" << m_activeBuffer.width() << "x" << m_activeBuffer.height() << "px, framebuffer:" << m_framebufferProperties.width() << "x" << m_framebufferProperties.height() << "px, scale:" << m_framebufferProperties.maxScreenScale() << ").";
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
		 * When memory mapping OR the accelerated source is enabled, we skip the pixmap entirely. */
		if ( !m_memoryMappingEnabled && !m_acceleratedSourceEnabled && !buffer.pixmap.isValid() )
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

		buffer.image = std::make_shared< Image >(
			renderer.device(),
			VK_IMAGE_TYPE_2D,
			Image::getFormat< uint8_t >(colorCount),
			VkExtent3D{
				.width = width,
				.height = height,
				.depth = 1U
			},
			imageUsage,
			0, /* createFlags */
			1, /* mipLevels */
			1, /* arrayLayers */
			VK_SAMPLE_COUNT_1_BIT,
			imageTiling,
			m_memoryMappingEnabled /* hostVisible */
		);
		buffer.image->setIdentifier(ClassId, this->name(), "Image");

		if ( m_memoryMappingEnabled || m_acceleratedSourceEnabled )
		{
			/* NOTE: Memory-mapping path: just create the image on hardware, the caller writes
			 * directly to the mapped memory. Accelerated-source path: same empty creation — the
			 * content arrives later through a GPU→GPU copy (importAcceleratedFrame()). */
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

		const std::scoped_lock lock{m_framebufferAccess};

		if ( !m_transitionBuffer.isValid() )
		{
			TraceError{ClassId} << "The surface '" << this->name() << "' transition buffer is invalid !";

			return false;
		}

		/* NOTE: Swap the buffer structures (transition becomes active, active becomes transition). */
		std::swap(m_transitionBuffer, m_activeBuffer);

		TraceSuccess{ClassId} << "Surface '" << this->name() << "' transition buffer committed (" << m_activeBuffer.width() << "x" << m_activeBuffer.height() << "px, scale:" << m_framebufferProperties.maxScreenScale() << ").";

		/* NOTE: After commit, the transition buffer status returns to Ready for next resize. */
		m_transitionBufferStatus = TransitionBufferStatus::Ready;

		/* NOTE: When memory mapping or the accelerated source is enabled, the content lands
		 * directly in GPU memory, so the video memory is already up to date. When disabled,
		 * mark as outdated so the pixmap gets uploaded via staging buffer. */
		if ( !m_memoryMappingEnabled && !m_acceleratedSourceEnabled )
		{
			m_videoMemoryUpToDate = false;
		}

		return true;
	}

	void
	Surface::requestTransitionBufferResize (uint32_t width, uint32_t height) noexcept
	{
		if ( !m_transitionBufferEnabled || width == 0 || height == 0 )
		{
			return;
		}

		const std::lock_guard< std::mutex > lock{m_requestedTransitionSizeMutex};

		m_requestedTransitionWidth = width;
		m_requestedTransitionHeight = height;
		m_transitionResizeRequested = true;
	}

	bool
	Surface::recreateTransitionBufferToRequestedSize (Renderer & renderer) noexcept
	{
		uint32_t requestedWidth = 0;
		uint32_t requestedHeight = 0;

		{
			const std::lock_guard< std::mutex > lock{m_requestedTransitionSizeMutex};

			if ( !m_transitionResizeRequested )
			{
				return true;
			}

			requestedWidth = m_requestedTransitionWidth;
			requestedHeight = m_requestedTransitionHeight;
			m_transitionResizeRequested = false;
		}

		/* NOTE: The content provider's painted size is authoritative. If the transition buffer
		 * already matches it, there is nothing to do — the next incoming frame will commit. */
		if ( m_transitionBuffer.matchesSize(requestedWidth, requestedHeight) )
		{
			return true;
		}

		TraceWarning{ClassId} <<
			"Surface '" << this->name() << "' transition buffer size (" << m_transitionBuffer.width() << "x" << m_transitionBuffer.height() <<
			"px) diverges from the content provider's painted size (" << requestedWidth << "x" << requestedHeight <<
			"px). Recreating the transition buffer at the painted size.";

		/* NOTE: Block any transition write/commit from the content provider while we destroy and
		 * recreate the GPU resources (isTransitionBufferReady() returns false during Resizing). */
		m_transitionBufferStatus = TransitionBufferStatus::Resizing;

		/* NOTE: Memory-mapping and accelerated-source paths write straight to GPU memory and need
		 * no pixmap. CPU pixmap path needs an empty pixmap of the requested size. */
		if ( !m_memoryMappingEnabled && !m_acceleratedSourceEnabled )
		{
			if ( !m_transitionBuffer.pixmap.initialize(requestedWidth, requestedHeight, ChannelMode::RGBA) )
			{
				TraceError{ClassId} << "Unable to initialize the transition pixmap for the surface '" << this->name() << "' !";

				return false;
			}
		}

		renderer.device()->waitIdle("Surface::recreateTransitionBufferToRequestedSize()");

		m_transitionBuffer.destroy();

		if ( !this->createFramebufferResources(m_transitionBuffer, renderer, requestedWidth, requestedHeight) )
		{
			m_transitionBuffer.destroy();

			return false;
		}

		/* NOTE: The transition buffer now matches the provider's painted size. The next incoming
		 * frame at this size will be accepted and committed. We deliberately DO NOT call
		 * onTransitionBufferReady() here: the provider is already painting at this size, and
		 * notifying it (which triggers WasResized() on the CEF side) could restart the loop. */
		m_transitionBufferStatus = TransitionBufferStatus::WaitingForContent;

		return true;
	}

	bool
	Surface::updatePhysicalRepresentation (Renderer & renderer) noexcept
	{
		const auto & framebuffer = this->framebufferProperties();
		const auto & geometry = this->geometry();

		const auto textureWidth = framebuffer.getSurfaceWidth(geometry.width());
		const auto textureHeight = framebuffer.getSurfaceHeight(geometry.height());

		/* NOTE: During an aggressive resize or a minimize, the framebuffer can transiently
		 * report 0 px, yielding a 0-sized surface. Creating a 0-dimension pixmap/image fails
		 * and would otherwise propagate up as a hard error that disables the whole UIScreen
		 * permanently. A degenerate size is not an error but a transient state: keep the
		 * current buffer and defer recreation. The next resize event (window back to a valid
		 * size) re-invalidates the surface and recreates it correctly. */
		if ( textureWidth == 0 || textureHeight == 0 )
		{
			return true;
		}

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

			/* NOTE: When memory mapping or the accelerated source is enabled, skip the pixmap entirely.
			 * When disabled, copy and resize the active buffer content to the transition buffer
			 * to have a placeholder image while waiting for new content. */
			if ( !m_memoryMappingEnabled && !m_acceleratedSourceEnabled )
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
			 * If pixmap copy is disabled, or memory mapping / accelerated source is enabled,
			 * the buffer is empty and waiting for content.
			 * If pixmap copy is enabled, the buffer has a resized placeholder and is ready. */
			if ( m_disablePixmapCopyInTransitionBuffer || m_memoryMappingEnabled || m_acceleratedSourceEnabled )
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
		if ( !m_memoryMappingEnabled && !m_acceleratedSourceEnabled )
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

	bool
	Surface::prepareAcceleratedCopyResources (Renderer & renderer) noexcept
	{
		if ( m_acceleratedCommandPool != nullptr && m_acceleratedCommandBuffer != nullptr && m_acceleratedFence != nullptr )
		{
			return true;
		}

		const auto & device = renderer.device();

		m_acceleratedCommandPool = std::make_shared< CommandPool >(device, device->getGraphicsFamilyIndex(), true, true, false);
		m_acceleratedCommandPool->setIdentifier(ClassId, this->name(), "AcceleratedCopyCommandPool");

		if ( !m_acceleratedCommandPool->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the accelerated-copy command pool for the surface '" << this->name() << "' !";

			m_acceleratedCommandPool.reset();

			return false;
		}

		m_acceleratedCommandBuffer = std::make_shared< CommandBuffer >(m_acceleratedCommandPool, true);
		m_acceleratedCommandBuffer->setIdentifier(ClassId, this->name(), "AcceleratedCopyCommandBuffer");

		if ( !m_acceleratedCommandBuffer->isCreated() )
		{
			TraceError{ClassId} << "Unable to create the accelerated-copy command buffer for the surface '" << this->name() << "' !";

			m_acceleratedCommandBuffer.reset();
			m_acceleratedCommandPool.reset();

			return false;
		}

		m_acceleratedFence = std::make_shared< Sync::Fence >(device, 0);
		m_acceleratedFence->setIdentifier(ClassId, this->name(), "AcceleratedCopyFence");

		if ( !m_acceleratedFence->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the accelerated-copy fence for the surface '" << this->name() << "' !";

			m_acceleratedFence.reset();
			m_acceleratedCommandBuffer.reset();
			m_acceleratedCommandPool.reset();

			return false;
		}

		return true;
	}

#if IS_WINDOWS || IS_MACOS
	/**
	 * @brief Platform dispatch of the external image import — the surrounding copy/sync logic
	 * in the two importAccelerated*Frame() methods is platform-neutral.
	 */
	static
	std::shared_ptr< Vulkan::Image >
	importExternalImage (const std::shared_ptr< Vulkan::Device > & device, const Vulkan::ExternalImageDescriptor & descriptor) noexcept
	{
#if IS_WINDOWS
		return Vulkan::Image::importFromWin32Handle(device, descriptor);
#else
		return Vulkan::Image::importFromIOSurface(device, descriptor);
#endif
	}
#endif

	bool
	Surface::importAcceleratedFrame (const Vulkan::ExternalImageDescriptor & descriptor) noexcept
	{
#if IS_WINDOWS || IS_MACOS
		if ( !m_acceleratedSourceEnabled )
		{
			TraceError{ClassId} << "The surface '" << this->name() << "' is not in accelerated source mode ! Call enableAcceleratedSource() before createOnHardware().";

			return false;
		}

		if ( m_renderer == nullptr )
		{
			TraceError{ClassId} << "The surface '" << this->name() << "' is not created on the GPU yet ! Unable to import an accelerated frame.";

			return false;
		}

		auto & renderer = *m_renderer;

		const std::scoped_lock lock{m_framebufferAccess};

		/* NOTE: Route the frame by size through the transition machinery — same rules as the
		 * CPU paths (see directPaint()/indirectPaint() rationale in the consumer). */
		const auto target = this->determineTargetBuffer(descriptor.width, descriptor.height);

		if ( target == TargetBuffer::None && m_transitionBufferEnabled )
		{
			/* NOTE: CONVERGENCE: the producer's painted size is authoritative — ask for a transition
			 * buffer at that exact size. Meanwhile the frame is clamp-copied into the active buffer
			 * (SAFETY NET — the copy extent below is the min of both sizes by construction). */
			this->requestTransitionBufferResize(descriptor.width, descriptor.height);
		}

		auto & targetBuffer = ( target == TargetBuffer::Transition ) ? m_transitionBuffer : m_activeBuffer;

		if ( targetBuffer.image == nullptr || !targetBuffer.image->isCreated() )
		{
			TraceError{ClassId} << "The surface '" << this->name() << "' has no valid target image for the accelerated frame !";

			return false;
		}

		if ( !this->prepareAcceleratedCopyResources(renderer) )
		{
			return false;
		}

		auto * queue = renderer.graphicsQueue();

		if ( queue == nullptr )
		{
			TraceError{ClassId} << "The renderer has no graphics queue yet ! Unable to copy the accelerated frame for the surface '" << this->name() << "'.";

			return false;
		}

		/* NOTE: Import the external texture. The handle is borrowed and only valid during the
		 * producer's callback — everything below (record, submit, WAIT) happens synchronously. */
		const auto importedImage = importExternalImage(renderer.device(), descriptor);

		if ( importedImage == nullptr )
		{
			return false;
		}

		if ( !m_acceleratedFence->reset() )
		{
			TraceError{ClassId} << "Unable to reset the accelerated-copy fence for the surface '" << this->name() << "' !";

			return false;
		}

		if ( !m_acceleratedCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			TraceError{ClassId} << "Unable to begin the accelerated-copy command buffer for the surface '" << this->name() << "' !";

			return false;
		}

		const auto graphicsFamilyIndex = renderer.device()->getGraphicsFamilyIndex();

#if IS_WINDOWS
		/* NOTE: Acquire the imported external image (VK_QUEUE_FAMILY_EXTERNAL → graphics family).
		 * Spec carve-out for D3D11 imports: the UNDEFINED transition preserves the texture content. */
		const Sync::ImageMemoryBarrier acquireExternalBarrier{
			*importedImage,
			0,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_QUEUE_FAMILY_EXTERNAL,
			graphicsFamilyIndex
		};
#else
		/* NOTE: macOS — the IOSurface-backed image is NOT Vulkan external memory (metal_objects
		 * import): no external queue family to acquire from. Plain layout transition; MoltenVK
		 * layout transitions are Metal no-ops, the IOSurface content is preserved. */
		static_cast< void >(graphicsFamilyIndex);

		const Sync::ImageMemoryBarrier acquireExternalBarrier{
			*importedImage,
			0,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		};
#endif

		m_acceleratedCommandBuffer->pipelineBarrier(acquireExternalBarrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		/* NOTE: The surface image leaves the sampled state. srcStage FRAGMENT_SHADER orders this
		 * against every in-flight frame command buffer sampling it — valid because the copy is
		 * submitted on the renderer's single frame queue (FIFO). */
		const Sync::ImageMemoryBarrier toTransferDstBarrier{
			*targetBuffer.image,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			targetBuffer.image->currentImageLayout(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		};

		m_acceleratedCommandBuffer->pipelineBarrier(toTransferDstBarrier, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		/* NOTE: The copy extent is the min of both images (clamped by CommandBuffer::copyImage). */
		m_acceleratedCommandBuffer->copyImage(*importedImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *targetBuffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		/* NOTE: A view frame is a FULL copy and erases any composited popup — re-composite the
		 * popup cache on top while it is visible (mirror of the CPU compositePopup() semantics). */
		this->recordAcceleratedPopupComposite(*targetBuffer.image);

		const Sync::ImageMemoryBarrier toSampledBarrier{
			*targetBuffer.image,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		m_acceleratedCommandBuffer->pipelineBarrier(toSampledBarrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		if ( !m_acceleratedCommandBuffer->end() )
		{
			TraceError{ClassId} << "Unable to end the accelerated-copy command buffer for the surface '" << this->name() << "' !";

			return false;
		}

		if ( !queue->submit(*m_acceleratedCommandBuffer, SynchInfo{}.withFence(m_acceleratedFence->handle())) )
		{
			TraceError{ClassId} << "Unable to submit the accelerated-copy command buffer for the surface '" << this->name() << "' !";

			return false;
		}

		/* NOTE: WAIT before returning — the producer reclaims the shared texture right after
		 * its callback returns (copy-during-callback contract, no keyed mutex on CEF 126). */
		if ( !m_acceleratedFence->wait() )
		{
			TraceError{ClassId} << "Unable to wait the accelerated-copy fence for the surface '" << this->name() << "' !";

			return false;
		}

		targetBuffer.image->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		/* NOTE: A frame at the transition size completes a resize — commit inline (the commit
		 * logic from commitTransitionBuffer(), replicated because m_framebufferAccess is held). */
		if ( target == TargetBuffer::Transition )
		{
			std::swap(m_transitionBuffer, m_activeBuffer);

			m_transitionBufferStatus = TransitionBufferStatus::Ready;

			TraceSuccess{ClassId} << "Surface '" << this->name() << "' transition buffer committed by an accelerated frame (" << m_activeBuffer.width() << "x" << m_activeBuffer.height() << "px).";
		}

		/* NOTE: The GPU texture is current — only a re-composite is needed (on-demand rendering). */
		this->notifyRedrawRequired();

		return true;
#else
		static_cast< void >(descriptor);

		TraceError{ClassId} << "The accelerated frame import is not implemented on this platform yet (surface '" << this->name() << "') !";

		return false;
#endif
	}

	void
	Surface::setAcceleratedPopupVisible (bool visible) noexcept
	{
		const std::scoped_lock lock{m_framebufferAccess};

		m_acceleratedPopupVisible = visible;

		if ( !visible )
		{
			/* NOTE: Release the cache — the next view frame (a full copy) erases the popup on
			 * screen. Safe to destroy inline: every accelerated submission fence-waits before
			 * returning, so the GPU holds no pending reference on the cache image. */
			m_acceleratedPopupImage.reset();
		}
	}

	void
	Surface::recordAcceleratedPopupComposite (Vulkan::Image & targetImage) noexcept
	{
		if ( !m_acceleratedPopupVisible || m_acceleratedPopupImage == nullptr || !m_acceleratedPopupImage->isCreated() )
		{
			return;
		}

		const auto targetWidth = targetImage.width();
		const auto targetHeight = targetImage.height();

		/* NOTE: Clamp the popup area to the target boundaries (negative positions and overflow). */
		const auto popupX = static_cast< uint32_t >(std::max(0, m_acceleratedPopupX));
		const auto popupY = static_cast< uint32_t >(std::max(0, m_acceleratedPopupY));

		if ( popupX >= targetWidth || popupY >= targetHeight )
		{
			return;
		}

		const auto copyWidth = std::min(m_acceleratedPopupImage->width(), targetWidth - popupX);
		const auto copyHeight = std::min(m_acceleratedPopupImage->height(), targetHeight - popupY);

		if ( copyWidth == 0 || copyHeight == 0 )
		{
			return;
		}

		VkImageCopy region{};
		region.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		region.srcOffset = {0, 0, 0};
		region.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
		region.dstOffset = {static_cast< int32_t >(popupX), static_cast< int32_t >(popupY), 0};
		region.extent = {copyWidth, copyHeight, 1};

		m_acceleratedCommandBuffer->copyImage(*m_acceleratedPopupImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, targetImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region);
	}

	bool
	Surface::importAcceleratedPopupFrame (const Vulkan::ExternalImageDescriptor & descriptor) noexcept
	{
#if IS_WINDOWS || IS_MACOS
		if ( !m_acceleratedSourceEnabled )
		{
			TraceError{ClassId} << "The surface '" << this->name() << "' is not in accelerated source mode !";

			return false;
		}

		if ( m_renderer == nullptr )
		{
			TraceError{ClassId} << "The surface '" << this->name() << "' is not created on the GPU yet ! Unable to import an accelerated popup frame.";

			return false;
		}

		auto & renderer = *m_renderer;

		const std::scoped_lock lock{m_framebufferAccess};

		if ( !m_acceleratedPopupVisible )
		{
			/* NOTE: The popup was hidden while the frame was in flight — drop it silently. */
			return true;
		}

		if ( m_activeBuffer.image == nullptr || !m_activeBuffer.image->isCreated() )
		{
			TraceError{ClassId} << "The surface '" << this->name() << "' has no valid active image for the accelerated popup frame !";

			return false;
		}

		if ( !this->prepareAcceleratedCopyResources(renderer) )
		{
			return false;
		}

		auto * queue = renderer.graphicsQueue();

		if ( queue == nullptr )
		{
			TraceError{ClassId} << "The renderer has no graphics queue yet ! Unable to copy the accelerated popup frame for the surface '" << this->name() << "'.";

			return false;
		}

		/* NOTE: (Re)create the persistent popup cache when the popup size changed. Safe inline:
		 * every accelerated submission fence-waits, the GPU holds no reference on the old image. */
		if ( m_acceleratedPopupImage == nullptr || m_acceleratedPopupImage->width() != descriptor.width || m_acceleratedPopupImage->height() != descriptor.height )
		{
			m_acceleratedPopupImage.reset();

			auto popupImage = std::make_shared< Vulkan::Image >(
				renderer.device(),
				VK_IMAGE_TYPE_2D,
				descriptor.format,
				VkExtent3D{descriptor.width, descriptor.height, 1},
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
			);
			popupImage->setIdentifier(ClassId, this->name(), "AcceleratedPopupImage");

			if ( !popupImage->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the accelerated popup cache image for the surface '" << this->name() << "' !";

				return false;
			}

			m_acceleratedPopupImage = popupImage;
		}

		/* NOTE: Import the external popup texture (borrowed handle — synchronous contract). */
		const auto importedImage = importExternalImage(renderer.device(), descriptor);

		if ( importedImage == nullptr )
		{
			return false;
		}

		if ( !m_acceleratedFence->reset() || !m_acceleratedCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			TraceError{ClassId} << "Unable to prepare the accelerated-copy command buffer for the popup of surface '" << this->name() << "' !";

			return false;
		}

		const auto graphicsFamilyIndex = renderer.device()->getGraphicsFamilyIndex();

		/* 1. Acquire the imported external popup texture (queue-family transfer on Windows only —
		 * see the view-path barrier rationale above). */
#if IS_WINDOWS
		const Sync::ImageMemoryBarrier acquireExternalBarrier{
			*importedImage,
			0,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_QUEUE_FAMILY_EXTERNAL,
			graphicsFamilyIndex
		};
#else
		static_cast< void >(graphicsFamilyIndex);

		const Sync::ImageMemoryBarrier acquireExternalBarrier{
			*importedImage,
			0,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		};
#endif

		m_acceleratedCommandBuffer->pipelineBarrier(acquireExternalBarrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		/* 2. Refresh the popup cache (external → cache). */
		const VkAccessFlags cacheSourceAccess = m_acceleratedPopupImage->currentImageLayout() == VK_IMAGE_LAYOUT_UNDEFINED ? 0U : static_cast< VkAccessFlags >(VK_ACCESS_TRANSFER_READ_BIT);

		const Sync::ImageMemoryBarrier cacheToDstBarrier{
			*m_acceleratedPopupImage,
			cacheSourceAccess,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			m_acceleratedPopupImage->currentImageLayout(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		};

		m_acceleratedCommandBuffer->pipelineBarrier(cacheToDstBarrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		m_acceleratedCommandBuffer->copyImage(*importedImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *m_acceleratedPopupImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		const Sync::ImageMemoryBarrier cacheToSrcBarrier{
			*m_acceleratedPopupImage,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
		};

		m_acceleratedCommandBuffer->pipelineBarrier(cacheToSrcBarrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		/* 3. Composite the cache onto the active buffer at the popup position. */
		const Sync::ImageMemoryBarrier activeToDstBarrier{
			*m_activeBuffer.image,
			VK_ACCESS_SHADER_READ_BIT,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			m_activeBuffer.image->currentImageLayout(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		};

		m_acceleratedCommandBuffer->pipelineBarrier(activeToDstBarrier, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		this->recordAcceleratedPopupComposite(*m_activeBuffer.image);

		const Sync::ImageMemoryBarrier activeToSampledBarrier{
			*m_activeBuffer.image,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		m_acceleratedCommandBuffer->pipelineBarrier(activeToSampledBarrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

		if ( !m_acceleratedCommandBuffer->end() )
		{
			TraceError{ClassId} << "Unable to end the accelerated-copy command buffer for the popup of surface '" << this->name() << "' !";

			return false;
		}

		if ( !queue->submit(*m_acceleratedCommandBuffer, SynchInfo{}.withFence(m_acceleratedFence->handle())) )
		{
			TraceError{ClassId} << "Unable to submit the accelerated popup copy for the surface '" << this->name() << "' !";

			return false;
		}

		if ( !m_acceleratedFence->wait() )
		{
			TraceError{ClassId} << "Unable to wait the accelerated popup copy fence for the surface '" << this->name() << "' !";

			return false;
		}

		m_acceleratedPopupImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		m_activeBuffer.image->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		this->notifyRedrawRequired();

		return true;
#else
		static_cast< void >(descriptor);

		TraceError{ClassId} << "The accelerated popup import is not implemented on this platform yet (surface '" << this->name() << "') !";

		return false;
#endif
	}

	std::ostream &
	operator<< (std::ostream & out, const Surface & obj)
	{
		return out << "Surface '" << obj.name() << "' [stack-depth:" << obj.m_depth << "] " << obj.geometry() << "Model matrix : " << obj.modelMatrix();
	}

	std::string
	to_string (const Surface & obj)
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
