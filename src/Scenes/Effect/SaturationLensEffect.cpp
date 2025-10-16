/*
 * src/Scenes/Effect/SaturationLensEffect.cpp
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

#include "SaturationLensEffect.hpp"

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

	SaturationLensEffect::SaturationLensEffect (float saturation) noexcept
		: m_saturation(saturation)
	{

	}

	bool
	SaturationLensEffect::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, Saturation, "1.0"});

		fragmentShader.addComment("Saturation control.");

		Code{fragmentShader} <<
			/* NOTE: Algorithm from Chapter 16 of OpenGL Shading Language. */
			"float intensity = dot(" << PostProcessor::Fragment << ".rgb, vec3(0.2125, 0.7154, 0.0721));" << Line::Blank <<

			PostProcessor::Fragment << " = vec4(mix(vec3(intensity), " << PostProcessor::Fragment << ".rgb, " << Saturation << "), " << PostProcessor::Fragment << ".a);";

		return true;
	}

	void
	SaturationLensEffect::setSaturation (float value) noexcept
	{
		m_saturation = value;
	}

	float
	SaturationLensEffect::saturation () const noexcept
	{
		return m_saturation;
	}
}
