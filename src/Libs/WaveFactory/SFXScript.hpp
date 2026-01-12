/*
 * src/Libs/WaveFactory/SFXScript.hpp
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
#include <cstddef>
#include <filesystem>
#include <string>

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Synthesizer.hpp"

namespace EmEn::Libs::WaveFactory
{
	/**
	 * @brief The SFXScript class parses JSON files to generate sound effects.
	 * @tparam precision_t The precision type of the wave. Default int16_t.
	 *
	 * JSON format:
	 * @code{.json}
	 * {
	 *   "duration": 5000,
	 *   "channels": 2,
	 *   "tracks": [
	 *	 {
	 *	   "preInstructions": [...],
	 *	   "regions": [
	 *		 { "offset": 0, "length": 2000, "instructions": [...] }
	 *	   ],
	 *	   "instructions": [...]
	 *	 },
	 *	 {
	 *	   "preInstructions": [...],
	 *	   "regions": [
	 *		 { "offset": 0, "length": 2000, "instructions": [...] }
	 *	   ],
	 *	   "instructions": [...]
	 *	 }
	 *   ],
	 *   "finalInstructions": [...]
	 * }
	 * @endcode
	 *
	 * - `channels`: Number of channels (1 = mono, 2 = stereo)
	 * - `tracks`: Array of track definitions, one per channel
	 * - Each track is synthesized in mono, then interleaved into the final Wave
	 * - `preInstructions`: Applied first on the full track (generators)
	 * - `regions`: Applied to specific portions of the track (modifiers)
	 * - `instructions`: Applied last on the full track (post-processing)
	 * - `finalInstructions`: Applied to all tracks uniformly before interleaving
	 */
	template< typename precision_t = int16_t >
	requires (std::is_arithmetic_v< precision_t >)
	class SFXScript final
	{
		public:

			/**
			 * @brief Constructs a SFXScript with target wave and sample rate.
			 * @param wave A reference to the wave to synthesize into.
			 * @param frequency The sample rate (default: 48kHz).
			 */
			explicit
			SFXScript (Wave< precision_t > & wave, Frequency frequency = Frequency::PCM48000Hz) noexcept
				: m_wave{wave},
				m_frequency{frequency}
			{

			}

			/**
			 * @brief Generates audio from a JSON file.
			 * @param filepath Path to the JSON script file.
			 * @return bool True on success, false on error.
			 */
			[[nodiscard]]
			bool
			generateFromFile (const std::filesystem::path & filepath) noexcept
			{
				const auto root = FastJSON::getRootFromFile(filepath);

				if ( !root.has_value() )
				{
					std::cerr << "[SFXScript] Failed to parse JSON file: " << filepath << "\n";

					return false;
				}

				return this->processScript(root.value());
			}

			/**
			 * @brief Generates audio from a JSON string.
			 * @param jsonString The JSON content as a string.
			 * @return bool True on success, false on error.
			 */
			[[nodiscard]]
			bool
			generateFromString (const std::string & jsonString) noexcept
			{
				const auto root = FastJSON::getRootFromString(jsonString);

				if ( !root.has_value() )
				{
					std::cerr << "[SFXScript] Failed to parse JSON string !\n";

					return false;
				}

				return this->processScript(root.value());
			}

			/**
			 * @brief Generates audio from a pre-parsed JSON value.
			 * @param data The JSON data node.
			 * @return bool True on success, false on error.
			 */
			[[nodiscard]]
			bool
			generateFromData (const Json::Value & data) noexcept
			{
				return this->processScript(data);
			}

		private:

			/**
			 * @brief Converts channel count to Channels enum.
			 * @param count The number of channels.
			 * @return Channels The channel enum value.
			 */
			[[nodiscard]]
			static
			Channels
			channelCountToEnum (size_t count) noexcept
			{
				switch ( count )
				{
					case 1:
						return Channels::Mono;

					case 2:
						return Channels::Stereo;

					default:
						return Channels::Invalid;
				}
			}

			/**
			 * @brief Processes the main script structure.
			 * @param root The root JSON node.
			 * @return bool
			 */
			bool
			processScript (const Json::Value & root) noexcept
			{
				/* Get duration in milliseconds. */
				const auto durationMs = FastJSON::getValue< uint32_t >(root, "duration");

				if ( !durationMs.has_value() )
				{
					std::cerr << "[SFXScript] Missing 'duration' field (in milliseconds) !\n";

					return false;
				}

				/* Get channel count from JSON. */
				const auto channelCount = FastJSON::getValue< size_t >(root, "channels").value_or(1);
				const auto channels = channelCountToEnum(channelCount);

				if ( channels == Channels::Invalid )
				{
					std::cerr << "[SFXScript] Invalid channel count: " << channelCount << " (only 1 or 2 supported) !\n";

					return false;
				}

				/* Get tracks array. */
				const auto tracks = FastJSON::getArray(root, "tracks");

				if ( !tracks.has_value() )
				{
					std::cerr << "[SFXScript] Missing 'tracks' array !\n";

					return false;
				}

				if ( tracks.value().size() != channelCount )
				{
					std::cerr << "[SFXScript] Track count (" << tracks.value().size() << ") doesn't match channel count (" << channelCount << ") !\n";

					return false;
				}

				/* Calculate sample count from duration. */
				const auto sampleRate = static_cast< size_t >(m_frequency);
				const auto sampleCount = (sampleRate * durationMs.value()) / 1000;

				/* Get finalInstructions early since they will be applied to each track. */
				const auto finalInstructions = FastJSON::getArray(root, "finalInstructions");

				/* Synthesize each track separately in mono. */
				std::vector< Wave< precision_t > > trackWaves(channelCount);

				for ( size_t trackIndex = 0; trackIndex < channelCount; ++trackIndex )
				{
					const auto & trackData = tracks.value()[static_cast< Json::ArrayIndex >(trackIndex)];

					if ( !this->processTrack(trackWaves[trackIndex], trackData, sampleCount, sampleRate) )
					{
						std::cerr << "[SFXScript] Failed to process track " << trackIndex << " !\n";

						return false;
					}

					/* Apply finalInstructions to each track (still mono). */
					if ( finalInstructions.has_value() )
					{
						Synthesizer< precision_t > finalSynth{trackWaves[trackIndex]};

						for ( const auto & instruction : finalInstructions.value() )
						{
							if ( !this->executeInstruction(finalSynth, instruction) )
							{
								return false;
							}
						}
					}
				}

				/* Initialize the final wave with proper channel count. */
				if ( !m_wave.initialize(sampleCount, channels, m_frequency) )
				{
					std::cerr << "[SFXScript] Failed to initialize output wave !\n";

					return false;
				}

				/* Interleave track data into the final wave. */
				auto & outputData = m_wave.data();

				for ( size_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex )
				{
					for ( size_t channel = 0; channel < channelCount; ++channel )
					{
						outputData[sampleIndex * channelCount + channel] = trackWaves[channel].data()[sampleIndex];
					}
				}

				return true;
			}

			/**
			 * @brief Processes a single track.
			 * @param trackWave The output wave for this track.
			 * @param trackData The track JSON node.
			 * @param sampleCount The number of samples.
			 * @param sampleRate The sample rate for time conversion.
			 * @return bool
			 */
			bool
			processTrack (Wave< precision_t > & trackWave, const Json::Value & trackData, size_t sampleCount, size_t sampleRate) noexcept
			{
				/* Initialize the track wave in mono. */
				Synthesizer< precision_t > synth{trackWave, sampleCount, m_frequency};

				/* Process preInstructions first (generators on full track). */
				if ( const auto preInstructions = FastJSON::getArray(trackData, "preInstructions"); preInstructions.has_value() )
				{
					for ( const auto & instruction : preInstructions.value() )
					{
						if ( !this->executeInstruction(synth, instruction) )
						{
							return false;
						}
					}
				}

				/* Process regions if present (modifiers on specific portions). */
				if ( const auto regions = FastJSON::getArray(trackData, "regions"); regions.has_value() )
				{
					for ( const auto & region : regions.value() )
					{
						if ( !this->processRegion(synth, region, sampleRate) )
						{
							return false;
						}
					}
				}

				/* Process instructions last (post-processing on full track). */
				if ( const auto instructions = FastJSON::getArray(trackData, "instructions"); instructions.has_value() )
				{
					synth.resetRegion();

					for ( const auto & instruction : instructions.value() )
					{
						if ( !this->executeInstruction(synth, instruction) )
						{
							return false;
						}
					}
				}

				return true;
			}

			/**
			 * @brief Processes a single region.
			 * @param synth The synthesizer instance.
			 * @param region The region JSON node.
			 * @param sampleRate The sample rate for time conversion.
			 * @return bool
			 */
			bool
			processRegion (Synthesizer< precision_t > & synth, const Json::Value & region, size_t sampleRate) noexcept
			{
				/* Get offset and length in milliseconds. */
				const auto offsetMs = FastJSON::getValue< uint32_t >(region, "offset").value_or(0);
				const auto lengthMs = FastJSON::getValue< uint32_t >(region, "length").value_or(0);

				/* Convert to samples. */
				const auto offsetSamples = (sampleRate * offsetMs) / 1000;
				const auto lengthSamples = (lengthMs > 0) ? (sampleRate * lengthMs) / 1000 : 0;

				synth.setRegion(offsetSamples, lengthSamples);

				/* Process instructions in this region. */
				if ( const auto instructions = FastJSON::getArray(region, "instructions"); instructions.has_value() )
				{
					for ( const auto & instruction : instructions.value() )
					{
						if ( !this->executeInstruction(synth, instruction) )
						{
							return false;
						}
					}
				}

				return true;
			}

			/**
			 * @brief Executes a single instruction.
			 * @param synth The synthesizer instance.
			 * @param instruction The instruction JSON node.
			 * @return bool
			 */
			bool
			executeInstruction (Synthesizer< precision_t > & synth, const Json::Value & instruction) noexcept
			{
				const auto type = FastJSON::getValue< std::string >(instruction, "type");

				if ( !type.has_value() )
				{
					std::cerr << "[SFXScript] Instruction missing 'type' field !\n";

					return false;
				}

				const auto & typeStr = type.value();

				/* Oscillators / Generators. */
				if ( typeStr == "whiteNoise" )
				{
					return synth.whiteNoise();
				}

				if ( typeStr == "pinkNoise" )
				{
					return synth.pinkNoise();
				}

				if ( typeStr == "brownNoise" )
				{
					return synth.brownNoise();
				}

				if ( typeStr == "blueNoise" )
				{
					return synth.blueNoise();
				}

				if ( typeStr == "sineWave" )
				{
					const auto frequency = FastJSON::getValue< float >(instruction, "frequency").value_or(440.0F);
					const auto amplitude = FastJSON::getValue< float >(instruction, "amplitude").value_or(0.5F);

					return synth.sineWave(frequency, amplitude);
				}

				if ( typeStr == "squareWave" )
				{
					const auto frequency = FastJSON::getValue< float >(instruction, "frequency").value_or(440.0F);
					const auto amplitude = FastJSON::getValue< float >(instruction, "amplitude").value_or(0.5F);

					return synth.squareWave(frequency, amplitude);
				}

				if ( typeStr == "triangleWave" )
				{
					const auto frequency = FastJSON::getValue< float >(instruction, "frequency").value_or(440.0F);
					const auto amplitude = FastJSON::getValue< float >(instruction, "amplitude").value_or(0.5F);

					return synth.triangleWave(frequency, amplitude);
				}

				if ( typeStr == "sawtoothWave" )
				{
					const auto frequency = FastJSON::getValue< float >(instruction, "frequency").value_or(440.0F);
					const auto amplitude = FastJSON::getValue< float >(instruction, "amplitude").value_or(0.5F);

					return synth.sawtoothWave(frequency, amplitude);
				}

				if ( typeStr == "pitchSweep" )
				{
					const auto startFreq = FastJSON::getValue< float >(instruction, "startFrequency").value_or(440.0F);
					const auto endFreq = FastJSON::getValue< float >(instruction, "endFrequency").value_or(880.0F);
					const auto amplitude = FastJSON::getValue< float >(instruction, "amplitude").value_or(0.5F);

					return synth.pitchSweep(startFreq, endFreq, amplitude);
				}

				if ( typeStr == "noiseBurst" )
				{
					const auto decayTime = FastJSON::getValue< float >(instruction, "decayTime").value_or(0.1F);
					const auto amplitude = FastJSON::getValue< float >(instruction, "amplitude").value_or(0.8F);
					const auto useWhite = FastJSON::getValue< bool >(instruction, "whiteNoise").value_or(true);

					return synth.noiseBurst(decayTime, amplitude, useWhite);
				}

				/* Envelope / Modulation. */
				if ( typeStr == "applyADSR" )
				{
					const auto attack = FastJSON::getValue< float >(instruction, "attack").value_or(0.01F);
					const auto decay = FastJSON::getValue< float >(instruction, "decay").value_or(0.1F);
					const auto sustain = FastJSON::getValue< float >(instruction, "sustain").value_or(0.7F);
					const auto release = FastJSON::getValue< float >(instruction, "release").value_or(0.1F);

					return synth.applyADSR(attack, decay, sustain, release);
				}

				if ( typeStr == "applyVibrato" )
				{
					const auto rate = FastJSON::getValue< float >(instruction, "rate").value_or(5.0F);
					const auto depth = FastJSON::getValue< float >(instruction, "depth").value_or(0.02F);

					return synth.applyVibrato(rate, depth);
				}

				if ( typeStr == "applyTremolo" )
				{
					const auto rate = FastJSON::getValue< float >(instruction, "rate").value_or(8.0F);
					const auto depth = FastJSON::getValue< float >(instruction, "depth").value_or(0.5F);

					return synth.applyTremolo(rate, depth);
				}

				if ( typeStr == "applyFadeIn" )
				{
					const auto fadeTime = FastJSON::getValue< float >(instruction, "time").value_or(0.1F);

					return synth.applyFadeIn(fadeTime);
				}

				if ( typeStr == "applyFadeOut" )
				{
					const auto fadeTime = FastJSON::getValue< float >(instruction, "time").value_or(0.1F);

					return synth.applyFadeOut(fadeTime);
				}

				/* Filters. */
				if ( typeStr == "applyLowPass" )
				{
					const auto cutoff = FastJSON::getValue< float >(instruction, "cutoff").value_or(1000.0F);

					return synth.applyLowPass(cutoff);
				}

				if ( typeStr == "applyHighPass" )
				{
					const auto cutoff = FastJSON::getValue< float >(instruction, "cutoff").value_or(500.0F);

					return synth.applyHighPass(cutoff);
				}

				if ( typeStr == "applyWahWah" )
				{
					const auto rate = FastJSON::getValue< float >(instruction, "rate").value_or(2.0F);
					const auto depth = FastJSON::getValue< float >(instruction, "depth").value_or(0.8F);
					const auto minFreq = FastJSON::getValue< float >(instruction, "minFrequency").value_or(400.0F);
					const auto maxFreq = FastJSON::getValue< float >(instruction, "maxFrequency").value_or(2000.0F);

					return synth.applyWahWah(rate, depth, minFreq, maxFreq);
				}

				if ( typeStr == "applyAutoWah" )
				{
					const auto sensitivity = FastJSON::getValue< float >(instruction, "sensitivity").value_or(3.0F);
					const auto minFreq = FastJSON::getValue< float >(instruction, "minFrequency").value_or(200.0F);
					const auto maxFreq = FastJSON::getValue< float >(instruction, "maxFrequency").value_or(3000.0F);
					const auto attack = FastJSON::getValue< float >(instruction, "attack").value_or(0.01F);
					const auto release = FastJSON::getValue< float >(instruction, "release").value_or(0.1F);

					return synth.applyAutoWah(sensitivity, minFreq, maxFreq, attack, release);
				}

				/* Distortion effects. */
				if ( typeStr == "applyDistortion" )
				{
					const auto gain = FastJSON::getValue< float >(instruction, "gain").value_or(10.0F);
					const auto mix = FastJSON::getValue< float >(instruction, "mix").value_or(1.0F);
					const auto hardClip = FastJSON::getValue< bool >(instruction, "hardClip").value_or(false);

					return synth.applyDistortion(gain, mix, hardClip);
				}

				if ( typeStr == "applyOverdrive" )
				{
					const auto drive = FastJSON::getValue< float >(instruction, "drive").value_or(5.0F);
					const auto tone = FastJSON::getValue< float >(instruction, "tone").value_or(0.5F);

					return synth.applyOverdrive(drive, tone);
				}

				if ( typeStr == "applyFuzz" )
				{
					const auto intensity = FastJSON::getValue< float >(instruction, "intensity").value_or(10.0F);
					const auto octaveUp = FastJSON::getValue< bool >(instruction, "octaveUp").value_or(false);

					return synth.applyFuzz(intensity, octaveUp);
				}

				if ( typeStr == "applyBitCrush" )
				{
					const auto bits = FastJSON::getValue< int32_t >(instruction, "bits").value_or(8);

					return synth.applyBitCrush(bits);
				}

				if ( typeStr == "applySampleRateReduce" )
				{
					const auto factor = FastJSON::getValue< int32_t >(instruction, "factor").value_or(4);

					return synth.applySampleRateReduce(factor);
				}

				if ( typeStr == "applyRingModulation" )
				{
					const auto frequency = FastJSON::getValue< float >(instruction, "frequency").value_or(440.0F);

					return synth.applyRingModulation(frequency);
				}

				/* Modulation effects. */
				if ( typeStr == "applyChorus" )
				{
					const auto rate = FastJSON::getValue< float >(instruction, "rate").value_or(1.5F);
					const auto depth = FastJSON::getValue< float >(instruction, "depth").value_or(10.0F);
					const auto mix = FastJSON::getValue< float >(instruction, "mix").value_or(0.5F);

					return synth.applyChorus(rate, depth, mix);
				}

				if ( typeStr == "applyFlanger" )
				{
					const auto rate = FastJSON::getValue< float >(instruction, "rate").value_or(0.5F);
					const auto depth = FastJSON::getValue< float >(instruction, "depth").value_or(5.0F);
					const auto feedback = FastJSON::getValue< float >(instruction, "feedback").value_or(0.7F);
					const auto mix = FastJSON::getValue< float >(instruction, "mix").value_or(0.5F);

					return synth.applyFlanger(rate, depth, feedback, mix);
				}

				if ( typeStr == "applyPhaser" )
				{
					const auto rate = FastJSON::getValue< float >(instruction, "rate").value_or(0.5F);
					const auto depth = FastJSON::getValue< float >(instruction, "depth").value_or(0.7F);
					const auto stages = FastJSON::getValue< int32_t >(instruction, "stages").value_or(4);
					const auto feedback = FastJSON::getValue< float >(instruction, "feedback").value_or(0.5F);
					const auto mix = FastJSON::getValue< float >(instruction, "mix").value_or(0.5F);

					return synth.applyPhaser(rate, depth, stages, feedback, mix);
				}

				/* Delay / Reverb. */
				if ( typeStr == "applyDelay" )
				{
					const auto delayTime = FastJSON::getValue< float >(instruction, "delayTime").value_or(300.0F);
					const auto feedback = FastJSON::getValue< float >(instruction, "feedback").value_or(0.4F);
					const auto mix = FastJSON::getValue< float >(instruction, "mix").value_or(0.5F);

					return synth.applyDelay(delayTime, feedback, mix);
				}

				if ( typeStr == "applyReverb" )
				{
					const auto roomSize = FastJSON::getValue< float >(instruction, "roomSize").value_or(0.5F);
					const auto damping = FastJSON::getValue< float >(instruction, "damping").value_or(0.5F);
					const auto mix = FastJSON::getValue< float >(instruction, "mix").value_or(0.3F);

					return synth.applyReverb(roomSize, damping, mix);
				}

				/* Dynamics. */
				if ( typeStr == "applyCompressor" )
				{
					const auto threshold = FastJSON::getValue< float >(instruction, "threshold").value_or(-20.0F);
					const auto ratio = FastJSON::getValue< float >(instruction, "ratio").value_or(4.0F);
					const auto attack = FastJSON::getValue< float >(instruction, "attack").value_or(0.01F);
					const auto release = FastJSON::getValue< float >(instruction, "release").value_or(0.1F);
					const auto makeupGain = FastJSON::getValue< float >(instruction, "makeupGain").value_or(0.0F);

					return synth.applyCompressor(threshold, ratio, attack, release, makeupGain);
				}

				if ( typeStr == "applyNoiseGate" )
				{
					const auto threshold = FastJSON::getValue< float >(instruction, "threshold").value_or(-40.0F);
					const auto attack = FastJSON::getValue< float >(instruction, "attack").value_or(0.001F);
					const auto hold = FastJSON::getValue< float >(instruction, "hold").value_or(0.05F);
					const auto release = FastJSON::getValue< float >(instruction, "release").value_or(0.1F);

					return synth.applyNoiseGate(threshold, attack, hold, release);
				}

				if ( typeStr == "applyPitchShift" )
				{
					const auto semitones = FastJSON::getValue< float >(instruction, "semitones").value_or(0.0F);
					const auto mix = FastJSON::getValue< float >(instruction, "mix").value_or(1.0F);

					return synth.applyPitchShift(semitones, mix);
				}

				/* Utilities. */
				if ( typeStr == "normalize" )
				{
					return synth.normalize();
				}

				if ( typeStr == "reverse" )
				{
					return synth.reverse();
				}

				std::cerr << "[SFXScript] Unknown instruction type: " << typeStr << "\n";

				return false;
			}

			Wave< precision_t > & m_wave;
			Frequency m_frequency;
	};
}
