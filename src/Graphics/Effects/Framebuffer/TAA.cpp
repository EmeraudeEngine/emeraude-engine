/*
 * src/Graphics/Effects/Framebuffer/TAA.cpp
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

#include "TAA.hpp"

/* STL inclusions. */
#include <string>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "PrimaryServices.hpp"
#include "Saphir/ShaderManager.hpp"
#include "SettingKeys.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

static constexpr auto TracerTag{"TAAEffect"};

namespace
{
	using namespace EmEn;

	/* ---- Push Constants ---- */

	struct TAAPushConstants
	{
		float texelSizeX;
		float texelSizeY;
		float alpha;
		float varianceGamma;
		float lumaWeighting;
		/* Current-frame jitter in UV units (NDC * 0.5). The resolve converts it to texels and
		 * folds it into the sub-pixel distance of each reconstruction tap: the scene was
		 * rasterized with that offset, so the source must be reconstructed back at pixel
		 * centers. Without it the current sample and the neighborhood statistics tremble with
		 * the jitter phase and the variance clip drags the history along. */
		float jitterUVX;
		float jitterUVY;
		float padding0;
	};

	static_assert(sizeof(TAAPushConstants) == 32, "TAAPushConstants must be 32 bytes.");

	/* ---- GLSL Shader Sources ----
	 *
	 * Technique credits (state-of-the-art TAA composition):
	 * - Variance clipping in YCoCg space: M. Salvi, "An Excursion in Temporal Supersampling",
	 *   GDC 2016. AABB clipping helper after Playdead's INSIDE TAA (L. Pedersen, GDC 2016).
	 * - History reprojection with 9-tap Catmull-Rom on 5 bilinear fetches: J. Jimenez,
	 *   "Filmic SMAA", SIGGRAPH 2016 (widely redistributed, e.g. MJP's DX12 samples, MIT).
	 * - HDR inverse-luminance blend weighting: B. Karis, "High Quality Temporal
	 *   Supersampling", SIGGRAPH 2014.
	 * - Depth-nearest velocity dilation: same 3x3 closest-depth search as the engine's
	 *   RTGI temporal resolve (shared convention: velocity NDC delta * 0.5 = UV delta).
	 * - Filtered source reconstruction at pixel center: B. Karis, ibid. ("treat the new frame
	 *   as a set of sub-samples and filter over a local neighborhood"), with
	 *   Mitchell-Netravali weights (D. Mitchell & A. Netravali, "Reconstruction Filters in
	 *   Computer Graphics", SIGGRAPH 1988, B = C = 1/3) evaluated at each tap's sub-pixel
	 *   distance -- cf. A. Tardif, "Temporal Antialiasing Starter Pack" (which also flags
	 *   folding the current jitter into that distance) and M. Pettineo's reconstruction-filter
	 *   comparison (Mitchell preferred over Catmull-Rom: less pronounced negative lobes). */

	static constexpr auto TAAResolveFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outResolved;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler2D velocityTex;
layout(set = 0, binding = 3) uniform sampler2D historyTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float alpha;
	float varianceGamma;
	float lumaWeighting;
	float jitterUVX;
	float jitterUVY;
	float padding0;
};

float luminanceOf(vec3 color)
{
	return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 RGBToYCoCg(vec3 c)
{
	return vec3(
		 0.25 * c.r + 0.5 * c.g + 0.25 * c.b,
		 0.5  * c.r             - 0.5  * c.b,
		-0.25 * c.r + 0.5 * c.g - 0.25 * c.b
	);
}

vec3 YCoCgToRGB(vec3 c)
{
	return vec3(
		c.x + c.y - c.z,
		c.x       + c.z,
		c.x - c.y - c.z
	);
}

/* Mitchell-Netravali reconstruction filter, B = C = 1/3 (the paper's recommended
 * compromise between ringing and blur), support radius 2 -- which is why a 3x3 tap set
 * is enough to cover it for any sub-pixel jitter below half a texel. Sharper than a
 * B-spline, milder negative lobes than Catmull-Rom; the caller clamps the result to
 * zero because those lobes can still undershoot on HDR edges. */
float mitchellNetravali(float x)
{
	const float B = 1.0 / 3.0;
	const float C = 1.0 / 3.0;

	float x2 = x * x;
	float x3 = x2 * x;

	if (x < 1.0)
	{
		return ((12.0 - 9.0 * B - 6.0 * C) * x3
		      + (-18.0 + 12.0 * B + 6.0 * C) * x2
		      + (6.0 - 2.0 * B)) / 6.0;
	}

	if (x < 2.0)
	{
		return ((-B - 6.0 * C) * x3
		      + (6.0 * B + 30.0 * C) * x2
		      + (-12.0 * B - 48.0 * C) * x
		      + (8.0 * B + 24.0 * C)) / 6.0;
	}

	return 0.0;
}

/* Clips a point toward the AABB center (Playdead INSIDE): keeps more history
 * energy than a plain clamp when the history sits outside one axis only. */
vec3 clipToAABB(vec3 aabbMin, vec3 aabbMax, vec3 point)
{
	vec3 center = 0.5 * (aabbMax + aabbMin);
	vec3 extents = 0.5 * (aabbMax - aabbMin) + 1e-6;

	vec3 offset = point - center;
	vec3 absUnit = abs(offset / extents);
	float maxUnit = max(absUnit.x, max(absUnit.y, absUnit.z));

	if (maxUnit > 1.0)
	{
		return center + offset / maxUnit;
	}

	return point;
}

/* Sharpness-preserving history fetch: 9-tap Catmull-Rom collapsed onto 5 bilinear
 * fetches (Jimenez, Filmic SMAA). Bilinear history sampling blurs the accumulation
 * to mush within a second of camera motion — the filter choice is load-bearing. */
vec3 sampleHistoryCatmullRom(vec2 uv, vec2 texelSize)
{
	vec2 samplePos = uv / texelSize;
	vec2 texPos1 = floor(samplePos - 0.5) + 0.5;
	vec2 f = samplePos - texPos1;

	vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
	vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
	vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
	vec2 w3 = f * f * (-0.5 + 0.5 * f);

	vec2 w12 = w1 + w2;
	vec2 offset12 = w2 / w12;

	vec2 texPos0 = (texPos1 - 1.0) * texelSize;
	vec2 texPos3 = (texPos1 + 2.0) * texelSize;
	vec2 texPos12 = (texPos1 + offset12) * texelSize;

	vec3 result =
		texture(historyTex, vec2(texPos0.x,  texPos0.y)).rgb  * (w0.x  * w0.y) +
		texture(historyTex, vec2(texPos12.x, texPos0.y)).rgb  * (w12.x * w0.y) +
		texture(historyTex, vec2(texPos3.x,  texPos0.y)).rgb  * (w3.x  * w0.y) +
		texture(historyTex, vec2(texPos0.x,  texPos12.y)).rgb * (w0.x  * w12.y) +
		texture(historyTex, vec2(texPos12.x, texPos12.y)).rgb * (w12.x * w12.y) +
		texture(historyTex, vec2(texPos3.x,  texPos12.y)).rgb * (w3.x  * w12.y) +
		texture(historyTex, vec2(texPos0.x,  texPos3.y)).rgb  * (w0.x  * w3.y) +
		texture(historyTex, vec2(texPos12.x, texPos3.y)).rgb  * (w12.x * w3.y) +
		texture(historyTex, vec2(texPos3.x,  texPos3.y)).rgb  * (w3.x  * w3.y);

	/* The Catmull-Rom negative lobes can undershoot below zero on HDR edges. */
	return max(result, vec3(0.0));
}

void main()
{
	vec2 texel = vec2(texelSizeX, texelSizeY);

	/* SOURCE RECONSTRUCTION (Karis): the scene and its G-buffer were rasterized with a
	 * sub-pixel projection offset, so the source is a set of sub-samples, NOT an image
	 * sampled at pixel centers. Reconstruct the pixel-center value by filtering the 3x3
	 * neighborhood: the tap at texel offset (x, y) carries the scene value belonging at
	 * (x, y) - jitter, hence that is the distance to feed the reconstruction filter.
	 *
	 * Taps land exactly on TEXEL CENTERS (vUV, never vUV + jitter): a fractional offset
	 * would make every tap a bilinear blend whose blur depends on the jitter phase, which
	 * is precisely what made the previous single-tap version breathe over the jitter cycle
	 * (measured: a Laplacian-correlated temporal residual, 2.6x the TAA-off baseline).
	 * The same taps feed the variance clip below, so the moments are crisp and
	 * phase-independent too. */
	vec2 jitterTexels = vec2(jitterUVX, jitterUVY) / texel;

	vec3 sourceTotal = vec3(0.0);
	float sourceWeightTotal = 0.0;
	vec3 m1 = vec3(0.0);
	vec3 m2 = vec3(0.0);

	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			vec3 tap = texture(sceneTex, vUV + vec2(x, y) * texel).rgb;

			vec3 tapYCoCg = RGBToYCoCg(tap);
			m1 += tapYCoCg;
			m2 += tapYCoCg * tapYCoCg;

			float weight = mitchellNetravali(length(vec2(x, y) - jitterTexels));

			sourceTotal += tap * weight;
			sourceWeightTotal += weight;
		}
	}

	/* Normalized (the truncated support does not sum to one), clamped because the filter's
	 * negative lobes can undershoot below zero on HDR edges. */
	vec3 current = max(sourceTotal / max(sourceWeightTotal, 1e-6), vec3(0.0));

	/* First frame after (re)creation: the history image is uninitialized. */
	if (alpha >= 1.0)
	{
		outResolved = vec4(current, 1.0);
		return;
	}

	/* Velocity DILATION: use the velocity of the 3x3 neighbour closest to the camera,
	 * so thin foreground silhouettes drag their motion over the background edge pixels
	 * instead of smearing (same convention as the RTGI temporal resolve). */
	vec2 closestOffset = vec2(0.0);
	float closestDepth = texture(depthTex, vUV).r;

	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			vec2 offset = vec2(x, y) * texel;
			float d = texture(depthTex, vUV + offset).r;

			if (d < closestDepth)
			{
				closestDepth = d;
				closestOffset = offset;
			}
		}
	}

	/* Motion vectors are jitter-free by construction (no matrix carries the jitter -- it is
	 * applied to gl_Position by a per-draw push constant). NDC delta -> UV delta.
	 * Sampled at TEXEL CENTERS: bilinear filtering across a depth or velocity
	 * discontinuity invents values that exist on neither surface. */
	vec2 velocity = texture(velocityTex, vUV + closestOffset).rg;
	vec2 prevUV = vUV - velocity * 0.5;

	if (any(lessThan(prevUV, vec2(0.0))) || any(greaterThan(prevUV, vec2(1.0))))
	{
		/* Off-screen reprojection: no usable history. */
		outResolved = vec4(current, 1.0);
		return;
	}

	vec3 history = sampleHistoryCatmullRom(prevUV, texel);

	/* Never let a stray NaN/Inf poison the feedback loop permanently. */
	if (any(isnan(history)) || any(isinf(history)))
	{
		history = current;
	}

	/* History rectification: VARIANCE CLIPPING in YCoCg (Salvi). The 3x3 first and
	 * second moments build a statistical AABB (mu +/- gamma * sigma) that follows the
	 * local signal much tighter than a min/max hull — stale history gets pulled in
	 * (anti-ghosting) without the min/max flicker on high-frequency detail.
	 * The moments come from the reconstruction loop above: same taps, one pass. */
	vec3 mu = m1 / 9.0;
	vec3 sigma = sqrt(max(m2 / 9.0 - mu * mu, vec3(0.0)));

	vec3 clippedYCoCg = clipToAABB(mu - varianceGamma * sigma, mu + varianceGamma * sigma, RGBToYCoCg(history));
	history = max(YCoCgToRGB(clippedYCoCg), vec3(0.0));

	vec3 result;

	if (lumaWeighting > 0.5)
	{
		/* HDR accumulation guard (Karis): inverse-luminance weights tame fireflies —
		 * a single very bright jittered sample no longer flashes the whole pixel. */
		float weightCurrent = alpha / (1.0 + luminanceOf(current));
		float weightHistory = (1.0 - alpha) / (1.0 + luminanceOf(history));

		result = (current * weightCurrent + history * weightHistory) / max(weightCurrent + weightHistory, 1e-6);
	}
	else
	{
		result = mix(history, current, alpha);
	}

	outResolved = vec4(result, 1.0);
}
)GLSL";

}

namespace EmEn::Graphics::Effects::Framebuffer
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	TAA::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		/* User-facing parameters, engine-wide and persisted in the settings file.
		 * These override any constructor-provided values. */
		auto & settings = renderer.primaryServices().settings();
		m_parameters.alpha = settings.getOrSetDefault< float >(GraphicsTAAAlphaKey, DefaultGraphicsTAAAlpha);
		m_parameters.varianceGamma = settings.getOrSetDefault< float >(GraphicsTAAVarianceGammaKey, DefaultGraphicsTAAVarianceGamma);
		m_parameters.lumaWeighting = settings.getOrSetDefault< bool >(GraphicsTAALumaWeightingKey, DefaultGraphicsTAALumaWeighting);

		/* History starts invalid: the first frame after (re)creation must not read the
		 * uninitialized ping-pong images (alpha forced to 1). The default resize()
		 * (destroy + create) resets this naturally on swap-chain recreation. */
		m_historyValid = false;
		m_historyWriteIndex = 0;

		/* Full-resolution HDR ping-pong: the resolve output doubles as the history. */
		for ( size_t index = 0; index < 2; ++index )
		{
			if ( !m_historyTargets[index].create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "TAA_History" + std::to_string(index)) )
			{
				TraceError{TracerTag} << "Failed to create TAA history target #" << index << " !";

				return false;
			}
		}

		/* Compile shaders. */
		const auto vertexModule = this->getFullscreenVertexShader();

		const auto fragmentModule = renderer.shaderManager().getShaderModuleFromSourceCode(renderer.device(), "TAAResolveFS", Saphir::ShaderType::FragmentShader, TAAResolveFragmentShader);

		if ( vertexModule == nullptr || fragmentModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile TAA shaders !";

			return false;
		}

		/* Descriptor set layout: scene color + depth + velocity + history samplers. */
		auto descriptorSetLayout = this->getInputLayout(4);

		if ( descriptorSetLayout == nullptr )
		{
			TraceError{TracerTag} << "Failed to create descriptor set layout !";

			return false;
		}

		/* Pipeline layout. */
		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(descriptorSetLayout);

			m_pipelineLayout = renderer.layoutManager().getPipelineLayout(sets, {
				VkPushConstantRange{
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(TAAPushConstants)
				}
			});
		}

		if ( m_pipelineLayout == nullptr )
		{
			TraceError{TracerTag} << "Failed to create pipeline layout !";

			return false;
		}

		/* Graphics pipeline. NOTE: Created against [0], recorded into either ping-pong
		 * target — Vulkan render pass compatibility (identical format/ops), same trick
		 * as the RTGI temporal pass. */
		m_pipeline = this->createFullscreenPipeline(ClassId, "TAAResolve", vertexModule, fragmentModule, m_pipelineLayout, m_historyTargets[0]);

		if ( m_pipeline == nullptr )
		{
			TraceError{TracerTag} << "Failed to create TAA pipeline !";

			return false;
		}

		/* Per-frame descriptor sets. */
		m_descriptorSets = this->createPerFrameDescriptorSets(descriptorSetLayout, ClassId, "TAADescSet");

		if ( m_descriptorSets.empty() )
		{
			TraceError{TracerTag} << "Failed to create per-frame descriptor sets !";

			return false;
		}

		return true;
	}

	void
	TAA::destroy () noexcept
	{
		m_descriptorSets.clear();
		m_pipeline.reset();
		m_pipelineLayout.reset();

		for ( auto & target : m_historyTargets )
		{
			target.destroy();
		}

		m_historyValid = false;
		m_historyWriteIndex = 0;
	}

	/* ---- Execute ---- */

	const TextureInterface &
	TAA::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();

		const uint32_t writeIdx = m_historyWriteIndex;
		const uint32_t readIdx = 1U - writeIdx;

		/* Update the per-frame descriptor set: current input + history ping-pong. */
		static_cast< void >(m_descriptorSets[frameIndex]->writeCombinedImageSampler(0, inputColor));

		if ( context.depth != nullptr )
		{
			static_cast< void >(m_descriptorSets[frameIndex]->writeCombinedImageSampler(1, *context.depth));
		}

		if ( context.velocity != nullptr )
		{
			static_cast< void >(m_descriptorSets[frameIndex]->writeCombinedImageSampler(2, *context.velocity));
		}

		static_cast< void >(m_descriptorSets[frameIndex]->writeCombinedImageSampler(3, m_historyTargets[readIdx]));

		/* Without velocity there is no valid reprojection: pass the input through
		 * (alpha 1) rather than accumulating a smear. */
		const bool historyUsable = m_historyValid && context.velocity != nullptr && context.depth != nullptr;

		const TAAPushConstants pc{
			.texelSizeX = 1.0F / static_cast< float >(m_historyTargets[writeIdx].width()),
			.texelSizeY = 1.0F / static_cast< float >(m_historyTargets[writeIdx].height()),
			.alpha = historyUsable ? m_parameters.alpha : 1.0F,
			.varianceGamma = m_parameters.varianceGamma,
			.lumaWeighting = m_parameters.lumaWeighting ? 1.0F : 0.0F,
			/* NDC jitter -> UV units (frame-history contract, see the shader note). */
			.jitterUVX = context.projectionJitter.x() * 0.5F,
			.jitterUVY = context.projectionJitter.y() * 0.5F,
			.padding0 = 0.0F
		};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_historyTargets[writeIdx],
			*m_pipeline,
			*m_pipelineLayout,
			*m_descriptorSets[frameIndex],
			&pc,
			sizeof(pc)
		);

		/* Flip the ping-pong: the freshly resolved image becomes next frame's history. */
		m_historyWriteIndex = readIdx;
		m_historyValid = true;

		return m_historyTargets[writeIdx];
	}
}
