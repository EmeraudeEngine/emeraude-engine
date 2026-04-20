/*
 * src/Libs/WaveFactory/FileFormatSNDFile.hpp
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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <iostream>
#include <type_traits>

#ifdef LIBSNDFILE_ENABLED
/* Third-party inclusions. */
#include "sndfile.h"
#endif

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"
#include "Wave.hpp"

namespace EmEn::Libs::WaveFactory
{
	/**
	 * @brief Class for reading and writing audio formats via libsndfile.
	 * @tparam precision_t The sample precision type. Default int16_t.
	 * @extends EmEn::Libs::WaveFactory::FileFormatInterface The base IO class.
	 * @note Supports WAV, FLAC, OGG, AIFF, and many other formats.
	 */
	template< typename precision_t = int16_t >
	requires (std::is_arithmetic_v< precision_t >)
	class FileFormatSNDFile final : public FileFormatInterface< precision_t >
	{
		public:

			FileFormatSNDFile () noexcept = default;

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::readStream() */
			[[nodiscard]]
			bool
			readStream (IO::ByteStream & /*stream*/, Wave< precision_t > & /*wave*/, const ReadOptions & /*options*/) noexcept override
			{
				std::cerr << "[WaveFactory::FileFormatSNDFile] readStream(), precision format not handled !\n";

				return false;
			}

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::writeStream() */
			[[nodiscard]]
			bool
			writeStream (IO::ByteStream & /*stream*/, const Wave< precision_t > & /*wave*/, const WriteOptions & /*options*/) const noexcept override
			{
				std::cerr << "[WaveFactory::FileFormatSNDFile] writeStream(), precision format not handled !\n";

				return false;
			}
	};

#ifdef LIBSNDFILE_ENABLED

	/**
	 * @brief Virtual I/O callbacks for libsndfile using ByteStream.
	 * @note These are static free functions matching the SF_VIRTUAL_IO signature.
	 * The user_data pointer is cast to IO::ByteStream*.
	 */
	namespace SNDFileCallbacks
	{
		inline sf_count_t
		getFileLength (void * userData) noexcept
		{
			auto * stream = static_cast< IO::ByteStream * >(userData);

			return static_cast< sf_count_t >(stream->size());
		}

		inline sf_count_t
		seek (sf_count_t offset, int whence, void * userData) noexcept
		{
			auto * stream = static_cast< IO::ByteStream * >(userData);

			return static_cast< sf_count_t >(stream->seek(static_cast< int64_t >(offset), whence));
		}

		inline sf_count_t
		read (void * ptr, sf_count_t count, void * userData) noexcept
		{
			auto * stream = static_cast< IO::ByteStream * >(userData);

			if ( stream->read(ptr, static_cast< size_t >(count)) )
			{
				return count;
			}

			return 0;
		}

		inline sf_count_t
		write (const void * ptr, sf_count_t count, void * userData) noexcept
		{
			auto * stream = static_cast< IO::ByteStream * >(userData);

			if ( stream->write(ptr, static_cast< size_t >(count)) )
			{
				return count;
			}

			return 0;
		}

		inline sf_count_t
		tell (void * userData) noexcept
		{
			auto * stream = static_cast< IO::ByteStream * >(userData);

			return static_cast< sf_count_t >(stream->tell());
		}
	}

	/**
	 * @brief Specialization for int16_t (16-bit PCM audio).
	 */
	template<>
	class FileFormatSNDFile< int16_t > final : public FileFormatInterface< int16_t >
	{
		public:

			FileFormatSNDFile () noexcept = default;

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::readStream() */
			[[nodiscard]]
			bool
			readStream (IO::ByteStream & stream, Wave< int16_t > & wave, const ReadOptions & /*options*/) noexcept override
			{
				if ( !stream.isOpen() )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] readStream(), stream is not open !\n";

					return false;
				}

				SF_VIRTUAL_IO virtualIO;
				virtualIO.get_filelen = SNDFileCallbacks::getFileLength;
				virtualIO.seek = SNDFileCallbacks::seek;
				virtualIO.read = SNDFileCallbacks::read;
				virtualIO.write = SNDFileCallbacks::write;
				virtualIO.tell = SNDFileCallbacks::tell;

				SF_INFO soundFileInfos;
				soundFileInfos.frames = 0;
				soundFileInfos.samplerate = 0;
				soundFileInfos.channels = 0;
				soundFileInfos.format = 0;
				soundFileInfos.sections = 0;
				soundFileInfos.seekable = 0;

				auto * file = sf_open_virtual(&virtualIO, SFM_READ, &soundFileInfos, &stream);

				if ( file == nullptr )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] readStream(), unable to open audio stream !\n";

					return false;
				}

				/* NOTE: Enable libsndfile clipping when converting float samples (Ogg/Vorbis, FLAC 24-bit,
				 * etc.) to int16_t. Without this, inter-sample peaks that go above +1.0 or below -1.0
				 * (common on transients like drum hits and vocal sibilants) wrap around int16_t range
				 * instead of being clipped, producing sharp audible clicks exactly at those peaks.
				 * SFC_SET_CLIPPING is a no-op for non-float input formats, so it's safe to apply here. */
				sf_command(file, SFC_SET_CLIPPING, nullptr, SF_TRUE);

				auto isDataValid = true;

				const auto samples = static_cast< size_t >(soundFileInfos.frames);
				const auto channels = toChannels(soundFileInfos.channels);
				const auto frequency = toFrequency(soundFileInfos.samplerate);

				if ( channels == Channels::Invalid )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] readStream(), invalid channels !\n";

					isDataValid = false;
				}

				if ( frequency == Frequency::Invalid )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] readStream(), invalid frequency !\n";

					isDataValid = false;
				}

				if constexpr ( WaveFactoryDebugEnabled )
				{
					std::cout <<
						"[WaveFactory::FileFormatSNDFile] Stream loaded.\n"
						"\t" "Frames (Samples) : " << soundFileInfos.frames << "\n"
						"\t" "Sample rates (Frequency) : " << soundFileInfos.samplerate << " Hz\n"
						"\t" "Duration : " << ( static_cast< float >(soundFileInfos.frames) / static_cast< float >(soundFileInfos.samplerate) ) << " seconds\n"
						"\t" "Channels : " << soundFileInfos.channels << "\n"
						"\t" "Format (Bits) : " << soundFileInfos.format << "\n"
						"\t" "Sections : " << soundFileInfos.sections << "\n"
						"\t" "Seekable : " << soundFileInfos.seekable << '\n';
				}

				if ( isDataValid )
				{
					if ( wave.initialize(samples, channels, frequency) )
					{
						/* NOTE: libsndfile decodes Ogg Vorbis (and FLAC > 16-bit) internally as floats.
						 * sf_readf_short() does a float→int16 conversion, and for lossy codecs the decoded
						 * floats can exceed [-1.0, +1.0] on transients (drum hits, vocal sibilants). On int16
						 * conversion without explicit clipping, those overshoots wrap the value, producing
						 * sharp clicks exactly at those peaks. SFC_SET_CLIPPING is documented to handle this,
						 * but the effect is not reliable across all codec paths. To be safe, we decode as
						 * float and perform the conversion + explicit clamp ourselves. sf_info.frames is an
						 * *estimate* for VBR formats; we trust the actual count returned by sf_readf_float. */
						std::vector< float > floatBuffer(static_cast< size_t >(soundFileInfos.frames) * soundFileInfos.channels);

						const auto actualRead = sf_readf_float(file, floatBuffer.data(), soundFileInfos.frames);

						if ( actualRead <= 0 )
						{
							std::cerr << "[WaveFactory::FileFormatSNDFile] readStream(), libsndfile returned no frames: " << sf_strerror(file) << "\n";

							isDataValid = false;
						}
						else
						{
							if ( actualRead < soundFileInfos.frames )
							{
								/* Trim the wave to the actual frame count (estimate was optimistic). */
								wave.initialize(static_cast< size_t >(actualRead), channels, frequency);
							}

							/* Manual float→int16 conversion with hard clipping at ±1.0. */
							const auto sampleCount = static_cast< size_t >(actualRead) * soundFileInfos.channels;
							auto * out = wave.data().data();

							for ( size_t i = 0; i < sampleCount; ++i )
							{
								float s = floatBuffer[i];

								if ( s > 1.0F )
								{
									s = 1.0F;
								}
								else if ( s < -1.0F )
								{
									s = -1.0F;
								}

								out[i] = static_cast< int16_t >(s * 32767.0F);
							}
						}
					}
					else
					{
						std::cerr << "[WaveFactory::FileFormatSNDFile] readStream(), unable to allocate memory !\n";

						isDataValid = false;
					}
				}

				sf_close(file);

				return isDataValid;
			}

			/** @copydoc EmEn::Libs::WaveFactory::FileFormatInterface::writeStream() */
			[[nodiscard]]
			bool
			writeStream (IO::ByteStream & stream, const Wave< int16_t > & wave, const WriteOptions & options) const noexcept override
			{
				if ( !stream.isOpen() )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] writeStream(), stream is not open !\n";

					return false;
				}

				SF_VIRTUAL_IO virtualIO;
				virtualIO.get_filelen = SNDFileCallbacks::getFileLength;
				virtualIO.seek = SNDFileCallbacks::seek;
				virtualIO.read = SNDFileCallbacks::read;
				virtualIO.write = SNDFileCallbacks::write;
				virtualIO.tell = SNDFileCallbacks::tell;

				int format = SF_FORMAT_PCM_16;

				switch ( options.format )
				{
					case AudioFormat::WAV :
						format |= SF_FORMAT_WAV;
						break;

					case AudioFormat::FLAC :
						format |= SF_FORMAT_FLAC;
						break;

					case AudioFormat::OGG :
						format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
						break;
				}

				SF_INFO infos;
				infos.channels = static_cast< int >(wave.channels());
				infos.samplerate = static_cast< int >(wave.frequency());
				infos.format = format;

				auto * file = sf_open_virtual(&virtualIO, SFM_WRITE, &infos, &stream);

				if ( file == nullptr )
				{
					std::cerr << "[WaveFactory::FileFormatSNDFile] writeStream(), unable to open audio stream for writing !\n";

					return false;
				}

				sf_writef_short(file, wave.data().data(), static_cast< sf_count_t >(wave.sampleCount()));

				sf_close(file);

				return true;
			}
	};
#endif
}