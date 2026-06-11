/*
 * src/Overlay/Framebuffer.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <memory>

/* Local inclusions. */
#include "PixelFactory/Pixmap.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/DescriptorSet.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class ImageView;
}

namespace EmEn::Overlay
{
	/**
	 * @brief Encapsulates all resources for a single framebuffer.
	 * @details This structure groups the local pixmap data with its corresponding
	 * GPU resources (image, image view, descriptor set). Used internally by Surface
	 * for both single and double buffer modes.
	 */
	class Framebuffer final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"UIFramebuffer"};

			/**
			 * @brief Constructs a default framebuffer.
			 */
			Framebuffer () noexcept = default;

			/**
			 * @brief Checks if all GPU resources are valid and created.
			 * @return bool True if the framebuffer is ready for rendering.
			 */
			[[nodiscard]]
			bool isValid () const noexcept;

			/**
			 * @brief Returns the framebuffer width in pixels.
			 * @return uint32_t The width, or 0 if not initialized.
			 */
			[[nodiscard]]
			uint32_t width () const noexcept;

			/**
			 * @brief Returns the framebuffer height in pixels.
			 * @return uint32_t The height, or 0 if not initialized.
			 */
			[[nodiscard]]
			uint32_t height () const noexcept;

			/**
			 * @brief Checks if the image dimensions match the given size.
			 * @param targetWidth Target width in pixels.
			 * @param targetHeight Target height in pixels.
			 * @return bool True if dimensions match.
			 */
			[[nodiscard]]
			bool
			matchesSize (uint32_t targetWidth, uint32_t targetHeight) const noexcept
			{
				return this->width() == targetWidth && this->height() == targetHeight;
			}

			/**
			 * @brief Destroys all GPU resources.
			 * @return void
			 */
			void destroy () noexcept;

			/**
			 * @brief Writes to the GPU image using memory mapping with RAII safety.
			 * @details Maps the GPU memory, calls the provided function with the mapped pointer
			 * and row pitch, then unmaps automatically. Only works when image is host visible.
			 * @tparam function_t Function type accepting (void* mappedPtr, VkDeviceSize rowPitch) and returning bool.
			 * @param writeFunction The function to call with the mapped memory.
			 * @return bool True if mapping and write succeeded, false otherwise.
			 */
			template< typename function_t >
			requires std::invocable< function_t, void *, VkDeviceSize > && std::convertible_to< std::invoke_result_t< function_t, void *, VkDeviceSize >, bool >
			[[nodiscard]]
			bool
			writeWithMapping (function_t && writeFunction) const noexcept
			{
				if ( image == nullptr || !image->isHostVisible() )
				{
					return false;
				}

				void * mappedPtr = image->mapMemory();

				if ( mappedPtr == nullptr )
				{
					return false;
				}

				const auto rowPitch = image->rowPitch();

				const bool result = std::forward< function_t >(writeFunction)(mappedPtr, rowPitch);

				image->unmapMemory();

				return result;
			}

			/* TODO: Get these members in a private section. */

			/** @brief Local pixmap data (CPU-side). Only used when memory mapping is disabled. */
			Base::PixelFactory::Pixmap< uint8_t > pixmap;
			/** @brief Vulkan image on GPU. */
			std::shared_ptr< Vulkan::Image > image;
			/** @brief Vulkan image view for the image. */
			std::shared_ptr< Vulkan::ImageView > imageView;
			/** @brief Descriptor set binding the image for shader access. */
			std::unique_ptr< Vulkan::DescriptorSet > descriptorSet;
	};
}
