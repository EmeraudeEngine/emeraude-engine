/*
 * src/Graphics/Effects/Display/Sharpen.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Sharpen.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Keys.hpp"

namespace EmEn::Graphics::Effects::Display
{
	using namespace Saphir;
	using namespace Saphir::Keys;

	bool
	Sharpen::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("Adaptive contrast sharpening (CAS-style, 5-tap cross).");

		/* The working fragment already holds the center sample (standard fetch);
		 * the four neighbors are re-sampled from the chain output around the pixel.
		 * Same math as the retired single-pass Framebuffer::Sharpen effect. */
		Code{fragmentShader} <<
			"vec3 shrpCenter = " << PostProcessor::Fragment << ".rgb;" << Line::End <<
			"vec3 shrpNorth = textureOffset(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ", ivec2( 0, -1)).rgb;" << Line::End <<
			"vec3 shrpSouth = textureOffset(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ", ivec2( 0,  1)).rgb;" << Line::End <<
			"vec3 shrpEast = textureOffset(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ", ivec2( 1,  0)).rgb;" << Line::End <<
			"vec3 shrpWest = textureOffset(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ", ivec2(-1,  0)).rgb;";

		/* Rec. 709 luminance of the cross; local contrast = luminance range. */
		Code{fragmentShader} <<
			"const vec3 shrpLumaWeights = vec3(0.2126, 0.7152, 0.0722);" << Line::End <<
			"float shrpLN = dot(shrpNorth, shrpLumaWeights);" << Line::End <<
			"float shrpLS = dot(shrpSouth, shrpLumaWeights);" << Line::End <<
			"float shrpLE = dot(shrpEast, shrpLumaWeights);" << Line::End <<
			"float shrpLW = dot(shrpWest, shrpLumaWeights);" << Line::End <<
			"float shrpLC = dot(shrpCenter, shrpLumaWeights);" << Line::End <<
			"float shrpContrast = max(shrpLC, max(max(shrpLN, shrpLS), max(shrpLE, shrpLW))) - min(shrpLC, min(min(shrpLN, shrpLS), min(shrpLE, shrpLW)));";

		/* Adaptive weight: quadratic falloff in high-contrast areas to prevent ringing,
		 * then highpass sharpening (center minus neighborhood average). */
		Code{fragmentShader} <<
			"float shrpPeak = max(1.0 - shrpContrast, 0.0);" << Line::End <<
			"float shrpWeight = shrpPeak * shrpPeak * " << m_parameters.sharpness << ";" << Line::End <<
			"vec3 shrpAverage = (shrpNorth + shrpSouth + shrpEast + shrpWest) * 0.25;" << Line::End <<
			PostProcessor::Fragment << " = vec4(clamp(shrpCenter + (shrpCenter - shrpAverage) * shrpWeight, 0.0, 1.0), 1.0);";

		return true;
	}
}
