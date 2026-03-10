/*
 * src/Libs/PixelFactory/FileFormatJpeg.hpp
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
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

/* Third-party inclusions. */
#include <jpeglib.h>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/IO/ByteStream.hpp"
#include "Pixmap.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief Class for read and write JPEG format via byte streams.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 * @extends EmEn::Libs::PixelFactory::FileFormatInterface The base IO class.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	class FileFormatJpeg final : public FileFormatInterface< pixel_data_t, dimension_t >
	{
		public:

			FileFormatJpeg () noexcept = default;

			/** @copydoc EmEn::Libs::PixelFactory::FileFormatInterface::readStream() */
			[[nodiscard]]
			bool
			readStream (IO::ByteStream & stream, Pixmap< pixel_data_t, dimension_t > & pixmap) noexcept override
			{
				pixmap.clear();

				/* JPEG requires all data in memory for jpeg_mem_src. */
				std::vector< unsigned char > inputBuffer;
				const unsigned char * sourcePtr = nullptr;
				unsigned long sourceSize = 0;

				if ( stream.isMemoryBacked() && stream.data() != nullptr )
				{
					sourcePtr = reinterpret_cast< const unsigned char * >(stream.data());
					sourceSize = static_cast< unsigned long >(stream.size());
				}
				else
				{
					const auto totalSize = stream.size();

					if ( totalSize == 0 )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", empty stream !" "\n";

						return false;
					}

					inputBuffer.resize(totalSize);

					if ( !stream.read(inputBuffer.data(), totalSize) )
					{
						std::cerr << __PRETTY_FUNCTION__ << ", failed to read stream data !" "\n";

						return false;
					}

					sourcePtr = inputBuffer.data();
					sourceSize = static_cast< unsigned long >(totalSize);
				}

				jpeg_decompress_struct info{};
				jpeg_error_mgr error{};

				info.err = jpeg_std_error(&error);

				jpeg_create_decompress(&info);

				jpeg_mem_src(&info, sourcePtr, sourceSize);

				jpeg_read_header(&info, 1);
				jpeg_start_decompress(&info);

				if constexpr ( PixelFactoryDebugEnabled )
				{
					std::cout <<
						"[Jpeg_DEBUG] Reading header." << '\n' <<
						"\tWidth : " << info.output_width << '\n' <<
						"\tHeight : " << info.output_height << '\n' <<
						"\tComponents : " << info.output_components << '\n';
				}

				auto pixmapAllocated = false;

				switch ( info.output_components )
				{
					case 1:
						pixmapAllocated = pixmap.initialize(info.output_width, info.output_height, ChannelMode::Grayscale);
						break;

					case 2:
						pixmapAllocated = pixmap.initialize(info.output_width, info.output_height, ChannelMode::GrayscaleAlpha);
						break;

					case 3:
						pixmapAllocated = pixmap.initialize(info.output_width, info.output_height, ChannelMode::RGB);
						break;

					case 4:
						pixmapAllocated = pixmap.initialize(info.output_width, info.output_height, ChannelMode::RGBA);
						break;

					default:
						break;
				}

				if ( pixmapAllocated )
				{
					/* Always decode to canonical top-left origin. */
					size_t rowIndex = 0;

					while ( info.output_scanline < info.output_height )
					{
						auto rowData = pixmap.rowPointer(rowIndex);

						jpeg_read_scanlines(&info, &rowData, 1);

						rowIndex++;
					}
				}

				jpeg_finish_decompress(&info);
				jpeg_destroy_decompress(&info);

				return pixmapAllocated;
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

				if ( pixmap.colorCount() != 3 && pixmap.colorCount() != 1 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", only rgb and grayscale format is supported for now !" "\n";

					return false;
				}

				jpeg_compress_struct info{};
				jpeg_error_mgr error{};

				info.err = jpeg_std_error(&error);

				jpeg_create_compress(&info);

				/* Use memory destination, write to stream afterwards. */
				unsigned char * outBuffer = nullptr;
				unsigned long outSize = 0;

				jpeg_mem_dest(&info, &outBuffer, &outSize);

				info.image_width = static_cast< JDIMENSION >(pixmap.width());
				info.image_height = static_cast< JDIMENSION >(pixmap.height());

				switch ( pixmap.channelMode() )
				{
					case ChannelMode::RGB :
					case ChannelMode::RGBA :
						info.input_components = 3;
						info.in_color_space = JCS_RGB;
						break;

					case ChannelMode::Grayscale :
					case ChannelMode::GrayscaleAlpha :
						info.input_components = 1;
						info.in_color_space = JCS_GRAYSCALE;
						break;

					default:
						std::cerr << __PRETTY_FUNCTION__ << ", unhandled format !" "\n";

						jpeg_destroy_compress(&info);
						free(outBuffer);

						return false;
				}

				if constexpr ( PixelFactoryDebugEnabled )
				{
					std::cout <<
						"[Jpeg_DEBUG] Writing header." << '\n' <<
						"\tWidth : " << info.image_width << '\n' <<
						"\tHeight : " << info.image_height << '\n' <<
						"\tComponents : " << info.input_components << '\n';
				}

				jpeg_set_defaults(&info);

				/* Apply JPEG options. */
				const auto quality = std::clamp(options.jpeg.quality, 0, 100);
				jpeg_set_quality(&info, quality, 1);

				if ( options.jpeg.optimizeHuffman )
				{
					info.optimize_coding = TRUE;
				}

				if ( options.jpeg.progressive )
				{
					jpeg_simple_progression(&info);
				}

				/* Apply chroma subsampling for color images. */
				if ( options.jpeg.subsampling != ChromaSubsampling::Auto && info.input_components == 3 )
				{
					switch ( options.jpeg.subsampling )
					{
						case ChromaSubsampling::Sample444 :
							info.comp_info[0].h_samp_factor = 1;
							info.comp_info[0].v_samp_factor = 1;
							break;

						case ChromaSubsampling::Sample422 :
							info.comp_info[0].h_samp_factor = 2;
							info.comp_info[0].v_samp_factor = 1;
							break;

						case ChromaSubsampling::Sample420 :
							info.comp_info[0].h_samp_factor = 2;
							info.comp_info[0].v_samp_factor = 2;
							break;

						default:
							break;
					}

					info.comp_info[1].h_samp_factor = 1;
					info.comp_info[1].v_samp_factor = 1;
					info.comp_info[2].h_samp_factor = 1;
					info.comp_info[2].v_samp_factor = 1;
				}

				jpeg_start_compress(&info, 1);

				if ( options.invertYAxis )
				{
					auto rowIndex = static_cast< size_t >(pixmap.height() - 1);

					while ( info.next_scanline < info.image_height )
					{
						auto * rowData = const_cast< JSAMPROW >(pixmap.rowPointer(rowIndex));

						jpeg_write_scanlines(&info, &rowData, 1);

						--rowIndex;
					}
				}
				else
				{
					size_t rowIndex = 0;

					while ( info.next_scanline < info.image_height )
					{
						auto * rowData = const_cast< JSAMPROW >(pixmap.rowPointer(rowIndex));

						jpeg_write_scanlines(&info, &rowData, 1);

						rowIndex++;
					}
				}

				jpeg_finish_compress(&info);
				jpeg_destroy_compress(&info);

				/* Write compressed data to the stream. */
				const auto result = stream.write(outBuffer, static_cast< size_t >(outSize));

				/* Free the buffer allocated by jpeg_mem_dest. */
				free(outBuffer);

				return result;
			}
	};
}
