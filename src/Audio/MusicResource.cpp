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

/* STL inclusions. */
#include <array>

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
		const auto sampleRate = static_cast< size_t >(frequencyPlayback);

		/* Create a seamless looping placeholder melody (~42 seconds = 64 measures at 90 BPM).
		 * Structure: A - A' - B - A - C - A' - B' - A (turnaround)
		 * Each section is 8 measures (4 chords x 2 repetitions).
		 * The final A section leads back smoothly to the beginning. */
		constexpr float BeatsPerMinute = 90.0F;
		constexpr float SecondsPerBeat = 60.0F / BeatsPerMinute;
		constexpr size_t BeatsPerMeasure = 4;
		constexpr size_t MeasuresPerSection = 8;
		constexpr size_t TotalSections = 8;
		constexpr size_t TotalMeasures = MeasuresPerSection * TotalSections;  /* 64 measures */
		const auto samplesPerBeat = static_cast< size_t >(SecondsPerBeat * static_cast< float >(sampleRate));
		const auto samplesPerMeasure = samplesPerBeat * BeatsPerMeasure;
		const auto totalSamples = samplesPerMeasure * TotalMeasures;

		/* Note frequencies (extended A minor / C major scale). */
		constexpr float E2 = 82.41F;
		constexpr float G2 = 98.00F;
		constexpr float A2 = 110.00F;
		constexpr float B2 = 123.47F;
		constexpr float C3 = 130.81F;
		constexpr float D3 = 146.83F;
		constexpr float E3 = 164.81F;
		constexpr float F3 = 174.61F;
		constexpr float G3 = 196.00F;
		constexpr float A3 = 220.00F;
		constexpr float B3 = 246.94F;
		constexpr float C4 = 261.63F;
		constexpr float D4 = 293.66F;
		constexpr float E4 = 329.63F;
		constexpr float F4 = 349.23F;
		constexpr float G4 = 392.00F;
		constexpr float GSharp4 = 415.30F;
		constexpr float A4 = 440.00F;
		constexpr float B4 = 493.88F;
		constexpr float C5 = 523.25F;
		constexpr float D5 = 587.33F;
		constexpr float E5 = 659.25F;
		constexpr float F5 = 698.46F;
		constexpr float G5 = 783.99F;

		/* Chord definitions (bass, root, third, fifth, optional seventh). */
		struct Chord
		{
			float bass;
			float root;
			float third;
			float fifth;
		};

		/* Section A chords: Am - F - C - G (classic pop progression in Am). */
		constexpr std::array< Chord, 4 > chordsA = {{
			{A2, A3, C4, E4},      /* Am */
			{F3, F3, A3, C4},      /* F */
			{C3, C4, E4, G4},      /* C */
			{G2, G3, B3, D4}       /* G */
		}};

		/* Section A' chords: Am - F - C - E (variation with E for tension). */
		constexpr std::array< Chord, 4 > chordsAv = {{
			{A2, A3, C4, E4},           /* Am */
			{F3, F3, A3, C4},           /* F */
			{C3, C4, E4, G4},           /* C */
			{E2, E3, GSharp4 / 2.0F, B3} /* E (dominant) */
		}};

		/* Section B chords: Dm - G - C - Am (ii-V-I-vi). */
		constexpr std::array< Chord, 4 > chordsB = {{
			{D3, D4, F4, A4},      /* Dm */
			{G2, G3, B3, D4},      /* G */
			{C3, C4, E4, G4},      /* C */
			{A2, A3, C4, E4}       /* Am */
		}};

		/* Section B' chords: Dm - E - Am - Am (turnaround). */
		constexpr std::array< Chord, 4 > chordsBv = {{
			{D3, D4, F4, A4},           /* Dm */
			{E2, E3, GSharp4 / 2.0F, B3}, /* E */
			{A2, A3, C4, E4},           /* Am */
			{A2, A3, C4, E4}            /* Am (sustain for loop) */
		}};

		/* Section C chords: F - G - Am - Em (bridge, new color). */
		constexpr std::array< Chord, 4 > chordsC = {{
			{F3, F3, A3, C4},      /* F */
			{G2, G3, B3, D4},      /* G */
			{A2, A3, C4, E4},      /* Am */
			{E2, E3, G3, B3}       /* Em */
		}};

		/* Melody patterns - varied for each section. */
		constexpr std::array< std::array< float, 4 >, 4 > melodyA = {{
			{A4, C5, E5, C5},      /* Ascending arpeggio */
			{F4, A4, C5, A4},      /* F arpeggio */
			{G4, C5, E5, G5},      /* C with reach to G5 */
			{G4, B4, D5, B4}       /* G arpeggio */
		}};

		constexpr std::array< std::array< float, 4 >, 4 > melodyAv = {{
			{E5, C5, A4, C5},      /* Descending start */
			{C5, A4, F4, A4},      /* Answer phrase */
			{E5, G5, E5, C5},      /* High point */
			{B4, GSharp4, E4, GSharp4}  /* Tension on E chord */
		}};

		constexpr std::array< std::array< float, 4 >, 4 > melodyB = {{
			{D5, F5, A4, F5},      /* Dm - higher energy */
			{D5, B4, G4, B4},      /* G - descending */
			{C5, E5, G5, E5},      /* C - bright */
			{A4, C5, E5, A4}       /* Am - resolution */
		}};

		constexpr std::array< std::array< float, 4 >, 4 > melodyBv = {{
			{F5, D5, A4, D5},      /* Dm - variation */
			{E5, GSharp4, B4, E5}, /* E - tension */
			{A4, C5, E5, C5},      /* Am - soft landing */
			{A4, E4, A4, E4}       /* Am - simple for loop point */
		}};

		constexpr std::array< std::array< float, 4 >, 4 > melodyC = {{
			{A4, C5, F5, C5},      /* F - lyrical */
			{B4, D5, G5, D5},      /* G - ascending */
			{C5, E5, A4, E5},      /* Am - floating */
			{B4, E5, G4, E5}       /* Em - ethereal */
		}};

		/* Bass patterns for more movement. */
		constexpr std::array< std::array< float, 4 >, 4 > bassPatternA = {{
			{A2, A2, E3, A2},      /* Root-root-fifth-root */
			{F3, F3, C3, F3},
			{C3, C3, G3, C3},
			{G2, G2, D3, G2}
		}};

		/* Initialize two mono wave buffers for stereo separation.
		 * Left channel: Bass + Chords (harmonic foundation)
		 * Right channel: Melody + Counter-melody (lead voices) */
		WaveFactory::Wave< int16_t > leftWave;
		WaveFactory::Wave< int16_t > rightWave;
		WaveFactory::Synthesizer synthLeft{leftWave, totalSamples, frequencyPlayback};
		WaveFactory::Synthesizer synthRight{rightWave, totalSamples, frequencyPlayback};

		/* Rhythm pattern types for variation. */
		enum class RhythmStyle
		{
			Straight,       /* Quarter notes */
			Syncopated,     /* Off-beat accents */
			Arpeggiated,    /* Broken chord */
			Sparse          /* Fewer notes, more space */
		};

		/* Texture types. */
		enum class TextureStyle
		{
			Pad,            /* Sustained chords */
			Plucked,        /* Short attacks */
			Layered,        /* Rich harmonics */
			Minimal         /* Simple, clean */
		};

		/* Half-beat duration for eighth notes. */
		const auto samplesPerEighth = samplesPerBeat / 2;

		/* Lambda to add a note to a specific channel. */
		auto addNoteToChannel = [&frequencyPlayback](WaveFactory::Synthesizer< int16_t > & synth,
		                                              size_t sample, size_t length, float freq, float amp,
		                                              float attack, float decay, float sustain, float release,
		                                              bool useTriangle = false)
		{
			WaveFactory::Wave< int16_t > tempWave;
			WaveFactory::Synthesizer tempSynth{tempWave, length, frequencyPlayback};

			if ( useTriangle )
			{
				tempSynth.triangleWave(freq, amp);
			}
			else
			{
				tempSynth.sineWave(freq, amp);
			}

			tempSynth.applyADSR(attack, decay, sustain, release);
			synth.mix(tempWave, 0.6F);
		};

		/* Lambda to generate a measure with rhythm and texture variations.
		 * Left channel: Bass + Chords, Right channel: Melody + Counter-melody */
		auto generateMeasure = [&](size_t & sample, const Chord & chord,
		                           const std::array< float, 4 > & melodyLine,
		                           const std::array< float, 4 > & bassLine,
		                           float intensity, RhythmStyle rhythm, TextureStyle texture,
		                           size_t measureInSection, bool addCounterMelody)
		{
			/* Vary intensity slightly within measure for more life. */
			const std::array< float, 4 > beatAccents = {{1.0F, 0.7F, 0.85F, 0.75F}};

			for ( size_t beat = 0; beat < BeatsPerMeasure && sample < totalSamples; ++beat )
			{
				const auto beatIntensity = intensity * beatAccents[beat];
				const auto melodyNote = melodyLine[beat];
				const auto bassNote = bassLine[beat];

				switch ( rhythm )
				{
					case RhythmStyle::Straight:
					{
						/* Standard quarter note rhythm. */
						const auto noteLength = std::min(samplesPerBeat, totalSamples - sample);

						/* LEFT: Bass + Chords */
						synthLeft.setRegion(sample, noteLength);
						synthLeft.sineWave(bassNote, 0.15F * beatIntensity);

						if ( texture == TextureStyle::Pad || texture == TextureStyle::Layered )
						{
							addNoteToChannel(synthLeft, sample, noteLength, chord.root, 0.09F * beatIntensity, 0.05F, 0.1F, 0.7F, 0.15F);
							addNoteToChannel(synthLeft, sample, noteLength, chord.third, 0.07F * beatIntensity, 0.06F, 0.1F, 0.65F, 0.15F);
							addNoteToChannel(synthLeft, sample, noteLength, chord.fifth, 0.05F * beatIntensity, 0.07F, 0.1F, 0.6F, 0.15F);
						}
						synthLeft.applyADSR(0.02F, 0.05F, 0.8F, 0.1F);

						/* RIGHT: Melody */
						synthRight.setRegion(sample, noteLength);
						addNoteToChannel(synthRight, sample, noteLength, melodyNote, 0.22F * beatIntensity, 0.02F, 0.08F, 0.7F, 0.1F, true);

						if ( addCounterMelody && (beat == 1 || beat == 3) )
						{
							addNoteToChannel(synthRight, sample, noteLength, melodyNote * 0.75F, 0.08F * beatIntensity, 0.03F, 0.1F, 0.5F, 0.15F, true);
						}
						synthRight.applyADSR(0.02F, 0.05F, 0.8F, 0.1F);

						sample += noteLength;
						break;
					}

					case RhythmStyle::Syncopated:
					{
						/* First eighth: rest or soft. */
						const auto firstHalf = std::min(samplesPerEighth, totalSamples - sample);

						/* LEFT: Soft bass */
						synthLeft.setRegion(sample, firstHalf);
						synthLeft.sineWave(bassNote, 0.08F * beatIntensity);
						if ( beat == 0 || beat == 2 )
						{
							addNoteToChannel(synthLeft, sample, firstHalf, chord.root, 0.05F * beatIntensity, 0.01F, 0.05F, 0.4F, 0.1F);
						}
						synthLeft.applyADSR(0.01F, 0.03F, 0.5F, 0.08F);

						/* RIGHT: Rest on first half */
						synthRight.setRegion(sample, firstHalf);
						synthRight.applyADSR(0.01F, 0.03F, 0.3F, 0.08F);

						sample += firstHalf;

						/* Second eighth: accented (syncopation). */
						const auto secondHalf = std::min(samplesPerEighth, totalSamples - sample);

						/* LEFT: Accented bass + chord */
						synthLeft.setRegion(sample, secondHalf);
						synthLeft.sineWave(bassNote, 0.16F * beatIntensity);
						addNoteToChannel(synthLeft, sample, secondHalf, chord.root, 0.1F * beatIntensity, 0.01F, 0.06F, 0.7F, 0.1F);
						addNoteToChannel(synthLeft, sample, secondHalf, chord.third, 0.08F * beatIntensity, 0.015F, 0.06F, 0.65F, 0.1F);
						synthLeft.applyADSR(0.01F, 0.04F, 0.85F, 0.08F);

						/* RIGHT: Melody on offbeat */
						synthRight.setRegion(sample, secondHalf);
						addNoteToChannel(synthRight, sample, secondHalf, melodyNote, 0.25F * beatIntensity, 0.01F, 0.05F, 0.75F, 0.08F, true);
						synthRight.applyADSR(0.01F, 0.04F, 0.85F, 0.08F);

						sample += secondHalf;
						break;
					}

					case RhythmStyle::Arpeggiated:
					{
						/* Break the beat into smaller arpeggiated notes. */
						const std::array< float, 2 > arpNotes = {{chord.root, chord.fifth}};

						for ( size_t arpIdx = 0; arpIdx < 2 && sample < totalSamples; ++arpIdx )
						{
							const auto arpLength = std::min(samplesPerEighth, totalSamples - sample);

							/* LEFT: Walking bass + arpeggio */
							synthLeft.setRegion(sample, arpLength);
							if ( arpIdx == 0 )
							{
								synthLeft.sineWave(bassNote, 0.12F * beatIntensity);
							}
							addNoteToChannel(synthLeft, sample, arpLength, arpNotes[arpIdx], 0.12F * beatIntensity, 0.01F, 0.04F, 0.6F, 0.08F);
							synthLeft.applyADSR(0.01F, 0.03F, 0.7F, 0.06F);

							/* RIGHT: Melody + passing tones */
							synthRight.setRegion(sample, arpLength);
							if ( arpIdx == 0 && (beat == 0 || beat == 2) )
							{
								addNoteToChannel(synthRight, sample, arpLength, melodyNote, 0.2F * beatIntensity, 0.015F, 0.06F, 0.7F, 0.1F, true);
							}
							else if ( arpIdx == 1 && (beat == 1 || beat == 3) )
							{
								const auto passingNote = melodyNote * 1.06F;
								addNoteToChannel(synthRight, sample, arpLength, passingNote, 0.1F * beatIntensity, 0.01F, 0.04F, 0.5F, 0.08F, true);
							}
							synthRight.applyADSR(0.01F, 0.03F, 0.7F, 0.06F);

							sample += arpLength;
						}
						break;
					}

					case RhythmStyle::Sparse:
					{
						const auto noteLength = std::min(samplesPerBeat, totalSamples - sample);

						/* Only play on beats 1 and 3 (half notes feel). */
						if ( beat == 0 || beat == 2 )
						{
							/* LEFT: Sustained bass + simple chord */
							synthLeft.setRegion(sample, noteLength);
							synthLeft.sineWave(bassNote, 0.12F * beatIntensity);
							addNoteToChannel(synthLeft, sample, noteLength, chord.root, 0.08F * beatIntensity, 0.08F, 0.15F, 0.6F, 0.2F);
							addNoteToChannel(synthLeft, sample, noteLength, chord.fifth, 0.05F * beatIntensity, 0.1F, 0.15F, 0.55F, 0.2F);
							synthLeft.applyADSR(0.04F, 0.1F, 0.7F, 0.2F);

							/* RIGHT: Melody with longer envelope */
							synthRight.setRegion(sample, noteLength);
							addNoteToChannel(synthRight, sample, noteLength, melodyNote, 0.18F * beatIntensity, 0.03F, 0.1F, 0.65F, 0.2F, true);
							synthRight.applyADSR(0.04F, 0.1F, 0.7F, 0.2F);
						}
						else
						{
							/* Ghost notes - very quiet on left only */
							synthLeft.setRegion(sample, noteLength);
							synthLeft.sineWave(bassNote * 2.0F, 0.03F * beatIntensity);
							synthLeft.applyADSR(0.02F, 0.05F, 0.3F, 0.1F);
						}

						sample += noteLength;
						break;
					}
				}

				/* Add high shimmer on beat 1 of certain measures (right channel). */
				if ( beat == 0 && (measureInSection == 0 || measureInSection == 4) && texture == TextureStyle::Layered )
				{
					const auto shimmerLength = samplesPerBeat / 4;
					if ( sample >= shimmerLength )
					{
						synthRight.setRegion(sample - shimmerLength, shimmerLength);
						addNoteToChannel(synthRight, sample - shimmerLength, shimmerLength, melodyNote * 2.0F, 0.03F * intensity, 0.01F, 0.02F, 0.3F, 0.05F);
					}
				}
			}
		};

		/* Lambda to generate a full section with style parameters. */
		auto generateSection = [&](size_t & sample,
		                           const std::array< Chord, 4 > & chords,
		                           const std::array< std::array< float, 4 >, 4 > & melody,
		                           float intensity, RhythmStyle rhythm, TextureStyle texture,
		                           bool counterMelody, bool varySecondPass)
		{
			/* Play the chord progression twice per section. */
			for ( size_t rep = 0; rep < 2 && sample < totalSamples; ++rep )
			{
				/* Vary rhythm on second pass if requested. */
				RhythmStyle currentRhythm = rhythm;
				if ( varySecondPass && rep == 1 )
				{
					/* Switch to a complementary rhythm. */
					if ( rhythm == RhythmStyle::Straight )
					{
						currentRhythm = RhythmStyle::Syncopated;
					}
					else if ( rhythm == RhythmStyle::Arpeggiated )
					{
						currentRhythm = RhythmStyle::Straight;
					}
				}

				/* Slightly vary intensity between passes. */
				const auto passIntensity = intensity * (rep == 0 ? 0.95F : 1.05F);

				for ( size_t chordIdx = 0; chordIdx < 4 && sample < totalSamples; ++chordIdx )
				{
					const auto & chord = chords[chordIdx];
					const auto & melodyLine = melody[chordIdx];
					const auto & bassLine = bassPatternA[chordIdx];
					const auto measureNum = rep * 4 + chordIdx;

					generateMeasure(sample, chord, melodyLine, bassLine, passIntensity,
					                currentRhythm, texture, measureNum, counterMelody);
				}
			}
		};

		/* Generate the full loop structure with varied styles. */
		size_t currentSample = 0;

		/* Section 1: A (intro - gentle, sparse). */
		generateSection(currentSample, chordsA, melodyA, 0.75F, RhythmStyle::Sparse, TextureStyle::Minimal, false, false);

		/* Section 2: A' (building - add rhythm). */
		generateSection(currentSample, chordsAv, melodyAv, 0.85F, RhythmStyle::Straight, TextureStyle::Pad, false, true);

		/* Section 3: B (development - syncopated energy). */
		generateSection(currentSample, chordsB, melodyB, 1.0F, RhythmStyle::Syncopated, TextureStyle::Layered, true, false);

		/* Section 4: A (return - full arpeggiated). */
		generateSection(currentSample, chordsA, melodyA, 0.95F, RhythmStyle::Arpeggiated, TextureStyle::Pad, false, true);

		/* Section 5: C (bridge - contrast, sparse and ethereal). */
		generateSection(currentSample, chordsC, melodyC, 0.85F, RhythmStyle::Sparse, TextureStyle::Layered, true, false);

		/* Section 6: A' (after bridge - building back). */
		generateSection(currentSample, chordsAv, melodyAv, 0.95F, RhythmStyle::Straight, TextureStyle::Layered, true, true);

		/* Section 7: B' (climax - full syncopation). */
		generateSection(currentSample, chordsBv, melodyBv, 1.0F, RhythmStyle::Syncopated, TextureStyle::Layered, true, false);

		/* Section 8: A (loop point - return to sparse, matches intro). */
		generateSection(currentSample, chordsA, melodyA, 0.8F, RhythmStyle::Sparse, TextureStyle::Minimal, false, false);

		/* Apply global effects to each channel. */
		synthLeft.resetRegion();
		synthRight.resetRegion();

		/* Gentle chorus for width. */
		synthLeft.applyChorus(0.7F, 6.0F, 0.25F);
		synthRight.applyChorus(0.7F, 6.0F, 0.25F);

		/* Soft reverb for ambience. */
		synthLeft.applyReverb(0.35F, 0.55F, 0.2F);
		synthRight.applyReverb(0.35F, 0.55F, 0.2F);

		/* Final normalization per channel. */
		synthLeft.normalize();
		synthRight.normalize();

		/* Interleave the two mono channels into a stereo buffer. */
		m_localData.initialize(totalSamples, WaveFactory::Channels::Stereo, frequencyPlayback);
		auto & stereoData = m_localData.data();
		const auto & leftData = leftWave.data();
		const auto & rightData = rightWave.data();

		for ( size_t i = 0; i < totalSamples; ++i )
		{
			stereoData[i * 2] = leftData[i];
			stereoData[i * 2 + 1] = rightData[i];
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

		if ( !WaveFactory::FileIO::read(filepath, m_localData) )
		{
			TraceError{ClassId} << "Unable to load the music file '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		/* Checks frequency for playback within the audio engine. */
		if ( m_localData.frequency() != Manager::frequencyPlayback() )
		{
			TraceWarning{ClassId} <<
				"Music '" << this->name() << "' frequency mismatch the system ! "
				"Resampling the wave from " << static_cast< int >(m_localData.frequency()) << "Hz to " << static_cast< int >(Manager::frequencyPlayback()) << "Hz ...";

			/* Copy the buffer in float (single precision) format. */
			WaveFactory::Processor processor{m_localData};

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
			if ( !processor.resample(Manager::frequencyPlayback()) )
			{
				TraceError{ClassId} << "Unable to resample the wave to " << static_cast< int >(Manager::frequencyPlayback()) << "Hz !";

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
		if ( filepath.extension() != ".mid" && filepath.extension() != ".midi" )
		{
			this->readMetaData(filepath);
		}

		return this->setLoadSuccess(true);
	}

	bool
	MusicResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & data) noexcept
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
			TraceError{ClassId} << "Failed to generate music '" << this->name() << "' from JSON data !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}
}
