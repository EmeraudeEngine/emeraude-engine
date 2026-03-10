/*
 * src/Libs/PixelFactory/FileFormatPNG.hpp
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
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include <png.h>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/IO/ByteStream.hpp"
#include "Types.hpp"
#include "Pixmap.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief Error context for PNG operations, used to replace setjmp/longjmp mechanism.
	 * @note This struct is passed as user data to libPNG error callbacks.
	 */
	struct PNGErrorContext final
	{
		bool hasError{false};
		std::string errorMessage{};
	};

	/**
	 * @brief Class for read and write PNG format via byte streams.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 * @extends EmEn::Libs::PixelFactory::FileFormatInterface The base IO class.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	class FileFormatPNG final : public FileFormatInterface< pixel_data_t, dimension_t >
	{
		public:

			FileFormatPNG () noexcept = default;

			/** @copydoc EmEn::Libs::PixelFactory::FileFormatInterface::readStream() */
			[[nodiscard]]
			bool
			readStream (IO::ByteStream & stream, Pixmap< pixel_data_t, dimension_t > & pixmap) noexcept override
			{
				pixmap.clear();

				std::array< png_byte, 8 > signature{};

				/* Read signature and check it. */
				if ( !stream.read(signature.data(), sizeof(signature)) )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", unable to read the PNG signature !" "\n";

					return false;
				}

				if ( !png_check_sig(signature.data(), sizeof(signature)) )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", data is not a PNG stream !" "\n";

					return false;
				}

				/* Error context for capturing libPNG errors without using setjmp/longjmp. */
				PNGErrorContext errorContext{};

				auto * png = png_create_read_struct(PNG_LIBPNG_VER_STRING, &errorContext, pngErrorCallback, pngWarningCallback);

				if ( png == nullptr )
				{
					return false;
				}

				auto * pngInfo = png_create_info_struct(png);

				if ( pngInfo == nullptr )
				{
					png_destroy_read_struct(&png, nullptr, nullptr);

					return false;
				}

				/* Setup PNG for reading from ByteStream. */
				png_set_read_fn(png, &stream, customReadFunction);

				/* Tell PNG that we have already read the magic number. */
				png_set_sig_bytes(png, sizeof(signature));

				png_read_info(png, pngInfo);

				if ( errorContext.hasError )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", PNG read failed: " << errorContext.errorMessage << "\n";

					png_destroy_read_struct(&png, &pngInfo, nullptr);

					return false;
				}

				const auto width = png_get_image_width(png, pngInfo);
				const auto height = png_get_image_height(png, pngInfo);
				const auto bitDepth = png_get_bit_depth(png, pngInfo);

				auto pixmapAllocated = false;

				switch ( png_get_color_type(png, pngInfo) )
				{
					case PNG_COLOR_TYPE_GRAY :
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
						png_set_palette_to_rgb(png);

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

						png_destroy_read_struct(&png, &pngInfo, nullptr);

						return false;
				}

				if ( !pixmapAllocated )
				{
					png_destroy_read_struct(&png, &pngInfo, nullptr);

					return false;
				}

				/* Update info structure to apply transformations. */
				png_read_update_info(png, pngInfo);

				/* Set up row pointers for canonical top-left origin. */
				std::vector< png_bytep > rowPointers(pixmap.height(), nullptr);
				auto & buffer = pixmap.data();

				for ( size_t yIndex = 0; yIndex < pixmap.height(); ++yIndex )
				{
					const auto offset = yIndex * pixmap.width() * pixmap.colorCount();

					rowPointers.at(yIndex) = static_cast< png_bytep >(buffer.data() + offset);
				}

				png_read_image(png, rowPointers.data());

				if ( errorContext.hasError )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", PNG read failed: " << errorContext.errorMessage << "\n";

					png_destroy_read_struct(&png, &pngInfo, nullptr);

					return false;
				}

				png_read_end(png, nullptr);

				png_destroy_read_struct(&png, &pngInfo, nullptr);

				return !errorContext.hasError;
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

				/* Error context for capturing libPNG errors without using setjmp/longjmp. */
				PNGErrorContext errorContext{};

				auto * png = png_create_write_struct(PNG_LIBPNG_VER_STRING, &errorContext, pngErrorCallback, pngWarningCallback);

				if ( png == nullptr )
				{
					return false;
				}

				auto * pngInfo = png_create_info_struct(png);

				if ( pngInfo == nullptr )
				{
					png_destroy_write_struct(&png, nullptr);

					return false;
				}

				/* Apply the interlace mode from options. */
				const auto interlaceType = (options.png.interlace == PngInterlace::Adam7) ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE;

				png_set_IHDR(
					png,
					pngInfo,
					static_cast< png_uint_32 >(pixmap.width()),
					static_cast< png_uint_32 >(pixmap.height()),
					bitDepth,
					colorType,
					interlaceType,
					PNG_COMPRESSION_TYPE_BASE,
					PNG_FILTER_TYPE_BASE
				);

				/* Apply compression level from options. */
				const auto compressionLevel = std::clamp(options.png.compressionLevel, 0, 9);
				png_set_compression_level(png, compressionLevel);

				/* Apply filter strategy from options. */
				int pngFilter = PNG_ALL_FILTERS;

				switch ( options.png.filterStrategy )
				{
					case PngFilterStrategy::None :
						pngFilter = PNG_FILTER_NONE;
						break;

					case PngFilterStrategy::Sub :
						pngFilter = PNG_FILTER_SUB;
						break;

					case PngFilterStrategy::Up :
						pngFilter = PNG_FILTER_UP;
						break;

					case PngFilterStrategy::Average :
						pngFilter = PNG_FILTER_AVG;
						break;

					case PngFilterStrategy::Paeth :
						pngFilter = PNG_FILTER_PAETH;
						break;

					case PngFilterStrategy::Adaptive :
						pngFilter = PNG_ALL_FILTERS;
						break;
				}

				png_set_filter(png, 0, pngFilter);

				/* Prepare row pointers with optional Y-axis inversion. */
				std::vector< png_bytep > rowPointers{pixmap.height(), nullptr};

				for ( size_t yIndex = 0; yIndex < pixmap.height(); ++yIndex )
				{
					const auto rowIndex = options.invertYAxis ?
						static_cast< uint32_t >(pixmap.height() - 1 - yIndex) : static_cast< uint32_t >(yIndex);

					rowPointers[yIndex] = const_cast< png_bytep >(pixmap.rowPointer(rowIndex));
				}

				/* Setup PNG for writing to ByteStream. */
				png_set_write_fn(png, &stream, customWriteFunction, nullptr);

				png_set_rows(png, pngInfo, rowPointers.data());
				png_write_png(png, pngInfo, PNG_TRANSFORM_IDENTITY, nullptr);

				png_destroy_write_struct(&png, &pngInfo);

				if ( errorContext.hasError )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", PNG write failed: " << errorContext.errorMessage << "\n";

					return false;
				}

				return true;
			}

		private:

			static
			void
			pngErrorCallback (png_structp pngPtr, png_const_charp message) noexcept
			{
				auto * errorContext = static_cast< PNGErrorContext * >(png_get_error_ptr(pngPtr));

				if ( errorContext != nullptr )
				{
					errorContext->hasError = true;
					errorContext->errorMessage = message;
				}
			}

			static
			void
			pngWarningCallback (png_structp /* pngPtr */, png_const_charp message) noexcept
			{
				std::cerr << "PNG warning: " << message << "\n";
			}

			/**
			 * @brief Custom read function for libPNG using ByteStream.
			 * @param pngPtr The PNG structure pointer (io_ptr points to ByteStream).
			 * @param data Destination buffer.
			 * @param length Number of bytes to read.
			 */
			static
			void
			customReadFunction (png_structp pngPtr, png_bytep data, png_size_t length) noexcept
			{
				auto * stream = static_cast< IO::ByteStream * >(png_get_io_ptr(pngPtr));

				stream->read(data, length);
			}

			/**
			 * @brief Custom write function for libPNG using ByteStream.
			 * @param pngPtr The PNG structure pointer (io_ptr points to ByteStream).
			 * @param data Source buffer.
			 * @param length Number of bytes to write.
			 */
			static
			void
			customWriteFunction (png_structp pngPtr, png_bytep data, png_size_t length) noexcept
			{
				auto * stream = static_cast< IO::ByteStream * >(png_get_io_ptr(pngPtr));

				stream->write(data, length);
			}
	};
}
