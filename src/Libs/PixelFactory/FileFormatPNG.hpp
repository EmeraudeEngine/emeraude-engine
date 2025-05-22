/*
 * src/Libs/PixelFactory/FileFormatPNG.hpp
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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <iostream>
#include <fstream>
#include <type_traits>
#include <array>
#include <vector>

/* Third-party inclusions. */
#include <png.h>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"
#include "Pixmap.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief Class for read and write PNG format.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 * @extends EmEn::Libs::PixelFactory::FileFormatInterface The base IO class.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	class FileFormatPNG final : public FileFormatInterface< pixel_data_t, dimension_t >
	{
		public:

			/**
			 * @brief Constructs a PNG format IO.
			 */
			FileFormatPNG () noexcept = default;

			/** @copydoc EmEn::Libs::PixelFactory::FileFormatInterface::readFile() */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, Pixmap< pixel_data_t, dimension_t > & pixmap) noexcept override
			{
				pixmap.clear();

				std::ifstream file{filepath, std::ios::binary};
				
				if ( !file.is_open() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", " << filepath << " cannot be read !" "\n";

					return false;
				}

				std::array< png_byte, 8 > signature{};

				/* Read signature and check it. */
				if ( !file.read(reinterpret_cast< char * >(signature.data() ), sizeof(signature)) )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unable to read the PNG signature !" "\n";

					return false;
				}

				if ( !png_check_sig(signature.data(), sizeof(signature)) )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", " << filepath << " is not a PNG file !" "\n";

					return false;
				}

				/* create a png read struct. */
				auto * png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

				if ( png == nullptr )
				{
					return false;
				}
				
				/* create a png info struct. */
				auto * pngInfo = png_create_info_struct(png);

				if ( pngInfo == nullptr )
				{
					return false;
				}

				if ( setjmp(png_jmpbuf(png)) )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", PNG read failed !" "\n";

					return false;
				}

				/* setup PNG for using standard C++ std::fstream::read() function. */
				png_set_read_fn(png, &file, customReadFunction);

				/* tell PNG that we have already read the magic number. */
				png_set_sig_bytes(png, sizeof(signature));

				/* read png info */
				png_read_info(png, pngInfo);

				const auto width = png_get_image_width(png, pngInfo);
				const auto height = png_get_image_height(png, pngInfo);
				const auto bitDepth = png_get_bit_depth(png, pngInfo);

				auto pixmapAllocated = false;

				switch ( png_get_color_type(png, pngInfo) )
				{
					case PNG_COLOR_TYPE_GRAY :
						/* Convert 1-2-4 bits grayscale images to 8 bits grayscale. */
						if ( bitDepth < 8 )
						{
							png_set_expand_gray_1_2_4_to_8(png);
						}
						else if ( bitDepth == 16 )
						{
							png_set_strip_16(png);
						}

						pixmapAllocated = pixmap.initialize(width, height, ChannelMode::Grayscale);
						break;

					case PNG_COLOR_TYPE_PALETTE :
					{
						/* Convert the indexed colors to RGB(A). */
						png_set_palette_to_rgb(png);

						/* Check if there is alpha channel. */
						png_bytep transAlpha = nullptr;
						int count = 0;
						png_color_16p color = nullptr;

						png_get_tRNS(png, pngInfo, &transAlpha, &count, &color);

						if ( transAlpha != nullptr )
						{
							pixmapAllocated = pixmap.initialize(width, height, ChannelMode::RGBA);
						}
						else
						{
							pixmapAllocated = pixmap.initialize(width, height, ChannelMode::RGB);
						}
					}
						break;

					case PNG_COLOR_TYPE_RGB :
						if ( bitDepth < 8 )
						{
							png_set_packing(png);
						}
						else if ( bitDepth == 16 )
						{
							png_set_strip_16(png);
						}

						pixmapAllocated = pixmap.initialize(width, height, ChannelMode::RGB);
						break;

					case PNG_COLOR_TYPE_RGB_ALPHA :
						if ( bitDepth < 8 )
						{
							png_set_packing(png);
						}
						else if ( bitDepth == 16 )
						{
							png_set_strip_16(png);
						}

						pixmapAllocated = pixmap.initialize(width, height, ChannelMode::RGBA);
						break;

					case PNG_COLOR_TYPE_GRAY_ALPHA :
						if ( bitDepth < 8 )
						{
							png_set_packing(png);
						}
						else if ( bitDepth == 16 )
						{
							png_set_strip_16(png);
						}

						pixmapAllocated = pixmap.initialize(width, height, ChannelMode::GrayscaleAlpha);
						break;

					default:
						std::cerr << __PRETTY_FUNCTION__ << ", unhandled format !" "\n";

						return false;
				}

				if ( !pixmapAllocated )
				{
					png_destroy_read_struct(&png, &pngInfo, nullptr);

					return false;
				}

				/* Update info structure to apply transformations */
				png_read_update_info(png, pngInfo);

				/* Set up a pointer array. Each one points at the beginning of a row. */
				std::vector< png_bytep > rowPointers(pixmap.height(), nullptr);
				auto & buffer = pixmap.data();

				for ( size_t yIndex = 0; yIndex < pixmap.height(); ++yIndex )
				{
					const auto rowIndex = this->invertYAxis() ? static_cast< uint32_t >(pixmap.height() - 1 - yIndex) : yIndex;
					const auto offset = rowIndex * pixmap.width() * pixmap.colorCount();

					rowPointers.at(yIndex) = static_cast< png_bytep >(buffer.data() + offset);
				}

				/* read pixel data using row pointers */
				png_read_image(png, rowPointers.data());

				/* finish decompression and release memory */
				png_read_end(png, nullptr);

				png_destroy_read_struct(&png, &pngInfo, nullptr);

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

				const int bitDepth = 8;
				int colorType = 0;

				switch ( pixmap.channelMode() )
				{
					case ChannelMode::Grayscale :
						colorType = PNG_COLOR_TYPE_GRAY;
						break;

					case ChannelMode::GrayscaleAlpha :
						colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
						break;

					case ChannelMode::RGB :
						colorType = PNG_COLOR_TYPE_RGB;
						break;

					case ChannelMode::RGBA :
						colorType = PNG_COLOR_TYPE_RGB_ALPHA;
						break;

					default:
						std::cerr << __PRETTY_FUNCTION__ << ", invalid color count !" "\n";

						return false;
				}

				std::ofstream file{filepath, std::ios::binary};

				if ( !file.is_open() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", " << filepath << " cannot be write !" "\n";

					return false;
				}

				auto * png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

				if ( png == nullptr )
				{
					return false;
				}

				auto * pngInfo = png_create_info_struct(png);

				if ( pngInfo == nullptr )
				{
					return false;
				}

				if ( setjmp(png_jmpbuf(png)) )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", PNG read failed !" "\n";

					png_destroy_write_struct(&png, &pngInfo);

					return false;
				}

				png_set_IHDR(
					png,
					pngInfo,
					static_cast< png_uint_32 >(pixmap.width()),
					static_cast< png_uint_32 >(pixmap.height()),
					bitDepth,
					colorType,
					PNG_INTERLACE_NONE, // PNG_INTERLACE_ADAM7
					PNG_COMPRESSION_TYPE_BASE,
					PNG_FILTER_TYPE_BASE
				);

				/* Prepare every pixel row pointer into a list. */
				std::vector< png_bytep > rowPointers{pixmap.height(), nullptr};

				for ( size_t yIndex = 0; yIndex < pixmap.height(); ++yIndex )
				{
					auto rowIndex = this->invertYAxis() ? static_cast< uint32_t >(pixmap.height() - 1 - yIndex) : yIndex;

					rowPointers[yIndex] = const_cast< png_bytep >(pixmap.rowPointer(rowIndex));
				}

				png_set_write_fn(png, &file, customWriteFunction, nullptr);

				png_set_rows(png, pngInfo, rowPointers.data());
				png_write_png(png, pngInfo, PNG_TRANSFORM_IDENTITY, nullptr); // PNG_TRANSFORM_INVERT_ALPHA

				png_destroy_write_struct(&png, &pngInfo);

				return true;
			}

		private:

			/**
			 * @brief Custom function to read with std::fstream with libPNG.
			 * @param pngPtr
			 * @param data
			 * @param length
			 * @return void
			 */
			static
			void
			customReadFunction (png_structp pngPtr, png_bytep data, png_size_t length) noexcept
			{
				png_voidp file = png_get_io_ptr(pngPtr);

				static_cast< std::ifstream * >(file)->read(reinterpret_cast< char * >(data), static_cast< uint32_t>(length));
			}

			/**
			 * @brief Custom function to write with std::fstream with libPNG.
			 * @param pngPtr
			 * @param data
			 * @param length
			 * @return void
			 */
			static
			void
			customWriteFunction (png_structp pngPtr, png_bytep data, png_size_t length) noexcept
			{
				png_voidp file = png_get_io_ptr(pngPtr);

				static_cast< std::ofstream * >(file)->write(reinterpret_cast< const char * >(data), static_cast< uint32_t>(length));
			}
	};
}
