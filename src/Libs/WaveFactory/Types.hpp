/*
 * src/Libs/WaveFactory/Types.hpp
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
#include <cstdint>
#include <iostream>

/* Forward declarations. */
struct tsf;

namespace EmEn::Libs::WaveFactory
{
	/**
	 * @brief The channel mode enumeration.
	 */
	enum class Channels : size_t
	{
		Invalid = 0,
		Mono = 1,
		Stereo = 2
	};

	/**
	 * @brief The channel index enumeration.
	 */
	enum class Channel : size_t
	{
		Mono = 0,
		Left = 0,
		Right = 1
	};

	/**
	 * @brief Standard PCM frequency enumeration.
	 */
	enum class Frequency
	{
		Invalid = 0,
		PCM4000Hz = 4000,
		PCM8000Hz = 8000,
		PCM11025Hz = 11025,
		PCM16000Hz = 16000,
		PCM22050Hz = 22050,
		PCM32000Hz = 32000,
		PCM44100Hz = 44100,
		PCM48000Hz = 48000,
		PCM96000Hz = 96000,
		PCM192000Hz = 192000
	};

	/**
	 * @brief Supported audio output container formats.
	 */
	enum class AudioFormat : uint8_t
	{
		WAV,
		FLAC,
		OGG
	};

	struct Chunk
	{
		size_t offset{0};
		size_t bytes{0};
	};

	/**
	 * @brief Options for reading/decoding audio data.
	 * @note synthesisFrequency and soundfont are used by procedural format handlers (JSON, MIDI).
	 */
	struct ReadOptions
	{
		Frequency synthesisFrequency{Frequency::PCM48000Hz};
		tsf * soundfont{nullptr};
	};

	/**
	 * @brief Options for writing/encoding audio data.
	 * @note The format field is used by StreamIO to specify the output container
	 * when no file extension is available. FileIO infers format from the file extension.
	 */
	struct WriteOptions
	{
		AudioFormat format{AudioFormat::WAV};
	};

	/**
	 * @brief Converts a channel count number to a corresponding enumeration.
	 * @param channelsCount The channel count.
	 * @return Channels
	 */
	[[nodiscard]]
	inline
	Channels
	toChannels (int channelsCount) noexcept
	{
		switch ( channelsCount )
		{
			case 1 :
				return Channels::Mono;

			case 2 :
				return Channels::Stereo;

			default:
				std::cerr << __PRETTY_FUNCTION__ << ", " << channelsCount << " channels is not handled !" "\n";

				return Channels::Invalid;
		}
	}

	/**
	 * @brief Converts a frequency number to a corresponding enumeration.
	 * @param frequency The frequency in Hz.
	 * @return Frequency
	 */
	[[nodiscard]]
	inline
	Frequency
	toFrequency (int frequency) noexcept
	{
		switch ( frequency )
		{
			case 4000 :
				return Frequency::PCM4000Hz;

			case 8000 :
				return Frequency::PCM8000Hz;

			case 11025 :
				return Frequency::PCM11025Hz;

			case 16000 :
				return Frequency::PCM16000Hz;

			case 22050 :
				return Frequency::PCM22050Hz;

			case 32000 :
				return Frequency::PCM32000Hz;

			case 44100 :
				return Frequency::PCM44100Hz;

			case 48000 :
				return Frequency::PCM48000Hz;

			case 96000 :
				return Frequency::PCM96000Hz;

			case 192000 :
				return Frequency::PCM192000Hz;

			default:
				std::cerr << __PRETTY_FUNCTION__ << ", the frequency " << frequency << " is not standard !" "\n";

				return Frequency::Invalid;
		}
	}
}