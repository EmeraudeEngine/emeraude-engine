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
#include "Libs/WaveFactory/FileIO.hpp"
#include "Libs/WaveFactory/Processor.hpp"
#include "Libs/WaveFactory/Synthesizer.hpp"
#include "Libs/WaveFactory/SFXScript.hpp"
#include "Resources/Manager.hpp"
#include "Manager.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio
{
	using namespace Libs;

	bool
	SoundResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
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
		const auto sampleRate = static_cast< size_t >(frequencyPlayback);

		/* Default/fallback sound: A retro "alert" double-beep.
		 * Two short beeps with a pitch sweep, easily recognizable as a placeholder. */
		const auto beepDuration = sampleRate / 10;  /* 100ms per beep */
		const auto silenceDuration = sampleRate / 20;  /* 50ms silence between beeps */
		const auto totalDuration = beepDuration * 2 + silenceDuration;

		WaveFactory::Synthesizer synth{m_localData, totalDuration, frequencyPlayback};

		/* First beep: descending pitch sweep (880Hz -> 440Hz). */
		synth.setRegion(0, beepDuration);

		if ( !synth.pitchSweep(880.0F, 440.0F, 0.6F) )
		{
			return this->setLoadSuccess(false);
		}

		/* Apply a punchy envelope. */
		if ( !synth.applyADSR(0.01F, 0.02F, 0.7F, 0.05F) )
		{
			return this->setLoadSuccess(false);
		}

		/* Add a bit of bit-crush for that retro feel. */
		if ( !synth.applyBitCrush(12) )
		{
			return this->setLoadSuccess(false);
		}

		/* Second beep: ascending pitch sweep (440Hz -> 660Hz). */
		synth.setRegion(beepDuration + silenceDuration, beepDuration);

		if ( !synth.pitchSweep(440.0F, 660.0F, 0.6F) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !synth.applyADSR(0.01F, 0.02F, 0.7F, 0.05F) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !synth.applyBitCrush(12) )
		{
			return this->setLoadSuccess(false);
		}

		/* Final normalization to ensure good volume. */
		synth.resetRegion();

		if ( !synth.normalize() )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	SoundResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const std::filesystem::path & filepath) noexcept
	{
		if ( !Manager::isAudioSystemAvailable() )
		{
			return true;
		}

		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !WaveFactory::FileIO::read(filepath, m_localData) )
		{
			TraceError{ClassId} << "Unable to load the sound file '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		const auto frequencyPlayback = Manager::frequencyPlayback();

		/* Checks if the sound is valid for the engine.
		 * It must be mono and meet the audio engine frequency. */
		if ( m_localData.channels() != WaveFactory::Channels::Mono || m_localData.frequency() != frequencyPlayback )
		{
			/* Copy the buffer in float (single precision) format. */
			WaveFactory::Processor processor{m_localData};

			/* Launch a mix-down process... */
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
	SoundResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & data) noexcept
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

		/* Use SFXScript to generate audio from JSON data. */
		WaveFactory::SFXScript script{m_localData, frequencyPlayback};

		if ( !script.generateFromData(data) )
		{
			TraceError{ClassId} << "Failed to generate sound '" << this->name() << "' from JSON data !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
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
