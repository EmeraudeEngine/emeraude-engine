/*
 * src/Overlay/Elements/Button.cpp
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

#include "Button.hpp"

/* Local inclusions. */
#include "Libs/PixelFactory/Processor.hpp"
#include "Graphics/ImageResource.hpp"

namespace EmEn::Overlay::Elements
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;
	using namespace Graphics;

	Button::Button (const FramebufferProperties & framebufferProperties, const std::string & name, const Math::Space2D::AARectangle< float > & geometry, float depth) noexcept
		: ComposedSurface(framebufferProperties, name, geometry, depth)//, m_textWriter(this->pixmap())
	{
		m_textWriter.enableWrapping(true);

		this->enablePointerListening(true);
	}

	void
	Button::onPointerEnter (float /*positionX*/, float /*positionY*/) noexcept
	{
		if ( m_status == Status::Disabled )
		{
			return;
		}

		m_status = Status::Over;

		this->setTextureOffsetY(0.25F);
	}

	void
	Button::onPointerLeave (float /*positionX*/, float /*positionY*/) noexcept
	{
		if ( m_status == Status::Disabled )
		{
			return;
		}

		m_status = Status::Normal;

		this->setTextureOffsetY(0.0F);
	}

	bool
	Button::onButtonPress (float /*positionX*/, float /*positionY*/, int32_t /*buttonNumber*/, int32_t /*modifiers*/) noexcept
	{
		if ( m_status == Status::Disabled )
		{
			return false;
		}

		m_status = Status::Active;

		this->setTextureOffsetY(0.5F);

		return true;
	}

	bool
	Button::onButtonRelease (float /*positionX*/, float /*positionY*/, int /*buttonNumber*/, int /*modifiers*/) noexcept
	{
		if ( m_status == Status::Disabled )
		{
			return false;
		}

		m_status = Status::Over;

		this->setTextureOffsetY(0.25F);

		/* Executing the function linked to the button. */
		if ( m_function == nullptr )
		{
			TraceDebug{ClassId} << "There is no function attached to this button !";

			return false;
		}

		m_function();

		return true;
	}

	TextWriter &
	Button::textWriter () noexcept
	{
		return m_textWriter;
	}

	void
	Button::setLabel (const std::string & label) noexcept
	{
		m_label = label;
	}

	const std::string &
	Button::label () const noexcept
	{
		return m_label;
	}

	void
	Button::setImage (const std::shared_ptr< ImageResource > & image) noexcept
	{
		m_image = image;
	}

	std::shared_ptr< ImageResource >
	Button::image () const noexcept
	{
		return m_image;
	}

	void
	Button::attachFunction (const Function & function) noexcept
	{
		m_function = function;
	}

	bool
	Button::onDrawFinished (Pixmap< uint8_t > & pixmap) noexcept
	{
		/* Applies the current background. */
		pixmap = Processor< uint8_t >::resize(m_image->data(), pixmap.width(), pixmap.height());

		/* Write the text on it. */
		if ( m_label.empty() )
		{
			return true;
		}

		if ( !m_textWriter.isReady() )
		{
			Tracer::warning(ClassId, "TextWriter is not ready !");

			return false;
		}

		// FIXME
		/*const auto quarterHeight = pixmap.height() / 4;

		const auto xOffset = static_cast< int >((pixmap.width() / 2) - (m_textWriter.widthRequest(m_label) / 2));
		const auto yBaseOffset = (quarterHeight / 2) - (m_textWriter.font()->lineHeight() / 2);

		for ( size_t num = 0; num < 4; num++ )
		{
			const auto yOffset = static_cast< int >((quarterHeight * num) + yBaseOffset);

			m_textWriter.setCursor(xOffset, yOffset);

			if ( m_textWriter.write(m_label, TextWriter::Alignment::AtCursor) == 0 )
			{
				Tracer::warning(ClassId, "Unable to write label on the button !");

				break;
			}
		}*/

		return true;
	}
}
