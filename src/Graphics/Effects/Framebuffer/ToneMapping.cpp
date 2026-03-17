/*
 * src/Graphics/Effects/Framebuffer/ToneMapping.cpp
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

#include "ToneMapping.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
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

namespace
{
	using namespace EmEn;

	/* ---- Internal push constant structures ---- */

	struct LuminancePushConstants
	{
		float texelSizeX;
		float texelSizeY;
		float padding0;
		float padding1;
	};

	struct AdaptationPushConstants
	{
		float deltaTime;
		float adaptSpeedUp;
		float adaptSpeedDown;
		float padding;
	};

	struct AutoExposurePushConstants
	{
		float exposure;
		float gamma;
		uint32_t tonemapOperator;
		float keyValue;
		float minExposure;
		float maxExposure;
		float padding0;
		float padding1;
	};

	/* ---- GLSL Shader Sources ---- */

	/* Extracts log-luminance from HDR input, averaging 4 bilinear taps. */
	static constexpr auto LuminanceExtractFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float padding0;
	float padding1;
};

void main()
{
	vec2 ts = vec2(texelSizeX, texelSizeY);

	/* Sample 4 texels in a 2x2 pattern; bilinear filtering gives a free 4-tap average. */
	vec3 s0 = texture(inputTex, vUV + ts * vec2(-0.25, -0.25)).rgb;
	vec3 s1 = texture(inputTex, vUV + ts * vec2( 0.25, -0.25)).rgb;
	vec3 s2 = texture(inputTex, vUV + ts * vec2(-0.25,  0.25)).rgb;
	vec3 s3 = texture(inputTex, vUV + ts * vec2( 0.25,  0.25)).rgb;

	/* Convert to luminance (Rec. 709 coefficients). */
	const vec3 w = vec3(0.2126, 0.7152, 0.0722);
	float l0 = dot(s0, w);
	float l1 = dot(s1, w);
	float l2 = dot(s2, w);
	float l3 = dot(s3, w);

	/* Work in log-space for better HDR averaging. */
	const float epsilon = 0.0001;
	float logLum = (log(max(l0, epsilon)) + log(max(l1, epsilon))
	             +  log(max(l2, epsilon)) + log(max(l3, epsilon))) * 0.25;

	outColor = vec4(logLum, 0.0, 0.0, 1.0);
}
)GLSL";

	/* Progressive 4x4 bilinear downsample of log-luminance.
	 * Uses 4 bilinear taps placed at the centers of 4 quadrants within a 4x4 texel area.
	 * Each bilinear tap averages a 2x2 region, giving 16 texels total per output pixel.
	 * This achieves a 4x reduction per pass instead of 2x, halving the number of passes. */
	static constexpr auto LuminanceDownsampleFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float padding0;
	float padding1;
};

void main()
{
	vec2 ts = vec2(texelSizeX, texelSizeY);

	/* 4 bilinear taps at the centers of each 2x2 quadrant within a 4x4 texel area.
	 * Offsets are ±0.75 texels from center, landing on the boundary between texels
	 * in each 2x2 block so bilinear filtering averages 4 texels per tap. */
	float s0 = texture(inputTex, vUV + ts * vec2(-0.75, -0.75)).r;
	float s1 = texture(inputTex, vUV + ts * vec2( 0.75, -0.75)).r;
	float s2 = texture(inputTex, vUV + ts * vec2(-0.75,  0.75)).r;
	float s3 = texture(inputTex, vUV + ts * vec2( 0.75,  0.75)).r;

	outColor = vec4((s0 + s1 + s2 + s3) * 0.25, 0.0, 0.0, 1.0);
}
)GLSL";

	/* Temporal adaptation via asymmetric EMA.
	 * Algorithm: standard eye-adaptation used in Frostbite, UE4, CryEngine.
	 * Reference: Lagarde & de Rousiers, "Moving Frostbite to PBR", SIGGRAPH 2014. */
	static constexpr auto AdaptationFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D currentLumTex;
layout(set = 0, binding = 1) uniform sampler2D prevAdaptedTex;

layout(push_constant) uniform PushConstants
{
	float deltaTime;
	float adaptSpeedUp;
	float adaptSpeedDown;
	float padding;
};

void main()
{
	float currentLogLum = texture(currentLumTex, vec2(0.5)).r;
	float prevAdapted = texture(prevAdaptedTex, vec2(0.5)).r;

	/* Asymmetric speed: adapt faster when the scene brightens (pupil constricts),
	 * slower when it darkens (pupil dilates) — matching human eye behavior. */
	float speed = (currentLogLum > prevAdapted) ? adaptSpeedDown : adaptSpeedUp;
	float adapted = prevAdapted + (currentLogLum - prevAdapted) * (1.0 - exp(-deltaTime * speed));

	outColor = vec4(adapted, 0.0, 0.0, 1.0);
}
)GLSL";

	/* Standard tone mapping without auto-exposure (unchanged from original). */
	static constexpr auto ToneMappingFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTex;

layout(push_constant) uniform PushConstants
{
	float exposure;
	float gamma;
	uint tonemapOperator;
	float padding;
};

/* ACES Filmic approximation (Stephen Hill). */
vec3 acesFilmic (vec3 x)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

/* Reinhard tone mapping. */
vec3 reinhard (vec3 x)
{
	return x / (1.0 + x);
}

/* Uncharted 2 tone mapping (John Hable). */
vec3 uncharted2Tonemap (vec3 x)
{
	const float A = 0.15;
	const float B = 0.50;
	const float C = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 uncharted2 (vec3 color)
{
	const float W = 11.2;
	vec3 curr = uncharted2Tonemap(color);
	vec3 whiteScale = vec3(1.0) / uncharted2Tonemap(vec3(W));
	return curr * whiteScale;
}

void main()
{
	vec3 hdrColor = texture(inputTex, vUV).rgb;

	/* Apply exposure. */
	hdrColor *= exposure;

	/* Apply selected tone mapping operator. */
	vec3 mapped;
	if (tonemapOperator == 0u)
	{
		mapped = acesFilmic(hdrColor);
	}
	else if (tonemapOperator == 1u)
	{
		mapped = reinhard(hdrColor);
	}
	else
	{
		mapped = uncharted2(hdrColor);
	}

	/* Gamma correction. */
	mapped = pow(mapped, vec3(1.0 / gamma));

	outColor = vec4(mapped, 1.0);
}
)GLSL";

	/* Tone mapping with auto-exposure: reads adapted luminance from a 1x1 texture. */
	static constexpr auto AutoExposureToneMappingFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTex;
layout(set = 0, binding = 1) uniform sampler2D adaptedLumTex;

layout(push_constant) uniform PushConstants
{
	float exposure;
	float gamma;
	uint tonemapOperator;
	float keyValue;
	float minExposure;
	float maxExposure;
	float padding0;
	float padding1;
};

/* ACES Filmic approximation (Stephen Hill). */
vec3 acesFilmic (vec3 x)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

/* Reinhard tone mapping. */
vec3 reinhard (vec3 x)
{
	return x / (1.0 + x);
}

/* Uncharted 2 tone mapping (John Hable). */
vec3 uncharted2Tonemap (vec3 x)
{
	const float A = 0.15;
	const float B = 0.50;
	const float C = 0.10;
	const float D = 0.20;
	const float E = 0.02;
	const float F = 0.30;
	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 uncharted2 (vec3 color)
{
	const float W = 11.2;
	vec3 curr = uncharted2Tonemap(color);
	vec3 whiteScale = vec3(1.0) / uncharted2Tonemap(vec3(W));
	return curr * whiteScale;
}

void main()
{
	vec3 hdrColor = texture(inputTex, vUV).rgb;

	/* Compute auto-exposure from adapted log-luminance. */
	float adaptedLogLum = texture(adaptedLumTex, vec2(0.5)).r;
	float avgLuminance = exp(adaptedLogLum);
	float autoExposure = keyValue / max(avgLuminance, 0.001);
	autoExposure = clamp(autoExposure, minExposure, maxExposure);

	/* Apply combined manual + auto exposure. */
	hdrColor *= exposure * autoExposure;

	/* Apply selected tone mapping operator. */
	vec3 mapped;
	if (tonemapOperator == 0u)
	{
		mapped = acesFilmic(hdrColor);
	}
	else if (tonemapOperator == 1u)
	{
		mapped = reinhard(hdrColor);
	}
	else
	{
		mapped = uncharted2(hdrColor);
	}

	/* Gamma correction. */
	mapped = pow(mapped, vec3(1.0 / gamma));

	outColor = vec4(mapped, 1.0);
}
)GLSL";

}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Vulkan;

	/* ---- Pipeline Creation ---- */

	bool
	ToneMapping::createPipelines (Renderer & renderer) noexcept
	{
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		/* Get the shared fullscreen vertex shader from the base class. */
		auto vertexModule = getFullscreenVertexShader(renderer);

		if ( vertexModule == nullptr )
		{
			TraceError{ClassId} << "Failed to compile tone mapping vertex shader !";

			return false;
		}

		/* Compile fragment shaders. */
		auto tmFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "ToneMappingFS", Saphir::ShaderType::FragmentShader, ToneMappingFragmentShader
		);

		auto lumExtractFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "LumExtractFS", Saphir::ShaderType::FragmentShader, LuminanceExtractFragmentShader
		);

		auto lumDownsampleFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "LumDownsampleFS", Saphir::ShaderType::FragmentShader, LuminanceDownsampleFragmentShader
		);

		auto adaptFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "AdaptationFS", Saphir::ShaderType::FragmentShader, AdaptationFragmentShader
		);

		auto autoExpFragment = shaderManager.getShaderModuleFromSourceCode(
			device, "AutoExpToneMappingFS", Saphir::ShaderType::FragmentShader, AutoExposureToneMappingFragmentShader
		);

		if ( tmFragment == nullptr || lumExtractFragment == nullptr ||
			 lumDownsampleFragment == nullptr || adaptFragment == nullptr ||
			 autoExpFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile tone mapping shaders !";

			return false;
		}

		/* Descriptor set layouts. */
		auto singleInputLayout = getInputLayout(renderer, 1);
		auto dualInputLayout = getInputLayout(renderer, 2);

		if ( singleInputLayout == nullptr || dualInputLayout == nullptr )
		{
			return false;
		}

		/* Push constant ranges. */
		const Libs::StaticVector< VkPushConstantRange, 4 > pcRange16{
			VkPushConstantRange{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = 16  /* sizeof(ToneMappingPushConstants) / LuminancePushConstants / AdaptationPushConstants */
			}
		};

		const Libs::StaticVector< VkPushConstantRange, 4 > pcRange32{
			VkPushConstantRange{
				.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = 32  /* sizeof(AutoExposurePushConstants) */
			}
		};

		/* Pipeline layouts. */
		auto & layoutManager = renderer.layoutManager();

		{
			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(singleInputLayout);

			m_pipelineLayout = layoutManager.getPipelineLayout(sets, pcRange16);
		}
		{
			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(dualInputLayout);

			m_adaptPipelineLayout = layoutManager.getPipelineLayout(sets, pcRange16);
		}
		{
			Libs::StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > sets;
			sets.emplace_back(dualInputLayout);

			m_autoExpPipelineLayout = layoutManager.getPipelineLayout(sets, pcRange32);
		}

		if ( m_pipelineLayout == nullptr || m_adaptPipelineLayout == nullptr || m_autoExpPipelineLayout == nullptr )
		{
			return false;
		}

		/* Create pipelines. */
		m_pipeline = IndirectPostProcessEffect::createFullscreenPipeline(
			renderer, ClassId, "ToneMapping", vertexModule, tmFragment, m_pipelineLayout, m_outputTarget
		);

		m_lumExtractPipeline = IndirectPostProcessEffect::createFullscreenPipeline(
			renderer, ClassId, "LumExtract", vertexModule, lumExtractFragment, m_pipelineLayout, *m_lumTargets[0]
		);

		m_lumDownsamplePipeline = IndirectPostProcessEffect::createFullscreenPipeline(
			renderer, ClassId, "LumDownsample", vertexModule, lumDownsampleFragment, m_pipelineLayout, *m_lumTargets[0]
		);

		m_adaptPipeline = IndirectPostProcessEffect::createFullscreenPipeline(
			renderer, ClassId, "Adaptation", vertexModule, adaptFragment, m_adaptPipelineLayout, m_adaptTargets[0]
		);

		m_autoExposurePipeline = IndirectPostProcessEffect::createFullscreenPipeline(
			renderer, ClassId, "AutoExpToneMapping", vertexModule, autoExpFragment, m_autoExpPipelineLayout, m_outputTarget
		);

		return m_pipeline != nullptr
			&& m_lumExtractPipeline != nullptr
			&& m_lumDownsamplePipeline != nullptr
			&& m_adaptPipeline != nullptr
			&& m_autoExposurePipeline != nullptr;
	}

	/* ---- Descriptor Set Creation ---- */

	bool
	ToneMapping::createDescriptorSets (Renderer & renderer) noexcept
	{
		const auto singleInputLayout = getInputLayout(renderer, 1);
		const auto dualInputLayout = getInputLayout(renderer, 2);

		if ( singleInputLayout == nullptr || dualInputLayout == nullptr )
		{
			return false;
		}

		/* Per-frame single-input descriptor sets.
		 * Used for non-auto-exposure tone mapping AND luminance extraction.
		 * Updated each frame to point to the HDR input. */
		m_descriptorSets = createPerFrameDescriptorSets(renderer, singleInputLayout, ClassId, "ToneMappingDescSet");

		if ( m_descriptorSets.empty() )
		{
			return false;
		}

		/* Luminance downsample chain descriptor sets (fixed, single input).
		 * m_lumDownDescSets[i] reads from m_lumTargets[i] (input for the pass
		 * writing to m_lumTargets[i+1]).
		 * NOTE: These are fixed (not per-frame) because they bind internal chain
		 * targets that do not change between frames. */
		m_lumDownDescSets.clear();

		if ( m_lumTargets.size() > 1 )
		{
			const auto & pool = renderer.descriptorPool();

			m_lumDownDescSets.reserve(m_lumTargets.size() - 1);

			for ( size_t i = 0; i < m_lumTargets.size() - 1; ++i )
			{
				auto ds = std::make_unique< DescriptorSet >(pool, singleInputLayout);
				ds->setIdentifier(ClassId, "LumDownDescSet" + std::to_string(i), "DescriptorSet");

				if ( !ds->create() )
				{
					return false;
				}

				if ( !ds->writeCombinedImageSampler(0, *m_lumTargets[i]) )
				{
					return false;
				}

				m_lumDownDescSets.emplace_back(std::move(ds));
			}
		}

		/* Per-frame adaptation descriptor sets (dual input, updated each frame for ping-pong).
		 * Binding 0 = current 1x1 luminance, binding 1 = previous adapted luminance. */
		m_adaptPerFrame = createPerFrameDescriptorSets(renderer, dualInputLayout, ClassId, "AdaptDescSet");

		if ( m_adaptPerFrame.empty() )
		{
			return false;
		}

		/* Per-frame auto-exposure tone mapping descriptor sets (dual input).
		 * Binding 0 = HDR input, binding 1 = adapted luminance. */
		m_autoExpDescPerFrame = createPerFrameDescriptorSets(renderer, dualInputLayout, ClassId, "AutoExpDescSet");

		if ( m_autoExpDescPerFrame.empty() )
		{
			return false;
		}

		return true;
	}

	/* ---- Lifecycle ---- */

	bool
	ToneMapping::create (Renderer & renderer, uint32_t width, uint32_t height) noexcept
	{
		constexpr auto lumFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

		/* Output is LDR (8-bit per channel). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R8G8B8A8_UNORM, "ToneMappingOutput") )
		{
			TraceError{ClassId} << "Failed to create tone mapping output target !";

			return false;
		}

		/* Create luminance downsample chain (quarter-res steps -> 1x1).
		 * First level is half-res (from the extract pass which does 2x downsample).
		 * Subsequent levels use 4x downsample (shader samples 4x4 texel area),
		 * halving the number of downsample passes compared to 2x per step. */
		{
			auto lumW = width / 2;
			auto lumH = height / 2;

			if ( lumW == 0 ) { lumW = 1; }
			if ( lumH == 0 ) { lumH = 1; }

			while ( true )
			{
				auto target = std::make_unique< IntermediateRenderTarget >();

				if ( !target->create(renderer, lumW, lumH, lumFormat,
					"LumTarget" + std::to_string(m_lumTargets.size())) )
				{
					TraceError{ClassId} << "Failed to create luminance target (" << lumW << "x" << lumH << ") !";

					return false;
				}

				m_lumTargets.emplace_back(std::move(target));

				if ( lumW == 1 && lumH == 1 )
				{
					break;
				}

				lumW = std::max(lumW / 4, 1U);
				lumH = std::max(lumH / 4, 1U);
			}
		}

		/* Create adaptation ping-pong targets (2x 1x1). */
		for ( uint32_t i = 0; i < 2; ++i )
		{
			if ( !m_adaptTargets[i].create(renderer, 1, 1, lumFormat, "AdaptLum" + std::to_string(i)) )
			{
				TraceError{ClassId} << "Failed to create adaptation target " << i << " !";

				return false;
			}
		}

		m_currentAdaptIndex = 0;

		/* Create pipelines. */
		if ( !this->createPipelines(renderer) )
		{
			TraceError{ClassId} << "Failed to create tone mapping pipelines !";

			return false;
		}

		/* Create descriptor sets. */
		if ( !this->createDescriptorSets(renderer) )
		{
			TraceError{ClassId} << "Failed to create tone mapping descriptor sets !";

			return false;
		}

		m_renderer = &renderer;
		m_firstFrame = true;
		m_previousTime = 0.0F;

		TraceSuccess{ClassId} << "ToneMapping effect created (" << width << "x" << height
			<< ", " << m_lumTargets.size() << " luminance levels).";

		return true;
	}

	void
	ToneMapping::destroy () noexcept
	{
		/* Auto-exposure descriptor sets. */
		m_autoExpDescPerFrame.clear();
		m_adaptPerFrame.clear();
		m_lumDownDescSets.clear();

		/* Standard descriptor sets. */
		m_descriptorSets.clear();

		/* Auto-exposure pipelines. */
		m_autoExposurePipeline.reset();
		m_adaptPipeline.reset();
		m_lumDownsamplePipeline.reset();
		m_lumExtractPipeline.reset();

		/* Standard pipeline. */
		m_pipeline.reset();

		/* Auto-exposure pipeline layouts. */
		m_autoExpPipelineLayout.reset();
		m_adaptPipelineLayout.reset();

		/* Standard pipeline layout. */
		m_pipelineLayout.reset();

		/* Adaptation targets. */
		for ( auto & target : m_adaptTargets )
		{
			target.destroy();
		}

		/* Luminance chain targets. */
		for ( auto & target : m_lumTargets )
		{
			target->destroy();
		}

		m_lumTargets.clear();

		/* Output target. */
		m_outputTarget.destroy();

		m_renderer = nullptr;
		m_firstFrame = true;
		m_previousTime = 0.0F;
		m_currentAdaptIndex = 0;
	}

	/* ---- Execute ---- */

	const TextureInterface &
	ToneMapping::execute (
		const CommandBuffer & commandBuffer,
		const TextureInterface & inputColor,
		[[maybe_unused]] const TextureInterface * inputDepth,
		[[maybe_unused]] const TextureInterface * inputNormals,
		[[maybe_unused]] const TextureInterface * inputMaterialProperties,
		const PostProcessor::PushConstants & constants
	) noexcept
	{
		const auto frameIndex = m_renderer->currentFrameIndex();

		/* Update the per-frame descriptor set for the HDR input. */
		static_cast< void >(m_descriptorSets[frameIndex]->writeCombinedImageSampler(0, inputColor));

		if ( m_parameters.autoExposureEnabled && !m_lumTargets.empty() )
		{
			/* ---- Compute deltaTime ---- */
			float deltaTime;

			if ( m_firstFrame )
			{
				/* First frame: snap adaptation immediately to current luminance. */
				deltaTime = 100.0F;
				m_firstFrame = false;
			}
			else
			{
				deltaTime = constants.time - m_previousTime;
				deltaTime = std::clamp(deltaTime, 0.001F, 0.5F);
			}

			m_previousTime = constants.time;

			/* ---- Pass 1: Luminance Extraction (HDR -> half-res log-luminance) ---- */
			{
				const auto inputW = static_cast< float >(m_lumTargets[0]->width() * 2);
				const auto inputH = static_cast< float >(m_lumTargets[0]->height() * 2);

				const LuminancePushConstants pc{
					.texelSizeX = 1.0F / inputW,
					.texelSizeY = 1.0F / inputH,
					.padding0 = 0.0F,
					.padding1 = 0.0F
				};

				IndirectPostProcessEffect::recordFullscreenPass(
					commandBuffer,
					*m_lumTargets[0],
					*m_lumExtractPipeline,
					*m_pipelineLayout,
					*m_descriptorSets[frameIndex],
					&pc,
					sizeof(pc)
				);
			}

			/* ---- Passes 2..N: Progressive 4x Downsample (quarter-res steps -> 1x1) ---- */
			for ( size_t i = 1; i < m_lumTargets.size(); ++i )
			{
				const auto inputW = static_cast< float >(m_lumTargets[i - 1]->width());
				const auto inputH = static_cast< float >(m_lumTargets[i - 1]->height());

				const LuminancePushConstants pc{
					.texelSizeX = 1.0F / inputW,
					.texelSizeY = 1.0F / inputH,
					.padding0 = 0.0F,
					.padding1 = 0.0F
				};

				IndirectPostProcessEffect::recordFullscreenPass(
					commandBuffer,
					*m_lumTargets[i],
					*m_lumDownsamplePipeline,
					*m_pipelineLayout,
					*m_lumDownDescSets[i - 1],
					&pc,
					sizeof(pc)
				);
			}

			/* ---- Adaptation Pass (1x1 current + 1x1 previous -> 1x1 adapted) ---- */
			{
				const auto prevAdaptIdx = 1 - m_currentAdaptIndex;

				/* Update the adaptation descriptor set with current ping-pong targets. */
				static_cast< void >(m_adaptPerFrame[frameIndex]->writeCombinedImageSampler(0, *m_lumTargets.back()));
				static_cast< void >(m_adaptPerFrame[frameIndex]->writeCombinedImageSampler(1, m_adaptTargets[prevAdaptIdx]));

				const AdaptationPushConstants pc{
					.deltaTime = deltaTime,
					.adaptSpeedUp = m_parameters.adaptSpeedUp,
					.adaptSpeedDown = m_parameters.adaptSpeedDown,
					.padding = 0.0F
				};

				IndirectPostProcessEffect::recordFullscreenPass(
					commandBuffer,
					m_adaptTargets[m_currentAdaptIndex],
					*m_adaptPipeline,
					*m_adaptPipelineLayout,
					*m_adaptPerFrame[frameIndex],
					&pc,
					sizeof(pc)
				);
			}

			/* ---- Auto-Exposure Tone Mapping ---- */
			{
				/* Update descriptor set: binding 0 = HDR input, binding 1 = adapted luminance. */
				static_cast< void >(m_autoExpDescPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));
				static_cast< void >(m_autoExpDescPerFrame[frameIndex]->writeCombinedImageSampler(1, m_adaptTargets[m_currentAdaptIndex]));

				const AutoExposurePushConstants pc{
					.exposure = m_parameters.exposure,
					.gamma = m_parameters.gamma,
					.tonemapOperator = static_cast< uint32_t >(m_parameters.tonemapOperator),
					.keyValue = m_parameters.keyValue,
					.minExposure = m_parameters.minExposure,
					.maxExposure = m_parameters.maxExposure,
					.padding0 = 0.0F,
					.padding1 = 0.0F
				};

				IndirectPostProcessEffect::recordFullscreenPass(
					commandBuffer,
					m_outputTarget,
					*m_autoExposurePipeline,
					*m_autoExpPipelineLayout,
					*m_autoExpDescPerFrame[frameIndex],
					&pc,
					sizeof(pc)
				);
			}

			/* Flip ping-pong for the next frame. */
			m_currentAdaptIndex = 1 - m_currentAdaptIndex;
		}
		else
		{
			/* ---- Standard Tone Mapping (fixed exposure) ---- */
			m_previousTime = constants.time;

			const ToneMappingPushConstants pc{
				.exposure = m_parameters.exposure,
				.gamma = m_parameters.gamma,
				.tonemapOperator = static_cast< uint32_t >(m_parameters.tonemapOperator),
				.padding = 0.0F
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
		}

		return m_outputTarget;
	}
}
