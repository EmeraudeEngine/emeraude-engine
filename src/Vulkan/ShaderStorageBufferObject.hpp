/*
 * src/Vulkan/ShaderStorageBufferObject.hpp
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

#pragma once

/* STL inclusions. */
#include <memory>

/* Local inclusions. */
#include "AbstractDeviceBuffer.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief Defines a convenient way to build a shader storage buffer object.
	 * @note OpenGL guaranteed a minimum size of 128MB.
	 * @extends EmEn::Vulkan::AbstractDeviceBuffer This is a device-side buffer.
	 */
	class ShaderStorageBufferObject final : public AbstractDeviceBuffer
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanShaderStorageBufferObject"};

			/**
			 * @brief Constructs a shader storage buffer object.
			 * @param device A reference to the device smart pointer.
			 * @param size The size of the buffer.
			 */
			ShaderStorageBufferObject (const std::shared_ptr< Device > & device, VkDeviceSize size) noexcept
				: AbstractDeviceBuffer{device, 0, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT}
			{

			}
	};
}
