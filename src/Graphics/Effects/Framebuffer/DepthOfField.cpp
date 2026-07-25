/*
 * src/Graphics/Effects/Framebuffer/DepthOfField.cpp
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

#include "DepthOfField.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Scenes/Component/Camera.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
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
	static constexpr auto TracerTag{"DepthOfFieldEffect"};

	/* Auto-focus pass (1x1): measures the scene depth around the screen center and
	 * relaxes the focus distance toward it (exponential rack focus). The 1x1 ping-pong
	 * history stores R = focus distance, G = the write timestamp (for the frame delta).
	 * Manual focus goes through the same relaxation: focus pulls are always smooth. */
	static constexpr auto DoFFocusFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outFocus;

layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D previousFocusTex;

layout(push_constant) uniform PushConstants
{
	float nearPlane;
	float farPlane;
	float focusDistance;
	float autoFocusSpeed;
	float time;
	float texelSizeX;
	float texelSizeY;
	uint flags; /* Bit 0 = auto-focus, bit 1 = reset history. */
};

float linearizeDepth (float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
	vec4 previous = texelFetch(previousFocusTex, ivec2(0), 0);

	float target = focusDistance;

	if ((flags & 1u) != 0u)
	{
		/* 5x5 Gaussian-weighted sampling around the screen center. */
		float totalWeight = 0.0;
		float totalDepth = 0.0;
		vec2 center = vec2(0.5);
		vec2 texel = vec2(texelSizeX, texelSizeY);

		for (int y = -2; y <= 2; ++y)
		{
			for (int x = -2; x <= 2; ++x)
			{
				vec2 uv = center + vec2(float(x), float(y)) * texel * 16.0;
				float d = texture(depthTex, uv).r;

				if (d >= 1.0)
					continue;

				float w = exp(-float(x * x + y * y) / 4.5);
				totalDepth += linearizeDepth(d) * w;
				totalWeight += w;
			}
		}

		/* Nothing measurable at the center (sky): hold the previous focus. */
		target = (totalWeight > 0.0) ? totalDepth / totalWeight : previous.r;
	}

	float focus = target;

	if ((flags & 2u) == 0u)
	{
		/* Exponential rack focus: never snaps, like a real focus ring. */
		float dt = clamp(time - previous.g, 0.0, 1.0);
		float a = 1.0 - exp(-autoFocusSpeed * dt);
		focus = mix(previous.r, target, a);
	}

	outFocus = vec4(focus, time, 0.0, 0.0);
}
)GLSL";

	/* Setup pass (half-res): downsampled color + SIGNED normalized circle of confusion.
	 * Thin lens model; positive CoC = far field (behind the focus plane), negative =
	 * near field (in front of it). The sky (far plane) naturally lands in the far field. */
	static constexpr auto DoFSetupFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outSetup;

layout(set = 0, binding = 0) uniform sampler2D colorTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler2D focusTex;

layout(push_constant) uniform PushConstants
{
	float nearPlane;
	float farPlane;
	float aperture;
	float focalLength;
	float cocScale;
	float padding1;
	float padding2;
	float padding3;
};

float linearizeDepth (float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
	vec3 color = texture(colorTex, vUV).rgb;
	float linearZ = linearizeDepth(texture(depthTex, vUV).r);
	float fd = texelFetch(focusTex, ivec2(0), 0).r;

	/* Thin lens circle of confusion, SIGNED (positive behind the focus plane).
	 * CoC = aperture * f * (z - fd) / (z * (fd - f)) */
	float focalLengthM = focalLength * 0.001; /* mm to meters. */
	float coc = aperture * focalLengthM * (linearZ - fd) / (linearZ * max(fd - focalLengthM, 0.001));

	outSetup = vec4(color, clamp(coc * cocScale, -1.0, 1.0));
}
)GLSL";

	/* Near-CoC dilation (separable max filter): spreads the near-field coverage BEYOND
	 * the silhouettes so the foreground blur bleeds over the sharp background — the
	 * defining trait of a real out-of-focus foreground. */
	static constexpr auto DoFDilateFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outDilated;

layout(set = 0, binding = 0) uniform sampler2D inputTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float directionX;
	float directionY;
	int radius;
	uint extractFromAlpha;
};

void main()
{
	vec2 texel = vec2(texelSizeX, texelSizeY);
	vec2 dir = vec2(directionX, directionY);

	float result = 0.0;

	for (int i = -radius; i <= radius; ++i)
	{
		vec2 uv = vUV + dir * texel * float(i);
		float value = (extractFromAlpha != 0u) ? max(-texture(inputTex, uv).a, 0.0) : texture(inputTex, uv).r;
		result = max(result, value);
	}

	outDilated = vec4(result, 0.0, 0.0, 1.0);
}
)GLSL";

	/* Far-field gather (half-res): golden-angle spiral disc — circular bokeh.
	 * Scatter-as-gather: a sample contributes when its OWN circle of confusion is wide
	 * enough to reach the pixel being shaded. Near-field samples are excluded (a sharp
	 * or foreground object must never smear into the background blur behind it). */
	static constexpr auto DoFFarGatherFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outFar;

layout(set = 0, binding = 0) uniform sampler2D setupTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float maxCoCRadius;
	uint sampleCount;
};

const float GOLDEN_ANGLE = 2.39996323;

void main()
{
	vec2 texel = vec2(texelSizeX, texelSizeY);
	vec4 center = texture(setupTex, vUV);
	float centerCoC = max(center.a, 0.0);
	float radiusPx = centerCoC * maxCoCRadius;

	/* In focus (or near field): keep sharp, zero blend. */
	if (radiusPx < 0.5)
	{
		outFar = vec4(center.rgb, 0.0);
		return;
	}

	vec3 accum = center.rgb;
	float accumWeight = 1.0;

	for (uint i = 1u; i < sampleCount; ++i)
	{
		float r = sqrt((float(i) + 0.5) / float(sampleCount)) * radiusPx;
		float theta = float(i) * GOLDEN_ANGLE;
		vec2 offset = vec2(cos(theta), sin(theta)) * r * texel;

		vec4 s = texture(setupTex, vUV + offset);
		float sampleRadiusPx = max(s.a, 0.0) * maxCoCRadius;

		/* The sample's own CoC must cover the gather distance. */
		float w = clamp(sampleRadiusPx - r + 1.0, 0.0, 1.0);

		accum += s.rgb * w;
		accumWeight += w;
	}

	/* Alpha = composite blend factor, saturating quickly with the CoC. */
	outFar = vec4(accum / accumWeight, clamp(centerCoC * 4.0, 0.0, 1.0));
}
)GLSL";

	/* Near-field gather (half-res): same spiral, but driven by the DILATED near CoC so
	 * the foreground blur extends past its own silhouette. No occlusion rejection: the
	 * out-of-focus foreground freely covers whatever is behind it. */
	static constexpr auto DoFNearGatherFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outNear;

layout(set = 0, binding = 0) uniform sampler2D setupTex;
layout(set = 0, binding = 1) uniform sampler2D dilatedNearTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float maxCoCRadius;
	uint sampleCount;
};

const float GOLDEN_ANGLE = 2.39996323;

void main()
{
	vec2 texel = vec2(texelSizeX, texelSizeY);
	float dilatedNear = texture(dilatedNearTex, vUV).r;
	float radiusPx = dilatedNear * maxCoCRadius;

	vec4 center = texture(setupTex, vUV);

	if (radiusPx < 0.5)
	{
		outNear = vec4(center.rgb, 0.0);
		return;
	}

	vec3 accum = vec3(0.0);
	float accumWeight = 0.0;

	for (uint i = 0u; i < sampleCount; ++i)
	{
		float r = sqrt((float(i) + 0.5) / float(sampleCount)) * radiusPx;
		float theta = float(i) * GOLDEN_ANGLE;
		vec2 offset = vec2(cos(theta), sin(theta)) * r * texel;

		vec4 s = texture(setupTex, vUV + offset);
		float sampleNearPx = max(-s.a, 0.0) * maxCoCRadius;

		float w = clamp(sampleNearPx - r + 1.0, 0.0, 1.0);

		accum += s.rgb * w;
		accumWeight += w;
	}

	if (accumWeight <= 0.0)
	{
		outNear = vec4(center.rgb, 0.0);
		return;
	}

	/* Coverage: how much of the disc actually holds near-field content. */
	float coverage = clamp(accumWeight * 3.0 / float(sampleCount), 0.0, 1.0);

	outNear = vec4(accum / accumWeight, coverage);
}
)GLSL";

	/* Composite pass (full-res): sharp base, far field blended by its CoC factor, then
	 * the near field composited OVER everything (foreground bleed). The per-pixel
	 * material DoF mask (A channel low nibble) exempts HUD-like surfaces. */
	static constexpr auto DoFCompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D originalTex;
layout(set = 0, binding = 1) uniform sampler2D farTex;
layout(set = 0, binding = 2) uniform sampler2D nearTex;
layout(set = 0, binding = 3) uniform sampler2D materialPropsTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	uint nearFieldEnabled;
	float padding;
};

void main()
{
	vec4 original = texture(originalTex, vUV);
	vec4 far = texture(farTex, vUV);

	/* Per-pixel material DoF mask: 1.0 = full DoF (default), 0.0 = always sharp. */
	vec4 mp = texture(materialPropsTex, vUV);
	uint aPacked = uint(mp.a * 255.0);
	float dofMask = float(aPacked & 0xFu) / 15.0;

	vec3 result = mix(original.rgb, far.rgb, clamp(far.a, 0.0, 1.0) * dofMask);

	if (nearFieldEnabled != 0u)
	{
		vec4 near = texture(nearTex, vUV);
		result = mix(result, near.rgb, clamp(near.a, 0.0, 1.0) * dofMask);
	}

	outColor = vec4(result, original.a);
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
	DepthOfField::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		auto & settings = renderer.primaryServices().settings();

		/* Effect-quality knobs, engine-wide and persisted in the settings file.
		 * The OPTICAL parameters are NOT settings: they belong to the active camera. */
		m_parameters.cocScale = settings.getOrSetDefault< float >(GraphicsDepthOfFieldCoCScaleKey, DefaultGraphicsDepthOfFieldCoCScale);
		m_parameters.maxCoCRadius = settings.getOrSetDefault< float >(GraphicsDepthOfFieldMaxRadiusKey, DefaultGraphicsDepthOfFieldMaxRadius);
		m_parameters.sampleCount = settings.getOrSetDefault< uint32_t >(GraphicsDepthOfFieldSampleCountKey, DefaultGraphicsDepthOfFieldSampleCount);
		m_parameters.autoFocusSpeed = settings.getOrSetDefault< float >(GraphicsDepthOfFieldAutoFocusSpeedKey, DefaultGraphicsDepthOfFieldAutoFocusSpeed);
		m_parameters.nearFieldEnabled = settings.getOrSetDefault< bool >(GraphicsDepthOfFieldNearFieldKey, DefaultGraphicsDepthOfFieldNearField);

		m_focusValid = false;
		m_focusWriteIndex = 0;

		const auto halfWidth = width > 1 ? width / 2 : 1U;
		const auto halfHeight = height > 1 ? height / 2 : 1U;

		/* Auto-focus history (1x1 RG32F ping-pong: focus distance + timestamp). */
		for ( size_t index = 0; index < 2; ++index )
		{
			if ( !m_focusTargets[index].create(renderer, 1, 1, VK_FORMAT_R32G32_SFLOAT, "DoF_Focus" + std::to_string(index)) )
			{
				TraceError{TracerTag} << "Failed to create DoF focus target #" << index << " !";

				return false;
			}
		}

		/* Setup target: color + signed CoC (half-res). */
		if ( !m_setupTarget.create(renderer, halfWidth, halfHeight, VK_FORMAT_R16G16B16A16_SFLOAT, "DoF_Setup") )
		{
			TraceError{TracerTag} << "Failed to create DoF setup target !";

			return false;
		}

		/* Near-CoC dilation targets (half-res, single channel). */
		if ( !m_dilateHTarget.create(renderer, halfWidth, halfHeight, VK_FORMAT_R16_SFLOAT, "DoF_DilateH") )
		{
			TraceError{TracerTag} << "Failed to create DoF dilate H target !";

			return false;
		}

		if ( !m_dilateVTarget.create(renderer, halfWidth, halfHeight, VK_FORMAT_R16_SFLOAT, "DoF_DilateV") )
		{
			TraceError{TracerTag} << "Failed to create DoF dilate V target !";

			return false;
		}

		/* Gather targets (half-res). */
		if ( !m_farGatherTarget.create(renderer, halfWidth, halfHeight, VK_FORMAT_R16G16B16A16_SFLOAT, "DoF_FarGather") )
		{
			TraceError{TracerTag} << "Failed to create DoF far gather target !";

			return false;
		}

		if ( !m_nearGatherTarget.create(renderer, halfWidth, halfHeight, VK_FORMAT_R16G16B16A16_SFLOAT, "DoF_NearGather") )
		{
			TraceError{TracerTag} << "Failed to create DoF near gather target !";

			return false;
		}

		/* Output at full resolution. */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "DoF_Output") )
		{
			TraceError{TracerTag} << "Failed to create DoF output target !";

			return false;
		}

		/* ---- Descriptor set layouts (shared from base class) ---- */
		auto singleLayout = this->getInputLayout(1);
		auto dualLayout = this->getInputLayout(2);
		auto tripleLayout = this->getInputLayout(3);
		auto quadLayout = this->getInputLayout(4);

		if ( singleLayout == nullptr || dualLayout == nullptr || tripleLayout == nullptr || quadLayout == nullptr )
		{
			TraceError{TracerTag} << "Failed to get the descriptor set layouts !";

			return false;
		}

		/* ---- Pipeline layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		{
			/* Focus: depth + previous focus. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(dualLayout);

			m_focusLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FocusPushConstants)}
			});
		}

		{
			/* Setup: color + depth + focus. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(tripleLayout);

			m_setupLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SetupPushConstants)}
			});
		}

		{
			/* Dilate: one input. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(singleLayout);

			m_dilateLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(DilatePushConstants)}
			});
		}

		{
			/* Far gather: setup only. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(singleLayout);

			m_farGatherLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GatherPushConstants)}
			});
		}

		{
			/* Near gather: setup + dilated near CoC. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(dualLayout);

			m_nearGatherLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(GatherPushConstants)}
			});
		}

		{
			/* Composite: original + far + near + material properties. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(quadLayout);

			m_compositeLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePushConstants)}
			});
		}

		if ( m_focusLayout == nullptr || m_setupLayout == nullptr || m_dilateLayout == nullptr || m_farGatherLayout == nullptr || m_nearGatherLayout == nullptr || m_compositeLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto focusFragment = shaderManager.getShaderModuleFromSourceCode(device, "DoF_Focus_FS", ShaderType::FragmentShader, DoFFocusFragmentShader);
		const auto setupFragment = shaderManager.getShaderModuleFromSourceCode(device, "DoF_Setup_FS", ShaderType::FragmentShader, DoFSetupFragmentShader);
		const auto dilateFragment = shaderManager.getShaderModuleFromSourceCode(device, "DoF_Dilate_FS", ShaderType::FragmentShader, DoFDilateFragmentShader);
		const auto farGatherFragment = shaderManager.getShaderModuleFromSourceCode(device, "DoF_FarGather_FS", ShaderType::FragmentShader, DoFFarGatherFragmentShader);
		const auto nearGatherFragment = shaderManager.getShaderModuleFromSourceCode(device, "DoF_NearGather_FS", ShaderType::FragmentShader, DoFNearGatherFragmentShader);
		const auto compositeFragment = shaderManager.getShaderModuleFromSourceCode(device, "DoF_Composite_FS", ShaderType::FragmentShader, DoFCompositeFragmentShader);

		if ( vertexModule == nullptr || focusFragment == nullptr || setupFragment == nullptr || dilateFragment == nullptr || farGatherFragment == nullptr || nearGatherFragment == nullptr || compositeFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile DoF shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		/* NOTE: The focus pipeline is created against m_focusTargets[0] and records into
		 * both ping-pong targets (render pass compatibility, identical format/ops). */
		m_focusPipeline = this->createFullscreenPipeline(ClassId, "DoF_Focus", vertexModule, focusFragment, m_focusLayout, m_focusTargets[0]);
		m_setupPipeline = this->createFullscreenPipeline(ClassId, "DoF_Setup", vertexModule, setupFragment, m_setupLayout, m_setupTarget);
		m_dilatePipeline = this->createFullscreenPipeline(ClassId, "DoF_Dilate", vertexModule, dilateFragment, m_dilateLayout, m_dilateHTarget);
		m_farGatherPipeline = this->createFullscreenPipeline(ClassId, "DoF_FarGather", vertexModule, farGatherFragment, m_farGatherLayout, m_farGatherTarget);
		m_nearGatherPipeline = this->createFullscreenPipeline(ClassId, "DoF_NearGather", vertexModule, nearGatherFragment, m_nearGatherLayout, m_nearGatherTarget);
		m_compositePipeline = this->createFullscreenPipeline(ClassId, "DoF_Composite", vertexModule, compositeFragment, m_compositeLayout, m_outputTarget);

		if ( m_focusPipeline == nullptr || m_setupPipeline == nullptr || m_dilatePipeline == nullptr || m_farGatherPipeline == nullptr || m_nearGatherPipeline == nullptr || m_compositePipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */

		/* Focus: depth (per-frame) + previous focus (ping-pong, rewritten per frame). */
		m_focusPerFrame = this->createPerFrameDescriptorSets(dualLayout, ClassId, "Focus_DescSet");

		/* Setup: color + depth (per-frame) + fresh focus (ping-pong, rewritten per frame). */
		m_setupPerFrame = this->createPerFrameDescriptorSets(tripleLayout, ClassId, "Setup_DescSet");

		/* Composite: original color + material properties per-frame; gathers static. */
		m_compositePerFrame = this->createPerFrameDescriptorSets(quadLayout, ClassId, "Composite_DescSet");

		if ( m_focusPerFrame.empty() || m_setupPerFrame.empty() || m_compositePerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_compositePerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(1, m_farGatherTarget) )
			{
				return false;
			}

			if ( !descriptorSet->writeCombinedImageSampler(2, m_nearGatherTarget) )
			{
				return false;
			}
		}

		/* Static sets: dilate H (setup), dilate V (dilate H), far gather (setup),
		 * near gather (setup + dilate V). */
		{
			const auto & pool = renderer.descriptorPool();

			m_dilateHDescSet = std::make_unique< DescriptorSet >(pool, singleLayout);
			m_dilateHDescSet->setIdentifier(ClassId, "DilateH_DescSet", "DescriptorSet");

			if ( !m_dilateHDescSet->create() || !m_dilateHDescSet->writeCombinedImageSampler(0, m_setupTarget) )
			{
				return false;
			}

			m_dilateVDescSet = std::make_unique< DescriptorSet >(pool, singleLayout);
			m_dilateVDescSet->setIdentifier(ClassId, "DilateV_DescSet", "DescriptorSet");

			if ( !m_dilateVDescSet->create() || !m_dilateVDescSet->writeCombinedImageSampler(0, m_dilateHTarget) )
			{
				return false;
			}

			m_farGatherDescSet = std::make_unique< DescriptorSet >(pool, singleLayout);
			m_farGatherDescSet->setIdentifier(ClassId, "FarGather_DescSet", "DescriptorSet");

			if ( !m_farGatherDescSet->create() || !m_farGatherDescSet->writeCombinedImageSampler(0, m_setupTarget) )
			{
				return false;
			}

			m_nearGatherDescSet = std::make_unique< DescriptorSet >(pool, dualLayout);
			m_nearGatherDescSet->setIdentifier(ClassId, "NearGather_DescSet", "DescriptorSet");

			if ( !m_nearGatherDescSet->create() || !m_nearGatherDescSet->writeCombinedImageSampler(0, m_setupTarget) || !m_nearGatherDescSet->writeCombinedImageSampler(1, m_dilateVTarget) )
			{
				return false;
			}
		}

		return true;
	}

	void
	DepthOfField::destroy () noexcept
	{
		m_nearGatherDescSet.reset();
		m_farGatherDescSet.reset();
		m_dilateVDescSet.reset();
		m_dilateHDescSet.reset();
		m_compositePerFrame.clear();
		m_setupPerFrame.clear();
		m_focusPerFrame.clear();

		m_compositePipeline.reset();
		m_nearGatherPipeline.reset();
		m_farGatherPipeline.reset();
		m_dilatePipeline.reset();
		m_setupPipeline.reset();
		m_focusPipeline.reset();
		m_compositeLayout.reset();
		m_nearGatherLayout.reset();
		m_farGatherLayout.reset();
		m_dilateLayout.reset();
		m_setupLayout.reset();
		m_focusLayout.reset();

		m_outputTarget.destroy();
		m_nearGatherTarget.destroy();
		m_farGatherTarget.destroy();
		m_dilateVTarget.destroy();
		m_dilateHTarget.destroy();
		m_setupTarget.destroy();

		for ( auto & target : m_focusTargets )
		{
			target.destroy();
		}

		m_focusValid = false;
		m_focusWriteIndex = 0;
	}

	const TextureInterface &
	DepthOfField::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputMaterialProperties = context.materialProperties;
		const auto & constants = context.constants;

		const auto frameIndex = this->renderer().currentFrameIndex();

		const uint32_t writeIdx = m_focusWriteIndex;
		const uint32_t readIdx = 1U - writeIdx;

		/* Effective optics: the ACTIVE CAMERA is the single source of truth for the
		 * photographic options; the local parameters only act as a fallback. */
		float aperture = m_parameters.aperture;
		float focalLength = m_parameters.focalLength;
		float focusDistance = m_parameters.focusDistance;
		bool autoFocus = m_parameters.autoFocus;

		if ( context.camera != nullptr )
		{
			aperture = context.camera->aperture();
			focalLength = context.camera->focalLength();
			focusDistance = context.camera->focusDistance();
			autoFocus = context.camera->isAutoFocusEnabled();
		}

		/* ---- Per-frame descriptor updates ---- */

		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_focusPerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
			static_cast< void >(m_setupPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputDepth));
		}

		/* Focus ping-pong: this frame reads [readIdx] and writes [writeIdx]. */
		static_cast< void >(m_focusPerFrame[frameIndex]->writeCombinedImageSampler(1, m_focusTargets[readIdx]));
		static_cast< void >(m_setupPerFrame[frameIndex]->writeCombinedImageSampler(2, m_focusTargets[writeIdx]));

		static_cast< void >(m_setupPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));
		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(3, *inputMaterialProperties));
		}

		/* ---- Pass 1: Auto-focus (1x1, rack focus EMA) ---- */
		{
			uint32_t flags = 0;

			if ( autoFocus )
			{
				flags |= 1U;
			}

			if ( !m_focusValid )
			{
				flags |= 2U;
			}

			const FocusPushConstants pc{
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.focusDistance = focusDistance,
				.autoFocusSpeed = m_parameters.autoFocusSpeed,
				.time = constants.time,
				.texelSizeX = 1.0F / static_cast< float >(m_outputTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_outputTarget.height()),
				.flags = flags
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_focusTargets[writeIdx],
				*m_focusPipeline,
				*m_focusLayout,
				*m_focusPerFrame[frameIndex],
				&pc,
				sizeof(pc)
			);
		}

		/* ---- Pass 2: CoC setup (signed, half-res) ---- */
		{
			const SetupPushConstants pc{
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.aperture = aperture,
				.focalLength = focalLength,
				.cocScale = m_parameters.cocScale,
				.padding1 = 0.0F,
				.padding2 = 0.0F,
				.padding3 = 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_setupTarget,
				*m_setupPipeline,
				*m_setupLayout,
				*m_setupPerFrame[frameIndex],
				&pc,
				sizeof(pc)
			);
		}

		/* ---- Passes 3/4: Near-CoC dilation (H then V) ---- */
		if ( m_parameters.nearFieldEnabled )
		{
			const auto radius = static_cast< int32_t >(m_parameters.maxCoCRadius);

			const DilatePushConstants dilateH{
				.texelSizeX = 1.0F / static_cast< float >(m_dilateHTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_dilateHTarget.height()),
				.directionX = 1.0F,
				.directionY = 0.0F,
				.radius = radius,
				.extractFromAlpha = 1U
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_dilateHTarget,
				*m_dilatePipeline,
				*m_dilateLayout,
				*m_dilateHDescSet,
				&dilateH,
				sizeof(dilateH)
			);

			const DilatePushConstants dilateV{
				.texelSizeX = 1.0F / static_cast< float >(m_dilateVTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_dilateVTarget.height()),
				.directionX = 0.0F,
				.directionY = 1.0F,
				.radius = radius,
				.extractFromAlpha = 0U
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_dilateVTarget,
				*m_dilatePipeline,
				*m_dilateLayout,
				*m_dilateVDescSet,
				&dilateV,
				sizeof(dilateV)
			);
		}

		/* ---- Pass 5: Far-field gather (circular bokeh) ---- */
		{
			const GatherPushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_farGatherTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_farGatherTarget.height()),
				.maxCoCRadius = m_parameters.maxCoCRadius,
				.sampleCount = m_parameters.sampleCount
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_farGatherTarget,
				*m_farGatherPipeline,
				*m_farGatherLayout,
				*m_farGatherDescSet,
				&pc,
				sizeof(pc)
			);
		}

		/* ---- Pass 6: Near-field gather (foreground bleed) ---- */
		if ( m_parameters.nearFieldEnabled )
		{
			const GatherPushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_nearGatherTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_nearGatherTarget.height()),
				.maxCoCRadius = m_parameters.maxCoCRadius,
				.sampleCount = m_parameters.sampleCount
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_nearGatherTarget,
				*m_nearGatherPipeline,
				*m_nearGatherLayout,
				*m_nearGatherDescSet,
				&pc,
				sizeof(pc)
			);
		}

		/* ---- Pass 7: Composite ---- */
		{
			const CompositePushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_outputTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_outputTarget.height()),
				.nearFieldEnabled = m_parameters.nearFieldEnabled ? 1U : 0U,
				.padding = 0.0F
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_outputTarget,
				*m_compositePipeline,
				*m_compositeLayout,
				*m_compositePerFrame[frameIndex],
				&pc,
				sizeof(pc)
			);
		}

		/* Flip the focus ping-pong for the next frame. */
		m_focusWriteIndex = readIdx;
		m_focusValid = true;

		return m_outputTarget;
	}
}
