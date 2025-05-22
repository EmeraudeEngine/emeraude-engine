/*
 * src/Overlay/FramebufferProperties.cpp
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

#include "FramebufferProperties.hpp"

/* STL inclusions. */
#include <ostream>
#include <sstream>

namespace EmEn::Overlay
{
	void
	FramebufferProperties::updateScaledResolution () noexcept
	{
		if ( m_screenScaleX <= 0.0F )
		{
			m_screenScaleX = 1.0F;
		}

		if ( m_screenScaleY <= 0.0F )
		{
			m_screenScaleY = 1.0F;
		}

		m_inverseScreenScaleX = 1.0F / m_screenScaleX;
		m_inverseScreenScaleY = 1.0F / m_screenScaleY;

		m_resolutionX = static_cast< float >(m_width) * m_inverseScreenScaleX;
		m_resolutionY = static_cast< float >(m_height) * m_inverseScreenScaleY;
	}

	void
	FramebufferProperties::reset () noexcept
	{
		m_width = 0;
		m_height = 0;
		m_screenScaleX = 1.0F;
		m_screenScaleY = 1.0F;
		m_inverseScreenScaleX = 1.0F;
		m_inverseScreenScaleY = 1.0F;
		m_resolutionX = 0.0F;
		m_resolutionY = 0.0F;
	}
}
