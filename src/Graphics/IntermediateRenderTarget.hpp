/*
 * src/Graphics/IntermediateRenderTarget.hpp
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
#include <string>

/* Local inclusions for inheritances. */
#include "Vulkan/TextureInterface.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class CommandBuffer;
		class Framebuffer;
		class RenderPass;
	}

	namespace Graphics
	{
		class Renderer;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief An offscreen render target for intermediate post-processing passes.
	 * @note Provides Image + ImageView + Sampler + RenderPass + Framebuffer as a self-contained
	 * unit that can be used as both a render target (via beginRenderPass/endRenderPass) and
	 * a texture input (via the TextureInterface).
	 * @extends EmEn::Vulkan::TextureInterface This is usable as a texture.
	 */
	class IntermediateRenderTarget final : public Vulkan::TextureInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"IntermediateRenderTarget"};

			/**
			 * @brief Constructs an intermediate render target.
			 */
			IntermediateRenderTarget () noexcept = default;

			/**
			 * @brief Destructs the intermediate render target.
			 */
			~IntermediateRenderTarget () override = default;

			/**
			 * @brief Creates the render target on the GPU.
			 * @param renderer A reference to the graphics renderer.
			 * @param width The width of the render target.
			 * @param height The height of the render target.
			 * @param format The Vulkan image format (e.g. VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8_UNORM).
			 * @param identifier A string identifier for debug labeling.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer, uint32_t width, uint32_t height, VkFormat format, const std::string & identifier) noexcept;

			/**
			 * @brief Destroys the render target from the GPU.
			 * @return void
			 */
			void destroy () noexcept;

			/**
			 * @brief Recreates the render target with new dimensions.
			 * @param renderer A reference to the graphics renderer.
			 * @param width The new width.
			 * @param height The new height.
			 * @param format The Vulkan image format.
			 * @param identifier A string identifier for debug labeling.
			 * @return bool
			 */
			[[nodiscard]]
			bool recreate (Renderer & renderer, uint32_t width, uint32_t height, VkFormat format, const std::string & identifier) noexcept;

			/**
			 * @brief Returns the framebuffer associated with this render target.
			 * @return const Vulkan::Framebuffer &
			 */
			[[nodiscard]]
			const Vulkan::Framebuffer & framebuffer () const noexcept;

			/**
			 * @brief Returns the render pass associated with this render target.
			 * @return const Vulkan::RenderPass &
			 */
			[[nodiscard]]
			const Vulkan::RenderPass & renderPass () const noexcept;

			/**
			 * @brief Records a render pass begin command into the command buffer.
			 * @param commandBuffer A reference to the active command buffer.
			 * @return void
			 */
			void beginRenderPass (const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Records a render pass end command into the command buffer.
			 * @param commandBuffer A reference to the active command buffer.
			 * @return void
			 */
			void endRenderPass (const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Returns the width of the render target.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			width () const noexcept
			{
				return m_width;
			}

			/**
			 * @brief Returns the height of the render target.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			height () const noexcept
			{
				return m_height;
			}

			/**
			 * @brief Returns the format of the render target.
			 * @return VkFormat
			 */
			[[nodiscard]]
			VkFormat
			format () const noexcept
			{
				return m_format;
			}

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

			/**
			 * @brief Creates a render pass suitable for this render target.
			 * @param device A reference to the device smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createRenderPass (const std::shared_ptr< Vulkan::Device > & device) noexcept;

			/**
			 * @brief Creates the framebuffer for this render target.
			 * @param device A reference to the device smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createFramebuffer (const std::shared_ptr< Vulkan::Device > & device) noexcept;

			std::shared_ptr< Vulkan::Image > m_image;
			std::shared_ptr< Vulkan::ImageView > m_imageView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::shared_ptr< const Vulkan::RenderPass > m_renderPass;
			std::unique_ptr< Vulkan::Framebuffer > m_framebuffer;
			uint32_t m_width{0};
			uint32_t m_height{0};
			VkFormat m_format{VK_FORMAT_UNDEFINED};
	};
}
