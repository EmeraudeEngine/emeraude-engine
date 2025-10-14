/*
 * src/Graphics/CubemapResource.hpp
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
#include "Types.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The cubemap resource class.
	 * @extends EmEn::Resources::ResourceTrait This is a loadable resource.
	 */
	class CubemapResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< CubemapResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CubemapResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a cubemap resource.
			 * @param name A string for the resource name [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			explicit
			CubemapResource (std::string name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{std::move(name), resourceFlags}
			{

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
				size_t bytes = sizeof(*this);

				for ( const auto & pixmap : m_faces )
				{
					bytes += pixmap.bytes< size_t >();
				}

				return bytes;
			}

			/**
			 * @brief Loads a cubemap from a packed pixmap.
			 * @param pixmap A reference to the pixmap.
			 * @return bool
			 */
			bool load (const Libs::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept;

			/**
			 * @brief Loads a cubemap from a packed pixmap.
			 * @param pixmaps A reference to a fixed array of pixmaps.
			 * @return bool
			 */
			bool load (const CubemapPixmaps & pixmaps) noexcept;

			/**
			 * @brief Returns the pixmap.
			 * @param faceIndex The face number.
			 * @return const Libraries::PixelFactory::Pixmap< uint8_t > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Pixmap< uint8_t > & data (size_t faceIndex) const noexcept;

			/**
			 * @brief Returns faces of the cubemap.
			 * @return const CubemapPixmaps &
			 */
			[[nodiscard]]
			const CubemapPixmaps &
			faces () const noexcept
			{
				return m_faces;
			}

			/**
			 * @brief Returns the size of the cubemap.
			 * @note Returns the width of the first cubemap face.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			cubeSize () const noexcept
			{
				return m_faces[0].width();
			}

			/**
			 * @brief Returns whether pixmaps are all gray scale.
			 * @return bool
			 */
			[[nodiscard]]
			bool isGrayScale () const noexcept;

			/**
			 * @brief Returns the average color of the cubemap.
			 * @return Libraries::PixelFactory::Color< float >
			 */
			[[nodiscard]]
			Libs::PixelFactory::Color< float > averageColor () const noexcept;

		private:

			/* JSON keys */
			static constexpr auto PackedKey{"Packed"};
			static constexpr auto FileFormatKey{"FileFormat"};

			CubemapPixmaps m_faces;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Cubemaps = Container< Graphics::CubemapResource >;
}
