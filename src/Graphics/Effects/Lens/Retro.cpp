/*
 * src/Graphics/Effects/Lens/Retro.cpp
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

#include "Retro.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"

namespace EmEn::Graphics::Effects::Lens
{
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	Retro::generateFragmentShaderCode (Generator::Abstract & /*generator*/, Saphir::FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Retro color quantization effect.");

		Code{fragmentShader} <<
			"float rtScale = 1.0 / float(" << (m_levels - 1) << ");" << Line::Blank <<

			"float rtR = clamp(floor(" << PostProcessor::Fragment << ".r / rtScale + 0.5) * rtScale, 0.0, 1.0);" << Line::End <<
			"float rtG = clamp(floor(" << PostProcessor::Fragment << ".g / rtScale + 0.5) * rtScale, 0.0, 1.0);" << Line::End <<
			"float rtB = clamp(floor(" << PostProcessor::Fragment << ".b / rtScale + 0.5) * rtScale, 0.0, 1.0);" << Line::Blank <<

			PostProcessor::Fragment << " = vec4(rtR, rtG, rtB, " << PostProcessor::Fragment << ".a);";

		return true;
	}

	void
	Retro::setLevels (int levels) noexcept
	{
		if ( levels < 2 || levels > 256 )
		{
			Tracer::warning(ClassId, "Levels must be in range [2, 256] !");

			return;
		}

		m_levels = levels;
	}
}
