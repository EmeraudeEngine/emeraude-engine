/*
 * src/Vulkan/Framebuffer.hpp
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
#include <cstdint>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"
#include "RenderPass.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The framebuffer class
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class Framebuffer final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanFramebuffer"};

			/**
			 * @brief Constructs a framebuffer.
			 * @param renderPass A reference to a RenderPass smart pointer.
			 * @param extent A reference to an VkExtent2D.
			 * @param layerCount The number of framebuffer layers. Default 1.
			 * @param createFlags The createInfo flags. Default none.
			 */
			Framebuffer (const std::shared_ptr< const RenderPass > & renderPass, const VkExtent2D & extent, uint32_t layerCount = 1, VkFramebufferCreateFlags createFlags = 0) noexcept
				: AbstractDeviceDependentObject{renderPass->device()},
				m_renderPass{renderPass}
			{
				m_createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				m_createInfo.pNext = nullptr;
				m_createInfo.flags = createFlags;
				m_createInfo.renderPass = VK_NULL_HANDLE;
				m_createInfo.attachmentCount = 0;
				m_createInfo.pAttachments = nullptr;
				m_createInfo.width = extent.width;
				m_createInfo.height = extent.height;
				m_createInfo.layers = layerCount;

				m_attachments.reserve(3);
			}

			/**
			 * @brief Constructs a framebuffer.
			 * @param renderPass A reference to a RenderPass smart pointer.
			 * @param extent A reference to an VkExtent3D.
			 * @param createFlags The createInfo flags. Default none.
			 */
			Framebuffer (const std::shared_ptr< const RenderPass > & renderPass, const VkExtent3D & extent, VkFramebufferCreateFlags createFlags = 0) noexcept
				: Framebuffer{renderPass, VkExtent2D{extent.width, extent.height}, extent.depth, createFlags}
			{

			}

			/**
			 * @brief Constructs a framebuffer with createInfo.
			 * @param renderPass A reference to a RenderPass smart pointer.
			 * @param createInfo A reference to a createInfo.
			 */
			Framebuffer (const std::shared_ptr< const RenderPass > & renderPass, const VkFramebufferCreateInfo & createInfo) noexcept
				: AbstractDeviceDependentObject{renderPass->device()},
				m_createInfo{createInfo},
				m_renderPass{renderPass}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Framebuffer (const Framebuffer & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Framebuffer (Framebuffer && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Framebuffer & operator= (const Framebuffer & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Framebuffer & operator= (Framebuffer && copy) noexcept = default;

			/**
			 * @brief Destructs the framebuffer.
			 */
			~Framebuffer () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Adds an image view to the framebuffer.
			 * @return void
			 */
			void
			addAttachment (VkImageView imageViewHandle) noexcept
			{
				m_attachments.emplace_back(imageViewHandle);
			}

			/**
			 * @brief Returns the framebuffer vulkan handle.
			 * @return VkFramebuffer
			 */
			[[nodiscard]]
			VkFramebuffer
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the framebuffer createInfo.
			 * @return const VkFramebufferCreateInfo &
			 */
			[[nodiscard]]
			const VkFramebufferCreateInfo &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Returns the render pass associated to this framebuffer.
			 * @return std::shared_ptr< const RenderPass >
			 */
			[[nodiscard]]
			std::shared_ptr< const RenderPass >
			renderPass () const noexcept
			{
				return m_renderPass;
			}

		private:

			VkFramebuffer m_handle{VK_NULL_HANDLE};
			VkFramebufferCreateInfo m_createInfo{};
			std::shared_ptr< const RenderPass > m_renderPass;
			std::vector< VkImageView > m_attachments;
	};
}
