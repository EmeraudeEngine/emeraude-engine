/*
 * src/Graphics/Effects/Framebuffer/Sharpen.cpp
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

#include "Sharpen.hpp"

/* STL inclusions. */
#include <string>

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

/* Defining the resource owner of this translation unit. */
/* NOLINTBEGIN(cert-err58-cpp) : We need static strings. */
static constexpr auto TracerTag{"SharpenEffect"};
/* NOLINTEND(cert-err58-cpp) */

namespace
{
	using namespace EmEn;

	/* ---- Push Constants ---- */

	struct SharpenPushConstants
	{
		float sharpness;
		float padding0;
		float padding1;
		float padding2;
	};

	static_assert(sizeof(SharpenPushConstants) == 16, "SharpenPushConstants must be 16 bytes.");

	/* ---- GLSL Shader Sources ---- */

	static constexpr auto SharpenFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTex;

layout(push_constant) uniform PushConstants
{
	float sharpness;
	float padding0;
	float padding1;
	float padding2;
};

void main()
{
	/* 5-tap cross pattern using textureOffset (no texel size needed). */
	vec3 center = texture(inputTex, vUV).rgb;
	vec3 north  = textureOffset(inputTex, vUV, ivec2( 0, -1)).rgb;
	vec3 south  = textureOffset(inputTex, vUV, ivec2( 0,  1)).rgb;
	vec3 east   = textureOffset(inputTex, vUV, ivec2( 1,  0)).rgb;
	vec3 west   = textureOffset(inputTex, vUV, ivec2(-1,  0)).rgb;

	/* Compute luminance (Rec. 709). */
	const vec3 lumaW = vec3(0.2126, 0.7152, 0.0722);
	float lN = dot(north, lumaW);
	float lS = dot(south, lumaW);
	float lE = dot(east, lumaW);
	float lW = dot(west, lumaW);
	float lC = dot(center, lumaW);

	/* Local contrast: range of the 5-tap cross. */
	float mn = min(lC, min(min(lN, lS), min(lE, lW)));
	float mx = max(lC, max(max(lN, lS), max(lE, lW)));
	float contrast = mx - mn;

	/* Adaptive weight: quadratic falloff in high-contrast areas to prevent ringing. */
	float peak = max(1.0 - contrast, 0.0);
	float w = peak * peak * sharpness;

	/* Highpass sharpening: center minus neighborhood average, scaled by weight. */
	vec3 average = (north + south + east + west) * 0.25;
	vec3 result = center + (center - average) * w;

	outColor = vec4(clamp(result, 0.0, 1.0), 1.0);
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
	Sharpen::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		/* Output is LDR (8-bit per channel). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R8G8B8A8_UNORM, "SharpenOutput") )
		{
			TraceError{TracerTag} << "Failed to create sharpen output target !";

			return false;
		}

		/* Compile shaders. */
		auto vertexModule = this->getFullscreenVertexShader();

		auto fragmentModule = renderer.shaderManager().getShaderModuleFromSourceCode(
			renderer.device(), "SharpenFS", Saphir::ShaderType::FragmentShader, SharpenFragmentShader
		);

		if ( vertexModule == nullptr || fragmentModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile sharpen shaders !";

			return false;
		}

		/* Descriptor set layout: 1 combined image sampler. */
		auto descriptorSetLayout = this->getInputLayout(1);

		if ( descriptorSetLayout == nullptr )
		{
			TraceError{TracerTag} << "Failed to get single-input descriptor set layout !";

			return false;
		}

		/* Pipeline layout. */
		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(descriptorSetLayout);

			/* Push constant range (16 bytes). */
			m_pipelineLayout = renderer.layoutManager().getPipelineLayout(sets, {
				VkPushConstantRange{
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(SharpenPushConstants)
				}
			});
		}

		if ( m_pipelineLayout == nullptr )
		{
			TraceError{TracerTag} << "Failed to create pipeline layout !";

			return false;
		}

		/* Graphics pipeline. */
		m_pipeline = this->createFullscreenPipeline(ClassId, "Sharpen", vertexModule, fragmentModule, m_pipelineLayout, m_outputTarget);

		if ( m_pipeline == nullptr )
		{
			TraceError{TracerTag} << "Failed to create sharpen pipeline !";

			return false;
		}

		/* Per-frame descriptor sets. */
		m_descriptorSets = this->createPerFrameDescriptorSets(descriptorSetLayout, ClassId, "SharpenDescSet");

		if ( m_descriptorSets.empty() )
		{
			TraceError{TracerTag} << "Failed to create per-frame descriptor sets !";

			return false;
		}

		return true;
	}

	void
	Sharpen::destroy () noexcept
	{
		m_descriptorSets.clear();
		m_pipeline.reset();
		m_pipelineLayout.reset();
		m_outputTarget.destroy();
	}

	/* ---- Execute ---- */

	const TextureInterface &
	Sharpen::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, [[maybe_unused]] const TextureInterface * inputDepth, [[maybe_unused]] const TextureInterface * inputNormals, [[maybe_unused]] const TextureInterface * inputMaterialProperties, [[maybe_unused]] const Scenes::LightSet * lightSet, [[maybe_unused]] const PostProcessor::PushConstants & constants) noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Update the per-frame descriptor set with the current input. */
		static_cast< void >(m_descriptorSets[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Build push constants. */
		const SharpenPushConstants pc{
			.sharpness = m_parameters.sharpness,
			.padding0 = 0.0F,
			.padding1 = 0.0F,
			.padding2 = 0.0F
		};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_outputTarget,
			*m_pipeline,
			*m_pipelineLayout,
			*m_descriptorSets[frameIndex],
			&pc,
			sizeof(pc)
		);

		return m_outputTarget;
	}
}
