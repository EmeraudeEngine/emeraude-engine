/*
 * src/Graphics/Effects/Framebuffer/RTAO.cpp
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

#include "RTAO.hpp"

/* Local inclusions. */
#include "RTAlphaTestGLSL.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
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

	/* RTAO trace pass: casts short hemisphere rays against the TLAS.
	 * Each pixel sends N rays in a cosine-weighted hemisphere around the surface normal.
	 * Output: single-channel occlusion factor (1.0 = fully lit, 0.0 = fully occluded).
	 *
	 * Descriptor set 0 (RT data — bound from Renderer::rtDescriptorSet()):
	 *   binding 0: accelerationStructureEXT (TLAS)
	 *
	 * Descriptor set 1 (input textures — per-frame):
	 *   binding 0: depth texture
	 *   binding 1: normals texture
	 */
	static constexpr auto RTAOTraceFragmentShader = R"GLSL(
#version 460
#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec2 outAO; /* R = AO, G = depth (for bilateral blur). */

/* RT data (set 0): the TLAS, and the mesh/material SSBOs the alpha-test rule reads. */
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
)GLSL" EMEN_RT_SCENE_DATA_GLSL(2) EMEN_RT_ALPHA_TEST_GLSL_FUNCTIONS R"GLSL(

/* Input textures (set 1). */
layout(set = 1, binding = 0) uniform sampler2D depthTex;
layout(set = 1, binding = 1) uniform sampler2D normalTex;

layout(push_constant) uniform PushConstants
{
	mat4 invViewProj;
	vec3 invViewCol0; float viewPosX;
	vec3 invViewCol1; float viewPosY;
	vec3 invViewCol2; float viewPosZ;
	float maxDistance;
	float intensity;
	float bias;
	uint sampleCount;
};

/* Hash function for pseudo-random sampling. */
float hash (vec2 p)
{
	return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
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

void main()
{
	/* Use texelFetch (no bilinear filtering) to avoid interpolating
	 * depth/normals across geometric edges at half-resolution. */
	ivec2 fullResCoord = ivec2(vUV * vec2(textureSize(depthTex, 0)));
	float depth = texelFetch(depthTex, fullResCoord, 0).r;

	/* Skip far-plane fragments. */
	if (depth >= 1.0)
	{
		outAO = vec2(1.0, depth);
		return;
	}

	/* Read view-space normal from MRT. */
	vec4 normalData = texelFetch(normalTex, fullResCoord, 0);
	vec3 rawN = normalData.rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outAO = vec2(1.0, depth);
		return;
	}

	/* Reconstruct world-space position from NDC + depth via inverse VP. */
	vec2 ndc = vUV * 2.0 - 1.0;
	vec4 clipPos = vec4(ndc, depth, 1.0);
	vec4 wp = invViewProj * clipPos;
	vec3 worldPos = wp.xyz / wp.w;

	/* Transform view-space normal to world space. */
	mat3 invViewRot = mat3(invViewCol0, invViewCol1, invViewCol2);
	vec3 worldNormal = normalize(invViewRot * normalize(rawN));

	/* Build a tangent frame (TBN) around the world normal for hemisphere sampling. */
	vec3 up = abs(worldNormal.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 tangent = normalize(cross(up, worldNormal));
	vec3 bitangent = cross(worldNormal, tangent);
	mat3 TBN = mat3(tangent, bitangent, worldNormal);

	/* World-space noise: camera-independent sampling to prevent flickering on rotation. */
	vec2 noiseVec = vec2(hash(worldPos.xz), hash(worldPos.yz));

	/* Adaptive bias: scale with camera distance AND grazing angle.
	 * Distance: pixel footprint grows → needs larger offset.
	 * NdotV: at grazing angles, rays easily clip the surface → needs extra offset. */
	vec3 viewDir = normalize(worldPos - vec3(viewPosX, viewPosY, viewPosZ));
	float cameraDist = length(worldPos - vec3(viewPosX, viewPosY, viewPosZ));
	float NdotV = max(abs(dot(worldNormal, -viewDir)), 0.001);
	float grazingFactor = 1.0 / NdotV; /* Grows as view becomes more grazing. */
	float adaptiveBias = bias * max(1.0, cameraDist) * min(grazingFactor, 10.0);

	/* Distance fadeout: AO is a near-field effect.
	 * Fade to 1.0 (no occlusion) beyond maxDistance * 20 from camera. */
	float aoFadeRange = maxDistance * 20.0;
	float aoFade = clamp(cameraDist / aoFadeRange, 0.0, 1.0);

	/* Offset ray origin along normal to prevent self-intersection. */
	vec3 rayOrigin = worldPos + worldNormal * adaptiveBias;

	/* Accumulate occlusion: count how many rays are blocked. */
	float occlusion = 0.0;

	for (uint i = 0u; i < sampleCount; ++i)
	{
		vec3 sampleDir = TBN * hemispherePoint(i, noiseVec);

		/* Ensure the sample direction is in the hemisphere of the normal. */
		if (dot(sampleDir, worldNormal) < 0.0)
		{
			sampleDir = -sampleDir;
		}

		/* Trace a short ray in the sampled direction.
		 * tMin is a tiny CONSTANT, decoupled from the adaptive origin offset: the offset
		 * alone prevents self-intersection (hemisphere directions never descend below the
		 * surface), while a tMin equal to the adaptive bias SKIPS real occluders closer
		 * than it — at wall/floor creases this erased the occlusion entirely and drew a
		 * bright line exactly where the AO must be darkest (worse with distance/grazing,
		 * the adaptive bias can exceed a metre). */
		/* ⚠️ NOT gl_RayFlagsOpaqueEXT: a cutout instance (foliage) hands its triangles over as
		 * candidates and the opaque flag accepted them whole — a leaf occluded as a solid quad,
		 * and the ground under a tree was darkened by the card, not by the leaves. The shared
		 * alpha-test rule judges each candidate (RTAlphaTestGLSL.hpp); TerminateOnFirstHit
		 * still ends the traversal at the first CONFIRMED one. */
		rayQueryEXT rayQuery;
		rayQueryInitializeEXT(
			rayQuery, topLevelAS,
			gl_RayFlagsTerminateOnFirstHitEXT,
			0xFF,
			rayOrigin, 0.001, sampleDir, maxDistance
		);

		while (rayQueryProceedEXT(rayQuery))
		{
)GLSL" EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE(rayQuery) R"GLSL(
		}

		/* If the ray hit something, this sample is occluded. */
		if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
		{
			/* Distance-weighted occlusion: closer hits occlude more. */
			float hitT = rayQueryGetIntersectionTEXT(rayQuery, true);
			float weight = 1.0 - clamp(hitT / maxDistance, 0.0, 1.0);
			occlusion += weight;
		}
	}

	/* Pure visibility term — the user-facing intensity is applied ONCE, in the apply pass.
	 * It used to be multiplied here AND fed to an extrapolating mix() there (t > 1), a
	 * double application that made default RTAO far too dark (same defect as SSAO). */
	occlusion = occlusion / float(sampleCount);
	float ao = clamp(1.0 - occlusion, 0.0, 1.0);
	outAO = vec2(mix(ao, 1.0, aoFade), depth);
}
)GLSL";

}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	RTAO::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		auto & settings = renderer.primaryServices().settings();

		/* User-facing parameters, engine-wide and persisted in the settings file.
		 * These override any constructor-provided values. */
		m_parameters.sampleCount = settings.getOrSetDefault< uint32_t >(GraphicsRayTracingAOSampleCountKey, DefaultGraphicsRayTracingAOSampleCount);
		m_parameters.intensity = settings.getOrSetDefault< float >(GraphicsRayTracingAOIntensityKey, DefaultGraphicsRayTracingAOIntensity);
		m_parameters.bias = settings.getOrSetDefault< float >(GraphicsRayTracingAOBiasKey, DefaultGraphicsRayTracingAOBias);
		m_parameters.maxDistance = settings.getOrSetDefault< float >(GraphicsRayTracingAOMaxDistanceKey, DefaultGraphicsRayTracingAOMaxDistance);
		m_parameters.blurRadius = settings.getOrSetDefault< uint32_t >(GraphicsRayTracingAOBlurRadiusKey, DefaultGraphicsRayTracingAOBlurRadius);
		m_parameters.normalSigma = settings.getOrSetDefault< float >(GraphicsRayTracingAONormalSigmaKey, DefaultGraphicsRayTracingAONormalSigma);

		/* Pixel doubling: half-res for performance (default), full-res for quality. */
		const auto pixelDoubling = settings.getOrSetDefault< bool >(GraphicsRayTracingAOPixelDoublingKey, DefaultGraphicsRayTracingAOPixelDoubling);
		const auto halfW = pixelDoubling ? ((width > 1) ? width / 2 : 1U) : width;
		const auto halfH = pixelDoubling ? ((height > 1) ? height / 2 : 1U) : height;

		/* Trace target (half-res, RG16F: AO + depth for bilateral blur). */
		if ( !m_traceTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16_SFLOAT, "RTAO_Trace") )
		{
			TraceError{ClassId} << "Failed to create RTAO trace target !";

			return false;
		}

		/* Blur targets (half-res, RG16F: AO + depth pass-through). */
		if ( !m_blurHTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16_SFLOAT, "RTAO_BlurH") )
		{
			TraceError{ClassId} << "Failed to create RTAO blur H target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16_SFLOAT, "RTAO_BlurV") )
		{
			TraceError{ClassId} << "Failed to create RTAO blur V target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Trace input (set 1): depth + normals — 2 combined image samplers. */
		auto traceInputLayout = this->getInputLayout(2);

		if ( traceInputLayout == nullptr )
		{
			return false;
		}

		/* RT descriptor set layout (set 0) — from the Renderer. */
		auto rtLayout = renderer.rtDescriptorSetLayout();

		if ( rtLayout == nullptr )
		{
			TraceError{ClassId} << "RT descriptor set layout not available !";

			return false;
		}

		/* Bindless texture descriptor set layout (set 2) — from BindlessTextureManager. The
		 * alpha-test rule samples the opacity/albedo texture of a cutout candidate. */
		auto bindlessLayout = renderer.bindlessTextureManager().descriptorSetLayout();

		if ( bindlessLayout == nullptr )
		{
			TraceError{ClassId} << "Bindless texture descriptor set layout not available !";

			return false;
		}

		/* ---- Pipeline layouts ---- */
		{
			/* Trace: set 0 = RT data (TLAS + SSBOs), set 1 = depth + normals, set 2 = bindless textures. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(rtLayout);
			sets.emplace_back(traceInputLayout);
			sets.emplace_back(bindlessLayout);

			m_traceLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TracePushConstants)}
			});
		}

		if ( m_traceLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		auto vertexModule = this->getFullscreenVertexShader();
		auto traceFragment = shaderManager.getShaderModuleFromSourceCode(device, "RTAO_Trace_FS", ShaderType::FragmentShader, RTAOTraceFragmentShader);

		if ( vertexModule == nullptr || traceFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile RTAO shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = this->createFullscreenPipeline(ClassId, "RTAO_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);

		if ( m_tracePipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		/* Trace: set 1 reads depth + normals (updated per-frame). */
		m_tracePerFrame = this->createPerFrameDescriptorSets(traceInputLayout, ClassId, "Trace_DescSet");

		if ( m_tracePerFrame.empty() )
		{
			return false;
		}

		return true;
	}

	void
	RTAO::destroy () noexcept
	{
		m_tracePerFrame.clear();

		m_tracePipeline.reset();
		m_traceLayout.reset();

		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_traceTarget.destroy();
	}

	void
	RTAO::recordPreDenoisePasses (const CommandBuffer & commandBuffer, const TextureInterface & /*inputColor*/, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputNormals = context.normals;


		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Update depth + normals descriptors for this frame's trace pass. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		/* ---- Pass 1: Ray Trace AO ---- */
		{
			/* Use readStateIndex for the SAME view matrix that produced the depth buffer. */
			const auto readStateIndex = this->renderer().currentReadStateIndex();
			const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
			const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
			const auto & projMat = viewMatrices.projectionMatrix(readStateIndex);
			const auto invViewProj = (projMat * viewMat).inverse();
			const auto * ivp = invViewProj.data();

			/* Inverse view rotation for normal transformation (view → world). */
			const auto invView = viewMat.inverse();
			const auto * inv = invView.data();

			const TracePushConstants pc{
				.invViewProj = {
					ivp[0], ivp[1], ivp[2], ivp[3],
					ivp[4], ivp[5], ivp[6], ivp[7],
					ivp[8], ivp[9], ivp[10], ivp[11],
					ivp[12], ivp[13], ivp[14], ivp[15]
				},
				.invViewCol0 = {inv[0], inv[1], inv[2]},
				.viewPosX = inv[12],
				.invViewCol1 = {inv[4], inv[5], inv[6]},
				.viewPosY = inv[13],
				.invViewCol2 = {inv[8], inv[9], inv[10]},
				.viewPosZ = inv[14],
				.maxDistance = m_parameters.maxDistance,
				.intensity = m_parameters.intensity,
				.bias = m_parameters.bias,
				.sampleCount = m_parameters.sampleCount
			};

			/* Custom recording: bind set 0 (RT) from Renderer, set 1 (input textures) per-frame. */
			m_traceTarget.beginRenderPass(commandBuffer);

			commandBuffer.bind(*m_tracePipeline);

			const VkViewport viewport{
				.x = 0.0F,
				.y = 0.0F,
				.width = static_cast< float >(m_traceTarget.width()),
				.height = static_cast< float >(m_traceTarget.height()),
				.minDepth = 0.0F,
				.maxDepth = 1.0F
			};
			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{
				.offset = {0, 0},
				.extent = {m_traceTarget.width(), m_traceTarget.height()}
			};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			vkCmdPushConstants(
				commandBuffer.handle(),
				m_traceLayout->handle(),
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(TracePushConstants),
				&pc
			);

			/* Bind set 0: RT descriptor set (TLAS). */
			if ( const auto * rtDescSet = this->renderer().rtDescriptorSet(); rtDescSet != nullptr )
			{
				commandBuffer.bind(*rtDescSet, *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			}

			/* Bind set 1: Input textures (depth + normals). */
			commandBuffer.bind(*m_tracePerFrame[frameIndex], *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 1);

			/* Bind set 2: Bindless textures (the alpha-test rule samples cutout textures). */
			if ( const auto * bindlessDescSet = this->renderer().bindlessTextureManager().descriptorSet(); bindlessDescSet != nullptr )
			{
				commandBuffer.bind(*bindlessDescSet, *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 2);
			}

			commandBuffer.draw(3, 1);

			m_traceTarget.endRenderPass(commandBuffer);
		}
	}

	IndirectPostProcessEffect::DenoiseContribution
	RTAO::denoiseContribution (const FrameContext & /*context*/) const noexcept
	{
		DenoiseContribution contribution;
		contribution.prefix = "rtao";
		contribution.source = &m_traceTarget;
		contribution.targetH = const_cast< IntermediateRenderTarget * >(&m_blurHTarget);
		contribution.targetV = const_cast< IntermediateRenderTarget * >(&m_blurVTarget);
		contribution.needsNormals = true;
		contribution.dynamics = {m_parameters.normalSigma, static_cast< float >(m_parameters.blurRadius), 0.0F, 0.0F};

		/* Same bilateral kernel as the retired RTAO_Blur_FS pass: gaussian spatial term,
		 * depth edge-stopping from the source's own G channel, normal edge-stopping from
		 * the shared normals guide, far-plane early-out (center pass-through). */
		contribution.code =
			"\tvec2 rtaoTexel = 1.0 / vec2(textureSize(rtaoSrc, 0));\n"
			"\tvec2 rtaoCenter = texture(rtaoSrc, vUV).rg;\n"
			"\tfloat rtaoCenterDepth = rtaoCenter.g;\n"
			"\tvec4 rtaoResult = vec4(rtaoCenter, 0.0, 1.0);\n"
			"\tif (rtaoCenterDepth < 1.0)\n"
			"\t{\n"
			"\t\tvec3 rtaoCenterNormal = texture(emNormals, vUV).rgb;\n"
			"\t\tint rtaoRadius = int(emDyn.rtaoDynamics0.y);\n"
			"\t\tfloat rtaoSpatialSigma = float(rtaoRadius) * 0.5;\n"
			"\t\tfloat rtaoInvSpatialSigma2 = 1.0 / (2.0 * rtaoSpatialSigma * rtaoSpatialSigma);\n"
			"\t\tfloat rtaoSum = 0.0;\n"
			"\t\tfloat rtaoTotalWeight = 0.0;\n"
			"\t\tfor (int rtaoI = -rtaoRadius; rtaoI <= rtaoRadius; rtaoI++)\n"
			"\t\t{\n"
			"\t\t\tvec2 rtaoSampleUV = vUV + emDenoiseDir * rtaoTexel * float(rtaoI);\n"
			"\t\t\tvec2 rtaoS = texture(rtaoSrc, rtaoSampleUV).rg;\n"
			"\t\t\tvec3 rtaoSampleNormal = texture(emNormals, rtaoSampleUV).rgb;\n"
			"\t\t\tfloat rtaoSpatialW = exp(-float(rtaoI * rtaoI) * rtaoInvSpatialSigma2);\n"
			"\t\t\tfloat rtaoDepthDiff = abs(rtaoS.g - rtaoCenterDepth);\n"
			"\t\t\tfloat rtaoDepthW = exp(-rtaoDepthDiff * rtaoDepthDiff * 10000.0);\n"
			"\t\t\tfloat rtaoNormalDot = max(dot(rtaoCenterNormal, rtaoSampleNormal), 0.0);\n"
			"\t\t\tfloat rtaoNormalW = pow(rtaoNormalDot, 1.0 / max(emDyn.rtaoDynamics0.x, 0.001));\n"
			"\t\t\tfloat rtaoW = rtaoSpatialW * rtaoDepthW * rtaoNormalW;\n"
			"\t\t\trtaoSum += rtaoS.r * rtaoW;\n"
			"\t\t\trtaoTotalWeight += rtaoW;\n"
			"\t\t}\n"
			"\t\trtaoResult = vec4(rtaoSum / max(rtaoTotalWeight, 0.001), rtaoCenterDepth, 0.0, 1.0);\n"
			"\t}\n";

		return contribution;
	}

	IndirectPostProcessEffect::CombineContribution
	RTAO::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		CombineContribution contribution;
		contribution.prefix = "rtao";
		contribution.samplers.emplace_back(CombineSamplerInput{"Tex", &m_blurVTarget});
		contribution.needsMaterialProperties = true;
		contribution.dynamics.emplace_back(Base::Math::Vector< 4, float >{m_parameters.intensity, 0.0F, 0.0F, 0.0F});

		/* Same math as the retired RTAO_Apply_FS pass: user intensity (clamped mix, an
		 * intensity above 1 saturates rather than extrapolating), then the material
		 * aoResponse with emissive rejection. */
		contribution.code =
			"\tfloat rtaoAO = texture(rtaoTex, vUV).r;\n"
			"\tvec4 rtaoMp = texture(emMaterialProps, vUV);\n"
			"\tfloat rtaoResponse = float(uint(rtaoMp.g * 255.0) >> 4u) / 15.0;\n"
			"\tfloat rtaoEmissive = float(uint(rtaoMp.b * 255.0) & 0xFu) / 15.0;\n"
			"\trtaoAO = clamp(mix(1.0, rtaoAO, emDyn.rtaoDynamics0.x), 0.0, 1.0);\n"
			"\trtaoAO = mix(1.0, rtaoAO, rtaoResponse * (1.0 - rtaoEmissive));\n"
			"\tem_Color.rgb *= rtaoAO;\n";

		return contribution;
	}
}
