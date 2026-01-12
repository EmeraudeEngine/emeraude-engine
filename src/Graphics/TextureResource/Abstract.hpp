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

			/** @brief Access to the graphics renderer for loading GPU resources. */
			static Renderer * s_graphicsRenderer;

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

		protected:

			/**
			 * @brief Constructs an abstract texture resource.
			 * @param textureName A string for the texture name [std::move].
			 * @param textureFlags The resource flag bits.
			 */
			Abstract (std::string textureName, uint32_t textureFlags) noexcept
				: ResourceTrait{std::move(textureName), textureFlags}
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

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;
	};
}
