/*
 * src/Graphics/Effects/Framebuffer/LensFlare.cpp
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

#include "LensFlare.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Scenes/LightSet.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

static constexpr auto TracerTag{"LensFlareEffect"};
/* NOLINTEND(cert-err58-cpp) */

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ---- */

	static constexpr auto ThresholdFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float threshold;
	float softKnee;
};

void main()
{
	/* 3x3 box blur to soften individual bright pixels before thresholding. */
	vec2 texelSize = vec2(texelSizeX, texelSizeY);
	vec3 color = vec3(0.0);

	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			color += texture(sceneTex, vUV + vec2(float(x), float(y)) * texelSize).rgb;
		}
	}

	color /= 9.0;

	/* Soft brightness thresholding. */
	float brightness = max(max(color.r, color.g), color.b);
	float kneeWidth = threshold * softKnee;
	float soft = brightness - threshold + kneeWidth;
	soft = clamp(soft, 0.0, 2.0 * kneeWidth);
	soft = soft * soft / (4.0 * kneeWidth + 0.00001);
	float contribution = max(soft, brightness - threshold) / max(brightness, 0.00001);

	outColor = vec4(color * max(contribution, 0.0), 1.0);
}
)GLSL";

	static constexpr auto GhostHaloFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D thresholdTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;

layout(push_constant) uniform PushConstants
{
	float lightScreenX;
	float lightScreenY;
	float ghostSpacing;
	float haloRadius;
	float haloThickness;
	float chromaticDistortion;
	float intensity;
	int ghostCount;
	float occlusionRadiusX;
	float occlusionRadiusY;
};

/* Source visibility: the fraction of a 16-tap disk around the projected light that reads the far
 * plane (a directional source is at infinity — anything written in the depth buffer there hides
 * it). A Vogel spiral, fixed rotation: the same probe every frame, no shimmer. */
float sourceVisibility (vec2 lightPos)
{
	float visible = 0.0;

	for (int i = 0; i < 16; ++i)
	{
		float r = sqrt((float(i) + 0.5) / 16.0);
		float a = float(i) * 2.39996323;
		vec2 p = lightPos + vec2(cos(a) * occlusionRadiusX, sin(a) * occlusionRadiusY) * r;
		p = clamp(p, vec2(0.0), vec2(1.0));
		visible += texture(depthTex, p).r >= 0.99999 ? 1.0 : 0.0;
	}

	return visible / 16.0;
}

/* Sample with chromatic distortion along a radial direction. */
vec3 chromaticSample(sampler2D tex, vec2 uv, vec2 direction, float distortion)
{
	return vec3(
		texture(tex, uv + direction * distortion).r,
		texture(tex, uv).g,
		texture(tex, uv - direction * distortion).b
	);
}

void main()
{
		vec2 lightPos = vec2(lightScreenX, lightScreenY);

	/* A source the geometry hides produces no flare at all. */
	float visibility = sourceVisibility(lightPos);

	if (visibility <= 0.0)
	{
		outColor = vec4(0.0);
		return;
	}

	/* Ghost direction: from light position toward the current pixel.
	 * Ghosts are placed at increasing offsets along this axis. */
	vec2 ghostVec = vUV - lightPos;
	float ghostLen = length(ghostVec);

	if (ghostLen < 0.001)
	{
		outColor = vec4(0.0);
		return;
	}

	vec2 ghostDir = ghostVec / ghostLen;

	vec3 result = vec3(0.0);

	/* ---- Ghost generation ---- */
	/* Ghosts sample the threshold texture on the mirrored side of the light position.
	 * Each ghost is placed at increasing distances along the pixel→light→mirror axis. */
	for (int i = 0; i < ghostCount; ++i)
	{
		float offset = float(i + 1) * ghostSpacing;
		vec2 ghostUV = lightPos + ghostDir * offset;

		/* Discard ghosts outside valid texture range. */
		if (ghostUV.x < 0.0 || ghostUV.x > 1.0 || ghostUV.y < 0.0 || ghostUV.y > 1.0)
			continue;

		/* Distance-based weight: ghosts far from light position are dimmer. */
		float d = distance(ghostUV, lightPos);
		float weight = 1.0 - smoothstep(0.0, 0.75, d);
		weight *= weight;

		/* Per-ghost falloff: further ghosts are progressively dimmer. */
		weight *= 1.0 / float(i + 1);

		/* Chromatic distortion along the ghost direction. */
		result += chromaticSample(thresholdTex, ghostUV, ghostDir, chromaticDistortion) * weight;
	}

	/* ---- Halo ring ---- */
	/* The halo forms a ring centered on the light's screen position. */
	{
		float d = distance(vUV, lightPos);
		float haloWeight = 1.0 - abs(d - haloRadius) / max(haloThickness, 0.001);
		haloWeight = clamp(haloWeight, 0.0, 1.0);
		haloWeight *= haloWeight;

		if (haloWeight > 0.001)
		{
			/* Sample the threshold texture at the current UV (the ring is an overlay). */
			vec2 haloDir = normalize(vUV - lightPos);
			vec2 haloUV = lightPos + haloDir * haloRadius;

			if (haloUV.x >= 0.0 && haloUV.x <= 1.0 && haloUV.y >= 0.0 && haloUV.y <= 1.0)
			{
				result += chromaticSample(thresholdTex, haloUV, haloDir, chromaticDistortion * 0.5) * haloWeight * 0.5;
			}
		}
	}

	outColor = vec4(result * intensity * visibility, 1.0);
}
)GLSL";

}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	/* ---- Lifecycle ---- */

	bool
	LensFlare::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		constexpr auto format = VK_FORMAT_R16G16B16A16_SFLOAT;

		const auto halfW = std::max(width / 2, 1U);
		const auto halfH = std::max(height / 2, 1U);

		/* Create threshold target (half-res). */
		if ( !m_thresholdTarget.create(renderer, halfW, halfH, format, "LF_Threshold") )
		{
			TraceError{TracerTag} << "Failed to create threshold target !";

			return false;
		}

		/* Create ghost+halo target (half-res). */
		if ( !m_ghostHaloTarget.create(renderer, halfW, halfH, format, "LF_GhostHalo") )
		{
			TraceError{TracerTag} << "Failed to create ghost+halo target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

				/* Single input layout (1 combined image sampler): the threshold pass. */
		auto singleInputLayout = this->getInputLayout(1);

		if ( singleInputLayout == nullptr )
		{
			return false;
		}

		/* Dual input layout: the ghost + halo pass reads the threshold target AND the scene depth
		 * (the source occlusion probe). */
		auto dualInputLayout = this->getInputLayout(2);

		if ( dualInputLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(singleInputLayout);

			m_thresholdLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ThresholdPushConstants)}
			});
		}

				{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(dualInputLayout);

			m_ghostHaloLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GhostHaloPushConstants)}
			});
		}

		if ( m_thresholdLayout == nullptr || m_ghostHaloLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		const auto vertexModule = this->getFullscreenVertexShader();

		if ( vertexModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile vertex shader !";

			return false;
		}

		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		auto thresholdFragment = shaderManager.getShaderModuleFromSourceCode(device, "LF_Threshold_FS", ShaderType::FragmentShader, ThresholdFragmentShader);

		if ( thresholdFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile threshold shader !";

			return false;
		}

		auto ghostHaloFragment = shaderManager.getShaderModuleFromSourceCode(device, "LF_GhostHalo_FS", ShaderType::FragmentShader, GhostHaloFragmentShader);

		if ( ghostHaloFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile ghost+halo shader !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_thresholdPipeline = this->createFullscreenPipeline(ClassId, "LF_Threshold", vertexModule, thresholdFragment, m_thresholdLayout, m_thresholdTarget);
		m_ghostHaloPipeline = this->createFullscreenPipeline(ClassId, "LF_GhostHalo", vertexModule, ghostHaloFragment, m_ghostHaloLayout, m_ghostHaloTarget);

		if ( m_thresholdPipeline == nullptr || m_ghostHaloPipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */

		/* Threshold: reads scene color (updated per-frame). */
		m_thresholdPerFrame = this->createPerFrameDescriptorSets(singleInputLayout, ClassId, "LF_Threshold_DescSet");

		if ( m_thresholdPerFrame.empty() )
		{
			return false;
		}

		/* Ghost+Halo: binding 0 = threshold target (fixed), binding 1 = scene depth (per frame). */
		m_ghostHaloPerFrame = this->createPerFrameDescriptorSets(dualInputLayout, ClassId, "LF_GhostHalo_DescSet");

		if ( m_ghostHaloPerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_ghostHaloPerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(0, m_thresholdTarget) )
			{
				return false;
			}
		}

		return true;
	}

	void
	LensFlare::destroy () noexcept
	{
		m_ghostHaloPerFrame.clear();
		m_thresholdPerFrame.clear();

		m_ghostHaloPipeline.reset();
		m_thresholdPipeline.reset();
		m_ghostHaloLayout.reset();
		m_thresholdLayout.reset();

		m_ghostHaloTarget.destroy();
		m_thresholdTarget.destroy();
	}

	void
	LensFlare::recordOverlayPasses (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto * lightSet = context.lightSet;


		const auto frameIndex = this->renderer().currentFrameIndex();

		/* 1. Project light direction to screen space.
		 * Use readStateIndex to match the view matrix that produced the depth buffer. */
		const auto readStateIndex = this->renderer().currentReadStateIndex();
		const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
		const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
		const auto & projMat = viewMatrices.projectionMatrix(readStateIndex);
		const auto & camPos = viewMatrices.position(readStateIndex);

		/* Light source direction (opposite of emission direction). */
		const auto lightDirection = lightSet->mainDirectionalLight()->direction();
		const auto lightSource = (-lightDirection).normalized();

		/* Project a far point along the light source direction. */
		const auto farPointX = camPos[0] + lightSource.x() * 10000.0F;
		const auto farPointY = camPos[1] + lightSource.y() * 10000.0F;
		const auto farPointZ = camPos[2] + lightSource.z() * 10000.0F;

		/* Transform to clip space. */
		const Math::Vector< 4, float > worldPos{farPointX, farPointY, farPointZ, 1.0F};
		const auto viewPos = viewMat * worldPos;
		const auto clipPos = projMat * viewPos;

		auto lightOnScreen = (clipPos[3] > 0.0F) ? 1.0F : 0.0F;
		auto screenX = 0.5F;
		auto screenY = 0.5F;

		if ( clipPos[3] > 0.001F )
		{
			screenX = (clipPos[0] / clipPos[3]) * 0.5F + 0.5F;
			screenY = (clipPos[1] / clipPos[3]) * 0.5F + 0.5F;
		}

		/* Fade when light is near screen edges or off-screen. */
		const auto dx = screenX - 0.5F;
		const auto dy = screenY - 0.5F;
		const auto distFromCenter = std::sqrt(dx * dx + dy * dy);
		lightOnScreen *= std::max(0.0F, std::min(1.0F, 1.5F - distFromCenter));

		/* Expose the visibility factor to the combine pass (dynamics0.x). */
		m_lastLightOnScreen = lightOnScreen;

		/* 2. Update per-frame threshold descriptor with scene color. */
		static_cast< void >(m_thresholdPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* 3. Pass 1: Threshold extraction (half-res). */
		const ThresholdPushConstants thresholdPC{
			.texelSizeX = 1.0F / static_cast< float >(m_thresholdTarget.width()),
			.texelSizeY = 1.0F / static_cast< float >(m_thresholdTarget.height()),
			.threshold = m_parameters.threshold,
			.softKnee = m_parameters.softKnee
		};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_thresholdTarget,
			*m_thresholdPipeline,
			*m_thresholdLayout,
			*m_thresholdPerFrame[frameIndex],
			&thresholdPC,
			sizeof(ThresholdPushConstants)
		);

				/* 4. Pass 2: Ghost generation + Halo (half-res), with the source occlusion probe: binding 1
		 * is the scene depth of THIS frame (requiresDepth() guarantees it); the probe radius is a
		 * fraction of the screen height, scaled per axis so the disk stays round on screen. */
		static_cast< void >(m_ghostHaloPerFrame[frameIndex]->writeCombinedImageSampler(1, *context.depth));

		const auto aspect = static_cast< float >(m_ghostHaloTarget.height()) / static_cast< float >(std::max(1U, m_ghostHaloTarget.width()));

		const GhostHaloPushConstants ghostHaloPC{
			.lightScreenX = screenX,
			.lightScreenY = screenY,
			.ghostSpacing = m_parameters.ghostSpacing,
			.haloRadius = m_parameters.haloRadius,
			.haloThickness = m_parameters.haloThickness,
			.chromaticDistortion = m_parameters.chromaticDistortion,
			.intensity = m_parameters.intensity,
			.ghostCount = m_parameters.ghostCount,
			.occlusionRadiusX = m_parameters.occlusionRadius * aspect,
			.occlusionRadiusY = m_parameters.occlusionRadius
		};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_ghostHaloTarget,
						*m_ghostHaloPipeline,
			*m_ghostHaloLayout,
			*m_ghostHaloPerFrame[frameIndex],
			&ghostHaloPC,
			sizeof(GhostHaloPushConstants)
		);
	}

	IndirectPostProcessEffect::CombineContribution
	LensFlare::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		CombineContribution contribution;
		contribution.prefix = "lflare";
		contribution.samplers.emplace_back(CombineSamplerInput{"Tex", &m_ghostHaloTarget});
		contribution.dynamics.emplace_back(Base::Math::Vector< 4, float >{m_lastLightOnScreen, 0.0F, 0.0F, 0.0F});

		/* Same math as the retired LF_Composite_FS pass: additive flare modulated by
		 * lightOnScreen (fades out when the light leaves the screen or is behind the
		 * camera), alpha forced to 1 as the original composite did. */
		contribution.code =
			"\tem_Color.rgb += texture(lflareTex, vUV).rgb * emDyn.lflareDynamics0.x;\n"
			"\tem_Color.a = 1.0;\n";

		return contribution;
	}
}
