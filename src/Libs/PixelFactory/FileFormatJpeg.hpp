/*
 * src/Libs/PixelFactory/FileFormatJpeg.hpp
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
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>

/* Third-party inclusions. */
#include <jpeglib.h>

/* Local inclusions for inheritances. */
#include "FileFormatInterface.hpp"

/* Local inclusions for usages. */
#include "Pixmap.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief Class for read and write JPEG format.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 * @extends EmEn::Libs::PixelFactory::FileFormatInterface The base IO class.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	class FileFormatJpeg final : public FileFormatInterface< pixel_data_t, dimension_t >
	{
		public:

			/**
			 * @brief Constructs a JPEG format IO.
			 */
			FileFormatJpeg () noexcept = default;

			/** @copydoc EmEn::Libs::PixelFactory::FileFormatInterface::readFile() */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, Pixmap< pixel_data_t, dimension_t > & pixmap) noexcept override
			{
				pixmap.clear();

				auto loaded = false;

				/* FIXME: Use C++ stream. */
#if IS_WINDOWS
				auto * file = _wfopen(filepath.c_str(), L"rb");
#else
				auto * file = fopen(filepath.c_str(), "rb");
#endif

				if ( file != nullptr )
				{
					jpeg_decompress_struct info{};
					jpeg_error_mgr error{};

					info.err = jpeg_std_error(&error);

					jpeg_create_decompress(&info);

					jpeg_stdio_src(&info, file);

					/* This does not really read header. */
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
							/* Badness */
							break;
					}

					if ( pixmapAllocated )
					{
						if ( this->invertYAxis() )
						{
							auto rowIndex = static_cast< size_t >(info.output_height - 1);

							while (info.output_scanline < info.output_height )
							{
								auto rowData = pixmap.rowPointer(rowIndex);

								jpeg_read_scanlines(&info, &rowData, 1);

								rowIndex--;
							}
						}
						else
						{
							size_t rowIndex = 0;

							while (info.output_scanline < info.output_height )
							{
								auto rowData = pixmap.rowPointer(rowIndex);

								jpeg_read_scanlines(&info, &rowData, 1);

								rowIndex++;
							}
						}

						loaded = true;
					}

					jpeg_finish_decompress(&info);
					jpeg_destroy_decompress(&info);

					fclose(file);
				}

				return loaded;
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

				if ( pixmap.colorCount() != 3 && pixmap.colorCount() != 1 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", only rgb and grayscale format is supported for now !" "\n";

					return false;
				}

				/* FIXME: Use C++ stream. */
#if IS_WINDOWS
				auto * file = _wfopen(filepath.c_str(), L"wb");
#else
				auto * file = fopen(filepath.c_str(), "wb");
#endif

				if ( file != nullptr )
				{
					jpeg_compress_struct info{};
					jpeg_error_mgr error{};

					info.err = jpeg_std_error(&error);

					jpeg_create_compress(&info);

					jpeg_stdio_dest(&info, file);

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

					jpeg_set_quality(&info, m_quality > 100 ? 100 : m_quality, 1);
					jpeg_start_compress(&info, 1);

					if ( this->invertYAxis() )
					{
						auto rowIndex = static_cast< size_t >(pixmap.height() - 1);

						while (info.next_scanline < info.image_height )
						{
							auto * rowData = (JSAMPROW)pixmap.rowPointer(rowIndex);

							jpeg_write_scanlines(&info, &rowData, 1);

							--rowIndex;
						}
					}
					else
					{
						size_t rowIndex = 0;

						while (info.next_scanline < info.image_height )
						{
							auto * rowData = (JSAMPROW)pixmap.rowPointer(rowIndex);

							jpeg_write_scanlines(&info, &rowData, 1);

							rowIndex++;
						}
					}

					jpeg_finish_compress(&info);
					jpeg_destroy_compress(&info);

					fclose(file);

					return true;
				}

				return false;
			}

			/**
			 * @brief Sets the JPEG compression quality.
			 * @param value A value from 0 to 100.
			 * @return void
			 */
			void
			setQuality (int value) noexcept
			{
				if ( value > 100 )
				{ 
					m_quality = 100;
				}
				else if ( value < 0 )
				{ 
					m_quality = 0; 
				}
				else
				{ 
					m_quality = value; 
				}
			}

			/**
			 * @brief Returns the JPEG compression quality.
			 * @return int
			 */
			[[nodiscard]]
			int
			getQuality () const noexcept
			{
				return m_quality;
			}

		private:

			int m_quality{75};
	};
}
