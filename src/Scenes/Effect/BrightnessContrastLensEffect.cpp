/*
 * src/Scenes/Effect/BrightnessContrastLensEffect.cpp
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

#include "BrightnessContrastLensEffect.hpp"

/* STL inclusions. */
#include <cmath>

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Code.hpp"

namespace Emeraude::Scenes
{
	using namespace Libraries;
	using namespace Saphir;
	using namespace Saphir::Keys;
	using namespace Graphics;

	BrightnessContrastLensEffect::BrightnessContrastLensEffect (float brightness, float contrast) noexcept
		: m_brightness(brightness), m_contrast(std::abs(contrast))
	{

	}

	bool
	BrightnessContrastLensEffect::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, Brightness, "0.0"});
		//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Float, Contrast, "1.0"});

		fragmentShader.addComment("Brightness/Contrast control.");

		// FIXME : Set a parametric output fragment coming from the generator.
		Code{fragmentShader} <<
			"const vec3 bcCorrection = ((" << PostProcessor::Fragment << ".rgb - 0.5) * max(" << Contrast << ", 0.0)) + 0.5;" << Line::Blank <<

			"bcCorrection += " << Brightness << ";" << Line::Blank <<

			PostProcessor::Fragment << " = vec4(clamp(bcCorrection, 0.0, 1.0), " << PostProcessor::Fragment << ".a);";

		return true;
	}

	void
	BrightnessContrastLensEffect::setBrightness (float value) noexcept
	{
		m_brightness = value;
	}

	float
	BrightnessContrastLensEffect::brightness () const noexcept
	{
		return m_brightness;
	}

	void
	BrightnessContrastLensEffect::setContrast (float value) noexcept
	{
		m_contrast = std::abs(value);
	}

	float
	BrightnessContrastLensEffect::contrast () const noexcept
	{
		return m_contrast;
	}
}
