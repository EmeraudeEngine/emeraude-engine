/*
 * src/Audio/MusicResource.cpp
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

#include "MusicResource.hpp"

/* STL inclusions. */
#include <array>
#include <mutex>

/* Third-party inclusions. */
#include "taglib/tag.h"
#include "taglib/fileref.h"

/* Local inclusions. */
#include "Libs/WaveFactory/FileIO.hpp"
#include "Libs/WaveFactory/Synthesizer.hpp"
#include "Libs/WaveFactory/SFXScript.hpp"
#include "Libs/WaveFactory/Processor.hpp"
#include "Resources/Manager.hpp"
#include "Buffer.hpp"
#include "Manager.hpp"
#include "SoundfontResource.hpp"
#include "Settings.hpp"
#include "SettingKeys.hpp"
#include "Tracer.hpp"

namespace EmEn::Audio
{
	using namespace Libs;

	/* NOTE: TSF (TinySoundFont) is not thread-safe for concurrent rendering operations.
	 * This mutex protects SF2-based MIDI loading when multiple MusicResources load in parallel. */
	static std::mutex s_tsfMutex;

	bool
	MusicResource::onDependenciesLoaded () noexcept
	{
		/* If we have a pending MIDI file, render it now that the soundfont is loaded. */
		if ( !m_pendingMidiPath.empty() )
		{
			if ( !this->renderPendingMidi() )
			{
				return false;
			}
		}

		/* Create audio buffers from the loaded wave data. */
		const auto chunkSize = m_chunkSize;
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

		/* Clear temporary references now that loading is complete. */
		m_pendingMidiPath.clear();
		m_soundfontDependency.reset();

		return true;
	}

	bool
	MusicResource::renderPendingMidi () noexcept
	{
		if ( m_soundfontDependency == nullptr || !m_soundfontDependency->isValid() )
		{
			TraceError{ClassId} << "Soundfont dependency is not valid for MIDI rendering of '" << this->name() << "' !";

			return false;
		}

		/* Lock the mutex to ensure thread-safe access to the TSF handle.
		 * TSF maintains internal state that cannot be safely accessed concurrently. */
		const std::lock_guard< std::mutex > tsfLock{s_tsfMutex};

		if ( !WaveFactory::FileIO::read(m_pendingMidiPath, m_localData, m_frequency, m_soundfontDependency->handle()) )
		{
			TraceError{ClassId} << "Unable to render MIDI file '" << m_pendingMidiPath << "' !";

			return false;
		}

		return true;
	}


	void
	MusicResource::readMetaData (const std::filesystem::path & filepath) noexcept
	{
		const TagLib::FileRef file{filepath.c_str()};

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
	MusicResource::load (Resources::AbstractServiceProvider & serviceProvider) noexcept
	{
		if ( !serviceProvider.audioManager().isAudioSystemAvailable() )
		{
			return true;
		}

		m_frequency = serviceProvider.audioManager().frequencyPlayback();
		m_chunkSize = serviceProvider.audioManager().musicChunkSize();

		if ( !this->beginLoading() )
		{
			return false;
		}

		const auto sampleRate = static_cast< size_t >(m_frequency);

		/* Music box arrangement of "Crocodile Tears", composed by Frank Klepacki
		 * for the video game "The Legend of Kyrandia: Hand of Fate" (1993).
		 * Notes extracted directly from the original MIDI (3 channels, 84 BPM).
		 * Looped 5 times (~71 seconds) with music box timbre. */
		constexpr float BPM = 72.0F;
		constexpr float SecondsPerBeat = 60.0F / BPM;
		const auto samplesPerBeat = static_cast< size_t >(SecondsPerBeat * static_cast< float >(sampleRate));

		/* The original piece spans ~19.05 beats. Tight loop with minimal
		 * gap so the next repetition starts almost immediately. */
		constexpr float LoopBeats = 19.10F;
		constexpr size_t Repetitions = 5;
		const auto loopSamples = static_cast< size_t >(LoopBeats * static_cast< float >(samplesPerBeat));
		const auto totalSamples = loopSamples * Repetitions;

		/* Note event: exact position and duration in beats, with frequency. */
		struct NoteEvent
		{
			float startBeat;
			float durationBeats;
			float frequency;
		};

		/* Main melody (channel 2 from the original MIDI). */
		constexpr std::array< NoteEvent, 47 > melodyNotes = {{
			{ 0.00F, 0.58F, 523.25F}, /* C5   */
			{ 0.58F, 0.31F, 783.99F}, /* G5   */
			{ 0.89F, 0.29F, 739.99F}, /* F#5  */
			{ 1.18F, 0.60F, 783.99F}, /* G5   */
			{ 1.78F, 0.60F, 880.00F}, /* A5   */
			{ 2.38F, 0.60F, 880.00F}, /* A5   */
			{ 2.97F, 0.29F, 698.46F}, /* F5   */
			{ 3.26F, 0.31F, 659.26F}, /* E5   */
			{ 3.57F, 1.18F, 698.46F}, /* F5   */
			{ 4.75F, 0.60F, 493.88F}, /* B4   */
			{ 5.35F, 0.31F, 783.99F}, /* G5   */
			{ 5.65F, 0.29F, 698.46F}, /* F5   */
			{ 5.94F, 0.60F, 783.99F}, /* G5   */
			{ 6.54F, 0.60F, 698.46F}, /* F5   */
			{ 7.14F, 0.60F, 659.26F}, /* E5   */
			{ 7.74F, 0.29F, 392.00F}, /* G4   */
			{ 8.03F, 0.29F, 659.26F}, /* E5   */
			{ 8.32F, 0.31F, 587.33F}, /* D5   */
			{ 8.62F, 0.29F, 493.88F}, /* B4   */
			{ 8.92F, 0.31F, 329.63F}, /* E4   */
			{ 9.22F, 0.29F, 493.88F}, /* B4   */
			{ 9.51F, 0.60F, 523.25F}, /* C5   */
			{10.11F, 0.29F, 440.00F}, /* A4   */
			{10.40F, 0.31F, 392.00F}, /* G4   */
			{10.71F, 0.60F, 440.00F}, /* A4   */
			{11.31F, 0.60F, 523.25F}, /* C5   */
			{11.90F, 0.58F, 493.88F}, /* B4   */
			{12.49F, 0.31F, 392.00F}, /* G4   */
			{12.79F, 0.29F, 329.63F}, /* E4   */
			{13.08F, 0.60F, 392.00F}, /* G4   */
			{13.68F, 0.60F, 493.88F}, /* B4   */
			{14.28F, 0.29F, 440.00F}, /* A4   */
			{14.57F, 0.31F, 493.88F}, /* B4   */
			{14.88F, 0.29F, 523.25F}, /* C5   */
			{15.17F, 0.31F, 587.33F}, /* D5   */
			{15.47F, 0.29F, 659.26F}, /* E5   */
			{15.76F, 0.31F, 587.33F}, /* D5   */
			{16.07F, 0.29F, 523.25F}, /* C5   */
			{16.36F, 0.29F, 493.88F}, /* B4   */
			{16.65F, 0.31F, 392.00F}, /* G4   */
			{16.96F, 0.29F, 493.88F}, /* B4   */
			{17.25F, 0.31F, 587.33F}, /* D5   */
			{17.56F, 0.29F, 698.46F}, /* F5   */
			{17.85F, 0.31F, 783.99F}, /* G5   */
			{18.15F, 0.29F, 698.46F}, /* F5   */
			{18.44F, 0.29F, 587.33F}, /* D5   */
			{18.74F, 0.31F, 493.88F}  /* B4   */
		}};

		/* Single mono synth piano sound, duplicated to stereo. */
		WaveFactory::Wave< int16_t > monoWave;
		WaveFactory::Synthesizer synth{monoWave, totalSamples, m_frequency};

		/* Convert beat position to sample index. */
		const auto beatToSample = [&samplesPerBeat] (float beat) -> size_t
		{
			return static_cast< size_t >(beat * static_cast< float >(samplesPerBeat));
		};

		/* Transpose one octave down for a warmer, deeper tone. */
		constexpr float Transpose = 0.5F;

		/* Place each melody note: single sine wave, transposed down. */
		for ( size_t rep = 0; rep < Repetitions; ++rep )
		{
			const auto loopOffset = rep * loopSamples;

			for ( const auto & note : melodyNotes )
			{
				const auto start = loopOffset + beatToSample(note.startBeat);
				auto length = beatToSample(note.durationBeats);

				if ( start >= totalSamples )
				{
					break;
				}

				length = std::min(length, totalSamples - start);

				synth.setRegion(start, length);
				synth.sineWave(note.frequency * Transpose, 0.55F);
				synth.applyADSR(0.01F, 0.03F, 0.60F, 0.01F);
				synth.applyFadeIn(0.005F);
				synth.applyFadeOut(0.005F);
			}
		}

		/* Smooth loop boundaries: short fade-out at the end of each repetition
		 * and fade-in at the start of the next to eliminate clicks. */
		constexpr float FadeTime = 0.03F; /* 30ms crossfade */
		const auto fadeSamples = static_cast< size_t >(FadeTime * static_cast< float >(sampleRate));

		for ( size_t rep = 0; rep < Repetitions; ++rep )
		{
			const auto loopOffset = rep * loopSamples;

			/* Fade-in at the start of each repetition (except the very first). */
			if ( rep > 0 )
			{
				synth.setRegion(loopOffset, std::min(fadeSamples, totalSamples - loopOffset));
				synth.applyFadeIn(FadeTime);
			}

			/* Fade-out at the end of each repetition (except the very last). */
			if ( rep < Repetitions - 1 )
			{
				const auto fadeStart = loopOffset + loopSamples - fadeSamples;

				if ( fadeStart < totalSamples )
				{
					synth.setRegion(fadeStart, std::min(fadeSamples, totalSamples - fadeStart));
					synth.applyFadeOut(FadeTime);
				}
			}
		}

		/* Apply global effects. */
		synth.resetRegion();

		/* Chorus for slight width. */
		synth.applyChorus(0.35F, 4.5F, 0.22F);

		/* Warm reverb for depth. */
		synth.applyReverb(0.55F, 0.35F, 0.30F);

		/* Final normalization. */
		synth.normalize();

		/* Duplicate mono to stereo. */
		m_localData.initialize(totalSamples, WaveFactory::Channels::Stereo, m_frequency);
		auto & stereoData = m_localData.data();
		const auto & monoData = monoWave.data();

		for ( size_t sampleIndex = 0; sampleIndex < totalSamples; ++sampleIndex )
		{
			stereoData[sampleIndex * 2] = monoData[sampleIndex];
			stereoData[sampleIndex * 2 + 1] = monoData[sampleIndex];
		}

		return this->setLoadSuccess(true);
	}

	bool
	MusicResource::load (Resources::AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept
	{
		if ( !serviceProvider.audioManager().isAudioSystemAvailable() )
		{
			return true;
		}

		m_frequency = serviceProvider.audioManager().frequencyPlayback();
		m_chunkSize = serviceProvider.audioManager().musicChunkSize();

		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Check if this is a MIDI file that might need a soundfont. */
		const auto extension = filepath.extension().string();

		if ( extension == ".mid" || extension == ".midi" )
		{
			/* Query the soundfont name from settings. */
			const auto soundfontName = serviceProvider.settings().getOrSetDefault< std::string >(AudioMusicSoundfontKey, DefaultAudioMusicSoundfont);

			if ( !soundfontName.empty() )
			{
				/* Load the soundfont as a dependency. */
				auto * soundfontContainer = serviceProvider.container< SoundfontResource >();

				if ( soundfontContainer != nullptr )
				{
					m_soundfontDependency = soundfontContainer->getResource(soundfontName, true);

					if ( m_soundfontDependency != nullptr )
					{
						/* Store the MIDI path for later rendering in onDependenciesLoaded(). */
						m_pendingMidiPath = filepath;

						/* Add the soundfont as a dependency - rendering will happen once it's loaded. */
						if ( !this->addDependency(m_soundfontDependency) )
						{
							TraceWarning{ClassId} <<
								"Failed to add soundfont '" << soundfontName << "' as dependency for music '" << this->name() << "'. "
								"Falling back to additive synthesis.";

							m_soundfontDependency.reset();
							m_pendingMidiPath.clear();
						}
						else
						{
							/* Dependency added successfully. Loading will complete in onDependenciesLoaded(). */
							return this->setLoadSuccess(true);
						}
					}
					else
					{
						TraceWarning{ClassId} <<
							"Soundfont '" << soundfontName << "' not found for music '" << this->name() << "'. "
							"Using additive synthesis fallback.";
					}
				}
			}

			/* No soundfont configured or available - use additive synthesis. */
			if ( !WaveFactory::FileIO::read(filepath, m_localData) )
			{
				TraceError{ClassId} << "Unable to load the MIDI file '" << filepath << "' with additive synthesis !";

				return this->setLoadSuccess(false);
			}
		}
		else
		{
			/* Standard audio file loading (WAV, MP3, OGG, etc.). */
			if ( !WaveFactory::FileIO::read(filepath, m_localData) )
			{
				TraceError{ClassId} << "Unable to load the music file '" << filepath << "' !";

				return this->setLoadSuccess(false);
			}

			/* Read optional metadata from the soundtrack. */
			this->readMetaData(filepath);
		}

		/* Check frequency for playback within the audio engine. */
		if ( m_localData.frequency() != m_frequency )
		{
			TraceWarning{ClassId} <<
				"Music '" << this->name() << "' frequency mismatch the system ! "
				"Resampling the wave from " << static_cast< int >(m_localData.frequency()) << "Hz to " << static_cast< int >(m_frequency) << "Hz ...";

			/* Copy the buffer in float (single precision) format. */
			WaveFactory::Processor processor{m_localData};

			/* Launch a resampling process ... */
			if ( !processor.resample(m_frequency) )
			{
				TraceError{ClassId} << "Unable to resample the wave to " << static_cast< int >(m_frequency) << "Hz !";

				return this->setLoadSuccess(false);
			}

			/* Gets back the buffer in 16 bits integer format. */
			if ( !processor.toWave(m_localData) )
			{
				Tracer::error(ClassId, "Unable to copy the fixed wave format !");

				return this->setLoadSuccess(false);
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	MusicResource::load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept
	{
		if ( !serviceProvider.audioManager().isAudioSystemAvailable() )
		{
			return true;
		}

		m_frequency = serviceProvider.audioManager().frequencyPlayback();
		m_chunkSize = serviceProvider.audioManager().musicChunkSize();

		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Use SFXScript to generate audio from JSON data. */
		WaveFactory::SFXScript script{m_localData, m_frequency};

		if ( !script.generateFromData(data) )
		{
			TraceError{ClassId} << "Failed to generate music '" << this->name() << "' from JSON data !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}
}
