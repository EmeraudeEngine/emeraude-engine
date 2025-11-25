/*
 * src/Graphics/TextureResource/Texture3D.hpp
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

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Graphics/VolumetricImageResource.hpp"
#include "Resources/Container.hpp"

namespace EmEn::Graphics::TextureResource
{
	/**
	 * @class Texture3D
	 * @brief Three-dimensional Vulkan texture resource loaded from VolumetricImageResource.
	 *
	 * Represents a 3D texture (VK_IMAGE_TYPE_3D with VK_IMAGE_VIEW_TYPE_3D) suitable for volumetric
	 * data such as volume rendering, 3D noise textures, and 3D lookup tables. Unlike Texture1D and
	 * Texture2D which depend on ImageResource, this class depends on VolumetricImageResource for
	 * volumetric pixel data.
	 *
	 * Key characteristics:
	 * - Uses VK_IMAGE_TYPE_3D and VK_IMAGE_VIEW_TYPE_3D
	 * - Anisotropic filtering is disabled (not typical for 3D textures)
	 * - Supports mipmapping based on renderer settings
	 * - Creates the Image via createOnHardware() then transfers data via writeData()
	 * - Requires 3D texture coordinates (U, V, W)
	 * - Inherits fail-safe behavior from ResourceTrait
	 *
	 * Typical usage:
	 * 1. Load texture from filepath or VolumetricImageResource via load() methods
	 * 2. Call createTexture() with a Renderer to upload to GPU
	 * 3. Access Vulkan objects via image(), imageView(), and sampler()
	 * 4. destroyTexture() is called automatically on destruction
	 *
	 * Common use cases:
	 * - Volume rendering (medical imaging, scientific visualization)
	 * - 3D Perlin/Simplex noise textures for procedural generation
	 * - 3D color grading lookup tables (LUTs)
	 * - Volumetric fog or smoke data
	 *
	 * @note The data transfer workflow differs from 2D textures: this class calls createOnHardware()
	 * to allocate the Image, then writeData() to transfer raw bytes from the VolumetricImageResource.
	 *
	 * @extends EmEn::Graphics::TextureResource::Abstract This is a loadable texture resource.
	 * @see EmEn::Graphics::VolumetricImageResource
	 * @see EmEn::Graphics::TextureResource::Abstract
	 * @see EmEn::Vulkan::TextureInterface
	 * @version 0.8.35
	 */
	class Texture3D final : public Abstract
	{
		friend class Resources::Container< Texture3D >;

		using ResourceTrait::load;

		public:

			/**
			 * @brief Class identifier used for tracing and resource identification.
			 * @version 0.8.35
			 */
			static constexpr auto ClassId{"Texture3DResource"};

			/**
			 * @brief Defines the resource dependency complexity level.
			 *
			 * Set to DepComplexity::One because Texture3D depends on a single VolumetricImageResource.
			 *
			 * @version 0.8.35
			 */
			static constexpr auto Complexity{Resources::DepComplexity::One};

			/**
			 * @brief Constructs a 3D texture resource.
			 *
			 * Creates an empty texture resource that must be loaded via load() methods before use.
			 * The texture is not created on the GPU until createTexture() is called.
			 *
			 * @param textureName Unique name identifying this texture resource.
			 * @param textureFlags Optional resource flag bits for future use. Default is 0 (unused).
			 * @version 0.8.35
			 */
			explicit
			Texture3D (std::string textureName, uint32_t textureFlags = 0) noexcept
				: Abstract{std::move(textureName), textureFlags}
			{

			}

			/**
			 * @brief Destructs the texture 3D resource.
			 *
			 * Automatically calls destroyTexture() to cleanup Vulkan objects (Image, ImageView, Sampler).
			 *
			 * @version 0.8.35
			 */
			~Texture3D () override
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
				return Vulkan::TextureType::Texture3D;
			}

			/**
			 * @copydoc EmEn::Vulkan::TextureInterface::dimensions() const noexcept
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				return 3;
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
			 *
			 * @note Always returns true for Texture3D as it requires 3D coordinates (U, V, W).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			request3DTextureCoordinates () const noexcept override
			{
				return true;
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
				static const size_t classUID = EmEn::Libs::Hash::FNV1a(ClassId);

				return classUID;
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
			 * Creates Vulkan Image (VK_IMAGE_TYPE_3D), ImageView (VK_IMAGE_VIEW_TYPE_3D), and Sampler
			 * on the GPU. Uses settings from the renderer for filtering and mipmapping. Anisotropic
			 * filtering is disabled as it is not typical for 3D textures.
			 *
			 * This method first calls createOnHardware() to allocate the Image, then writeData() to
			 * transfer raw bytes from the VolumetricImageResource to GPU memory.
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
			 * Loads the default VolumetricImageResource from the service provider as volumetric data source.
			 *
			 * @version 0.8.35
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider) noexcept override;

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &, const std::filesystem::path &)
			 *
			 * Loads a VolumetricImageResource from the specified filepath and sets it as the volumetric data source.
			 *
			 * @version 0.8.35
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept override;

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::load(Resources::Manager &, const Json::Value &)
			 *
			 * Not intended to be used for Texture3D resources. Always returns false.
			 *
			 * @note This resource has no local store and cannot be loaded from JSON data.
			 * @version 0.8.35
			 */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept override;

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept
			 *
			 * Returns only the size of the Texture3D object itself. The actual volumetric data is stored
			 * in the dependent VolumetricImageResource, and GPU memory is managed by Vulkan objects.
			 *
			 * @version 0.8.35
			 */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				/* NOTE: The resource itself doesn't contain loaded data. */
				return sizeof(*this);
			}

			/**
			 * @brief Loads texture from an existing VolumetricImageResource.
			 *
			 * Establishes a dependency on the provided VolumetricImageResource for volumetric data.
			 * The VolumetricImageResource must be loaded before this texture can be created on the GPU.
			 *
			 * @param volumetricImageResource Shared pointer to a VolumetricImageResource containing the volumetric data. Must not be null.
			 * @return True if the VolumetricImageResource was successfully set as a dependency, false otherwise.
			 * @version 0.8.35
			 */
			bool load (const std::shared_ptr< VolumetricImageResource > & volumetricImageResource) noexcept;

			/**
			 * @brief Returns the dependent VolumetricImageResource containing volumetric data.
			 *
			 * @return Shared pointer to the VolumetricImageResource, or nullptr if not loaded.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< VolumetricImageResource >
			localData () noexcept
			{
				return m_localData;
			}

		private:

			std::shared_ptr< VolumetricImageResource > m_localData; ///< Dependent VolumetricImageResource providing volumetric data.
			std::shared_ptr< Vulkan::Image > m_image;               ///< Vulkan Image object (VK_IMAGE_TYPE_3D) on GPU.
			std::shared_ptr< Vulkan::ImageView > m_imageView;       ///< Vulkan ImageView (VK_IMAGE_VIEW_TYPE_3D) for shader access.
			std::shared_ptr< Vulkan::Sampler > m_sampler;           ///< Vulkan Sampler with filtering settings (no anisotropy).
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Texture3Ds = Container< Graphics::TextureResource::Texture3D >;
}
