/*
 * src/Vulkan/TransferManager.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <chrono>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Mutex.hpp"
#include "BufferTransferOperation.hpp"
#include "ImageTransferOperation.hpp"
#include "Tracer.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Device;
	class CommandPool;
	class Buffer;
	class Image;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief The transfer manager service class.
	 * @extends EmEn::ServiceInterface This class is a service.
	 */
	class EMEN_API TransferManager final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanTransferManagerService"};

			/** @brief An idle staging buffer larger than this is released at the first frame after its fence signalled
			 * (releaseIdleStagingBuffers(), owner decision 2026-10-12). */
			static constexpr size_t StagingLargeBytes{16UL * 1024UL * 1024UL};

			/** @brief A staging buffer no upload reserved for this long is released, whatever its size (releaseIdleStagingBuffers()). */
			static constexpr std::chrono::seconds StagingIdleDelay{5};

			/** @brief The minimum time between two idle-delay passes of releaseIdleStagingBuffers(), and between two log lines. */
			static constexpr std::chrono::seconds StagingReleaseInterval{1};

			/**
			 * @brief Constructs the transfer manager.
			 */
			TransferManager () noexcept
				: ServiceInterface{ClassId}
			{

			}

			/**
			 * @brief Destructs the transfer manager.
			 */
			~TransferManager () override = default;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			TransferManager (const TransferManager & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			TransferManager (TransferManager && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return TransferManager &
			 */
			TransferManager & operator= (const TransferManager & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return TransferManager &
			 */
			TransferManager & operator= (TransferManager && copy) noexcept = delete;

			/**
			 * @brief Sets the device used by the transfer manager.
			 */
			void
			setDevice (const std::shared_ptr< Device > & device) noexcept
			{
				m_device = device;
			}

			/**
			 * @brief Returns the device of the transfer manager.
			 * @return std::shared_ptr< Device >
			 */
			[[nodiscard]]
			std::shared_ptr< Device >
			device () const noexcept
			{
				return m_device;
			}

			/**
			 * @brief Transfer data to a buffer inside the GPU memory.
			 * @tparam function_t The type of lambda to describe the data to write. Signature: bool (const Buffer &).
			 * @param targetBuffer A writable reference to the destination buffer.
			 * @param requiredBytes The amount of data to write in bytes.
			 * @param writeData A reference to a function to write data into the staging buffer.
			 * @return bool
			 */
			template< typename function_t >
			bool
			uploadBuffer (Buffer & targetBuffer, size_t requiredBytes, function_t && writeData) noexcept requires (std::is_invocable_v< function_t, const Buffer & >)
			{
				/* [VULKAN-CPU-SYNC] Transfer to GPU (Abusive lock!) */
				const Base::MutexLock lock{m_transferOperationsAccess};

				if ( !this->usable() )
				{
					TraceError{ClassId} << "The transfer manager is not usable !";

					return false;
				}

				//TraceDebug{ClassId} << "Initialize a buffer transfer operation for " << requiredBytes << " bytes (Buffer:" << targetBuffer.identifier() << ") ...";

				auto * const transferOperation = this->getAndReserveBufferTransferOperation(requiredBytes);

				if ( transferOperation == nullptr )
				{
					return false;
				}

				auto * stagingBuffer = transferOperation->stagingBuffer();

				if ( stagingBuffer == nullptr )
				{
					return false;
				}

				if ( !std::forward< function_t >(writeData)(*stagingBuffer) )
				{
					TraceError{ClassId} << "Unable to write " << requiredBytes << " bytes of data in the staging buffer (Buffer) !";

					return false;
				}

				return transferOperation->transfer(m_device, targetBuffer, 0);
			}

			/**
			 * @brief Transfer data to an image inside the GPU memory.
			 * @tparam function_t The type of lambda to describe the data to write. Signature: bool (const Buffer &).
			 * @param targetImage A writable reference to the destination buffer.
			 * @param requiredBytes The amount of data to write in bytes.
			 * @param writeData A reference to a function to write data into the staging buffer.
			 * @return bool
			 */
			template< typename function_t >
			bool
			uploadImage (Image & targetImage, size_t requiredBytes, function_t && writeData) noexcept requires (std::is_invocable_v< function_t, const Buffer & >)
			{
				/* [VULKAN-CPU-SYNC] Transfer to GPU (Abusive lock!) */
				const Base::MutexLock lock{m_transferOperationsAccess};

				if ( !this->usable() )
				{
					TraceError{ClassId} << "The transfer manager is not usable !";

					return false;
				}

				//TraceDebug{ClassId} << "Initialize an image transfer operation for " << requiredBytes << " bytes (Image:" << targetImage.identifier() << ") ...";

				auto * const transferOperation = this->getAndReserveImageTransferOperation(requiredBytes);

				if ( transferOperation == nullptr )
				{
					return false;
				}

				auto * stagingBuffer = transferOperation->stagingBuffer();

				if ( stagingBuffer == nullptr )
				{
					return false;
				}

				if ( !std::forward< function_t >(writeData)(*stagingBuffer) )
				{
					TraceError{ClassId} << "Unable to write " << requiredBytes << " bytes of data in the staging buffer (Image) !";

					return false;
				}

				return transferOperation->transfer(m_device, targetImage, 0);
			}

			/**
			 * @brief Transfer pre-compressed multi-mip data to an image inside the GPU memory.
			 * @tparam function_t The type of lambda to describe the data to write. Signature: bool (const Buffer &).
			 * @param targetImage A writable reference to the destination image.
			 * @param requiredBytes The total amount of compressed data to write in bytes.
			 * @param writeData A reference to a function to write data into the staging buffer.
			 * @param mipRegions A vector of buffer-to-image copy regions, one per mip level.
			 * @return bool
			 */
			template< typename function_t >
			bool
			uploadCompressedImage (Image & targetImage, size_t requiredBytes, function_t && writeData, const std::vector< VkBufferImageCopy > & mipRegions) noexcept requires (std::is_invocable_v< function_t, const Buffer & >)
			{
				/* [VULKAN-CPU-SYNC] Transfer to GPU (Abusive lock!) */
				const Base::MutexLock lock{m_transferOperationsAccess};

				if ( !this->usable() )
				{
					TraceError{ClassId} << "The transfer manager is not usable !";

					return false;
				}

				auto * const transferOperation = this->getAndReserveImageTransferOperation(requiredBytes);

				if ( transferOperation == nullptr )
				{
					return false;
				}

				auto * stagingBuffer = transferOperation->stagingBuffer();

				if ( stagingBuffer == nullptr )
				{
					return false;
				}

				if ( !std::forward< function_t >(writeData)(*stagingBuffer) )
				{
					TraceError{ClassId} << "Unable to write " << requiredBytes << " bytes of compressed data in the staging buffer !";

					return false;
				}

				return transferOperation->transferCompressed(m_device, targetImage, mipRegions);
			}

			/**
			 * @brief Transfer data to a SUB-REGION of an image already living in GPU memory.
			 * @note Unlike uploadImage(), the pixels outside the region are preserved. Use it to
			 * refresh only the part of an image that actually changed.
			 * @warning The target image must already hold a complete upload (it must be in
			 * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL). The call fails otherwise, and the caller is
			 * expected to fall back to a full uploadImage().
			 * @tparam function_t The type of lambda to describe the data to write. Signature: bool (const Buffer &).
			 * @param targetImage A writable reference to the destination image.
			 * @param requiredBytes The amount of data to write in bytes (the region, not the image).
			 * @param writeData A reference to a function to write data into the staging buffer.
			 * @param region The buffer-to-image copy region to update.
			 * @return bool
			 */
			template< typename function_t >
			bool
			uploadImageRegion (Image & targetImage, size_t requiredBytes, function_t && writeData, const VkBufferImageCopy & region) noexcept requires (std::is_invocable_v< function_t, const Buffer & >)
			{
				/* [VULKAN-CPU-SYNC] Transfer to GPU (Abusive lock!) */
				const Base::MutexLock lock{m_transferOperationsAccess};

				if ( !this->usable() )
				{
					TraceError{ClassId} << "The transfer manager is not usable !";

					return false;
				}

				auto * const transferOperation = this->getAndReserveImageTransferOperation(requiredBytes);

				if ( transferOperation == nullptr )
				{
					return false;
				}

				auto * stagingBuffer = transferOperation->stagingBuffer();

				if ( stagingBuffer == nullptr )
				{
					return false;
				}

				if ( !std::forward< function_t >(writeData)(*stagingBuffer) )
				{
					TraceError{ClassId} << "Unable to write " << requiredBytes << " bytes of data in the staging buffer (Image region) !";

					return false;
				}

				return transferOperation->transferRegion(m_device, targetImage, region);
			}

			/**
			 * @brief Transitions the layout of a Vulkan image.
			 * @param image A reference to an image.
			 * @param aspectMask The type of image.
			 * @param oldLayout The current layout of the image.
			 * @param newLayout The desired new layout of the image.
			 * @return bool
			 */
			[[nodiscard]]
			bool transitionImageLayout (Image & image, VkImageAspectFlags aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout) const noexcept;

			/**
			 * @brief Downloads an image from GPU memory to a pixmap.
			 * @param sourceImage A reference to the source image in GPU memory.
			 * @param currentLayout The current layout of the source image.
			 * @param aspectMask The aspect flags (color, depth, or stencil).
			 * @param pixmap A reference to the destination pixmap to fill with data.
			 * @return bool
			 */
			[[nodiscard]]
			bool downloadImage (const Image & sourceImage, VkImageLayout currentLayout, VkImageAspectFlags aspectMask, Base::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept;

			/**
			 * @brief Downloads a whole buffer from GPU memory: a copy into a temporary host-readable (cached) buffer on
			 * the graphics queue, then a wait.
			 * @note Blocking: for a load-time reader (a released CPU copy rebuilt from its GPU copy), never per frame.
			 * The source must have been created with VK_BUFFER_USAGE_TRANSFER_SRC_BIT.
			 * @param sourceBuffer A reference to the source buffer.
			 * @param destination The bytes to fill, exactly the buffer's size.
			 * @return bool
			 */
			[[nodiscard]]
			bool downloadBuffer (const Buffer & sourceBuffer, std::span< std::byte > destination) noexcept;

			/**
			 * @brief Clears a depth image to a specific depth value.
			 * @note The image must be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout.
			 * @param image A reference to the depth image to clear.
			 * @param depthValue The depth value to clear to (0.0 to 1.0).
			 * @param stencilValue The stencil value to clear to. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool clearDepthImage (Image & image, float depthValue, uint32_t stencilValue = 0) const noexcept;

			/**
			 * @brief Clears a color image to a specific RGBA value, every mip level and every layer.
			 * @note The image must be in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout.
			 * @param image A reference to the color image to clear.
			 * @param clearColor The RGBA clear color values.
			 * @return bool
			 */
			[[nodiscard]]
			bool clearColorImage (Image & image, VkClearColorValue clearColor) const noexcept;

			/**
			 * @brief Releases the idle staging buffers (owner decision 2026-10-12): the pools only grow during a load (one
			 * operation per upload still in flight, each enlarged to the largest upload it served), and without this pass
			 * they kept that memory until the service terminated — host-visible memory, RAM taken from the system on
			 * unified memory (385 MiB after WorldLobby on macOS, 417 to 457 MiB after sponza on an Apple M6).
			 * Two rules: an operation larger than StagingLargeBytes is released as soon as its fence has signalled (the
			 * peak during a load stays near the bytes really in flight), and every operation no upload reserved for
			 * StagingIdleDelay is released (nothing stays after a load).
			 * @note Called once per frame by the renderer after its frame fence; the idle-delay rule runs at most every
			 * StagingReleaseInterval. Never waits: while an upload holds the lock, the pass is skipped. Only an operation
			 * whose fence has signalled is released, so no command buffer or staging buffer in use by the GPU is freed.
			 * See docs/subsystems/graphics/37-gpu-memory-budget-and-report.md § Staging buffers.
			 */
			void releaseIdleStagingBuffers () noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Returns a locked buffer transfer operation.
			 * @param requiredBytes The size in bytes of the buffer.
			 * @return BufferTransferOperation *
			 */
			[[nodiscard]]
			BufferTransferOperation * getAndReserveBufferTransferOperation (size_t requiredBytes) noexcept EMEN_REQUIRES(m_transferOperationsAccess);

			/**
			 * @brief Returns a locked image transfer operation.
			 * @param requiredBytes The size in bytes of the buffer.
			 * @return ImageTransferOperation *
			 */
			[[nodiscard]]
			ImageTransferOperation * getAndReserveImageTransferOperation (size_t requiredBytes) noexcept EMEN_REQUIRES(m_transferOperationsAccess);

			std::shared_ptr< Device > m_device;
			std::shared_ptr< CommandPool > m_transferCommandPool;
			std::shared_ptr< CommandPool > m_graphicsCommandPool;
			std::vector< BufferTransferOperation > m_bufferTransferOperations EMEN_GUARDED_BY(m_transferOperationsAccess);
			std::vector< ImageTransferOperation > m_imageTransferOperations EMEN_GUARDED_BY(m_transferOperationsAccess);
			std::unique_ptr< CommandBuffer > m_imageLayoutTransitionCommandBuffer EMEN_GUARDED_BY(m_transferOperationsAccess);
			std::unique_ptr< Sync::Fence > m_imageLayoutTransitionFence EMEN_GUARDED_BY(m_transferOperationsAccess);
			std::chrono::steady_clock::time_point m_lastStagingRelease;
			size_t m_releasedStagingCount{0};
			size_t m_releasedStagingBytes{0};
			mutable Base::Mutex m_transferOperationsAccess;
	};
}
