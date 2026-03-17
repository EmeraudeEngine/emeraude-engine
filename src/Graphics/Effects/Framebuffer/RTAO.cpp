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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "RTAO.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ShaderModule.hpp"

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

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec2 outAO; /* R = AO, G = depth (for bilateral blur). */

/* RT data (set 0). */
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;

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

		/* Trace a short ray in the sampled direction. */
		rayQueryEXT rayQuery;
		rayQueryInitializeEXT(
			rayQuery, topLevelAS,
			gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT,
			0xFF,
			rayOrigin, adaptiveBias, sampleDir, maxDistance
		);

		while (rayQueryProceedEXT(rayQuery)) {}

		/* If the ray hit something, this sample is occluded. */
		if (rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionTriangleEXT)
		{
			/* Distance-weighted occlusion: closer hits occlude more. */
			float hitT = rayQueryGetIntersectionTEXT(rayQuery, true);
			float weight = 1.0 - clamp(hitT / maxDistance, 0.0, 1.0);
			occlusion += weight;
		}
	}

	/* Normalize and apply intensity, then fade out at distance. */
	occlusion = (occlusion / float(sampleCount)) * intensity;
	float ao = clamp(1.0 - occlusion, 0.0, 1.0);
	outAO = vec2(mix(ao, 1.0, aoFade), depth);
}
)GLSL";

	/* Bilateral blur shader — depth-weighted Gaussian to preserve geometric edges. */
	static constexpr auto RTAOBlurFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec2 outBlur; /* R = blurred AO, G = depth (pass-through center). */

layout(set = 0, binding = 0) uniform sampler2D inputTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float directionX;
	float directionY;
};

void main()
{
	vec2 texelSize = vec2(texelSizeX, texelSizeY);
	vec2 dir = vec2(directionX, directionY);

	vec2 center = texture(inputTex, vUV).rg;
	float centerDepth = center.g;

	/* 9-tap bilateral Gaussian blur: spatial weight * depth weight. */
	float result = 0.0;
	float total = 0.0;

	for (int i = -4; i <= 4; i++)
	{
		vec2 s = texture(inputTex, vUV + dir * texelSize * float(i)).rg;

		float spatialW = 5.0 - abs(float(i));
		float depthDiff = abs(s.g - centerDepth);
		float depthW = exp(-depthDiff * depthDiff * 10000.0);
		float w = spatialW * depthW;

		result += s.r * w;
		total += w;
	}

	outBlur = vec2(result / max(total, 0.001), centerDepth);
}
)GLSL";

	/* Apply pass: multiply scene color by AO factor. */
	static constexpr auto RTAOApplyFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D aoTex;
layout(set = 0, binding = 2) uniform sampler2D materialPropsTex;

layout(push_constant) uniform PushConstants
{
	float intensity;
	float padding1;
	float padding2;
	float padding3;
};

void main()
{
	vec4 color = texture(colorTex, vUV);
	float ao = texture(aoTex, vUV).r;

	/* Decode material properties from G-buffer. */
	vec4 mp = texture(materialPropsTex, vUV);
	uint gPacked = uint(mp.g * 255.0);
	float aoResponse = float(gPacked >> 4u) / 15.0;
	uint bPacked = uint(mp.b * 255.0);
	float emissiveMask = float(bPacked & 0xFu) / 15.0;

	/* Mix towards full AO based on intensity. */
	ao = mix(1.0, ao, intensity);

	/* Modulate AO by material aoResponse; emissive surfaces reject AO darkening. */
	ao = mix(1.0, ao, aoResponse * (1.0 - emissiveMask));

	outColor = vec4(color.rgb * ao, color.a);
}
)GLSL";
}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Vulkan;

	bool
	RTAO::create (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

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

		/* Apply target (full-res, RGBA16F). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "RTAO_Output") )
		{
			TraceError{ClassId} << "Failed to create RTAO output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Trace input (set 1): depth + normals — 2 combined image samplers. */
		auto traceInputLayout = getInputLayout(renderer, 2);

		/* Single input (blur): 1 combined image sampler. */
		auto singleLayout = getInputLayout(renderer, 1);

		/* Apply input (color + blurred AO + material properties): 3 combined image samplers. */
		auto applyLayout = getInputLayout(renderer, 3);

		if ( traceInputLayout == nullptr || singleLayout == nullptr || applyLayout == nullptr )
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

		/* ---- Pipeline layouts ---- */
		{
			/* Trace: set 0 = RT data (TLAS only), set 1 = depth + normals. */
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(TracePushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(rtLayout);
			sets.emplace_back(traceInputLayout);
			m_traceLayout = layoutManager.getPipelineLayout(sets, ranges);
		}
		{
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlurPushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(singleLayout);
			m_blurLayout = layoutManager.getPipelineLayout(sets, ranges);
		}
		{
			const Libs::StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ApplyPushConstants)}
			};

			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(applyLayout);
			m_applyLayout = layoutManager.getPipelineLayout(sets, ranges);
		}

		if ( m_traceLayout == nullptr || m_blurLayout == nullptr || m_applyLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		auto vertexModule = getFullscreenVertexShader(renderer);
		auto traceFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "RTAO_Trace_FS", Saphir::ShaderType::FragmentShader, RTAOTraceFragmentShader
		);
		auto blurFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "RTAO_Blur_FS", Saphir::ShaderType::FragmentShader, RTAOBlurFragmentShader
		);
		auto applyFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "RTAO_Apply_FS", Saphir::ShaderType::FragmentShader, RTAOApplyFragmentShader
		);

		if ( vertexModule == nullptr || traceFragment == nullptr || blurFragment == nullptr || applyFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile RTAO shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_tracePipeline = createFullscreenPipeline(renderer, ClassId, "RTAO_Trace", vertexModule, traceFragment, m_traceLayout, m_traceTarget);
		m_blurPipeline = createFullscreenPipeline(renderer, ClassId, "RTAO_Blur", vertexModule, blurFragment, m_blurLayout, m_blurHTarget);
		m_applyPipeline = createFullscreenPipeline(renderer, ClassId, "RTAO_Apply", vertexModule, applyFragment, m_applyLayout, m_outputTarget);

		if ( m_tracePipeline == nullptr || m_blurPipeline == nullptr || m_applyPipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		const auto & pool = renderer.descriptorPool();

		/* Trace: set 1 reads depth + normals (updated per-frame). */
		m_tracePerFrame = createPerFrameDescriptorSets(renderer, traceInputLayout, ClassId, "Trace_DescSet");

		if ( m_tracePerFrame.empty() )
		{
			return false;
		}

		/* Blur H: reads trace result. */
		m_blurHDescSet = std::make_unique< DescriptorSet >(pool, singleLayout);
		m_blurHDescSet->setIdentifier(ClassId, "BlurH_DescSet", "DescriptorSet");

		if ( !m_blurHDescSet->create() )
		{
			return false;
		}

		if ( !m_blurHDescSet->writeCombinedImageSampler(0, m_traceTarget) )
		{
			return false;
		}

		/* Blur V: reads blur H result. */
		m_blurVDescSet = std::make_unique< DescriptorSet >(pool, singleLayout);
		m_blurVDescSet->setIdentifier(ClassId, "BlurV_DescSet", "DescriptorSet");

		if ( !m_blurVDescSet->create() )
		{
			return false;
		}

		if ( !m_blurVDescSet->writeCombinedImageSampler(0, m_blurHTarget) )
		{
			return false;
		}

		/* Apply: reads color (per-frame) + blurred AO (fixed). */
		m_applyPerFrame = createPerFrameDescriptorSets(renderer, applyLayout, ClassId, "Apply_DescSet");

		if ( m_applyPerFrame.empty() )
		{
			return false;
		}

		for ( auto & ds : m_applyPerFrame )
		{
			if ( !ds->writeCombinedImageSampler(1, m_blurVTarget) )
			{
				return false;
			}
		}

		m_renderer = &renderer;

		return true;
	}

	void
	RTAO::destroy () noexcept
	{
		m_applyPerFrame.clear();
		m_tracePerFrame.clear();
		m_blurVDescSet.reset();
		m_blurHDescSet.reset();

		m_renderer = nullptr;

		m_applyPipeline.reset();
		m_blurPipeline.reset();
		m_tracePipeline.reset();
		m_applyLayout.reset();
		m_blurLayout.reset();
		m_traceLayout.reset();

		m_outputTarget.destroy();
		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_traceTarget.destroy();
	}

	const TextureInterface &
	RTAO::execute (
		const CommandBuffer & commandBuffer,
		const TextureInterface & inputColor,
		const TextureInterface * inputDepth,
		const TextureInterface * inputNormals,
		const TextureInterface * inputMaterialProperties,
		const PostProcessor::PushConstants & constants
	) noexcept
	{
		const auto frameIndex = m_renderer->currentFrameIndex();

		/* Update depth + normals descriptors for this frame's trace pass. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_tracePerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		/* Update color descriptor for apply pass. */
		static_cast< void >(m_applyPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Update material properties descriptor for apply pass. */
		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_applyPerFrame[frameIndex]->writeCombinedImageSampler(2, *inputMaterialProperties));
		}

		/* ---- Pass 1: Ray Trace AO ---- */
		{
			/* Use readStateIndex for the SAME view matrix that produced the depth buffer. */
			const auto readStateIndex = m_renderer->currentReadStateIndex();
			const auto & viewMatrices = m_renderer->mainRenderTarget()->viewMatrices();
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
			const auto * rtDescSet = m_renderer->rtDescriptorSet();

			if ( rtDescSet != nullptr )
			{
				commandBuffer.bind(*rtDescSet, *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
			}

			/* Bind set 1: Input textures (depth + normals). */
			commandBuffer.bind(*m_tracePerFrame[frameIndex], *m_traceLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 1);

			commandBuffer.draw(3, 1);

			m_traceTarget.endRenderPass(commandBuffer);
		}

		/* ---- Pass 2: Blur Horizontal ---- */
		{
			const BlurPushConstants blurH{
				.texelSizeX = 1.0F / static_cast< float >(m_blurHTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurHTarget.height()),
				.directionX = 1.0F,
				.directionY = 0.0F
			};

			recordFullscreenPass(
				commandBuffer, m_blurHTarget, *m_blurPipeline, *m_blurLayout,
				*m_blurHDescSet, &blurH, sizeof(BlurPushConstants)
			);
		}

		/* ---- Pass 3: Blur Vertical ---- */
		{
			const BlurPushConstants blurV{
				.texelSizeX = 1.0F / static_cast< float >(m_blurVTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_blurVTarget.height()),
				.directionX = 0.0F,
				.directionY = 1.0F
			};

			recordFullscreenPass(
				commandBuffer, m_blurVTarget, *m_blurPipeline, *m_blurLayout,
				*m_blurVDescSet, &blurV, sizeof(BlurPushConstants)
			);
		}

		/* ---- Pass 4: Apply AO to scene color ---- */
		{
			const ApplyPushConstants apply{
				.intensity = m_parameters.intensity,
				.padding1 = 0.0F,
				.padding2 = 0.0F,
				.padding3 = 0.0F
			};

			recordFullscreenPass(
				commandBuffer, m_outputTarget, *m_applyPipeline, *m_applyLayout,
				*m_applyPerFrame[frameIndex], &apply, sizeof(ApplyPushConstants)
			);
		}

		return m_outputTarget;
	}
}
