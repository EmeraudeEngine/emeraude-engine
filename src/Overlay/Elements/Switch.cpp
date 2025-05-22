/*
 * src/Overlay/Elements/Switch.cpp
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

#include "Switch.hpp"

/* Local inclusions. */
#include "Libs/PixelFactory/Processor.hpp"
#include "Graphics/ImageResource.hpp"

namespace EmEn::Overlay::Elements
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace PixelFactory;
	using namespace Graphics;

	Switch::Switch (const FramebufferProperties & framebufferProperties, const std::string & name, const Math::Space2D::AARectangle< float > & geometry, float depth) noexcept
		: ComposedSurface(framebufferProperties, name, geometry, depth)
	{
		this->enablePointerListening(true);
	}

	void
	Switch::onPointerEnter (float /*positionX*/, float /*positionY*/) noexcept
	{
		if ( m_status == Status::Disabled )
		{
			return;
		}

		m_status = Status::Over;

		this->setTextureOffsetY(0.33333F);
	}

	void
	Switch::onPointerLeave (float /*positionX*/, float /*positionY*/) noexcept
	{
		if ( m_status == Status::Disabled )
		{
			return;
		}

		m_status = Status::Normal;

		this->setTextureOffsetY(0.0F);
	}

	bool
	Switch::onButtonPress (float /*positionX*/, float /*positionY*/, int32_t /*buttonNumber*/, int32_t /*modifiers*/) noexcept
	{
		if ( m_status == Status::Disabled )
		{
			return false;
		}

		m_status = Status::Active;

		return true;
	}

	bool
	Switch::onButtonRelease (float /*positionX*/, float /*positionY*/, int /*buttonNumber*/, int /*modifiers*/) noexcept
	{
		if ( m_status == Status::Disabled )
		{
			return false;
		}

		m_status = Status::Over;

		this->toggle();

		return true;
	}

	void
	Switch::toggle () noexcept
	{
		m_state = !m_state;

		this->setTextureOffsetX(m_state ? 0.5F : 0.0F);
	}

	void
	Switch::setState (bool state) noexcept
	{
		if ( m_state == state )
		{
			return;
		}

		m_state = state;

		this->setTextureOffsetX(m_state ? 0.5F : 0.0F);
	}

	bool
	Switch::onDrawFinished (Libs::PixelFactory::Pixmap< uint8_t > & pixmap) noexcept
	{
		/* Applies the current background. */
		pixmap = Processor< uint8_t >::resize(m_image->data(), pixmap.width(), pixmap.height());

		return true;
	}
}
