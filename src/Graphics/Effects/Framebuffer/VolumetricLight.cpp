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
 * https://github.com/londnoir/emeraude-engine
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
};

void main()
{
	float depth = texture(depthTex, vUV).r;

	/* Sky pixels emit light, geometry blocks it. */
	float isLit = (depth >= depthThreshold) ? 1.0 : 0.0;

	vec3 lightColor = vec3(lightColorR, lightColorG, lightColorB);
	outColor = vec4(lightColor * lightIntensity * isLit, isLit);
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
};

void main()
{
	vec2 lightPos = vec2(lightScreenX, lightScreenY);
	vec2 deltaUV = (vUV - lightPos);
	deltaUV *= (1.0 / float(numSamples)) * density;

	vec2 sampleUV = vUV;
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

	static constexpr auto CompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;
layout(set = 0, binding = 1) uniform sampler2D shaftsTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float padding1;
	float padding2;
};

void main()
{
	vec3 scene = texture(sceneTex, vUV).rgb;
	vec3 shafts = texture(shaftsTex, vUV).rgb;
	outColor = vec4(scene + shafts, 1.0);
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
	VolumetricLight::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		constexpr auto format = VK_FORMAT_R16G16B16A16_SFLOAT;

		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		/* Create occlusion target (half-res). */
		if ( !m_occlusionTarget.create(renderer, halfW, halfH, format, "VL_Occlusion") )
		{
			TraceError{TracerTag} << "Failed to create occlusion target !";

			return false;
		}

		/* Create radial blur target (half-res). */
		if ( !m_radialTarget.create(renderer, halfW, halfH, format, "VL_Radial") )
		{
			TraceError{TracerTag} << "Failed to create radial blur target !";

			return false;
		}

		/* Create output target (full-res). */
		if ( !m_outputTarget.create(renderer, width, height, format, "VL_Output") )
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
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(singleInputLayout);

			m_occlusionLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScatterPushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(singleInputLayout);

			m_radialLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScatterPushConstants)}
			});
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(dualInputLayout);

			m_compositeLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(CompositePushConstants)}
			});
		}

		if ( m_occlusionLayout == nullptr || m_radialLayout == nullptr || m_compositeLayout == nullptr )
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

		const auto compositeFragment = shaderManager.getShaderModuleFromSourceCode(device, "VL_Composite_FS", ShaderType::FragmentShader, CompositeFragmentShader);

		if ( compositeFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile composite shader !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_occlusionPipeline = this->createFullscreenPipeline(ClassId, "VL_Occlusion", vertexModule, occlusionFragment, m_occlusionLayout, m_occlusionTarget);
		m_radialPipeline = this->createFullscreenPipeline(ClassId, "VL_Radial", vertexModule, radialFragment, m_radialLayout, m_radialTarget);
		m_compositePipeline = this->createFullscreenPipeline(ClassId, "VL_Composite", vertexModule, compositeFragment, m_compositeLayout, m_outputTarget);

		if ( m_occlusionPipeline == nullptr || m_radialPipeline == nullptr || m_compositePipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */

		/* Occlusion: reads depth (updated per-frame). */
		m_occlusionPerFrame = this->createPerFrameDescriptorSets(singleInputLayout, ClassId, "VL_Occlusion_DescSet");

		if ( m_occlusionPerFrame.empty() )
		{
			return false;
		}

		/* Radial blur: reads occlusion target (fixed, single set). */
		const auto & pool = renderer.descriptorPool();

		m_radialDescSet = std::make_unique< DescriptorSet >(pool, singleInputLayout);
		m_radialDescSet->setIdentifier(ClassId, "Radial_DescSet", "DescriptorSet");

		if ( !m_radialDescSet->create() )
		{
			return false;
		}

		if ( !m_radialDescSet->writeCombinedImageSampler(0, m_occlusionTarget) )
		{
			return false;
		}

		/* Composite: reads scene color (updated per-frame, binding 0) + radial result (fixed, binding 1). */
		m_compositePerFrame = this->createPerFrameDescriptorSets(dualInputLayout, ClassId, "VL_Composite_DescSet");

		if ( m_compositePerFrame.empty() )
		{
			return false;
		}

		/* Write binding 1 (radial blur result) for each composite frame descriptor. */
		for ( const auto & descriptorSet : m_compositePerFrame )
		{
			if ( !descriptorSet->writeCombinedImageSampler(1, m_radialTarget) )
			{
				return false;
			}
		}

		return true;
	}

	void
	VolumetricLight::destroy () noexcept
	{
		m_compositePerFrame.clear();
		m_radialDescSet.reset();
		m_occlusionPerFrame.clear();

		m_compositePipeline.reset();
		m_radialPipeline.reset();
		m_occlusionPipeline.reset();
		m_compositeLayout.reset();
		m_radialLayout.reset();
		m_occlusionLayout.reset();

		m_outputTarget.destroy();
		m_radialTarget.destroy();
		m_occlusionTarget.destroy();
	}

	const TextureInterface &
	VolumetricLight::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const TextureInterface * inputDepth, [[maybe_unused]] const TextureInterface * inputNormals, [[maybe_unused]] const TextureInterface * inputMaterialProperties, const Scenes::LightSet * lightSet, const PostProcessor::PushConstants & constants) noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();

		/* 1. Project light direction to screen space.
		 * Use readStateIndex to match the view matrix that produced the depth buffer. */
		const auto readStateIndex = this->renderer().currentReadStateIndex();
		const auto & viewMatrices =this->renderer().mainRenderTarget()->viewMatrices();
		const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
		const auto & projMat = viewMatrices.projectionMatrix(readStateIndex);
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

		/* 2. Update per-frame occlusion descriptor with depth texture. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_occlusionPerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		/* 3. Update per-frame composite descriptor with scene color. */
		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Build scatter push constants (shared by occlusion and radial passes). */
		const ScatterPushConstants scatterPC{
			.lightScreenX = screenX,
			.lightScreenY = screenY,
			.texelSizeX = 1.0F / static_cast< float >(m_occlusionTarget.width()),
			.texelSizeY = 1.0F / static_cast< float >(m_occlusionTarget.height()),
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
			.lightOnScreen = lightOnScreen
		};

		/* 4. Pass 1: Occlusion extraction. */
		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_occlusionTarget,
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
			*m_radialDescSet,
			&scatterPC,
			sizeof(ScatterPushConstants)
		);

		/* 6. Pass 3: Composite (additive blend scene + light shafts). */
		const CompositePushConstants compositePC{
			.texelSizeX = 1.0F / static_cast< float >(m_outputTarget.width()),
			.texelSizeY = 1.0F / static_cast< float >(m_outputTarget.height()),
			.padding1 = 0.0F,
			.padding2 = 0.0F
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
