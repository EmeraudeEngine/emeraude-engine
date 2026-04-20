/*
 * src/Audio/PlaylistResource.cpp
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

#include "PlaylistResource.hpp"

/* STL inclusions. */
#include <fstream>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Audio
{
	bool
	PlaylistResource::load () noexcept
	{
		/* Neutral resource: empty playlist. Lets the track mixer gracefully ignore missing manifests. */
		if ( !this->beginLoading() )
		{
			return false;
		}

		m_trackNames.clear();

		return this->setLoadSuccess(true);
	}

	bool
	PlaylistResource::load (const std::filesystem::path & filepath) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		std::ifstream file{filepath};

		if ( !file.is_open() )
		{
			TraceError{ClassId} << "Unable to open playlist manifest '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		Json::Value root;
		Json::CharReaderBuilder readerBuilder;
		std::string errors;

		if ( !Json::parseFromStream(readerBuilder, file, &root, &errors) )
		{
			TraceError{ClassId} << "Failed to parse playlist manifest '" << filepath << "': " << errors;

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(this->parseTracks(root));
	}

	bool
	PlaylistResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		return this->setLoadSuccess(this->parseTracks(data));
	}

	bool
	PlaylistResource::parseTracks (const Json::Value & data) noexcept
	{
		if ( !data.isMember("tracks") || !data["tracks"].isArray() )
		{
			TraceError{ClassId} << "Playlist '" << this->name() << "' is missing required 'tracks' array !";

			return false;
		}

		m_trackNames.clear();
		m_trackNames.reserve(data["tracks"].size());

		for ( const auto & entry : data["tracks"] )
		{
			if ( !entry.isString() )
			{
				TraceWarning{ClassId} << "Playlist '" << this->name() << "' contains a non-string track entry. Skipped.";

				continue;
			}

			m_trackNames.emplace_back(entry.asString());
		}

		if ( m_trackNames.empty() )
		{
			TraceWarning{ClassId} << "Playlist '" << this->name() << "' contains no valid track entries.";
		}

		return true;
	}
}
