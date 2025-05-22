/*
 * src/Overlay/Elements/Text.cpp
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

#include "Text.hpp"

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Overlay::Elements
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace PixelFactory;
	using namespace Graphics;

	Text::Text (const FramebufferProperties & framebufferProperties, const std::string & name, const Math::Space2D::AARectangle< float > & geometry, float depth) noexcept
		: ComposedSurface(framebufferProperties, name, geometry, depth)//, m_textWriter(this->pixmap())
	{
		m_textWriter.enableWrapping(true);
		m_textWriter.enableAutoLineFeed(true);
		m_textWriter.enableScrollUp(true);
	}

	void
	Text::write (const std::string & text, bool clear) noexcept
	{
		/* Clear the text. */
		if ( clear )
		{
			this->clear();
		}

		/* Write the new one. */
		if ( m_textWriter.write(text, TextWriter::Alignment::AtCursor) != 0U )
		{
			m_text += text;

			if ( m_textWriter.autoLineFeedEnabled() )
			{
				m_text += "\n";
			}
		}
		else
		{
			Tracer::error(ClassId, "Unable to write text !");
		}

		/* Tell the Renderer there is new data to upload to video memory. */
		this->drawFinished();
	}

	void
	Text::clear () noexcept
	{
		m_text.clear();

		//this->pixmap().fill(m_clearColor);

		m_textWriter.resetCursorToOrigin();

		/* Tell Renderer there is new data to upload to video memory. */
		this->drawFinished();
	}
}
