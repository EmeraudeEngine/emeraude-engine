/*
 * src/Graphics/TextureResource/Abstract.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class Framebuffer;
		class RenderPass;
		class Image;
		class ImageView;
		class Sampler;
	}

	namespace Graphics
	{
		class Renderer;
	}
}

namespace EmEn::Graphics::TextureResource
{
	/**
	 * @brief Higher level texture type enumeration.
	 * @todo Adds every type of texture (Multisampling, shadows, non-float, ...)
	 */
	enum class Type: uint8_t
	{
		Texture1D = 0,
		Texture2D = 1,
		Texture3D = 2,
		TextureCube = 3,
		Texture1DArray = 4,
		Texture2DArray = 5,
		TextureCubeArray = 6
	};

	/**
	 * @brief This is the base class for every vulkan texture resource.
	 * @extends EmEn::Resources::ResourceTrait This is a loadable resource.
	 */
	class Abstract : public Resources::ResourceTrait
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
			 * @brief Returns the descriptor image info.
			 * @todo Remove this !
			 * @return VkDescriptorBufferInfo
			 */
			[[nodiscard]]
			VkDescriptorImageInfo getDescriptorInfo () const noexcept;

			/**
			 * @brief Returns whether the image is ready.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isCreated () const noexcept = 0;

			/**
			 * @brief Creates the image buffer from local data to video memory.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			virtual bool createOnHardware (Renderer & renderer) noexcept = 0;

			/**
			 * @brief Destroys the image from video memory.
			 * @return bool
			 */
			virtual bool destroyFromHardware () noexcept = 0;

			/**
			 * @brief Returns the final texture type.
			 * @note This is useful for shader generation to get the right sampler/texture type conversion.
			 * @return Type
			 */
			[[nodiscard]]
			virtual Type type () const noexcept = 0;

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
			 * @brief Returns the dimensions for use with texture coordinates. Should be 1, 2 or 3.
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual uint32_t dimensions () const noexcept = 0;

			/**
			 * @brief Returns whether the texture is a cubemap.
			 * @return bool
			 */
			[[nodiscard]]
			virtual
			bool
			isCubemapTexture () const noexcept
			{
				return false;
			}

			/**
			 * @brief Returns the number frame.
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual
			uint32_t
			frameCount () const noexcept
			{
				if ( !this->isLoaded() )
				{
					return 0;
				}

				return 1;
			}

			/**
			 * @brief Returns the duration in milliseconds.
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual
			uint32_t
			duration () const noexcept
			{
				return 0;
			}

			/**
			 * @brief Returns the index of the frame at a specific time.
			 * @param sceneTime The current scene time in milliseconds.
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual
			uint32_t
			frameIndexAt (uint32_t sceneTime) const noexcept
			{
				return 0;
			}

			/**
			 * @brief Returns the image of the texture.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Vulkan::Image > image () const noexcept = 0;

			/**
			 * @brief Returns the image view of the texture.
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Vulkan::ImageView > imageView () const noexcept = 0;

			/**
			 * @brief Returns the sampler used by of the texture.
			 * @return std::shared_ptr< Vulkan::Sampler >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Vulkan::Sampler > sampler () const noexcept = 0;

			/**
			 * @brief Returns whether the texture needs 3D texture coordinates to be fully functional.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool request3DTextureCoordinates () const noexcept = 0;

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
