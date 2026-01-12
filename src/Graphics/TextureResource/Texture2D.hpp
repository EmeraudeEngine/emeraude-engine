/*
 * src/Graphics/TextureResource/Texture2D.hpp
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
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Resources/Container.hpp"
#include "Graphics/ImageResource.hpp"

namespace EmEn::Graphics::TextureResource
{
	/**
	 * @class Texture2D
	 * @brief Two-dimensional Vulkan texture resource loaded from ImageResource.
	 *
	 * Represents a 2D texture (VK_IMAGE_TYPE_2D with VK_IMAGE_VIEW_TYPE_2D), the most common
	 * texture type used for diffuse maps, normal maps, roughness maps, and other surface properties.
	 * This class depends on an ImageResource for pixel data and creates the necessary Vulkan objects
	 * (Image, ImageView, Sampler) on the GPU.
	 *
	 * Key characteristics:
	 * - Uses VK_IMAGE_TYPE_2D and VK_IMAGE_VIEW_TYPE_2D
	 * - Supports anisotropic filtering based on renderer settings
	 * - Supports mipmapping based on renderer settings
	 * - Inherits fail-safe behavior from ResourceTrait
	 *
	 * Typical usage:
	 * 1. Load texture from filepath or ImageResource via load() methods
	 * 2. Call createTexture() with a Renderer to upload to GPU
	 * 3. Access Vulkan objects via image(), imageView(), and sampler()
	 * 4. destroyTexture() is called automatically on destruction
	 *
	 * Common use cases:
	 * - Albedo/diffuse color maps
	 * - Normal maps for surface detail
	 * - Roughness and metallic maps for PBR
	 * - Ambient occlusion maps
	 * - Emissive maps
	 *
	 * @extends EmEn::Graphics::TextureResource::Abstract This is a loadable texture resource.
	 * @see EmEn::Graphics::ImageResource
	 * @see EmEn::Graphics::TextureResource::Abstract
	 * @see EmEn::Vulkan::TextureInterface
	 * @version 0.8.35
	 */
	class Texture2D final : public Abstract
	{
		friend class Resources::Container< Texture2D >;

		using ResourceTrait::load;

		public:

			/**
			 * @brief Class identifier used for tracing and resource identification.
			 * @version 0.8.35
			 */
			static constexpr auto ClassId{"Texture2DResource"};

			/**
			 * @brief Defines the resource dependency complexity level.
			 *
			 * Set to DepComplexity::One because Texture2D depends on a single ImageResource.
			 *
			 * @version 0.8.35
			 */
			static constexpr auto Complexity{Resources::DepComplexity::One};

			/**
			 * @brief Constructs a 2D texture resource.
			 *
			 * Creates an empty texture resource that must be loaded via load() methods before use.
			 * The texture is not created on the GPU until createTexture() is called.
			 *
			 * @param textureName Unique name identifying this texture resource.
			 * @param textureFlags Optional resource flag bits for future use. Default is 0 (unused).
			 * @version 0.8.35
			 */
			explicit
			Texture2D (std::string textureName, uint32_t textureFlags = 0) noexcept
				: Abstract{std::move(textureName), textureFlags}
			{

			}

			/**
			 * @brief Destructs the texture 2D resource.
			 *
			 * Automatically calls destroyTexture() to cleanup Vulkan objects (Image, ImageView, Sampler).
			 *
			 * @version 0.8.35
			 */
			~Texture2D () override
			{
				this->destroyTexture();
			}

			/**
			 * @copydoc EmEn::Vulkan::TextureInterface::isCreated() const noexcept
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool isCreated () const noexcept override;

			/**
			 * @copydoc EmEn::Vulkan::TextureInterface::type() const noexcept
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Vulkan::TextureType
			type () const noexcept override
			{
				return Vulkan::TextureType::Texture2D;
			}

			/**
			 * @copydoc EmEn::Vulkan::TextureInterface::dimensions() const noexcept
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				return 2;
			}

			/**
			 * @copydoc EmEn::Vulkan::TextureInterface::isCubemapTexture() const noexcept
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isCubemapTexture () const noexcept override
			{
				return false;
			}

			/**
			 * @copydoc EmEn::Vulkan::TextureInterface::image() const noexcept
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			image () const noexcept override
			{
				return m_image;
			}

			/**
			 * @copydoc EmEn::Vulkan::TextureInterface::imageView() const noexcept
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_imageView;
			}

			/**
			 * @copydoc EmEn::Vulkan::TextureInterface::sampler() const noexcept
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_sampler;
			}

			/**
			 * @copydoc EmEn::Vulkan::TextureInterface::request3DTextureCoordinates() const noexcept
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			request3DTextureCoordinates () const noexcept override
			{
				return false;
			}

			/**
			 * @brief Returns the unique compile-time identifier for this class.
			 *
			 * Generates a hash from ClassId using FNV1a algorithm. Thread-safe due to static initialization.
			 *
			 * @return Unique identifier (hash of ClassId).
			 * @version 0.8.35
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Libs::Hash::FNV1a(ClassId);
			}

			/**
			 * @copydoc EmEn::Libs::ObservableTrait::classUID() const
			 * @version 0.8.35
			 */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/**
			 * @copydoc EmEn::Libs::ObservableTrait::is() const
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/**
			 * @copydoc EmEn::Graphics::TextureResource::Abstract::createTexture()
			 *
			 * Creates Vulkan Image (VK_IMAGE_TYPE_2D), ImageView (VK_IMAGE_VIEW_TYPE_2D), and Sampler
			 * on the GPU. Uses settings from the renderer for filtering, mipmapping, and anisotropic
			 * filtering levels.
			 *
			 * @note Requires that load() has been called successfully before creation.
			 * @version 0.8.35
			 */
			bool createTexture (Renderer & renderer) noexcept override;

			/**
			 * @copydoc EmEn::Graphics::TextureResource::Abstract::destroyTexture()
			 *
			 * Releases Vulkan Image, ImageView, and Sampler from GPU memory and resets internal pointers.
			 *
			 * @version 0.8.35
			 */
			bool destroyTexture () noexcept override;

			/**
			 * @copydoc EmEn::Graphics::TextureResource::Abstract::isGrayScale()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool isGrayScale () const noexcept override;

			/**
			 * @copydoc EmEn::Graphics::TextureResource::Abstract::averageColor()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Libs::PixelFactory::Color< float > averageColor () const noexcept override;

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::classLabel() const
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &)
			 *
			 * Loads the default ImageResource from the service provider as pixel data source.
			 *
			 * @version 0.8.35
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider) noexcept override;

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &, const std::filesystem::path &)
			 *
			 * Loads an ImageResource from the specified filepath and sets it as the pixel data source.
			 *
			 * @version 0.8.35
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept override;

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::load(Resources::Manager &, const Json::Value &)
			 *
			 * Not intended to be used for Texture2D resources. Always returns false.
			 *
			 * @note This resource has no local store and cannot be loaded from JSON data.
			 * @version 0.8.35
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept override;

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept
			 *
			 * Returns only the size of the Texture2D object itself. The actual pixel data is stored
			 * in the dependent ImageResource, and GPU memory is managed by Vulkan objects.
			 *
			 * @version 0.8.35
			 */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				/* NOTE: The resource its doesn't contain loaded data. */
				return sizeof(*this);
			}

			/**
			 * @brief Loads texture from an existing ImageResource.
			 *
			 * Establishes a dependency on the provided ImageResource for pixel data. The ImageResource
			 * must be loaded before this texture can be created on the GPU.
			 *
			 * @param imageResource Shared pointer to an ImageResource containing the pixel data. Must not be null.
			 * @return True if the ImageResource was successfully set as a dependency, false otherwise.
			 * @version 0.8.35
			 */
			bool load (const std::shared_ptr< ImageResource > & imageResource) noexcept;

			/**
			 * @brief Returns the dependent ImageResource containing pixel data.
			 *
			 * @return Shared pointer to the ImageResource, or nullptr if not loaded.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< ImageResource > localData () noexcept;

		private:

			std::shared_ptr< ImageResource > m_localData;	 ///< Dependent ImageResource providing pixel data.
			std::shared_ptr< Vulkan::Image > m_image;		 ///< Vulkan Image object (VK_IMAGE_TYPE_2D) on GPU.
			std::shared_ptr< Vulkan::ImageView > m_imageView; ///< Vulkan ImageView (VK_IMAGE_VIEW_TYPE_2D) for shader access.
			std::shared_ptr< Vulkan::Sampler > m_sampler;	 ///< Vulkan Sampler with filtering and anisotropy settings.
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Texture2Ds = Container< Graphics::TextureResource::Texture2D >;
}
