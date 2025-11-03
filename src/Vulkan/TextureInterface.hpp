/*
 * src/Vulkan/TextureInterface.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <memory>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions. */
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief Texture type enumeration.
	 * @todo Adds every type of texture (Multisampling, shadows, non-float, ...)
	 */
	enum class TextureType : uint8_t
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
	 * @brief Pure interface for any object usable as a texture in Vulkan.
	 * @note This interface guarantees GPU texture capabilities without file loading logic.
	 */
	class TextureInterface
	{
		public:

			/**
			 * @brief Destructs the texture interface.
			 */
			virtual ~TextureInterface () = default;

			/**
			 * @brief Returns the descriptor image info for shaders.
			 * @return VkDescriptorImageInfo
			 */
			[[nodiscard]]
			VkDescriptorImageInfo getDescriptorInfo () const noexcept;

			/**
			 * @brief Returns whether the texture is created on GPU.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isCreated () const noexcept = 0;

			/**
			 * @brief Returns the texture type.
			 * @return TextureType
			 */
			[[nodiscard]]
			virtual TextureType type () const noexcept = 0;

			/**
			 * @brief Returns the number of dimensions (1, 2, or 3).
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual uint32_t dimensions () const noexcept = 0;

			/**
			 * @brief Returns whether this is a cubemap texture.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isCubemapTexture () const noexcept = 0;

			/**
			 * @brief Returns the image of the texture. This is the image data bloc.
			 * @return std::shared_ptr< Image >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Image > image () const noexcept = 0;

			/**
			 * @brief Returns the image view of the texture. This is how to read the image data bloc.
			 * @return std::shared_ptr< ImageView >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< ImageView > imageView () const noexcept = 0;

			/**
			 * @brief Returns the sampler used by of the texture.
			 * @return std::shared_ptr< Sampler >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Sampler > sampler () const noexcept = 0;

			/**
			 * @brief Returns whether the texture needs 3D texture coordinates to be fully functional.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool request3DTextureCoordinates () const noexcept = 0;

			/**
			 * @brief Returns the frame count for animated textures.
			 * @note Override this method for animation. The default implementation returns 1 for static textures.
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual
			uint32_t
			frameCount () const noexcept
			{
				return 1;
			}

			/**
			 * @brief Returns the duration in milliseconds for animated textures.
			 * @note Override this method for animation. The default implementation returns 0 for static textures.
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
			 * @brief Returns the frame index at a specific time.
			 * @note Override this method for animation. The default implementation returns 0 for static textures.
			 * @param sceneTimeMS The scene time in milliseconds.
			 * @return uint32_t
			 */
			[[nodiscard]]
			virtual
			uint32_t
			frameIndexAt (uint32_t /*sceneTimeMS*/) const noexcept
			{
				return 0;
			}

		protected:

			/**
			 * @brie Constructs a default texture interface.
			 */
			TextureInterface () noexcept = default;
	};
}
