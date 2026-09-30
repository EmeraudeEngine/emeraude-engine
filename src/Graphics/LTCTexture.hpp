/*
 * src/Graphics/LTCTexture.hpp
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
#include <memory>

/* Local inclusions for inheritances. */
#include "Vulkan/TextureInterface.hpp"

namespace EmEn::Graphics
{
	class Renderer;

	/**
	 * @brief The LTC tables of the GGX BRDF as ONE sampled 2D array texture of two layers (Graphics/LTCTables.hpp):
	 * layer 0 the inverse matrix, layer 1 the magnitude and Fresnel. 64 x 64, RGBA16F, linear filtering, clamped.
	 * @note Created once by the Renderer, bound at binding 1 of every line light's descriptor set (the unified light
	 * layout: UBO + one sampler). A shader samples it at uv = (roughness, √(1 − N·V)) × 63/64 + 0.5/64.
	 * @extends EmEn::Vulkan::TextureInterface This is a texture.
	 */
	class EMEN_API LTCTexture final : public Vulkan::TextureInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LTCTexture"};

			/**
			 * @brief Constructs the LTC texture (nothing on the GPU yet).
			 */
			LTCTexture () noexcept = default;

			LTCTexture (const LTCTexture & copy) noexcept = delete;
			LTCTexture (LTCTexture && copy) noexcept = delete;
			LTCTexture & operator= (const LTCTexture & copy) noexcept = delete;
			LTCTexture & operator= (LTCTexture && copy) noexcept = delete;

			/**
			 * @brief Destructs the LTC texture.
			 */
			~LTCTexture () override = default;

			/**
			 * @brief Uploads the tables to the GPU.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer) noexcept;

			/**
			 * @brief Releases the GPU objects.
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
