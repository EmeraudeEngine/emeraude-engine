/*
 * src/Graphics/Effects/Display/FXAA.cpp
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

#include "FXAA.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/Code.hpp"
#include "Saphir/Declaration/Function.hpp"
#include "Saphir/FragmentShader.hpp"
#include "Saphir/Keys.hpp"

namespace EmEn::Graphics::Effects::Display
{
	using namespace Saphir;
	using namespace Saphir::Keys;

	bool
	FXAA::declareFunctions (FragmentShader & fragmentShader) noexcept
	{
		/* Rec. 709 luminance of the chain output at an arbitrary UV. */
		{
			Declaration::Function luma{"em_fxaaLuma", GLSL::Float};
			luma.addInParameter(GLSL::Sampler2D, "srcSampler");
			luma.addInParameter(GLSL::FloatVector2, "uv");

			Code{luma, Location::Output} <<
				"return dot(texture(srcSampler, uv).rgb, vec3(0.2126, 0.7152, 0.0722));";

			if ( !fragmentShader.declare(luma) )
			{
				return false;
			}
		}

		/* FXAA 3.11 Quality 12 — returns the anti-aliased color at uv.
		 * Timothy Lottes' algorithm (NVIDIA), same math as the retired
		 * Effects::Framebuffer::FXAA single-pass shader. */
		{
			Declaration::Function fxaa{"em_fxaa", GLSL::FloatVector3};
			fxaa.addInParameter(GLSL::Sampler2D, "srcSampler");
			fxaa.addInParameter(GLSL::FloatVector2, "uv");
			fxaa.addInParameter(GLSL::FloatVector2, "texelSize");
			fxaa.addInParameter(GLSL::Float, "subpixelQuality");
			fxaa.addInParameter(GLSL::Float, "edgeThreshold");
			fxaa.addInParameter(GLSL::Float, "edgeThresholdMin");

			Code{fxaa, Location::Output} <<
				/* Step 1+2: 3x3 neighborhood luminances (center keeps its color). */
				"vec3 rgbM = texture(srcSampler, uv).rgb;" << Line::End <<
				"float lumaM = dot(rgbM, vec3(0.2126, 0.7152, 0.0722));" << Line::End <<
				"float lumaN = em_fxaaLuma(srcSampler, uv + vec2( 0.0, -texelSize.y));" << Line::End <<
				"float lumaS = em_fxaaLuma(srcSampler, uv + vec2( 0.0,  texelSize.y));" << Line::End <<
				"float lumaE = em_fxaaLuma(srcSampler, uv + vec2( texelSize.x,  0.0));" << Line::End <<
				"float lumaW = em_fxaaLuma(srcSampler, uv + vec2(-texelSize.x,  0.0));" << Line::End <<
				"float lumaNW = em_fxaaLuma(srcSampler, uv + vec2(-texelSize.x, -texelSize.y));" << Line::End <<
				"float lumaNE = em_fxaaLuma(srcSampler, uv + vec2( texelSize.x, -texelSize.y));" << Line::End <<
				"float lumaSW = em_fxaaLuma(srcSampler, uv + vec2(-texelSize.x,  texelSize.y));" << Line::End <<
				"float lumaSE = em_fxaaLuma(srcSampler, uv + vec2( texelSize.x,  texelSize.y));" << Line::End <<
				/* Step 3: early exit on low contrast. */
				"float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));" << Line::End <<
				"float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));" << Line::End <<
				"float lumaRange = lumaMax - lumaMin;" << Line::End <<
				"if (lumaRange < max(edgeThresholdMin, lumaMax * edgeThreshold))" << Line::End <<
				"{" << Line::End <<
				"	return rgbM;" << Line::End <<
				"}" << Line::End <<
				/* Step 4: edge orientation (horizontal vs vertical). */
				"float edgeH = abs(lumaNW + lumaNE - 2.0 * lumaN) + abs(lumaW + lumaE - 2.0 * lumaM) * 2.0 + abs(lumaSW + lumaSE - 2.0 * lumaS);" << Line::End <<
				"float edgeV = abs(lumaNW + lumaSW - 2.0 * lumaW) + abs(lumaN + lumaS - 2.0 * lumaM) * 2.0 + abs(lumaNE + lumaSE - 2.0 * lumaE);" << Line::End <<
				"bool isHorizontal = (edgeH >= edgeV);" << Line::End <<
				/* Step 5: select edge-perpendicular direction. */
				"float stepLength = isHorizontal ? texelSize.y : texelSize.x;" << Line::End <<
				"float luma1 = isHorizontal ? lumaN : lumaW;" << Line::End <<
				"float luma2 = isHorizontal ? lumaS : lumaE;" << Line::End <<
				"float gradient1 = abs(luma1 - lumaM);" << Line::End <<
				"float gradient2 = abs(luma2 - lumaM);" << Line::End <<
				"bool is1Steeper = (gradient1 >= gradient2);" << Line::End <<
				"float gradientScaled = 0.25 * max(gradient1, gradient2);" << Line::End <<
				"float lumaLocalAverage;" << Line::End <<
				"if (is1Steeper)" << Line::End <<
				"{" << Line::End <<
				"	stepLength = -stepLength;" << Line::End <<
				"	lumaLocalAverage = 0.5 * (luma1 + lumaM);" << Line::End <<
				"}" << Line::End <<
				"else" << Line::End <<
				"{" << Line::End <<
				"	lumaLocalAverage = 0.5 * (luma2 + lumaM);" << Line::End <<
				"}" << Line::End <<
				/* Step 6: shift UV to edge center. */
				"vec2 currentUV = uv;" << Line::End <<
				"if (isHorizontal)" << Line::End <<
				"{" << Line::End <<
				"	currentUV.y += stepLength * 0.5;" << Line::End <<
				"}" << Line::End <<
				"else" << Line::End <<
				"{" << Line::End <<
				"	currentUV.x += stepLength * 0.5;" << Line::End <<
				"}" << Line::End <<
				/* Step 7: edge endpoint search (12 iterations, growing steps). */
				"vec2 offset = isHorizontal ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);" << Line::End <<
				"vec2 uv1 = currentUV - offset;" << Line::End <<
				"vec2 uv2 = currentUV + offset;" << Line::End <<
				"float lumaEnd1 = em_fxaaLuma(srcSampler, uv1) - lumaLocalAverage;" << Line::End <<
				"float lumaEnd2 = em_fxaaLuma(srcSampler, uv2) - lumaLocalAverage;" << Line::End <<
				"bool reached1 = (abs(lumaEnd1) >= gradientScaled);" << Line::End <<
				"bool reached2 = (abs(lumaEnd2) >= gradientScaled);" << Line::End <<
				"bool reachedBoth = reached1 && reached2;" << Line::End <<
				"if (!reached1) uv1 -= offset;" << Line::End <<
				"if (!reached2) uv2 += offset;" << Line::End <<
				"const float QUALITY[12] = float[12](1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0);" << Line::End <<
				"if (!reachedBoth)" << Line::End <<
				"{" << Line::End <<
				"	for (int i = 2; i < 12; i++)" << Line::End <<
				"	{" << Line::End <<
				"		if (!reached1)" << Line::End <<
				"		{" << Line::End <<
				"			lumaEnd1 = em_fxaaLuma(srcSampler, uv1) - lumaLocalAverage;" << Line::End <<
				"		}" << Line::End <<
				"		if (!reached2)" << Line::End <<
				"		{" << Line::End <<
				"			lumaEnd2 = em_fxaaLuma(srcSampler, uv2) - lumaLocalAverage;" << Line::End <<
				"		}" << Line::End <<
				"		reached1 = (abs(lumaEnd1) >= gradientScaled);" << Line::End <<
				"		reached2 = (abs(lumaEnd2) >= gradientScaled);" << Line::End <<
				"		reachedBoth = reached1 && reached2;" << Line::End <<
				"		if (!reached1) uv1 -= offset * QUALITY[i];" << Line::End <<
				"		if (!reached2) uv2 += offset * QUALITY[i];" << Line::End <<
				"		if (reachedBoth) break;" << Line::End <<
				"	}" << Line::End <<
				"}" << Line::End <<
				/* Step 8: compute edge blend factor. */
				"float distance1, distance2;" << Line::End <<
				"if (isHorizontal)" << Line::End <<
				"{" << Line::End <<
				"	distance1 = uv.x - uv1.x;" << Line::End <<
				"	distance2 = uv2.x - uv.x;" << Line::End <<
				"}" << Line::End <<
				"else" << Line::End <<
				"{" << Line::End <<
				"	distance1 = uv.y - uv1.y;" << Line::End <<
				"	distance2 = uv2.y - uv.y;" << Line::End <<
				"}" << Line::End <<
				"bool isDirection1 = (distance1 < distance2);" << Line::End <<
				"float distanceFinal = min(distance1, distance2);" << Line::End <<
				"float edgeLength = distance1 + distance2;" << Line::End <<
				"float pixelOffset = -distanceFinal / edgeLength + 0.5;" << Line::End <<
				"bool isLumaMSmaller = (lumaM < lumaLocalAverage);" << Line::End <<
				"bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0) != isLumaMSmaller;" << Line::End <<
				"float edgeBlend = correctVariation ? pixelOffset : 0.0;" << Line::End <<
				/* Step 9: subpixel aliasing blend factor. */
				"float lumaAverage = (lumaN + lumaS + lumaE + lumaW) * (1.0 / 6.0) + (lumaNW + lumaNE + lumaSW + lumaSE) * (1.0 / 12.0);" << Line::End <<
				"float subpixelOffset = clamp(abs(lumaAverage - lumaM) / lumaRange, 0.0, 1.0);" << Line::End <<
				"float subpixelBlend = (-2.0 * subpixelOffset + 3.0) * subpixelOffset * subpixelOffset;" << Line::End <<
				"subpixelBlend = subpixelBlend * subpixelBlend * subpixelQuality;" << Line::End <<
				/* Step 10: final blend. */
				"float finalBlend = max(edgeBlend, subpixelBlend);" << Line::End <<
				"vec2 finalUV = uv;" << Line::End <<
				"if (isHorizontal)" << Line::End <<
				"{" << Line::End <<
				"	finalUV.y += finalBlend * stepLength;" << Line::End <<
				"}" << Line::End <<
				"else" << Line::End <<
				"{" << Line::End <<
				"	finalUV.x += finalBlend * stepLength;" << Line::End <<
				"}" << Line::End <<
				"return texture(srcSampler, finalUV).rgb;";

			if ( !fragmentShader.declare(fxaa) )
			{
				return false;
			}
		}

		return true;
	}

	bool
	FXAA::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("FXAA 3.11 Quality 12 anti-aliasing (fetch override).");

		if ( !declareFunctions(fragmentShader) )
		{
			return false;
		}

		Code{fragmentShader} <<
			PostProcessor::Fragment << " = vec4(em_fxaa(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ", 1.0 / " << PostProcessingPC(PushConstant::Component::FrameSize) << ", "
			<< m_parameters.subpixelQuality << ", " << m_parameters.edgeThreshold << ", " << m_parameters.edgeThresholdMin << "), 1.0);";

		return true;
	}
}
