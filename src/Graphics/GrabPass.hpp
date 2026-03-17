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
		class Image;
		class ImageView;
		class Sampler;
	}

	namespace Graphics
	{
		class Renderer;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief A grab pass that captures the current frame's color and depth output.
	 * @note The color texture captures the rendered frame after the render pass ends,
	 * making it available for sampling in the next frame (one-frame delay).
	 * The depth texture is captured alongside for depth-based effects (underwater attenuation, etc.).
	 * Typical use case: refraction effects on transparent objects.
	 * @extends EmEn::Vulkan::TextureInterface This is a texture (color).
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
			 * @brief Creates the grab pass textures (color and depth) on the GPU.
			 * @param renderer A reference to the graphics renderer.
			 * @param width The width of the texture.
			 * @param height The height of the texture.
			 * @param colorFormat The image format matching the swapchain color.
			 * @param depthFormat The image format matching the swapchain depth. VK_FORMAT_UNDEFINED to skip depth.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer, uint32_t width, uint32_t height, VkFormat colorFormat, VkFormat depthFormat = VK_FORMAT_UNDEFINED, VkFormat normalsFormat = VK_FORMAT_UNDEFINED, VkFormat materialPropertiesFormat = VK_FORMAT_UNDEFINED) noexcept;

			/**
			 * @brief Destroys the grab pass textures from the GPU.
			 * @return void
			 */
			void destroy () noexcept;

			/**
			 * @brief Recreates the grab pass textures with new dimensions.
			 * @param renderer A reference to the graphics renderer.
			 * @param width The new width.
			 * @param height The new height.
			 * @param colorFormat The image format matching the swapchain color.
			 * @param depthFormat The image format matching the swapchain depth. VK_FORMAT_UNDEFINED to skip depth.
			 * @return bool
			 */
			[[nodiscard]]
			bool recreate (Renderer & renderer, uint32_t width, uint32_t height, VkFormat colorFormat, VkFormat depthFormat = VK_FORMAT_UNDEFINED, VkFormat normalsFormat = VK_FORMAT_UNDEFINED, VkFormat materialPropertiesFormat = VK_FORMAT_UNDEFINED) noexcept;

			/**
			 * @brief Records the blit/copy commands from the swapchain images to this grab pass.
			 * @param commandBuffer A reference to the command buffer.
			 * @param srcColorImage A reference to the source swapchain color image.
			 * @param srcDepthImage A pointer to the source depth image. Null to skip depth copy.
			 * @return void
			 */
			void recordBlit (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::Image & srcColorImage, const Vulkan::Image * srcDepthImage = nullptr, const Vulkan::Image * srcNormalsImage = nullptr, const Vulkan::Image * srcMaterialPropertiesImage = nullptr) const noexcept;

			/**
			 * @brief Returns whether the depth texture is available.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasDepth () const noexcept
			{
				return m_depthImage != nullptr && m_depthImage->isCreated();
			}

			/**
			 * @brief Returns the depth image.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			depthImage () const noexcept
			{
				return m_depthImage;
			}

			/**
			 * @brief Returns the depth image view.
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			depthImageView () const noexcept
			{
				return m_depthImageView;
			}

			/**
			 * @brief Returns the depth sampler.
			 * @return std::shared_ptr< Vulkan::Sampler >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler >
			depthSampler () const noexcept
			{
				return m_depthSampler;
			}

			/**
			 * @brief Builds a VkDescriptorImageInfo from the depth components.
			 * @note Used to register the depth texture in the bindless texture manager.
			 * @return VkDescriptorImageInfo
			 */
			[[nodiscard]]
			VkDescriptorImageInfo depthDescriptorInfo () const noexcept;

			/**
			 * @brief Returns whether the normals texture is available.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasNormals () const noexcept
			{
				return m_normalsImage != nullptr && m_normalsImage->isCreated();
			}

			/**
			 * @brief Returns the normals image.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			normalsImage () const noexcept
			{
				return m_normalsImage;
			}

			/**
			 * @brief Returns the normals image view.
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			normalsImageView () const noexcept
			{
				return m_normalsImageView;
			}

			/**
			 * @brief Returns the normals sampler.
			 * @return std::shared_ptr< Vulkan::Sampler >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler >
			normalsSampler () const noexcept
			{
				return m_normalsSampler;
			}

			/**
			 * @brief Builds a VkDescriptorImageInfo from the normals components.
			 * @return VkDescriptorImageInfo
			 */
			[[nodiscard]]
			VkDescriptorImageInfo normalsDescriptorInfo () const noexcept;

			/**
			 * @brief Returns whether the material properties texture is available.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasMaterialProperties () const noexcept
			{
				return m_materialPropertiesImage != nullptr && m_materialPropertiesImage->isCreated();
			}

			/**
			 * @brief Returns the material properties image.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			materialPropertiesImage () const noexcept
			{
				return m_materialPropertiesImage;
			}

			/**
			 * @brief Returns the material properties image view.
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			materialPropertiesImageView () const noexcept
			{
				return m_materialPropertiesImageView;
			}

			/**
			 * @brief Returns the material properties sampler.
			 * @return std::shared_ptr< Vulkan::Sampler >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler >
			materialPropertiesSampler () const noexcept
			{
				return m_materialPropertiesSampler;
			}

			/**
			 * @brief Builds a VkDescriptorImageInfo from the material properties components.
			 * @return VkDescriptorImageInfo
			 */
			[[nodiscard]]
			VkDescriptorImageInfo materialPropertiesDescriptorInfo () const noexcept;

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

			/* Color grab pass resources. */
			std::shared_ptr< Vulkan::Image > m_image;
			std::shared_ptr< Vulkan::ImageView > m_imageView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;

			/* Depth grab pass resources. */
			std::shared_ptr< Vulkan::Image > m_depthImage;
			std::shared_ptr< Vulkan::ImageView > m_depthImageView;
			std::shared_ptr< Vulkan::Sampler > m_depthSampler;

			/* Normals grab pass resources. */
			std::shared_ptr< Vulkan::Image > m_normalsImage;
			std::shared_ptr< Vulkan::ImageView > m_normalsImageView;
			std::shared_ptr< Vulkan::Sampler > m_normalsSampler;

			/* Material properties grab pass resources. */
			std::shared_ptr< Vulkan::Image > m_materialPropertiesImage;
			std::shared_ptr< Vulkan::ImageView > m_materialPropertiesImageView;
			std::shared_ptr< Vulkan::Sampler > m_materialPropertiesSampler;
	};
}
