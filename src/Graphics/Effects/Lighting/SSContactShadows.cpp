/*
 * src/Graphics/Effects/Lighting/SSContactShadows.cpp
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

#include "SSContactShadows.hpp"

/* Local inclusions. */
#include "Graphics/Effects/Shared/MarchDitherGLSL.hpp"

/* STL inclusions. */
#include <cstring>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Scenes/LightSet.hpp"
#include "SettingKeys.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

static constexpr auto TracerTag{"SSContactShadowsEffect"};

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ---- */

	constexpr auto SSShadowFragmentShader = R"GLSL(
#version 460

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

/* Input textures and parameters (set 0). No RT set, no bindless set: this lane reads the
 * G-buffer and nothing else. */
layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D normalTex;

layout(set = 0, binding = 2, std140) uniform MarchParams
{
	mat4 inverseProjectionMatrix;	/* clip -> view */
	mat4 projectionMatrix;			/* view -> clip */
	vec4 lightParameters;			/* xyz = light EMISSION direction (VIEW space), w = maxDistance */
	vec4 marchParameters;			/* x = normalBias, y = thickness, z = step count, w = unused */
};
)GLSL" EMEN_MARCH_DITHER_GLSL R"GLSL(

/* Clip -> view. ⚠️ The uv -> ndc mapping is a plain `uv * 2 - 1`, with NO Y flip, because that
 * is exactly what the ray-traced sibling does and its reconstruction is proven correct on
 * screen. The engine flipped Y inside the PROJECTION matrix (Aug 2026), so the flip must not be
 * applied a second time here — that is the mirror defect all over again. */
vec3 reconstructViewPosition(vec2 uv, float depth)
{
	vec2 ndc = uv * 2.0 - 1.0;
	vec4 viewPos = inverseProjectionMatrix * vec4(ndc, depth, 1.0);

	return viewPos.xyz / viewPos.w;
}

void main()
{
	const float maxDistance = lightParameters.w;
	const float normalBias = marchParameters.x;
	const float thickness = marchParameters.y;
	const int stepCount = int(marchParameters.z);

	const ivec2 depthSize = textureSize(depthTex, 0);
	const ivec2 texCoord = ivec2(vUV * vec2(depthSize));
	const float rawDepth = texelFetch(depthTex, texCoord, 0).r;

	/* Sky pixels (depth at the far plane) are never in contact with anything. */
	if (rawDepth >= 0.9999)
	{
		outColor = vec4(1.0, 1.0, 0.0, 1.0);
		return;
	}

	const vec4 normalData = texelFetch(normalTex, texCoord, 0);
	const vec3 rawN = normalData.rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outColor = vec4(1.0, 1.0, 0.0, 1.0);
		return;
	}

	/* Reconstruct from the texel that was actually FETCHED, not from vUV. */
	const vec2 texelUV = (vec2(texCoord) + 0.5) / vec2(depthSize);
	const vec3 viewPos = reconstructViewPosition(texelUV, rawDepth);
	const vec3 viewNormal = normalize(rawN);

	/* ⚠️ The sign of the view-space forward axis is READ FROM THE DATA, not assumed. This pixel
	 * is in front of the camera by construction (the sky was rejected above), so the sign of its
	 * view-space z IS the convention, whichever the projection uses. Guessing it wrong would
	 * invert every depth comparison below and the effect would shadow exactly the pixels it
	 * should leave lit — a failure that looks like a tuning problem, not like a sign error. */
	const float forwardSign = viewPos.z < 0.0 ? -1.0 : 1.0;
	const float viewDepthHere = viewPos.z * forwardSign;

	/* Adaptive bias, same rule as the ray-traced sibling: it grows with camera distance (the
	 * pixel footprint grows) and with the grazing angle (a ray leaving a nearly edge-on surface
	 * clips its own neighbourhood). */
	const vec3 viewDir = normalize(viewPos);
	const float NdotV = max(abs(dot(viewNormal, -viewDir)), 0.001);
	const float grazingFactor = min(1.0 / NdotV, 10.0);
	const float adaptiveBias = normalBias * max(1.0, viewDepthHere) * grazingFactor;

	/* ⚠️⚠️ Offset the ray ORIGIN along the NORMAL. Never start the march further along the light
	 * direction instead: at the terminator the light is grazing, so advancing along it never
	 * leaves the surface, and raising the start distance skips the near occluders a contact
	 * shadow exists to draw. The ray-traced sibling learned this as a FACETED terminator on the
	 * DamagedHelmet dome — 19 axis-aligned steps, 72.3 % of the boundary perfectly flat. */
	const vec3 rayOrigin = viewPos + viewNormal * adaptiveBias;

	/* Toward the light, in view space. */
	const vec3 lightDir = normalize(-lightParameters.xyz);

	/* A surface facing away from the light is fully shadowed by its own geometry; marching would
	 * only find its own back side. */
	if (dot(viewNormal, lightDir) <= 0.0)
	{
		outColor = vec4(1.0, 1.0, 0.0, 1.0);
		return;
	}

	const float stepLength = maxDistance / float(stepCount);

	/* ⚠️ Dither the march ORIGIN, static per pixel. Uniform steps do not undersample into noise,
	 * they undersample into coherent BANDING, and banding is TAA-hostile. Never mix a frame index
	 * in: TAA integrates over its own jitter and a frame-varying dither fights the history. */
	const float jitter = emInterleavedGradientNoise(gl_FragCoord.xy);

	float shadow = 1.0;
	float normalizedHitDist = 1.0;

	for (int i = 0; i < stepCount; ++i)
	{
		const float travelled = (float(i) + jitter) * stepLength;
		const vec3 samplePos = rayOrigin + lightDir * travelled;

		/* Project the step back onto the screen. */
		const vec4 clipPos = projectionMatrix * vec4(samplePos, 1.0);

		if (clipPos.w <= 0.0)
		{
			break;
		}

		const vec2 sampleUV = (clipPos.xy / clipPos.w) * 0.5 + 0.5;

		/* ⚠️ The STRUCTURAL ceiling of this lane: an occluder that is not on screen casts
		 * nothing. Measured on SSR, 43.3 % of the rays leave the screen, and raising the step
		 * count bought nothing there. Leaving the frame ends the march unshadowed rather than
		 * clamping to the edge, which would smear the border pixel across the whole margin. */
		if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
		{
			break;
		}

		const float sceneDepth = texture(depthTex, sampleUV).r;

		if (sceneDepth >= 0.9999)
		{
			continue;
		}

		const vec3 sceneViewPos = reconstructViewPosition(sampleUV, sceneDepth);
		const float sceneViewDepth = sceneViewPos.z * forwardSign;
		const float sampleViewDepth = samplePos.z * forwardSign;

		/* How far BEHIND the visible surface the ray currently is. Positive means the ray passed
		 * behind something. */
		const float behind = sampleViewDepth - sceneViewDepth;

		/* ⚠️ The depth buffer is a heightfield: it says where a surface STARTS, never how deep it
		 * goes. `thickness` is the assumption that fills that in, and it is the knob that decides
		 * whether this effect looks right — too thin and light leaks through thin geometry, too
		 * thick and a shadow halo trails behind every occluder. The lower bound rejects the ray's
		 * own surface. */
		if (behind > adaptiveBias && behind < thickness)
		{
			shadow = smoothstep(0.0, maxDistance, travelled);
			normalizedHitDist = clamp(travelled / maxDistance, 0.0, 1.0);
			break;
		}
	}

	/* Contact shadows are a near-field effect: fade them out with camera distance, exactly like
	 * the ray-traced sibling, so a lane switch does not change WHERE shadows exist. */
	const float shadowFade = clamp(viewDepthHere / (maxDistance * 10.0), 0.0, 1.0);
	shadow = mix(shadow, 1.0, shadowFade);

	/* R = shadow factor, G = normalized contact distance (the PCSS-lite blur radius). Same
	 * encoding as RTContactShadows — the shared denoise and the combine must not know which
	 * occupant of the slot produced it. */
	outColor = vec4(shadow, normalizedHitDist, 0.0, 1.0);
}
)GLSL";

}

namespace EmEn::Graphics::Effects::Lighting
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	/* ---- Lifecycle ---- */

	bool
	SSContactShadows::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();
		auto & settings = renderer.primaryServices().settings();

		m_parameters.maxDistance = settings.getOrSetDefault< float >(GraphicsPPContactShadowsSSMaxDistanceKey, DefaultGraphicsPPContactShadowsSSMaxDistance);
		m_parameters.normalBias = settings.getOrSetDefault< float >(GraphicsPPContactShadowsSSNormalBiasKey, DefaultGraphicsPPContactShadowsSSNormalBias);
		m_parameters.intensity = settings.getOrSetDefault< float >(GraphicsPPContactShadowsSSIntensityKey, DefaultGraphicsPPContactShadowsSSIntensity);
		m_parameters.maxBlurRadius = settings.getOrSetDefault< float >(GraphicsPPContactShadowsSSMaxBlurRadiusKey, DefaultGraphicsPPContactShadowsSSMaxBlurRadius);
		m_parameters.thickness = settings.getOrSetDefault< float >(GraphicsPPContactShadowsSSThicknessKey, DefaultGraphicsPPContactShadowsSSThickness);
		m_parameters.stepCount = settings.getOrSetDefault< uint32_t >(GraphicsPPContactShadowsSSStepCountKey, DefaultGraphicsPPContactShadowsSSStepCount);

		if ( m_parameters.stepCount < 1 )
		{
			m_parameters.stepCount = 1;
		}

		/* ⚠️ FULL resolution — see the class note. The ray-traced sibling halves it to amortise
		 * ray traversal; a depth march does not pay that, and halving would blur away the fine
		 * contact detail this effect exists to produce. */
		if ( !m_shadowTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "SSCS_Shadow") )
		{
			TraceError{TracerTag} << "Failed to create shadow target !";

			return false;
		}

		if ( !m_blurHTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "SSCS_BlurH") )
		{
			TraceError{TracerTag} << "Failed to create horizontal blur target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "SSCS_BlurV") )
		{
			TraceError{TracerTag} << "Failed to create vertical blur target !";

			return false;
		}

		/* Set 0: depth (0), normals (1), per-frame parameters (2). */
		m_shadowInputLayout = this->getInputLayout(2, 1);

		if ( m_shadowInputLayout == nullptr )
		{
			return false;
		}

		auto & layoutManager = renderer.layoutManager();

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(m_shadowInputLayout);
			m_shadowLayout = layoutManager.getPipelineLayout(sets, {});
		}

		if ( m_shadowLayout == nullptr )
		{
			return false;
		}

		const auto vertexModule = this->getFullscreenVertexShader();

		if ( vertexModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile vertex shader !";

			return false;
		}

		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto shadowFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSCS_Shadow_FS", ShaderType::FragmentShader, SSShadowFragmentShader);

		if ( shadowFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile the screen-space contact shadow shader !";

			return false;
		}

		m_shadowPipeline = this->createFullscreenPipeline(ClassId, "SSCS_Shadow", vertexModule, shadowFragment, m_shadowLayout, m_shadowTarget);

		if ( m_shadowPipeline == nullptr )
		{
			return false;
		}

		m_shadowPerFrame = this->createPerFrameDescriptorSets(m_shadowInputLayout, ClassId, "SSCS_Shadow_DescSet");

		if ( m_shadowPerFrame.empty() )
		{
			return false;
		}

		m_shadowFrameUBOs = this->createPerFrameUniformBuffers(sizeof(MarchFrameUBOData), ClassId, "SSCS_Shadow_Frame_UBO");

		if ( m_shadowFrameUBOs.size() != m_shadowPerFrame.size() )
		{
			TraceError{TracerTag} << "Failed to create the per-frame march parameter buffers !";

			return false;
		}

		for ( size_t frameIndex = 0; frameIndex < m_shadowPerFrame.size(); ++frameIndex )
		{
			if ( !m_shadowPerFrame[frameIndex]->writeUniformBufferObject(2, *m_shadowFrameUBOs[frameIndex]) )
			{
				TraceError{TracerTag} << "Failed to bind the march parameter buffer for frame " << frameIndex << " !";

				return false;
			}
		}

		return true;
	}

	void
	SSContactShadows::destroy () noexcept
	{
		m_shadowFrameUBOs.clear();
		m_shadowPerFrame.clear();

		m_shadowPipeline.reset();
		m_shadowLayout.reset();
		m_shadowInputLayout.reset();

		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_shadowTarget.destroy();
	}

	void
	SSContactShadows::recordPreDenoisePasses (const CommandBuffer & commandBuffer, const TextureInterface & /*inputColor*/, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputNormals = context.normals;
		const auto * lightSet = context.lightSet;

		if ( lightSet == nullptr || lightSet->mainDirectionalLight() == nullptr )
		{
			return;
		}

		const auto frameIndex = this->renderer().currentFrameIndex();

		/* ⚠️ readStateIndex, not the default overload: it must be the very matrices that produced
		 * the depth buffer being marched. The default reads the LOGIC state, which may already
		 * have advanced — the reconstruction would then be one frame off and the shadow would
		 * flicker with camera motion. */
		const auto readStateIndex = this->renderer().currentReadStateIndex();
		const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
		const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
		const auto & projMat = viewMatrices.projectionMatrix(readStateIndex);
		const auto invProjMat = projMat.inverse();

		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_shadowPerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_shadowPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		MarchFrameUBOData marchData{};
		std::memcpy(marchData.inverseProjectionMatrix.data(), invProjMat.data(), marchData.inverseProjectionMatrix.size() * sizeof(float));
		std::memcpy(marchData.projectionMatrix.data(), projMat.data(), marchData.projectionMatrix.size() * sizeof(float));

		/* The light EMISSION direction, rotated into VIEW space. ⚠️ The rotation only — a
		 * direction has no position — so the view matrix's three rotation columns are applied by
		 * hand rather than multiplying a vec4 the translation would pollute. Column-major
		 * storage: columns are elements {0,1,2}, {4,5,6}, {8,9,10}. */
		const auto lightDirection = lightSet->mainDirectionalLight()->direction();
		const auto * view = viewMat.data();
		const auto viewLightX = view[0] * lightDirection.x() + view[4] * lightDirection.y() + view[8] * lightDirection.z();
		const auto viewLightY = view[1] * lightDirection.x() + view[5] * lightDirection.y() + view[9] * lightDirection.z();
		const auto viewLightZ = view[2] * lightDirection.x() + view[6] * lightDirection.y() + view[10] * lightDirection.z();

		marchData.lightParameters = {viewLightX, viewLightY, viewLightZ, m_parameters.maxDistance};
		marchData.marchParameters = {m_parameters.normalBias, m_parameters.thickness, static_cast< float >(m_parameters.stepCount), 0.0F};

		if ( !updateUniformBufferData(*m_shadowFrameUBOs[frameIndex], &marchData, sizeof(marchData)) )
		{
			TraceError{TracerTag} << "Failed to update the march parameter buffer !";

			return;
		}

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_shadowTarget,
			*m_shadowPipeline,
			*m_shadowLayout,
			*m_shadowPerFrame[frameIndex],
			nullptr,
			0
		);
	}

	IndirectPostProcessEffect::DenoiseContribution
	SSContactShadows::denoiseContribution (const FrameContext & /*context*/) const noexcept
	{
		DenoiseContribution contribution;
		contribution.prefix = "sscs";
		contribution.source = &m_shadowTarget;
		contribution.targetH = const_cast< IntermediateRenderTarget * >(&m_blurHTarget);
		contribution.targetV = const_cast< IntermediateRenderTarget * >(&m_blurVTarget);
		contribution.dynamics = {m_parameters.maxBlurRadius, 0.0F, 0.0F, 0.0F};

		/* The SAME PCSS-lite kernel as the ray-traced sibling: a 9-tap gaussian whose radius
		 * scales with the normalized contact distance (G channel), passing through below half a
		 * texel. Identical on purpose — a lane A/B must not also be a blur A/B. */
		contribution.code =
			"\tvec2 sscsTexel = 1.0 / vec2(textureSize(sscsSrc, 0));\n"
			"\tvec4 sscsCenter = texture(sscsSrc, vUV);\n"
			"\tfloat sscsHitDist = sscsCenter.g;\n"
			"\tfloat sscsRadius = emDyn.sscsDynamics0.x * sscsHitDist;\n"
			"\tvec4 sscsResult = sscsCenter;\n"
			"\tif (sscsRadius >= 0.5)\n"
			"\t{\n"
			"\t\tconst float sscsWeights[5] = float[](0.227027, 0.194596, 0.121621, 0.054054, 0.016216);\n"
			"\t\tvec2 sscsStep = emDenoiseDir * sscsTexel;\n"
			"\t\tfloat sscsSum = sscsCenter.r * sscsWeights[0];\n"
			"\t\tfor (int sscsI = 1; sscsI < 5; sscsI++)\n"
			"\t\t{\n"
			"\t\t\tvec2 sscsOffset = sscsStep * (float(sscsI) / 4.0 * sscsRadius);\n"
			"\t\t\tsscsSum += texture(sscsSrc, vUV + sscsOffset).r * sscsWeights[sscsI];\n"
			"\t\t\tsscsSum += texture(sscsSrc, vUV - sscsOffset).r * sscsWeights[sscsI];\n"
			"\t\t}\n"
			"\t\tsscsResult = vec4(sscsSum, sscsHitDist, 0.0, 1.0);\n"
			"\t}\n";

		return contribution;
	}

	IndirectPostProcessEffect::CombineContribution
	SSContactShadows::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		CombineContribution contribution;
		contribution.prefix = "sscs";
		contribution.samplers.emplace_back(CombineSamplerInput{"Tex", &m_blurVTarget});
		contribution.needsMaterialProperties = true;
		contribution.dynamics.emplace_back(Base::Math::Vector< 4, float >{m_parameters.intensity, 0.0F, 0.0F, 0.0F});

		/* Identical to the ray-traced sibling: user intensity, then the material shadowResponse
		 * (LOW nibble of mp.g — the HIGH nibble is the aoResponse). */
		contribution.code =
			"\tfloat sscsShadow = texture(sscsTex, vUV).r;\n"
			"\tvec4 sscsMp = texture(emMaterialProps, vUV);\n"
			"\tfloat sscsResponse = float(uint(sscsMp.g * 255.0) & 0xFu) / 15.0;\n"
			"\tsscsShadow = mix(1.0, sscsShadow, emDyn.sscsDynamics0.x);\n"
			"\tsscsShadow = mix(1.0, sscsShadow, sscsResponse);\n"
			"\tem_Color.rgb *= sscsShadow;\n";

		return contribution;
	}
}
