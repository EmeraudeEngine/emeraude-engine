/*
 * src/Graphics/Effects/Framebuffer/ContactShadows.cpp
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

#include "ContactShadows.hpp"

/* Local inclusions. */
#include "RTAlphaTestGLSL.hpp"

/* STL inclusions. */
#include <cstring>

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

static constexpr auto TracerTag{"ContactShadowsEffect"};
/* NOLINTEND(cert-err58-cpp) */

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ---- */

	constexpr auto RTShadowFragmentShader = R"GLSL(
#version 460

#extension GL_EXT_ray_query : require
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

/* RT data (set 0, the Renderer's set): the TLAS, and the mesh/material SSBOs the alpha-test
 * rule reads. */
layout(set = 0, binding = 0) uniform accelerationStructureEXT topLevelAS;
)GLSL" EMEN_RT_SCENE_DATA_GLSL(2) EMEN_RT_ALPHA_TEST_GLSL_FUNCTIONS R"GLSL(

/* Input textures (set 1). */
layout(set = 1, binding = 0) uniform sampler2D depthTex;
layout(set = 1, binding = 1) uniform sampler2D normalTex;

/* Per-frame parameters (set 1, binding 2). A UBO and not push constants: with the inverse
 * view rotation the block is 132 bytes, above the 128-byte push constant minimum guarantee. */
layout(set = 1, binding = 2, std140) uniform ShadowParams
{
	mat4 inverseProjViewMatrix;
	vec4 invViewCol0;	/* xyz = inverse view rotation column 0, w = camera position X. */
	vec4 invViewCol1;	/* xyz = inverse view rotation column 1, w = camera position Y. */
	vec4 invViewCol2;	/* xyz = inverse view rotation column 2, w = camera position Z. */
	vec4 lightParameters;	/* xyz = light EMISSION direction (world), w = maxDistance. */
	vec4 shadowParameters;	/* x = normalBias, yzw = unused. */
};

vec3 reconstructWorldPosition(vec2 uv, float depth)
{
	vec2 ndc = uv * 2.0 - 1.0;
	vec4 clipPos = vec4(ndc, depth, 1.0);
	vec4 worldPos = inverseProjViewMatrix * clipPos;

	return worldPos.xyz / worldPos.w;
}

void main()
{
	const float maxDistance = lightParameters.w;
	const float normalBias = shadowParameters.x;

	/* ⚠️ texelFetch, NOT texture(): the pass runs at HALF resolution, so vUV lands exactly on
	 * the corner of a 2x2 full-res block and a filtered read returns the average of four
	 * depths every single pixel. Depth is non-linear, so that average is the depth of no real
	 * surface and the reconstructed ray origin floats off the geometry — worst where the depth
	 * gradient is steepest, which is precisely a curved silhouette. Same rule as RTAO. */
	const ivec2 depthSize = textureSize(depthTex, 0);
	const ivec2 fullResCoord = ivec2(vUV * vec2(depthSize));
	float rawDepth = texelFetch(depthTex, fullResCoord, 0).r;

	/* Skip sky pixels (depth at far plane). */
	if (rawDepth >= 0.9999)
	{
		outColor = vec4(1.0);
		return;
	}

	/* View-space normal from the G-buffer. The effect declares requiresNormals() and the
	 * renderer binds this attachment; the shader USED TO DECLARE IT AND NEVER READ IT, which
	 * is what left the ray origin without a normal offset (see below). */
	vec4 normalData = texelFetch(normalTex, fullResCoord, 0);
	vec3 rawN = normalData.rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outColor = vec4(1.0);
		return;
	}

	/* Reconstruct the world position of the texel that was actually FETCHED, not of vUV: at
	 * half resolution the two differ by half a full-res texel, and the ray origin must sit on
	 * the surface the depth belongs to. */
	vec2 depthTexelUV = (vec2(fullResCoord) + 0.5) / vec2(depthSize);
	vec3 worldPos = reconstructWorldPosition(depthTexelUV, rawDepth);

	/* View-space normal to world space. */
	mat3 invViewRotation = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);
	vec3 worldNormal = normalize(invViewRotation * normalize(rawN));

	vec3 viewPosition = vec3(invViewCol0.w, invViewCol1.w, invViewCol2.w);
	vec3 viewDir = normalize(worldPos - viewPosition);
	float cameraDist = length(worldPos - viewPosition);

	/* Adaptive bias: scale with camera distance (the pixel footprint grows) AND with the
	 * grazing angle (a ray leaving a nearly edge-on surface clips its own facets). Same rule
	 * as RTAO. */
	float NdotV = max(abs(dot(worldNormal, -viewDir)), 0.001);
	float grazingFactor = 1.0 / NdotV;
	float adaptiveBias = normalBias * max(1.0, cameraDist) * min(grazingFactor, 10.0);

	/* Distance fade: contact shadows are a near-field effect.
	 * Fade to 1.0 (no shadow) beyond maxDistance * 10 from camera. */
	float shadowFadeRange = maxDistance * 10.0;
	float shadowFade = clamp(cameraDist / shadowFadeRange, 0.0, 1.0);

	/* Light direction in world space (negate emission direction to get toward-light). */
	vec3 lightDir = normalize(-lightParameters.xyz);

	/* ⚠️⚠️ Offset the ray ORIGIN along the surface normal, and keep tMin a tiny CONSTANT.
	 * The bias used to be handed to rayQueryInitializeEXT as tMin, i.e. as a distance along
	 * the LIGHT direction: at the shadow terminator the light is grazing, so advancing along
	 * it never leaves the surface and the ray re-hit the neighbouring triangles. The terminator
	 * then came out FACETED — measured on the DamagedHelmet dome (asset-loader, options
	 * 7,0,1,0,0,0): 19 axis-aligned steps of >= 4px, 72.3% of the boundary perfectly flat.
	 * Raising tMin hides it but skips genuine near occluders (peter-panning), which is exactly
	 * what a contact shadow exists to draw. Same rule as RTAO. */
	vec3 rayOrigin = worldPos + worldNormal * adaptiveBias;

	/* Initialize and execute the ray query. */
	rayQueryEXT rayQuery;
	/* ⚠️ NOT gl_RayFlagsOpaqueEXT: a cutout instance (foliage) hands its triangles over as
	 * candidates and the opaque flag accepted them whole — a leaf cast a contact shadow as a
	 * solid quad. The shared alpha-test rule judges each candidate (RTAlphaTestGLSL.hpp);
	 * TerminateOnFirstHit still ends the traversal at the first CONFIRMED one. */
	rayQueryInitializeEXT(
		rayQuery,
		topLevelAS,
		gl_RayFlagsTerminateOnFirstHitEXT,
		0xFF,
		rayOrigin,
		0.001,
		lightDir,
		maxDistance
	);

	while (rayQueryProceedEXT(rayQuery))
	{
)GLSL" EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE(rayQuery) R"GLSL(
	}

	float shadow = 1.0;
	float normalizedHitDist = 1.0;

	if (rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT)
	{
		float hitT = rayQueryGetIntersectionTEXT(rayQuery, true);
		shadow = smoothstep(0.0, maxDistance, hitT);
		normalizedHitDist = clamp(hitT / maxDistance, 0.0, 1.0);
	}

	/* Apply distance fade: no contact shadows at distance. */
	shadow = mix(shadow, 1.0, shadowFade);

	/* R = shadow factor, G = normalized hit distance (for PCSS-lite blur). */
	outColor = vec4(shadow, normalizedHitDist, 0.0, 1.0);
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
	ContactShadows::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		auto & settings = renderer.primaryServices().settings();

		/* Pixel doubling: half-res for performance (default), full-res for quality.
		 * SAME key and rule as RTAO — the shared denoise pass blurs the whole group in
		 * one multi-target pass, so every member's blur targets must share one extent. */
		const auto pixelDoubling = settings.getOrSetDefault< bool >(GraphicsRayTracingAOPixelDoublingKey, DefaultGraphicsRayTracingAOPixelDoubling);
		const auto halfW = pixelDoubling ? ((width > 1) ? width / 2 : 1U) : width;
		const auto halfH = pixelDoubling ? ((height > 1) ? height / 2 : 1U) : height;

		/* Create shadow mask target (half-res, RT gives clean results — the combine
		 * upsamples bilinearly back to full resolution). */
		if ( !m_shadowTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "CS_RTShadow") )
		{
			TraceError{TracerTag} << "Failed to create shadow target !";

			return false;
		}

		/* Create blur intermediate targets (half-res). */
		if ( !m_blurHTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "CS_BlurH") )
		{
			TraceError{TracerTag} << "Failed to create horizontal blur target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, halfW, halfH, VK_FORMAT_R16G16B16A16_SFLOAT, "CS_BlurV") )
		{
			TraceError{TracerTag} << "Failed to create vertical blur target !";

			return false;
		}

		/* ---- Descriptor set layouts ----
		 * Set 0 = the Renderer's RT set (TLAS + mesh/material SSBOs), set 1 = depth + normals,
		 * set 2 = the bindless textures. The shadow ray judges its alpha-tested candidates with the
		 * shared rule (RTAlphaTestGLSL.hpp), which reads the material SSBO and samples cutout
		 * textures — the effect used to carry a private set with its own TLAS binding and could
		 * reach neither. */
		auto rtLayout = renderer.rtDescriptorSetLayout();

		if ( rtLayout == nullptr )
		{
			TraceError{TracerTag} << "RT descriptor set layout not available !";

			return false;
		}

		/* Set 1: depth (0), normals (1), per-frame parameters (2). */
		m_shadowInputLayout = this->getInputLayout(2, 1);

		if ( m_shadowInputLayout == nullptr )
		{
			return false;
		}

		auto bindlessLayout = renderer.bindlessTextureManager().descriptorSetLayout();

		if ( bindlessLayout == nullptr )
		{
			TraceError{TracerTag} << "Bindless texture descriptor set layout not available !";

			return false;
		}

		/* ---- Pipeline layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		{
			/* No push constant range: the per-frame data lives in a UBO (set 1, binding 2)
			 * because it exceeds the 128-byte minimum guarantee. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(rtLayout);
			sets.emplace_back(m_shadowInputLayout);
			sets.emplace_back(bindlessLayout);
			m_shadowLayout = layoutManager.getPipelineLayout(sets, {});
		}

		if ( m_shadowLayout == nullptr )
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

		const auto shadowFragment = shaderManager.getShaderModuleFromSourceCode(device, "CS_RTShadow_FS", ShaderType::FragmentShader, RTShadowFragmentShader);

		if ( shadowFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile RT shadow shader !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_shadowPipeline = this->createFullscreenPipeline(ClassId, "CS_RTShadow", vertexModule, shadowFragment, m_shadowLayout, m_shadowTarget);

		if ( m_shadowPipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */

		/* RT shadow pass, set 1: depth (binding 0) + normals (binding 1). The TLAS comes with
		 * the Renderer's set 0, bound at record time. */
		m_shadowPerFrame = this->createPerFrameDescriptorSets(m_shadowInputLayout, ClassId, "CS_RTShadow_DescSet");

		if ( m_shadowPerFrame.empty() )
		{
			return false;
		}

		/* Per-frame parameter buffers (set 1, binding 2). The image bindings are rewritten
		 * every frame, the buffer binding only once: the buffer handle never changes. */
		m_shadowFrameUBOs = this->createPerFrameUniformBuffers(sizeof(ShadowFrameUBOData), ClassId, "CS_RTShadow_Frame_UBO");

		if ( m_shadowFrameUBOs.size() != m_shadowPerFrame.size() )
		{
			TraceError{TracerTag} << "Failed to create the per-frame shadow parameter buffers !";

			return false;
		}

		for ( size_t frameIndex = 0; frameIndex < m_shadowPerFrame.size(); ++frameIndex )
		{
			if ( !m_shadowPerFrame[frameIndex]->writeUniformBufferObject(2, *m_shadowFrameUBOs[frameIndex]) )
			{
				TraceError{TracerTag} << "Failed to bind the shadow parameter buffer for frame " << frameIndex << " !";

				return false;
			}
		}

		return true;
	}

	void
	ContactShadows::destroy () noexcept
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
	ContactShadows::recordPreDenoisePasses (const CommandBuffer & commandBuffer, const TextureInterface & /*inputColor*/, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputNormals = context.normals;
		const auto * lightSet = context.lightSet;


		const auto frameIndex = this->renderer().currentFrameIndex();

		/* 1. Compute inverse view-projection matrix for world-space reconstruction.
		 * CRITICAL: Use readStateIndex to match the view matrix that produced the depth buffer.
		 * The default overload reads m_logicState which may have advanced → flickering. */
		const auto readStateIndex = this->renderer().currentReadStateIndex();
		const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
		const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
		const auto & projMat = viewMatrices.projectionMatrix(readStateIndex);
		const auto viewProjMat = projMat * viewMat;
		const auto invViewProjMat = viewProjMat.inverse();

		/* 2. Update the per-frame input set (set 1): depth (binding 0), normals (binding 1). */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_shadowPerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_shadowPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		/* Inverse view matrix: columns 0-2 carry the rotation that brings the view-space
		 * G-buffer normal back to world space, column 3 the camera world position. */
		const auto invView = viewMat.inverse();
		const auto * inv = invView.data();

		/* 3. Pass 1: RT shadow query (half-res). */
		ShadowFrameUBOData shadowData{};
		std::memcpy(shadowData.inverseProjViewMatrix.data(), invViewProjMat.data(), shadowData.inverseProjViewMatrix.size() * sizeof(float));
		shadowData.invViewCol0 = {inv[0], inv[1], inv[2], inv[12]};
		shadowData.invViewCol1 = {inv[4], inv[5], inv[6], inv[13]};
		shadowData.invViewCol2 = {inv[8], inv[9], inv[10], inv[14]};
		const auto lightDirection = lightSet->mainDirectionalLight()->direction();
		shadowData.lightParameters = {lightDirection.x(), lightDirection.y(), lightDirection.z(), m_parameters.maxDistance};
		shadowData.shadowParameters = {m_parameters.normalBias, 0.0F, 0.0F, 0.0F};

		if ( !updateUniformBufferData(*m_shadowFrameUBOs[frameIndex], &shadowData, sizeof(shadowData)) )
		{
			TraceError{TracerTag} << "Failed to update the shadow parameter buffer !";

			return;
		}

		/* Custom recording: THREE sets — the shared fullscreen recorder binds one (plus an
		 * optional bindless set at index 1), and this pass needs the Renderer's RT set at 0, its
		 * own inputs at 1 and the bindless textures at 2. */
		m_shadowTarget.beginRenderPass(commandBuffer);

		commandBuffer.bind(*m_shadowPipeline);

		const VkViewport viewport{
			.x = 0.0F,
			.y = 0.0F,
			.width = static_cast< float >(m_shadowTarget.width()),
			.height = static_cast< float >(m_shadowTarget.height()),
			.minDepth = 0.0F,
			.maxDepth = 1.0F
		};
		vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

		const VkRect2D scissor{
			.offset = {0, 0},
			.extent = {m_shadowTarget.width(), m_shadowTarget.height()}
		};
		vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

		/* Set 0: the Renderer's RT set (TLAS + scene SSBOs). */
		if ( const auto * rtDescSet = this->renderer().rtDescriptorSet(); rtDescSet != nullptr )
		{
			commandBuffer.bind(*rtDescSet, *m_shadowLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
		}

		/* Set 1: depth + normals. */
		commandBuffer.bind(*m_shadowPerFrame[frameIndex], *m_shadowLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 1);

		/* Set 2: bindless textures (the alpha-test rule samples cutout textures). */
		if ( const auto * bindlessDescSet = this->renderer().bindlessTextureManager().descriptorSet(); bindlessDescSet != nullptr )
		{
			commandBuffer.bind(*bindlessDescSet, *m_shadowLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 2);
		}

		commandBuffer.draw(3, 1);

		m_shadowTarget.endRenderPass(commandBuffer);
	}

	IndirectPostProcessEffect::DenoiseContribution
	ContactShadows::denoiseContribution (const FrameContext & /*context*/) const noexcept
	{
		DenoiseContribution contribution;
		contribution.prefix = "cshdw";
		contribution.source = &m_shadowTarget;
		contribution.targetH = const_cast< IntermediateRenderTarget * >(&m_blurHTarget);
		contribution.targetV = const_cast< IntermediateRenderTarget * >(&m_blurVTarget);
		contribution.dynamics = {m_parameters.maxBlurRadius, 0.0F, 0.0F, 0.0F};

		/* Same PCSS-lite kernel as the retired CS_Blur_FS pass: 9-tap gaussian whose
		 * radius scales with the normalized hit distance (G channel of the source),
		 * early-out pass-through below half a texel, hit distance preserved in G. */
		contribution.code =
			"\tvec2 cshdwTexel = 1.0 / vec2(textureSize(cshdwSrc, 0));\n"
			"\tvec4 cshdwCenter = texture(cshdwSrc, vUV);\n"
			"\tfloat cshdwHitDist = cshdwCenter.g;\n"
			"\tfloat cshdwRadius = emDyn.cshdwDynamics0.x * cshdwHitDist;\n"
			"\tvec4 cshdwResult = cshdwCenter;\n"
			"\tif (cshdwRadius >= 0.5)\n"
			"\t{\n"
			"\t\tconst float cshdwWeights[5] = float[](0.227027, 0.194596, 0.121621, 0.054054, 0.016216);\n"
			"\t\tvec2 cshdwStep = emDenoiseDir * cshdwTexel;\n"
			"\t\tfloat cshdwSum = cshdwCenter.r * cshdwWeights[0];\n"
			"\t\tfor (int cshdwI = 1; cshdwI < 5; cshdwI++)\n"
			"\t\t{\n"
			"\t\t\tvec2 cshdwOffset = cshdwStep * (float(cshdwI) / 4.0 * cshdwRadius);\n"
			"\t\t\tcshdwSum += texture(cshdwSrc, vUV + cshdwOffset).r * cshdwWeights[cshdwI];\n"
			"\t\t\tcshdwSum += texture(cshdwSrc, vUV - cshdwOffset).r * cshdwWeights[cshdwI];\n"
			"\t\t}\n"
			"\t\tcshdwResult = vec4(cshdwSum, cshdwHitDist, 0.0, 1.0);\n"
			"\t}\n";

		return contribution;
	}

	IndirectPostProcessEffect::CombineContribution
	ContactShadows::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		CombineContribution contribution;
		contribution.prefix = "cshdw";
		contribution.samplers.emplace_back(CombineSamplerInput{"Tex", &m_blurVTarget});
		contribution.needsMaterialProperties = true;
		contribution.dynamics.emplace_back(Base::Math::Vector< 4, float >{m_parameters.intensity, 0.0F, 0.0F, 0.0F});

		/* Same math as the retired CS_Apply_FS pass: user intensity, then the material
		 * shadowResponse (LOW nibble of mp.g — the HIGH nibble is the aoResponse). */
		contribution.code =
			"\tfloat cshdwShadow = texture(cshdwTex, vUV).r;\n"
			"\tvec4 cshdwMp = texture(emMaterialProps, vUV);\n"
			"\tfloat cshdwResponse = float(uint(cshdwMp.g * 255.0) & 0xFu) / 15.0;\n"
			"\tcshdwShadow = mix(1.0, cshdwShadow, emDyn.cshdwDynamics0.x);\n"
			"\tcshdwShadow = mix(1.0, cshdwShadow, cshdwResponse);\n"
			"\tem_Color.rgb *= cshdwShadow;\n";

		return contribution;
	}
}
