/*
 * src/Audio/Buffer.cpp
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

#include "Buffer.hpp"

/* Local inclusions. */
#include "Manager.hpp"
#include "Tracer.hpp"
#include "Utility.hpp"

namespace EmEn::Audio
{
	using namespace Libs;

	Buffer::Buffer () noexcept
	{
		if ( !Manager::isAudioSystemAvailable() )
		{
			*(this->identifierPointer()) =  1;

			return;
		}

		alFlushErrors();

		alGenBuffers(1, this->identifierPointer());

		if ( alGetErrors("alGenBuffers()", __FILE__, __LINE__) || this->identifier() <= 0 )
		{
			Tracer::error(ClassId, "Unable to create a buffer object !");
		}
	}

	Buffer::~Buffer ()
	{
		if ( this->isCreated() )
		{
			alDeleteBuffers(1, this->identifierPointer());
		}

		this->resetIdentifier();
	}

	bool
	Buffer::feedData (const WaveFactory::Wave< short int > & wave, size_t chunkIndex, size_t length) noexcept
	{
		if ( !wave.isValid() )
		{
			Tracer::error(ClassId, "Wave object is not usable to load in buffer.");

			return false;
		}

		if ( !Manager::isAudioSystemAvailable() )
		{
			return true;
		}

		ALenum format = 0;

		switch ( wave.channels() )
		{
			case WaveFactory::Channels::Mono :
				format = AL_FORMAT_MONO16;
				break;

			case WaveFactory::Channels::Stereo :
				format = AL_FORMAT_STEREO16;
				break;

			/*case 4 :
				format = alGetEnumValue("AL_FORMAT_QUAD16");
				break;

			case 6 :
				format = alGetEnumValue("AL_FORMAT_51CHN16");
				break;

			case 7 :
				format = alGetEnumValue("AL_FORMAT_61CHN16");
				break;

			case 8 :
				format = alGetEnumValue("AL_FORMAT_71CHN16");
				break;*/

			case WaveFactory::Channels::Invalid :
				Tracer::error(ClassId, "Unsupported number of channels.");

				return false;
		}

		if ( length > 0 )
		{
			const auto chunk = wave.chunk(chunkIndex, length);

			alBufferData(this->identifier(), format, wave.samplePointer(chunk.offset), static_cast< ALsizei >(chunk.bytes), static_cast< ALsizei >(wave.frequency()));
		}
		else
		{
			alBufferData(this->identifier(), format, wave.samplePointer(0), wave.bytes< ALsizei >(), static_cast< ALsizei >(wave.frequency()));
		}

		if ( alGetErrors("alBufferData()", __FILE__, __LINE__) )
		{
			Tracer::error(ClassId, "Unable to load data to OpenAL buffer !");

			return false;
		}

		return true;
	}

	int
	Buffer::frequency () const noexcept
	{
		auto value = -1;

		if ( Manager::isAudioSystemAvailable() )
		{
			if ( this->isCreated() )
			{
				alGetBufferi(this->identifier(), AL_FREQUENCY, &value);
			}
			else
			{
				Tracer::warning(ClassId, "Trying to get frequency from an unloaded audio source !");
			}
		}

		return value;
	}

	int
	Buffer::bits () const noexcept
	{
		auto value = -1;

		if ( Manager::isAudioSystemAvailable() )
		{
			if ( this->isCreated() )
			{
				alGetBufferi(this->identifier(), AL_BITS, &value);
			}
			else
			{
				Tracer::warning(ClassId, "Trying to get bits from an unloaded audio source !");
			}
		}

		return value;
	}

	int
	Buffer::channels () const noexcept
	{
		auto value = -1;

		if ( Manager::isAudioSystemAvailable() )
		{
			if ( this->isCreated() )
			{
				alGetBufferi(this->identifier(), AL_CHANNELS, &value);
			}
			else
			{
				Tracer::warning(ClassId, "Trying to get channels from an unloaded audio source !");
			}
		}

		return value;
	}

	int
	Buffer::size () const noexcept
	{
		auto value = -1;

		if ( Manager::isAudioSystemAvailable() )
		{
			if ( this->isCreated() )
			{
				alGetBufferi(this->identifier(), AL_SIZE, &value);
			}
			else
			{
				Tracer::warning(ClassId, "Trying to get size from an unloaded audio source !");
			}
		}

		return value;
	}
}
