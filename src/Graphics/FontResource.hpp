/*
 * src/Graphics/FontResource.hpp
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

/* Local inclusions. */
#include "Libs/PixelFactory/Font.hpp"
#include "Resources/Container.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The font resource class.
	 * This is a wrapper around PixelFactory's Font class to make it a loadable resource.
	 * @extends EmEn::Resources::ResourceTrait This is a loadable resource.
	 */
	class FontResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< FontResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"FontResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a font resource.
			 * @param name A string for resource name [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			explicit
			FontResource (std::string name, uint32_t resourceFlags = 0) noexcept
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
				return sizeof(*this) + m_font.bytes< size_t >();
			}

			/**
			 * @brie Returns the font.
			 * @return const Libs::PixelFactory::Font< uint8_t > &
			 */
			const Libs::PixelFactory::Font< uint8_t > &
			font () const noexcept
			{
				return m_font;
			}

		private:

			Libs::PixelFactory::Font< uint8_t > m_font;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Fonts = Container< Graphics::FontResource >;
}
