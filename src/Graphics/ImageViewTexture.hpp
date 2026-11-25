/*
 * src/Graphics/ImageViewTexture.hpp
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
#include <utility>

/* Local inclusions for inheritances. */
#include "Vulkan/TextureInterface.hpp"

/* Local inclusions for usages. */
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief A 2D texture made of an image, one view of it and a sampler that already exist: what lets a pass sample,
	 * through a table of textures (the bindless one, a material), a view its owner created for itself.
	 * @note It creates nothing and destroys nothing: it shares the three objects with their owner, which keeps
	 * writing the image. The layout a descriptor is written with is the image's current one (Vulkan::TextureInterface).
	 * @extends EmEn::Vulkan::TextureInterface This is a texture.
	 */
	class ImageViewTexture final : public Vulkan::TextureInterface
	{
		public:

			/**
			 * @brief Constructs a texture over existing objects.
			 * @param image The image.
			 * @param imageView The view of it a shader samples (a 2D view).
			 * @param sampler The sampler.
			 */
			ImageViewTexture (std::shared_ptr< Vulkan::Image > image, std::shared_ptr< Vulkan::ImageView > imageView, std::shared_ptr< Vulkan::Sampler > sampler) noexcept
				: m_image{std::move(image)},
				m_imageView{std::move(imageView)},
				m_sampler{std::move(sampler)}
			{

			}

			/** @copydoc EmEn::Vulkan::TextureInterface::isCreated() const noexcept */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				return m_image != nullptr && m_image->isCreated() && m_imageView != nullptr && m_imageView->isCreated() && m_sampler != nullptr && m_sampler->isCreated();
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::type() const noexcept */
			[[nodiscard]]
			Vulkan::TextureType
			type () const noexcept override
			{
				return Vulkan::TextureType::Texture2D;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::dimensions() const noexcept */
			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				return 2;
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
				return false;
			}

		private:

			std::shared_ptr< Vulkan::Image > m_image;
			std::shared_ptr< Vulkan::ImageView > m_imageView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
	};
}
