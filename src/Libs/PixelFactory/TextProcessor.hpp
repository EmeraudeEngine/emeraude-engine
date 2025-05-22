/*
 * src/Libs/PixelFactory/TextProcessor.hpp
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

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <string>

/* Local inclusions for usages. */
#include "Pixmap.hpp"
#include "Font.hpp"
#include "Types.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief The text processor is used to write on the targeted pixmap.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	class TextProcessor final
	{
		public:

			/**
			 * @brief Constructs a processor on a pixmap.
			 */
			TextProcessor () noexcept = default;

			/**
			 * @brief Constructs a processor on a pixmap.
			 * @param target A writable reference to a pixmap.
			 */
			explicit
			TextProcessor (Pixmap< pixel_data_t > & target) noexcept
				: m_pixmap(&target),
				m_rectangle(target.area())
			{

			}

			/**
			 * @brief Sets the target pixmap to write on.
			 * @param pixmap A reference to a pixmap.
			 * @return bool
			 */
			bool
			setPixmap (Pixmap< pixel_data_t > & pixmap) noexcept
			{
				if ( !pixmap.isValid() )
				{
					return false;
				}

				m_pixmap = &pixmap;
				m_rectangle = pixmap.rectangle();

				return true;
			}

			/**
			 * @brief Sets a rectangle where to write the text onto the pixmap.
			 * @param rectangle A reference to a rectangle.
			 * @return void
			 */
			void
			setRectangle (const Math::Space2D::AARectangle< int32_t > & rectangle) noexcept
			{
				m_rectangle = rectangle;

				this->updateMetrics();
			}

			/**
			 * @brief Returns the rectangle where the text is written onto the pixmap.
			 * @return const Math::Space2D::AARectangle< int32_t > &
			 */
			[[nodiscard]]
			const Math::Space2D::AARectangle< int32_t > &
			rectangle () const noexcept
			{
				return m_rectangle;
			}

			/**
			 * @brief Sets a font to write on the pixmap.
			 * @param font A pointer to a font.
			 * @param fontSize The size in the font.
			 * @return void
			 */
			void
			setFont (const Font< pixel_data_t > & font, uint32_t fontSize) noexcept
			{
				m_selectedFont = font.glyphs(fontSize);

				this->updateMetrics();
			}

			/**
			 * @brief Sets the font color.
			 * @param color A reference to a color.
			 * @return void
			 */
			void
			setFontColor (const Color< float > & color) noexcept
			{
				m_fontColor = color;
			}

			/**
			 * @brief Set the mode to print characters on the pixmap.
			 * @param mode The draw pixel mode.
			 * @return void
			 */
			void
			setDrawMode (DrawPixelMode mode) noexcept
			{
				m_mode = mode;
			}

			/**
			 * @brief Returns the mode to print characters on the pixmap.
			 * @return DrawPixelMode
			 */
			[[nodiscard]]
			DrawPixelMode
			setDrawMode () const noexcept
			{
				return m_mode;
			}

			/**
			 * @brief Set space between lines.
			 * @param lineSpace The space.
			 * @return void
			 */
			void
			setLineSpace (dimension_t lineSpace) noexcept
			{
				m_textMetrics.lineSpace = lineSpace;

				this->updateMetrics();
			}

			/**
			 * @brief Returns the current lince space.
			 * @return dimension_t
			 */
			[[nodiscard]]
			dimension_t
			lineSpace () const noexcept
			{
				return m_textMetrics.lineSpace;
			}

			/**
			 * @brief Writes a text on the pixmap with the font and color setup.
			 * @param text A reference to a string.
			 * @return bool
			 */
			bool
			write (const std::string & text) noexcept
			{
				if ( m_pixmap == nullptr || !m_rectangle.isValid() )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", the area is not valid !" "\n";

					return false;
				}

				if ( m_selectedFont == nullptr )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", there is no font selected !" "\n";

					return false;
				}

				/* Text buffer position. */
				dimension_t currentChar = 0;

				for ( dimension_t currentRow = 0; currentRow < m_textMetrics.maxRows; ++currentRow )
				{
					dimension_t currentColumn = 0;

					for ( ; currentChar < text.size(); ++currentChar )
					{
						auto ASCIICode = text.at(currentChar);

						/* NOTE: Check for a new line feed. */
						if ( ASCIICode == '\n' )
						{
							++currentChar;

							break;
						}

						/* NOTE: Check for spaces special cases. */
						if ( ASCIICode == ' ' )
						{
							/* NOTE: Do not print a space at a beginning of a line. */
							if ( currentColumn == 0 )
							{
								continue;
							}

							/* NOTE: Check if the next word can fit in the row. */
							const auto nextSeparator = text.find_first_of(' ', currentChar + 1);

							if ( nextSeparator != std::string::npos )
							{
								const auto nextWordLength = nextSeparator - currentChar;
								const auto spaceLeft = m_textMetrics.maxColumns - currentColumn;

								if ( nextWordLength < m_textMetrics.maxColumns && spaceLeft < nextWordLength )
								{
									break;
								}
							}
						}

						this->blitCharacter(ASCIICode, currentColumn, currentRow);

						++currentColumn;

						if ( currentColumn >= m_textMetrics.maxColumns )
						{
							currentColumn = 0;
							++currentChar;

							break;
						}
					}

					/* NOTE: No more text to print. */
					if ( currentChar >= text.size() )
					{
						break;
					}
				}

				return true;
			}

		private:

			/**
			 * @brief Blit a character on the target pixmap.
			 * @param ASCIICode The ASCII code of the character.
			 * @param column The column inside the area.
			 * @param row The row inside the area.
			 * @return bool
			 */
			bool
			blitCharacter (char ASCIICode, dimension_t column, dimension_t row) noexcept
			{
				const auto & glyph = m_selectedFont->glyph(ASCIICode);

				/* Compute the final area on the pixmap. */
				const auto glyphArea = glyph.rectangle(
					m_rectangle.left() + (column * m_selectedFont->widestChar()),
					m_rectangle.top() + (row * m_textMetrics.lineHeight)
				);

				for ( dimension_t coordX = 0; coordX < glyphArea.width(); ++coordX )
				{
					for ( dimension_t coordY = 0; coordY < glyphArea.height(); ++coordY )
					{
						if constexpr ( std::is_floating_point_v< pixel_data_t > )
						{
							m_pixmap->blendPixel(
								glyphArea.offsetX() + coordX,
								glyphArea.offsetY() + coordY,
								m_fontColor,
								m_mode,
								glyph.pixelElement(coordX, coordY, Channel::Red)
							);
						}
						else
						{
							const auto value = static_cast< float >(glyph.pixelElement(coordX, coordY, Channel::Red)) / static_cast< float >(std::numeric_limits< pixel_data_t >::max());

							m_pixmap->blendPixel(
								glyphArea.left() + coordX,
								glyphArea.top() + coordY,
								m_fontColor,
								m_mode,
								value
							);
						}
					}
				}

				return true;
			}

			/**
			 * @brief Computes line height and line available on the pixmap.
			 * @return void
			 */
			void
			updateMetrics () noexcept
			{
				if ( !m_rectangle.isValid() || m_selectedFont == nullptr )
				{
					return;
				}

				m_textMetrics.lineHeight = m_selectedFont->height() + m_textMetrics.lineSpace;
				m_textMetrics.maxColumns = std::floor(m_rectangle.width() / m_selectedFont->widestChar());
				m_textMetrics.maxRows = std::floor(m_rectangle.height() / m_textMetrics.lineHeight);
			}

			Pixmap< pixel_data_t > * m_pixmap{nullptr};
			Math::Space2D::AARectangle< int32_t > m_rectangle;
			const ASCIIGlyphArray< pixel_data_t > * m_selectedFont{nullptr};
			Color< float > m_fontColor = White;
			DrawPixelMode m_mode{DrawPixelMode::Normal};
			TextMetrics< dimension_t > m_textMetrics;
	};
}
