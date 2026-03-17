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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "ContactShadows.hpp"

/* STL inclusions. */
#include <cstring>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Scenes/LightSet.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Vulkan/AccelerationStructure.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Tracer.hpp"

/* Defining the resource owner of this translation unit. */
/* NOLINTBEGIN(cert-err58-cpp) : We need static strings. */
static constexpr auto TracerTag{"ContactShadowsEffect"};
/* NOLINTEND(cert-err58-cpp) */

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ---- */

	constexpr auto RTShadowFragmentShader = R"GLSL(
#version 460

#extension GL_EXT_ray_query : require

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D normalTex;
layout(set = 0, binding = 2) uniform accelerationStructureEXT topLevelAS;

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
	rayQueryInitializeEXT(
		rayQuery,
		topLevelAS,
		gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT,
		0xFF,
		worldPos,
		adaptiveBias,
		lightDir,
		maxDistance
	);

	while (rayQueryProceedEXT(rayQuery))
	{
		/* No custom intersection handling needed for opaque geometry. */
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

	constexpr auto ApplyFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D shadowTex;
layout(set = 0, binding = 2) uniform sampler2D materialPropsTex;

layout(push_constant) uniform PushConstants
{
	float intensity;
};

void main()
{
	vec4 color = texture(colorTex, vUV);
	float shadow = texture(shadowTex, vUV).r;

	/* Decode material properties from G-buffer. */
	vec4 mp = texture(materialPropsTex, vUV);
	uint gPacked = uint(mp.g * 255.0);
	float shadowResponse = float(gPacked & 0xFu) / 15.0;

	/* Mix toward full shadow based on intensity. */
	shadow = mix(1.0, shadow, intensity);

	/* Modulate shadow by material shadowResponse. */
	shadow = mix(1.0, shadow, shadowResponse);

	outColor = vec4(color.rgb * shadow, color.a);
}
)GLSL";

	constexpr auto BlurFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTex;

layout(push_constant) uniform PushConstants
{
	float directionX;
	float directionY;
	float texelSizeX;
	float texelSizeY;
	float maxBlurRadius;
};

void main()
{
	vec4 center = texture(inputTex, vUV);
	float shadow = center.r;
	float hitDist = center.g;

	/* Blur radius proportional to normalized hit distance (PCSS-lite).
	 * Close occluders produce tight contact shadows; far occluders produce soft penumbra. */
	float radius = maxBlurRadius * hitDist;

	/* Skip blur for fully lit pixels or negligible radius. */
	if (radius < 0.5)
	{
		outColor = center;
		return;
	}

	/* 9-tap Gaussian kernel (center + 4 bilateral pairs).
	 * Weights are pre-normalized (sum ≈ 1.0). */
	const float weights[5] = float[](0.227027, 0.194596, 0.121621, 0.054054, 0.016216);

	vec2 step = vec2(directionX * texelSizeX, directionY * texelSizeY);

	float result = shadow * weights[0];

	for (int i = 1; i < 5; i++)
	{
		vec2 offset = step * (float(i) / 4.0 * radius);
		result += texture(inputTex, vUV + offset).r * weights[i];
		result += texture(inputTex, vUV - offset).r * weights[i];
	}

	/* Preserve hit distance in G channel for the vertical blur pass. */
	outColor = vec4(result, hitDist, 0.0, 1.0);
}
)GLSL";

}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Libs;
	using namespace Vulkan;
	using namespace Saphir;

	/* ---- Lifecycle ---- */

	bool
	ContactShadows::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		/* Create shadow mask target (full-res, RT gives clean results). */
		if ( !m_shadowTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "CS_RTShadow") )
		{
			TraceError{TracerTag} << "Failed to create shadow target !";

			return false;
		}

		/* Create blur intermediate targets (full-res). */
		if ( !m_blurHTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "CS_BlurH") )
		{
			TraceError{TracerTag} << "Failed to create horizontal blur target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "CS_BlurV") )
		{
			TraceError{TracerTag} << "Failed to create vertical blur target !";

			return false;
		}

		/* Create output target (full-res). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "CS_Output") )
		{
			TraceError{TracerTag} << "Failed to create output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto singleInputLayout = this->getInputLayout(1);

		if ( singleInputLayout == nullptr )
		{
			return false;
		}

		auto tripleInputLayout = this->getInputLayout(3);

		if ( tripleInputLayout == nullptr )
		{
			return false;
		}

		/* RT shadow pass layout: depth (binding 0) + normals (binding 1) + TLAS (binding 2). */
		{
			const auto & device = renderer.device();

			m_shadowDescLayout = std::make_shared< DescriptorSetLayout >(device, "CS_RTShadow_DSL");

			if ( !m_shadowDescLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT) ||
				!m_shadowDescLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT) ||
				!m_shadowDescLayout->declareAccelerationStructureKHR(2, VK_SHADER_STAGE_FRAGMENT_BIT) )
			{
				TraceError{TracerTag} << "Failed to declare RT shadow descriptor set layout !";

				return false;
			}

			if ( !m_shadowDescLayout->createOnHardware() )
			{
				TraceError{TracerTag} << "Failed to create RT shadow descriptor set layout !";

				return false;
			}
		}

		/* ---- Pipeline layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		{
			const StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ShadowPushConstants)}
			};

			StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(m_shadowDescLayout);
			m_shadowLayout = layoutManager.getPipelineLayout(sets, ranges);
		}

		{
			const StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ApplyPushConstants)}
			};

			StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(tripleInputLayout);
			m_applyLayout = layoutManager.getPipelineLayout(sets, ranges);
		}

		{
			const StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BlurPushConstants)}
			};

			StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(singleInputLayout);
			m_blurLayout = layoutManager.getPipelineLayout(sets, ranges);
		}

		if ( m_shadowLayout == nullptr || m_applyLayout == nullptr || m_blurLayout == nullptr )
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

		const auto applyFragment = shaderManager.getShaderModuleFromSourceCode(device, "CS_Apply_FS", ShaderType::FragmentShader, ApplyFragmentShader);

		if ( applyFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile apply shader !";

			return false;
		}

		const auto blurFragment = shaderManager.getShaderModuleFromSourceCode(device, "CS_Blur_FS", ShaderType::FragmentShader, BlurFragmentShader);

		if ( blurFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile blur shader !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_shadowPipeline = this->createFullscreenPipeline(ClassId, "CS_RTShadow", vertexModule, shadowFragment, m_shadowLayout, m_shadowTarget);
		m_blurHPipeline = this->createFullscreenPipeline(ClassId, "CS_BlurH", vertexModule, blurFragment, m_blurLayout, m_blurHTarget);
		m_blurVPipeline = this->createFullscreenPipeline(ClassId, "CS_BlurV", vertexModule, blurFragment, m_blurLayout, m_blurVTarget);
		m_applyPipeline = this->createFullscreenPipeline(ClassId, "CS_Apply", vertexModule, applyFragment, m_applyLayout, m_outputTarget);

		if ( m_shadowPipeline == nullptr || m_blurHPipeline == nullptr || m_blurVPipeline == nullptr || m_applyPipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */

		/* RT shadow pass: reads depth (binding 0) + normals (binding 1) + TLAS (binding 2). */
		m_shadowPerFrame = this->createPerFrameDescriptorSets(m_shadowDescLayout, ClassId, "CS_RTShadow_DescSet");

		if ( m_shadowPerFrame.empty() )
		{
			return false;
		}

		/* Blur H: reads shadow mask (binding 0, fixed). */
		m_blurHPerFrame = this->createPerFrameDescriptorSets(singleInputLayout, ClassId, "CS_BlurH_DescSet");

		if ( m_blurHPerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_blurHPerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(0, m_shadowTarget) )
			{
				return false;
			}
		}

		/* Blur V: reads blur H output (binding 0, fixed). */
		m_blurVPerFrame = this->createPerFrameDescriptorSets(singleInputLayout, ClassId, "CS_BlurV_DescSet");

		if ( m_blurVPerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_blurVPerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(0, m_blurHTarget) )
			{
				return false;
			}
		}

		/* Apply: reads scene color (binding 0, per-frame) + blurred shadow (binding 1, fixed) + material properties (per-frame). */
		m_applyPerFrame = this->createPerFrameDescriptorSets(tripleInputLayout, ClassId, "CS_Apply_DescSet");

		if ( m_applyPerFrame.empty() )
		{
			return false;
		}

		/* Write binding 1 (blurred shadow mask) for each apply frame descriptor. */
		for ( const auto & descriptorSet : m_applyPerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(1, m_blurVTarget) )
			{
				return false;
			}
		}

		return true;
	}

	void
	ContactShadows::destroy () noexcept
	{
		m_applyPerFrame.clear();
		m_blurVPerFrame.clear();
		m_blurHPerFrame.clear();
		m_shadowPerFrame.clear();

		m_applyPipeline.reset();
		m_blurVPipeline.reset();
		m_blurHPipeline.reset();
		m_shadowPipeline.reset();
		m_applyLayout.reset();
		m_blurLayout.reset();
		m_shadowLayout.reset();
		m_shadowDescLayout.reset();

		m_outputTarget.destroy();
		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_shadowTarget.destroy();
	}

	const TextureInterface &
	ContactShadows::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const TextureInterface * inputDepth, const TextureInterface * inputNormals, const TextureInterface * inputMaterialProperties, const Scenes::LightSet * lightSet, const PostProcessor::PushConstants & constants) noexcept
	{
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

		/* 2. Update per-frame RT shadow descriptor with depth (binding 0), normals (binding 1), and TLAS (binding 2). */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_shadowPerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_shadowPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		/* Bind the current TLAS for ray queries. */
		const auto * TLAS = this->renderer().currentTLAS();

		if ( TLAS != nullptr && TLAS->isCreated() )
		{
			static_cast< void >(m_shadowPerFrame[frameIndex]->writeAccelerationStructure(2, TLAS->handle()));
		}

		/* 3. Update per-frame apply descriptor with scene color. */
		static_cast< void >(m_applyPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Update material properties descriptor for apply pass. */
		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_applyPerFrame[frameIndex]->writeCombinedImageSampler(2, *inputMaterialProperties));
		}

		/* Extract camera position from inverse view matrix. */
		const auto invView = viewMat.inverse();
		const auto * inv = invView.data();

		/* 4. Pass 1: RT shadow query (full-res). */
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

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_shadowTarget,
			*m_shadowPipeline,
			*m_shadowLayout,
			*m_shadowPerFrame[frameIndex],
			&shadowPC,
			sizeof(ShadowPushConstants)
		);

		/* 5. Pass 2: Horizontal Gaussian blur (PCSS-lite, radius from hit distance). */
		const float texelX = 1.0F / static_cast< float >(m_blurHTarget.width());
		const float texelY = 1.0F / static_cast< float >(m_blurHTarget.height());

		const BlurPushConstants blurHPC{
			.directionX = 1.0F,
			.directionY = 0.0F,
			.texelSizeX = texelX,
			.texelSizeY = texelY,
			.maxBlurRadius = m_parameters.maxBlurRadius
		};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_blurHTarget,
			*m_blurHPipeline,
			*m_blurLayout,
			*m_blurHPerFrame[frameIndex],
			&blurHPC,
			sizeof(BlurPushConstants)
		);

		/* 6. Pass 3: Vertical Gaussian blur (PCSS-lite). */
		const BlurPushConstants blurVPC{
			.directionX = 0.0F,
			.directionY = 1.0F,
			.texelSizeX = texelX,
			.texelSizeY = texelY,
			.maxBlurRadius = m_parameters.maxBlurRadius
		};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_blurVTarget,
			*m_blurVPipeline,
			*m_blurLayout,
			*m_blurVPerFrame[frameIndex],
			&blurVPC,
			sizeof(BlurPushConstants)
		);

		/* 7. Pass 4: Apply blurred shadow mask to scene color. */
		const ApplyPushConstants applyPC{
			.intensity = m_parameters.intensity
		};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_outputTarget,
			*m_applyPipeline,
			*m_applyLayout,
			*m_applyPerFrame[frameIndex],
			&applyPC,
			sizeof(ApplyPushConstants)
		);

		return m_outputTarget;
	}
}
