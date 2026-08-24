/*
 * src/Graphics/IBLTexture.hpp
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
#include <vector>

/* Local inclusions for inheritances. */
#include "Vulkan/TextureInterface.hpp"

namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Graphics
{
	/**
	 * @brief An engine-baked IBL texture (BRDF LUT, irradiance cubemap or prefiltered cubemap).
	 * @note This texture is GPU-only: it has no CPU-side pixel data and is written by a compute
	 * pass (Graphics::Compute::IBLBaker), then sampled by fragment shaders through the bindless
	 * texture table reserved slots. Every image is created with STORAGE|SAMPLED usage in
	 * RGBA16F — the only 16F layout with mandatory STORAGE_IMAGE support on Vulkan
	 * (R16G16_SFLOAT storage is an optional feature, never rely on it cross-platform).
	 * @extends EmEn::Vulkan::TextureInterface This is a texture.
	 */
	class EMEN_API IBLTexture final : public Vulkan::TextureInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"IBLTexture"};

			/** @brief The purpose of the texture, which drives its dimensions and sampler. */
			enum class Role : uint8_t
			{
				/** @brief Split-sum BRDF LUT (2D, scale/bias on F0 in RG, 1 mip). */
				BRDFLut,
				/** @brief Diffuse irradiance cubemap (cosine-convolved, low frequency, 1 mip). */
				IrradianceCubemap,
				/** @brief GGX-prefiltered specular cubemap (one roughness per mip level). */
				PrefilteredCubemap
			};

			/** @brief BRDF LUT resolution (NdotV x roughness). */
			static constexpr uint32_t BRDFLutSize{128};

			/** @brief Irradiance cubemap face size. The cosine convolution has almost no
			 * high frequency content; bilinear filtering reconstructs the rest. */
			static constexpr uint32_t IrradianceSize{32};

			/** @brief Prefiltered cubemap face size at mip 0 (perfect mirror). */
			static constexpr uint32_t PrefilteredSize{128};

			/** @brief Prefiltered cubemap mip count: 128 -> 4 (roughness 0 -> 1). */
			static constexpr uint32_t PrefilteredMipLevels{6};

			/**
			 * @brief Constructs an IBL texture.
			 * @param role The purpose of the texture.
			 */
			explicit IBLTexture (Role role) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			IBLTexture (const IBLTexture & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			IBLTexture (IBLTexture && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return IBLTexture &
			 */
			IBLTexture & operator= (const IBLTexture & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return IBLTexture &
			 */
			IBLTexture & operator= (IBLTexture && copy) noexcept = delete;

			/**
			 * @brief Destructs the IBL texture.
			 */
			~IBLTexture () override = default;

			/**
			 * @brief Creates the texture on the GPU (image, views, sampler). The image layout is
			 * left UNDEFINED: the content must be produced by Graphics::Compute::IBLBaker before
			 * the texture is published to the bindless table.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer) noexcept;

			/**
			 * @brief Destroys the texture from the GPU.
			 * @return void
			 */
			void destroy () noexcept;

			/**
			 * @brief Returns the role of the texture.
			 * @return Role
			 */
			[[nodiscard]]
			Role
			role () const noexcept
			{
				return m_role;
			}

			/**
			 * @brief Returns the number of mip levels of the image.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t mipLevels () const noexcept;

			/**
			 * @brief Returns the per-mip storage view used by compute passes to write the texture.
			 * @note Cubemap roles expose a 2D-array view (6 layers) of the requested mip level,
			 * the LUT role a plain 2D view.
			 * @param mipLevel The targeted mip level.
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView > storageView (uint32_t mipLevel) const noexcept;

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
			 * @brief Returns a display name for the role (identifiers, traces).
			 * @return const char *
			 */
			[[nodiscard]]
			const char *
			roleName () const noexcept
			{
				switch ( m_role )
				{
					case Role::BRDFLut :
						return "BRDFLut";

					case Role::IrradianceCubemap :
						return "IrradianceCubemap";

					case Role::PrefilteredCubemap :
						return "PrefilteredCubemap";

					default :
						return "Unknown";
				}
			}

			std::shared_ptr< Vulkan::Image > m_image;
			std::shared_ptr< Vulkan::ImageView > m_imageView;
			std::vector< std::shared_ptr< Vulkan::ImageView > > m_storageViews;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			Role m_role;
	};
}
