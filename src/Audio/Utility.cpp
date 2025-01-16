/*
 * src/Audio/Utility.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Utility.hpp"

/* STL inclusions */
#include <iostream>
#include <sstream>
#include <string>

/* Third-party inclusions. */
#include "AL/al.h"
#include "AL/alc.h"
#include "AL/alext.h"

namespace Emeraude::Audio
{
	bool
	alGetErrors (const std::string & lastFunctionCalled, const std::string & filename, int line) noexcept
	{
		auto hasError = false;

		/* Load the first error, if exists... */
		ALenum error = 0;

		while ( (error = alGetError()) != AL_NO_ERROR )
		{
			std::cerr << "[OpenAL AC API][" << lastFunctionCalled << ":" << filename << '@' << line << "] ";

			switch ( error )
			{
				case AL_INVALID_NAME :
					std::cerr << "AL_INVALID_NAME : ";
					break;

				case AL_INVALID_ENUM :
					std::cerr << "AL_INVALID_ENUM : ";
					break;

				case AL_INVALID_VALUE :
					std::cerr << "AL_INVALID_VALUE : ";
					break;

				case AL_INVALID_OPERATION :
					std::cerr << "AL_INVALID_OPERATION : ";
					break;

				case AL_OUT_OF_MEMORY :
					std::cerr << "AL_OUT_OF_MEMORY : ";
					break;

				default :
					std::cerr << error << " : ";
					break;
			}

			std::cerr << alGetString(error) << '\n';

			hasError = true;
		}

		return hasError;
	}

	void
	alFlushErrors () noexcept
	{
		auto error = alGetError();

		while ( error != AL_NO_ERROR )
		{
			error = alGetError();
		}
	}

	bool
	alcGetErrors (ALCdevice * device, const std::string & lastFunctionCalled, const std::string & filename, int line) noexcept
	{
		auto hasError = false;

		/* Load the first error, if exists... */
		ALenum error = 0;

		while ( (error = alcGetError(device)) != ALC_NO_ERROR )
		{
			std::cerr << "[OpenAL ALC API][" << lastFunctionCalled << ":" << filename << ':' << line << "] : ";

			switch ( error )
			{
				case ALC_INVALID_DEVICE :
					std::cerr << "Invalid device, ";
					break;

				case ALC_INVALID_CONTEXT :
					std::cerr << "Invalid context, ";
					break;

				case ALC_INVALID_ENUM :
					std::cerr << "Invalid enumeration, ";
					break;

				case ALC_INVALID_VALUE :
					std::cerr << "Invalid value, ";
					break;

				case ALC_OUT_OF_MEMORY :
					std::cerr << "Out of memory, ";
					break;

				default :
					std::cerr << "Unknown, ";
					break;
			}

			std::cerr << alGetString(error) << '\n';

			hasError = true;
		}

		return hasError;
	}

	void
	alcFlushErrors (ALCdevice * device) noexcept
	{
		auto error = alcGetError(device);

		while ( error != ALC_NO_ERROR )
		{
			error = alcGetError(device);
		}
	}

	std::string
	alcKeyToLabel (ALCint key) noexcept
	{
		std::stringstream output;

		switch ( key )
		{
			case ALC_MAJOR_VERSION :
				output << "ALC Major version";
				break;

			case ALC_MINOR_VERSION :
				output << "ALC Minor version";
				break;

			case ALC_EFX_MAJOR_VERSION :
				output << "EFX Major version";
				break;

			case ALC_EFX_MINOR_VERSION :
				output << "EFX Minor version";
				break;

			/** Context attribute: <int> Hz. */
			case ALC_FREQUENCY :
				output << "Playback frequency (Hz)";
				break;

			/** Context attribute: <int> Hz. */
			case ALC_REFRESH :
				output << "API refresh rate (Hz)";
				break;

			/** Context attribute: AL_TRUE or AL_FALSE. */
			case ALC_SYNC :
				output << "API sync state (ON|OFF)";
				break;

			/** Context attribute: <int> requested Mono (3D) Sources. */
			case ALC_MONO_SOURCES :
				output << "Mono source count";
				break;

			/** Context attribute: <int> requested Stereo Sources. */
			case ALC_STEREO_SOURCES :
				output << "Stereo source count";
				break;

			case ALC_MAX_AUXILIARY_SENDS :
				output << "Max auxiliary sends";
				break;

			case ALC_CAPTURE_SAMPLES :
				output << "Capture samples (Hz)";
				break;

			case ALC_FORMAT_CHANNELS_SOFT :
				output << "Format channels";
				break;

			case ALC_FORMAT_TYPE_SOFT :
				output << "Format type";
				break;

			case 0x1997 : //ALC_AMBISONIC_LAYOUT_SOFT :
				output << "Ambisonic layout";
				break;

			case 0x1998 : //ALC_AMBISONIC_SCALING_SOFT :
				output << "Ambisonic scaling";
				break;

			case 0x1999 : //ALC_AMBISONIC_ORDER_SOFT :
				output << "Ambisonic order";
				break;

			case 0x199B : //ALC_MAX_AMBISONIC_ORDER_SOFT :
				output << "Max ambisonic order";
				break;

			case ALC_HRTF_SOFT :
				output << "HRTF (ON|OFF)";
				break;

			case ALC_HRTF_STATUS_SOFT :
				output << "HRTF status";
				break;

			case ALC_NUM_HRTF_SPECIFIERS_SOFT :
				output << "Num HRTF specifiers";
				break;

			case ALC_OUTPUT_LIMITER_SOFT :
				output << "Output limiter (ON|OFF)";
				break;

			case 0x19AC : //ALC_OUTPUT_MODE_SOFT :
				output << "Output mode";
				break;

			case ALC_CONNECTED :
				output << "Connected (ON|OFF)";
				break;

			case ALC_ATTRIBUTES_SIZE:
			case ALC_ALL_ATTRIBUTES:
				output << "Not a relevant ALC property";
				break;

			default :
				output << "Unknown ALC key (" << key << ')';
				break;
		}

		return output.str();
	}
}
