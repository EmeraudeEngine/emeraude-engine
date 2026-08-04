/*
 * src/Graphics/Effects/Display/FXAASharpen.cpp
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

#include "FXAASharpen.hpp"

/* Local inclusions. */
#include "Graphics/Effects/Display/FXAA.hpp"
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Keys.hpp"

namespace EmEn::Graphics::Effects::Display
{
	using namespace Saphir;
	using namespace Saphir::Keys;

	bool
	FXAASharpen::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("FXAA 3.11 + CAS sharpening (fetch override).");

		if ( !FXAA::declareFunctions(fragmentShader) )
		{
			return false;
		}

		/* Anti-aliased center color. */
		Code{fragmentShader} <<
			"vec3 fxsCenter = em_fxaa(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ", 1.0 / " << PostProcessingPC(PushConstant::Component::FrameSize) << ", "
			<< m_parameters.subpixelQuality << ", " << m_parameters.edgeThreshold << ", " << m_parameters.edgeThresholdMin << ");";

		/* CAS 5-tap cross from the original texture (post-FXAA neighbors are the original
		 * texels — FXAA only shifts the center sample, not neighbors). Same math as the
		 * retired single-pass Framebuffer::FXAASharpen shader. */
		Code{fragmentShader} <<
			"vec3 fxsNorth = textureOffset(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ", ivec2( 0, -1)).rgb;" << Line::End <<
			"vec3 fxsSouth = textureOffset(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ", ivec2( 0,  1)).rgb;" << Line::End <<
			"vec3 fxsEast = textureOffset(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ", ivec2( 1,  0)).rgb;" << Line::End <<
			"vec3 fxsWest = textureOffset(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ", ivec2(-1,  0)).rgb;";

		Code{fragmentShader} <<
			"const vec3 fxsLumaWeights = vec3(0.2126, 0.7152, 0.0722);" << Line::End <<
			"float fxsLN = dot(fxsNorth, fxsLumaWeights);" << Line::End <<
			"float fxsLS = dot(fxsSouth, fxsLumaWeights);" << Line::End <<
			"float fxsLE = dot(fxsEast, fxsLumaWeights);" << Line::End <<
			"float fxsLW = dot(fxsWest, fxsLumaWeights);" << Line::End <<
			"float fxsLC = dot(fxsCenter, fxsLumaWeights);" << Line::End <<
			"float fxsContrast = max(fxsLC, max(max(fxsLN, fxsLS), max(fxsLE, fxsLW))) - min(fxsLC, min(min(fxsLN, fxsLS), min(fxsLE, fxsLW)));";

		Code{fragmentShader} <<
			"float fxsPeak = max(1.0 - fxsContrast, 0.0);" << Line::End <<
			"float fxsWeight = fxsPeak * fxsPeak * " << m_parameters.sharpness << ";" << Line::End <<
			"vec3 fxsAverage = (fxsNorth + fxsSouth + fxsEast + fxsWest) * 0.25;" << Line::End <<
			PostProcessor::Fragment << " = vec4(clamp(fxsCenter + (fxsCenter - fxsAverage) * fxsWeight, 0.0, 1.0), 1.0);";

		return true;
	}
}
