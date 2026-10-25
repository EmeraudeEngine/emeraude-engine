/*
 * src/Graphics/StorageTexture3D.hpp
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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <string>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions for inheritances. */
#include "Vulkan/TextureInterface.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Vulkan
{
	class Image;
	class ImageView;
	class Sampler;
}

namespace EmEn::Graphics
{
	/**
	 * @brief A 3D texture a compute shader writes (a storage view) and any shader samples (a trilinear, clamped
	 * sampler): a volume lookup table (the aerial perspective's froxels, SkyAtmosphere).
	 * @note One mip, one layer. Its content is produced by a compute pass; the image is left UNDEFINED at creation and
	 * the producer moves it to GENERAL then SHADER_READ_ONLY with its own barriers.
	 * @extends EmEn::Vulkan::TextureInterface This is a texture.
	 */
	class EMEN_API StorageTexture3D final : public Vulkan::TextureInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"StorageTexture3D"};

			/**
			 * @brief Constructs a storage texture.
			 * @param name The name of the texture, for the debug labels.
			 * @param extent The size in texels.
			 * @param format The texel format: it must support the storage usage (RGBA16F and RGBA32F always do; linear
			 * filtering of RGBA32F is optional: SkyAtmosphere::tableFormat()).
			 */
			StorageTexture3D (std::string name, VkExtent3D extent, VkFormat format) noexcept;

			/**
			 * @brief Creates the image, its views and its sampler.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer) noexcept;

			/**
			 * @brief Destroys the texture from the GPU.
			 */
			void destroy () noexcept;

			/**
			 * @brief Changes the texel format, before the texture is created.
			 * @param format The texel format (see the constructor).
			 * @return bool False once the texture is created: its format is then fixed.
			 */
			[[nodiscard]]
			bool setFormat (VkFormat format) noexcept;

			/**
			 * @brief Returns the texel format.
			 * @return VkFormat
			 */
			[[nodiscard]]
			VkFormat
			format () const noexcept
			{
				return m_format;
			}

			/**
			 * @brief Returns the view a compute shader writes through.
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			storageView () const noexcept
			{
				return m_storageView;
			}

			/**
			 * @brief Returns the width in texels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			width () const noexcept
			{
				return m_extent.width;
			}

			/**
			 * @brief Returns the height in texels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			height () const noexcept
			{
				return m_extent.height;
			}

			/**
			 * @brief Returns the depth in texels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			depth () const noexcept
			{
				return m_extent.depth;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::isCreated() const noexcept */
			[[nodiscard]]
			bool isCreated () const noexcept override;

			/** @copydoc EmEn::Vulkan::TextureInterface::type() const noexcept */
			[[nodiscard]]
			Vulkan::TextureType
			type () const noexcept override
			{
				return Vulkan::TextureType::Texture3D;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::dimensions() const noexcept */
			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				return 3;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::isCubemapTexture() const noexcept */
			[[nodiscard]]
			bool
			isCubemapTexture () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::image() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			image () const noexcept override
			{
				return m_image;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::imageView() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_imageView;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::sampler() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_sampler;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::request3DTextureCoordinates() const noexcept */
			[[nodiscard]]
			bool
			request3DTextureCoordinates () const noexcept override
			{
				return true;
			}

		private:

			std::string m_name;
			std::shared_ptr< Vulkan::Image > m_image;
			std::shared_ptr< Vulkan::ImageView > m_imageView;
			std::shared_ptr< Vulkan::ImageView > m_storageView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			VkExtent3D m_extent;
			VkFormat m_format;
	};
}
