/*
 * src/Audio/PlaylistResource.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usage. */
#include "Resources/Container.hpp"

namespace EmEn::Audio
{
	/**
	 * @brief Resource class for a music playlist manifest.
	 * @extends EmEn::Resources::ResourceTrait The base resource class.
	 * @note The manifest is a JSON file whose minimal schema is:
	 * @code{.json}
	 * { "tracks": ["Kyrandia2/Ferry", "Kyrandia2/Swamp"] }
	 * @endcode
	 * The playlist name is derived from the resource filename (e.g. `Kyrandia2.json` -> "Kyrandia2").
	 * Track entries must be valid MusicResource names resolvable via the Musics container.
	 */
	class PlaylistResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< PlaylistResource >;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PlaylistResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a playlist resource.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			PlaylistResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags = 0) noexcept
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
				return Libs::Hash::FNV1a(ClassId);
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
				size_t total = sizeof(*this);

				for ( const auto & trackName : m_trackNames )
				{
					total += trackName.capacity();
				}

				return total;
			}

			/**
			 * @brief Returns the ordered list of track resource names referenced by this playlist.
			 * @return const std::vector< std::string > &
			 */
			[[nodiscard]]
			const std::vector< std::string > &
			trackNames () const noexcept
			{
				return m_trackNames;
			}

			/**
			 * @brief Returns the number of tracks referenced by this playlist.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			trackCount () const noexcept
			{
				return m_trackNames.size();
			}

		private:

			/**
			 * @brief Parses the "tracks" array from a JSON manifest into m_trackNames.
			 * @param data The root JSON value (expects a "tracks" array of strings).
			 * @return True on successful extraction (including empty but valid arrays), false on missing/invalid schema.
			 */
			[[nodiscard]]
			bool parseTracks (const Json::Value & data) noexcept;

			std::vector< std::string > m_trackNames;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using Playlists = Container< Audio::PlaylistResource >;
}
