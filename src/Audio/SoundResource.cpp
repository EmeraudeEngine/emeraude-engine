/*
 * src/Audio/SoundResource.cpp
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

#include "SoundResource.hpp"

/* Local inclusions. */
#include "Libs/WaveFactory/Processor.hpp"
#include "Resources/Manager.hpp"
#include "Manager.hpp"
#include "Tracer.hpp"

/* Defining the resource manager ClassId. */
template<>
const char * const EmEn::Resources::Container< EmEn::Audio::SoundResource >::ClassId{"SoundContainer"};

/* Defining the resource manager ClassUID. */
template<>
const size_t EmEn::Resources::Container< EmEn::Audio::SoundResource >::ClassUID{getClassUID(ClassId)};

namespace EmEn::Audio
{
	using namespace EmEn::Libs;

	const size_t SoundResource::ClassUID{getClassUID(ClassId)};

	SoundResource::SoundResource (const std::string & name, uint32_t resourceFlags) noexcept
		: ResourceTrait(name, resourceFlags)
	{

	}

	bool
	SoundResource::load () noexcept
	{
		if ( Manager::audioDisabled() )
		{
			return true;
		}

		if ( !this->beginLoading() )
		{
			return false;
		}

		const auto frequencyPlayback = Manager::instance()->frequencyPlayback();
		const auto oneSecond = 1U * static_cast< size_t >(frequencyPlayback);

		if ( !m_localData.initialize(oneSecond, WaveFactory::Channels::Mono, frequencyPlayback) )
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
	SoundResource::load (const std::filesystem::path & filepath) noexcept
	{
		if ( Manager::audioDisabled() )
		{
			return true;
		}

		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !m_localData.readFile(filepath) )
		{
			TraceError{ClassId} << "Unable to load the sound file '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		const auto frequencyPlayback = Manager::instance()->frequencyPlayback();

		/* Checks if the sound is valid for the engine.
		 * It must be mono and meet the audio engine frequency. */
		if ( m_localData.channels() != WaveFactory::Channels::Mono || m_localData.frequency() != frequencyPlayback )
		{
			/* Copy the buffer in float (single precision) format. */
			WaveFactory::Processor processor{m_localData};

			/* Launch a mix down process... */
			if ( m_localData.channels() != WaveFactory::Channels::Mono )
			{
				if ( !s_quietConversion )
				{
					TraceWarning{ClassId} << "The sound '" << this->name() << "' is multichannel ! Performing a mix down ...";
				}

				if ( !processor.mixDown() )
				{
					Tracer::error(ClassId, "Mix down failed !");

					return this->setLoadSuccess(false);
				}
			}

			/* Launch a resampling process ... */
			if ( m_localData.frequency() != frequencyPlayback )
			{
				if ( !s_quietConversion )
				{
					TraceWarning{ClassId} <<
					   "Sound '" << this->name() << "' frequency mismatch the system ! "
					   "Resampling the wave from " << static_cast< int >(m_localData.frequency()) << "Hz to " << static_cast< int >(frequencyPlayback) << "Hz ...";
				}

				if ( !processor.resample(frequencyPlayback) )
				{
					TraceError{ClassId} << "Unable to resample the wave to " << static_cast< int >(frequencyPlayback) << "Hz !";

					return this->setLoadSuccess(false);
				}
			}

			/* Gets back the buffer in 16bits integer format. */
			if ( !processor.toWave(m_localData) )
			{
				Tracer::error(ClassId, "Unable to copy the fixed wave format !");

				return this->setLoadSuccess(false);
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	SoundResource::load (const Json::Value & /*data*/) noexcept
	{
		if ( Manager::audioDisabled() )
		{
			return true;
		}

		if ( !this->beginLoading() )
		{
			return false;
		}

		/* TODO: Empty function. */

		return this->setLoadSuccess(false);
	}

	bool
	SoundResource::onDependenciesLoaded () noexcept
	{
		m_buffer = std::make_shared< Buffer >();

		if ( !m_buffer->isCreated() )
		{
			Tracer::error(ClassId, "Unable to create a buffer in audio memory !");

			return false;
		}

		if ( !m_buffer->feedData(m_localData, 0, 0) )
		{
			Tracer::error(ClassId, "Unable to load local data in audio buffer !");

			return false;
		}

		return true;
	}
}
