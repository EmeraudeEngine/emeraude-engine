/*
 * src/Graphics/TextureResource/Abstract.hpp
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

/* Local inclusions for inheritances. */
#include "Vulkan/TextureInterface.hpp"
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "PixelFactory/Color.hpp"
#include "PixelFactory/Pixmap.hpp"

namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Graphics::TextureResource
{
	/**
	 * @brief How texture coordinates outside [0, 1] resolve.
	 * @note These are exactly the three modes a glTF sampler can declare (wrapS / wrapT), and
	 * each maps one-to-one onto a VkSamplerAddressMode. An asset's choice here is not cosmetic:
	 * a texture authored with a border and sampled with CLAMP_TO_EDGE shows that border once,
	 * while REPEAT tiles it — the Khronos TextureTransformTest reads as a failure on exactly
	 * that difference.
	 */
	enum class WrapMode : uint8_t
	{
		Repeat,
		MirroredRepeat,
		ClampToEdge
	};

	/**
	 * @brief Converts a wrap mode to its Vulkan address mode.
	 * @param mode The wrap mode.
	 * @return VkSamplerAddressMode
	 */
	[[nodiscard]]
	constexpr
	VkSamplerAddressMode
	toVulkanAddressMode (WrapMode mode) noexcept
	{
		switch ( mode )
		{
			case WrapMode::MirroredRepeat :
				return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

			case WrapMode::ClampToEdge :
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

			case WrapMode::Repeat :
				break;
		}

		return VK_SAMPLER_ADDRESS_MODE_REPEAT;
	}

	/**
	 * @brief Returns the one-letter code of a wrap mode, for a sampler cache identifier.
	 * @param mode The wrap mode.
	 * @return char
	 */
	[[nodiscard]]
	constexpr
	char
	wrapModeCode (WrapMode mode) noexcept
	{
		switch ( mode )
		{
			case WrapMode::MirroredRepeat :
				return 'M';

			case WrapMode::ClampToEdge :
				return 'C';

			case WrapMode::Repeat :
				break;
		}

		return 'R';
	}

	/**
	 * @brief This is the base class for every vulkan texture resource loaded from disk.
	 * @extends EmEn::Vulkan::TextureInterface This provides GPU texture capabilities.
	 * @extends EmEn::Resources::ResourceTrait This is a loadable resource.
	 */
	class EMEN_API Abstract : public Vulkan::TextureInterface, public Resources::ResourceTrait
	{
		public:

			/**
			 * @brief Destructs the abstract texture resource.
			 */
			~Abstract () override = default;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (Abstract && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (Abstract && copy) noexcept = delete;

			/**
			 * @brief Creates the texture objects in the video memory.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool createTexture (Renderer & renderer) noexcept = 0;

			/**
			 * @brief Destroys the texture objects from the video memory.
			 * @return bool
			 */
			virtual bool destroyTexture () noexcept = 0;

			/**
			 * @brief Returns whether the texture is grayscale or not.
			 * @note This should be done by a local data analysis.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isGrayScale () const noexcept = 0;

			/**
			 * @brief Returns the average color of the texture.
			 * @note This should be done by a local data analysis.
			 * @return Color< float >
			 */
			[[nodiscard]]
			virtual Base::PixelFactory::Color< float > averageColor () const noexcept = 0;

			/**
			 * @brief Validates a pixmap for Vulkan requirements.
			 * @param classId A pointer to the class id validating the pixmap.
			 * @param resourceName A reference to a string.
			 * @param pixmap A reference to a pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			static bool validatePixmap (const char * classId, const std::string & resourceName, Base::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept;

			/**
			 * @brief Enables sRGB format for this texture resource.
			 * @note Must be called before createTexture(). Color textures (albedo, emissive, etc.)
			 * should use sRGB so the GPU automatically converts sRGB to linear on sampling.
			 * Data textures (normal, roughness, metallic, AO) must remain UNORM (linear).
			 * @param enable True for sRGB, false for linear (UNORM).
			 */
			void
			enableSRGB (bool enable) noexcept
			{
				m_sRGB = enable;
			}

			/**
			 * @brief Returns whether this texture uses sRGB format.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSRGB () const noexcept
			{
				return m_sRGB;
			}

			/**
			 * @brief Enables flipping the green (Y) channel of a normal map at load time.
			 * @note Converts between OpenGL (Y+ up) and DirectX (Y+ down) normal map conventions.
			 * @param enable True to flip the Y channel before GPU upload.
			 */
			void
			enableFlipNormalMapY (bool enable) noexcept
			{
				m_flipNormalMapY = enable;
			}

			/**
			 * @brief Returns whether the normal map Y channel flip is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFlipNormalMapYEnabled () const noexcept
			{
				return m_flipNormalMapY;
			}

			/**
			 * @brief Sets how texture coordinates outside [0, 1] resolve.
			 * @warning Must be called BEFORE the texture is created on hardware: the sampler is
			 * built once, from these values, and never revisited.
			 * @note This carries the asset's own intent (glTF wrapS / wrapT). Defaults to repeat
			 * on both axes, which is both the Vulkan and the glTF default.
			 * @param wrapU Horizontal (S) wrap mode.
			 * @param wrapV Vertical (T) wrap mode.
			 * @return void
			 */
			void
			setWrapModes (WrapMode wrapU, WrapMode wrapV) noexcept
			{
				m_wrapU = wrapU;
				m_wrapV = wrapV;
			}

			/**
			 * @brief Returns the horizontal (S) wrap mode.
			 * @return WrapMode
			 */
			[[nodiscard]]
			WrapMode
			wrapModeU () const noexcept
			{
				return m_wrapU;
			}

			/**
			 * @brief Returns the vertical (T) wrap mode.
			 * @return WrapMode
			 */
			[[nodiscard]]
			WrapMode
			wrapModeV () const noexcept
			{
				return m_wrapV;
			}

			/**
			 * @brief Returns whether both axes keep the default repeat mode.
			 * @note Lets a caller keep the historical sampler identifier for the common case,
			 * so the sampler cache is not fragmented by textures that ask for nothing special.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			usesDefaultWrapModes () const noexcept
			{
				return m_wrapU == WrapMode::Repeat && m_wrapV == WrapMode::Repeat;
			}

		protected:

			/**
			 * @brief Constructs an abstract texture resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param textureName A string for the texture name [std::move].
			 * @param textureFlags The resource flag bits.
			 */
			Abstract (Resources::AbstractServiceProvider & serviceProvider, std::string textureName, uint32_t textureFlags) noexcept
				: ResourceTrait{serviceProvider, std::move(textureName), textureFlags}
			{

			}

			/**
			 * @brief Validates a texture for Vulkan requirements.
			 * @note This method is called just before sending the texture to the GPU.
			 * @param pixmap A reference to a pixmap.
			 * @param disablePowerOfTwoCheck Disable the check for size pixmap check.
			 * @return bool
			 */
			[[nodiscard]]
			bool validateTexture (const Base::PixelFactory::Pixmap< uint8_t > & pixmap, bool disablePowerOfTwoCheck) const noexcept;

			/**
			 * @brief Applies the normal map Y flip if enabled.
			 * @note Call this after validatePixmap() and before GPU upload.
			 * @param pixmap A reference to the pixmap to modify.
			 */
			void applyFlipNormalMapY (Base::PixelFactory::Pixmap< uint8_t > & pixmap) const noexcept;

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			/* NOTE: The asset's own addressing intent, consumed once when the sampler is built. */
			WrapMode m_wrapU{WrapMode::Repeat};
			WrapMode m_wrapV{WrapMode::Repeat};
			bool m_sRGB{false};
			bool m_flipNormalMapY{false};
	};
}
