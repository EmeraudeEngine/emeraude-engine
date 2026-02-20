/*
 * src/Scenes/LensEffects/Scanlines.cpp
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

#include "Scanlines.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Keys.hpp"

namespace EmEn::Scenes::LensEffects
{
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	Scanlines::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("CRT scanline effect.");

		/* Compute the scanline period and determine where in the cycle the current pixel falls.
		 * smoothstep creates a soft transition between bright scanline and dark gap. */
		const auto period = m_lineWidth + m_gapWidth;

		Code{fragmentShader} <<
			"float slPeriod = " << period << ";" << Line::End <<
			"float slPhase = mod(gl_FragCoord.y, slPeriod);" << Line::End <<
			"float slDark = smoothstep(" << (m_lineWidth - 0.5F) << ", " << (m_lineWidth + 0.5F) << ", slPhase);" << Line::End <<
			PostProcessor::Fragment << ".rgb *= 1.0 - slDark * " << m_intensity << ";";

		return true;
	}

	void
	Scanlines::setLineWidth (float width) noexcept
	{
		if ( width <= 0.0F )
		{
			Tracer::warning(ClassId, "Line width must be > 0 !");

			return;
		}

		m_lineWidth = width;
	}

	void
	Scanlines::setGapWidth (float width) noexcept
	{
		if ( width <= 0.0F )
		{
			Tracer::warning(ClassId, "Gap width must be > 0 !");

			return;
		}

		m_gapWidth = width;
	}
}
