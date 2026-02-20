/*
 * src/Graphics/Effects/Bloom.cpp
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

#include "Bloom.hpp"

/* STL inclusions. */
#include <string>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ShaderModule.hpp"

/* Defining the resource owner of this translation unit. */
/* NOLINTBEGIN(cert-err58-cpp) : We need static strings. */
static constexpr auto TracerTag{"BloomEffect"};
/* NOLINTEND(cert-err58-cpp) */

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ---- */

	static constexpr auto FullscreenVertexShader = R"GLSL(
#version 450

layout(location = 0) out vec2 vUV;

void main()
{
	vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

	static constexpr auto DownsampleFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float threshold;
	float softKnee;
	float intensity;
	float spread;
};

/* 13-tap downsample filter (Jimenez, COD:MW). */
vec4 downsample13Tap (sampler2D tex, vec2 uv, vec2 ts)
{
	vec4 A = texture(tex, uv + ts * vec2(-1.0, -1.0));
	vec4 B = texture(tex, uv + ts * vec2( 0.0, -1.0));
	vec4 C = texture(tex, uv + ts * vec2( 1.0, -1.0));
	vec4 D = texture(tex, uv + ts * vec2(-0.5, -0.5));
	vec4 E = texture(tex, uv + ts * vec2( 0.5, -0.5));
	vec4 F = texture(tex, uv + ts * vec2(-1.0,  0.0));
	vec4 G = texture(tex, uv);
	vec4 H = texture(tex, uv + ts * vec2( 1.0,  0.0));
	vec4 I = texture(tex, uv + ts * vec2(-0.5,  0.5));
	vec4 J = texture(tex, uv + ts * vec2( 0.5,  0.5));
	vec4 K = texture(tex, uv + ts * vec2(-1.0,  1.0));
	vec4 L = texture(tex, uv + ts * vec2( 0.0,  1.0));
	vec4 M = texture(tex, uv + ts * vec2( 1.0,  1.0));

	vec4 result = vec4(0.0);
	result += (D + E + I + J) * 0.5;
	result += (A + B + F + G) * 0.125;
	result += (B + C + G + H) * 0.125;
	result += (F + G + K + L) * 0.125;
	result += (G + H + L + M) * 0.125;

	return result * 0.25;
}

/* Soft brightness thresholding. */
vec3 applyThreshold (vec3 color, float thresh, float knee)
{
	float brightness = max(max(color.r, color.g), color.b);
	float kneeWidth = thresh * knee;
	float soft = brightness - thresh + kneeWidth;
	soft = clamp(soft, 0.0, 2.0 * kneeWidth);
	soft = soft * soft / (4.0 * kneeWidth + 0.00001);
	float contribution = max(soft, brightness - thresh) / max(brightness, 0.00001);
	return color * contribution;
}

void main()
{
	vec2 ts = vec2(texelSizeX, texelSizeY);
	vec4 color = downsample13Tap(inputTex, vUV, ts);

	if (threshold > 0.0)
	{
		color.rgb = applyThreshold(color.rgb, threshold, softKnee);
	}

	outColor = color;
}
)GLSL";

	static constexpr auto UpsampleFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTex;
layout(set = 0, binding = 1) uniform sampler2D currentLevel;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float threshold;
	float softKnee;
	float intensity;
	float spread;
};

/* 9-tap tent filter upsample. */
vec4 upsampleTent (sampler2D tex, vec2 uv, vec2 ts, float sampleScale)
{
	vec2 s = ts * sampleScale;

	vec4 result = vec4(0.0);
	result += texture(tex, uv + vec2(-s.x, -s.y));
	result += texture(tex, uv + vec2( 0.0, -s.y)) * 2.0;
	result += texture(tex, uv + vec2( s.x, -s.y));
	result += texture(tex, uv + vec2(-s.x,  0.0)) * 2.0;
	result += texture(tex, uv)                      * 4.0;
	result += texture(tex, uv + vec2( s.x,  0.0)) * 2.0;
	result += texture(tex, uv + vec2(-s.x,  s.y));
	result += texture(tex, uv + vec2( 0.0,  s.y)) * 2.0;
	result += texture(tex, uv + vec2( s.x,  s.y));

	return result / 16.0;
}

void main()
{
	vec2 ts = vec2(texelSizeX, texelSizeY);
	vec4 upsampled = upsampleTent(inputTex, vUV, ts, spread);
	vec4 current = texture(currentLevel, vUV);

	outColor = current + upsampled;
}
)GLSL";

	static constexpr auto CompositeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D originalTex;
layout(set = 0, binding = 1) uniform sampler2D bloomTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float threshold;
	float softKnee;
	float intensity;
	float spread;
};

void main()
{
	vec4 original = texture(originalTex, vUV);
	vec4 bloom = texture(bloomTex, vUV);

	outColor = original + bloom * intensity;
}
)GLSL";

	/**
	 * @brief Creates a fullscreen-pass graphics pipeline for an IntermediateRenderTarget.
	 * @param renderer A reference to the renderer.
	 * @param name The pipeline name for debugging.
	 * @param vertexModule The vertex shader module.
	 * @param fragmentModule The fragment shader module.
	 * @param pipelineLayout The pipeline layout.
	 * @param target The IRT whose render pass will be used for pipeline creation.
	 * @return std::shared_ptr< Vulkan::GraphicsPipeline >
	 */
	[[nodiscard]]
	std::shared_ptr< Vulkan::GraphicsPipeline >
	createFullscreenPipeline (
		Graphics::Renderer & renderer,
		const std::string & name,
		const std::shared_ptr< Vulkan::ShaderModule > & vertexModule,
		const std::shared_ptr< Vulkan::ShaderModule > & fragmentModule,
		const std::shared_ptr< Vulkan::PipelineLayout > & pipelineLayout,
		const Graphics::IntermediateRenderTarget & target
	) noexcept
	{
		auto pipeline = std::make_shared< Vulkan::GraphicsPipeline >(renderer.device());
		pipeline->setIdentifier(TracerTag, name, "GraphicsPipeline");

		/* Shader stages. */
		Libs::StaticVector< std::shared_ptr< Vulkan::ShaderModule >, 5 > shaderModules;
		shaderModules.emplace_back(vertexModule);
		shaderModules.emplace_back(fragmentModule);

		if ( !pipeline->configureShaderStages(shaderModules) )
		{
			return nullptr;
		}

		/* Empty vertex input (fullscreen triangle generated in vertex shader). */
		if ( !pipeline->configureEmptyVertexInputState() )
		{
			return nullptr;
		}

		/* Triangle list. */
		if ( !pipeline->configureInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) )
		{
			return nullptr;
		}

		/* Dynamic viewport and scissor. */
		Libs::StaticVector< VkDynamicState, 16 > dynamicStates;
		dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);

		if ( !pipeline->configureDynamicStates(dynamicStates) )
		{
			return nullptr;
		}

		/* Viewport (initial, overridden by dynamic state). */
		if ( !pipeline->configureViewportState(target.width(), target.height()) )
		{
			return nullptr;
		}

		/* Rasterization: no culling, fill. */
		VkPipelineRasterizationStateCreateInfo rasterization{};
		rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization.rasterizerDiscardEnable = VK_FALSE;
		rasterization.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization.cullMode = VK_CULL_MODE_NONE;
		rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterization.depthBiasEnable = VK_FALSE;
		rasterization.lineWidth = 1.0F;

		if ( !pipeline->configureRasterizationState(rasterization) )
		{
			return nullptr;
		}

		/* Single-sample. */
		if ( !pipeline->configureMultisampleState(1) )
		{
			return nullptr;
		}

		/* No depth test. */
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		if ( !pipeline->configureDepthStencilState(depthStencil) )
		{
			return nullptr;
		}

		/* Opaque color blend (no blending). */
		Libs::StaticVector< VkPipelineColorBlendAttachmentState, 8 > attachments;
		attachments.emplace_back(VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
			                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		});

		VkPipelineColorBlendStateCreateInfo colorBlend{};
		colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlend.logicOpEnable = VK_FALSE;

		if ( !pipeline->configureColorBlendState(attachments, colorBlend) )
		{
			return nullptr;
		}

		/* Finalize against the IRT's render pass. */
		const auto renderPass = target.framebuffer().renderPass();

		if ( !pipeline->finalize(renderPass, pipelineLayout, false, false) )
		{
			return nullptr;
		}

		return pipeline;
	}
}

namespace EmEn::Graphics::Effects
{
	using namespace Vulkan;

	/* ---- Descriptor Set Layouts ---- */

	std::shared_ptr< DescriptorSetLayout >
	Bloom::getSingleInputLayout (Renderer & renderer) noexcept
	{
		static constexpr auto LayoutId = "BloomSingleInput";

		auto & layoutManager = renderer.layoutManager();
		auto layout = layoutManager.getDescriptorSetLayout(LayoutId);

		if ( layout == nullptr )
		{
			layout = layoutManager.prepareNewDescriptorSetLayout(LayoutId);
			layout->setIdentifier(ClassId, LayoutId, "DescriptorSetLayout");
			layout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(layout) )
			{
				return nullptr;
			}
		}

		return layout;
	}

	std::shared_ptr< DescriptorSetLayout >
	Bloom::getDualInputLayout (Renderer & renderer) noexcept
	{
		static constexpr auto LayoutId = "BloomDualInput";

		auto & layoutManager = renderer.layoutManager();
		auto layout = layoutManager.getDescriptorSetLayout(LayoutId);

		if ( layout == nullptr )
		{
			layout = layoutManager.prepareNewDescriptorSetLayout(LayoutId);
			layout->setIdentifier(ClassId, LayoutId, "DescriptorSetLayout");
			layout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			layout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(layout) )
			{
				return nullptr;
			}
		}

		return layout;
	}

	/* ---- Pipeline Creation ---- */

	bool
	Bloom::createPipelines (Renderer & renderer) noexcept
	{
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		/* Compile the shared fullscreen vertex shader. */
		auto vertexModule = shaderManager.getShaderModuleFromSourceCode(
			device, "BloomFullscreenVS", Saphir::ShaderType::VertexShader, FullscreenVertexShader
		);

		if ( vertexModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile bloom vertex shader !";

			return false;
		}

		/* Compile the downsample fragment shader. */
		auto downsampleFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "BloomDownsampleFS", Saphir::ShaderType::FragmentShader, DownsampleFragmentShader
		);

		if ( downsampleFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile bloom downsample shader !";

			return false;
		}

		/* Compile the upsample fragment shader. */
		auto upsampleFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "BloomUpsampleFS", Saphir::ShaderType::FragmentShader, UpsampleFragmentShader
		);

		if ( upsampleFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile bloom upsample shader !";

			return false;
		}

		/* Compile the composite fragment shader. */
		auto compositeFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "BloomCompositeFS", Saphir::ShaderType::FragmentShader, CompositeFragmentShader
		);

		if ( compositeFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile bloom composite shader !";

			return false;
		}

		/* Descriptor set layouts. */
		auto singleInputLayout = getSingleInputLayout(renderer);
		auto dualInputLayout = getDualInputLayout(renderer);

		if ( singleInputLayout == nullptr || dualInputLayout == nullptr )
		{
			return false;
		}

		/* Push constant range (shared by all pipelines). */
		const Libs::StaticVector< VkPushConstantRange, 4 > pushConstantRanges{
			VkPushConstantRange{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(BloomPushConstants)
			}
		};

		/* Pipeline layouts. */
		{
			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(singleInputLayout);

			m_downsampleLayout = renderer.layoutManager().getPipelineLayout(sets, pushConstantRanges);
		}
		{
			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(dualInputLayout);

			m_upsampleLayout = renderer.layoutManager().getPipelineLayout(sets, pushConstantRanges);
		}
		{
			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(dualInputLayout);

			m_compositeLayout = renderer.layoutManager().getPipelineLayout(sets, pushConstantRanges);
		}

		if ( m_downsampleLayout == nullptr || m_upsampleLayout == nullptr || m_compositeLayout == nullptr )
		{
			return false;
		}

		/* Create the three pipelines. */
		m_downsamplePipeline = createFullscreenPipeline(
			renderer, "BloomDownsample", vertexModule, downsampleFragment, m_downsampleLayout, m_downTargets[0]
		);
		m_upsamplePipeline = createFullscreenPipeline(
			renderer, "BloomUpsample", vertexModule, upsampleFragment, m_upsampleLayout, m_upTargets[0]
		);
		m_compositePipeline = createFullscreenPipeline(
			renderer, "BloomComposite", vertexModule, compositeFragment, m_compositeLayout, m_outputTarget
		);

		return m_downsamplePipeline != nullptr && m_upsamplePipeline != nullptr && m_compositePipeline != nullptr;
	}

	/* ---- Descriptor Set Creation ---- */

	bool
	Bloom::createDescriptorSets (Renderer & renderer) noexcept
	{
		const auto singleInputLayout = getSingleInputLayout(renderer);
		const auto dualInputLayout = getDualInputLayout(renderer);
		const auto & pool = renderer.descriptorPool();

		if ( singleInputLayout == nullptr || dualInputLayout == nullptr )
		{
			return false;
		}

		/* Downsample descriptor sets (single input each).
		 * Set [0] is updated per-frame in execute() to point to the input texture,
		 * so we create per-frame-in-flight copies to avoid descriptor update conflicts.
		 * Sets [1..4] are pre-written to the previous downsample target. */
		const auto frameCount = renderer.framesInFlight();

		for ( int i = 0; i < MipLevels; ++i )
		{
			m_downDescSets[static_cast< size_t >(i)] = std::make_unique< DescriptorSet >(pool, singleInputLayout);
			m_downDescSets[static_cast< size_t >(i)]->setIdentifier(ClassId, "DownDescSet" + std::to_string(i), "DescriptorSet");

			if ( !m_downDescSets[static_cast< size_t >(i)]->create() )
			{
				return false;
			}

			if ( i > 0 )
			{
				const auto & input = m_downTargets[static_cast< size_t >(i - 1)];

				if ( !m_downDescSets[static_cast< size_t >(i)]->writeCombinedImageSampler(0, input) )
				{
					return false;
				}
			}
		}

		/* Per-frame copies of m_downDescSets[0] for safe per-frame updates. */
		m_downFirstPerFrame.clear();
		m_downFirstPerFrame.reserve(frameCount);

		for ( uint32_t f = 0; f < frameCount; ++f )
		{
			auto ds = std::make_unique< DescriptorSet >(pool, singleInputLayout);
			ds->setIdentifier(ClassId, "DownDescSet0-F" + std::to_string(f), "DescriptorSet");

			if ( !ds->create() )
			{
				return false;
			}

			m_downFirstPerFrame.emplace_back(std::move(ds));
		}

		/* Upsample descriptor sets (dual input each).
		 * Binding 0 = previous upsample output (or bottom mip), binding 1 = downsample at this level. */
		for ( int i = 0; i < MipLevels - 1; ++i )
		{
			m_upDescSets[static_cast< size_t >(i)] = std::make_unique< DescriptorSet >(pool, dualInputLayout);
			m_upDescSets[static_cast< size_t >(i)]->setIdentifier(ClassId, "UpDescSet" + std::to_string(i), "DescriptorSet");

			if ( !m_upDescSets[static_cast< size_t >(i)]->create() )
			{
				return false;
			}

			/* Binding 0: previous level (bottom mip for first pass, then upTargets). */
			if ( i == 0 )
			{
				/* First upsample reads from the smallest downsample target. */
				if ( !m_upDescSets[0]->writeCombinedImageSampler(0, m_downTargets[MipLevels - 1]) )
				{
					return false;
				}
			}
			else
			{
				if ( !m_upDescSets[static_cast< size_t >(i)]->writeCombinedImageSampler(0, m_upTargets[static_cast< size_t >(i - 1)]) )
				{
					return false;
				}
			}

			/* Binding 1: downsample at this level.
			 * Upsample pass i writes to upTargets[i], which has the same resolution as downTargets[MipLevels - 2 - i].
			 * We blend with the downsample at the target resolution. */
			const auto downIndex = static_cast< size_t >(MipLevels - 2 - i);

			if ( !m_upDescSets[static_cast< size_t >(i)]->writeCombinedImageSampler(1, m_downTargets[downIndex]) )
			{
				return false;
			}
		}

		/* Composite descriptor sets (dual input, per-frame-in-flight).
		 * Binding 0 = original input (updated per-frame), binding 1 = final bloom (top upsample). */
		m_compositePerFrame.clear();
		m_compositePerFrame.reserve(frameCount);

		for ( uint32_t f = 0; f < frameCount; ++f )
		{
			auto ds = std::make_unique< DescriptorSet >(pool, dualInputLayout);
			ds->setIdentifier(ClassId, "CompositeDescSet-F" + std::to_string(f), "DescriptorSet");

			if ( !ds->create() )
			{
				return false;
			}

			/* Binding 1: top upsample result (same for all frames). */
			if ( !ds->writeCombinedImageSampler(1, m_upTargets[MipLevels - 2]) )
			{
				return false;
			}

			m_compositePerFrame.emplace_back(std::move(ds));
		}

		return true;
	}

	/* ---- Lifecycle ---- */

	bool
	Bloom::create (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		constexpr auto format = VK_FORMAT_R16G16B16A16_SFLOAT;

		/* Create downsample targets (halving resolution at each level). */
		auto mipW = width / 2;
		auto mipH = height / 2;

		for ( int i = 0; i < MipLevels; ++i )
		{
			if ( mipW == 0 ) { mipW = 1; }
			if ( mipH == 0 ) { mipH = 1; }

			if ( !m_downTargets[static_cast< size_t >(i)].create(renderer, mipW, mipH, format, "BloomDown" + std::to_string(i)) )
			{
				TraceError{TracerTag} << "Failed to create downsample target " << i << " !";

				return false;
			}

			mipW /= 2;
			mipH /= 2;
		}

		/* Create upsample targets (matching downsample levels in reverse, excluding the smallest).
		 * upTargets[0] matches downTargets[MipLevels-2] resolution (1/16),
		 * upTargets[MipLevels-2] matches downTargets[0] resolution (1/2). */
		for ( int i = 0; i < MipLevels - 1; ++i )
		{
			const auto downIndex = static_cast< size_t >(MipLevels - 2 - i);
			const auto upW = m_downTargets[downIndex].width();
			const auto upH = m_downTargets[downIndex].height();

			if ( !m_upTargets[static_cast< size_t >(i)].create(renderer, upW, upH, format, "BloomUp" + std::to_string(i)) )
			{
				TraceError{TracerTag} << "Failed to create upsample target " << i << " !";

				return false;
			}
		}

		/* Create output target at full resolution. */
		if ( !m_outputTarget.create(renderer, width, height, format, "BloomOutput") )
		{
			TraceError{TracerTag} << "Failed to create bloom output target !";

			return false;
		}

		/* Create pipelines. */
		if ( !this->createPipelines(renderer) )
		{
			TraceError{TracerTag} << "Failed to create bloom pipelines !";

			return false;
		}

		/* Create descriptor sets. */
		if ( !this->createDescriptorSets(renderer) )
		{
			TraceError{TracerTag} << "Failed to create bloom descriptor sets !";

			return false;
		}

		m_renderer = &renderer;

		return true;
	}

	void
	Bloom::destroy () noexcept
	{
		m_compositePerFrame.clear();
		m_downFirstPerFrame.clear();
		m_compositeDescSet.reset();

		for ( auto & descSet : m_upDescSets )
		{
			descSet.reset();
		}

		for ( auto & descSet : m_downDescSets )
		{
			descSet.reset();
		}

		m_compositePipeline.reset();
		m_upsamplePipeline.reset();
		m_downsamplePipeline.reset();
		m_compositeLayout.reset();
		m_upsampleLayout.reset();
		m_downsampleLayout.reset();

		m_outputTarget.destroy();

		for ( auto & target : m_upTargets )
		{
			target.destroy();
		}

		for ( auto & target : m_downTargets )
		{
			target.destroy();
		}
	}

	bool
	Bloom::resize (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		this->destroy();

		return this->create(renderer, width, height);
	}

	/* ---- Fullscreen Pass Helper ---- */

	void
	Bloom::recordFullscreenPass (
		const CommandBuffer & commandBuffer,
		IntermediateRenderTarget & target,
		const GraphicsPipeline & pipeline,
		const PipelineLayout & pipelineLayout,
		const DescriptorSet & descriptorSet,
		const BloomPushConstants & pc
	) const noexcept
	{
		target.beginRenderPass(commandBuffer);

		commandBuffer.bind(pipeline);

		/* Dynamic viewport and scissor. */
		const VkViewport viewport{
			.x = 0.0F,
			.y = 0.0F,
			.width = static_cast< float >(target.width()),
			.height = static_cast< float >(target.height()),
			.minDepth = 0.0F,
			.maxDepth = 1.0F
		};
		vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

		const VkRect2D scissor{
			.offset = {0, 0},
			.extent = {target.width(), target.height()}
		};
		vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

		/* Push constants. */
		vkCmdPushConstants(
			commandBuffer.handle(),
			pipelineLayout.handle(),
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(BloomPushConstants),
			&pc
		);

		/* Bind descriptor set. */
		commandBuffer.bind(descriptorSet, pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

		/* Draw fullscreen triangle. */
		commandBuffer.draw(3, 1);

		target.endRenderPass(commandBuffer);
	}

	/* ---- Execute ---- */

	const TextureInterface &
	Bloom::execute (
		const CommandBuffer & commandBuffer,
		const TextureInterface & inputColor,
		[[maybe_unused]] const TextureInterface * inputDepth,
		[[maybe_unused]] const TextureInterface * inputNormals,
		[[maybe_unused]] const PostProcessor::PushConstants & constants
	) noexcept
	{
		/* Update the per-frame descriptor sets to point to the external input.
		 * Each frame-in-flight has its own copy to avoid updating a descriptor
		 * set still referenced by a pending command buffer. */
		const auto frameIndex = m_renderer->currentFrameIndex();

		static_cast< void >(m_downFirstPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));
		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* ---- Downsample Chain ---- */
		for ( int i = 0; i < MipLevels; ++i )
		{
			const auto idx = static_cast< size_t >(i);

			/* Texel size of the INPUT texture for this pass. */
			float inputW, inputH;

			if ( i == 0 )
			{
				/* First pass reads from the external input. */
				inputW = static_cast< float >(m_downTargets[0].width() * 2);
				inputH = static_cast< float >(m_downTargets[0].height() * 2);
			}
			else
			{
				inputW = static_cast< float >(m_downTargets[idx - 1].width());
				inputH = static_cast< float >(m_downTargets[idx - 1].height());
			}

			const BloomPushConstants pc{
				.texelSizeX = 1.0F / inputW,
				.texelSizeY = 1.0F / inputH,
				.threshold = (i == 0) ? m_parameters.threshold : 0.0F,
				.softKnee = m_parameters.softKnee,
				.intensity = m_parameters.intensity,
				.spread = m_parameters.spread
			};

			/* For the first pass (i==0), use the per-frame descriptor set
			 * since it's updated every frame with the external input. */
			const auto & descSet = (i == 0) ? *m_downFirstPerFrame[frameIndex] : *m_downDescSets[idx];

			this->recordFullscreenPass(
				commandBuffer,
				m_downTargets[idx],
				*m_downsamplePipeline,
				*m_downsampleLayout,
				descSet,
				pc
			);
		}

		/* ---- Upsample Chain ---- */
		for ( int i = 0; i < MipLevels - 1; ++i )
		{
			const auto idx = static_cast< size_t >(i);

			/* Texel size of the OUTPUT target for this pass. */
			const auto outW = static_cast< float >(m_upTargets[idx].width());
			const auto outH = static_cast< float >(m_upTargets[idx].height());

			const BloomPushConstants pc{
				.texelSizeX = 1.0F / outW,
				.texelSizeY = 1.0F / outH,
				.threshold = 0.0F,
				.softKnee = 0.0F,
				.intensity = m_parameters.intensity,
				.spread = m_parameters.spread
			};

			this->recordFullscreenPass(
				commandBuffer,
				m_upTargets[idx],
				*m_upsamplePipeline,
				*m_upsampleLayout,
				*m_upDescSets[idx],
				pc
			);
		}

		/* ---- Composite ---- */
		{
			const auto outW = static_cast< float >(m_outputTarget.width());
			const auto outH = static_cast< float >(m_outputTarget.height());

			const BloomPushConstants pc{
				.texelSizeX = 1.0F / outW,
				.texelSizeY = 1.0F / outH,
				.threshold = 0.0F,
				.softKnee = 0.0F,
				.intensity = m_parameters.intensity,
				.spread = m_parameters.spread
			};

			this->recordFullscreenPass(
				commandBuffer,
				m_outputTarget,
				*m_compositePipeline,
				*m_compositeLayout,
				*m_compositePerFrame[frameIndex],
				pc
			);
		}

		return m_outputTarget;
	}
}
