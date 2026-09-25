/*
 * src/Graphics/Effects/Camera/LensFlare.cpp
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
#include <numbers>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/LightSet.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

static constexpr auto TracerTag{"LensFlareEffect"};

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ---- */

	/* The SOURCE pass: what is bright enough to leave a visible ghost, plus the analytic sun. ⚠️ The
	 * output is in DISPLAY units (nits × exposure), NOT nits: the sun's disc carries ~5e7 nits, far past
	 * the 65 504 of a half float — written in nits it overflowed to infinity, then NaN once filtered, and
	 * no ghost ever showed. The ghost pass brings its result back to nits. */
	constexpr auto SourceFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
/* The clouds' view transmittance (VolumetricClouds, R), meaningful when cloudTransmittanceEnabled. */
layout(set = 0, binding = 2) uniform sampler2D cloudTransmittanceTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float threshold;
	float softKnee;
	float exposure;
	float lightScreenX;
	float lightScreenY;
	float sunRadiusX;
	float sunRadiusY;
	float sunLuminanceR;
	float sunLuminanceG;
	float sunLuminanceB;
	float occlusionRadiusX;
	float occlusionRadiusY;
	float cloudTransmittanceEnabled;
};

/* The sun's visibility: the fraction of a 16-tap disk around its projected position that reads the far
 * plane (a directional source is at infinity — anything written in the depth buffer there hides it),
 * each sky tap weighted by the clouds' transmittance. A Vogel spiral, fixed rotation: no shimmer. */
float sunVisibility (vec2 lightPos)
{
	float visible = 0.0;

	for ( int i = 0; i < 16; ++i )
	{
		float r = sqrt((float(i) + 0.5) / 16.0);
		float a = float(i) * 2.39996323;
		vec2 p = clamp(lightPos + vec2(cos(a) * occlusionRadiusX, sin(a) * occlusionRadiusY) * r, vec2(0.0), vec2(1.0));

		/* ⚠️ The sky is the clear value, 1.0 exactly: 0.99999 let every mountain past ~8.9 km show the sun. */
		float tap = texture(depthTex, p).r >= 1.0 ? 1.0 : 0.0;

		if ( cloudTransmittanceEnabled > 0.5 )
		{
			tap *= texture(cloudTransmittanceTex, p).r;
		}

		visible += tap;
	}

	return visible / 16.0;
}

void main()
{
	/* 3x3 box blur to soften individual bright pixels before thresholding. */
	vec2 texelSize = vec2(texelSizeX, texelSizeY);
	vec3 color = vec3(0.0);

	for ( int y = -1; y <= 1; ++y )
	{
		for ( int x = -1; x <= 1; ++x )
		{
			color += texture(sceneTex, vUV + vec2(float(x), float(y)) * texelSize).rgb;
		}
	}

	color /= 9.0;

	/* Soft brightness thresholding, in DISPLAY units: the chain colour here is an absolute luminance
	 * in nits (the camera phase runs before the tone mapping), and a threshold compared with nits let
	 * a whole daylight sky feed the ghosts. The contribution is a RATIO, so the colour stays in nits. */
	float brightness = max(max(color.r, color.g), color.b) * exposure;
	float kneeWidth = threshold * softKnee;
	float soft = clamp(brightness - threshold + kneeWidth, 0.0, 2.0 * kneeWidth);
	soft = soft * soft / (4.0 * kneeWidth + 0.00001);
	float contribution = max(soft, brightness - threshold) / max(brightness, 0.00001);

	vec3 result = color * max(contribution, 0.0) * exposure;

	/* The ANALYTIC sun: the light is not in the image (a skybox's painted sun is clipped, far under the
	 * threshold once exposed), so it is injected here as a soft disc carrying the light's illuminance.
	 * Its visibility is probed only where the disc is drawn. */
	if ( sunRadiusY > 0.0 )
	{
		vec2 lightPos = vec2(lightScreenX, lightScreenY);
		float r = length((vUV - lightPos) / vec2(sunRadiusX, sunRadiusY));

		if ( r < 1.0 )
		{
			result += vec3(sunLuminanceR, sunLuminanceG, sunLuminanceB) * (1.0 - smoothstep(0.6, 1.0, r)) * sunVisibility(lightPos);
		}
	}

	/* A half float stops at 65 504. */
	outColor = vec4(min(result, vec3(60000.0)), 1.0);
}
)GLSL";

	/* The GHOST pass: John Chapman's "pseudo lens flare" (2013). Every bright source of the source image
	 * is reflected through the centre of the screen as a row of ghosts, plus a halo ring. */
	constexpr auto GhostFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sourceTex;

layout(push_constant) uniform PushConstants
{
	float ghostDispersal;
	float haloWidth;
	float chromaticDistortion;
	float intensity;
	int ghostCount;
	float aspect;
	float inverseExposure;
	float haloIntensity;
};

/* A sample with its red and blue channels displaced along the ghost direction (the lens dispersion). */
vec3 chromaticSample (vec2 uv, vec2 direction)
{
	vec2 offset = direction * chromaticDistortion;

	return vec3(
		texture(sourceTex, uv + offset).r,
		texture(sourceTex, uv).g,
		texture(sourceTex, uv - offset).b
	);
}

/* Weight of a sample by its distance to the centre: a source far off the optical axis leaves weaker
 * ghosts. ⚠️ GENTLE on purpose (power 2): Chapman's power 10 was there to damp arbitrary bright pixels,
 * which the lens reflectance (Parameters::intensity) already does here; with it, a sun a quarter of the
 * screen off-centre kept 1 % of its ghosts. */
float centreWeight (vec2 uv, float power)
{
	return pow(1.0 - min(length(vec2(0.5) - uv) / length(vec2(0.5)), 1.0), power);
}

void main()
{
	/* The image mirrored through the centre, and the vector from each pixel toward it. */
	vec2 uv = vec2(1.0) - vUV;
	vec2 toCentre = vec2(0.5) - uv;
	vec2 ghostVec = toCentre * ghostDispersal;

	float toCentreLength = length(toCentre * vec2(aspect, 1.0));

	if ( toCentreLength < 1.0e-5 )
	{
		outColor = vec4(0.0);

		return;
	}

	/* The direction of the dispersion, and of the halo, round on screen. */
	vec2 direction = (toCentre * vec2(aspect, 1.0)) / toCentreLength;

	vec3 result = vec3(0.0);

	/* ---- Ghosts: samples marching toward the centre, one ghost of every bright source each. ----
	 * ⚠️ FLUX CONSERVED: ghost i is a copy of the source magnified 1 / |1 - dispersal · i| times, and
	 * Chapman's sum gives every ghost the source's full luminance — at a dispersal of 0.35 the fourth
	 * ghost was 20× the sun and carried 400× its flux, a white disc over half the frame. Each ghost is
	 * weighted by the inverse of its area ratio, (1 - dispersal · i)². */
	for ( int i = 0; i < ghostCount; ++i )
	{
		vec2 offset = uv + ghostVec * float(i);

		if ( any(lessThan(offset, vec2(0.0))) || any(greaterThan(offset, vec2(1.0))) )
		{
			continue;
		}

		float shrink = 1.0 - ghostDispersal * float(i);

		result += chromaticSample(offset, direction / vec2(aspect, 1.0)) * centreWeight(offset, 2.0) * (shrink * shrink);
	}

	/* ---- Halo: a ring of radius haloWidth around the centre, fed by what lies across it. ---- */
	{
		vec2 haloUV = uv + (direction / vec2(aspect, 1.0)) * haloWidth;

		if ( all(greaterThanEqual(haloUV, vec2(0.0))) && all(lessThanEqual(haloUV, vec2(1.0))) )
		{
			/* The ring spreads a source over its whole circumference: haloIntensity conserves the flux. */
			result += chromaticSample(haloUV, direction / vec2(aspect, 1.0)) * centreWeight(haloUV, 2.0) * haloIntensity;
		}
	}

	/* Back to nits for the chain (the source was in display units); a half float stops at 65 504. */
	outColor = vec4(min(result * intensity * inverseExposure, vec3(60000.0)), 1.0);
}
)GLSL";
}

namespace EmEn::Graphics::Effects::Camera
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

		if ( !m_sourceTarget.create(renderer, halfW, halfH, format, "LF_Source") )
		{
			TraceError{TracerTag} << "Failed to create the source target !";

			return false;
		}

		if ( !m_ghostTarget.create(renderer, halfW, halfH, format, "LF_Ghost") )
		{
			TraceError{TracerTag} << "Failed to create the ghost target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Source: the chain colour, the scene depth (the sun probe), the clouds' transmittance (the same probe). */
		auto sourceInputLayout = this->getInputLayout(3);
		/* Ghost: the source target. */
		auto ghostInputLayout = this->getInputLayout(1);

		if ( sourceInputLayout == nullptr || ghostInputLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(sourceInputLayout);

			m_sourceLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SourcePushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(ghostInputLayout);

			m_ghostLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GhostPushConstants)}
			});
		}

		if ( m_sourceLayout == nullptr || m_ghostLayout == nullptr )
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

		const auto sourceFragment = shaderManager.getShaderModuleFromSourceCode(device, "LF_Source_FS", ShaderType::FragmentShader, SourceFragmentShader);
		const auto ghostFragment = shaderManager.getShaderModuleFromSourceCode(device, "LF_Ghost_FS", ShaderType::FragmentShader, GhostFragmentShader);

		if ( sourceFragment == nullptr || ghostFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile the lens flare shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_sourcePipeline = this->createFullscreenPipeline(ClassId, "LF_Source", vertexModule, sourceFragment, m_sourceLayout, m_sourceTarget);
		m_ghostPipeline = this->createFullscreenPipeline(ClassId, "LF_Ghost", vertexModule, ghostFragment, m_ghostLayout, m_ghostTarget);

		if ( m_sourcePipeline == nullptr || m_ghostPipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		m_sourcePerFrame = this->createPerFrameDescriptorSets(sourceInputLayout, ClassId, "LF_Source_DescSet");
		m_ghostPerFrame = this->createPerFrameDescriptorSets(ghostInputLayout, ClassId, "LF_Ghost_DescSet");

		if ( m_sourcePerFrame.empty() || m_ghostPerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_ghostPerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(0, m_sourceTarget) )
			{
				return false;
			}
		}

		return true;
	}

	void
	LensFlare::destroy () noexcept
	{
		m_ghostPerFrame.clear();
		m_sourcePerFrame.clear();

		m_ghostPipeline.reset();
		m_sourcePipeline.reset();
		m_ghostLayout.reset();
		m_sourceLayout.reset();

		m_ghostTarget.destroy();
		m_sourceTarget.destroy();
	}

	void
	LensFlare::recordOverlayPasses (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();
		const auto aspect = static_cast< float >(m_sourceTarget.height()) / static_cast< float >(std::max(1U, m_sourceTarget.width()));

		SourcePushConstants sourcePC{
			.texelSizeX = 1.0F / static_cast< float >(m_sourceTarget.width()),
			.texelSizeY = 1.0F / static_cast< float >(m_sourceTarget.height()),
			.threshold = m_parameters.threshold,
			.softKnee = m_parameters.softKnee,
			.exposure = context.displayExposure > 0.0F ? context.displayExposure : 1.0F,
			.lightScreenX = 0.5F,
			.lightScreenY = 0.5F,
			.sunRadiusX = 0.0F,
			.sunRadiusY = 0.0F,
			.sunLuminanceR = 0.0F,
			.sunLuminanceG = 0.0F,
			.sunLuminanceB = 0.0F,
			.occlusionRadiusX = m_parameters.occlusionRadius * aspect,
			.occlusionRadiusY = m_parameters.occlusionRadius,
			.cloudTransmittanceEnabled = m_cloudTransmittance != nullptr ? 1.0F : 0.0F
		};

		/* ---- The analytic sun: the main directional light, projected. Without one, the bright pixels
		 * of the image alone make the ghosts (a lamp at night). ---- */
		if ( const auto mainLight = context.lightSet != nullptr ? context.lightSet->mainDirectionalLight() : nullptr; mainLight != nullptr && mainLight->isEnabled() )
		{
			/* readStateIndex: the view matrices that produced the depth buffer. */
			const auto readStateIndex = this->renderer().currentReadStateIndex();
			const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
			const auto & viewMatrix = viewMatrices.viewMatrix(readStateIndex, false, 0);
			const auto & projectionMatrix = viewMatrices.projectionMatrix(readStateIndex);
			const auto & cameraPosition = viewMatrices.position(readStateIndex);

			/* A far point toward the light (opposite of its propagation). */
			const auto toLight = (-mainLight->direction()).normalized();
			const Math::Vector< 4, float > farPoint{cameraPosition[0] + toLight[0] * 10000.0F, cameraPosition[1] + toLight[1] * 10000.0F, cameraPosition[2] + toLight[2] * 10000.0F, 1.0F};
			const auto clipPosition = projectionMatrix * (viewMatrix * farPoint);

			if ( clipPosition[3] > 0.001F )
			{
				const auto screenX = (clipPosition[0] / clipPosition[3]) * 0.5F + 0.5F;
				const auto screenY = (clipPosition[1] / clipPosition[3]) * 0.5F + 0.5F;

				/* Fade near the screen edges: a sun leaving the frame takes its ghosts with it. */
				const auto distanceFromCentre = std::hypot(screenX - 0.5F, screenY - 0.5F);
				const auto onScreen = std::clamp(1.5F - distanceFromCentre, 0.0F, 1.0F);

				if ( onScreen > 0.0F && m_parameters.sunDiscRadius > 0.0F )
				{
					/* The disc's solid angle: its radius is a fraction of the screen height, which spans
					 * 2 tan(fovY / 2) in tangent space (small angles). The light's illuminance spread
					 * over it is the disc's luminance — the flux of the sun, conserved. */
					const auto tanHalfFovY = 1.0F / std::max(std::abs(projectionMatrix(1, 1)), 1.0e-6F);
					const auto angularRadius = m_parameters.sunDiscRadius * 2.0F * tanHalfFovY;
					const auto solidAngle = std::numbers::pi_v< float > * angularRadius * angularRadius * SunDiscProfileArea;
					/* In DISPLAY units (the source target's unit), bounded under the half-float range.
					 * ⚠️ The ceiling holds on the BRIGHTEST CHANNEL: the emitted chromaticity has unit luminance, so a
					 * channel exceeds 1 (1.65 in red for a 2000 K sun, 13.85 in blue for a pure blue) and a ceiling on
					 * the scalar alone let it reach 82 000 at 2000 K, past the 65 504 of a half float — and the hue is
					 * kept. */
					const auto & color = mainLight->emissionChromaticity();
					const auto peakChannel = std::max({color[Base::Math::X], color[Base::Math::Y], color[Base::Math::Z], 1.0F});
					const auto luminance = std::min(mainLight->illuminance() / std::max(solidAngle, 1.0e-12F) * sourcePC.exposure * onScreen, MaxSunDisplayLuminance / peakChannel);

					sourcePC.lightScreenX = screenX;
					sourcePC.lightScreenY = screenY;
					sourcePC.sunRadiusX = m_parameters.sunDiscRadius * aspect;
					sourcePC.sunRadiusY = m_parameters.sunDiscRadius;
					sourcePC.sunLuminanceR = color[Base::Math::X] * luminance;
					sourcePC.sunLuminanceG = color[Base::Math::Y] * luminance;
					sourcePC.sunLuminanceB = color[Base::Math::Z] * luminance;
				}
			}
		}

		/* ---- Pass 1: the source (half resolution). ⚠️ Binding 2 is the clouds' transmittance when the
		 * stack paired one this frame; otherwise the depth stands in (a valid descriptor) and the flag
		 * tells the shader to ignore it. requiresDepth() guarantees the depth. ---- */
		auto & sourceSet = *m_sourcePerFrame[frameIndex];

		static_cast< void >(sourceSet.writeCombinedImageSampler(0, inputColor));
		static_cast< void >(sourceSet.writeCombinedImageSampler(1, *context.depth));
		static_cast< void >(sourceSet.writeCombinedImageSampler(2, m_cloudTransmittance != nullptr ? *m_cloudTransmittance : *context.depth));

		IndirectPostProcessEffect::recordFullscreenPass(commandBuffer, m_sourceTarget, *m_sourcePipeline, *m_sourceLayout, sourceSet, &sourcePC, sizeof(SourcePushConstants));

		/* ---- Pass 2: the ghosts and the halo (half resolution). ---- */
		const GhostPushConstants ghostPC{
			.ghostDispersal = m_parameters.ghostDispersal,
			.haloWidth = m_parameters.haloWidth,
			.chromaticDistortion = m_parameters.chromaticDistortion,
			.intensity = m_parameters.intensity,
			.ghostCount = m_parameters.ghostCount,
			.aspect = 1.0F / aspect,
			.inverseExposure = 1.0F / sourcePC.exposure,
			.haloIntensity = m_parameters.haloIntensity
		};

		IndirectPostProcessEffect::recordFullscreenPass(commandBuffer, m_ghostTarget, *m_ghostPipeline, *m_ghostLayout, *m_ghostPerFrame[frameIndex], &ghostPC, sizeof(GhostPushConstants));
	}

	IndirectPostProcessEffect::CombineContribution
	LensFlare::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		CombineContribution contribution;
		contribution.prefix = "lflare";
		contribution.samplers.emplace_back(CombineSamplerInput{"Tex", &m_ghostTarget});

		/* A pure additive blend of the ghosts: the sun's edge fade and visibility are already in the
		 * injected disc, and a bright source of the image needs neither. Alpha forced to 1. */
		contribution.code =
			"\tem_Color.rgb += texture(lflareTex, vUV).rgb;\n"
			"\tem_Color.a = 1.0;\n";

		return contribution;
	}
}
