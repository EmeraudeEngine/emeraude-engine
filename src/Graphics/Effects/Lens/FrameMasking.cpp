/*
 * src/Graphics/Effects/Lens/FrameMasking.cpp
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

#include "FrameMasking.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Keys.hpp"

namespace EmEn::Graphics::Effects::Lens
{
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	FrameMasking::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("CRT frame masking effect (rounded corners).");

		/* Step 1: Compute normalized UV centered at screen middle with aspect correction. */
		Code{fragmentShader} <<
			"vec2 fmUV = gl_FragCoord.xy / " << PostProcessingPC(PushConstant::Component::FrameSize) << ";" << Line::End <<
			"vec2 fmCenter = fmUV - 0.5;" << Line::End <<
			"float fmAspect = " << PostProcessingPC(PushConstant::Component::FrameSize) << ".x / " << PostProcessingPC(PushConstant::Component::FrameSize) << ".y;" << Line::End <<
			"fmCenter.x *= fmAspect;";

		/* Step 2: Signed distance field for a rounded rectangle.
		 * The half-extents are shrunk by cornerRadius so the rounding stays inside the screen. */
		Code{fragmentShader} <<
			"vec2 fmHalf = vec2(fmAspect * 0.5, 0.5) - vec2(" << m_cornerRadius << ");" << Line::End <<
			"vec2 fmD = abs(fmCenter) - fmHalf;" << Line::End <<
			"float fmDist = length(max(fmD, 0.0)) + min(max(fmD.x, fmD.y), 0.0) - " << m_cornerRadius << ";";

		/* Step 3: Apply soft black mask at the edges. */
		Code{fragmentShader} <<
			"float fmMask = 1.0 - smoothstep(-" << m_edgeSoftness << ", 0.0, fmDist);" << Line::End <<
			PostProcessor::Fragment << ".rgb *= fmMask;";

		return true;
	}

	void
	FrameMasking::setCornerRadius (float radius) noexcept
	{
		if ( radius <= 0.0F || radius > 0.5F )
		{
			Tracer::warning(ClassId, "Corner radius must be in range (0, 0.5] !");

			return;
		}

		m_cornerRadius = radius;
	}

	void
	FrameMasking::setEdgeSoftness (float softness) noexcept
	{
		if ( softness <= 0.0F )
		{
			Tracer::warning(ClassId, "Edge softness must be > 0 !");

			return;
		}

		m_edgeSoftness = softness;
	}
}
