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

layout(push_constant) uniform PushConstants
{
	mat4 inverseProjViewMatrix;
	float lightDirWorldX;
	float lightDirWorldY;
	float lightDirWorldZ;
	float maxDistance;
	float normalBias;
	float viewPosX;
	float viewPosY;
	float viewPosZ;
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
	float rawDepth = texture(depthTex, vUV).r;

	/* Skip sky pixels (depth at far plane). */
	if (rawDepth >= 0.9999)
	{
		outColor = vec4(1.0);
		return;
	}

	/* Reconstruct world-space position directly from depth + inverse VP. */
	vec3 worldPos = reconstructWorldPosition(vUV, rawDepth);

	/* Adaptive bias: scale with camera distance to prevent self-intersection
	 * at distance where pixel footprint is large. */
	float cameraDist = length(worldPos - vec3(viewPosX, viewPosY, viewPosZ));
	float adaptiveBias = normalBias * max(1.0, cameraDist);

	/* Distance fade: contact shadows are a near-field effect.
	 * Fade to 1.0 (no shadow) beyond maxDistance * 10 from camera. */
	float shadowFadeRange = maxDistance * 10.0;
	float shadowFade = clamp(cameraDist / shadowFadeRange, 0.0, 1.0);

	/* Light direction in world space (negate emission direction to get toward-light). */
	vec3 lightDir = normalize(vec3(-lightDirWorldX, -lightDirWorldY, -lightDirWorldZ));

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
		worldPos,
		adaptiveBias,
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

		m_shadowInputLayout = this->getInputLayout(2);

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
			const StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ShadowPushConstants)}
			};

			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(rtLayout);
			sets.emplace_back(m_shadowInputLayout);
			sets.emplace_back(bindlessLayout);
			m_shadowLayout = layoutManager.getPipelineLayout(sets, ranges);
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

		return true;
	}

	void
	ContactShadows::destroy () noexcept
	{
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

		/* Extract camera position from inverse view matrix. */
		const auto invView = viewMat.inverse();
		const auto * inv = invView.data();

		/* 3. Pass 1: RT shadow query (half-res). */
		ShadowPushConstants shadowPC{};
		std::memcpy(shadowPC.inverseProjViewMatrix, invViewProjMat.data(), sizeof(shadowPC.inverseProjViewMatrix));
		const auto lightDirection = lightSet->mainDirectionalLight()->direction();
		shadowPC.lightDirWorldX = lightDirection.x();
		shadowPC.lightDirWorldY = lightDirection.y();
		shadowPC.lightDirWorldZ = lightDirection.z();
		shadowPC.maxDistance = m_parameters.maxDistance;
		shadowPC.normalBias = m_parameters.normalBias;
		shadowPC.viewPosX = inv[12];
		shadowPC.viewPosY = inv[13];
		shadowPC.viewPosZ = inv[14];

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

		vkCmdPushConstants(
			commandBuffer.handle(),
			m_shadowLayout->handle(),
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(ShadowPushConstants),
			&shadowPC
		);

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
