/*
 * src/Libs/WaveFactory/Synthesizer.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <numbers>
#include <random>
#include <type_traits>

/* Local inclusions for usages. */
#include "Wave.hpp"
#include "Types.hpp"

namespace EmEn::Libs::WaveFactory
{
	/**
	 * @brief The Synthesizer class. Used to generate sounds directly into a Wave.
	 * @tparam precision_t The precision type of the wave. Default int16_t.
	 */
	template< typename precision_t = int16_t >
	requires (std::is_arithmetic_v< precision_t >)
	class Synthesizer final
	{
		public:

			/**
			 * @brief General MIDI instrument families for waveform selection.
			 * @note Each family has a unique harmonic profile for rich synthesis.
			 */
			enum class InstrumentFamily : uint8_t
			{
				Piano,	   /* 0-7: Soft sine-like tones. */
				Chromatic,   /* 8-15: Mallet percussion. */
				Organ,	   /* 16-23: Rich harmonics. */
				Guitar,	  /* 24-31: Bright plucked strings. */
				Bass,		/* 32-39: Low-end. */
				Strings,	 /* 40-47: Bowed strings. */
				Ensemble,	/* 48-55: String/choir ensembles. */
				Brass,	   /* 56-63: Bright, punchy. */
				Reed,		/* 64-71: Woodwinds. */
				Pipe,		/* 72-79: Flutes, whistles. */
				SynthLead,   /* 80-87: Electronic leads. */
				SynthPad,	/* 88-95: Pads, atmospheres. */
				SynthFX,	 /* 96-103: Effects. */
				Ethnic,	  /* 104-111: World instruments. */
				Percussive,  /* 112-119: Melodic percussion. */
				SoundFX	  /* 120-127: Sound effects. */
			};

			/**
			 * @brief Harmonic definition for wavetable synthesis.
			 * @note Each harmonic has a multiplier (1 = fundamental, 2 = octave, etc.) and amplitude.
			 */
			struct Harmonic
			{
				float multiplier{1.0F};  /* Frequency multiplier (1, 2, 3, ...). */
				float amplitude{1.0F};   /* Relative amplitude (0.0 to 1.0). */
			};

			/**
			 * @brief Filter state for per-sample resonant low-pass filter.
			 * @note Use with applyResonantFilterSample() for real-time filtering.
			 */
			struct FilterState
			{
				float buf0{0.0F};  /* First filter buffer. */
				float buf1{0.0F};  /* Second filter buffer. */
			};

			/**
			 * @brief Normalized harmonic for pre-computed wavetable synthesis.
			 * @note Amplitudes are pre-divided by total amplitude sum for efficiency.
			 */
			struct NormalizedHarmonic
			{
				float multiplier{1.0F};		  /* Frequency multiplier. */
				float normalizedAmplitude{1.0F}; /* Pre-normalized amplitude. */
			};

			/* ======================================================================================== */
			/*							  SINE LOOKUP TABLE (LUT)									  */
			/* ======================================================================================== */

			/** @brief Size of sine lookup table (4096 = ~0.09° resolution, good quality). */
			static constexpr size_t SinTableSize = 4096;

			/**
			 * @brief Generates a sine lookup table at runtime.
			 * @return std::array< float, SinTableSize > The sine table covering [0, 2π).
			 * @note Cannot be constexpr because std::sin() is not constexpr in standard C++.
			 */
			static std::array< float, SinTableSize >
			generateSinTable () noexcept
			{
				std::array< float, SinTableSize > table{};

				for ( size_t i = 0; i < SinTableSize; ++i )
				{
					/* Use double precision for table generation, cast to float. */
					const double angle = (static_cast< double >(i) / static_cast< double >(SinTableSize)) * (2.0 * std::numbers::pi);
					table[i] = static_cast< float >(std::sin(angle));
				}

				return table;
			}

			/** @brief Pre-computed sine lookup table (initialized at program startup). */
			static inline const std::array< float, SinTableSize > SinTable = generateSinTable();

			/**
			 * @brief Fast sine approximation using lookup table with linear interpolation.
			 * @param phase Phase in range [0.0, 1.0) representing one full cycle.
			 * @return float Sine value in range [-1.0, 1.0].
			 * @note ~10-20x faster than std::sin() with acceptable audio quality.
			 */
			[[nodiscard]]
			static float
			fastSin (float phase) noexcept
			{
				/* Wrap phase to [0, 1) range. */
				phase -= std::floor(phase);

				/* Convert phase to table index with fractional part. */
				const float indexF = phase * static_cast< float >(SinTableSize);
				const auto index0 = static_cast< size_t >(indexF) & (SinTableSize - 1);
				const auto index1 = (index0 + 1) & (SinTableSize - 1);
				const float frac = indexF - std::floor(indexF);

				/* Linear interpolation between adjacent table entries. */
				return SinTable[index0] + frac * (SinTable[index1] - SinTable[index0]);
			}

			/**
			 * @brief Fast sine for angle in radians.
			 * @param radians Angle in radians.
			 * @return float Sine value.
			 */
			[[nodiscard]]
			static float
			fastSinRadians (float radians) noexcept
			{
				constexpr float InvTwoPi = 1.0F / (2.0F * std::numbers::pi_v< float >);
				return fastSin(radians * InvTwoPi);
			}

			/**
			 * @brief Constructs a synthesizer using an existing wave's parameters.
			 * @param wave A reference to the wave to synthesize into.
			 */
			explicit
			Synthesizer (Wave< precision_t > & wave) noexcept
				: m_wave{wave}
			{

			}

			/**
			 * @brief Constructs a synthesizer and initializes/reinitializes the wave in mono.
			 * @param wave A reference to the wave to synthesize into.
			 * @param sampleCount The number of samples.
			 * @param frequency The sample rate.
			 */
			Synthesizer (Wave< precision_t > & wave, size_t sampleCount, Frequency frequency) noexcept
				: m_wave{wave}
			{
				m_wave.initialize(sampleCount, Channels::Mono, frequency);
			}

			/* ======================================================================================== */
			/*							  STATIC PER-SAMPLE FUNCTIONS								 */
			/* ======================================================================================== */

			/**
			 * @brief Returns the instrument family for a GM program number.
			 * @param program The MIDI program number (0-127).
			 * @return InstrumentFamily
			 */
			[[nodiscard]]
			static InstrumentFamily
			getInstrumentFamily (uint8_t program) noexcept
			{
				return static_cast< InstrumentFamily >(program / 8);
			}

			/**
			 * @brief Converts a MIDI note number to frequency in Hz.
			 * @note MIDI note 69 = A4 = 440 Hz.
			 * @param noteNumber The MIDI note number (0-127).
			 * @return float The frequency in Hz.
			 */
			[[nodiscard]]
			static constexpr float
			noteToFrequency (uint8_t noteNumber) noexcept
			{
				/* Using exp2f is ~3x faster than pow(2, x) on x86_64. */
				return 440.0F * std::exp2f((static_cast< float >(noteNumber) - 69.0F) / 12.0F);
			}

			/**
			 * @brief Generates a rich waveform sample using additive synthesis with harmonics.
			 * @param family The instrument family.
			 * @param phase The current phase (0.0 to 1.0).
			 * @return float The sample value (normalized -1.0 to 1.0).
			 */
			[[nodiscard]]
			static float
			generateWaveformSample (InstrumentFamily family, float phase) noexcept
			{
				constexpr float TwoPi = 2.0F * std::numbers::pi_v< float >;

				/* Define harmonic profiles for each instrument family.
				 * These are inspired by real instrument spectra and classic DOS game sounds. */
				switch ( family )
				{
					case InstrumentFamily::Piano:
					{
						/* Piano: Strong fundamental with decaying upper harmonics. */
						constexpr std::array< Harmonic, 8 > harmonics{{
							{1.0F, 1.00F}, {2.0F, 0.50F}, {3.0F, 0.33F}, {4.0F, 0.25F},
							{5.0F, 0.15F}, {6.0F, 0.10F}, {7.0F, 0.07F}, {8.0F, 0.05F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::Chromatic:
					{
						/* Chromatic percussion: Bell-like with prominent upper partials. */
						constexpr std::array< Harmonic, 6 > harmonics{{
							{1.0F, 1.00F}, {2.0F, 0.60F}, {3.0F, 0.40F},
							{4.0F, 0.80F}, {5.0F, 0.20F}, {6.0F, 0.30F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::Organ:
					{
						/* Organ: Hammond-style drawbar sound. */
						constexpr std::array< Harmonic, 9 > harmonics{{
							{0.5F, 0.80F}, {1.0F, 1.00F}, {1.5F, 0.70F}, {2.0F, 0.90F}, {3.0F, 0.60F},
							{4.0F, 0.70F}, {5.0F, 0.40F}, {6.0F, 0.50F}, {8.0F, 0.30F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::Guitar:
					{
						/* Guitar: Plucked string with bright attack. */
						constexpr std::array< Harmonic, 10 > harmonics{{
							{1.0F, 1.00F}, {2.0F, 0.70F}, {3.0F, 0.45F}, {4.0F, 0.35F}, {5.0F, 0.25F},
							{6.0F, 0.18F}, {7.0F, 0.12F}, {8.0F, 0.08F}, {9.0F, 0.05F}, {10.0F, 0.03F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::Bass:
					{
						/* Bass: Strong fundamental, less upper content. */
						constexpr std::array< Harmonic, 6 > harmonics{{
							{1.0F, 1.00F}, {2.0F, 0.55F}, {3.0F, 0.30F},
							{4.0F, 0.15F}, {5.0F, 0.08F}, {6.0F, 0.04F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::Strings:
					case InstrumentFamily::Ensemble:
					{
						/* Strings: Bowed with slight detuning for warmth. */
						constexpr std::array< Harmonic, 12 > harmonics{{
							{1.000F, 1.00F}, {2.003F, 0.50F}, {2.997F, 0.33F}, {4.002F, 0.25F},
							{4.998F, 0.20F}, {6.001F, 0.17F}, {6.999F, 0.14F}, {8.003F, 0.12F},
							{8.997F, 0.11F}, {10.002F, 0.10F}, {10.998F, 0.09F}, {12.001F, 0.08F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::Brass:
					{
						/* Brass: Rich in harmonics, bright and punchy. */
						constexpr std::array< Harmonic, 10 > harmonics{{
							{1.0F, 1.00F}, {2.0F, 0.85F}, {3.0F, 0.70F}, {4.0F, 0.55F}, {5.0F, 0.45F},
							{6.0F, 0.35F}, {7.0F, 0.28F}, {8.0F, 0.22F}, {9.0F, 0.18F}, {10.0F, 0.15F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::Reed:
					{
						/* Reed instruments: Predominantly odd harmonics (clarinet-like). */
						constexpr std::array< Harmonic, 8 > harmonics{{
							{1.0F, 1.00F}, {3.0F, 0.75F}, {5.0F, 0.50F}, {7.0F, 0.35F},
							{9.0F, 0.25F}, {11.0F, 0.18F}, {13.0F, 0.12F}, {15.0F, 0.08F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::Pipe:
					{
						/* Flute/Pipe: Mostly fundamental, pure tone. */
						constexpr std::array< Harmonic, 4 > harmonics{{
							{1.0F, 1.00F}, {2.0F, 0.15F}, {3.0F, 0.08F}, {4.0F, 0.03F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::SynthLead:
					{
						/* Synth Lead: Classic sawtooth-like. */
						constexpr std::array< Harmonic, 12 > harmonics{{
							{1.0F, 1.00F}, {2.0F, 0.50F}, {3.0F, 0.33F}, {4.0F, 0.25F},
							{5.0F, 0.20F}, {6.0F, 0.17F}, {7.0F, 0.14F}, {8.0F, 0.12F},
							{9.0F, 0.11F}, {10.0F, 0.10F}, {11.0F, 0.09F}, {12.0F, 0.08F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::SynthPad:
					{
						/* Synth Pad: Smooth with detuned oscillators. */
						constexpr std::array< Harmonic, 6 > harmonics{{
							{1.000F, 1.00F}, {1.005F, 0.80F}, {2.000F, 0.40F},
							{2.007F, 0.35F}, {3.000F, 0.20F}, {4.000F, 0.10F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::SynthFX:
					{
						/* Synth FX: Metallic/inharmonic partials. */
						constexpr std::array< Harmonic, 6 > harmonics{{
							{1.0F, 1.00F}, {1.414F, 0.70F}, {2.0F, 0.50F},
							{2.828F, 0.40F}, {3.5F, 0.30F}, {5.0F, 0.20F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::Ethnic:
					{
						/* Ethnic: Sitar-like with sympathetic resonance. */
						constexpr std::array< Harmonic, 8 > harmonics{{
							{1.0F, 1.00F}, {2.0F, 0.60F}, {3.0F, 0.45F}, {4.0F, 0.35F},
							{5.0F, 0.50F}, {6.0F, 0.25F}, {7.0F, 0.40F}, {8.0F, 0.15F}
						}};
						return computeHarmonics(harmonics, phase, TwoPi);
					}

					case InstrumentFamily::Percussive:
					case InstrumentFamily::SoundFX:
					default:
					{
						/* Default: Simple sine using fast LUT. */
						return fastSin(phase);
					}
				}
			}

			/**
			 * @brief Converts filter cutoff CC value (0-127) to normalized coefficient.
			 * @param cutoffCC The CC#74 value (0-127).
			 * @param sampleRate The sample rate for frequency calculation.
			 * @return float Normalized cutoff coefficient for the filter (0.0 to 1.0).
			 */
			[[nodiscard]]
			static float
			filterCutoffToCoefficient (int16_t cutoffCC, float sampleRate) noexcept
			{
				constexpr float MinFreq = 100.0F;
				const float maxFreq = sampleRate * 0.45F;

				const float normalized = static_cast< float >(cutoffCC) / 127.0F;
				/* exp2f(log2(x) * n) = x^n, faster than pow(). */
				const float frequency = MinFreq * std::exp2f(std::log2f(maxFreq / MinFreq) * normalized);

				return std::min(1.0F, 2.0F * std::sin(std::numbers::pi_v< float > * frequency / sampleRate));
			}

			/**
			 * @brief Converts filter resonance CC value (0-127) to feedback amount.
			 * @param resonanceCC The CC#71 value (0-127).
			 * @return float Resonance feedback amount (0.0 to ~3.8).
			 */
			[[nodiscard]]
			static float
			filterResonanceToFeedback (int16_t resonanceCC) noexcept
			{
				return static_cast< float >(resonanceCC) / 127.0F * 3.8F;
			}

			/**
			 * @brief Applies a 2-pole resonant low-pass filter to a single sample.
			 * @param sample The input sample.
			 * @param cutoff The filter cutoff coefficient (from filterCutoffToCoefficient).
			 * @param resonance The resonance feedback (from filterResonanceToFeedback).
			 * @param state The filter state (updated in place).
			 * @return float The filtered sample.
			 */
			[[nodiscard]]
			static float
			applyResonantFilterSample (float sample, float cutoff, float resonance, FilterState & state) noexcept
			{
				if ( cutoff >= 0.99F ) [[unlikely]]
				{
					return sample; /* Filter fully open, bypass. */
				}

				const float feedback = resonance + resonance / (1.0F - cutoff * 0.5F);
				state.buf0 += cutoff * (sample - state.buf0 + feedback * (state.buf0 - state.buf1));
				state.buf1 += cutoff * (state.buf0 - state.buf1);

				return state.buf1;
			}

			/**
			 * @brief Calculates ADSR envelope value at a given sample position.
			 * @param localSample The sample index relative to note start.
			 * @param sampleRate The sample rate.
			 * @param totalSamples Total samples in the note.
			 * @param attack Attack time in seconds.
			 * @param decay Decay time in seconds.
			 * @param sustain Sustain level (0.0 to 1.0).
			 * @param release Release time in seconds.
			 * @return float The envelope value (0.0 to 1.0).
			 */
			[[nodiscard]]
			static float
			calculateEnvelopeSample (uint32_t localSample, uint32_t sampleRate, uint32_t totalSamples, float attack, float decay, float sustain, float release) noexcept
			{
				const auto sampleRateF = static_cast< float >(sampleRate);
				const auto attackSamples = static_cast< uint32_t >(attack * sampleRateF);
				const auto decaySamples = static_cast< uint32_t >(decay * sampleRateF);
				const auto releaseSamples = static_cast< uint32_t >(release * sampleRateF);
				const uint32_t sustainSamples = (totalSamples > attackSamples + decaySamples + releaseSamples)
					? totalSamples - attackSamples - decaySamples - releaseSamples
					: 0;

				if ( localSample < attackSamples )
				{
					return static_cast< float >(localSample) / static_cast< float >(attackSamples);
				}

				if ( localSample < attackSamples + decaySamples )
				{
					const auto decayProgress = static_cast< float >(localSample - attackSamples) / static_cast< float >(decaySamples);
					return 1.0F - decayProgress * (1.0F - sustain);
				}

				if ( localSample < attackSamples + decaySamples + sustainSamples )
				{
					return sustain;
				}

				const auto releaseProgress = static_cast< float >(localSample - attackSamples - decaySamples - sustainSamples) / static_cast< float >(releaseSamples);

				return sustain * std::max(0.0F, 1.0F - releaseProgress);
			}

			/**
			 * @brief Calculates vibrato multiplier for a given time.
			 * @param time Time in seconds from note start.
			 * @param depth Modulation depth (0.0 to 1.0, from CC#1/127).
			 * @param rate Vibrato rate in Hz. Default 5.5.
			 * @param maxDepth Maximum pitch deviation. Default 0.02 (2%).
			 * @return float Frequency multiplier to apply.
			 */
			[[nodiscard]]
			static float
			calculateVibratoMultiplier (float time, float depth, float rate = 5.5F, float maxDepth = 0.02F) noexcept
			{
				/* fastSin takes phase [0,1), so phase = rate * time (cycles). */
				return 1.0F + maxDepth * depth * fastSin(rate * time);
			}

			/**
			 * @brief Calculates tremolo multiplier for a given time.
			 * @param time Time in seconds from note start.
			 * @param depth Tremolo depth (0.0 to 1.0, from CC#92/127).
			 * @param rate Tremolo rate in Hz. Default 5.0.
			 * @return float Amplitude multiplier to apply (0.5 to 1.0 at max depth).
			 */
			[[nodiscard]]
			static float
			calculateTremoloMultiplier (float time, float depth, float rate = 5.0F) noexcept
			{
				/* fastSin takes phase [0,1), so phase = rate * time (cycles). */
				return 1.0F - depth * 0.5F * (1.0F - fastSin(rate * time));
			}

			/**
			 * @brief Calculates pitch bend multiplier.
			 * @param bendValue Pitch bend value (-8192 to +8191).
			 * @param bendRange Bend range in semitones. Default 2.0.
			 * @return float Frequency multiplier to apply.
			 */
			[[nodiscard]]
			static float
			calculatePitchBendMultiplier (int16_t bendValue, float bendRange = 2.0F) noexcept
			{
				/* exp2f is ~3x faster than pow(2, x) on x86_64. */
				return std::exp2f((static_cast< float >(bendValue) / 8192.0F) * bendRange / 12.0F);
			}

			/**
			 * @brief Calculates portamento frequency for logarithmic glide.
			 * @param startFreq Starting frequency in Hz.
			 * @param endFreq Target frequency in Hz.
			 * @param progress Glide progress (0.0 to 1.0).
			 * @return float Current frequency in Hz.
			 */
			[[nodiscard]]
			static float
			calculatePortamentoFrequency (float startFreq, float endFreq, float progress) noexcept
			{
				if ( startFreq <= 0.0F || endFreq <= 0.0F || progress <= 0.0F ) [[unlikely]]
				{
					return endFreq;
				}

				/* exp2f(log2(x) * n) = x^n, faster than pow(). */
				return startFreq * std::exp2f(std::log2f(endFreq / startFreq) * progress);
			}

			/**
			 * @brief Converts portamento time CC value to duration in seconds.
			 * @param portamentoCC The CC#5 value (0-127).
			 * @return float Duration in seconds (0 to ~2 seconds).
			 */
			[[nodiscard]]
			static float
			portamentoTimeToSeconds (int16_t portamentoCC) noexcept
			{
				if ( portamentoCC <= 0 ) [[unlikely]]
				{
					return 0.0F;
				}

				const float normalized = static_cast< float >(portamentoCC) / 127.0F;
				return 2.0F * normalized * normalized;
			}

			/* ======================================================================================== */
			/*							  INSTANCE METHODS (WAVE OPERATIONS)						  */
			/* ======================================================================================== */

			/**
			 * @brief Generates white noise (flat spectrum, equal energy per frequency).
			 * @return bool
			 */
			bool
			whiteNoise () noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				auto & data = m_wave.data();

				std::random_device randomDevice;
				std::mt19937 generator{randomDevice()};

				if constexpr ( std::is_floating_point_v< precision_t > )
				{
					std::uniform_real_distribution< precision_t > distribution(static_cast< precision_t >(-1), static_cast< precision_t >(1));

					for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
					{
						data[sampleIndex] = distribution(generator);
					}
				}
				else
				{
					std::uniform_int_distribution< precision_t > distribution(
						std::numeric_limits< precision_t >::lowest(),
						std::numeric_limits< precision_t >::max()
					);

					for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
					{
						data[sampleIndex] = distribution(generator);
					}
				}

				return true;
			}

			/**
			 * @brief Generates pink noise (-3 dB/octave, equal energy per octave).
			 * @note Useful for natural ambiences like wind, waterfalls, etc.
			 * @return bool
			 */
			bool
			pinkNoise () noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				std::random_device randomDevice;
				std::mt19937 generator(randomDevice());
				std::uniform_real_distribution< float > distribution(-1.0F, 1.0F);

				/* Voss-McCartney algorithm using multiple octave bands.
				 * This creates pink noise with -3 dB/octave rolloff. */
				constexpr size_t NumRows = 16;
				std::array< float, NumRows > rows{};
				float runningSum = 0.0F;
				size_t index = 0;

				const auto [startSample, endSample] = this->getEffectiveRange();
				auto & data = m_wave.data();

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					/* Determine which rows to update based on trailing zeros in index. */
					size_t numZeros = 0;
					size_t tempIndex = index;

					while ( (tempIndex & 1) == 0 && numZeros < NumRows )
					{
						numZeros++;
						tempIndex >>= 1;
					}

					/* Update the appropriate row. */
					if ( numZeros < NumRows )
					{
						runningSum -= rows[numZeros];
						rows[numZeros] = distribution(generator);
						runningSum += rows[numZeros];
					}

					/* Add white noise component and normalize. */
					const auto pinkSample = (runningSum + distribution(generator)) / (static_cast< float >(NumRows) + 1.0F);

					data[sampleIndex] = toSampleFormat(pinkSample);

					index++;
				}

				return true;
			}

			/**
			 * @brief Generates brown/red noise (-6 dB/octave, Brownian motion).
			 * @note Useful for deep rumbles, thunder, engine sounds.
			 * @return bool
			 */
			bool
			brownNoise () noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				std::random_device randomDevice;
				std::mt19937 generator(randomDevice());
				std::uniform_real_distribution< float > distribution(-1.0F, 1.0F);

				/* Brown noise is the integral of white noise (random walk).
				 * Each sample is previous sample + small random step. */
				constexpr float StepSize = 0.02F;
				constexpr float MaxValue = 1.0F;
				float lastValue = 0.0F;

				const auto [startSample, endSample] = this->getEffectiveRange();
				auto & data = m_wave.data();

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					/* Random walk step. */
					lastValue += distribution(generator) * StepSize;

					/* Clamp to prevent runaway. */
					lastValue = std::clamp(lastValue, -MaxValue, MaxValue);

					data[sampleIndex] = toSampleFormat(lastValue);
				}

				return true;
			}

			/**
			 * @brief Generates blue noise (+3 dB/octave, high frequency emphasis).
			 * @note Useful for audio dithering. Uses a first-order high-pass filter on white noise.
			 * @return bool
			 */
			bool
			blueNoise () noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				std::random_device randomDevice;
				std::mt19937 generator{randomDevice()};
				std::uniform_real_distribution< float > distribution(-1.0F, 1.0F);

				/* Blue noise uses a high-pass filter on white noise.
				 * First-order high-pass: y[n] = alpha * (y[n-1] + x[n] - x[n-1])
				 * With alpha close to 1 for strong high-pass effect. */
				constexpr float Alpha = 0.98F;
				float lastInput = 0.0F;
				float lastOutput = 0.0F;

				const auto [startSample, endSample] = this->getEffectiveRange();
				auto & data = m_wave.data();

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const float currentInput = distribution(generator);

					/* High-pass filter. */
					const float currentOutput = Alpha * (lastOutput + currentInput - lastInput);

					lastInput = currentInput;
					lastOutput = currentOutput;

					/* Clamp output. */
					data[sampleIndex] = toSampleFormat(std::clamp(currentOutput, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Generates a sine wave tone.
			 * @param toneFrequency The frequency of the sine wave in Hz. Default 440 (A4).
			 * @param amplitude The amplitude of the sine wave (0.0 to 1.0). Default 0.5.
			 * @return bool
			 */
			bool
			sineWave (float toneFrequency = 440.0F, float amplitude = 0.5F) noexcept
			{
				if ( !m_wave.isValid() ) [[unlikely]]
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Phase accumulation is ~6x faster than time-based calculation.
				 * phaseIncrement = frequency / sampleRate (cycles per sample). */
				const float phaseIncrement = toneFrequency / sampleRateFloat;
				float phase = 0.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					/* Using fastSin() instead of std::sin() for ~10-20x speedup. */
					data[sampleIndex] = toSampleFormat(amplitude * fastSin(phase));

					phase += phaseIncrement;
					if ( phase >= 1.0F ) [[unlikely]]
					{
						phase -= 1.0F;
					}
				}

				return true;
			}

			/**
			 * @brief Generates a square wave tone.
			 * @param toneFrequency The frequency of the square wave in Hz. Default 440 (A4).
			 * @param amplitude The amplitude of the square wave (0.0 to 1.0). Default 0.5.
			 * @return bool
			 */
			bool
			squareWave (float toneFrequency = 440.0F, float amplitude = 0.5F) noexcept
			{
				if ( !m_wave.isValid() ) [[unlikely]]
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Phase accumulation avoids expensive sin() call entirely for square wave. */
				const float phaseIncrement = toneFrequency / sampleRateFloat;
				float phase = 0.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					/* Square wave: positive for first half of cycle, negative for second half. */
					data[sampleIndex] = toSampleFormat(phase < 0.5F ? amplitude : -amplitude);

					phase += phaseIncrement;
					if ( phase >= 1.0F ) [[unlikely]]
					{
						phase -= 1.0F;
					}
				}

				return true;
			}

			/**
			 * @brief Generates a triangle wave tone.
			 * @note Softer sound than square wave, odd harmonics only.
			 * @param toneFrequency The frequency of the triangle wave in Hz. Default 440 (A4).
			 * @param amplitude The amplitude of the triangle wave (0.0 to 1.0). Default 0.5.
			 * @return bool
			 */
			bool
			triangleWave (float toneFrequency = 440.0F, float amplitude = 0.5F) noexcept
			{
				if ( !m_wave.isValid() ) [[unlikely]]
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Phase accumulation for triangle wave. */
				const float phaseIncrement = toneFrequency / sampleRateFloat;
				float phase = 0.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					/* Triangle wave: linear ramp up then down.
					 * phase [0, 0.5) -> ramp from -1 to +1
					 * phase [0.5, 1) -> ramp from +1 to -1 */
					const float triangleValue = phase < 0.5F
						? (4.0F * phase - 1.0F)
						: (3.0F - 4.0F * phase);

					data[sampleIndex] = toSampleFormat(amplitude * triangleValue);

					phase += phaseIncrement;
					if ( phase >= 1.0F ) [[unlikely]]
					{
						phase -= 1.0F;
					}
				}

				return true;
			}

			/**
			 * @brief Generates a sawtooth wave tone.
			 * @note Rich in harmonics, buzzy sound. Good for brass/string synthesis.
			 * @param toneFrequency The frequency of the sawtooth wave in Hz. Default 440 (A4).
			 * @param amplitude The amplitude of the sawtooth wave (0.0 to 1.0). Default 0.5.
			 * @return bool
			 */
			bool
			sawtoothWave (float toneFrequency = 440.0F, float amplitude = 0.5F) noexcept
			{
				if ( !m_wave.isValid() ) [[unlikely]]
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Phase accumulation for sawtooth wave. */
				const float phaseIncrement = toneFrequency / sampleRateFloat;
				float phase = 0.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					/* Sawtooth wave: linear ramp from -1 to +1 over each cycle. */
					data[sampleIndex] = toSampleFormat(amplitude * (2.0F * phase - 1.0F));

					phase += phaseIncrement;
					if ( phase >= 1.0F ) [[unlikely]]
					{
						phase -= 1.0F;
					}
				}

				return true;
			}

			/**
			 * @brief Applies an ADSR envelope to the current wave data.
			 * @note This modifies the existing wave by applying amplitude modulation.
			 * @param attackTime Attack time in seconds (ramp up from 0 to 1).
			 * @param decayTime Decay time in seconds (ramp down from 1 to sustain level).
			 * @param sustainLevel Sustain amplitude level (0.0 to 1.0).
			 * @param releaseTime Release time in seconds (ramp down from sustain to 0).
			 * @return bool
			 */
			bool
			applyADSR (float attackTime, float decayTime, float sustainLevel, float releaseTime) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto regionSampleCount = endSample - startSample;
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Calculate sample boundaries for each phase. */
				const auto attackSamples = static_cast< size_t >(attackTime * sampleRateFloat);
				const auto decaySamples = static_cast< size_t >(decayTime * sampleRateFloat);
				const auto releaseSamples = static_cast< size_t >(releaseTime * sampleRateFloat);

				/* Sustain fills the remaining time. */
				const size_t sustainSamples = regionSampleCount > attackSamples + decaySamples + releaseSamples
					? regionSampleCount - attackSamples - decaySamples - releaseSamples
					: 0;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto localIndex = sampleIndex - startSample;
					float envelope = 0.0F;

					if ( localIndex < attackSamples )
					{
						/* Attack phase: ramp from 0 to 1. */
						envelope = static_cast< float >(localIndex) / static_cast< float >(attackSamples);
					}
					else if ( localIndex < attackSamples + decaySamples )
					{
						/* Decay phase: ramp from 1 to sustain level. */
						const auto decayProgress = static_cast< float >(localIndex - attackSamples) / static_cast< float >(decaySamples);
						envelope = 1.0F - decayProgress * (1.0F - sustainLevel);
					}
					else if ( localIndex < attackSamples + decaySamples + sustainSamples )
					{
						/* Sustain phase: constant level. */
						envelope = sustainLevel;
					}
					else
					{
						/* Release phase: ramp from sustain to 0. */
						const auto releaseProgress = static_cast< float >(localIndex - attackSamples - decaySamples - sustainSamples) / static_cast< float >(releaseSamples);
						envelope = sustainLevel * (1.0F - releaseProgress);
					}

					/* Apply envelope. */
					const auto currentSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());
					data[sampleIndex] = toSampleFormat(currentSample * envelope);
				}

				return true;
			}

			/**
			 * @brief Generates a pitch sweep (glissando) sine wave.
			 * @note Great for laser sounds, power-ups, jumps.
			 * @param startFrequency Starting frequency in Hz.
			 * @param endFrequency Ending frequency in Hz.
			 * @param amplitude The amplitude (0.0 to 1.0). Default 0.5.
			 * @return bool
			 */
			bool
			pitchSweep (float startFrequency, float endFrequency, float amplitude = 0.5F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto regionSampleCount = endSample - startSample;
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				float phase = 0.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto localIndex = sampleIndex - startSample;

					/* Interpolate frequency linearly. */
					const auto progress = static_cast< float >(localIndex) / static_cast< float >(regionSampleCount);
					const auto currentFreq = startFrequency + progress * (endFrequency - startFrequency);

					/* Accumulate phase for continuous waveform. */
					phase += currentFreq / sampleRateFloat;
					/* Use fastSin() instead of std::sin() for ~10-20x speedup. */
					const auto sample = amplitude * fastSin(phase);

					data[sampleIndex] = toSampleFormat(sample);
				}

				return true;
			}

			/**
			 * @brief Generates a noise burst with envelope (for percussions, impacts, hits).
			 * @param decayTime Time in seconds for the noise to decay. Default 0.1.
			 * @param amplitude The amplitude (0.0 to 1.0). Default 0.8.
			 * @param useWhiteNoise True for white noise, false for pink noise. Default true.
			 * @return bool
			 */
			bool
			noiseBurst (float decayTime = 0.1F, float amplitude = 0.8F, bool useWhiteNoise = true) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				/* Generate the noise first (region-aware). */
				if ( useWhiteNoise )
				{
					this->whiteNoise();
				}
				else
				{
					this->pinkNoise();
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				const auto decaySamples = static_cast< size_t >(decayTime * sampleRateFloat);

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto localIndex = sampleIndex - startSample;

					/* Exponential decay envelope. */
					float envelope = 0.0F;

					if ( localIndex < decaySamples )
					{
						const auto progress = static_cast< float >(localIndex) / static_cast< float >(decaySamples);
						envelope = amplitude * std::exp(-5.0F * progress);
					}

					const auto currentSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());
					data[sampleIndex] = toSampleFormat(currentSample * envelope);
				}

				return true;
			}

			/**
			 * @brief Applies vibrato effect (frequency modulation) to the existing wave.
			 * @param vibratoRate Rate of vibrato oscillation in Hz. Default 5.
			 * @param vibratoDepth Depth of pitch variation (0.0 to 1.0). Default 0.02.
			 * @return bool
			 */
			bool
			applyVibrato (float vibratoRate = 5.0F, float vibratoDepth = 0.02F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Create a copy of original data for the region. */
				std::vector< precision_t > originalData(data.begin() + static_cast< ptrdiff_t >(startSample),
														data.begin() + static_cast< ptrdiff_t >(endSample));

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto localIndex = sampleIndex - startSample;
					const auto dTime = static_cast< float >(localIndex) / sampleRateFloat;

					/* Calculate the modulated read position within the region. */
					/* Use fastSin() instead of std::sin() for ~10-20x speedup. */
					const auto modulation = vibratoDepth * sampleRateFloat * fastSin(vibratoRate * dTime);
					const auto readPos = static_cast< float >(localIndex) + modulation;

					/* Interpolate between samples. */
					const auto readPosInt = static_cast< size_t >(std::max(0.0F, readPos));
					const auto readPosFrac = readPos - static_cast< float >(readPosInt);

					if ( readPosInt + 1 < originalData.size() )
					{
						/* Linear interpolation. */
						const auto sample1 = static_cast< float >(originalData[readPosInt]);
						const auto sample2 = static_cast< float >(originalData[readPosInt + 1]);
						data[sampleIndex] = static_cast< precision_t >(sample1 + readPosFrac * (sample2 - sample1));
					}
				}

				return true;
			}

			/**
			 * @brief Applies tremolo effect (amplitude modulation) to the existing wave.
			 * @param tremoloRate Rate of tremolo oscillation in Hz. Default 8.
			 * @param tremoloDepth Depth of amplitude variation (0.0 to 1.0). Default 0.5.
			 * @return bool
			 */
			bool
			applyTremolo (float tremoloRate = 8.0F, float tremoloDepth = 0.5F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto dTime = static_cast< float >(sampleIndex - startSample) / sampleRateFloat;

					/* Tremolo: amplitude modulation with LFO. */
					/* Use fastSin() instead of std::sin() for ~10-20x speedup. */
					const auto modulation = 1.0F - tremoloDepth * 0.5F * (1.0F + fastSin(tremoloRate * dTime));

					const auto currentSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());
					data[sampleIndex] = toSampleFormat(currentSample * modulation);
				}

				return true;
			}

			/**
			 * @brief Applies a fade in effect at the beginning of the region.
			 * @param fadeTime Duration of the fade in seconds.
			 * @return bool
			 */
			bool
			applyFadeIn (float fadeTime) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto regionSampleCount = endSample - startSample;
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				const auto fadeSamples = static_cast< size_t >(fadeTime * sampleRateFloat);

				for ( size_t localIndex = 0; localIndex < std::min(fadeSamples, regionSampleCount); ++localIndex )
				{
					const auto sampleIndex = startSample + localIndex;
					const auto envelope = static_cast< float >(localIndex) / static_cast< float >(fadeSamples);

					const auto currentSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());
					data[sampleIndex] = toSampleFormat(currentSample * envelope);
				}

				return true;
			}

			/**
			 * @brief Applies a fade out effect at the end of the region.
			 * @param fadeTime Duration of the fade in seconds.
			 * @return bool
			 */
			bool
			applyFadeOut (float fadeTime) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto regionSampleCount = endSample - startSample;
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				const auto fadeSamples = static_cast< size_t >(fadeTime * sampleRateFloat);
				const auto fadeStartLocal = (regionSampleCount > fadeSamples) ? regionSampleCount - fadeSamples : 0;

				for ( size_t localIndex = fadeStartLocal; localIndex < regionSampleCount; ++localIndex )
				{
					const auto sampleIndex = startSample + localIndex;
					const auto envelope = 1.0F - static_cast< float >(localIndex - fadeStartLocal) / static_cast< float >(fadeSamples);

					const auto currentSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());
					data[sampleIndex] = toSampleFormat(currentSample * envelope);
				}

				return true;
			}

			/**
			 * @brief Applies a low-pass filter to attenuate high frequencies.
			 * @note Useful for muffled sounds, underwater, distance effects.
			 * @param cutoffFrequency The cutoff frequency in Hz. Default 1000.
			 * @return bool
			 */
			bool
			applyLowPass (float cutoffFrequency = 1000.0F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Simple first-order low-pass filter coefficient. */
				const auto rc = 1.0F / (2.0F * std::numbers::pi_v< float > * cutoffFrequency);
				const auto dt = 1.0F / sampleRateFloat;
				const auto alpha = dt / (rc + dt);

				float prevSample = 0.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto currentSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Low-pass filter: y[n] = y[n-1] + alpha * (x[n] - y[n-1]) */
					const auto filteredSample = prevSample + alpha * (currentSample - prevSample);
					prevSample = filteredSample;

					data[sampleIndex] = toSampleFormat(filteredSample);
				}

				return true;
			}

			/**
			 * @brief Applies a high-pass filter to attenuate low frequencies.
			 * @note Useful for radio effects, tinny sounds.
			 * @param cutoffFrequency The cutoff frequency in Hz. Default 500.
			 * @return bool
			 */
			bool
			applyHighPass (float cutoffFrequency = 500.0F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Simple first-order high-pass filter coefficient. */
				const auto rc = 1.0F / (2.0F * std::numbers::pi_v< float > * cutoffFrequency);
				const auto dt = 1.0F / sampleRateFloat;
				const auto alpha = rc / (rc + dt);

				float prevInput = 0.0F;
				float prevOutput = 0.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto currentInput = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* High-pass filter: y[n] = alpha * (y[n-1] + x[n] - x[n-1]) */
					const auto filteredSample = alpha * (prevOutput + currentInput - prevInput);
					prevInput = currentInput;
					prevOutput = filteredSample;

					data[sampleIndex] = toSampleFormat(std::clamp(filteredSample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Mixes another wave into the current wave.
			 * @param other The wave to mix in.
			 * @param mixLevel The mix level of the other wave (0.0 to 1.0). Default 0.5.
			 * @return bool
			 */
			bool
			mix (const Wave< precision_t > & other, float mixLevel = 0.5F) noexcept
			{
				if ( !m_wave.isValid() || !other.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave(s) not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				auto & data = m_wave.data();
				const auto & otherData = other.data();
				const auto thisLevel = 1.0F - mixLevel * 0.5F;
				const auto otherLevel = mixLevel * 0.5F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					if ( sampleIndex >= otherData.size() )
					{
						break;
					}

					const auto sample1 = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());
					const auto sample2 = static_cast< float >(otherData[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());
					const auto mixedSample = thisLevel * sample1 + otherLevel * sample2;

					data[sampleIndex] = toSampleFormat(std::clamp(mixedSample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies ring modulation effect (for robotic, alien sounds).
			 * @param modulatorFrequency The frequency of the modulator oscillator in Hz. Default 440.
			 * @return bool
			 */
			bool
			applyRingModulation (float modulatorFrequency = 440.0F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto dTime = static_cast< float >(sampleIndex - startSample) / sampleRateFloat;

					/* Ring modulation: multiply signal by carrier. */
					/* Use fastSin() instead of std::sin() for ~10-20x speedup. */
					const auto modulator = fastSin(modulatorFrequency * dTime);

					const auto currentSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());
					data[sampleIndex] = toSampleFormat(currentSample * modulator);
				}

				return true;
			}

			/**
			 * @brief Applies bit crushing effect (for lo-fi, retro sounds).
			 * @param bitDepth The target bit depth (1 to 16). Default 8.
			 * @return bool
			 */
			bool
			applyBitCrush (int bitDepth = 8) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				bitDepth = std::clamp(bitDepth, 1, 16);
				const auto levels = static_cast< float >(1 << bitDepth);

				const auto [startSample, endSample] = this->getEffectiveRange();
				auto & data = m_wave.data();

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto currentSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Quantize to reduced bit depth. */
					const auto crushedSample = std::round(currentSample * levels) / levels;
					data[sampleIndex] = toSampleFormat(crushedSample);
				}

				return true;
			}

			/**
			 * @brief Applies distortion effect (for aggressive, crunchy sounds).
			 * @note Classic guitar distortion with soft/hard clipping.
			 * @param gain Pre-amplification gain before clipping (1.0 to 50.0). Default 10.0.
			 * @param mix Dry/wet mix (0.0 = dry, 1.0 = full distortion). Default 1.0.
			 * @param hardClip Use hard clipping (true) or soft clipping (false). Default false.
			 * @return bool
			 */
			bool
			applyDistortion (float gain = 10.0F, float mix = 1.0F, bool hardClip = false) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				gain = std::max(1.0F, gain);
				mix = std::clamp(mix, 0.0F, 1.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				auto & data = m_wave.data();

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto drySample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Apply gain. */
					auto wetSample = drySample * gain;

					/* Apply clipping. */
					if ( hardClip )
					{
						/* Hard clipping: brutal cut at ±1.0. */
						wetSample = std::clamp(wetSample, -1.0F, 1.0F);
					}
					else
					{
						/* Soft clipping using tanh for smoother saturation. */
						wetSample = std::tanh(wetSample);
					}

					/* Mix dry and wet signals. */
					const auto outputSample = drySample * (1.0F - mix) + wetSample * mix;

					data[sampleIndex] = toSampleFormat(std::clamp(outputSample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies overdrive effect (warm tube-like saturation).
			 * @note Simulates tube amp saturation with asymmetric clipping.
			 * @param drive Amount of drive/saturation (1.0 to 20.0). Default 5.0.
			 * @param tone Tone control, affects brightness (0.0 = dark, 1.0 = bright). Default 0.5.
			 * @return bool
			 */
			bool
			applyOverdrive (float drive = 5.0F, float tone = 0.5F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				drive = std::clamp(drive, 1.0F, 20.0F);
				tone = std::clamp(tone, 0.0F, 1.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				auto & data = m_wave.data();

				/* Simple one-pole low-pass for tone control. */
				const auto toneAlpha = 0.1F + tone * 0.8F;
				float lastSample = 0.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					auto sample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Apply drive. */
					sample *= drive;

					/* Asymmetric soft clipping (tube-like behavior).
					 * Positive half clips softer than negative half. */
					if ( sample > 0.0F )
					{
						/* Softer clipping on positive half. */
						sample = 1.0F - std::exp(-sample);
					}
					else
					{
						/* Slightly harder clipping on negative half. */
						sample = -1.0F + std::exp(sample);
					}

					/* Apply tone control (simple low-pass filter). */
					sample = lastSample + toneAlpha * (sample - lastSample);
					lastSample = sample;

					data[sampleIndex] = toSampleFormat(std::clamp(sample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies fuzz effect (extreme, aggressive distortion).
			 * @note Creates a heavily saturated, "broken amp" sound.
			 * @param intensity Fuzz intensity (1.0 to 20.0). Default 10.0.
			 * @param octaveUp Add octave-up effect for classic fuzz sound. Default false.
			 * @return bool
			 */
			bool
			applyFuzz (float intensity = 10.0F, bool octaveUp = false) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				intensity = std::clamp(intensity, 1.0F, 20.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				auto & data = m_wave.data();

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					auto sample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Apply intensity gain. */
					sample *= intensity;

					/* Octave-up effect: full-wave rectification before clipping.
					 * Classic fuzz trick that adds upper harmonics. */
					if ( octaveUp )
					{
						sample = std::abs(sample) * 2.0F - 1.0F;
					}

					/* Aggressive clipping using signum-based hard limiting
					 * with a slight curve for character. */
					if ( sample > 0.0F )
					{
						sample = 1.0F - std::exp(-sample * 3.0F);
					}
					else
					{
						sample = -1.0F + std::exp(sample * 3.0F);
					}

					/* Add some "broken" character with slight asymmetry. */
					sample *= (sample > 0.0F) ? 0.95F : 1.0F;

					data[sampleIndex] = toSampleFormat(std::clamp(sample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies chorus effect (thickens the sound with detuned copies).
			 * @note Creates a rich, "doubled" sound by mixing delayed/modulated copies.
			 * @param rate LFO rate in Hz (0.1 to 5.0). Default 1.5.
			 * @param depth Depth of modulation in milliseconds (1.0 to 30.0). Default 10.0.
			 * @param mix Dry/wet mix (0.0 to 1.0). Default 0.5.
			 * @return bool
			 */
			bool
			applyChorus (float rate = 1.5F, float depth = 10.0F, float mix = 0.5F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				rate = std::clamp(rate, 0.1F, 5.0F);
				depth = std::clamp(depth, 1.0F, 30.0F);
				mix = std::clamp(mix, 0.0F, 1.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Convert depth from ms to samples. */
				const auto depthSamples = depth * sampleRateFloat / 1000.0F;

				/* Create a copy of original data for reading. */
				std::vector< precision_t > originalData(data.begin() + static_cast< ptrdiff_t >(startSample),
														data.begin() + static_cast< ptrdiff_t >(endSample));

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto localIndex = sampleIndex - startSample;
					const auto dTime = static_cast< float >(localIndex) / sampleRateFloat;

					/* LFO modulates the delay time. */
					/* Use fastSin() instead of std::sin() for ~10-20x speedup. */
					const auto lfo = (1.0F + fastSin(rate * dTime)) * 0.5F;
					const auto delayAmount = lfo * depthSamples;
					const auto readPos = static_cast< float >(localIndex) - delayAmount;

					const auto drySample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					float wetSample = 0.0F;

					if ( readPos >= 0.0F )
					{
						/* Linear interpolation for smooth modulation. */
						const auto readPosInt = static_cast< size_t >(readPos);
						const auto readPosFrac = readPos - static_cast< float >(readPosInt);

						if ( readPosInt + 1 < originalData.size() )
						{
							const auto sample1 = static_cast< float >(originalData[readPosInt]) / static_cast< float >(std::numeric_limits< precision_t >::max());
							const auto sample2 = static_cast< float >(originalData[readPosInt + 1]) / static_cast< float >(std::numeric_limits< precision_t >::max());
							wetSample = sample1 + readPosFrac * (sample2 - sample1);
						}
					}

					const auto outputSample = drySample * (1.0F - mix) + wetSample * mix;
					data[sampleIndex] = toSampleFormat(std::clamp(outputSample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies flanger effect (jet/swoosh sound with feedback).
			 * @note Creates a "jet plane" or "swooshing" effect.
			 * @param rate LFO rate in Hz (0.1 to 2.0). Default 0.5.
			 * @param depth Depth of modulation in milliseconds (1.0 to 10.0). Default 5.0.
			 * @param feedback Amount of feedback (0.0 to 0.95). Default 0.7.
			 * @param mix Dry/wet mix (0.0 to 1.0). Default 0.5.
			 * @return bool
			 */
			bool
			applyFlanger (float rate = 0.5F, float depth = 5.0F, float feedback = 0.7F, float mix = 0.5F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				rate = std::clamp(rate, 0.1F, 2.0F);
				depth = std::clamp(depth, 1.0F, 10.0F);
				feedback = std::clamp(feedback, 0.0F, 0.95F);
				mix = std::clamp(mix, 0.0F, 1.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Convert depth from ms to samples. */
				const auto depthSamples = depth * sampleRateFloat / 1000.0F;
				const auto maxDelaySamples = static_cast< size_t >(depthSamples + 1);

				/* Delay buffer. */
				std::vector< float > delayBuffer(maxDelaySamples, 0.0F);
				size_t writeIndex = 0;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto localIndex = sampleIndex - startSample;
					const auto dTime = static_cast< float >(localIndex) / sampleRateFloat;

					/* LFO modulates the delay time (triangle wave for flanger). */
					const auto lfoPhase = std::fmod(rate * dTime, 1.0F);
					const auto lfo = 2.0F * std::abs(2.0F * lfoPhase - 1.0F) - 1.0F;
					const auto delaySamples = (lfo * 0.5F + 0.5F) * depthSamples;

					const auto inputSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Read from delay buffer with interpolation. */
					const auto readOffset = delaySamples;
					const auto readOffsetInt = static_cast< size_t >(readOffset);
					const auto readOffsetFrac = readOffset - static_cast< float >(readOffsetInt);

					size_t readIdx1 = (writeIndex + maxDelaySamples - readOffsetInt) % maxDelaySamples;
					size_t readIdx2 = (readIdx1 + maxDelaySamples - 1) % maxDelaySamples;

					const auto delayedSample = delayBuffer[readIdx1] * (1.0F - readOffsetFrac) +
											   delayBuffer[readIdx2] * readOffsetFrac;

					/* Write to delay buffer with feedback. */
					delayBuffer[writeIndex] = inputSample + delayedSample * feedback;
					writeIndex = (writeIndex + 1) % maxDelaySamples;

					/* Mix dry and wet. */
					const auto outputSample = inputSample * (1.0F - mix) + delayedSample * mix;
					data[sampleIndex] = toSampleFormat(std::clamp(outputSample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies phaser effect (sweeping notch filters).
			 * @note Creates a "whooshing" or "sweeping" effect.
			 * @param rate LFO rate in Hz (0.1 to 3.0). Default 0.5.
			 * @param depth Depth of effect (0.0 to 1.0). Default 0.7.
			 * @param stages Number of all-pass filter stages (2, 4, 6, 8, 10, 12). Default 4.
			 * @param feedback Amount of feedback (0.0 to 0.95). Default 0.5.
			 * @param mix Dry/wet mix (0.0 to 1.0). Default 0.5.
			 * @return bool
			 */
			bool
			applyPhaser (float rate = 0.5F, float depth = 0.7F, int stages = 4, float feedback = 0.5F, float mix = 0.5F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				rate = std::clamp(rate, 0.1F, 3.0F);
				depth = std::clamp(depth, 0.0F, 1.0F);
				stages = std::clamp(stages, 2, 12);
				stages = (stages / 2) * 2;  /* Ensure even number. */
				feedback = std::clamp(feedback, 0.0F, 0.95F);
				mix = std::clamp(mix, 0.0F, 1.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Frequency range for the sweeping notches. */
				constexpr float MinFreq = 200.0F;
				constexpr float MaxFreq = 4000.0F;

				/* All-pass filter state per stage. */
				std::vector< float > allpassState(static_cast< size_t >(stages), 0.0F);
				float feedbackState = 0.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto localIndex = sampleIndex - startSample;
					const auto dTime = static_cast< float >(localIndex) / sampleRateFloat;

					/* LFO modulates the all-pass frequencies. */
					/* Use fastSin() instead of std::sin() for ~10-20x speedup. */
					const auto lfo = (1.0F + fastSin(rate * dTime)) * 0.5F;
					const auto sweepFreq = MinFreq + lfo * depth * (MaxFreq - MinFreq);

					/* Calculate all-pass coefficient. */
					const auto w0 = 2.0F * std::numbers::pi_v< float > * sweepFreq / sampleRateFloat;
					const auto allpassCoeff = (1.0F - std::tan(w0 / 2.0F)) / (1.0F + std::tan(w0 / 2.0F));

					auto inputSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Add feedback. */
					inputSample += feedbackState * feedback;

					/* Chain of all-pass filters. */
					auto processedSample = inputSample;

					for ( int stage = 0; stage < stages; ++stage )
					{
						const auto stageIdx = static_cast< size_t >(stage);
						const auto allpassOutput = allpassCoeff * processedSample + allpassState[stageIdx];
						allpassState[stageIdx] = processedSample - allpassCoeff * allpassOutput;
						processedSample = allpassOutput;
					}

					feedbackState = processedSample;

					/* Mix dry and wet (inverted wet for notch effect). */
					const auto drySample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());
					const auto outputSample = drySample * (1.0F - mix) + processedSample * mix;

					data[sampleIndex] = toSampleFormat(std::clamp(outputSample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies delay/echo effect.
			 * @note Creates repeating echoes of the sound.
			 * @param delayTime Delay time in milliseconds (10 to 2000). Default 300.
			 * @param feedback Amount of feedback for repeating echoes (0.0 to 0.95). Default 0.4.
			 * @param mix Dry/wet mix (0.0 to 1.0). Default 0.5.
			 * @return bool
			 */
			bool
			applyDelay (float delayTime = 300.0F, float feedback = 0.4F, float mix = 0.5F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				delayTime = std::clamp(delayTime, 10.0F, 2000.0F);
				feedback = std::clamp(feedback, 0.0F, 0.95F);
				mix = std::clamp(mix, 0.0F, 1.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Convert delay from ms to samples. */
				const auto delaySamples = static_cast< size_t >(delayTime * sampleRateFloat / 1000.0F);

				/* Delay buffer. */
				std::vector< float > delayBuffer(delaySamples, 0.0F);
				size_t writeIndex = 0;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto inputSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Read from delay buffer. */
					const auto delayedSample = delayBuffer[writeIndex];

					/* Write to delay buffer with feedback. */
					delayBuffer[writeIndex] = inputSample + delayedSample * feedback;
					writeIndex = (writeIndex + 1) % delaySamples;

					/* Mix dry and wet. */
					const auto outputSample = inputSample * (1.0F - mix) + delayedSample * mix;
					data[sampleIndex] = toSampleFormat(std::clamp(outputSample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies a simple reverb effect (Schroeder-style).
			 * @note Creates a sense of space/room around the sound.
			 * @param roomSize Size of the simulated room (0.0 = small, 1.0 = large). Default 0.5.
			 * @param damping High frequency damping (0.0 = bright, 1.0 = dark). Default 0.5.
			 * @param mix Dry/wet mix (0.0 to 1.0). Default 0.3.
			 * @return bool
			 */
			bool
			applyReverb (float roomSize = 0.5F, float damping = 0.5F, float mix = 0.3F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				roomSize = std::clamp(roomSize, 0.0F, 1.0F);
				damping = std::clamp(damping, 0.0F, 1.0F);
				mix = std::clamp(mix, 0.0F, 1.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Comb filter delay times (in samples at 44100 Hz, scaled for actual sample rate). */
				const auto scaleFactor = sampleRateFloat / 44100.0F;
				const std::array< size_t, 4 > combDelays = {
					static_cast< size_t >(1116 * scaleFactor * (0.7F + 0.3F * roomSize)),
					static_cast< size_t >(1188 * scaleFactor * (0.7F + 0.3F * roomSize)),
					static_cast< size_t >(1277 * scaleFactor * (0.7F + 0.3F * roomSize)),
					static_cast< size_t >(1356 * scaleFactor * (0.7F + 0.3F * roomSize))
				};

				/* All-pass filter delay times. */
				const std::array< size_t, 2 > allpassDelays = {
					static_cast< size_t >(225 * scaleFactor),
					static_cast< size_t >(556 * scaleFactor)
				};

				/* Feedback amount based on room size. */
				const auto combFeedback = 0.7F + 0.28F * roomSize;
				const auto dampingCoeff = damping * 0.4F;

				/* Reverb state buffers. */
				std::array< std::vector< float >, 4 > combBuffers;
				std::array< size_t, 4 > combIndices{};
				std::array< float, 4 > combFilters{};
				std::array< std::vector< float >, 2 > allpassBuffers;
				std::array< size_t, 2 > allpassIndices{};

				for ( size_t i = 0; i < 4; ++i )
				{
					combBuffers[i].resize(combDelays[i], 0.0F);
				}

				for ( size_t i = 0; i < 2; ++i )
				{
					allpassBuffers[i].resize(allpassDelays[i], 0.0F);
				}

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto inputSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Parallel comb filters. */
					float combOut = 0.0F;

					for ( size_t i = 0; i < 4; ++i )
					{
						const auto delayed = combBuffers[i][combIndices[i]];

						/* Low-pass filter in feedback loop for damping. */
						combFilters[i] = delayed * (1.0F - dampingCoeff) + combFilters[i] * dampingCoeff;
						combBuffers[i][combIndices[i]] = inputSample + combFilters[i] * combFeedback;
						combIndices[i] = (combIndices[i] + 1) % combDelays[i];

						combOut += delayed;
					}

					combOut *= 0.25F;

					/* Series all-pass filters. */
					float allpassOut = combOut;

					for ( size_t i = 0; i < 2; ++i )
					{
						const auto delayed = allpassBuffers[i][allpassIndices[i]];
						const auto temp = allpassOut + delayed * 0.5F;
						allpassBuffers[i][allpassIndices[i]] = allpassOut;
						allpassOut = delayed - temp * 0.5F;
						allpassIndices[i] = (allpassIndices[i] + 1) % allpassDelays[i];
					}

					/* Mix dry and wet. */
					const auto outputSample = inputSample * (1.0F - mix) + allpassOut * mix;
					data[sampleIndex] = toSampleFormat(std::clamp(outputSample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies wah-wah effect (sweeping band-pass filter).
			 * @note Creates the classic "wah" sound.
			 * @param rate LFO rate in Hz (0.5 to 10.0). Default 2.0.
			 * @param depth Depth of frequency sweep (0.0 to 1.0). Default 0.8.
			 * @param minFreq Minimum filter frequency in Hz. Default 400.
			 * @param maxFreq Maximum filter frequency in Hz. Default 2000.
			 * @return bool
			 */
			bool
			applyWahWah (float rate = 2.0F, float depth = 0.8F, float minFreq = 400.0F, float maxFreq = 2000.0F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				rate = std::clamp(rate, 0.5F, 10.0F);
				depth = std::clamp(depth, 0.0F, 1.0F);
				minFreq = std::clamp(minFreq, 100.0F, 1000.0F);
				maxFreq = std::clamp(maxFreq, minFreq + 100.0F, 5000.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* State variable filter state. */
				float lowpass = 0.0F;
				float bandpass = 0.0F;
				float highpass = 0.0F;

				constexpr float Q = 5.0F;  /* Resonance. */

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto localIndex = sampleIndex - startSample;
					const auto dTime = static_cast< float >(localIndex) / sampleRateFloat;

					/* LFO modulates the filter frequency. */
					/* Use fastSin() instead of std::sin() for ~10-20x speedup. */
					const auto lfo = (1.0F + fastSin(rate * dTime)) * 0.5F;
					const auto sweepFreq = minFreq + lfo * depth * (maxFreq - minFreq);

					/* Calculate filter coefficient. */
					/* Use fastSinRadians() instead of std::sin() for ~10-20x speedup. */
					const auto f = 2.0F * fastSinRadians(std::numbers::pi_v< float > * sweepFreq / sampleRateFloat);

					const auto inputSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* State variable filter. */
					highpass = inputSample - lowpass - bandpass / Q;
					bandpass += f * highpass;
					lowpass += f * bandpass;

					/* Output the bandpass (wah sound). */
					data[sampleIndex] = toSampleFormat(std::clamp(bandpass, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies auto-wah/envelope filter effect.
			 * @note The filter frequency follows the signal amplitude (funky sound).
			 * @param sensitivity How much the envelope affects the filter (0.1 to 10.0). Default 3.0.
			 * @param minFreq Minimum filter frequency in Hz. Default 200.
			 * @param maxFreq Maximum filter frequency in Hz. Default 3000.
			 * @param attackTime Envelope attack time in seconds. Default 0.01.
			 * @param releaseTime Envelope release time in seconds. Default 0.1.
			 * @return bool
			 */
			bool
			applyAutoWah (float sensitivity = 3.0F, float minFreq = 200.0F, float maxFreq = 3000.0F, float attackTime = 0.01F, float releaseTime = 0.1F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				sensitivity = std::clamp(sensitivity, 0.1F, 10.0F);
				minFreq = std::clamp(minFreq, 50.0F, 1000.0F);
				maxFreq = std::clamp(maxFreq, minFreq + 100.0F, 5000.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Envelope follower coefficients. */
				const auto attackCoeff = std::exp(-1.0F / (attackTime * sampleRateFloat));
				const auto releaseCoeff = std::exp(-1.0F / (releaseTime * sampleRateFloat));

				/* Filter state. */
				float envelope = 0.0F;
				float lowpass = 0.0F;
				float bandpass = 0.0F;
				float highpass = 0.0F;

				constexpr float Q = 4.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto inputSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Envelope follower. */
					const auto inputAbs = std::abs(inputSample);

					if ( inputAbs > envelope )
					{
						envelope = attackCoeff * envelope + (1.0F - attackCoeff) * inputAbs;
					}
					else
					{
						envelope = releaseCoeff * envelope + (1.0F - releaseCoeff) * inputAbs;
					}

					/* Map envelope to frequency. */
					const auto envScaled = std::min(1.0F, envelope * sensitivity);
					const auto sweepFreq = minFreq + envScaled * (maxFreq - minFreq);

					/* Calculate filter coefficient. */
					/* Use fastSinRadians() instead of std::sin() for ~10-20x speedup. */
					const auto f = 2.0F * fastSinRadians(std::numbers::pi_v< float > * sweepFreq / sampleRateFloat);

					/* State variable filter. */
					highpass = inputSample - lowpass - bandpass / Q;
					bandpass += f * highpass;
					lowpass += f * bandpass;

					data[sampleIndex] = toSampleFormat(std::clamp(bandpass, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies compressor effect (dynamic range compression).
			 * @note Makes quiet parts louder and loud parts quieter for a more "punchy" sound.
			 * @param threshold Threshold in dB (-60 to 0). Default -20.
			 * @param ratio Compression ratio (1.0 to 20.0, where 20 is limiting). Default 4.0.
			 * @param attackTime Attack time in seconds. Default 0.01.
			 * @param releaseTime Release time in seconds. Default 0.1.
			 * @param makeupGain Makeup gain in dB (0 to 30). Default 0 (auto).
			 * @return bool
			 */
			bool
			applyCompressor (float threshold = -20.0F, float ratio = 4.0F, float attackTime = 0.01F, float releaseTime = 0.1F, float makeupGain = 0.0F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				threshold = std::clamp(threshold, -60.0F, 0.0F);
				ratio = std::clamp(ratio, 1.0F, 20.0F);
				attackTime = std::clamp(attackTime, 0.0001F, 1.0F);
				releaseTime = std::clamp(releaseTime, 0.001F, 2.0F);
				makeupGain = std::clamp(makeupGain, 0.0F, 30.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Convert threshold to linear. */
				const auto thresholdLinear = std::pow(10.0F, threshold / 20.0F);

				/* Envelope follower coefficients. */
				const auto attackCoeff = std::exp(-1.0F / (attackTime * sampleRateFloat));
				const auto releaseCoeff = std::exp(-1.0F / (releaseTime * sampleRateFloat));

				/* Auto makeup gain calculation. */
				auto actualMakeupGain = makeupGain;

				if ( actualMakeupGain == 0.0F )
				{
					/* Estimate makeup gain based on threshold and ratio. */
					actualMakeupGain = -threshold * (1.0F - 1.0F / ratio) * 0.5F;
				}

				const auto makeupLinear = std::pow(10.0F, actualMakeupGain / 20.0F);

				/* Envelope state. */
				float envelope = 0.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto inputSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Envelope follower (peak detector). */
					const auto inputAbs = std::abs(inputSample);

					if ( inputAbs > envelope )
					{
						envelope = attackCoeff * envelope + (1.0F - attackCoeff) * inputAbs;
					}
					else
					{
						envelope = releaseCoeff * envelope;
					}

					/* Calculate gain reduction. */
					float gain = 1.0F;

					if ( envelope > thresholdLinear )
					{
						/* Apply compression. */
						const auto overThreshold = envelope / thresholdLinear;
						const auto compressedLevel = thresholdLinear * std::pow(overThreshold, 1.0F / ratio);
						gain = compressedLevel / envelope;
					}

					/* Apply gain and makeup. */
					const auto outputSample = inputSample * gain * makeupLinear;

					data[sampleIndex] = toSampleFormat(std::clamp(outputSample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies noise gate effect.
			 * @note Silences audio below a threshold (removes noise, creates choppy effects).
			 * @param threshold Threshold in dB (-80 to 0). Default -40.
			 * @param attackTime Attack time in seconds. Default 0.001.
			 * @param holdTime Hold time in seconds (keeps gate open after signal drops). Default 0.05.
			 * @param releaseTime Release time in seconds. Default 0.1.
			 * @return bool
			 */
			bool
			applyNoiseGate (float threshold = -40.0F, float attackTime = 0.001F, float holdTime = 0.05F, float releaseTime = 0.1F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				threshold = std::clamp(threshold, -80.0F, 0.0F);
				attackTime = std::clamp(attackTime, 0.0001F, 0.1F);
				holdTime = std::clamp(holdTime, 0.0F, 1.0F);
				releaseTime = std::clamp(releaseTime, 0.001F, 2.0F);

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto sampleRateFloat = static_cast< float >(m_wave.frequency());
				auto & data = m_wave.data();

				/* Convert threshold to linear. */
				const auto thresholdLinear = std::pow(10.0F, threshold / 20.0F);

				/* Time constants. */
				const auto attackCoeff = std::exp(-1.0F / (attackTime * sampleRateFloat));
				const auto releaseCoeff = std::exp(-1.0F / (releaseTime * sampleRateFloat));
				const auto holdSamples = static_cast< size_t >(holdTime * sampleRateFloat);

				/* Gate state. */
				float envelope = 0.0F;
				float gateGain = 0.0F;
				size_t holdCounter = 0;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto inputSample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Simple envelope follower. */
					const auto inputAbs = std::abs(inputSample);
					envelope = 0.9F * envelope + 0.1F * inputAbs;

					/* Gate logic. */
					if ( envelope > thresholdLinear )
					{
						/* Open the gate. */
						gateGain = attackCoeff * gateGain + (1.0F - attackCoeff) * 1.0F;
						holdCounter = holdSamples;
					}
					else if ( holdCounter > 0 )
					{
						/* Hold phase. */
						holdCounter--;
					}
					else
					{
						/* Release phase. */
						gateGain = releaseCoeff * gateGain;
					}

					data[sampleIndex] = toSampleFormat(inputSample * gateGain);
				}

				return true;
			}

			/**
			 * @brief Applies pitch shifter effect (simple time-domain pitch shift).
			 * @note Shifts the pitch up or down by a given number of semitones.
			 * @param semitones Pitch shift in semitones (-12 to +12). Default 0.
			 * @param mix Dry/wet mix (0.0 to 1.0). Default 1.0.
			 * @return bool
			 */
			bool
			applyPitchShift (float semitones = 0.0F, float mix = 1.0F) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				semitones = std::clamp(semitones, -12.0F, 12.0F);
				mix = std::clamp(mix, 0.0F, 1.0F);

				if ( std::abs(semitones) < 0.01F )
				{
					return true;  /* No shift needed. */
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto regionSampleCount = endSample - startSample;
				auto & data = m_wave.data();

				/* Calculate pitch ratio. */
				const auto pitchRatio = std::pow(2.0F, semitones / 12.0F);

				/* Create a copy of original data. */
				std::vector< precision_t > originalData(data.begin() + static_cast< ptrdiff_t >(startSample),
														data.begin() + static_cast< ptrdiff_t >(endSample));

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto localIndex = sampleIndex - startSample;
					const auto drySample = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());

					/* Calculate read position with pitch shift. */
					const auto readPos = static_cast< float >(localIndex) * pitchRatio;
					const auto readPosInt = static_cast< size_t >(readPos);
					const auto readPosFrac = readPos - static_cast< float >(readPosInt);

					float wetSample = 0.0F;

					if ( readPosInt < regionSampleCount - 1 )
					{
						if ( readPosInt < originalData.size() && readPosInt + 1 < originalData.size() )
						{
							const auto sample1 = static_cast< float >(originalData[readPosInt]) / static_cast< float >(std::numeric_limits< precision_t >::max());
							const auto sample2 = static_cast< float >(originalData[readPosInt + 1]) / static_cast< float >(std::numeric_limits< precision_t >::max());
							wetSample = sample1 + readPosFrac * (sample2 - sample1);
						}
					}

					const auto outputSample = drySample * (1.0F - mix) + wetSample * mix;
					data[sampleIndex] = toSampleFormat(std::clamp(outputSample, -1.0F, 1.0F));
				}

				return true;
			}

			/**
			 * @brief Applies sample rate reducer effect (lo-fi/retro sound).
			 * @note Reduces the effective sample rate for a crunchy, retro effect.
			 * @param factor Reduction factor (1 = no change, 2 = half rate, etc.). Default 4.
			 * @return bool
			 */
			bool
			applySampleRateReduce (int factor = 4) noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				factor = std::clamp(factor, 1, 32);

				if ( factor == 1 )
				{
					return true;  /* No reduction needed. */
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				auto & data = m_wave.data();

				/* Hold sample value. */
				precision_t holdSample = 0;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto localIndex = sampleIndex - startSample;

					if ( localIndex % static_cast< size_t >(factor) == 0 )
					{
						/* Sample and hold. */
						holdSample = data[sampleIndex];
					}
					else
					{
						/* Hold previous sample. */
						data[sampleIndex] = holdSample;
					}
				}

				return true;
			}

			/**
			 * @brief Reverses the region (plays backward).
			 * @return bool
			 */
			bool
			reverse () noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				const auto regionSampleCount = endSample - startSample;
				auto & data = m_wave.data();

				for ( size_t localIndex = 0; localIndex < regionSampleCount / 2; ++localIndex )
				{
					const auto sampleIndex = startSample + localIndex;
					const auto mirrorIndex = endSample - 1 - localIndex;
					std::swap(data[sampleIndex], data[mirrorIndex]);
				}

				return true;
			}

			/**
			 * @brief Normalizes the region to maximum amplitude.
			 * @return bool
			 */
			bool
			normalize () noexcept
			{
				if ( !m_wave.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", wave is not initialized !" "\n";

					return false;
				}

				const auto [startSample, endSample] = this->getEffectiveRange();
				auto & data = m_wave.data();

				/* Find the maximum absolute value in the region. */
				float maxAbs = 0.0F;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto sampleFloat = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());
					maxAbs = std::max(maxAbs, std::abs(sampleFloat));
				}

				if ( maxAbs < 0.0001F )
				{
					return true;
				}

				/* Scale all samples in the region. */
				const auto scale = 1.0F / maxAbs;

				for ( size_t sampleIndex = startSample; sampleIndex < endSample; ++sampleIndex )
				{
					const auto sampleFloat = static_cast< float >(data[sampleIndex]) / static_cast< float >(std::numeric_limits< precision_t >::max());
					data[sampleIndex] = toSampleFormat(sampleFloat * scale);
				}

				return true;
			}

			/**
			 * @brief Sets a working region to limit effect/generation to a portion of the buffer.
			 * @note This allows generating multiple notes in sequence without creating temporary buffers.
			 * @param offset The starting sample index of the region.
			 * @param length The number of samples in the region (0 = from offset to end).
			 */
			void
			setRegion (size_t offset, size_t length = 0) noexcept
			{
				m_regionOffset = offset;
				m_regionLength = length;
			}

			/**
			 * @brief Resets the working region to the full buffer.
			 */
			void
			resetRegion () noexcept
			{
				m_regionOffset = 0;
				m_regionLength = 0;
			}

			/**
			 * @brief Gets the current region offset.
			 * @return size_t The offset in samples.
			 */
			[[nodiscard]]
			size_t
			regionOffset () const noexcept
			{
				return m_regionOffset;
			}

			/**
			 * @brief Gets the current region length.
			 * @return size_t The length in samples (0 means full buffer from offset).
			 */
			[[nodiscard]]
			size_t
			regionLength () const noexcept
			{
				return m_regionLength;
			}

		private:

			/**
			 * @brief Computes the sum of harmonics for additive synthesis.
			 * @tparam N The number of harmonics.
			 * @param harmonics The array of harmonic definitions.
			 * @param phase The current phase (0.0 to 1.0).
			 * @param twoPi Pre-computed 2*PI constant.
			 * @return float The normalized sample value.
			 */
			template< size_t N >
			[[nodiscard]]
			static float
			computeHarmonics (const std::array< Harmonic, N > & harmonics, float phase, [[maybe_unused]] float twoPi) noexcept
			{
				float sample = 0.0F;
				float totalAmp = 0.0F;

				for ( const auto & h : harmonics )
				{
					/* Use fastSin() with phase in [0,1) range - ~10-20x faster than std::sin(). */
					sample += h.amplitude * fastSin(phase * h.multiplier);
					totalAmp += h.amplitude;
				}

				return sample / totalAmp;
			}

			/**
			 * @brief Gets the effective sample range considering the current region settings.
			 * @return std::pair< size_t, size_t > The start and end sample indices.
			 */
			[[nodiscard]]
			std::pair< size_t, size_t >
			getEffectiveRange () const noexcept
			{
				const auto totalSamples = m_wave.sampleCount();
				const auto start = std::min(m_regionOffset, totalSamples);
				const auto end = (m_regionLength == 0)
					? totalSamples
					: std::min(m_regionOffset + m_regionLength, totalSamples);

				return {start, end};
			}

			/**
			 * @brief Gets the effective sample count for the current region.
			 * @return size_t The number of samples in the effective range.
			 */
			[[nodiscard]]
			size_t
			getEffectiveSampleCount () const noexcept
			{
				const auto [start, end] = this->getEffectiveRange();
				return end - start;
			}

			/**
			 * @brief Converts a normalized float sample [-1.0, 1.0] to the target precision type.
			 * @param sample The normalized sample value.
			 * @return precision_t
			 */
			static precision_t
			toSampleFormat (float sample) noexcept
			{
				if constexpr ( std::is_floating_point_v< precision_t > )
				{
					return static_cast< precision_t >(sample);
				}
				else
				{
					/* Scale to integer range. */
					const auto maxVal = static_cast< float >(std::numeric_limits< precision_t >::max());
					return static_cast< precision_t >(sample * maxVal);
				}
			}

			Wave< precision_t > & m_wave;
			size_t m_regionOffset{0};
			size_t m_regionLength{0};
	};
}
