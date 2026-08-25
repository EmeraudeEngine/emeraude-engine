/*
 * src/Graphics/Effects/Framebuffer/SSGI.cpp
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

#include "SSGI.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

namespace
{
	using namespace EmEn;

	/* SSGI trace pass: one-bounce diffuse indirect lighting via screen-space ray marching.
	 * For each pixel, casts cosine-weighted hemisphere rays through the depth buffer.
	 * On hit, samples the scene color at the hit UV to produce indirect radiance
	 * (color bleeding). This is the screen-space approximation of RTGI.
	 *
	 * Descriptor set 0 (input textures — per-frame):
	 *   binding 0: depth texture
	 *   binding 1: normals texture
	 *   binding 2: scene color texture (HDR, for bounce color sampling)
	 */
	constexpr auto SSGITraceFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outIndirect;

layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D normalTex;
layout(set = 0, binding = 2) uniform sampler2D colorTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float nearPlane;
	float farPlane;
	float tanHalfFovY;
	float aspectRatio;
	float maxDistance;
	float thickness;
	uint sampleCount;
	uint stepCount;
	float noiseFrameIndex;	/* R2 sequence index; < 0 = frozen pattern. */
};

/* Linearize depth from [0,1] range (Vulkan [0,1] depth convention). */
float linearizeDepth (float depth)
{
	return (nearPlane * farPlane) / (farPlane - depth * (farPlane - nearPlane));
}

/* Reconstruct view-space position from UV and depth. */
vec3 reconstructPosition (vec2 uv, float depth)
{
	float linearZ = linearizeDepth(depth);
	vec2 ndc = uv * 2.0 - 1.0;
	float t = tanHalfFovY;
	return vec3(ndc * vec2(abs(t) * aspectRatio, t) * linearZ, linearZ);
}

/* Project view-space position back to screen UV. */
vec2 projectToUV (vec3 viewPos)
{
	float t = tanHalfFovY;
	vec2 ndc = viewPos.xy / (viewPos.z * vec2(abs(t) * aspectRatio, t));
	return ndc * 0.5 + 0.5;
}

/* PCG integer hash → decorrelated white noise from integer pixel coordinates (same
 * upgrade as RTGI: the former fract(sin(dot(...))) hash has float-precision beating and
 * produced a fixed grid/banding pattern the spatial denoiser cannot remove). */
uint pcgHash (uint v)
{
	v = v * 747796405u + 2891336453u;
	uint s = ((v >> ((v >> 28u) + 4u)) ^ v) * 277803737u;
	return (s >> 22u) ^ s;
}

vec2 hash2 (uvec2 p)
{
	uint h = pcgHash(p.x + pcgHash(p.y));
	return vec2(float(h & 0xffffu), float((h >> 16u) & 0xffffu)) * (1.0 / 65535.0);
}

/* Generate a cosine-weighted hemisphere sample direction. */
vec3 hemispherePoint (uint i, vec2 noise)
{
	float fi = float(i);
	float angle = fi * 2.399963 + noise.x * 6.283185;
	float r = sqrt((fi + 0.5) / float(sampleCount));
	float z = sqrt(1.0 - r * r);
	return vec3(cos(angle) * r, sin(angle) * r, z);
}

/* Screen-edge fade: 0 at edges, 1 at center. */
float screenEdgeFade (vec2 uv)
{
	vec2 fade = smoothstep(vec2(0.0), vec2(0.05), uv)
			  * smoothstep(vec2(0.0), vec2(0.05), vec2(1.0) - uv);
	return fade.x * fade.y;
}

void main()
{
	float centerDepth = texture(depthTex, vUV).r;

	/* Skip far-plane fragments. */
	if (centerDepth >= 1.0)
	{
		outIndirect = vec4(0.0);
		return;
	}

	vec3 centerPos = reconstructPosition(vUV, centerDepth);

	/* Read view-space normal from MRT normal buffer. */
	vec3 rawN = texture(normalTex, vUV).rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outIndirect = vec4(0.0);
		return;
	}

	/* Convert to reconstruction space (Z negated: linearDepth is positive,
	 * view-space Z is negative for objects in front of the camera). */
	vec3 normal = normalize(vec3(rawN.x, rawN.y, -rawN.z));

	/* Per-pixel random rotation to break banding. Temporal decorrelation: advance the
	 * rotation every frame along the R2 low-discrepancy sequence (Roberts 2018) so the
	 * GIDenoiser resolve AVERAGES the estimator error instead of freezing it as a static
	 * pattern (stable outliers read as "converged signal" the variance guide protects).
	 * The index is negative when the temporal chain is off: animated noise without
	 * accumulation boils. */
	vec2 noiseVec = hash2(uvec2(gl_FragCoord.xy));

	if (noiseFrameIndex >= 0.0)
	{
		noiseVec = fract(noiseVec + noiseFrameIndex * vec2(0.7548776662, 0.5698402909));
	}

	/* Build a tangent-space basis around the view-space normal.
	 * Robust construction: pick an up vector not parallel to the normal, then cross (same method
	 * as RTGI). The previous noise-based Gram-Schmidt degenerated to normalize(0) = NaN whenever the
	 * noise vector aligned with the normal (side walls seen edge-on) — the NaN then propagated
	 * through the apply pass (color += NaN) and blacked out whole surfaces. The per-sample random
	 * rotation is already provided by hemispherePoint() via noiseVec, so a deterministic basis here
	 * is fine. */
	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	/* Compute stride length from max distance and step count. */
	float strideLen = maxDistance / float(stepCount);

	/* Adaptive stride: scale with depth so distant pixels cover more ground. */
	float adaptiveStride = strideLen * max(1.0, centerPos.z * 0.1);

	/* Accumulate indirect radiance. */
	vec3 indirectLight = vec3(0.0);

	for (uint i = 0u; i < sampleCount; ++i)
	{
		vec3 sampleDir = TBN * hemispherePoint(i, noiseVec);

		/* Ensure the sample direction is in the hemisphere of the normal. */
		if (dot(sampleDir, normal) < 0.0)
		{
			sampleDir = -sampleDir;
		}

		/* Ray march through the depth buffer. Instead of requiring the ray to LAND inside a thin
		 * thickness window (which a coarse march over-steps — the reason SSGI missed adjacent visible
		 * light while SSR, with 128 fine steps, did not), we detect the FRONT->BEHIND crossing
		 * (prevDiff < 0, diff > 0) and binary-refine the intersection. This catches surfaces the
		 * step size would otherwise skip, at a fraction of SSR's per-ray budget. */
		bool hit = false;
		vec2 hitUV = vec2(0.0);
		float hitDist = 0.0;

		float prevDiff = -1.0;
		vec3 prevRayPos = centerPos;

		for (uint s = 1u; s <= stepCount; ++s)
		{
			vec3 rayPos = centerPos + sampleDir * adaptiveStride * float(s);

			/* Project to screen space. */
			vec2 sampleUV = projectToUV(rayPos);

			/* Out of screen bounds. */
			if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
			{
				break;
			}

			/* Compare depth at the projected position. */
			float sampleDepth = linearizeDepth(texture(depthTex, sampleUV).r);
			float diff = rayPos.z - sampleDepth;

			/* Adaptive thickness based on distance (distant surfaces need larger threshold). */
			float adaptiveThick = thickness * max(1.0, sampleDepth * 0.05);

			/* Crossing: the ray went from in front of the surface to behind it. */
			if (prevDiff < 0.0 && diff > 0.0)
			{
				/* Binary-refine the intersection between the last two samples. */
				vec3 lo = prevRayPos;
				vec3 hi = rayPos;

				for (uint b = 0u; b < 8u; ++b)
				{
					vec3 mid = 0.5 * (lo + hi);
					float midDepth = linearizeDepth(texture(depthTex, projectToUV(mid)).r);

					if (mid.z - midDepth > 0.0)
					{
						hi = mid;
					}
					else
					{
						lo = mid;
					}
				}

				vec2 refUV = projectToUV(hi);
				float refDepth = linearizeDepth(texture(depthTex, refUV).r);

				/* Reject false crossings (silhouette / background gap): after refinement the ray
				 * must sit just behind the surface, not far behind it. */
				if ((hi.z - refDepth) < adaptiveThick)
				{
					hitUV = refUV;
					hitDist = length(hi - centerPos);
					hit = true;
					break;
				}
			}

			prevDiff = diff;
			prevRayPos = rayPos;
		}

		if (hit)
		{
			/* Sample scene color at the hit point (the indirect bounce). */
			vec3 hitColor = texture(colorTex, hitUV).rgb;

			/* Distance attenuation: closer bounces contribute more. */
			float distFade = 1.0 - clamp(hitDist / maxDistance, 0.0, 1.0);

			/* Screen edge fade at hit point to avoid artifacts at screen borders. */
			float edgeFade = screenEdgeFade(hitUV);

			/* hitColor is already the hit surface's outgoing radiance (the lit colour buffer),
			 * i.e. L_i. With cosine-weighted sampling the diffuse estimate is albedo * mean(L_i)
			 * (the receiver albedo is applied in the apply pass / omitted for white surfaces);
			 * there must be NO extra 1/PI here — dividing by PI made SSGI ~3.14x too dark. */
			indirectLight += hitColor * distFade * edgeFade;
		}
	}

	/* Normalize by sample count. Intensity is applied in the apply pass. */
	indirectLight = indirectLight / float(sampleCount);

	/* Safety net: never let a NaN/Inf or negative value reach the apply pass (color += gi would
	 * otherwise black out or blow up the pixel). */
	if (any(isnan(indirectLight)) || any(isinf(indirectLight)))
	{
		indirectLight = vec3(0.0);
	}

	indirectLight = max(indirectLight, vec3(0.0));

	outIndirect = vec4(indirectLight, 1.0);
}
)GLSL";

}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	SSGI::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		auto & settings = renderer.primaryServices().settings();

		/* User-facing parameters, engine-wide and persisted in the settings file.
		 * These override any constructor-provided values. */
		m_parameters.maxDistance = settings.getOrSetDefault< float >(GraphicsScreenSpaceGIMaxDistanceKey, DefaultGraphicsScreenSpaceGIMaxDistance);
		m_parameters.intensity = settings.getOrSetDefault< float >(GraphicsScreenSpaceGIIntensityKey, DefaultGraphicsScreenSpaceGIIntensity);
		m_parameters.thickness = settings.getOrSetDefault< float >(GraphicsScreenSpaceGIThicknessKey, DefaultGraphicsScreenSpaceGIThickness);
		m_parameters.sampleCount = settings.getOrSetDefault< uint32_t >(GraphicsScreenSpaceGISampleCountKey, DefaultGraphicsScreenSpaceGISampleCount);
		m_parameters.stepCount = settings.getOrSetDefault< uint32_t >(GraphicsScreenSpaceGIStepCountKey, DefaultGraphicsScreenSpaceGIStepCount);
		m_parameters.depthSigma = settings.getOrSetDefault< float >(GraphicsScreenSpaceGIDepthSigmaKey, DefaultGraphicsScreenSpaceGIDepthSigma);
		m_parameters.normalSigma = settings.getOrSetDefault< float >(GraphicsScreenSpaceGINormalSigmaKey, DefaultGraphicsScreenSpaceGINormalSigma);
		m_parameters.luminanceSigma = settings.getOrSetDefault< float >(GraphicsScreenSpaceGIDenoiserLuminanceSigmaKey, DefaultGraphicsScreenSpaceGIDenoiserLuminanceSigma);
		m_parameters.atrousIterations = settings.getOrSetDefault< uint32_t >(GraphicsScreenSpaceGIDenoiserIterationsKey, DefaultGraphicsScreenSpaceGIDenoiserIterations);
		m_parameters.temporalAlpha = settings.getOrSetDefault< float >(GraphicsScreenSpaceGITemporalAlphaKey, DefaultGraphicsScreenSpaceGITemporalAlpha);
		m_parameters.temporalDepthTolerance = settings.getOrSetDefault< float >(GraphicsScreenSpaceGITemporalDepthToleranceKey, DefaultGraphicsScreenSpaceGITemporalDepthTolerance);
		m_parameters.temporalNormalThreshold = settings.getOrSetDefault< float >(GraphicsScreenSpaceGITemporalNormalThresholdKey, DefaultGraphicsScreenSpaceGITemporalNormalThreshold);
		m_parameters.temporalVarianceGamma = settings.getOrSetDefault< float >(GraphicsScreenSpaceGITemporalVarianceGammaKey, DefaultGraphicsScreenSpaceGITemporalVarianceGamma);
		m_parameters.denoiserMaxAccumulation = settings.getOrSetDefault< uint32_t >(GraphicsScreenSpaceGIDenoiserMaxAccumulationKey, DefaultGraphicsScreenSpaceGIDenoiserMaxAccumulation);
		m_parameters.denoiserDebugView = settings.getOrSetDefault< uint32_t >(GraphicsScreenSpaceGIDenoiserDebugViewKey, DefaultGraphicsScreenSpaceGIDenoiserDebugView);
		m_parameters.denoiserAccumulationCounter = settings.getOrSetDefault< bool >(GraphicsScreenSpaceGIDenoiserAccumulationCounterKey, DefaultGraphicsScreenSpaceGIDenoiserAccumulationCounter);
		m_parameters.temporalEnabled = settings.getOrSetDefault< bool >(GraphicsScreenSpaceGITemporalEnabledKey, DefaultGraphicsScreenSpaceGITemporalEnabled);
		m_parameters.temporalNeighborhoodClamp = settings.getOrSetDefault< bool >(GraphicsScreenSpaceGITemporalNeighborhoodClampKey, DefaultGraphicsScreenSpaceGITemporalNeighborhoodClamp);
		m_parameters.temporalAnimatedNoise = settings.getOrSetDefault< bool >(GraphicsScreenSpaceGITemporalAnimatedNoiseKey, DefaultGraphicsScreenSpaceGITemporalAnimatedNoise);

		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		/* Trace target (half-res, RGBA16F: indirect radiance RGB). */
		if ( !m_traceTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "SSGI_Trace") )
		{
			TraceError{ClassId} << "Failed to create SSGI trace target !";

			return false;
		}

		/* The denoiser component (temporal resolve + moments + à-trous + histories) —
		 * SSGI's FIRST temporal accumulation. */
		m_denoiser.setTemporalEnabled(m_parameters.temporalEnabled);
		m_denoiser.setParameters(GIDenoiser::Parameters{
			.depthSigma = m_parameters.depthSigma,
			.normalSigma = m_parameters.normalSigma,
			.luminanceSigma = m_parameters.luminanceSigma,
			.atrousIterations = m_parameters.atrousIterations,
			.temporalAlpha = m_parameters.temporalAlpha,
			.temporalDepthTolerance = m_parameters.temporalDepthTolerance,
			.temporalNormalThreshold = m_parameters.temporalNormalThreshold,
			.temporalVarianceGamma = m_parameters.temporalVarianceGamma,
			.maxAccumulation = m_parameters.denoiserMaxAccumulation,
			.temporalNeighborhoodClamp = m_parameters.temporalNeighborhoodClamp,
			.temporalAnimatedNoise = m_parameters.temporalAnimatedNoise,
			.accumulationCounter = m_parameters.denoiserAccumulationCounter
		});

		if ( !m_denoiser.create(halfW, halfH) )
		{
			TraceError{ClassId} << "Failed to create the SSGI denoiser component !";

			return false;
		}

		/* ---- Descriptor set layouts (shared) ---- */
		auto tripleLayout = this->getInputLayout(3);

		if ( tripleLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(tripleLayout);

			m_traceLayout = layoutManager.getPipelineLayout(sets, {VkPushConstantRange{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(TracePushConstants)
			}});
		}

		if ( m_traceLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto traceFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSGI_Trace_FS", ShaderType::FragmentShader, SSGITraceFragmentShader);

		if ( vertexModule == nullptr || traceFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile SSGI shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = this->createFullscreenPipeline(ClassId, "SSGI_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);

		if ( m_tracePipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */

		/* Trace: reads depth + normals + scene color (all updated per-frame). */
		m_tracePerFrame = this->createPerFrameDescriptorSets(tripleLayout, ClassId, "Trace_DescSet");

		if ( m_tracePerFrame.empty() )
		{
			return false;
		}

		/* Combine source default: the raw trace. recordOverlayPasses() retargets it to the
		 * denoiser output every frame when the temporal chain is active. */
		m_combineSource = &m_traceTarget;

		return true;
	}

	void
	SSGI::destroy () noexcept
	{
		m_combineSource = nullptr;

		m_tracePerFrame.clear();

		m_tracePipeline.reset();
		m_traceLayout.reset();

		m_denoiser.destroy();

		m_traceTarget.destroy();
	}

	void
	SSGI::recordOverlayPasses (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputNormals = context.normals;
		const auto & constants = context.constants;

		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Update depth + normals + scene color descriptors for this frame's trace pass. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(2, inputColor));

		/* ---- Frame UBO of the denoiser (matrices + temporal parameters; SSGI has no
		 * feedback loop and no sky term — the trace scalars travel through its own push
		 * constants, only the noise index below is shared). ---- */
		const bool animated = m_denoiser.temporalActive() && m_parameters.temporalAnimatedNoise;
		const auto noiseFrameIndex = static_cast< float >(m_denoiser.noiseFrameIndex());

		static_cast< void >(m_denoiser.updateFrameData(frameIndex, context, GIDenoiser::FrameInputs{}));

		/* ---- Pass 1: Screen-Space GI Trace ---- */
		{
			const TracePushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_traceTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_traceTarget.height()),
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.tanHalfFovY = constants.tanHalfFovY,
				.aspectRatio = constants.frameWidth / constants.frameHeight,
				.maxDistance = m_parameters.maxDistance,
				.thickness = m_parameters.thickness,
				.sampleCount = m_parameters.sampleCount,
				.stepCount = m_parameters.stepCount,
				.noiseFrameIndex = animated ? noiseFrameIndex : -1.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_traceTarget,
				*m_tracePipeline,
				*m_traceLayout,
				*m_tracePerFrame[frameIndex],
				&pc,
				sizeof(TracePushConstants)
			);
		}

		/* ---- Denoise chain (SVGF order): temporal resolve on the RAW trace + moments
		 * accumulation + normal history, then the variance-guided à-trous iterations. */
		m_combineSource = m_denoiser.recordResolve(commandBuffer, m_traceTarget, context);
	}

	IndirectPostProcessEffect::CombineContribution
	SSGI::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		/* Denoiser debug views (diagnostic): draw the denoiser internals INSTEAD of the GI
		 * contribution. */
		if ( m_parameters.denoiserDebugView != 0U && m_denoiser.temporalActive() )
		{
			return m_denoiser.debugCombineContribution("ssgi", m_parameters.denoiserDebugView);
		}

		CombineContribution contribution;
		contribution.prefix = "ssgi";
		contribution.samplers.emplace_back(CombineSamplerInput{"Tex", m_combineSource});
		contribution.needsMaterialProperties = true;
		contribution.needsAlbedo = true;
		contribution.dynamics.emplace_back(Base::Math::Vector< 4, float >{m_parameters.intensity, 0.0F, 0.0F, 0.0F});

		/* Same math as the retired SSGI_Apply_FS pass: emissive surfaces reject GI
		 * (they emit their own light), the indirect diffuse is modulated by the
		 * receiver's albedo (albedo * irradiance — without it a coloured surface lit
		 * only by indirect light shows the raw incoming grey light), then the user
		 * intensity scales the additive blend. */
		contribution.code =
			"\tvec3 ssgiGI = texture(ssgiTex, vUV).rgb;\n"
			"\tvec4 ssgiMp = texture(emMaterialProps, vUV);\n"
			"\tfloat ssgiEmissive = float(uint(ssgiMp.b * 255.0) & 0xFu) / 15.0;\n"
			"\tssgiGI *= (1.0 - ssgiEmissive);\n"
			"\tssgiGI *= texture(emAlbedo, vUV).rgb;\n"
			"\tem_Color.rgb += ssgiGI * emDyn.ssgiDynamics0.x;\n";

		return contribution;
	}
}
