/*
 * src/Vulkan/Sync/ImageMemoryBarrier.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Local inclusions for inheritances. */
#include "Vulkan/AbstractObject.hpp"

namespace Emeraude::Vulkan
{
	class Image;
}

namespace Emeraude::Vulkan::Sync
{
	/**
	 * @brief The image memory barrier class. This is used for synchronize command buffers.
	 * @extends Emeraude::Vulkan::AbstractObject This is a sample Vulkan structure.
	 */
	class ImageMemoryBarrier final : public AbstractObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanImageMemoryBarrier"};

			/**
			 * @brief Constructs an image memory barrier.
			 * @param image A reference to a buffer.
			 * @param srcAccessMask A bitmask of VkAccessFlagBits specifying a source access mask.
			 * @param dstAccessMask A bitmask of VkAccessFlagBits specifying a destination access mask.
			 * @param oldLayout The old layout in an image layout transition.
			 * @param newLayout The new layout in an image layout transition.
			 * @param aspectMask Which layer the memory barrier applies. Default COLOR.
			 */
			ImageMemoryBarrier (const Image & image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ImageMemoryBarrier (const ImageMemoryBarrier & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ImageMemoryBarrier (ImageMemoryBarrier && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			ImageMemoryBarrier & operator= (const ImageMemoryBarrier & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			ImageMemoryBarrier & operator= (ImageMemoryBarrier && copy) noexcept = default;

			/**
			 * @brief Destructs an image memory barrier.
			 */
			~ImageMemoryBarrier () override;

			/**
			 * @brief Target a specific mipMap level.
			 * @param offset The mip level offset.
			 * @param count The number of mip level. Default 1.
			 * @return void
			 */
			void targetMipLevel (uint32_t offset, uint32_t count = 1) noexcept;

			/**
			 * @brief Target a specific array layer.
			 * @param offset The array layer offset.
			 * @param count The number of array layer. Default 1.
			 * @return void
			 */
			void targetLayer (uint32_t offset, uint32_t count = 1) noexcept;

			/**
			 * @brief Returns a reference to the image memory barrier vulkan structure.
			 * @return const VkImageMemoryBarrier &
			 */
			[[nodiscard]]
			const VkImageMemoryBarrier &
			get () const noexcept
			{
				return m_barrier;
			}

		private:

			VkImageMemoryBarrier m_barrier{};
	};
}
