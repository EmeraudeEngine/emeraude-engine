/*
 * src/Graphics/StarCatalogueResource.hpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Hash/FNV1a.hpp"
#include "Math/Vector.hpp"
#include "Resources/Types.hpp"

namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn::Graphics
{
	/**
	 * @brief A star catalogue: the stars the physical sky draws as points (engine item dynamic-sky-stars-and-milky-way, lot
	 * S1; graphics doc 42).
	 * @extends EmEn::Resources::ResourceTrait The base resource class.
	 * @note The canonical schema (EmeraudeStarCatalogue v1, data store `StarCatalogues`; projet-alpha's converters write
	 * it from the source catalogues):
	 * @code{.json}
	 * { "format": "EmeraudeStarCatalogue", "version": 1, "name": "...", "source": "...", "credit": "...",
	 *   "frame": "ICRS / J2000 equatorial, epoch 2000.0", "fields": ["ra_deg", "dec_deg", "vmag", "bv", "id", "bvEstimated"],
	 *   "stars": [[101.287083, -16.716111, -1.46, 0.0, "HR 2491"], ...] }
	 * @endcode
	 * A star whose position, V or B-V is missing, non-finite or out of range is refused (counted and traced): the file
	 * is a trust boundary.
	 * @note Immutable once loaded: read by any thread after the resource is loaded (its status publishes it).
	 */
	class EMEN_API StarCatalogueResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< StarCatalogueResource >;

		public:

			/**
			 * @brief A star of the catalogue.
			 */
			struct Star
			{
				/** @brief Unit vector toward the star in the J2000 equatorial frame (x toward the vernal equinox, z toward
				 * the north celestial pole). */
				Base::Math::Vector< 3, float > direction{1.0F, 0.0F, 0.0F};
				/** @brief Johnson V magnitude. */
				float visualMagnitude{0.0F};
				/** @brief Johnson B-V colour index. */
				float colourIndex{0.65F};
			};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"StarCatalogueResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/** @brief The format name the file must declare. */
			static constexpr auto FormatName{"EmeraudeStarCatalogue"};

			/** @brief The format version this reader understands. */
			static constexpr int FormatVersion{1};

			/** @brief The accepted V range: brighter than the Sun's apparent −26.7 or fainter than 30 is refused. */
			static constexpr float MinimumMagnitude{-27.0F};
			static constexpr float MaximumMagnitude{30.0F};

			/** @brief The accepted B-V range (the reddest carbon stars reach ~+5). */
			static constexpr float MinimumColourIndex{-0.6F};
			static constexpr float MaximumColourIndex{6.0F};

			/**
			 * @brief Constructs a star catalogue resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			StarCatalogueResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{serviceProvider, name, resourceFlags}
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
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
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

			/** @copydoc EmEn::Resources::ResourceTrait::load()
			 * @note The neutral resource: an empty catalogue (a sky without stars). */
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
				return sizeof(*this) + (m_stars.capacity() * sizeof(Star)) + m_catalogueName.capacity() + m_credit.capacity();
			}

			/**
			 * @brief Returns the stars.
			 * @return const std::vector< Star > &
			 */
			[[nodiscard]]
			const std::vector< Star > &
			stars () const noexcept
			{
				return m_stars;
			}

			/**
			 * @brief Returns the catalogue's own name ("Yale Bright Star Catalogue, 5th revised edition").
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			catalogueName () const noexcept
			{
				return m_catalogueName;
			}

			/**
			 * @brief Returns the credit line the catalogue asks for.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			credit () const noexcept
			{
				return m_credit;
			}

			/**
			 * @brief Returns the number of entries the file held that were refused.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			refusedCount () const noexcept
			{
				return m_refusedCount;
			}

		private:

			/**
			 * @brief Parses a canonical catalogue document.
			 * @param data A reference to the document's root.
			 * @return bool False (traced) for a wrong format or version, or no "stars" array; refused entries do not fail it.
			 */
			[[nodiscard]]
			bool parse (const Json::Value & data) noexcept;

			std::vector< Star > m_stars;
			std::string m_catalogueName;
			std::string m_credit;
			size_t m_refusedCount{0};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using StarCatalogues = Container< Graphics::StarCatalogueResource >;
}
