/*
 * src/Graphics/Effects/Framebuffer/VolumetricLight.cpp
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

#include "VolumetricLight.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <string>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "PrimaryServices.hpp"
#include "Scenes/LightSet.hpp"
#include "SettingKeys.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

static constexpr auto TracerTag{"VolumetricLightEffect"};
/* NOLINTEND(cert-err58-cpp) */

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ---- */

	static constexpr auto OcclusionFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D previousMaskTex;

layout(push_constant) uniform PushConstants
{
	float lightScreenX;
	float lightScreenY;
	float texelSizeX;
	float texelSizeY;
	float nearPlane;
	float farPlane;
	float lightColorR;
	float lightColorG;
	float lightColorB;
	float lightIntensity;
	float density;
	float decay;
	float exposure;
	float depthThreshold;
	uint numSamples;
	float lightOnScreen;
	float jitterUVX;
	float jitterUVY;
	float temporalAlpha;
};

void main()
{
	/* JITTER COMPENSATION: the depth buffer is rasterized with the TAA sub-pixel offset,
	 * so its content belongs at (pixel - jitter) — sampling at vUV + jitterUV reads the
	 * value belonging AT vUV, keeping the mask silhouette position phase-stable. */
	vec2 sampleUV = vUV + vec2(jitterUVX, jitterUVY);

	/* Sky pixels emit light, geometry blocks it. FRACTIONAL mask: average the binary
	 * test over the 2x2 depth quad (one gather) instead of thresholding a single tap —
	 * anti-aliases the half-res mask edge. (Thresholding an AVERAGED depth would be
	 * wrong — the mean of a doorway depth and the far plane is meaningless; average the
	 * TEST results.) */
	vec4 quad = textureGather(depthTex, sampleUV, 0);
	float isLit = dot(vec4(greaterThanEqual(quad, vec4(depthThreshold))), vec4(0.25));

	vec3 lightColor = vec3(lightColorR, lightColorG, lightColorB);
	vec4 current = vec4(lightColor * lightIntensity * isLit, isLit);

	/* TEMPORAL EMA of the mask: a source narrower than a pixel (a door slit) RASTERIZES
	 * differently at every TAA jitter offset — its flux in the depth buffer genuinely
	 * oscillates with the Halton phase, and the radial march integrates that into a
	 * streak vibration. Stable sampling cannot fix a source that really changes
	 * (fractional mask and jitter compensation both measured neutral); averaging over
	 * the jitter cycle can. No reprojection: the mask is a soft, view-anchored quantity
	 * and the streaks are blurrier still — the lag at alpha 0.2 is ~8 frames. */
	outColor = mix(texture(previousMaskTex, vUV), current, temporalAlpha);
}
)GLSL";

	static constexpr auto RadialBlurFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D occlusionTex;

layout(push_constant) uniform PushConstants
{
	float lightScreenX;
	float lightScreenY;
	float texelSizeX;
	float texelSizeY;
	float nearPlane;
	float farPlane;
	float lightColorR;
	float lightColorG;
	float lightColorB;
	float lightIntensity;
	float density;
	float decay;
	float exposure;
	float depthThreshold;
	uint numSamples;
	float lightOnScreen;
	float jitterUVX;
	float jitterUVY;
	float temporalAlpha;
};

void main()
{
	vec2 lightPos = vec2(lightScreenX, lightScreenY);
	vec2 deltaUV = (vUV - lightPos);
	deltaUV *= (1.0 / float(numSamples)) * density;

	/* Dither the march start by a per-pixel fraction of ONE step (interleaved gradient
	 * noise — J. Jimenez, "Next Generation Post Processing in Call of Duty: Advanced
	 * Warfare", SIGGRAPH 2014). Without it, a small bright source (a door slit, a sky
	 * gap) falls BETWEEN the uniform taps and each surviving tap paints a discrete ghost
	 * copy of the source — a banded "dash train" along the radial direction, which then
	 * flickers with the TAA jitter phase (the occlusion mask is cut from the jittered
	 * depth buffer, and every sub-pixel wiggle of the silhouette replicates onto all
	 * copies). The dither dissolves the banding into unstructured sub-step noise that
	 * the radial accumulation and TAA smooth out. Deliberately STATIC per pixel: TAA
	 * integrates over its own jitter; a frame-varying dither would fight the history
	 * (measured on the RTGI animated-noise bench, engine caution-points). */
	float dither = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));

	vec2 sampleUV = vUV - deltaUV * dither;
	vec3 color = vec3(0.0);
	float illuminationDecay = 1.0;

	for (uint i = 0u; i < numSamples; ++i)
	{
		sampleUV -= deltaUV;
		vec3 sampleColor = texture(occlusionTex, sampleUV).rgb;
		sampleColor *= illuminationDecay;
		color += sampleColor;
		illuminationDecay *= decay;
	}

	color *= exposure / float(numSamples);
	color *= lightOnScreen;

	outColor = vec4(color, 1.0);
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
	VolumetricLight::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		/* Runtime OVERRIDES, absent by default. This effect had no settings key at all while EIGHT
		 * demos instantiated it with hand-tuned constants, so nothing about it could be A/B-ed at
		 * runtime — including against the world-space single-scattering pass meant to replace it.
		 *
		 * ⚠️ get(), NOT getOrSetDefault(), and the CURRENT parameter as the default. The TAA and
		 * MotionBlur contract (settings override the constructor, and register themselves in the
		 * file) is wrong here: five demos pass deliberately tuned values — Citadel 1.2/0.97/0.12/96,
		 * Liminal 0.6/0.98/0.12/96, LightAndShadowDebug and BasicScenery 0.8/0.98/0.12/64 — and an
		 * engine-wide default would silently double their god rays (exposure 0.12 vs 0.25). Worse,
		 * getOrSetDefault would let the FIRST demo run write its own values into a key the other
		 * seven then inherit. An absent key must change nothing; only a key the owner deliberately
		 * created takes over.
		 *
		 * ⚠️ None of these describes a participating medium: 'density' is a screen-space step
		 * multiplier and 'exposure' an arbitrary gain turning the light's LUX into the nits buffer.
		 * They all become meaningless the day the medium is real. */
		const auto & settings = renderer.primaryServices().settings();
		m_parameters.density = settings.get< float >(GraphicsVolumetricLightDensityKey, m_parameters.density);
		m_parameters.decay = settings.get< float >(GraphicsVolumetricLightDecayKey, m_parameters.decay);
		m_parameters.exposure = settings.get< float >(GraphicsVolumetricLightExposureKey, m_parameters.exposure);
		m_parameters.numSamples = settings.get< uint32_t >(GraphicsVolumetricLightSampleCountKey, m_parameters.numSamples);
		m_parameters.temporalAlpha = settings.get< float >(GraphicsVolumetricLightTemporalAlphaKey, m_parameters.temporalAlpha);

		constexpr auto format = VK_FORMAT_R16G16B16A16_SFLOAT;

		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		m_historyValid = false;
		m_occlusionWriteIndex = 0;

		/* Create occlusion targets (half-res, ping-pong: the pass blends the current
		 * binary test with the previous frame's mask — see the shader note). */
		for ( size_t index = 0; index < 2; ++index )
		{
			if ( !m_occlusionTargets[index].create(renderer, halfW, halfH, format, "VL_Occlusion" + std::to_string(index)) )
			{
				TraceError{TracerTag} << "Failed to create occlusion target #" << index << " !";

				return false;
			}
		}

		/* Create radial blur target (half-res). */
		if ( !m_radialTarget.create(renderer, halfW, halfH, format, "VL_Radial") )
		{
			TraceError{TracerTag} << "Failed to create radial blur target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Occlusion: depth + previous mask. Radial: occlusion mask. */
		auto dualInputLayout = this->getInputLayout(2);
		auto singleInputLayout = this->getInputLayout(1);

		if ( dualInputLayout == nullptr || singleInputLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(dualInputLayout);

			m_occlusionLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScatterPushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(singleInputLayout);

			m_radialLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScatterPushConstants)}
			});
		}

		if ( m_occlusionLayout == nullptr || m_radialLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto vertexModule = this->getFullscreenVertexShader();

		if ( vertexModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile vertex shader !";

			return false;
		}

		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto occlusionFragment = shaderManager.getShaderModuleFromSourceCode(device, "VL_Occlusion_FS", ShaderType::FragmentShader, OcclusionFragmentShader);

		if ( occlusionFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile occlusion shader !";

			return false;
		}

		const auto radialFragment = shaderManager.getShaderModuleFromSourceCode(device, "VL_Radial_FS", ShaderType::FragmentShader, RadialBlurFragmentShader);

		if ( radialFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile radial blur shader !";

			return false;
		}

		/* ---- Create pipelines ---- */
		/* NOTE: The occlusion pipeline is created against the [0] target; recording into
		 * [1] relies on Vulkan render pass compatibility (identical format/ops), exactly
		 * like the GIDenoiser history ping-pong. */
		m_occlusionPipeline = this->createFullscreenPipeline(ClassId, "VL_Occlusion", vertexModule, occlusionFragment, m_occlusionLayout, m_occlusionTargets[0]);
		m_radialPipeline = this->createFullscreenPipeline(ClassId, "VL_Radial", vertexModule, radialFragment, m_radialLayout, m_radialTarget);

		if ( m_occlusionPipeline == nullptr || m_radialPipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets (all per-frame: the ping-pong bindings rotate) ---- */

		m_occlusionPerFrame = this->createPerFrameDescriptorSets(dualInputLayout, ClassId, "VL_Occlusion_DescSet");
		m_radialPerFrame = this->createPerFrameDescriptorSets(singleInputLayout, ClassId, "VL_Radial_DescSet");

		if ( m_occlusionPerFrame.empty() || m_radialPerFrame.empty() )
		{
			return false;
		}

		return true;
	}

	void
	VolumetricLight::destroy () noexcept
	{
		m_radialPerFrame.clear();
		m_occlusionPerFrame.clear();

		m_radialPipeline.reset();
		m_occlusionPipeline.reset();
		m_radialLayout.reset();
		m_occlusionLayout.reset();

		m_radialTarget.destroy();

		for ( auto & target : m_occlusionTargets )
		{
			target.destroy();
		}

		m_historyValid = false;
		m_occlusionWriteIndex = 0;
	}

	void
	VolumetricLight::recordOverlayPasses (const CommandBuffer & commandBuffer, const TextureInterface & /*inputColor*/, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * lightSet = context.lightSet;
		const auto & constants = context.constants;


		const auto frameIndex = this->renderer().currentFrameIndex();

		/* 1. Project light direction to screen space.
		 * Use readStateIndex to match the view matrix that produced the depth buffer. */
		const auto readStateIndex = this->renderer().currentReadStateIndex();
		const auto & viewMatrices =this->renderer().mainRenderTarget()->viewMatrices();
		const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
		/* UNJITTERED: the light screen position is the origin of every radial sampling
		 * line. Projected through the jittered matrix it wobbles with the Halton phase,
		 * and the whole line of taps shifts sub-pixel across an occlusion source that can
		 * be 1-2 half-res texels wide (a door slit) — the streaks vibrate. The position
		 * is a GEOMETRIC anchor, not a depth-buffer lookup: it must be phase-stable. */
		const auto & projMat = viewMatrices.unjitteredProjectionMatrix(readStateIndex);
		const auto & camPos = viewMatrices.position(readStateIndex);

		/* Light source direction (opposite of emission direction). */
		const auto mainLight = lightSet->mainDirectionalLight();
		const auto lightSource = (-mainLight->direction()).normalized();
		const auto lightColor = m_lightColorOverride.value_or(mainLight->color());
		const auto lightIntensity = m_lightIntensityOverride.value_or(mainLight->intensity());

		/* Project a far point along the light source direction. */
		const auto farPointX = camPos[0] + lightSource.x() * 10000.0F;
		const auto farPointY = camPos[1] + lightSource.y() * 10000.0F;
		const auto farPointZ = camPos[2] + lightSource.z() * 10000.0F;

		/* Transform to view space (Matrix<4> * Vector<4>). */
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

		/* Fade based on distance from screen center. */
		const auto dx = screenX - 0.5F;
		const auto dy = screenY - 0.5F;
		const auto distFromCenter = std::sqrt(dx * dx + dy * dy);
		lightOnScreen *= std::max(0.0F, std::min(1.0F, 1.0F - distFromCenter * 0.5F));

		/* 2. Update per-frame descriptors: depth + previous mask (ping-pong) for the
		 * occlusion pass, this frame's mask for the radial pass. */
		const uint32_t writeIdx = m_occlusionWriteIndex;
		const uint32_t readIdx = 1U - writeIdx;

		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_occlusionPerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		static_cast< void >(m_occlusionPerFrame[frameIndex]->writeCombinedImageSampler(1, m_occlusionTargets[readIdx]));
		static_cast< void >(m_radialPerFrame[frameIndex]->writeCombinedImageSampler(0, m_occlusionTargets[writeIdx]));

		/* Build scatter push constants (shared by occlusion and radial passes). */
		const ScatterPushConstants scatterPC{
			.lightScreenX = screenX,
			.lightScreenY = screenY,
			.texelSizeX = 1.0F / static_cast< float >(m_occlusionTargets[0].width()),
			.texelSizeY = 1.0F / static_cast< float >(m_occlusionTargets[0].height()),
			.nearPlane = constants.nearPlane,
			.farPlane = constants.farPlane,
			.lightColorR = lightColor.red(),
			.lightColorG = lightColor.green(),
			.lightColorB = lightColor.blue(),
			.lightIntensity = lightIntensity,
			.density = m_parameters.density,
			.decay = m_parameters.decay,
			.exposure = m_parameters.exposure,
			.depthThreshold = m_parameters.depthThreshold,
			.numSamples = m_parameters.numSamples,
			.lightOnScreen = lightOnScreen,
			/* NDC jitter -> UV units (frame-history contract, same as the TAA resolve). */
			.jitterUVX = context.projectionJitter.x() * 0.5F,
			.jitterUVY = context.projectionJitter.y() * 0.5F,
			/* First frame after (re)creation: the previous-mask image is uninitialised. */
			.temporalAlpha = m_historyValid ? m_parameters.temporalAlpha : 1.0F
		};

		/* 4. Pass 1: Occlusion extraction + temporal EMA. */
		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_occlusionTargets[writeIdx],
			*m_occlusionPipeline,
			*m_occlusionLayout,
			*m_occlusionPerFrame[frameIndex],
			&scatterPC,
			sizeof(ScatterPushConstants)
		);

		/* 5. Pass 2: Radial blur. */
		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_radialTarget,
			*m_radialPipeline,
			*m_radialLayout,
			*m_radialPerFrame[frameIndex],
			&scatterPC,
			sizeof(ScatterPushConstants)
		);

		/* Flip the mask ping-pong for the next frame. */
		m_occlusionWriteIndex = readIdx;
		m_historyValid = true;
	}

	IndirectPostProcessEffect::CombineContribution
	VolumetricLight::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		CombineContribution contribution;
		contribution.prefix = "vlight";
		contribution.samplers.emplace_back(CombineSamplerInput{"Tex", &m_radialTarget});

		/* Same math as the retired VL_Composite_FS pass: pure additive blend of the
		 * radially blurred light shafts (the lightOnScreen fade is already baked into
		 * the radial pass output), alpha forced to 1 as the original composite did. */
		contribution.code =
			"\tem_Color.rgb += texture(vlightTex, vUV).rgb;\n"
			"\tem_Color.a = 1.0;\n";

		return contribution;
	}
}
