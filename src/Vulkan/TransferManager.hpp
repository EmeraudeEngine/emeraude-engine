/*
 * src/Vulkan/TransferManager.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
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
	class TransferManager final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanTransferManagerService"};

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
			 * @brief Sets the device used by the transfer manager.
			 * @return void
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
			transferBuffer (Buffer & targetBuffer, size_t requiredBytes, function_t && writeData) noexcept requires (std::is_invocable_v< function_t, const Buffer & >)
			{
				/* [VULKAN-CPU-SYNC] Transfer to GPU (Abusive lock!) */
				const std::lock_guard< std::mutex > lock{m_transferOperationsAccess};

				if ( !this->usable() )
				{
					TraceError{ClassId} << "The transfer manager is not usable !";

					return false;
				}

				TraceDebug{ClassId} << "Initialize a buffer transfer operation for " << requiredBytes << " bytes (Buffer:" << targetBuffer.identifier() << ") ...";

				const auto transferOperation = this->getAndReserveBufferTransferOperation(requiredBytes);

				if ( transferOperation == nullptr )
				{
					return false;
				}

				auto * stagingBuffer = transferOperation->stagingBuffer();

				if ( stagingBuffer == nullptr )
				{
					return false;
				}

				if ( !writeData(*stagingBuffer) )
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
			transferImage (Image & targetImage, size_t requiredBytes, function_t && writeData) noexcept requires (std::is_invocable_v< function_t, const Buffer & >)
			{
				/* [VULKAN-CPU-SYNC] Transfer to GPU (Abusive lock!) */
				const std::lock_guard< std::mutex > lock{m_transferOperationsAccess};

				if ( !this->usable() )
				{
					TraceError{ClassId} << "The transfer manager is not usable !";

					return false;
				}

				TraceDebug{ClassId} << "Initialize an image transfer operation for " << requiredBytes << " bytes (Image:" << targetImage.identifier() << ") ...";

				const auto transferOperation = this->getAndReserveImageTransferOperation(requiredBytes);

				if ( transferOperation == nullptr )
				{
					return false;
				}

				auto * stagingBuffer = transferOperation->stagingBuffer();

				if ( stagingBuffer == nullptr )
				{
					return false;
				}

				if ( !writeData(*stagingBuffer) )
				{
					TraceError{ClassId} << "Unable to write " << requiredBytes << " bytes of data in the staging buffer (Image) !";

					return false;
				}

				return transferOperation->transfer(m_device, targetImage, 0);
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
			BufferTransferOperation * getAndReserveBufferTransferOperation (size_t requiredBytes) noexcept;

			/**
			 * @brief Returns a locked image transfer operation.
			 * @param requiredBytes The size in bytes of the buffer.
			 * @return ImageTransferOperation *
			 */
			[[nodiscard]]
			ImageTransferOperation * getAndReserveImageTransferOperation (size_t requiredBytes) noexcept;

			std::shared_ptr< Device > m_device;
			std::shared_ptr< CommandPool > m_transferCommandPool;
			std::shared_ptr< CommandPool > m_graphicsCommandPool;
			std::vector< BufferTransferOperation > m_bufferTransferOperations;
			std::vector< ImageTransferOperation > m_imageTransferOperations;
			std::unique_ptr< CommandBuffer > m_imageLayoutTransitionCommandBuffer;
			std::unique_ptr< Sync::Fence > m_imageLayoutTransitionFence;
			mutable std::mutex m_transferOperationsAccess;
	};
}
