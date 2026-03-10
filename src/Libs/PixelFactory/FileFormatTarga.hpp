/*
 * src/Libs/PixelFactory/FileFormatTarga.hpp
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
#include <array>
#include <cstdint>
#include <iostream>
#include <string>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/IO/ByteStream.hpp"
#include "Processor.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief Class for read and write Targa format via byte streams.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 * @extends EmEn::Libs::PixelFactory::FileFormatInterface The base IO class.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	class FileFormatTarga final : public FileFormatInterface< pixel_data_t, dimension_t >
	{
		public:

			FileFormatTarga () noexcept = default;

			/** @copydoc EmEn::Libs::PixelFactory::FileFormatInterface::readStream() */
			[[nodiscard]]
			bool
			readStream (IO::ByteStream & stream, Pixmap< pixel_data_t, dimension_t > & pixmap) noexcept override
			{
				pixmap.clear();

				auto RLE = false;

				Header fileHeader{};

				std::array< void *, 12 > ptr = {
					&fileHeader.idCharCount,
					&fileHeader.colorMapType,
					&fileHeader.imageTypeCode,
					&fileHeader.colorMapOrigin,
					&fileHeader.colorMapLength,
					&fileHeader.colorMapEntrySize,
					&fileHeader.xOrigin,
					&fileHeader.yOrigin,
					&fileHeader.width,
					&fileHeader.height,
					&fileHeader.imagePixelSize,
					&fileHeader.imageDescriptorByte
				};

				std::array< uint32_t, 12 > size = {1, 1, 1, 2, 2, 1, 2, 2, 2, 2, 1, 1};

				/* Read the TARGA header, field by field. */
				for ( auto i = 0U; i < 12; i++ )
				{
					if ( !stream.read(ptr.at(i), size.at(i)) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", unable to read the Targa header !" "\n";

						return false;
					}
				}

				if constexpr ( PixelFactoryDebugEnabled )
				{
					std::cout <<
						"[TARGA_DEBUG] Reading header.\n" <<
						"\t" "idCharCount : " << static_cast< int >(fileHeader.idCharCount) << "\n"
						"\t" "colorMapType : " << static_cast< int >(fileHeader.colorMapType) << "\n"
						"\t" "imageTypeCode : " << static_cast< int >(fileHeader.imageTypeCode) << "\n"
						"\t" "colorMapOrigin : " << static_cast< int >(fileHeader.colorMapOrigin) << "\n"
						"\t" "colorMapLength : " << static_cast< int >(fileHeader.colorMapLength) << "\n"
						"\t" "colorMapEntrySize : " << static_cast< int >(fileHeader.colorMapEntrySize) << "\n"
						"\t" "xOrigin : " << static_cast< int >(fileHeader.xOrigin) << "\n"
						"\t" "yOrigin : " << static_cast< int >(fileHeader.yOrigin) << "\n"
						"\t" "width : " << static_cast< int >(fileHeader.width) << "\n"
						"\t" "height : " << static_cast< int >(fileHeader.height) << "\n"
						"\t" "imagePixelSize : " << static_cast< int >(fileHeader.imagePixelSize) << "\n"
						"\t" "imageDescriptorByte : " << fileHeader.imageDescriptorByte << '\n';
				}

				auto pixmapAllocated = false;

				switch ( fileHeader.imageTypeCode )
				{
					/* 256 colors index */
					case 9 : /* 8bits (RLE) */
						RLE = true;

						[[fallthrough]];

					case 1 : /* 8bits */
						pixmapAllocated = pixmap.initialize(fileHeader.width, fileHeader.height, ChannelMode::RGB);
						break;

					/* Composite */
					case 10 : /* 16bits, 24bits, 32bits BGR(RLE) */
						RLE = true;

						[[fallthrough]];

					case 2 : /* 16bits, 24bits, 32bits BGR */
						pixmapAllocated = pixmap.initialize(fileHeader.width, fileHeader.height, ( fileHeader.imagePixelSize == 32 ) ? ChannelMode::RGBA : ChannelMode::RGB);
						break;

					/* Grayscale Targa file */
					case 11 : /* 8bits, 16bits Grayscale(RLE) */
						RLE = true;

						[[fallthrough]];

					case 3 : /* 8bits, 16bits Grayscale */
						pixmapAllocated = pixmap.initialize(fileHeader.width, fileHeader.height, ChannelMode::Grayscale);
						break;

					case 32 :
					case 33 :
						std::cerr << __PRETTY_FUNCTION__ << ", unhandled type of Targa file !" "\n";
						break;

					case 0 :
					default:
						std::cerr << __PRETTY_FUNCTION__ << ", no pixel data !" "\n";

						return false;
				}

				if ( !pixmapAllocated )
				{
					return false;
				}

				/* Store identification. */
				if ( fileHeader.idCharCount > 0 )
				{
					std::string identification;
					identification.resize(fileHeader.idCharCount + 1, '\0');

					if ( !stream.read(identification.data(), fileHeader.idCharCount) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", unable to read the Targa identification !" "\n";

						return false;
					}
				}

				/* Load data. */
				if ( RLE )
				{
					const auto bytesPerPixel = fileHeader.imagePixelSize / 8;
					const auto totalPixels = static_cast< size_t >(fileHeader.width) * static_cast< size_t >(fileHeader.height);
					auto & pixmapData = pixmap.data();
					size_t pixelIndex = 0;

					while ( pixelIndex < totalPixels )
					{
						uint8_t packetHeader = 0;

						if ( !stream.read(&packetHeader, 1) )
						{
							std::cerr << __PRETTY_FUNCTION__ << ", unable to read RLE packet header !" "\n";

							return false;
						}

						const bool isRLEPacket = (packetHeader & 0x80) != 0;
						const auto pixelCount = static_cast< size_t >((packetHeader & 0x7F) + 1);

						if ( isRLEPacket )
						{
							std::array< uint8_t, 4 > pixel{0, 0, 0, 255};

							if ( !stream.read(pixel.data(), bytesPerPixel) )
							{
								std::cerr << __PRETTY_FUNCTION__ << ", unable to read RLE pixel data !" "\n";

								return false;
							}

							for ( size_t i = 0; i < pixelCount && pixelIndex < totalPixels; ++i, ++pixelIndex )
							{
								const auto offset = pixelIndex * pixmap.template colorCount< size_t >();

								for ( size_t c = 0; c < pixmap.template colorCount< size_t >(); ++c )
								{
									pixmapData[offset + c] = pixel[c];
								}
							}
						}
						else
						{
							for ( size_t i = 0; i < pixelCount && pixelIndex < totalPixels; ++i, ++pixelIndex )
							{
								std::array< uint8_t, 4 > pixel{0, 0, 0, 255};

								if ( !stream.read(pixel.data(), bytesPerPixel) )
								{
									std::cerr << __PRETTY_FUNCTION__ << ", unable to read raw pixel data !" "\n";

									return false;
								}

								const auto offset = pixelIndex * pixmap.template colorCount< size_t >();

								for ( size_t c = 0; c < pixmap.template colorCount< size_t >(); ++c )
								{
									pixmapData[offset + c] = pixel[c];
								}
							}
						}
					}
				}
				else
				{
					if ( !stream.read(pixmap.data().data(), pixmap.bytes()) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", unable to read the Targa data !" "\n";

						return false;
					}
				}

				/* Normalize to canonical top-left origin.
				 * TGA yOrigin == 0 means bottom-left origin, needs vertical flip. */
				if ( fileHeader.yOrigin == 0 )
				{
					pixmap = Processor< pixel_data_t, dimension_t >::mirror(pixmap, MirrorMode::X);
				}

				/* Convert BGR to RGB format. */
				if ( pixmap.colorCount() > 1 )
				{
					pixmap = Processor< pixel_data_t, dimension_t >::swapChannels(pixmap);
				}

				return true;
			}

			/** @copydoc EmEn::Libs::PixelFactory::FileFormatInterface::writeStream() */
			[[nodiscard]]
			bool
			writeStream (IO::ByteStream & stream, const Pixmap< pixel_data_t, dimension_t > & pixmap, const WriteOptions & options = {}) const noexcept override
			{
				if ( !pixmap.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", pixmap parameter is invalid !" "\n";

					return false;
				}

				const auto useRLE = options.targa.rleCompression;

				Header fileHeader{};

				fileHeader.idCharCount = 0;
				/* TGA default: bottom-left origin. */
				fileHeader.yOrigin = 0;
				fileHeader.width = static_cast< uint16_t >(pixmap.width());
				fileHeader.height = static_cast< uint16_t >(pixmap.height());
				fileHeader.imagePixelSize = static_cast< uint8_t >(pixmap.bitPerPixel());
				fileHeader.imageDescriptorByte = (pixmap.channelMode() == ChannelMode::RGBA || pixmap.channelMode() == ChannelMode::GrayscaleAlpha) ? 8 : 0;

				switch ( pixmap.channelMode() )
				{
					case ChannelMode::Grayscale :
						fileHeader.imageTypeCode = useRLE ? 11 : 3;
						break;

					case ChannelMode::RGB :
					case ChannelMode::RGBA :
						fileHeader.imageTypeCode = useRLE ? 10 : 2;
						break;

					case ChannelMode::GrayscaleAlpha :
					default:
						std::cerr << __PRETTY_FUNCTION__ << ", unhandled color channel format to write a Targa image." "\n";

						return false;
				}

				std::array< void *, 12 > ptr = {
					&fileHeader.idCharCount,
					&fileHeader.colorMapType,
					&fileHeader.imageTypeCode,
					&fileHeader.colorMapOrigin,
					&fileHeader.colorMapLength,
					&fileHeader.colorMapEntrySize,
					&fileHeader.xOrigin,
					&fileHeader.yOrigin,
					&fileHeader.width,
					&fileHeader.height,
					&fileHeader.imagePixelSize,
					&fileHeader.imageDescriptorByte
				};

				std::array< uint32_t, 12 > size = {1, 1, 1, 2, 2, 1, 2, 2, 2, 2, 1, 1};

				if constexpr ( PixelFactoryDebugEnabled )
				{
					std::cout <<
						"[TARGA_DEBUG] Writing header.\n" <<
						"\t" "idCharCount : " << static_cast< int >(fileHeader.idCharCount) << "\n"
						"\t" "colorMapType : " << static_cast< int >(fileHeader.colorMapType) << "\n"
						"\t" "imageTypeCode : " << static_cast< int >(fileHeader.imageTypeCode) << "\n"
						"\t" "colorMapOrigin : " << static_cast< int >(fileHeader.colorMapOrigin) << "\n"
						"\t" "colorMapLength : " << static_cast< int >(fileHeader.colorMapLength) << "\n"
						"\t" "colorMapEntrySize : " << static_cast< int >(fileHeader.colorMapEntrySize) << "\n"
						"\t" "xOrigin : " << static_cast< int >(fileHeader.xOrigin) << "\n"
						"\t" "yOrigin : " << static_cast< int >(fileHeader.yOrigin) << "\n"
						"\t" "width : " << static_cast< int >(fileHeader.width) << "\n"
						"\t" "height : " << static_cast< int >(fileHeader.height) << "\n"
						"\t" "imagePixelSize : " << static_cast< int >(fileHeader.imagePixelSize) << "\n"
						"\t" "imageDescriptorByte : " << fileHeader.imageDescriptorByte << '\n';
				}

				/* Write the Targa header, field by field. */
				for ( auto i = 0U; i < 12; i++ )
				{
					if ( !stream.write(ptr.at(i), size.at(i)) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", unable to write the Targa header !" "\n";

						return false;
					}
				}

				/* Prepare pixmap: RGB->BGR and orientation for TGA convention. */
				Pixmap< pixel_data_t, dimension_t > processedPixmap;

				if ( pixmap.colorCount() > 1 )
				{
					processedPixmap = Processor< pixel_data_t, dimension_t >::swapChannels(pixmap);

					/* TGA expects bottom-left origin. Flip unless input is already bottom-left. */
					if ( !options.invertYAxis )
					{
						processedPixmap = Processor< pixel_data_t, dimension_t >::mirror(processedPixmap, MirrorMode::X);
					}
				}
				else
				{
					if ( options.invertYAxis )
					{
						processedPixmap = pixmap;
					}
					else
					{
						processedPixmap = Processor< pixel_data_t, dimension_t >::mirror(pixmap, MirrorMode::X);
					}
				}

				/* Write data. */
				if ( useRLE )
				{
					if ( !writeRLEData(stream, processedPixmap) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", unable to write the Targa RLE data !" "\n";

						return false;
					}
				}
				else
				{
					if ( !stream.write(processedPixmap.data().data(), processedPixmap.bytes()) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", unable to write the Targa data !" "\n";

						return false;
					}
				}

				return true;
			}

		private:

			/**
			 * @brief Writes pixmap data using RLE compression.
			 * @param stream Output byte stream.
			 * @param pixmap The source pixmap to compress.
			 * @return bool True if successful.
			 */
			[[nodiscard]]
			static
			bool
			writeRLEData (IO::ByteStream & stream, const Pixmap< pixel_data_t, dimension_t > & pixmap) noexcept
			{
				const auto & pixmapData = pixmap.data();
				const auto totalPixels = static_cast< size_t >(pixmap.width()) * static_cast< size_t >(pixmap.height());
				const auto bytesPerPixel = pixmap.template colorCount< size_t >();
				size_t currentPixel = 0;

				while ( currentPixel < totalPixels )
				{
					size_t runLength = 1;
					constexpr size_t maxRunLength{128};
					const auto currentOffset = currentPixel * bytesPerPixel;

					/* Check for RLE run (repeated pixels). */
					bool isRLE = false;

					if ( currentPixel + 1 < totalPixels )
					{
						const auto nextOffset = (currentPixel + 1) * bytesPerPixel;
						bool pixelsMatch = true;

						for ( size_t c = 0; c < bytesPerPixel; ++c )
						{
							if ( pixmapData[currentOffset + c] != pixmapData[nextOffset + c] )
							{
								pixelsMatch = false;
								break;
							}
						}

						if ( pixelsMatch )
						{
							isRLE = true;

							while ( runLength < maxRunLength && currentPixel + runLength < totalPixels )
							{
								const auto checkOffset = (currentPixel + runLength) * bytesPerPixel;
								bool stillMatching = true;

								for ( size_t c = 0; c < bytesPerPixel; ++c )
								{
									if ( pixmapData[currentOffset + c] != pixmapData[checkOffset + c] )
									{
										stillMatching = false;
										break;
									}
								}

								if ( !stillMatching )
								{
									break;
								}

								++runLength;
							}
						}
					}

					if ( isRLE )
					{
						const auto packetHeader = static_cast< uint8_t >(0x80 | (runLength - 1));

						if ( !stream.write(&packetHeader, 1) )
						{
							return false;
						}

						if ( !stream.write(&pixmapData[currentOffset], bytesPerPixel) )
						{
							return false;
						}
					}
					else
					{
						while ( runLength < maxRunLength && currentPixel + runLength < totalPixels )
						{
							const auto checkOffset = (currentPixel + runLength) * bytesPerPixel;
							const auto nextCheckOffset = (currentPixel + runLength + 1) * bytesPerPixel;

							if ( currentPixel + runLength + 1 >= totalPixels )
							{
								++runLength;
								break;
							}

							bool nextPixelsMatch = true;

							for ( size_t c = 0; c < bytesPerPixel; ++c )
							{
								if ( pixmapData[checkOffset + c] != pixmapData[nextCheckOffset + c] )
								{
									nextPixelsMatch = false;
									break;
								}
							}

							if ( nextPixelsMatch )
							{
								break;
							}

							++runLength;
						}

						const auto packetHeader = static_cast< uint8_t >(runLength - 1);

						if ( !stream.write(&packetHeader, 1) )
						{
							return false;
						}

						if ( !stream.write(&pixmapData[currentOffset], bytesPerPixel * runLength) )
						{
							return false;
						}
					}

					currentPixel += runLength;
				}

				return true;
			}

			struct Header
			{
				uint8_t idCharCount = 0;
				uint8_t colorMapType = 0;
				uint8_t imageTypeCode = 0;
				uint16_t colorMapOrigin = 0;
				uint16_t colorMapLength = 0;
				uint8_t colorMapEntrySize = 0;
				uint16_t xOrigin = 0;
				uint16_t yOrigin = 0;
				uint16_t width = 0;
				uint16_t height = 0;
				uint8_t imagePixelSize = 0;
				uint8_t imageDescriptorByte = 0;
			};
	};
}
