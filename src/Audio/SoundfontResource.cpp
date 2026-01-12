/*
 * src/Audio/SoundfontResource.cpp
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

#include "SoundfontResource.hpp"

/* STL inclusions. */
#include <fstream>

/* Third-party inclusions. */
/* TinySoundFont implementation (must be defined in exactly one .cpp file). */
#define TSF_IMPLEMENTATION
#include "tsf.h"

/* Local inclusions. */
#include "Resources/Manager.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio
{
	SoundfontResource::~SoundfontResource ()
	{
		if ( m_tsf != nullptr )
		{
			tsf_close(m_tsf);
			m_tsf = nullptr;
		}
	}

	bool
	SoundfontResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
	{
		/* Neutral resource: no soundfont loaded.
		 * MIDI rendering will fall back to additive synthesis. */
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* m_tsf remains nullptr - this is intentional for the fallback behavior. */
		return this->setLoadSuccess(true);
	}

	bool
	SoundfontResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const std::filesystem::path & filepath) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Read the entire file into memory.
		 * TSF needs the data to remain valid for the lifetime of the tsf handle. */
		std::ifstream file(filepath, std::ios::binary | std::ios::ate);

		if ( !file.is_open() )
		{
			TraceError{ClassId} << "Unable to open soundfont file '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		const auto fileSize = file.tellg();

		if ( fileSize <= 0 )
		{
			TraceError{ClassId} << "Soundfont file '" << filepath << "' is empty or unreadable !";

			return this->setLoadSuccess(false);
		}

		file.seekg(0, std::ios::beg);

		m_fileData.resize(static_cast< size_t >(fileSize));

		if ( !file.read(m_fileData.data(), fileSize) )
		{
			TraceError{ClassId} << "Failed to read soundfont file '" << filepath << "' !";

			m_fileData.clear();

			return this->setLoadSuccess(false);
		}

		/* Load the soundfont from memory. */
		m_tsf = tsf_load_memory(m_fileData.data(), static_cast< int >(m_fileData.size()));

		if ( m_tsf == nullptr )
		{
			TraceError{ClassId} << "Failed to parse soundfont file '" << filepath << "' ! Invalid SF2 format.";

			m_fileData.clear();

			return this->setLoadSuccess(false);
		}

		TraceInfo{ClassId} <<
			"Loaded soundfont '" << this->name() << "' with " << this->presetCount() << " presets "
			"(" << (m_fileData.size() / 1024) << " KB).";

		return this->setLoadSuccess(true);
	}

	bool
	SoundfontResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* JSON format expects a "file" key with the path to the SF2 file. */
		if ( !data.isMember("file") || !data["file"].isString() )
		{
			TraceError{ClassId} << "Soundfont JSON data missing 'file' key for resource '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		const std::filesystem::path filepath = data["file"].asString();

		/* Read the entire file into memory. */
		std::ifstream file(filepath, std::ios::binary | std::ios::ate);

		if ( !file.is_open() )
		{
			TraceError{ClassId} << "Unable to open soundfont file '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		const auto fileSize = file.tellg();

		if ( fileSize <= 0 )
		{
			TraceError{ClassId} << "Soundfont file '" << filepath << "' is empty or unreadable !";

			return this->setLoadSuccess(false);
		}

		file.seekg(0, std::ios::beg);

		m_fileData.resize(static_cast< size_t >(fileSize));

		if ( !file.read(m_fileData.data(), fileSize) )
		{
			TraceError{ClassId} << "Failed to read soundfont file '" << filepath << "' !";

			m_fileData.clear();

			return this->setLoadSuccess(false);
		}

		/* Load the soundfont from memory. */
		m_tsf = tsf_load_memory(m_fileData.data(), static_cast< int >(m_fileData.size()));

		if ( m_tsf == nullptr )
		{
			TraceError{ClassId} << "Failed to parse soundfont file '" << filepath << "' ! Invalid SF2 format.";

			m_fileData.clear();

			return this->setLoadSuccess(false);
		}

		TraceInfo{ClassId} <<
			"Loaded soundfont '" << this->name() << "' with " << this->presetCount() << " presets "
			"(" << (m_fileData.size() / 1024) << " KB).";

		return this->setLoadSuccess(true);
	}

	int
	SoundfontResource::presetCount () const noexcept
	{
		if ( m_tsf == nullptr )
		{
			return 0;
		}

		return tsf_get_presetcount(m_tsf);
	}

	std::string
	SoundfontResource::presetName (int presetIndex) const noexcept
	{
		if ( m_tsf == nullptr || presetIndex < 0 || presetIndex >= this->presetCount() )
		{
			return {};
		}

		const char * name = tsf_get_presetname(m_tsf, presetIndex);

		return name != nullptr ? std::string{name} : std::string{};
	}

	bool
	SoundfontResource::onDependenciesLoaded () noexcept
	{
		/* No additional processing needed after dependencies are loaded.
		 * The soundfont is already parsed and ready for use. */
		return true;
	}
}
