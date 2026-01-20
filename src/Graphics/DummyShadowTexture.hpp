/*
 * src/Graphics/DummyShadowTexture.hpp
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

/* Local inclusions for inheritances. */
#include "Vulkan/TextureInterface.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Graphics
{
	/**
	 * @brief A dummy shadow texture (1x1, depth value 1.0 = no shadow).
	 * @note This texture is used when a light doesn't have shadow mapping enabled,
	 * allowing unified descriptor set layouts across all lights.
	 * @extends EmEn::Vulkan::TextureInterface This is a texture.
	 */
	class DummyShadowTexture final : public Vulkan::TextureInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DummyShadowTexture"};

			/**
			 * @brief Constructs a dummy shadow texture.
			 * @param isCubemap Whether this is a cubemap (for point lights) or 2D texture.
			 */
			explicit DummyShadowTexture (bool isCubemap) noexcept;

			/**
			 * @brief Destructs the dummy shadow texture.
			 */
			~DummyShadowTexture () override = default;

			/**
			 * @brief Creates the dummy shadow texture on the GPU.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer) noexcept;

			/**
			 * @brief Destroys the dummy shadow texture from the GPU.
			 * @return void
			 */
			void destroy () noexcept;

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
			bool m_isCubemap{false};
	};
}
