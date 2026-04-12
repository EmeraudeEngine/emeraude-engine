/*
 * src/Scenes/Editor/Gizmo/Abstract.cpp
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

#include "Abstract.hpp"

/* STL inclusions. */
#include <cmath>

namespace EmEn::Scenes::Editor::Gizmo
{
	void
	Abstract::destroy () noexcept
	{
		m_program.reset();
		m_geometry.reset();
		m_created = false;
	}

	void
	Abstract::updateScreenScale (const Libs::Math::Vector< 3, float > & cameraPosition, float fieldOfView, float screenRatio) noexcept
	{
		const float distance = (m_worldFrame.position() - cameraPosition).length();

		/* NOTE: Scale = screenRatio * distance * tan(FOV/2).
		 * This makes the gizmo occupy a constant fraction of the viewport height
		 * regardless of camera distance. */
		if ( distance > 0.001F && fieldOfView > 0.001F )
		{
			m_screenScale = screenRatio * distance * std::tan(fieldOfView * 0.5F);
		}
	}
}
