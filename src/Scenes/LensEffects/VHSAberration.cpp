/*
 * src/Scenes/LensEffects/VHSAberration.cpp
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

#include "VHSAberration.hpp"

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
	VHSAberration::generateFragmentShaderCode (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader) const noexcept
	{
		fragmentShader.addComment("VHS tracking aberration effect.");

		/* Step 1: Compute normalized vertical position [0..1] (0 = bottom, 1 = top). */
		Code{fragmentShader} <<
			"vec2 vhsUV = gl_FragCoord.xy / " << PostProcessingPC(PushConstant::Component::FrameSize) << ";" << Line::End <<
			"float vhsY = vhsUV.y;";

		/* Step 2: Scrolling band position.
		 * The band slowly drifts upward from the bottom, wrapping around.
		 * fract() keeps it in [0..1] range, creating endless scrolling. */
		Code{fragmentShader} <<
			"float vhsBandCenter = fract(" << PostProcessingPC(PushConstant::Component::Time) << " * " << m_scrollSpeed << ");" << Line::End <<
			"float vhsBandHalf = " << (m_bandHeight * 0.5F) << ";";

		/* Step 3: Compute distance from band center with wrapping.
		 * This handles the wrap-around case when the band crosses the 0/1 boundary. */
		Code{fragmentShader} <<
			"float vhsDist = min(abs(vhsY - vhsBandCenter), min(abs(vhsY - vhsBandCenter + 1.0), abs(vhsY - vhsBandCenter - 1.0)));";

		/* Step 4: Band mask with smooth edges.
		 * smoothstep gives a soft falloff at the band boundaries. */
		Code{fragmentShader} <<
			"float vhsMask = 1.0 - smoothstep(0.0, vhsBandHalf, vhsDist);";

		/* Step 5: Horizontal displacement noise (Dave Hoskins hash).
		 * Creates the jagged horizontal shifting characteristic of VHS tracking errors. */
		Code{fragmentShader} <<
			"float vhsSeed = floor(vhsY * " << PostProcessingPC(PushConstant::Component::FrameSize) << ".y * 0.25) + floor(" << PostProcessingPC(PushConstant::Component::Time) << " * 30.0);" << Line::End <<
			"vec3 vhsH = fract(vec3(vhsSeed) * vec3(0.1031, 0.1030, 0.0973));" << Line::End <<
			"vhsH += dot(vhsH, vhsH.yzx + 33.33);" << Line::End <<
			"float vhsShift = (fract((vhsH.x + vhsH.y) * vhsH.z) - 0.5) * 2.0;";

		/* Step 6: Apply horizontal UV displacement within the band.
		 * Displaced pixels are re-sampled from the color buffer. */
		Code{fragmentShader} <<
			"float vhsOffset = vhsMask * vhsShift * " << m_displacement << " * " << m_intensity << ";" << Line::End <<
			"vec2 vhsDisplacedUV = vhsUV + vec2(vhsOffset, 0.0);" << Line::End <<
			"vhsDisplacedUV.x = clamp(vhsDisplacedUV.x, 0.0, 1.0);" << Line::End <<
			"vec3 vhsDisplaced = texture(" << Uniform::PrimarySampler << ", vhsDisplacedUV).rgb;";

		/* Step 7: Luminance noise in the band (white static flicker).
		 * Simulates the bright noisy pixels visible in real VHS tracking errors. */
		Code{fragmentShader} <<
			"float vhsNoiseSeed = vhsSeed * 7.13 + vhsUV.x * 431.0;" << Line::End <<
			"vec3 vhsNP = fract(vec3(vhsNoiseSeed) * vec3(0.1031, 0.1030, 0.0973));" << Line::End <<
			"vhsNP += dot(vhsNP, vhsNP.yzx + 33.33);" << Line::End <<
			"float vhsNoise = fract((vhsNP.x + vhsNP.y) * vhsNP.z);" << Line::End <<
			"float vhsLumaNoise = (vhsNoise - 0.5) * 0.3 * vhsMask * " << m_intensity << ";";

		/* Step 8: Mix displaced color with original based on band mask. */
		Code{fragmentShader} <<
			PostProcessor::Fragment << ".rgb = mix(" << PostProcessor::Fragment << ".rgb, vhsDisplaced, vhsMask * " << m_intensity << ");" << Line::End <<
			PostProcessor::Fragment << ".rgb += vec3(vhsLumaNoise);";

		/* Step 9 (optional): Head switching sync error at the very bottom of the frame.
		 * This is NOT the same as the tracking band above. On real VHS, the rotating
		 * head drum switches between two heads once per field. This causes a sharp bright
		 * horizontal line with a brutal horizontal sync loss below it.
		 * The image tears sideways and a bright white flash marks the switch point. */
		if ( m_headSwitchingEnabled )
		{
			fragmentShader.addComment("VHS head switching sync error (bottom of frame).");

			/* The switch point: a thin line at the bottom with subtle vertical oscillation.
			 * In Vulkan, vhsY=1.0 is the bottom of the screen.
			 * At ~3/4 of the scanline, the rotating drum switches from head A to head B.
			 * The step position drifts over time (mechanical instability of cheap VCR)
			 * and the transition is soft (analog signal bandwidth limitation). */
			Code{fragmentShader} <<
				"float hsTime = " << PostProcessingPC(PushConstant::Component::Time) << ";" << Line::End <<
				"float hsJitter = sin(hsTime * 17.3) * 0.004 + sin(hsTime * 31.7) * 0.002;" << Line::End <<
				"float hsStepPos = 0.72 + sin(hsTime * 2.3) * 0.06 + sin(hsTime * 5.7) * 0.03;" << Line::End <<
				"float hsHeadStep = smoothstep(hsStepPos - 0.04, hsStepPos + 0.04, vhsUV.x) * 0.007;" << Line::End <<
				"float hsSwitchY = 1.0 - " << m_headSwitchHeight << " + hsJitter + hsHeadStep;";

			/* Two separate zones with different spreads:
			 * - The tear zone (image displacement) is wide and bleeds far below the switch point.
			 * - The white line itself is thin, just a few scanlines. */
			Code{fragmentShader} <<
				"float hsTearMask = smoothstep(hsSwitchY - 0.003, hsSwitchY + 0.025, vhsY);" << Line::End <<
				"float hsLineMask = smoothstep(hsSwitchY - 0.002, hsSwitchY, vhsY) * (1.0 - smoothstep(hsSwitchY, hsSwitchY + 0.004, vhsY));";

			/* Horizontal sync loss: below the switch line, the entire image is shifted sideways.
			 * The shift amount oscillates sluggishly (analog PLL trying to re-lock). */
			Code{fragmentShader} <<
				"float hsSyncShift = (sin(hsTime * 4.3) * 0.5 + sin(hsTime * 9.7) * 0.25 + 0.35) * " << m_headSwitchBrightness << ";" << Line::End <<
				"vec2 hsTornUV = vhsUV + vec2(hsTearMask * hsSyncShift * 0.12, 0.0);" << Line::End <<
				"hsTornUV.x = fract(hsTornUV.x);" << Line::End <<
				"vec3 hsTornColor = texture(" << Uniform::PrimarySampler << ", hsTornUV).rgb;";

			/* Apply the horizontal tear in the bottom zone. */
			Code{fragmentShader} <<
				PostProcessor::Fragment << ".rgb = mix(" << PostProcessor::Fragment << ".rgb, hsTornColor, hsTearMask);";

			/* The bright switch line: Color Dodge blend (Photoshop "burn" mode).
			 * result = base / (1.0 - dodge) -- saturates highlights naturally,
			 * bright areas blow out to white while dark areas barely change.
			 * This is how analog signal overload actually behaves. */
			Code{fragmentShader} <<
				"float hsBurnGlow = smoothstep(hsSwitchY - 0.002, hsSwitchY, vhsY) * (1.0 - smoothstep(hsSwitchY, hsSwitchY + 0.003, vhsY));" << Line::End <<
				"float hsWaveX = 0.7 + 0.3 * sin(vhsUV.x * 6.0 + hsTime * 3.0);" << Line::End <<
				"float hsDodge = hsBurnGlow * hsWaveX * " << m_headSwitchBrightness << " * 0.99;" << Line::End <<
				PostProcessor::Fragment << ".rgb = clamp(" << PostProcessor::Fragment << ".rgb / max(vec3(1.0 - hsDodge), vec3(0.01)), 0.0, 1.0);";
		}

		return true;
	}

	void
	VHSAberration::setBandHeight (float height) noexcept
	{
		if ( height <= 0.0F || height > 1.0F )
		{
			Tracer::warning(ClassId, "Band height must be in range (0, 1] !");

			return;
		}

		m_bandHeight = height;
	}

	void
	VHSAberration::setDisplacement (float displacement) noexcept
	{
		if ( displacement < 0.0F )
		{
			Tracer::warning(ClassId, "Displacement must be >= 0 !");

			return;
		}

		m_displacement = displacement;
	}

	void
	VHSAberration::setHeadSwitchHeight (float height) noexcept
	{
		if ( height <= 0.0F || height > 1.0F )
		{
			Tracer::warning(ClassId, "Head switch height must be in range (0, 1] !");

			return;
		}

		m_headSwitchHeight = height;
	}
}
