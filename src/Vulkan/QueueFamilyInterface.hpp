/*
 * src/Vulkan/QueueFamilyInterface.hpp
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
#include <cstdint>
#include <utility>
#include <vector>
#include <memory>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "Types.hpp"

/* Forward declaration. */
namespace EmEn::Vulkan
{
	class Device;
	class Queue;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief The queue family interface.
	 */
	class QueueFamilyInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			QueueFamilyInterface (const QueueFamilyInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			QueueFamilyInterface (QueueFamilyInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return QueueFamilyInterface &
			 */
			QueueFamilyInterface & operator= (const QueueFamilyInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return QueueFamilyInterface &
			 */
			QueueFamilyInterface & operator= (QueueFamilyInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the queue family interface.
			 */
			virtual ~QueueFamilyInterface () = default;

			/**
			 * @brief Returns the queue family index in the physical device.
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual uint32_t index () const noexcept = 0;

			/**
			 * @brief Returns the maximum available queue for this family.
			 * @return size_t
			 */
			[[nodiscard]]
			virtual size_t maxQueueCount () const noexcept = 0;

			/**
			 * @brief Returns whether this family has a single queue.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool hasSingleQueue () const noexcept = 0;

			/**
			 * @brief Declares the queues structures inside the family.
			 * @param structure A reference to a vector for the desired job and priority.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool declareQueueStructure (const std::vector< std::pair< QueueJob, float > > & structure) noexcept = 0;

			/**
			 * @brief Returns the createInfo to build the queue family setup on the device.
			 * @param createFlag The flag for queue creation.
			 * @return void
			 */
			[[nodiscard]]
			virtual VkDeviceQueueCreateInfo getCreateInfo (VkDeviceQueueCreateFlags createFlag) const noexcept = 0;

			/**
			 * @brief Retrievers the allocated queues on the device for this queue family.
			 * @param device A reference to the device smart pointer where queues has been created.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool retrieveQueuesFromDevice (const std::shared_ptr< Device > & device) noexcept  = 0;

			/**
			 * @brief Returns the queue for a job type.
			 * @param job The job type to be performed by the queue.
			 * @param priority The priority of the queue.
			 * @return Queue *
			 */
			[[nodiscard]]
			virtual Queue * queue (QueueJob job, QueuePriority priority) noexcept = 0;

		protected:

			/** 
			 * @brief Constructs a queue family interface.
			 */
			QueueFamilyInterface () noexcept = default;
	};
}
