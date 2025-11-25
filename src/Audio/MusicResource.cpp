/*
 * src/Audio/MusicResource.cpp
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

#include "MusicResource.hpp"

/* Third-party inclusions. */
#include "taglib/tag.h"
#include "taglib/fileref.h"

/* Local inclusions. */
#include "Libs/WaveFactory/Processor.hpp"
#include "Resources/Manager.hpp"
#include "Buffer.hpp"
#include "Manager.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio
{
	using namespace Libs;

	bool
	MusicResource::onDependenciesLoaded () noexcept
	{
		const auto chunkSize = Manager::musicChunkSize();
		const auto chunkCount = m_localData.chunkCount(chunkSize);

		m_buffers.resize(chunkCount);

		for ( size_t chunkIndex = 0; chunkIndex < chunkCount; chunkIndex++ )
		{
			m_buffers[chunkIndex] = std::make_shared< Buffer >();

			if ( !m_buffers[chunkIndex]->isCreated() || !m_buffers[chunkIndex]->feedData(m_localData, chunkIndex, chunkSize) )
			{
				Tracer::error(ClassId, "Unable to load buffer in audio memory !");

				return false;
			}
		}

		return true;
	}


	void
	MusicResource::readMetaData (const std::filesystem::path & filepath) noexcept
	{
		const TagLib::FileRef file(filepath.c_str());

		if ( file.isNull() )
		{
			TraceError{ClassId} << "Unable to read file '" << filepath << "' for audio tag extraction.";

			return;
		}

		const auto * tag = file.tag();

		if ( tag == nullptr )
		{
			TraceWarning{ClassId} << "Unable to read audio metadata from '" << filepath << "' !";

			return;
		}

		m_title = tag->title().to8Bit(true);
		m_artist = tag->artist().to8Bit(true);
	}

	bool
	MusicResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
	{
		if ( !Manager::isAudioSystemAvailable() )
		{
			return true;
		}

		if ( !this->beginLoading() )
		{
			return false;
		}

		const auto frequencyPlayback = Manager::frequencyPlayback();
		const auto tenSeconds = 10 * static_cast< size_t >(frequencyPlayback);

		if ( !m_localData.initialize(tenSeconds, WaveFactory::Channels::Stereo, frequencyPlayback) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !m_localData.generateNoise() )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	MusicResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const std::filesystem::path & filepath) noexcept
	{
		if ( !Manager::isAudioSystemAvailable() )
		{
			return true;
		}

		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !m_localData.readFile(filepath) )
		{
			TraceError{ClassId} << "Unable to load the music file '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		const auto frequencyPlayback = Manager::frequencyPlayback();

		/* Checks frequency for playback within the audio engine. */
		if ( m_localData.frequency() != frequencyPlayback )
		{
			TraceWarning{ClassId} <<
				"Music '" << this->name() << "' frequency mismatch the system ! "
				"Resampling the wave from " << static_cast< int >(m_localData.frequency()) << "Hz to " << static_cast< int >(frequencyPlayback) << "Hz ...";

			/* Copy the buffer in float (single precision) format. */
			WaveFactory::Processor processor(m_localData);

			/* Launch a mix-down process ... */
			/* FIXME: If music is not stereo, so mono or 5.1 for instance set it to a stereo wave format. */
			/*if ( m_localData.channels() != WaveFactory::Channels::Stereo )
			{
				Tracer::info(ClassId, Blob() << "The sound '" << this->name() << "' is multichannel ! Performing a mix down ...");

				if ( !processor.mixDown() )
				{
					Tracer::error(ClassId, "Mix down failed !");

					return this->setLoadSuccess(false);
				}
			}*/

			/* Launch a resampling process ... */
			if ( !processor.resample(frequencyPlayback) )
			{
				TraceError{ClassId} << "Unable to resample the wave to " << static_cast< int >(frequencyPlayback) << "Hz !";

				return this->setLoadSuccess(false);
			}

			/* Gets back the buffer in 16 bits integer format. */
			if ( !processor.toWave(m_localData) )
			{
				Tracer::error(ClassId, "Unable to copy the fixed wave format !");

				return this->setLoadSuccess(false);
			}
		}

		/* Read optional metadata from the soundtrack if available. */
		this->readMetaData(filepath);

		return this->setLoadSuccess(true);
	}

	bool
	MusicResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		if ( !Manager::isAudioSystemAvailable() )
		{
			return true;
		}

		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "FIXME: This function is not available yet !");

		return this->setLoadSuccess(false);
	}
}
