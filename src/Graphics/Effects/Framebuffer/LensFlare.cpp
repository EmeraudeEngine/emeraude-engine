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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "LensFlare.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Scenes/LightSet.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ShaderModule.hpp"

/* Defining the resource owner of this translation unit. */
/* NOLINTBEGIN(cert-err58-cpp) : We need static strings. */
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
};

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

	outColor = vec4(result * intensity, 1.0);
}
)GLSL";

	static constexpr auto CompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;
layout(set = 0, binding = 1) uniform sampler2D flareTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float lightOnScreen;
	float padding;
};

void main()
{
	vec3 scene = texture(sceneTex, vUV).rgb;
	vec3 flare = texture(flareTex, vUV).rgb;

	/* Modulate flare by lightOnScreen: fades out when light is behind the camera. */
	outColor = vec4(scene + flare * lightOnScreen, 1.0);
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

		/* Create output target (full-res). */
		if ( !m_outputTarget.create(renderer, width, height, format, "LF_Output") )
		{
			TraceError{TracerTag} << "Failed to create output target !";

			return false;
		}

		/* ---- Descriptor set layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		/* Single input layout (1 combined image sampler). */
		auto singleInputLayout = this->getInputLayout(1);

		if ( singleInputLayout == nullptr )
		{
			return false;
		}

		/* Dual input layout (2 combined image samplers). */
		auto dualInputLayout = this->getInputLayout(2);

		if ( dualInputLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(singleInputLayout);

			m_thresholdLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ThresholdPushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(singleInputLayout);

			m_ghostHaloLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GhostHaloPushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(dualInputLayout);

			m_compositeLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePushConstants)}
			});
		}

		if ( m_thresholdLayout == nullptr || m_ghostHaloLayout == nullptr || m_compositeLayout == nullptr )
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

		auto compositeFragment = shaderManager.getShaderModuleFromSourceCode(device, "LF_Composite_FS", ShaderType::FragmentShader, CompositeFragmentShader);

		if ( compositeFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile composite shader !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_thresholdPipeline = this->createFullscreenPipeline(ClassId, "LF_Threshold", vertexModule, thresholdFragment, m_thresholdLayout, m_thresholdTarget);
		m_ghostHaloPipeline = this->createFullscreenPipeline(ClassId, "LF_GhostHalo", vertexModule, ghostHaloFragment, m_ghostHaloLayout, m_ghostHaloTarget);
		m_compositePipeline = this->createFullscreenPipeline(ClassId, "LF_Composite", vertexModule, compositeFragment, m_compositeLayout, m_outputTarget);

		if ( m_thresholdPipeline == nullptr || m_ghostHaloPipeline == nullptr || m_compositePipeline == nullptr )
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

		/* Ghost+Halo: reads threshold target (fixed, single set). */
		const auto & pool = renderer.descriptorPool();

		m_ghostHaloDescSet = std::make_unique< DescriptorSet >(pool, singleInputLayout);
		m_ghostHaloDescSet->setIdentifier(ClassId, "GhostHalo_DescSet", "DescriptorSet");

		if ( !m_ghostHaloDescSet->create() )
		{
			return false;
		}

		if ( !m_ghostHaloDescSet->writeCombinedImageSampler(0, m_thresholdTarget) )
		{
			return false;
		}

		/* Composite: reads scene color (updated per-frame, binding 0) + ghost+halo result (fixed, binding 1). */
		m_compositePerFrame = this->createPerFrameDescriptorSets(dualInputLayout, ClassId, "LF_Composite_DescSet");

		if ( m_compositePerFrame.empty() )
		{
			return false;
		}

		/* Write binding 1 (ghost+halo result) for each composite frame descriptor. */
		for ( const auto & descriptorSet : m_compositePerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(1, m_ghostHaloTarget) )
			{
				return false;
			}
		}

		return true;
	}

	void
	LensFlare::destroy () noexcept
	{
		m_compositePerFrame.clear();
		m_ghostHaloDescSet.reset();
		m_thresholdPerFrame.clear();

		m_compositePipeline.reset();
		m_ghostHaloPipeline.reset();
		m_thresholdPipeline.reset();
		m_compositeLayout.reset();
		m_ghostHaloLayout.reset();
		m_thresholdLayout.reset();

		m_outputTarget.destroy();
		m_ghostHaloTarget.destroy();
		m_thresholdTarget.destroy();
	}

	const TextureInterface &
	LensFlare::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, [[maybe_unused]] const TextureInterface * inputDepth, [[maybe_unused]] const TextureInterface * inputNormals, [[maybe_unused]] const TextureInterface * inputMaterialProperties, const Scenes::LightSet * lightSet, [[maybe_unused]] const PostProcessor::PushConstants & constants) noexcept
	{
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

		/* 2. Update per-frame threshold descriptor with scene color. */
		static_cast< void >(m_thresholdPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* 3. Update per-frame composite descriptor with scene color. */
		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* 4. Pass 1: Threshold extraction (half-res). */
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

		/* 5. Pass 2: Ghost generation + Halo (half-res). */
		const GhostHaloPushConstants ghostHaloPC{
			.lightScreenX = screenX,
			.lightScreenY = screenY,
			.ghostSpacing = m_parameters.ghostSpacing,
			.haloRadius = m_parameters.haloRadius,
			.haloThickness = m_parameters.haloThickness,
			.chromaticDistortion = m_parameters.chromaticDistortion,
			.intensity = m_parameters.intensity,
			.ghostCount = m_parameters.ghostCount
		};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_ghostHaloTarget,
			*m_ghostHaloPipeline,
			*m_ghostHaloLayout,
			*m_ghostHaloDescSet,
			&ghostHaloPC,
			sizeof(GhostHaloPushConstants)
		);

		/* 6. Pass 3: Composite (additive blend, full-res). */
		const CompositePushConstants compositePC{
			.texelSizeX = 1.0F / static_cast< float >(m_outputTarget.width()),
			.texelSizeY = 1.0F / static_cast< float >(m_outputTarget.height()),
			.lightOnScreen = lightOnScreen,
			.padding = 0.0F
		};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_outputTarget,
			*m_compositePipeline,
			*m_compositeLayout,
			*m_compositePerFrame[frameIndex],
			&compositePC,
			sizeof(CompositePushConstants)
		);

		return m_outputTarget;
	}
}
