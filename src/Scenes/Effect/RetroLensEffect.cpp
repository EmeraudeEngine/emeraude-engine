/*
 * src/Scenes/Effect/RetroLensEffect.cpp
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

#include "RetroLensEffect.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	bool
	RetroLensEffect::generateFragmentShaderCode (Generator::Abstract & /*generator*/, Saphir::FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Retro effect.");

		Code{fragmentShader} <<
			"const float scale = 1.0 / 4;" << Line::Blank <<

			"const float rComp = clamp(round((" << PostProcessor::Fragment << ".r / scale) + 0.5) * scale, 0.0, 1.0);" << Line::End <<
			"const float gComp = clamp(round((" << PostProcessor::Fragment << ".g / scale) + 0.5) * scale, 0.0, 1.0);" << Line::End <<
			"const float bComp = clamp(round((" << PostProcessor::Fragment << ".b / scale) + 0.5) * scale, 0.0, 1.0);" << Line::Blank <<

			PostProcessor::Fragment << " = vec4(rComp, gComp, bComp, " << PostProcessor::Fragment << ".a);";

		return true;
	}
}
