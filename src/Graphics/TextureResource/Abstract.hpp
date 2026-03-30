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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Local inclusions for inheritances. */
#include "Vulkan/TextureInterface.hpp"
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Graphics::TextureResource
{
	/**
	 * @brief This is the base class for every vulkan texture resource loaded from disk.
	 * @extends EmEn::Vulkan::TextureInterface This provides GPU texture capabilities.
	 * @extends EmEn::Resources::ResourceTrait This is a loadable resource.
	 */
	class Abstract : public Vulkan::TextureInterface, public Resources::ResourceTrait
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
			virtual Libs::PixelFactory::Color< float > averageColor () const noexcept = 0;

			/**
			 * @brief Validates a pixmap for Vulkan requirements.
			 * @param classId A pointer to the class id validating the pixmap.
			 * @param resourceName A reference to a string.
			 * @param pixmap A reference to a pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			static bool validatePixmap (const char * classId, const std::string & resourceName, Libs::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept;

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
			bool validateTexture (const Libs::PixelFactory::Pixmap< uint8_t > & pixmap, bool disablePowerOfTwoCheck) const noexcept;

			/**
			 * @brief Applies the normal map Y flip if enabled.
			 * @note Call this after validatePixmap() and before GPU upload.
			 * @param pixmap A reference to the pixmap to modify.
			 */
			void applyFlipNormalMapY (Libs::PixelFactory::Pixmap< uint8_t > & pixmap) const noexcept;

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			bool m_sRGB{false};
			bool m_flipNormalMapY{false};
	};
}
