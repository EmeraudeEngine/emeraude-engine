/*
 * src/Graphics/TextWriter.cpp
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

#include "TextWriter.hpp"

/* Local inclusions. */
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "FontResource.hpp"
#include "Libs/PixelFactory/Processor.hpp"
#include "Tracer.hpp"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <ostream>
#include <utility>

namespace EmEn::Graphics
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;

	TextWriter::TextWriter (Pixmap< uint8_t > & target) noexcept
		: m_target(&target)
	{

	}

	bool
	TextWriter::isReady () const noexcept
	{
		if ( m_target == nullptr )
		{
			return false;
		}

		if ( m_font == nullptr )
		{
			return false;
		}

		return true;
	}

	void
	TextWriter::enableWrapping (bool state) noexcept
	{
		m_flags[WrappingEnabled] = state;
	}

	bool
	TextWriter::isWrappingEnabled () const noexcept
	{
		return m_flags[WrappingEnabled];
	}

	void
	TextWriter::enableAllCharacters (bool state) noexcept
	{
		m_flags[AllCharactersEnabled] = state;
	}

	bool
	TextWriter::isAllCharactersEnabled () const noexcept
	{
		return m_flags[AllCharactersEnabled];
	}

	void
	TextWriter::enableScrollUp (bool state) noexcept
	{
		m_flags[ScrollUpEnabled] = state;
	}

	bool
	TextWriter::isScrollUpEnabled () const noexcept
	{
		return m_flags[ScrollUpEnabled];
	}

	void
	TextWriter::enableAutoLineFeed (bool state) noexcept
	{
		m_flags[AutoLineFeedEnabled] = state;
	}

	bool
	TextWriter::autoLineFeedEnabled () const noexcept
	{
		return m_flags[AutoLineFeedEnabled];
	}

	void
	TextWriter::setTarget (Pixmap< uint8_t > * target) noexcept
	{
		m_target = target;
	}

	Pixmap< uint8_t > *
	TextWriter::target () const noexcept
	{
		return m_target;
	}

	void
	TextWriter::setFont (const std::shared_ptr< FontResource > & font) noexcept
	{
		m_font = font;
	}

	std::shared_ptr< FontResource >
	TextWriter::font () const noexcept
	{
		return m_font;
	}

	void
	TextWriter::setColor (const Color< float > & color) noexcept
	{
		m_color = color;
	}

	const Color< float > &
	TextWriter::color () const noexcept
	{
		return m_color;
	}

	void
	TextWriter::setClearColor (const Color< float > & color) noexcept
	{
		m_clearColor = color;
	}

	const Color< float > &
	TextWriter::clearColor () const noexcept
	{
		return m_clearColor;
	}

	void
	TextWriter::resetCursorToOrigin () noexcept
	{
		m_currentCursor = m_originCursor;
	}

	const Vector< 2, int > &
	TextWriter::originCursor () const noexcept
	{
		return m_originCursor;
	}

	const Vector< 2, int > &
	TextWriter::currentCursor () const noexcept
	{
		return m_currentCursor;
	}

	void
	TextWriter::setSpacing (int spacing) noexcept
	{
		m_spacing = spacing;
	}

	void
	TextWriter::lineFeed (size_t count) noexcept
	{
		/* Reset the cursor in X to the original position. */
		m_currentCursor[X] = m_originCursor[X];

		/* Advance the cursor in Y by the highest char from the current Font. */
		if ( count > 0 )
		{
			// FIXME
			//this->moveCursorY(m_font->lineHeight() * count);
		}
	}

	size_t
	TextWriter::write (const std::string & text, TextWriter::Alignment alignment) noexcept
	{
		if ( text.empty() )
		{
			Tracer::warning(ClassId, "Text is empty !");

			return 0UL;
		}

		if ( !this->isReady() )
		{
			Tracer::error(ClassId, "There is no target or font selected !");

			return 0UL;
		}

		/* Compute the cursor. */
		if ( alignment != Alignment::AtCursor )
		{
			this->setCursor(0, 0);

			this->autoAlignCursor(alignment, this->computeSpaceRequired(text));
		}

		/* Count every chars printed. */
		auto charCount = 0UL;

		for ( const auto character : text )
		{
			if ( !this->executeCharacter(static_cast< uint8_t >(character)) )
			{
				break;
			}

			charCount++;
		}

		if ( m_flags[AutoLineFeedEnabled] )
		{
			this->lineFeed(1UL);
		}

		return charCount;
	}

	size_t
	TextWriter::widthRequest (const std::string & text) const noexcept
	{
		if ( m_font == nullptr )
		{
			return 0UL;
		}

		return this->computeSpaceRequired(text).first;
	}

	TextWriter::SpaceAvailability
	TextWriter::isSpaceLeftFor (size_t glyphWidth, size_t glyphHeight) const noexcept
	{
		auto spaceAvailableInX = static_cast< size_t >(m_currentCursor[X]) + glyphWidth <= m_target->width();
		//auto spaceAvailableInY = this->OpenGLCursorY(glyphHeight) > glyphHeight;
		auto spaceAvailableInY = static_cast< size_t >(m_currentCursor[Y]) + glyphHeight <= m_target->height();

		if ( spaceAvailableInX && spaceAvailableInY )
		{
			return SpaceAvailability::Yes;
		}

		if ( spaceAvailableInY )
		{
			return SpaceAvailability::RequestNewLine;
		}

		return SpaceAvailability::No;
	}

	bool
	TextWriter::renderGlyph (const Pixmap< uint8_t > & glyph) noexcept
	{
		switch ( this->isSpaceLeftFor(glyph.width(), glyph.height()) )
		{
			case SpaceAvailability::No :
				if ( !m_flags[ScrollUpEnabled] )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", no more space left to render a glyph ! [AllowScrollUp=false]" "\n";

					return false;
				}

			// FIXME
				/*m_currentCursor[X] = m_originCursor[X];
				m_currentCursor[Y] += -static_cast< int >(m_font->lineHeight());

				if ( !this->moveUp(m_font->lineHeight(), m_clearColor) )
				{
					return false;
				}*/

				break;

			case SpaceAvailability::RequestNewLine :
				if ( !m_flags[WrappingEnabled] )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", no more space left to render a glyph on the current line ! [WrappingEnabled=false]" "\n";

					return false;
				}

				this->lineFeed(1UL);

				break;

			case SpaceAvailability::Yes :
				/* Nothing to do ... */
				break;
		}

		/* Render the glyph. */
		/*const Processor proc{*m_target};

		if ( !proc.stencil(m_color, this->getArea(glyph), glyph) )
		{
			return false;
		}*/

		/* Advancing cursor. */
		m_currentCursor[X] += static_cast< int >(glyph.width()) + m_spacing;

		return true;
	}

	std::pair< size_t, size_t >
	TextWriter::computeSpaceRequired (const std::string & text) const noexcept
	{
		auto rowWidth = 0UL;

		auto width = 0UL;
		//auto lineCount = 1UL;

		for ( const auto character : text )
		{
			const auto ASCIICode = static_cast< uint8_t >(character);

			/* This will have the next width. */
			auto nextWidth = 0UL;

			switch ( ASCIICode )
			{
				/* HT \t Horizontal Tab[f] */
				case 9 :
					// FIXME
					//nextWidth += static_cast< unsigned long >(m_font->spacing() * 4);
					break;

				/* LF \n Line feed */
				case 10 :
					width = std::max(width, rowWidth);
					//lineCount++;

					rowWidth = 0UL;
					continue;

				/* VT \v Vertical Tab */
				case 11 :
					//lineCount += 4;
					continue;

				case 13 : // CR \r Carriage return[g]
					continue;

				/* White space */
				case 32 :
					// FIXME
					//nextWidth = static_cast< unsigned long >(m_font->spacing());
					break;

				/* Remaining control chars */
				case 0 : // NUL \0 Null character
				case 1 : // SOH Start of Header
				case 2 : // STX Start of Text
				case 3 : // ETX End of Text
				case 4 : // EOT End of Transmission
				case 5 : // ENQ Enquiry
				case 6 : // ACK Acknowledgment
				case 7 : // BEL \a Bell
				case 8 : // BS \b Backspace[d][e]
				case 12 : // FF \f Form feed
				case 14 : // SO Shift Out
				case 15 : // SI Shift In
				case 16 : // DLE Data Link Escape
				case 17 : // DC1 Device Control 1 (oft. XON)
				case 18 : // DC2 Device Control 2
				case 19 : // DC3 Device Control 3 (oft. XOFF)
				case 20 : // DC4 Device Control 4
				case 21 : // NAK Negative Acknowledgement
				case 22 : // SYN Synchronous idle
				case 23 : // ETB End of Transmission Block
				case 24 : // CAN Cancel
				case 25 : // EM End of Medium
				case 26 : // SUB Substitute
				case 27 : // ESC \e[h] 	Escape[i]
				case 28 : // FS File Separator
				case 29 : // GS Group Separator
				case 30 : // RS Record Separator
				case 31 : // US Unit Separator
				case 127 : // DEL Delete[k][e]
					if ( !m_flags[AllCharactersEnabled] )
					{
						continue;
					}

					[[fallthrough]];

				default :
					// FIXME
					//nextWidth = static_cast< unsigned long >(m_font->font().glyph(ASCIICode).width() + static_cast< size_t >(m_spacing));
					break;
			}

			/* Compute a graphical glyph size. */
			if ( (rowWidth + nextWidth) <= m_target->width() )
			{
				rowWidth += nextWidth;
			}
			else
			{
				width = std::max(width, rowWidth);
				//lineCount++;

				rowWidth = 0UL;
			}
		}

		// FIXME
		/*return {
			std::max(width, rowWidth),
			lineCount * m_font->lineHeight()
		};*/
		return {0, 0};
	}

	bool
	TextWriter::executeCharacter (uint8_t ASCIICode) noexcept
	{
		switch ( ASCIICode )
		{
			/* Remaining control chars. */
			case 0 : // NUL \0 Null character
			case 1 : // SOH Start of Header
			case 2 : // STX Start of Text
			case 3 : // ETX End of Text
			case 4 : // EOT End of Transmission
			case 5 : // ENQ Enquiry
			case 6 : // ACK Acknowledgment
			case 7 : // BEL \a Bell
			case 8 : // BS \b Backspace[d][e]
			case 12 : // FF \f Form feed
			case 14 : // SO Shift Out
			case 15 : // SI Shift In
			case 16 : // DLE Data Link Escape
			case 17 : // DC1 Device Control 1 (oft. XON)
			case 18 : // DC2 Device Control 2
			case 19 : // DC3 Device Control 3 (oft. XOFF)
			case 20 : // DC4 Device Control 4
			case 21 : // NAK Negative Acknowledgement
			case 22 : // SYN Synchronous idle
			case 23 : // ETB End of Transmission Block
			case 24 : // CAN Cancel
			case 25 : // EM End of Medium
			case 26 : // SUB Substitute
			case 27 : // ESC \e[h] 	Escape[i]
			case 28 : // FS File Separator
			case 29 : // GS Group Separator
			case 30 : // RS Record Separator
			case 31 : // US Unit Separator
			case 127 : // DEL Delete[k][e]
				if ( m_flags[AllCharactersEnabled] )
				{
					// FIXME
					//return this->renderGlyph(m_font->font().glyph(ASCIICode));
				}

				break;

			/* HT \t Horizontal Tab[f]. */
			case 9 :
				// FIXME
				//this->moveCursorX(m_font->spacing() * 4);
				break;

			/* LF \n Line feed. */
			case 10 :
				this->lineFeed(1UL);
				break;

			/* VT \v Vertical Tab. */
			case 11 :
				this->lineFeed(4UL);
				break;

			/* CR \r Carriage return[g]. */
			case 13 :
				m_currentCursor[X] = m_originCursor[X];
				break;

			/* White space. */
			case 32 :
				// FIXME
				//this->moveCursorX(m_font->spacing());
				break;

			/* Printable character. */
			default :
				// FIXME
				//return this->renderGlyph(m_font->font().glyph(ASCIICode));
				break;
		}

		return true;
	}

	bool
	TextWriter::autoAlignCursor (Alignment alignment, const std::pair< size_t, size_t > & requestedSizes) noexcept
	{
		size_t x = 0;
		size_t y = 0;

		switch ( alignment )
		{
			case Alignment::TopLeft :
				x = (m_target->width() - requestedSizes.first) - this->margin(MarginPosition::Right);
				y = (m_target->height() - requestedSizes.second) - this->margin(MarginPosition::Bottom);
				break;

			case Alignment::CenterLeft :
				x = (m_target->width() - requestedSizes.first) - this->margin(MarginPosition::Right);
				y = (m_target->height() - requestedSizes.second) / 2;
				break;

			case Alignment::BottomLeft :
				x = (m_target->width() - requestedSizes.first) - this->margin(MarginPosition::Right);
				y = this->margin(MarginPosition::Top);
				break;

			case Alignment::TopCenter :
				x = (m_target->width() - requestedSizes.first) / 2;
				y = (m_target->height() - requestedSizes.second) - this->margin(MarginPosition::Bottom);
				break;

			case Alignment::Center :
				x = (m_target->width() - requestedSizes.first) / 2;
				y = (m_target->height() - requestedSizes.second) / 2;
				break;

			case Alignment::BottomCenter :
				x = (m_target->width() - requestedSizes.first) / 2;
				y = this->margin(MarginPosition::Top);
				break;

			case Alignment::TopRight :
				x = this->margin(MarginPosition::Left);
				y = (m_target->height() - requestedSizes.second) - this->margin(MarginPosition::Bottom);
				break;

			case Alignment::CenterRight :
				x = this->margin(MarginPosition::Left);
				y = (m_target->height() - requestedSizes.second) / 2;
				break;

			case Alignment::BottomRight :
				x = this->margin(MarginPosition::Left);
				y = this->margin(MarginPosition::Top);
				break;

			case Alignment::AtCursor :
				break;

			default :
				std::cerr << __PRETTY_FUNCTION__ << ", unknow alignment !" "\n";

				return false;
		}

		this->setCursor(x, y);

		return true;
	}

	bool
	TextWriter::moveUp (uint32_t distance, const Color< float > & color) noexcept
	{
		Processor proc{*m_target};

		if ( !proc.shiftTextArea(static_cast< int >(distance)) )
		{
			return false;
		}

		return proc.copy(color, {0U, 0U, m_target->width(), distance});
	}

	std::string
	TextWriter::ASCIITable () noexcept
	{
		std::string table{};
		table.reserve(256);

		for ( char character = 0; character < 127; character++ )
		{
			table += character;
		}

		for ( char character = -127; character < 0; character++ )
		{
			table += character;
		}

		return table;
	}

	std::ostream &
	operator<< (std::ostream & out, const TextWriter & obj)
	{
		return out <<
			"TextWriter data :\n"
			/* Margin */
			"Margin top : " << obj.margin(MarginPosition::Top) << "\n"
			"Margin right : " << obj.margin(MarginPosition::Right) << "\n"
			"Margin bottom : " << obj.margin(MarginPosition::Bottom) << "\n"
			"margin left : " << obj.margin(MarginPosition::Left) << "\n"
			/* Printer */
			"Target pointer : " << obj.m_target << "\n"
			"Current Style : " << obj.m_font->name() << "\n"
			"Cursor origin X : " << obj.m_originCursor[X] << "\n"
			"Cursor origin Y : " << obj.m_originCursor[Y] << "\n"
			"Current cursor X : " << obj.m_currentCursor[X] << "\n"
			"Current cursor Y : " << obj.m_currentCursor[Y] << "\n"
			"Wrapping : " << ( obj.isWrappingEnabled() ? "Enabled" : "Disabled" ) << '\n';
	}

	std::string
	to_string (const TextWriter & obj)
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
