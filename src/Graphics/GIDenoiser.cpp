/*
 * src/Graphics/GIDenoiser.cpp
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

#include "GIDenoiser.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"

namespace
{
	/* Temporal resolve pass: exponential moving average between the current noisy GI and
	 * the reprojected history. The history UV is found through the velocity buffer
	 * (per-object motion vectors, NDC delta) with 3x3 depth-nearest dilation. History is
	 * rejected on disocclusion (camera-distance mismatch, normal mismatch) and optionally
	 * rectified by variance clipping against the current 3x3 neighbourhood (anti-ghosting).
	 * Output: RGB = resolved DEMODULATED indirect irradiance (receiver albedo applied at
	 * the combine), A = camera distance (0 = invalid/sky).
	 *
	 * Descriptor set 0:
	 *   binding 0: current noisy GI (the owner's blur output)
	 *   binding 1: depth texture
	 *   binding 2: normals texture (view space)
	 *   binding 3: GI history texture (previous resolved frame)
	 *   binding 4: world-normal history texture (previous frame)
	 *   binding 5: velocity texture (RG16F NDC-delta motion vectors)
	 *   binding 6: frame UBO (shared with the owner's trace pass)
	 */
	constexpr auto GIDenoiserTemporalFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outResolved;

layout(set = 0, binding = 0) uniform sampler2D giTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler2D normalTex;
layout(set = 0, binding = 3) uniform sampler2D historyTex;
layout(set = 0, binding = 4) uniform sampler2D historyNormalTex;
layout(set = 0, binding = 5) uniform sampler2D velocityTex;

layout(set = 0, binding = 6, std140) uniform FrameData
{
	mat4 invViewProj;
	mat4 prevViewProj;
	vec4 invViewCol0;	/* xyz = inverse view rotation column 0, w = camera position X. */
	vec4 invViewCol1;	/* xyz = inverse view rotation column 1, w = camera position Y. */
	vec4 invViewCol2;	/* xyz = inverse view rotation column 2, w = camera position Z. */
	vec4 prevCamPos;	/* xyz = previous frame camera position, w = unused. */
	vec4 traceParams;	/* x = maxDistance, y = bias, z = sampleCount, w = animated-noise frame index (R2). */
	vec4 temporalParams;	/* x = alpha, y = depthTolerance, z = normalThreshold, w = flags (bit0 variance clip, bit1 animated noise). */
	vec4 bounceParams;	/* x = multiBounceStrength, y = multiBounceClamp, z = variance-clip gamma, w = unused. */
};

void main()
{
	float depth = texture(depthTex, vUV).r;

	/* Sky/far-plane: no surface, invalid history marker (a = 0). */
	if (depth >= 1.0)
	{
		outResolved = vec4(0.0);
		return;
	}

	vec3 current = texture(giTex, vUV).rgb;

	/* Reconstruct world-space position from NDC + depth via inverse VP. */
	vec2 ndc = vUV * 2.0 - 1.0;
	vec4 clipPos = vec4(ndc, depth, 1.0);
	vec4 wp = invViewProj * clipPos;
	vec3 worldPos = wp.xyz / wp.w;

	vec3 viewPos = vec3(invViewCol0.w, invViewCol1.w, invViewCol2.w);
	float cameraDistance = length(worldPos - viewPos);

	/* Current world-space normal, for the history normal comparison. */
	mat3 invViewRot = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);
	vec3 worldNormal = normalize(invViewRot * normalize(texture(normalTex, vUV).rgb));

	float alpha = temporalParams.x;

	/* Reproject into the previous frame through the velocity buffer (per-object motion
	 * vectors, NDC delta = current - previous). Velocity DILATION: use the velocity of
	 * the 3x3 neighbour closest to the camera, so thin foreground silhouettes drag their
	 * motion over the background edge pixels instead of smearing. */
	vec2 texelD = 1.0 / vec2(textureSize(depthTex, 0));
	vec2 closestOffset = vec2(0.0);
	float closestDepth = depth;

	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			vec2 offset = vec2(x, y) * texelD;
			float d = texture(depthTex, vUV + offset).r;

			if (d < closestDepth)
			{
				closestDepth = d;
				closestOffset = offset;
			}
		}
	}

	vec2 velocity = texture(velocityTex, vUV + closestOffset).rg;
	vec2 prevUV = vUV - velocity * 0.5;

	if (any(lessThan(prevUV, vec2(0.0))) || any(greaterThan(prevUV, vec2(1.0))))
	{
		/* Off-screen: no history, full weight on the current estimate. */
		outResolved = vec4(current, cameraDistance);
		return;
	}

	vec4 history = texture(historyTex, prevUV);

	/* Disocclusion test 1: camera-distance mismatch (rotation-invariant). */
	float expectedDistance = length(worldPos - prevCamPos.xyz);
	bool distanceValid = history.a > 0.0 && abs(history.a - expectedDistance) <= temporalParams.y * expectedDistance;

	/* Disocclusion test 2: world-normal mismatch (silhouettes, grazing surfaces). */
	vec3 prevNormal = texture(historyNormalTex, prevUV).xyz;
	bool normalValid = dot(prevNormal, worldNormal) >= temporalParams.z;

	if (!distanceValid || !normalValid)
	{
		outResolved = vec4(current, cameraDistance);
		return;
	}

	/* History rectification (flag bit 0): VARIANCE CLIPPING — bound the history to
	 * mean ± gamma * sigma of the current 3x3 neighborhood (M. Salvi, "An Excursion in
	 * Temporal Supersampling", GDC 2016 — the same technique as the engine TAA). It
	 * replaced the min/max clamp: with ANIMATED noise the per-frame estimates are
	 * deliberately different, and a min/max box collapses onto whatever outlier the
	 * current frame produced, killing the convergence the animation exists for. The
	 * statistical bound keeps disocclusion ghosting bounded while letting the EMA
	 * actually accumulate. bounceParams.z = gamma (Temporal/VarianceGamma key). */
	if ((uint(temporalParams.w) & 1u) != 0u)
	{
		vec2 texel = 1.0 / vec2(textureSize(giTex, 0));
		vec3 m1 = vec3(0.0);
		vec3 m2 = vec3(0.0);

		for (int y = -1; y <= 1; y++)
		{
			for (int x = -1; x <= 1; x++)
			{
				vec3 nb = texture(giTex, vUV + vec2(x, y) * texel).rgb;
				m1 += nb;
				m2 += nb * nb;
			}
		}

		vec3 mu = m1 / 9.0;
		vec3 sigma = sqrt(max(m2 / 9.0 - mu * mu, vec3(0.0)));

		history.rgb = clamp(history.rgb, mu - bounceParams.z * sigma, mu + bounceParams.z * sigma);
	}

	outResolved = vec4(mix(history.rgb, current, alpha), cameraDistance);
}
)GLSL";

	/* Normal history pass: converts the current view-space normals G-buffer to world space
	 * (camera-rotation invariant) and stores it at history resolution for the NEXT frame's
	 * temporal validation. The normals MRT attachment is rewritten every frame, so the
	 * previous frame's normals must be explicitly retained.
	 *
	 * Descriptor set 0:
	 *   binding 0: normals texture (view space, current frame)
	 *   binding 1: frame UBO (shared with the owner's trace pass)
	 */
	constexpr auto GIDenoiserNormalCopyFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outWorldNormal;

layout(set = 0, binding = 0) uniform sampler2D normalTex;

layout(set = 0, binding = 1, std140) uniform FrameData
{
	mat4 invViewProj;
	mat4 prevViewProj;
	vec4 invViewCol0;	/* xyz = inverse view rotation column 0, w = camera position X. */
	vec4 invViewCol1;	/* xyz = inverse view rotation column 1, w = camera position Y. */
	vec4 invViewCol2;	/* xyz = inverse view rotation column 2, w = camera position Z. */
	vec4 prevCamPos;	/* xyz = previous frame camera position, w = unused. */
	vec4 traceParams;	/* x = maxDistance, y = bias, z = sampleCount, w = animated-noise frame index (R2). */
	vec4 temporalParams;	/* x = alpha, y = depthTolerance, z = normalThreshold, w = flags (bit0 variance clip, bit1 animated noise). */
	vec4 bounceParams;	/* x = multiBounceStrength, y = multiBounceClamp, z = variance-clip gamma, w = unused. */
};

void main()
{
	vec3 rawN = texture(normalTex, vUV).rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outWorldNormal = vec4(0.0);
		return;
	}

	mat3 invViewRot = mat3(invViewCol0.xyz, invViewCol1.xyz, invViewCol2.xyz);

	outWorldNormal = vec4(normalize(invViewRot * normalize(rawN)), 1.0);
}
)GLSL";
}

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	GIDenoiser::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		/* History starts invalid: the first frame after (re)creation must not read the
		 * uninitialized ping-pong images (alpha forced to 1, no multi-bounce feedback). */
		m_historyValid = false;
		m_historyWriteIndex = 0;
		m_noiseFrameIndex = 0;

		const std::string baseName{m_ownerLabel};

		/* ---- Per-frame UBOs (shared by the owner's trace and the denoiser passes) ----
		 * Allocated even when the temporal chain is off: the owner's trace reads the
		 * frame data (matrices, trace/sky scalars) unconditionally. */
		m_frameUBOs = this->createPerFrameUniformBuffers(sizeof(FrameUBOData), ClassId, baseName + "_Frame_UBO");

		if ( m_frameUBOs.empty() )
		{
			return false;
		}

		if ( !m_temporalEnabled )
		{
			return true;
		}

		/* Temporal history targets (owner resolution, ping-pong). Only allocated when the
		 * temporal accumulation is enabled, so the disabled path costs no VRAM. */
		for ( size_t index = 0; index < 2; ++index )
		{
			const auto suffix = std::to_string(index);

			if ( !m_historyTargets[index].create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, baseName + "_GIHistory" + suffix) )
			{
				TraceError{ClassId} << "Failed to create the GI history target #" << index << " !";

				return false;
			}

			if ( !m_normalHistoryTargets[index].create(renderer, width, height, VK_FORMAT_R16G16B16A16_SFLOAT, baseName + "_GINormalHistory" + suffix) )
			{
				TraceError{ClassId} << "Failed to create the GI normal history target #" << index << " !";

				return false;
			}
		}

		/* ---- Descriptor set layouts ---- */

		/* Temporal resolve input: GI + depth + normals + history + normal history + velocity,
		 * plus the frame UBO. */
		auto temporalInputLayout = this->getInputLayout(6, 1);

		/* Normal history input: normals, plus the frame UBO. */
		auto normalCopyInputLayout = this->getInputLayout(1, 1);

		if ( temporalInputLayout == nullptr || normalCopyInputLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		{
			/* Temporal resolve: single set, no push constants (frame UBO). */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(temporalInputLayout);

			m_temporalLayout = layoutManager.getPipelineLayout(sets, {});
		}

		{
			/* Normal history: single set, no push constants (frame UBO). */
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(normalCopyInputLayout);

			m_normalCopyLayout = layoutManager.getPipelineLayout(sets, {});
		}

		if ( m_temporalLayout == nullptr || m_normalCopyLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders + create pipelines ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto temporalFragment = shaderManager.getShaderModuleFromSourceCode(device, "GIDenoiser_Temporal_FS", ShaderType::FragmentShader, GIDenoiserTemporalFragmentShader);
		const auto normalCopyFragment = shaderManager.getShaderModuleFromSourceCode(device, "GIDenoiser_NormalCopy_FS", ShaderType::FragmentShader, GIDenoiserNormalCopyFragmentShader);

		if ( vertexModule == nullptr || temporalFragment == nullptr || normalCopyFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile the GI denoiser shaders !";

			return false;
		}

		/* NOTE: The pipelines are created against the [0] targets; recording into [1]
		 * relies on Vulkan render pass compatibility (identical format/ops), exactly
		 * like the shared denoise pipeline recording into both blur targets. */
		m_temporalPipeline = this->createFullscreenPipeline(ClassId, baseName + "_GITemporal", vertexModule, temporalFragment, m_temporalLayout, m_historyTargets[0]);
		m_normalCopyPipeline = this->createFullscreenPipeline(ClassId, baseName + "_GINormalCopy", vertexModule, normalCopyFragment, m_normalCopyLayout, m_normalHistoryTargets[0]);

		if ( m_temporalPipeline == nullptr || m_normalCopyPipeline == nullptr )
		{
			return false;
		}

		/* ---- Per-frame descriptor sets (texture bindings are rewritten every frame
		 * because of the history ping-pong; the UBO binding is written once here) ---- */
		m_temporalPerFrame = this->createPerFrameDescriptorSets(temporalInputLayout, ClassId, baseName + "_GITemporal_DescSet");
		m_normalCopyPerFrame = this->createPerFrameDescriptorSets(normalCopyInputLayout, ClassId, baseName + "_GINormalCopy_DescSet");

		if ( m_temporalPerFrame.empty() || m_normalCopyPerFrame.empty() )
		{
			return false;
		}

		for ( size_t f = 0; f < m_temporalPerFrame.size(); ++f )
		{
			if ( !m_temporalPerFrame[f]->writeUniformBufferObject(6, *m_frameUBOs[f]) )
			{
				return false;
			}

			if ( !m_normalCopyPerFrame[f]->writeUniformBufferObject(1, *m_frameUBOs[f]) )
			{
				return false;
			}
		}

		return true;
	}

	void
	GIDenoiser::destroy () noexcept
	{
		m_normalCopyPerFrame.clear();
		m_temporalPerFrame.clear();

		m_frameUBOs.clear();

		m_normalCopyPipeline.reset();
		m_temporalPipeline.reset();
		m_normalCopyLayout.reset();
		m_temporalLayout.reset();

		for ( auto & target : m_normalHistoryTargets )
		{
			target.destroy();
		}

		for ( auto & target : m_historyTargets )
		{
			target.destroy();
		}

		m_historyValid = false;
		m_historyWriteIndex = 0;
		m_noiseFrameIndex = 0;
	}

	bool
	GIDenoiser::updateFrameData (uint32_t frameIndex, const FrameUBOData & data) noexcept
	{
		const auto success = IndirectPostProcessEffect::updateUniformBufferData(*m_frameUBOs[frameIndex], &data, sizeof(FrameUBOData));

		if ( !success )
		{
			TraceError{ClassId} << "Failed to update the GI denoiser frame UBO !";
		}

		/* Advance the animated-noise sequence once per recorded frame (wraps at 4096,
		 * exactly representable in float32 so fract(n * R2) stays precise). */
		m_noiseFrameIndex = (m_noiseFrameIndex + 1U) % 4096U;

		return success;
	}

	const TextureInterface *
	GIDenoiser::recordResolve (const CommandBuffer & commandBuffer, const TextureInterface & noisyInput, const FrameContext & context) noexcept
	{
		if ( !this->temporalActive() )
		{
			return &noisyInput;
		}

		const auto frameIndex = this->renderer().currentFrameIndex();

		const uint32_t writeIdx = m_historyWriteIndex;
		const uint32_t readIdx = 1U - writeIdx;

		/* ---- Per-frame descriptor updates ---- */

		static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(0, noisyInput));

		/* History ping-pong: this frame reads [readIdx] and writes [writeIdx]. */
		static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(3, m_historyTargets[readIdx]));
		static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(4, m_normalHistoryTargets[readIdx]));

		if ( context.depth != nullptr )
		{
			static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(1, *context.depth));
		}

		if ( context.normals != nullptr )
		{
			static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(2, *context.normals));
			static_cast< void >(m_normalCopyPerFrame[frameIndex]->writeCombinedImageSampler(0, *context.normals));
		}

		if ( context.velocity != nullptr )
		{
			static_cast< void >(m_temporalPerFrame[frameIndex]->writeCombinedImageSampler(5, *context.velocity));
		}

		/* ---- Temporal resolve + normal history ---- */

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_historyTargets[writeIdx],
			*m_temporalPipeline,
			*m_temporalLayout,
			*m_temporalPerFrame[frameIndex],
			nullptr,
			0
		);

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_normalHistoryTargets[writeIdx],
			*m_normalCopyPipeline,
			*m_normalCopyLayout,
			*m_normalCopyPerFrame[frameIndex],
			nullptr,
			0
		);

		/* Flip the history ping-pong for the next frame. */
		m_historyWriteIndex = readIdx;
		m_historyValid = true;

		/* The combine consumes the freshly resolved history ([writeIdx] was written
		 * THIS frame — captured before the flip above). */
		return &m_historyTargets[writeIdx];
	}
}
