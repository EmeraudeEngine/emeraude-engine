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
#include "Tracer.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class Device;
		class CommandPool;
		class Buffer;
		class Image;
		class StagingBuffer;
	}

	class Arguments;
	class Settings;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief The transfer manager service class.
	 * @extends EmEn::ServiceInterface This class is a service.
	 * @todo Improve with multiple transfers at once on high-end GPU. For now the system is locked to one staging buffer at a time.
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

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_serviceInitialized;
			}

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
			 * @tparam function_t The type of lambda to describe the data to write. Signature: bool (const StagingBuffer &).
			 * @param targetBuffer A writable reference to the destination buffer.
			 * @param requiredBytes The amount of data to write in bytes.
			 * @param writeData A reference to a function to write data into the staging buffer.
			 * @return bool
			 */
			template< typename function_t >
			bool
			transfer (Buffer & targetBuffer, size_t requiredBytes, function_t && writeData) noexcept requires (std::is_invocable_v< function_t, const StagingBuffer & >)
			{
				/* [VULKAN-CPU-SYNC] */
				std::cout << "[" << std::this_thread::get_id() << "] TransferManager::transfer(Buffer), wait the device lock ..." << std::endl;
				const std::lock_guard< Device > lock{*m_device};
				std::cout << "[" << std::this_thread::get_id() << "] Lock acquired !" << std::endl;

				const auto stagingBuffer = this->getAndReserveStagingBuffer(requiredBytes);

				if ( stagingBuffer == nullptr )
				{
					return false;
				}

				if ( !writeData(*stagingBuffer) )
				{
					TraceError{ClassId} << "Unable to write " << requiredBytes << " bytes of data in the staging buffer (Buffer) !";

					return false;
				}

				return this->transfer(*stagingBuffer, targetBuffer, 0);
			}

			/**
			 * @brief Transfer data to an image inside the GPU memory.
			 * @tparam function_t The type of lambda to describe the data to write. Signature: bool (const StagingBuffer &).
			 * @param targetImage A writable reference to the destination buffer.
			 * @param requiredBytes The amount of data to write in bytes.
			 * @param writeData A reference to a function to write data into the staging buffer.
			 * @return bool
			 */
			template< typename function_t >
			bool
			transfer (Image & targetImage, size_t requiredBytes, function_t && writeData) noexcept requires (std::is_invocable_v< function_t, const StagingBuffer & >)
			{
				/* [VULKAN-CPU-SYNC] */
				std::cout << "[" << std::this_thread::get_id() << "] TransferManager::transfer(Buffer), wait the device lock ..." << std::endl;
				const std::lock_guard< Device > lock{*m_device};
				std::cout << "[" << std::this_thread::get_id() << "] Lock acquired !" << std::endl;

				const auto stagingBuffer = this->getAndReserveStagingBuffer(requiredBytes);

				if ( stagingBuffer == nullptr )
				{
					return false;
				}

				if ( !writeData(*stagingBuffer) )
				{
					TraceError{ClassId} << "Unable to write " << requiredBytes << " bytes of data in the staging buffer (Image) !";

					return false;
				}

				return this->transfer(*stagingBuffer, targetImage, 0);
			}

			/**
			 * @brief Returns a string with the allocated staging buffers.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getStagingBuffersStatistics () const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Returns a locked staging buffer.
			 * @param bytes The size in bytes of the buffer.
			 * @return std::shared_ptr< StagingBuffer >
			 */
			[[nodiscard]]
			std::shared_ptr< StagingBuffer > getAndReserveStagingBuffer (size_t bytes) noexcept;

			/**
			 * @brief Transfer a buffer from the CPU to the GPU.
			 * @param stagingBuffer A reference to the staging buffer (CPU side).
			 * @param dstBuffer A reference to the destination buffer (GPU side).
			 * @param offset The offset in the staging buffer where the date to copy starts. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool transfer (const StagingBuffer & stagingBuffer, const Buffer & dstBuffer, VkDeviceSize offset = 0) noexcept;

			/**
			 * @brief Transfer a buffer from the CPU to the GPU.
			 * @note This version will generate mip-mapping on the GPU and is using two command buffers (transfer and graphics).
			 * @param stagingBuffer A reference to the staging buffer (CPU side).
			 * @param dstImage A reference to the destination image (GPU side).
			 * @param offset The offset in the staging buffer where the date to copy starts. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool transfer (const StagingBuffer & stagingBuffer, Image & dstImage, VkDeviceSize offset = 0) noexcept;

			std::shared_ptr< Device > m_device;
			std::shared_ptr< CommandPool > m_transferCommandPool;
			std::shared_ptr< CommandPool > m_specificCommandPool;
			std::vector< std::shared_ptr< StagingBuffer > > m_stagingBuffers;
			bool m_serviceInitialized{false};
			bool m_separatedQueues{false};
	};
}
