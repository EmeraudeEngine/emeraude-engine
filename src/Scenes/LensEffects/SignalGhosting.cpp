/*
 * src/Scenes/LensEffects/SignalGhosting.cpp
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

#include "SignalGhosting.hpp"

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
	SignalGhosting::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Signal ghosting effect (multipath reception).");

		/* Primary ghost: a faint duplicate shifted to the right. */
		Code{fragmentShader} <<
			"vec3 sgGhost = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + vec2(" << m_offset << ", 0.0)).rgb;" << Line::End <<
			PostProcessor::Fragment << ".rgb += (sgGhost - " << PostProcessor::Fragment << ".rgb) * " << m_intensity << ";";

		/* Optional second ghost: further offset, more attenuated. */
		if ( m_secondGhostEnabled )
		{
			fragmentShader.addComment("Second ghost (weaker, further offset).");

			Code{fragmentShader} <<
				"vec3 sgGhost2 = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << " + vec2(" << (m_offset * 2.5F) << ", 0.0)).rgb;" << Line::End <<
				PostProcessor::Fragment << ".rgb += (sgGhost2 - " << PostProcessor::Fragment << ".rgb) * " << (m_intensity * 0.35F) << ";";
		}

		return true;
	}

	void
	SignalGhosting::setOffset (float offset) noexcept
	{
		if ( offset < 0.0F )
		{
			Tracer::warning(ClassId, "Offset must be >= 0 !");

			return;
		}

		m_offset = offset;
	}
}
