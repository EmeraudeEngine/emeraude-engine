/*
 * src/Graphics/TextureResource/Texture2D.hpp
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
#include "Resources/Container.hpp"
#include "Graphics/ImageResource.hpp"

namespace EmEn::Graphics::TextureResource
{
	/**
	 * @brief The texture 2D resource class.
	 * @extends EmEn::Graphics::TextureResource::Abstract This is a loadable texture resource.
	 */
	class Texture2D final : public Abstract
	{
		friend class Resources::Container< Texture2D >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Texture2DResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::One};

			/**
			 * @brief Constructs a texture 2D resource.
			 * @param textureName A string for the texture name [std::move].
			 * @param textureFlags The resource flag bits. Default none. (Unused yet)
			 */
			explicit
			Texture2D (std::string textureName, uint32_t textureFlags = 0) noexcept
				: Abstract{std::move(textureName), textureFlags}
			{

			}

			/**
			 * @brief Destructs the texture 2D resource.
			 */
			~Texture2D () override
			{
				this->destroyFromHardware();
			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				static const size_t classUID = EmEn::Libs::Hash::FNV1a(ClassId);

				return classUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::isCreated() */
			[[nodiscard]]
			bool isCreated () const noexcept override;

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::createOnHardware() */
			bool createOnHardware (Renderer & renderer) noexcept override;

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::type() */
			[[nodiscard]]
			Type
			type () const noexcept override
			{
				return Type::Texture2D;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::isGrayScale() */
			[[nodiscard]]
			bool isGrayScale () const noexcept override;

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::averageColor() */
			[[nodiscard]]
			Libs::PixelFactory::Color< float > averageColor () const noexcept override;

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::dimensions() */
			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				return 2;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::image() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			image () const noexcept override
			{
				return m_image;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::imageView() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_imageView;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::sampler() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_sampler;
			}

			/** @copydoc EmEn::Graphics::TextureResource::Abstract::request3DTextureCoordinates() */
			[[nodiscard]]
			bool
			request3DTextureCoordinates () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &) */
			bool load (Resources::ServiceProvider & serviceProvider) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &, const std::filesystem::path &) */
			bool load (Resources::ServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::Manager &, const Json::Value &) */
			bool load (Resources::ServiceProvider & serviceProvider, const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				/* NOTE: The resource its doesn't contain loaded data. */
				return sizeof(*this);
			}

			/**
			 * @brief Loads from an image resource.
			 * @param imageResource A reference to an image resource smart pointer.
			 * @return bool
			 */
			bool load (const std::shared_ptr< ImageResource > & imageResource) noexcept;

			/**
			 * @brief Returns the image resource smart pointer.
			 * @return std::shared_ptr< ImageResource >
			 */
			[[nodiscard]]
			std::shared_ptr< ImageResource > localData () noexcept;

		private:

			std::shared_ptr< ImageResource > m_localData;
			std::shared_ptr< Vulkan::Image > m_image;
			std::shared_ptr< Vulkan::ImageView > m_imageView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Texture2Ds = Container< Graphics::TextureResource::Texture2D >;
}
