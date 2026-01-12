/*
 * src/Vulkan/ImageView.hpp
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

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"
#include "Image.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The image view class
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class ImageView final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanImageView"};

			/**
			 * @brief Constructs an image view.
			 * @param image A reference to an image smart pointer.
			 * @param viewType
			 * @param subresourceRange
			 * @param components Default unchanged.
			 * @param createFlags The createInfo flags. Default none.
			 */
			ImageView (const std::shared_ptr< Image > & image, VkImageViewType viewType, VkImageSubresourceRange subresourceRange, VkComponentMapping components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY}, VkImageViewCreateFlags createFlags = 0) noexcept
				: AbstractDeviceDependentObject{image->device()},
				m_image{image}
			{
				m_createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				m_createInfo.pNext = nullptr;
				m_createInfo.flags = createFlags;
				m_createInfo.image = VK_NULL_HANDLE;
				m_createInfo.viewType = viewType;
				m_createInfo.format = VK_FORMAT_UNDEFINED;
				m_createInfo.components = components;
				m_createInfo.subresourceRange = subresourceRange;
			}

			/**
			 * @brief Constructs an image view with a createInfo.
			 * @param image A reference to an image smart pointer.
			 * @param createInfo A reference to a createInfo.
			 */
			ImageView (const std::shared_ptr< Image > & image, const VkImageViewCreateInfo & createInfo) noexcept
				: AbstractDeviceDependentObject{image->device()},
				m_createInfo{createInfo},
				m_image{image}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ImageView (const ImageView & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ImageView (ImageView && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			ImageView & operator= (const ImageView & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			ImageView & operator= (ImageView && copy) noexcept = default;

			/**
			 * @brief Destructs the image view.
			 */
			~ImageView () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns the image view vulkan handle.
			 * @return VkImageView
			 */
			[[nodiscard]]
			VkImageView
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the image view createInfo.
			 * @return const VkImageViewCreateInfo &
			 */
			[[nodiscard]]
			const VkImageViewCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Returns the associated image smart pointer.
			 * @return std::shared_ptr< Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Image >
			image () const noexcept
			{
				return m_image;
			}

		private:

			VkImageView m_handle{VK_NULL_HANDLE};
			VkImageViewCreateInfo m_createInfo{};
			std::shared_ptr< Image > m_image;
	};
}
