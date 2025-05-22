/*
 * src/Vulkan/StagingBuffer.hpp
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
#include <memory>

/* Local inclusions for inheritances. */
#include "Libs/LockableTrait.hpp"
#include "AbstractHostBuffer.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief This buffer is intended to push data all-purposes buffer from CPU to GPU specific buffer.
	 * @extends EmEn::Vulkan::AbstractHostBuffer This is a host-side buffer.
	 */
	class StagingBuffer final : public Libs::LockableTrait, public AbstractHostBuffer
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanStagingBuffer"};

			/**
			 * @brief Construct a staging buffer.
			 * @param device A reference to a device smart pointer.
			 * @param size The size in bytes. Default empty.
			 */
			explicit
			StagingBuffer (const std::shared_ptr< Device > & device, VkDeviceSize size = 0) noexcept
				: AbstractHostBuffer{device, 0, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT}
			{

			}
	};
}
