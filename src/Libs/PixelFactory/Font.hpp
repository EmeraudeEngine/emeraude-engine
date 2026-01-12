/*
 * src/Libs/PixelFactory/Font.hpp
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

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <map>

/* Third-party inclusions. */
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

/* Local inclusions. */
#include "Pixmap.hpp"
#include "FileIO.hpp"
#include "Processor.hpp"

namespace EmEn::Libs::PixelFactory
{
	static constexpr auto DefaultFontSize{16U};
	static constexpr auto ASCIICount{256U};
	static constexpr auto FontMapDivisor{16U};

	/**
	 * @brief Font format type.
	 */
	enum class FontType: uint8_t
	{
		None,
		PixmapFont,
		TrueTypeFont
	};

	/**
	 * @brief The glyph array for a specific font size.
	 * @tparam precision_t The type of number for pixmap precision. Default uint8_t.
	 */
	template< typename precision_t = uint8_t >
	class ASCIIGlyphArray
	{
		public:

			/**
			 * @brief Constructs an ASCII glyph array.
			 * @param height The size/height of the font.
			 */
			explicit
			ASCIIGlyphArray (uint32_t height) noexcept
				: m_height(height)
			{

			}

			/**
			 * @brief Completes the glyph array with a function calling each character.
			 * @param getGlyph A function to get the pixmap for the current character.
			 * @param fixedWidth Declares the font with a fixed width.
			 * @return bool
			 */
			template< typename GetGlyphFunc >
			[[nodiscard]]
			bool
			writeGlyphData (GetGlyphFunc && getGlyph, bool fixedWidth)
			{
				m_fixedWidth = fixedWidth;
				m_widestChar = 0;

				for ( size_t index = 0; index < ASCIICount; index++ )
				{
					const auto glyph = getGlyph(index);

					if ( !glyph.isValid() || glyph.height() > m_height )
					{
						std::cerr << "ASCIIGlyphArray::writeGlyphData(), unable to read glyph #" << index << "\n";

						return false;
					}

					m_glyphs[index] = glyph;

					if ( m_fixedWidth == false )
					{
						const auto width = glyph.width();

						if ( width > m_widestChar )
						{
							m_widestChar = width;
						}
					}
				}

				if ( m_fixedWidth )
				{
					m_widestChar = m_glyphs[0].width();
				}

				return true;
			}

			/**
			 * @brief Returns whether the font size as been completed with a fixed width.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFixedWidth () const noexcept
			{
				return m_fixedWidth;
			}

			/**
			 * @brief Returns the widest character in the array.
			 * @note With fixed width font, this function will return the fixed width value.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			widestChar () const noexcept
			{
				return m_widestChar;
			}

			/**
			 * @brief Returns the height of characters.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			height () const noexcept
			{
				return m_height;
			}

			/**
			 * @brief Returns the pixmap for a character.
			 * @param ASCIICode ASCII code for the character.
			 * @return const Pixmap< precision_t > &
			 */
			[[nodiscard]]
			const Pixmap< precision_t > &
			glyph (uint8_t ASCIICode) const noexcept
			{
				return m_glyphs[ASCIICode];
			}

			/**
			 * @brief Returns the size of the font in bytes.
			 * @tparam output_t The type of output data. Default uint32_t.
			 * @return output_t
			 */
			template< typename output_t = uint32_t >
			[[nodiscard]]
			output_t
			bytes () const noexcept
			{
				output_t bytes = 0;

				for ( const auto & glyph : m_glyphs )
				{
					bytes += glyph.bytes();
				}

				return bytes;
			}

		private:

			std::array< Pixmap< precision_t >, ASCIICount > m_glyphs;
			uint32_t m_widestChar{0};
			uint32_t m_height{0};
			bool m_fixedWidth{false};
	};

	/**
	 * @brief The font class to read a true type font or a bitmap and store multiple size of it.
	 * @tparam precision_t The type of number for pixmap precision. Default uint8_t.
	 */
	template< typename precision_t = uint8_t >
	requires (std::is_arithmetic_v< precision_t >)
	class Font final
	{
		public:

			/**
			 * @brief Constructs the font.
			 */
			Font () noexcept = default;

			/**
			 * @brief Reads a font file. At this point, this could be a true type font format or a bitmap.
			 * @param filepath A reference to a filesystem path.
			 * @param fontSize The desired font size.
			 * @param fixedWidth Enable each glyph to be the same size at the end of the process. Otherwise, each glyph will be cropped to the minimal width.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			readFile (const std::filesystem::path & filepath, uint32_t fontSize, bool fixedWidth)
			{
				switch ( Font::findFileType(filepath) )
				{
					case FontType::PixmapFont :
					{
						Pixmap< precision_t > charsMap;

						if ( !FileIO::read(filepath, charsMap) )
						{
							std::cerr << "[ERROR] " << __PRETTY_FUNCTION__ << ", can't load pixel data from " << filepath << " file !" "\n";

							return false;
						}

						return this->parsePixmap(charsMap, fontSize, fixedWidth);
					}

					case FontType::TrueTypeFont :
						return this->readTrueTypeFile(filepath, fontSize, fixedWidth);

					case FontType::None :
						std::cerr << "[ERROR] " << __PRETTY_FUNCTION__ << ", the font file " << filepath << " is not handled !" "\n";

					return false;
				}

				return false;
			}

			/**
			 * @brief Parses a pixmap as a 16 x 16 ASCII chars map.
			 * @param charsMap A reference to the pixmap.
			 * @param fontSize The desired font size.
			 * @param fixedWidth Enable each glyph to be the same size at the end of the process. Otherwise, each glyph will be cropped to the minimal width.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			parsePixmap (const Pixmap< precision_t > & charsMap, uint32_t fontSize, bool fixedWidth)
			{
				if ( !charsMap.isValid() )
				{
					std::cerr << "[ERROR] " << __PRETTY_FUNCTION__ << ", the chars map is empty !" "\n";

					return false;
				}

				if ( charsMap.channelMode() != ChannelMode::Grayscale )
				{
					std::cerr << "[ERROR] " << __PRETTY_FUNCTION__ << ", the chars map must be a grayscale !" "\n";

					return false;
				}

				if ( charsMap.width() % FontMapDivisor > 0 || charsMap.height() % FontMapDivisor > 0 )
				{
					std::cerr << "[ERROR] " << __PRETTY_FUNCTION__ << ", invalid chars map dimensions !" "\n";

					return false;
				}

				auto & glyphArray = this->getGlyphArray(fontSize);

				const auto glyphWidth = static_cast< uint32_t >(charsMap.width() / FontMapDivisor);
				const auto glyphHeight = static_cast< uint32_t >(charsMap.height() / FontMapDivisor);

				const auto ratio = static_cast< float >(glyphWidth) / static_cast< float >(glyphHeight);
				const auto targetWidth = static_cast< uint32_t >(std::round(static_cast< float >(fontSize) * ratio));

				return glyphArray.writeGlyphData([&] (size_t index) {
					const auto coordX = static_cast< uint32_t >(index) % FontMapDivisor;
					const auto coordY = static_cast< uint32_t >(std::floor(index / FontMapDivisor)); // Vulkan API
					//const auto coordY = static_cast< uint32_t >(15 - ((charNum - coordX) / FontMapDivisor)); // OpenGL API

					/* Crop the target out of the character map. */
					auto glyph = Processor< precision_t >::crop(charsMap, {
						coordX * glyphWidth, coordY * glyphHeight,
						glyphWidth, glyphHeight
					});

					if ( !fixedWidth )
					{
						glyph = Processor< precision_t >::crop(glyph, Font::getUsableWidth(glyph));
					}

					/* If the height fits, output the pixmap. */
					if ( glyph.height() == fontSize )
					{
						return glyph;
					}

					return Processor< precision_t >::resize(glyph, targetWidth, fontSize);
				}, fixedWidth);
			}

			/**
			 * @brief Returns the glyph list for a specific size.
			 * @warning The output can be nullptr.
			 * @note If the size do not exist, the closest one will be selected.
			 * @param size The font size.
			 * @return const ASCIIGlyphArray< precision_t > *
			 */
			[[nodiscard]]
			const ASCIIGlyphArray< precision_t > *
			glyphs (uint32_t size) const
			{
				auto arrayIt = m_fonts.find(size);

				if ( arrayIt == m_fonts.cend() )
				{
					const auto closestSize = findClosestSize(size);

					if ( closestSize == 0UL )
					{
						std::cerr << "[ERROR] " << __PRETTY_FUNCTION__ << ", the font size " << closestSize << " is not loaded !" "\n";

						return nullptr;
					}

					arrayIt = m_fonts.find(closestSize);
				}

				return &arrayIt->second;
			}

			/**
			 * @brief Returns the size of the font in bytes.
			 * @tparam output_t The type of output data. Default uint32_t.
			 * @return output_t
			 */
			template< typename output_t = uint32_t >
			[[nodiscard]]
			output_t
			bytes () const noexcept
			{
				output_t bytes = 0;

				for ( const auto & [size, font] : m_fonts )
				{
					bytes += font.bytes();
				}

				return bytes;
			}

			/**
			 * @brief Converts a char to an integer.
			 * @tparam integer_t The type of integer to return the ASCII code.
			 * @param ASCII ASCII code for the character.
			 * @return integer_t
			 */
			template< typename integer_t = int32_t >
			[[nodiscard]]
			static
			integer_t
			charToInt (char ASCII) noexcept
			{
				/* NOTE: First convert to an unsigned char. */
				return static_cast< integer_t >(static_cast< uint8_t >(ASCII));
			}

			/**
			 * @brief Returns a rectangle around the valid pixels on X axis.
			 * @param glyph A reference to a pixmap.
			 * @return Math::Space2D::AARectangle< uint32_t >
			 */
			[[nodiscard]]
			static
			Math::Space2D::AARectangle< uint32_t >
			getUsableWidth (const Pixmap< precision_t > & glyph) noexcept
			{
				const auto isColumnEmpty = [&glyph] (uint32_t coordX)
				{
					for ( uint32_t coordY = 0; coordY < glyph.height(); coordY++ )
					{
						if ( glyph.pixelElement(coordX, coordY, Channel::Red) > 0 )
						{
							return false;
						}
					}

					return true;
				};

				/* Gets the first column of pixels where a color changes. */
				uint32_t offset = 0U;

				for ( uint32_t coordX = 0; coordX < glyph.width(); coordX++ )
				{
					if ( !isColumnEmpty(coordX) )
					{
						break;
					}

					offset++;
				}

				if ( offset >= glyph.width() )
				{
					return {glyph.halfWidth(), glyph.height()};
				}

				/* Gets the last column of pixels where a color is present. */
				uint32_t width = glyph.width() - offset;

				for ( uint32_t coordX = glyph.width(); coordX-- > 0U; )
				{
					if ( !isColumnEmpty(coordX) )
					{
						break;
					}

					width = coordX - offset + 1;
				}

				return {offset, 0U, width, glyph.height()};
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Font & obj)
			{
				out << "Font data (" << obj.m_fonts.size() << " sizes cached):" "\n";

				for ( const auto & [size, glyphArray] : obj.m_fonts )
				{
					out << "  - Size: " << size
						<< " (Height: " << glyphArray.height()
						<< ", FixedWidth: " << std::boolalpha << glyphArray.isFixedWidth()
						<< ", Widest: " << glyphArray.widestChar() << ")" "\n";
				}

				return out;
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Font & obj)
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			/**
			 * @brief Returns the closest font size.
			 * @param targetSize The desired font size.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			findClosestSize (uint32_t targetSize) const noexcept
			{
				if ( m_fonts.empty() )
				{
					return 0;
				}

				const auto upper = m_fonts.lower_bound(targetSize);

				if ( upper == m_fonts.cbegin() )
				{
					return upper->first;
				}

				if ( upper == m_fonts.cend() )
				{
					return std::prev(upper)->first;
				}

				const auto lower = std::prev(upper);

				const uint32_t diffUpper = upper->first - targetSize;
				const uint32_t diffLower = targetSize - lower->first;

				if ( diffLower <= diffUpper )
				{
					return lower->first;
				}

				return upper->first;
			}

			/**
			 * @brief Gets or creates the glyph array for a size.
			 * @param size The desired font size.
			 * @return ASCIIGlyphArray< precision_t > &
			 */
			[[nodiscard]]
			ASCIIGlyphArray< precision_t > &
			getGlyphArray (uint32_t size)
			{
				auto it = m_fonts.find(size);

				if ( it == m_fonts.end() )
				{
					it = m_fonts.emplace(size, ASCIIGlyphArray< precision_t >{size}).first;
				}

				return it->second;
			}

			/**
			 * @brief Reads a TrueType font file.
			 * @param filepath A reference to a filesystem path.
			 * @param fontSize The desired font size.
			 * @param fixedWidth Enable each glyph to be the same width at the end of the process.
			 * @return bool
			 */
			bool
			readTrueTypeFile (const std::filesystem::path & filepath, uint32_t fontSize, bool fixedWidth)
			{
				FT_Library library{};
				FT_Face face{};

				/* Try to init FreeType 2. */
				if ( FT_Init_FreeType(&library) > 0 )
				{
					std::cerr << "[ERROR] " << __PRETTY_FUNCTION__ << ", FreeType 2 init failed !" "\n";

					return false;
				}

				/* Load the font face. Face index 0 (always available). */
				if ( FT_New_Face(library, filepath.string().data(), 0, &face) > 0 )
				{
					std::cerr << "[ERROR] " << __PRETTY_FUNCTION__ << ", Font file " << filepath << " cannot be open !" "\n";

					return false;
				}

				/* Prepare output sizes.
				 * NOTE: 0 means square. */
				if ( FT_Set_Pixel_Sizes(face, 0, static_cast< FT_UInt >(fontSize)) > 0 )
				{
					std::cerr << "[ERROR] " << __PRETTY_FUNCTION__ << ", the size request with this font is not available !" "\n";

					return false;
				}

				auto & glyphs = this->getGlyphArray(fontSize);

				const auto success = glyphs.writeGlyphData([&] (size_t index) {
					/* Gets the correct glyph index inside the font for the iso code. */
					const auto glyphIndex = FT_Get_Char_Index(face, static_cast< FT_ULong >(index));

					/* Gets the glyph loaded.
					 * NOTE: Only one font can be loaded at a time. */
					if ( FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER) > 0 )
					{
						std::cerr << "[ERROR] Glyph " << glyphIndex << " failed to load !" "\n";

						return Pixmap< precision_t >{};
					}

					//const auto glyphWidth = face->glyph->bitmap.width;
					//const auto glyphHeight = face->glyph->bitmap.rows;
					//const auto size = glyphWidth * glyphHeight;

					/*if ( size > 0 )
					{
						auto newWidth = glyphWidth + 2;

						// Checks for overflow
						newWidth = std::min(newWidth, size);

						Pixmap glyph{};

						if ( glyph.initialize(glyphWidth, glyphHeight, ChannelMode::Grayscale) )
						{
							const auto bufferSize = static_cast< uint32_t >(face->glyph->bitmap.width * face->glyph->bitmap.rows) * sizeof(uint8_t);

							glyph.fill(face->glyph->bitmap.buffer, bufferSize);
						}
						else
						{
							return {};
						}

						const auto offsetY = (m_maxHeight - face->glyph->bitmap_top) - (m_maxHeight / 4);

						if ( offsetY > m_maxHeight )
						{
							return {};
						}

						//int32_t offsetY = 0;
						//if ( face->glyph->bitmap_top != face->glyph->bitmap.rows )
						//	offsetY = (m_maxHeight - face->glyph->bitmap_top) / 2;
						//else
						//	offsetY = m_maxHeight - face->glyph->bitmap.rows;

						auto & currentGlyph = m_glyphs.at(charNum);

						currentGlyph.initialize(newWidth, m_maxHeight, ChannelMode::Grayscale);

						const Processor proc{currentGlyph};
						proc.blit(glyph, {1UL, static_cast< uint32_t >(offsetY), glyph.width(), glyph.height()});

						// Sets the highest width of a char.
						m_maxWidth = std::max< uint32_t >(newWidth, m_maxWidth);
					}
					else
					{
						auto currentGlyph = m_glyphs.at(charNum);

						// Empty char.
						currentGlyph.initialize(m_maxWidth / 2, m_maxHeight, ChannelMode::Grayscale);
						currentGlyph.fill(Color(0.0F, 0.0F, 0.0F));
					}*/

					return Pixmap< precision_t >{};
				}, fixedWidth);

				FT_Done_Face(face);
				FT_Done_FreeType(library);

				return success;
			}

			/**
			 * @brief Determines the type of the font file.
			 * @param filepath A reference to a filesystem path.
			 * @return FontType
			 */
			static
			FontType
			findFileType (const std::filesystem::path & filepath) noexcept
			{
				const auto extension = IO::getFileExtension(filepath, true);

				if ( extension == "ttf" )
				{
					return FontType::TrueTypeFont;
				}

				if ( extension == "bmp" )
				{
					return FontType::PixmapFont;
				}

				if ( extension == "tga" )
				{
					return FontType::PixmapFont;
				}

				if ( extension == "png" )
				{
					return FontType::PixmapFont;
				}

				if ( extension == "jpg" || extension == "jpeg" )
				{
					return FontType::PixmapFont;
				}

				return FontType::None;
			}

			std::map< uint32_t, ASCIIGlyphArray< precision_t > > m_fonts;
	};
}
