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
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class Device;
		class CommandPool;
		class StagingBuffer;
		class AbstractDeviceBuffer;
		class Image;

	}

	class Arguments;
	class Settings;
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
			 * @param type The GPU work type.
			 */
			explicit
			TransferManager (GPUWorkType type) noexcept
				: ServiceInterface{ClassId}
			{
				if ( s_instances.at(static_cast< size_t >(type)) != nullptr )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

					std::terminate();
				}

				s_instances.at(static_cast< size_t >(type)) = this;
			}

			/**
			 * @brief Destructs the transfer manager.
			 */
			~TransferManager () override
			{
				for ( auto & pointer : s_instances )
				{
					if ( pointer == this )
					{
						pointer = nullptr;

						break;
					}
				}
			}

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_flags[ServiceInitialized];
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
			 * @brief Returns the command pool.
			 * @return std::shared_ptr< CommandPool >
			 */
			[[nodiscard]]
			std::shared_ptr< CommandPool >
			commandPool () const noexcept
			{
				return m_specificCommandPool;
			}

			/**
			 * @brief Returns a string with the allocated staging buffers.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getStagingBuffersStatistics () const noexcept;

			/**
			 * @brief Returns a locked staging buffer.
			 * @param bytes The size in bytes of the buffer.
			 * @return std::shared_ptr< StagingBuffer >
			 */
			[[nodiscard]]
			std::shared_ptr< StagingBuffer > getStagingBuffer (size_t bytes) noexcept;

			/**
			 * @brief Transfer a buffer from the CPU to the GPU.
			 * @param stagingBuffer A reference to the staging buffer (CPU side).
			 * @param dstBuffer A reference to the destination buffer (GPU side).
			 * @param offset The offset in the staging buffer where the date to copy starts. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool transfer (const StagingBuffer & stagingBuffer, AbstractDeviceBuffer & dstBuffer, VkDeviceSize offset = 0) noexcept;

			/**
			 * @brief Transfer a buffer from the CPU to the GPU.
			 * @note This version will generate mip-mapping on the GPU and is using two command buffer (transfer and graphics).
			 * @param stagingBuffer A reference to the staging buffer (CPU side).
			 * @param dstImage A reference to the destination image (GPU side).
			 * @param offset The offset in the staging buffer where the date to copy starts. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool transfer (const StagingBuffer & stagingBuffer, Image & dstImage, VkDeviceSize offset = 0) noexcept;

			/**
			 * @brief Returns the instance of the transfer manager.
			 * @todo This method must be removed!
			 * @param type The transfer work type.
			 * @return TransferManager *
			 */
			//[[deprecated("This method must be removed !")]]
			[[nodiscard]]
			static
			TransferManager *
			instance (GPUWorkType type) noexcept
			{
				return s_instances.at(static_cast< size_t >(type)); // FIXME: Remove this
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Creates a staging buffer.
			 * @param bytes The size in bytes of the buffer.
			 * @return std::shared_ptr< StagingBuffer >
			 */
			[[nodiscard]]
			std::shared_ptr< StagingBuffer > createStagingBuffer (size_t bytes) const noexcept;

			/**
			 * @brief Returns a free staging buffer from a previous one or nullptr if no buffer is available.
			 * @param bytes The size in bytes of the buffer.
			 * @return std::shared_ptr< StagingBuffer >
			 */
			[[nodiscard]]
			std::shared_ptr< StagingBuffer > searchFreeStagingBuffer (size_t bytes) const noexcept;

			/* Flag names. */
			static constexpr auto Debug{0UL};
			static constexpr auto ServiceInitialized{1UL};
			static constexpr auto SeparatedQueues{2UL};

			static std::array< TransferManager *, 2 > s_instances;

			std::shared_ptr< Device > m_device;
			std::vector< std::shared_ptr< StagingBuffer > > m_stagingBuffers;
			std::shared_ptr< CommandPool > m_transferCommandPool;
			std::shared_ptr< CommandPool > m_specificCommandPool;
			/* FIXME: Check the usefulness of these mutexes since we use a device mutex to access queues. */
			std::mutex m_stagingBufferMutex;
			std::mutex m_transferMutex;
			std::array< bool, 8 > m_flags{
				false/*Debug*/,
				false/*ServiceInitialized*/,
				false/*SeparatedQueues*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
