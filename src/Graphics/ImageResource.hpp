/*
 * src/Graphics/ImageResource.hpp
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
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Resources/Container.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The image resource class.
	 * This is a wrapper around PixelFactory's Pixmap class to make it a loadable resource.
	 * @extends EmEn::Resources::ResourceTrait This is a loadable resource.
	 */
	class ImageResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< ImageResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ImageResource"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs an image resource.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			explicit
			ImageResource (std::string name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{std::move(name), resourceFlags}
			{

			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const std::filesystem::path &) */
			bool load (const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this) + m_pixmap.bytes< size_t >();
			}

			/**
			 * @brief Returns the pixmap.
			 * @return const Libraries::PixelFactory::Pixmap< uint8_t > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Pixmap< uint8_t > &
			data () const noexcept
			{
				return m_pixmap;
			}

			/**
			 * @brief Returns the width of the image.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			width () const noexcept
			{
				return m_pixmap.width();
			}

			/**
			 * @brief Returns the height of the image.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			height () const noexcept
			{
				return m_pixmap.height();
			}

			/**
			 * @brief Returns whether the image is a gray scale.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isGrayScale () const noexcept
			{
				return m_pixmap.isGrayScale();
			}

			/**
			 * @brief Returns the average color of the cubemap.
			 * @return Libraries::PixelFactory::Color< float >
			 */
			[[nodiscard]]
			Libs::PixelFactory::Color< float >
			averageColor () const noexcept
			{
				return m_pixmap.averageColor();
			}

		private:

			Libs::PixelFactory::Pixmap< uint8_t > m_pixmap;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Images = Container< Graphics::ImageResource >;
}
