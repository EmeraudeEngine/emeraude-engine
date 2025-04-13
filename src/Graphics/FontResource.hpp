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

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

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

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/**
			 * @brief Constructs a font resource.
			 * @param name A reference to a string for resource name.
			 * @param resourceFlagBits The resource flag bits. Default none. (Unused yet)
			 */
			explicit FontResource (const std::string & name, uint32_t resourceFlagBits = 0) noexcept;

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

			/**
			 * @brie Returns the font.
			 * @return const Libs::PixelFactory::Font< uint8_t > &
			 */
			const Libs::PixelFactory::Font< uint8_t > &
			font () const noexcept
			{
				return m_data;
			}

			/**
			 * @brief Returns a font resource by its name.
			 * @param resourceName A reference to a string.
			 * @param directLoad Use the direct loading mode. Default false.
			 * @return std::shared_ptr< FontResource >
			 */
			[[nodiscard]]
			static std::shared_ptr< FontResource > get (const std::string & resourceName, bool directLoad = false) noexcept;

			/**
			 * @brief Returns the default font resource.
			 * @return std::shared_ptr< FontResource >
			 */
			[[nodiscard]]
			static std::shared_ptr< FontResource > getDefault () noexcept;

		private:

			/** @copydoc EmEn::Resources::ResourceTrait::onDependenciesLoaded() */
			[[nodiscard]]
			bool onDependenciesLoaded () noexcept override;

			Libs::PixelFactory::Font< uint8_t > m_data;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Fonts = Container< Graphics::FontResource >;
}
