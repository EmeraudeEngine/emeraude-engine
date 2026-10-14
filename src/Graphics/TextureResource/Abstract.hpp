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
#include <atomic>
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
	class EMEN_LEAN_API Abstract : public Vulkan::TextureInterface, public Resources::ResourceTrait
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
			 * @brief Returns whether the alpha channel is a BINARY coverage mask (foliage, a grate, a
			 * sprite) rather than graded translucency (a glass pane, a decal at a uniform opacity).
			 * @note This is what lets a material tell a MIS-DECLARED cutout from a genuine blend. An
			 * asset may declare `alphaMode = BLEND` while carrying a mask — Sponza's cypress leaves
			 * measure 99.86 % binary — and a blended surface writes depth and G-buffer lanes over its
			 * whole QUAD, so every G-buffer effect then applies on the quad instead of on the visible
			 * leaves. See Graphics::Material::StandardResource::onDependenciesLoaded().
			 * @note The default is false: only a plain 2D texture can carry a coverage mask in this
			 * engine, and false is the answer that changes nothing. Override it in another texture
			 * kind the day one needs to be tested this way.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			isBinaryAlphaMask () const noexcept
			{
				return false;
			}

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
			 * @note Color textures (albedo, emissive, etc.) use sRGB so the GPU converts sRGB to linear on sampling;
			 * data textures (normal, roughness, metallic, AO) remain UNORM (linear).
			 * @note The colour space is part of the texture's IDENTITY: it decides the GPU format, read once when the
			 * image is created. Only the CREATOR sets it, before load() — in the creation function of
			 * `Container::getOrCreateResourceSync()` when a material setter will check it (the asynchronous form sets it
			 * later, on a worker). A store texture needing another colour space is a distinct resource, requested through
			 * getStoreTexture(). Never set it on a shared resource a consumer received: another
			 * consumer would get the format, or the write would race the upload (docs/coordinate-system.md).
			 * @param enable True for sRGB, false for linear (UNORM).
			 * @return bool False, with an error, when the loading has already started: the value is left unchanged.
			 */
			[[nodiscard]]
			bool enableSRGB (bool enable) noexcept;

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
			 * @note Converts a Direct3D-style map (+Y down) into the Khronos convention (+Y up); the flip is applied to
			 * a COPY of the pixels at upload.
			 * @note Part of the texture's IDENTITY, like the colour space: only the creator sets it, before load()
			 * (see enableSRGB()).
			 * @param enable True to flip the Y channel before GPU upload.
			 * @return bool False, with an error, when the loading has already started: the value is left unchanged.
			 */
			[[nodiscard]]
			bool enableFlipNormalMapY (bool enable) noexcept;

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
			 * @note No power-of-two requirement: Vulkan 1.0 core supports non-power-of-two images on every device (the
			 * former check refused valid data on single-queue-family GPUs, owner decision 2026-10-11).
			 * @param pixmap A reference to a pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			bool validateTexture (const Base::PixelFactory::Pixmap< uint8_t > & pixmap) const noexcept;

			/**
			 * @brief Applies the normal map Y flip if enabled.
			 * @note Call this after validatePixmap() and before GPU upload.
			 * @param pixmap A reference to the pixmap to modify.
			 */
			void applyFlipNormalMapY (Base::PixelFactory::Pixmap< uint8_t > & pixmap) const noexcept;

			/**
			 * @brief The onDependencyFailed() of a texture: takes its type's DEFAULT data in place of the failed one.
			 * @note Owner ruling 2026-10-01: a texture whose image (movie, cubemap, volume…) failed shows the default data
			 * of that type and its chain goes on — a glTF with one broken texture renders, the default one there, instead
			 * of rendering nothing. The failure and the substitution are both logged by ResourceTrait.
			 * @tparam data_t The type of the texture's data resource.
			 * @param localData A reference to the texture's data pointer, replaced on success.
			 * @param failed A reference to the dependency that failed.
			 * @return bool False when no loaded default exists (the texture then fails in turn).
			 */
			template< typename data_t >
			[[nodiscard]]
			bool
			takeDefaultData (std::shared_ptr< data_t > & localData, const ResourceTrait & failed) noexcept
			{
				auto * container = this->serviceProvider().template container< data_t >();

				if ( container == nullptr )
				{
					return false;
				}

				auto fallback = container->getDefaultResource();

				if ( fallback == nullptr || fallback.get() == &failed || !fallback->isLoaded() )
				{
					return false;
				}

				localData = std::move(fallback);

				return true;
			}

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			/**
			 * @brief Tells whether an identity property (colour space, normal map flip) may still be set.
			 * @param what A C-string naming the property, for the error message.
			 * @return bool False, with an error, once the loading has started.
			 */
			[[nodiscard]]
			bool isIdentityStillOpen (const char * what) const noexcept;

			/* NOTE: The asset's own addressing intent, consumed once when the sampler is built. */
			WrapMode m_wrapU{WrapMode::Repeat};
			WrapMode m_wrapV{WrapMode::Repeat};
			/* NOTE: Atomic: written by the creator before load(), read by the upload on a pool worker. */
			std::atomic< bool > m_sRGB{false};
			std::atomic< bool > m_flipNormalMapY{false};
	};
}
