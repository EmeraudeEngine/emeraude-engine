/*
 * src/Libs/PixelFactory/FileFormatTarga.hpp
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

#pragma once

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <array>
#include <iostream>
#include <fstream>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Processor.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief Class for read and write Targa format.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 * @extends EmEn::Libs::PixelFactory::FileFormatInterface The base IO class.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	class FileFormatTarga final : public FileFormatInterface< pixel_data_t, dimension_t >
	{
		public:

			/**
			 * @brief Constructs a Targa format IO.
			 */
			FileFormatTarga () noexcept = default;

			/**
			 * @brief Enables or disables RLE compression for writing.
			 * @param enabled True to enable RLE compression, false to disable.
			 */
			void
			setRLECompression (bool enabled) noexcept
			{
				m_useRLE = enabled;
			}

			/**
			 * @brief Returns whether RLE compression is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRLECompressionEnabled () const noexcept
			{
				return m_useRLE;
			}

			/** @copydoc EmEn::Libs::PixelFactory::FileFormatInterface::readFile() */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, Pixmap< pixel_data_t, dimension_t > & pixmap) noexcept override
			{
				pixmap.clear();

				std::ifstream file{filepath, std::ios::binary};

				if ( !file.is_open() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unable to read the Targa file " << filepath << " !" "\n";

					return false;
				}

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

				/* Read out the TARGA header, byte to byte */
				for ( auto i = 0U; i < 12; i++ )
				{
					if ( !file.read(static_cast< char * >(ptr.at(i)), size.at(i)) )
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
						"\t" "imageDescriptorByte : " << fileHeader.imageDescriptorByte  << '\n';
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

					case 32 : /* Compressed color-mapped data, using Huffman, Delta, and run-length encoding. */
					case 33 : /* Compressed color-mapped data, using Huffman, Delta, and run-length encoding.  4-pass quadtree-type process.*/
						std::cerr << __PRETTY_FUNCTION__ << ", unhandled type of Targa file !" "\n";
						break;

					case 0 : /* No image data included. */
					default:
						std::cerr << __PRETTY_FUNCTION__ << ", no pixel data !" "\n";

						return false;
				}

				/* Memory allocation. */
				if ( !pixmapAllocated )
				{
					return false;
				}

				/* Store identification. */
				if ( fileHeader.idCharCount > 0 )
				{
					std::string identification;
					identification.resize(fileHeader.idCharCount + 1, '\0');

					if ( !file.read(identification.data(), fileHeader.idCharCount) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", unable to read the Targa identification !" "\n";

						return false;
					}
				}

				/* Load data. */
				if ( RLE )
				{
					/* RLE decompression */
					const auto bytesPerPixel = fileHeader.imagePixelSize / 8;
					const auto totalPixels = static_cast< size_t >(fileHeader.width) * static_cast< size_t >(fileHeader.height);
					auto & pixmapData = pixmap.data();
					size_t pixelIndex = 0;

					while ( pixelIndex < totalPixels )
					{
						uint8_t packetHeader = 0;

						if ( !file.read(reinterpret_cast< char * >(&packetHeader), 1) )
						{
							std::cerr << __PRETTY_FUNCTION__ << ", unable to read RLE packet header !" "\n";

							return false;
						}

						const bool isRLEPacket = (packetHeader & 0x80) != 0;
						const auto pixelCount = static_cast< size_t >((packetHeader & 0x7F) + 1);

						if ( isRLEPacket )
						{
							/* RLE packet: read one pixel and repeat it */
							std::array< uint8_t, 4 > pixel{0, 0, 0, 255};

							if ( !file.read(reinterpret_cast< char * >(pixel.data()), bytesPerPixel) )
							{
								std::cerr << __PRETTY_FUNCTION__ << ", unable to read RLE pixel data !" "\n";

								return false;
							}

							/* Repeat the pixel */
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
							/* Raw packet: read pixels directly */
							for ( size_t i = 0; i < pixelCount && pixelIndex < totalPixels; ++i, ++pixelIndex )
							{
								std::array< uint8_t, 4 > pixel{0, 0, 0, 255};

								if ( !file.read(reinterpret_cast< char * >(pixel.data()), bytesPerPixel) )
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

					/* Checks the Y-Axis orientation. */
					if ( this->invertYAxis() )
					{
						/* NOTE: Origin is top-left in TGA file. */
						if ( fileHeader.yOrigin > 0 )
						{
							pixmap = Processor< uint8_t >::mirror(pixmap, MirrorMode::X);
						}
					}
					else
					{
						/* NOTE: Origin is bottom-left in TGA file. */
						if ( fileHeader.yOrigin == 0 )
						{
							pixmap = Processor< uint8_t >::mirror(pixmap, MirrorMode::X);
						}
					}

					/* Convert BGR to RGB format. */
					if ( pixmap.colorCount() > 1 )
					{
						pixmap = Processor< uint8_t >::swapChannels(pixmap);
					}
				}
				else
				{
					if ( !file.read(reinterpret_cast< char * >(pixmap.data().data()), pixmap.bytes()) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", unable to read the Targa data !" "\n";

						return false;
					}

					/* Checks the Y-Axis orientation. */
					if ( this->invertYAxis() )
					{
						/* NOTE: Origin is top-left in TGA file. */
						if ( fileHeader.yOrigin > 0 )
						{
							pixmap = Processor< uint8_t >::mirror(pixmap, MirrorMode::X);
						}
					}
					else
					{
						/* NOTE: Origin is bottom-left in TGA file. */
						if ( fileHeader.yOrigin == 0 )
						{
							pixmap = Processor< uint8_t >::mirror(pixmap, MirrorMode::X);
						}
					}

					/* Convert BGR to RGB format. */
					if ( pixmap.colorCount() > 1 )
					{
						pixmap = Processor< uint8_t >::swapChannels(pixmap);
					}
				}

				return true;
			}

			/** @copydoc EmEn::Libs::PixelFactory::FileFormatInterface::writeFile() */
			[[nodiscard]]
			bool
			writeFile (const std::filesystem::path & filepath, const Pixmap< pixel_data_t, dimension_t > & pixmap) const noexcept override
			{
				if ( !pixmap.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", pixmap parameter is invalid !" "\n";

					return false;
				}

				std::ofstream file{filepath, std::ios::binary};

				if ( !file.is_open() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unable to open a Targa file " << filepath << " for writing !" "\n";

					return false;
				}

				/* Identification string */
				const std::string identification("EmEn-Engine libPixelFactory");

				Header fileHeader{};

				/* FIXME: Check identification. */
				fileHeader.idCharCount = 0;//identification.size();
				/* NOTE: We use the default TGA bottom-left origin image. */
				fileHeader.yOrigin = 0;
				fileHeader.width = static_cast< uint16_t >(pixmap.width());
				fileHeader.height = static_cast< uint16_t >(pixmap.height());
				fileHeader.imagePixelSize = static_cast< uint8_t >(pixmap.bitPerPixel());
				/* Set image descriptor: bits 0-3 = alpha bits, bit 5 = 0 (bottom-left origin) */
				fileHeader.imageDescriptorByte = (pixmap.channelMode() == ChannelMode::RGBA || pixmap.channelMode() == ChannelMode::GrayscaleAlpha) ? 8 : 0;

				switch ( pixmap.channelMode() )
				{
					/* Grayscale Targa file */
					case ChannelMode::Grayscale :
						fileHeader.imageTypeCode = m_useRLE ? 11 : 3;
						break;

					/* Composite */
					case ChannelMode::RGB :
					case ChannelMode::RGBA :
						fileHeader.imageTypeCode = m_useRLE ? 10 : 2;
						break;

					case ChannelMode::GrayscaleAlpha :
					default:
						std::cerr << __PRETTY_FUNCTION__ << ", unhandled color channel format to write a Targa image." "\n";

						return false;;
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
						"\t" "imageDescriptorByte : " << fileHeader.imageDescriptorByte  << '\n';
				}

				/* Write in the Targa header, byte to byte */
				for ( auto i = 0U; i < 12; i++ )
				{
					if ( !file.write(static_cast< const char * >(ptr.at(i)), size.at(i)) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", unable to write the Targa header !" "\n";

						return false;
					}
				}

				/* Writing identification field. */
				if ( fileHeader.idCharCount > 0 )
				{
					if ( !file.write(identification.data(), fileHeader.idCharCount * sizeof(char)) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", unable to write the Targa identification !" "\n";

						return false;
					}
				}

				/* Prepare pixmap for writing (BGR and orientation) */
				Pixmap< pixel_data_t, dimension_t > processedPixmap;

				if ( pixmap.colorCount() > 1 )
				{
					/* RGB -> BGR */
					processedPixmap = Processor< uint8_t >::swapChannels(pixmap);

					if ( !this->invertYAxis() )
					{
						processedPixmap = Processor< uint8_t >::mirror(processedPixmap, MirrorMode::X);
					}
				}
				else
				{
					if ( this->invertYAxis() )
					{
						processedPixmap = pixmap;
					}
					else
					{
						processedPixmap = Processor< uint8_t >::mirror(pixmap, MirrorMode::X);
					}
				}

				/* Write data with or without RLE compression */
				if ( m_useRLE )
				{
					if ( !writeRLEData(file, processedPixmap) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", unable to write the Targa RLE data !" "\n";

						return false;
					}
				}
				else
				{
					if ( !file.write(reinterpret_cast< const char * >(processedPixmap.data().data()), processedPixmap.bytes()) )
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
			 * @param file Output file stream.
			 * @param pixmap The source pixmap to compress.
			 * @return bool True if successful.
			 */
			[[nodiscard]]
			static
			bool
			writeRLEData (std::ofstream & file, const Pixmap< pixel_data_t, dimension_t > & pixmap) noexcept
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

					/* Check for RLE run (repeated pixels) */
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

							/* Count consecutive identical pixels */
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
						/* Write RLE packet header (bit 7 = 1) */
						const auto packetHeader = static_cast< uint8_t >(0x80 | (runLength - 1));

						if ( !file.write(reinterpret_cast< const char * >(&packetHeader), 1) )
						{
							return false;
						}

						/* Write one pixel */
						if ( !file.write(reinterpret_cast< const char * >(&pixmapData[currentOffset]), bytesPerPixel) )
						{
							return false;
						}
					}
					else
					{
						/* Count consecutive non-repeating pixels for raw packet */
						while ( runLength < maxRunLength && currentPixel + runLength < totalPixels )
						{
							const auto checkOffset = (currentPixel + runLength) * bytesPerPixel;
							const auto nextCheckOffset = (currentPixel + runLength + 1) * bytesPerPixel;

							if ( currentPixel + runLength + 1 >= totalPixels )
							{
								++runLength;
								break;
							}

							/* Check if next two pixels are identical (start of RLE run) */
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
								/* Don't include these in raw packet, let next iteration handle as RLE */
								break;
							}

							++runLength;
						}

						/* Write raw packet header (bit 7 = 0) */
						const auto packetHeader = static_cast< uint8_t >(runLength - 1);

						if ( !file.write(reinterpret_cast< const char * >(&packetHeader), 1) )
						{
							return false;
						}

						/* Write all pixels in the raw packet */
						if ( !file.write(reinterpret_cast< const char * >(&pixmapData[currentOffset]), bytesPerPixel * runLength) )
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
				/* Number of Characters in Identification Field. */
				uint8_t idCharCount = 0; /* 0:1 (0, 255) */
				/* Color Map Type. */
				uint8_t colorMapType = 0; /* 1:1 */
				/* Image Type Code. */
				uint8_t imageTypeCode = 0; /* 2:1 */
				/* Color Map Specification. */
				uint16_t colorMapOrigin = 0; /* 3:2 */
				uint16_t colorMapLength = 0; /* 5:2 */
				uint8_t colorMapEntrySize = 0;  /* 7:1 (16, 24, 32) */
				/* Image Specification. */
				uint16_t xOrigin = 0; /* 8:2 */
				uint16_t yOrigin = 0; /* 10:2 */
				uint16_t width = 0; /* 12:2 */
				uint16_t height = 0; /* 14:2 */
				uint8_t imagePixelSize = 0; /* 16:1 (16, 24, 32) */
				uint8_t imageDescriptorByte = 0; /* 17:1 */
			};

			/** @brief Flag to enable/disable RLE compression for writing. */
			bool m_useRLE{false};
	};
}
