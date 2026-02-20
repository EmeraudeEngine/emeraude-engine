/*
 * src/Scenes/LensEffects/CRTPhosphor.cpp
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

#include "CRTPhosphor.hpp"

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
	CRTPhosphor::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("CRT phosphor grid effect (aperture grille).");

		/* Step 1: Compute horizontal position within a 3-pixel RGB triad.
		 * Each triad has one R, one G, and one B phosphor column. */
		Code{fragmentShader} <<
			"float cpX = mod(gl_FragCoord.x / " << m_scale << ", 3.0);";

		/* Step 2: Build per-channel mask using smoothstep transitions.
		 * R is bright in [0..0.8], G in [1.0..1.8], B in [2.0..2.8].
		 * Gaps between columns create the visible grid pattern. */
		Code{fragmentShader} <<
			"vec3 cpMask;" << Line::End <<
			"cpMask.r = 1.0 - smoothstep(0.8, 1.0, cpX) * (1.0 - smoothstep(2.0, 2.2, cpX));" << Line::End <<
			"cpMask.g = 1.0 - smoothstep(-0.2, 0.0, cpX - 1.0) * (1.0 - smoothstep(0.8, 1.0, cpX - 1.0));" << Line::End <<
			"cpMask.b = 1.0 - smoothstep(-0.2, 0.0, cpX - 2.0) * (1.0 - smoothstep(0.8, 1.0, cpX - 2.0));";

		/* Step 3: Vertical gap between phosphor rows (subtle horizontal banding). */
		Code{fragmentShader} <<
			"float cpY = mod(gl_FragCoord.y / " << m_scale << ", 2.0);" << Line::End <<
			"float cpRowGap = 0.85 + 0.15 * smoothstep(0.0, 0.3, cpY) * (1.0 - smoothstep(1.7, 2.0, cpY));" << Line::End <<
			"cpMask *= cpRowGap;";

		/* Step 4: Mix the phosphor mask with the original image. */
		Code{fragmentShader} <<
			PostProcessor::Fragment << ".rgb = mix(" << PostProcessor::Fragment << ".rgb, " << PostProcessor::Fragment << ".rgb * cpMask, " << m_intensity << ");";

		return true;
	}

	void
	CRTPhosphor::setScale (float scale) noexcept
	{
		if ( scale <= 0.0F )
		{
			Tracer::warning(ClassId, "Scale must be > 0 !");

			return;
		}

		m_scale = scale;
	}
}
