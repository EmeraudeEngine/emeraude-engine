/*
 * src/Graphics/GrabPass.hpp
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
#include <cstdint>
#include <memory>

/* Local inclusions for inheritances. */
#include "Vulkan/TextureInterface.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class CommandBuffer;
	}

	namespace Graphics
	{
		class Renderer;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief A grab pass texture that captures the current frame's color output.
	 * @note This texture captures the rendered frame after the render pass ends,
	 * making it available for sampling in the next frame (one-frame delay).
	 * Typical use case: refraction effects on transparent objects.
	 * @extends EmEn::Vulkan::TextureInterface This is a texture.
	 */
	class GrabPass final : public Vulkan::TextureInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GrabPass"};

			/**
			 * @brief Constructs a grab pass texture.
			 */
			GrabPass () noexcept = default;

			/**
			 * @brief Destructs the grab pass texture.
			 */
			~GrabPass () override = default;

			/**
			 * @brief Creates the grab pass texture on the GPU.
			 * @param renderer A reference to the graphics renderer.
			 * @param width The width of the texture.
			 * @param height The height of the texture.
			 * @param format The image format matching the swapchain.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer, uint32_t width, uint32_t height, VkFormat format) noexcept;

			/**
			 * @brief Destroys the grab pass texture from the GPU.
			 * @return void
			 */
			void destroy () noexcept;

			/**
			 * @brief Recreates the grab pass texture with new dimensions.
			 * @param renderer A reference to the graphics renderer.
			 * @param width The new width.
			 * @param height The new height.
			 * @param format The image format matching the swapchain.
			 * @return bool
			 */
			[[nodiscard]]
			bool recreate (Renderer & renderer, uint32_t width, uint32_t height, VkFormat format) noexcept;

			/**
			 * @brief Records the blit command from the swapchain color image to this texture.
			 * @param commandBuffer A reference to the command buffer.
			 * @param srcColorImage A reference to the source swapchain color image.
			 * @return void
			 */
			void recordBlit (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::Image & srcColorImage) const noexcept;

			/** @copydoc EmEn::Vulkan::TextureInterface::isCreated() const noexcept */
			[[nodiscard]]
			bool isCreated () const noexcept override;

			/** @copydoc EmEn::Vulkan::TextureInterface::type() const noexcept */
			[[nodiscard]]
			Vulkan::TextureType type () const noexcept override;

			/** @copydoc EmEn::Vulkan::TextureInterface::dimensions() const noexcept */
			[[nodiscard]]
			uint32_t dimensions () const noexcept override;

			/** @copydoc EmEn::Vulkan::TextureInterface::isCubemapTexture() const noexcept */
			[[nodiscard]]
			bool isCubemapTexture () const noexcept override;

			/** @copydoc EmEn::Vulkan::TextureInterface::image() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image > image () const noexcept override;

			/** @copydoc EmEn::Vulkan::TextureInterface::imageView() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView > imageView () const noexcept override;

			/** @copydoc EmEn::Vulkan::TextureInterface::sampler() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler > sampler () const noexcept override;

			/** @copydoc EmEn::Vulkan::TextureInterface::request3DTextureCoordinates() const noexcept */
			[[nodiscard]]
			bool request3DTextureCoordinates () const noexcept override;

		private:

			std::shared_ptr< Vulkan::Image > m_image;
			std::shared_ptr< Vulkan::ImageView > m_imageView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
	};
}
