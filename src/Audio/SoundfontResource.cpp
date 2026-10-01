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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "SoundfontResource.hpp"

/* STL inclusions. */
#include <cstdint>
#include <fstream>
#include <limits>

/* Third-party inclusions. */
/* TinySoundFont implementation (must be defined in exactly one .cpp file). */
#define TSF_IMPLEMENTATION
#include "tsf.h"

/* Local inclusions. */
#include "FastJSON.hpp"
#include "FileSystem.hpp"
#include "PrimaryServices.hpp"
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
	SoundfontResource::load () noexcept
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
	SoundfontResource::load (const std::filesystem::path & filepath) noexcept
	{
		/* NOTE: A JSON definition goes through ResourceTrait::load(), which parses it and calls load(json) — as
		 * MeshResource does for a non-glTF file; without this, a .json in the store was read as an SF2 and failed. */
		if ( filepath.extension() == ".json" )
		{
			return ResourceTrait::load(filepath);
		}

		if ( !this->beginLoading() )
		{
			return false;
		}

		return this->setLoadSuccess(this->readSoundfont(filepath));
	}

	bool
	SoundfontResource::load (const Json::Value & data) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* JSON format expects a "file" key with the path to the SF2 file. */
		const auto fileKey = Base::FastJSON::getValue< std::string >(data, "file");

		if ( !fileKey.has_value() )
		{
			TraceError{ClassId} << "Soundfont JSON data missing 'file' key for resource '" << this->name() << "' !";

			return this->setLoadSuccess(false);
		}

		/* NOTE: Owner ruling (2026-10-01): the file is a DATA path, resolved like every other one — inside the SoundBanks
		 * store of the data directories, confined (an absolute or escaping path is refused, traced by FileSystem). It
		 * used to be opened raw, relative to the process's working directory. */
		const auto filepath = this->serviceProvider().primaryServices().fileSystem().getFilepathFromDataDirectories(SoundBanksStore, *fileKey);

		if ( filepath.empty() )
		{
			TraceError{ClassId} << "The soundfont file '" << *fileKey << "' of resource '" << this->name() << "' is not in the '" << SoundBanksStore << "' store of any data directory !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(this->readSoundfont(filepath));
	}

	bool
	SoundfontResource::readSoundfont (const std::filesystem::path & filepath) noexcept
	{
		/* Read the entire file into memory.
		 * TSF needs the data to remain valid for the lifetime of the tsf handle. */
		std::ifstream file(filepath, std::ios::binary | std::ios::ate);

		if ( !file.is_open() )
		{
			TraceError{ClassId} << "Unable to open soundfont file '" << filepath << "' !";

			return false;
		}

		const auto fileSize = file.tellg();

		if ( fileSize <= 0 )
		{
			TraceError{ClassId} << "Soundfont file '" << filepath << "' is empty or unreadable !";

			return false;
		}

		/* NOTE: TinySoundFont takes the size as an int: a larger file cannot be handed to it. */
		if ( static_cast< uint64_t >(fileSize) > static_cast< uint64_t >(std::numeric_limits< int >::max()) )
		{
			TraceError{ClassId} << "Soundfont file '" << filepath << "' is " << static_cast< uint64_t >(fileSize) << " bytes, larger than the parser accepts (" << std::numeric_limits< int >::max() << ") !";

			return false;
		}

		file.seekg(0, std::ios::beg);

		m_fileData.resize(static_cast< size_t >(fileSize));

		if ( !file.read(m_fileData.data(), fileSize) )
		{
			TraceError{ClassId} << "Failed to read soundfont file '" << filepath << "' !";

			m_fileData.clear();

			return false;
		}

		/* Load the soundfont from memory. */
		m_tsf = tsf_load_memory(m_fileData.data(), static_cast< int >(m_fileData.size()));

		if ( m_tsf == nullptr )
		{
			TraceError{ClassId} << "Failed to parse soundfont file '" << filepath << "' ! Invalid SF2 format.";

			m_fileData.clear();

			return false;
		}

		TraceDebug{ClassId} << "Loaded soundfont '" << this->name() << "' with " << this->presetCount() << " presets (" << (m_fileData.size() / 1024) << " KB).";

		return true;
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
