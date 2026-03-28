/*
 * src/Graphics/Effects/Framebuffer/Bloom.cpp
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
	/* ---- GLSL Shader Sources ---- */

	static constexpr auto DownsampleFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTex;
layout(set = 0, binding = 1) uniform sampler2D materialPropsTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float threshold;
	float softKnee;
	float intensity;
	float spread;
};

/* Karis average weight: suppresses firefly pixels by weighting
 * each sample inversely proportional to its luminance.
 * Bright outlier pixels get near-zero weight. (Epic Games, UE4) */
float karisWeight (vec3 color)
{
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	return 1.0 / (1.0 + luma);
}

/* 13-tap downsample filter (Jimenez, COD:MW) with Karis anti-firefly.
 * The useAntiFirefly flag enables luminance-weighted averaging on the
 * first downsample pass to prevent single bright pixels from dominating. */
vec4 downsample13Tap (sampler2D tex, vec2 uv, vec2 ts, bool useAntiFirefly)
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

	if ( useAntiFirefly )
	{
		/* Weighted average using Karis: each 2x2 quad is weighted
		 * by the inverse of its luminance, killing fireflies. */
		vec4 g0 = (D + E + I + J);
		vec4 g1 = (A + B + F + G);
		vec4 g2 = (B + C + G + H);
		vec4 g3 = (F + G + K + L);
		vec4 g4 = (G + H + L + M);

		float w0 = karisWeight(g0.rgb * 0.25);
		float w1 = karisWeight(g1.rgb * 0.25);
		float w2 = karisWeight(g2.rgb * 0.25);
		float w3 = karisWeight(g3.rgb * 0.25);
		float w4 = karisWeight(g4.rgb * 0.25);

		float wSum = w0 * 0.5 + (w1 + w2 + w3 + w4) * 0.125;

		vec4 result = vec4(0.0);
		result += g0 * 0.5 * w0;
		result += g1 * 0.125 * w1;
		result += g2 * 0.125 * w2;
		result += g3 * 0.125 * w3;
		result += g4 * 0.125 * w4;

		return result / (wSum * 4.0 + 0.00001);
	}

	/* Standard unweighted downsample for subsequent mip levels. */
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

	/* Anti-firefly (Karis weight) on the first downsample pass only
	 * (when threshold > 0). Subsequent passes use standard averaging. */
	vec4 color = downsample13Tap(inputTex, vUV, ts, threshold > 0.0);

	/* Reject NaN/Inf to prevent contamination of the bloom chain. */
	if (any(isnan(color)) || any(isinf(color)))
	{
		outColor = vec4(0.0);
		return;
	}

	if (threshold > 0.0)
	{
		/* Decode bloom contribution and emissive mask from material properties G-buffer.
		 * B channel: high nibble = bloomContrib, low nibble = emissiveMask. */
		vec4 mp = texture(materialPropsTex, vUV);
		uint bPacked = uint(mp.b * 255.0);
		float bloomContrib = float(bPacked >> 4u) / 15.0;
		float emissiveMask = float(bPacked & 0xFu) / 15.0;

		/* Emissive pixels bloom more easily (lower threshold). */
		float effectiveThreshold = threshold * mix(1.0, 0.1, emissiveMask);

		color.rgb = applyThreshold(color.rgb, effectiveThreshold, softKnee);

		/* Modulate extracted brightness by per-pixel bloom contribution. */
		color.rgb *= bloomContrib;
	}

	/* Clamp after threshold: Karis handles moderate outliers,
	 * this catches extreme values that still slip through. */
	outColor = clamp(color, vec4(0.0), vec4(64.0));
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
}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Libs;
	using namespace Saphir;
	using namespace Vulkan;

	/* ---- Pipeline Creation ---- */

	bool
	Bloom::createPipelines () noexcept
	{
		auto & renderer = this->renderer();
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		/* Get the shared fullscreen vertex shader from the base class. */
		const auto vertexModule = this->getFullscreenVertexShader();

		if ( vertexModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to get fullscreen vertex shader !";

			return false;
		}

		/* Compile the downsample fragment shader. */
		const auto downsampleFragment = shaderManager.getShaderModuleFromSourceCode(device, "BloomDownsampleFS", ShaderType::FragmentShader, DownsampleFragmentShader);

		if ( downsampleFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile bloom downsample shader !";

			return false;
		}

		/* Compile the upsample fragment shader. */
		const auto upsampleFragment = shaderManager.getShaderModuleFromSourceCode(device, "BloomUpsampleFS", ShaderType::FragmentShader, UpsampleFragmentShader);

		if ( upsampleFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile bloom upsample shader !";

			return false;
		}

		/* Compile the composite fragment shader. */
		const auto compositeFragment = shaderManager.getShaderModuleFromSourceCode(device, "BloomCompositeFS", ShaderType::FragmentShader, CompositeFragmentShader);

		if ( compositeFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile bloom composite shader !";

			return false;
		}

		/* Descriptor set layouts (from base class shared utilities). */
		auto dualInputLayout = this->getInputLayout(2);

		if ( dualInputLayout == nullptr )
		{
			return false;
		}

		/* Push constant range (shared by all pipelines). */
		const StaticVector< VkPushConstantRange, 4 > pushConstantRanges{
			VkPushConstantRange{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(BloomPushConstants)
			}
		};

		/* Pipeline layouts. */
		{
			/* Downsample: binding 0 = input texture, binding 1 = material properties. */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(dualInputLayout);

			m_downsampleLayout = renderer.layoutManager().getPipelineLayout(sets, pushConstantRanges);
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(dualInputLayout);

			m_upsampleLayout = renderer.layoutManager().getPipelineLayout(sets, pushConstantRanges);
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(dualInputLayout);

			m_compositeLayout = renderer.layoutManager().getPipelineLayout(sets, pushConstantRanges);
		}

		if ( m_downsampleLayout == nullptr || m_upsampleLayout == nullptr || m_compositeLayout == nullptr )
		{
			return false;
		}

		/* Create the three pipelines using the base class utility. */
		m_downsamplePipeline = this->createFullscreenPipeline(ClassId, "BloomDownsample", vertexModule, downsampleFragment, m_downsampleLayout, m_downTargets[0]);
		m_upsamplePipeline = this->createFullscreenPipeline(ClassId, "BloomUpsample", vertexModule, upsampleFragment, m_upsampleLayout, m_upTargets[0]);
		m_compositePipeline = this->createFullscreenPipeline(ClassId, "BloomComposite", vertexModule, compositeFragment, m_compositeLayout, m_outputTarget);

		return m_downsamplePipeline != nullptr && m_upsamplePipeline != nullptr && m_compositePipeline != nullptr;
	}

	/* ---- Descriptor Set Creation ---- */

	bool
	Bloom::createDescriptorSets () noexcept
	{
		const auto dualInputLayout = this->getInputLayout(2);
		const auto & pool = this->renderer().descriptorPool();

		if ( dualInputLayout == nullptr )
		{
			return false;
		}

		/* Downsample descriptor sets (dual input: binding 0 = input, binding 1 = material properties).
		 * Set [0] is updated per-frame in execute() to point to the input texture,
		 * so we create per-frame-in-flight copies to avoid descriptor update conflicts.
		 * Sets [1..4] are pre-written to the previous downsample target.
		 * Binding 1 (materialPropsTex) is only sampled in the first pass (threshold > 0),
		 * but must be bound for all passes to satisfy the descriptor set layout. */
		for ( uint32_t mipLevel = 0; mipLevel < MipLevels; ++mipLevel )
		{
			m_downDescSets[mipLevel] = std::make_unique< DescriptorSet >(pool, dualInputLayout);
			m_downDescSets[mipLevel]->setIdentifier(ClassId, "DownDescSet" + std::to_string(mipLevel), "DescriptorSet");

			if ( !m_downDescSets[mipLevel]->create() )
			{
				return false;
			}

			if ( mipLevel > 0 )
			{
				const auto & input = m_downTargets[static_cast< size_t >(mipLevel - 1)];

				if ( !m_downDescSets[mipLevel]->writeCombinedImageSampler(0, input) )
				{
					return false;
				}

				/* Binding 1: write the previous downsample target as a dummy for
				 * material properties (not sampled in non-threshold passes). */
				if ( !m_downDescSets[mipLevel]->writeCombinedImageSampler(1, input) )
				{
					return false;
				}
			}
		}

		/* Per-frame copies of m_downDescSets[0] for safe per-frame updates. */
		m_downFirstPerFrame = this->createPerFrameDescriptorSets(dualInputLayout, ClassId, "DownDescSet0");

		if ( m_downFirstPerFrame.empty() )
		{
			return false;
		}

		/* Upsample descriptor sets (dual input each).
		 * Binding 0 = previous upsample output (or bottom mip), binding 1 = downsample at this level. */
		for ( uint32_t mipLevel = 0; mipLevel < MipLevels - 1; ++mipLevel )
		{
			m_upDescSets[mipLevel] = std::make_unique< DescriptorSet >(pool, dualInputLayout);
			m_upDescSets[mipLevel]->setIdentifier(ClassId, "UpDescSet" + std::to_string(mipLevel), "DescriptorSet");

			if ( !m_upDescSets[mipLevel]->create() )
			{
				return false;
			}

			/* Binding 0: previous level (bottom mip for first pass, then upTargets). */
			if ( mipLevel == 0 )
			{
				/* First upsample reads from the smallest downsample target. */
				if ( !m_upDescSets[0]->writeCombinedImageSampler(0, m_downTargets[MipLevels - 1]) )
				{
					return false;
				}
			}
			else
			{
				if ( !m_upDescSets[mipLevel]->writeCombinedImageSampler(0, m_upTargets[static_cast< size_t >(mipLevel - 1)]) )
				{
					return false;
				}
			}

			/* Binding 1: downsample at this level.
			 * Upsample pass i writes to upTargets[i], which has the same resolution as downTargets[MipLevels - 2 - i].
			 * We blend with the downsample at the target resolution. */
			const auto downIndex = static_cast< size_t >(MipLevels - 2 - mipLevel);

			if ( !m_upDescSets[mipLevel]->writeCombinedImageSampler(1, m_downTargets[downIndex]) )
			{
				return false;
			}
		}

		/* Composite descriptor sets (dual input, per-frame-in-flight).
		 * Binding 0 = original input (updated per-frame), binding 1 = final bloom (top upsample). */
		m_compositePerFrame = this->createPerFrameDescriptorSets(dualInputLayout, ClassId, "CompositeDescSet");

		if ( m_compositePerFrame.empty() )
		{
			return false;
		}

		for ( const auto & descriptorSet : m_compositePerFrame )
		{
			/* Binding 1: top upsample result (same for all frames). */
			if ( !descriptorSet->writeCombinedImageSampler(1, m_upTargets[MipLevels - 2]) )
			{
				return false;
			}
		}

		return true;
	}

	/* ---- Lifecycle ---- */

	bool
	Bloom::create (uint32_t width, uint32_t height) noexcept
	{
		constexpr auto format = VK_FORMAT_R16G16B16A16_SFLOAT;

		auto & renderer = this->renderer();

		/* Create downsample targets (halving resolution at each level). */
		auto mipWidth = width / 2;
		auto mipHeight = height / 2;

		for ( uint32_t mipLevel = 0; mipLevel < MipLevels; ++mipLevel )
		{
			if ( mipWidth == 0 )
			{
				mipWidth = 1;
			}
			
			if ( mipHeight == 0 )
			{
				mipHeight = 1;
			}

			if ( !m_downTargets[mipLevel].create(renderer, mipWidth, mipHeight, format, "BloomDown" + std::to_string(mipLevel)) )
			{
				TraceError{TracerTag} << "Failed to create downsample target " << mipLevel << " !";

				return false;
			}

			mipWidth /= 2;
			mipHeight /= 2;
		}

		/* Create upsample targets (matching downsample levels in reverse, excluding the smallest).
		 * upTargets[0] matches downTargets[MipLevels-2] resolution (1/16),
		 * upTargets[MipLevels-2] matches downTargets[0] resolution (1/2). */
		for ( uint32_t mipLevel = 0; mipLevel < MipLevels - 1; ++mipLevel )
		{
			const auto downIndex = static_cast< size_t >(MipLevels - 2 - mipLevel);
			const auto upWidth = m_downTargets[downIndex].width();
			const auto upHeight = m_downTargets[downIndex].height();

			if ( !m_upTargets[mipLevel].create(renderer, upWidth, upHeight, format, "BloomUp" + std::to_string(mipLevel)) )
			{
				TraceError{TracerTag} << "Failed to create upsample target " << mipLevel << " !";

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
		if ( !this->createPipelines() )
		{
			TraceError{TracerTag} << "Failed to create bloom pipelines !";

			return false;
		}

		/* Create descriptor sets. */
		if ( !this->createDescriptorSets() )
		{
			TraceError{TracerTag} << "Failed to create bloom descriptor sets !";

			return false;
		}

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

	/* ---- Execute ---- */

	const TextureInterface &
	Bloom::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, [[maybe_unused]] const TextureInterface * inputDepth, [[maybe_unused]] const TextureInterface * inputNormals, const TextureInterface * inputMaterialProperties, [[maybe_unused]] const Scenes::LightSet * lightSet, [[maybe_unused]] const PostProcessor::PushConstants & constants) noexcept
	{
		/* Update the per-frame descriptor sets to point to the external input.
		 * Each frame-in-flight has its own copy to avoid updating a descriptor
		 * set still referenced by a pending command buffer. */
		const auto frameIndex = this->renderer().currentFrameIndex();

		static_cast< void >(m_downFirstPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Binding 1: material properties G-buffer for bloom contribution/emissive modulation. */
		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_downFirstPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputMaterialProperties));
		}

		static_cast< void >(m_compositePerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* ---- Downsample Chain ---- */
		for ( uint32_t mipLevel = 0; mipLevel < MipLevels; ++mipLevel )
		{
			const auto idx = mipLevel;

			/* Texel size of the INPUT texture for this pass. */
			float inputW, inputH;

			if ( mipLevel == 0 )
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
				.threshold = (mipLevel == 0) ? m_parameters.threshold : 0.0F,
				.softKnee = m_parameters.softKnee,
				.intensity = m_parameters.intensity,
				.spread = m_parameters.spread
			};

			/* For the first pass (i==0), use the per-frame descriptor set
			 * since it's updated every frame with the external input. */
			const auto & descSet = (mipLevel == 0) ? *m_downFirstPerFrame[frameIndex] : *m_downDescSets[idx];

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_downTargets[idx],
				*m_downsamplePipeline,
				*m_downsampleLayout,
				descSet,
				&pc,
				sizeof(BloomPushConstants)
			);
		}

		/* ---- Upsample Chain ---- */
		for ( uint32_t mipLevel = 0; mipLevel < MipLevels - 1; ++mipLevel )
		{
			/* Texel size of the OUTPUT target for this pass. */
			const auto outW = static_cast< float >(m_upTargets[mipLevel].width());
			const auto outH = static_cast< float >(m_upTargets[mipLevel].height());

			const BloomPushConstants pc{
				.texelSizeX = 1.0F / outW,
				.texelSizeY = 1.0F / outH,
				.threshold = 0.0F,
				.softKnee = 0.0F,
				.intensity = m_parameters.intensity,
				.spread = m_parameters.spread
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_upTargets[mipLevel],
				*m_upsamplePipeline,
				*m_upsampleLayout,
				*m_upDescSets[mipLevel],
				&pc,
				sizeof(BloomPushConstants)
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

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_outputTarget,
				*m_compositePipeline,
				*m_compositeLayout,
				*m_compositePerFrame[frameIndex],
				&pc,
				sizeof(BloomPushConstants)
			);
		}

		return m_outputTarget;
	}
}
