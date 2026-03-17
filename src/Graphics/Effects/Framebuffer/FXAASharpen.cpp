/*
 * src/Graphics/Effects/Framebuffer/FXAASharpen.cpp
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

#include "FXAASharpen.hpp"

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
static constexpr auto TracerTag{"FXAASharpenEffect"};
/* NOLINTEND(cert-err58-cpp) */

namespace
{
	using namespace EmEn;

	/* ---- Push Constants ---- */

	struct FXAASharpenPushConstants
	{
		float texelSizeX;
		float texelSizeY;
		float subpixelQuality;
		float edgeThreshold;
		float edgeThresholdMin;
		float sharpness;
		float padding0;
		float padding1;
	};

	static_assert(sizeof(FXAASharpenPushConstants) == 32, "FXAASharpenPushConstants must be 32 bytes.");

	/* ---- GLSL Shader Sources ---- */

	/* Combined FXAA 3.11 Quality 12 + CAS Sharpen in a single pass.
	 * First applies FXAA to the input texture to get anti-aliased color,
	 * then applies CAS sharpening to that result.
	 * Based on Timothy Lottes' FXAA 3.11 (NVIDIA) and AMD FidelityFX CAS. */
	static constexpr auto FXAASharpenFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float subpixelQuality;
	float edgeThreshold;
	float edgeThresholdMin;
	float sharpness;
	float padding0;
	float padding1;
};

/* Rec. 709 luminance. */
float luminance(vec3 color)
{
	return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

/* ============================================================
 * FXAA 3.11 Quality 12 — returns the anti-aliased color at vUV.
 * ============================================================ */
vec3 fxaa()
{
	vec2 texelSize = vec2(texelSizeX, texelSizeY);

	/* Step 1: Sample 3x3 neighborhood. */
	vec3 rgbM  = texture(inputTex, vUV).rgb;
	vec3 rgbN  = texture(inputTex, vUV + vec2( 0.0, -texelSize.y)).rgb;
	vec3 rgbS  = texture(inputTex, vUV + vec2( 0.0,  texelSize.y)).rgb;
	vec3 rgbE  = texture(inputTex, vUV + vec2( texelSize.x,  0.0)).rgb;
	vec3 rgbW  = texture(inputTex, vUV + vec2(-texelSize.x,  0.0)).rgb;
	vec3 rgbNW = texture(inputTex, vUV + vec2(-texelSize.x, -texelSize.y)).rgb;
	vec3 rgbNE = texture(inputTex, vUV + vec2( texelSize.x, -texelSize.y)).rgb;
	vec3 rgbSW = texture(inputTex, vUV + vec2(-texelSize.x,  texelSize.y)).rgb;
	vec3 rgbSE = texture(inputTex, vUV + vec2( texelSize.x,  texelSize.y)).rgb;

	/* Step 2: Compute luminances. */
	float lumaM  = luminance(rgbM);
	float lumaN  = luminance(rgbN);
	float lumaS  = luminance(rgbS);
	float lumaE  = luminance(rgbE);
	float lumaW  = luminance(rgbW);
	float lumaNW = luminance(rgbNW);
	float lumaNE = luminance(rgbNE);
	float lumaSW = luminance(rgbSW);
	float lumaSE = luminance(rgbSE);

	/* Step 3: Early exit on low contrast. */
	float lumaMin = min(lumaM, min(min(lumaN, lumaS), min(lumaE, lumaW)));
	float lumaMax = max(lumaM, max(max(lumaN, lumaS), max(lumaE, lumaW)));
	float lumaRange = lumaMax - lumaMin;

	if (lumaRange < max(edgeThresholdMin, lumaMax * edgeThreshold))
	{
		return rgbM;
	}

	/* Step 4: Edge orientation (horizontal vs vertical). */
	float edgeH = abs(lumaNW + lumaNE - 2.0 * lumaN)
	            + abs(lumaW  + lumaE  - 2.0 * lumaM) * 2.0
	            + abs(lumaSW + lumaSE - 2.0 * lumaS);

	float edgeV = abs(lumaNW + lumaSW - 2.0 * lumaW)
	            + abs(lumaN  + lumaS  - 2.0 * lumaM) * 2.0
	            + abs(lumaNE + lumaSE - 2.0 * lumaE);

	bool isHorizontal = (edgeH >= edgeV);

	/* Step 5: Select edge-perpendicular direction. */
	float stepLength = isHorizontal ? texelSize.y : texelSize.x;

	float luma1 = isHorizontal ? lumaN : lumaW;
	float luma2 = isHorizontal ? lumaS : lumaE;
	float gradient1 = abs(luma1 - lumaM);
	float gradient2 = abs(luma2 - lumaM);

	bool is1Steeper = (gradient1 >= gradient2);

	float gradientScaled = 0.25 * max(gradient1, gradient2);
	float lumaLocalAverage;

	if (is1Steeper)
	{
		stepLength = -stepLength;
		lumaLocalAverage = 0.5 * (luma1 + lumaM);
	}
	else
	{
		lumaLocalAverage = 0.5 * (luma2 + lumaM);
	}

	/* Step 6: Shift UV to edge center. */
	vec2 currentUV = vUV;

	if (isHorizontal)
	{
		currentUV.y += stepLength * 0.5;
	}
	else
	{
		currentUV.x += stepLength * 0.5;
	}

	/* Step 7: Edge endpoint search (12 iterations, growing steps). */
	vec2 offset = isHorizontal ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);

	vec2 uv1 = currentUV - offset;
	vec2 uv2 = currentUV + offset;

	float lumaEnd1 = luminance(texture(inputTex, uv1).rgb) - lumaLocalAverage;
	float lumaEnd2 = luminance(texture(inputTex, uv2).rgb) - lumaLocalAverage;

	bool reached1 = (abs(lumaEnd1) >= gradientScaled);
	bool reached2 = (abs(lumaEnd2) >= gradientScaled);
	bool reachedBoth = reached1 && reached2;

	if (!reached1) uv1 -= offset;
	if (!reached2) uv2 += offset;

	/* Quality 12 step pattern: 1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0 */
	const float QUALITY[12] = float[12](1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0);

	if (!reachedBoth)
	{
		for (int i = 2; i < 12; i++)
		{
			if (!reached1)
			{
				lumaEnd1 = luminance(texture(inputTex, uv1).rgb) - lumaLocalAverage;
			}

			if (!reached2)
			{
				lumaEnd2 = luminance(texture(inputTex, uv2).rgb) - lumaLocalAverage;
			}

			reached1 = (abs(lumaEnd1) >= gradientScaled);
			reached2 = (abs(lumaEnd2) >= gradientScaled);
			reachedBoth = reached1 && reached2;

			if (!reached1) uv1 -= offset * QUALITY[i];
			if (!reached2) uv2 += offset * QUALITY[i];

			if (reachedBoth) break;
		}
	}

	/* Step 8: Compute edge blend factor. */
	float distance1, distance2;

	if (isHorizontal)
	{
		distance1 = vUV.x - uv1.x;
		distance2 = uv2.x - vUV.x;
	}
	else
	{
		distance1 = vUV.y - uv1.y;
		distance2 = uv2.y - vUV.y;
	}

	bool isDirection1 = (distance1 < distance2);
	float distanceFinal = min(distance1, distance2);
	float edgeLength = distance1 + distance2;
	float pixelOffset = -distanceFinal / edgeLength + 0.5;

	bool isLumaMSmaller = (lumaM < lumaLocalAverage);
	bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0) != isLumaMSmaller;

	float edgeBlend = correctVariation ? pixelOffset : 0.0;

	/* Step 9: Subpixel aliasing blend factor. */
	float lumaAverage = (lumaN + lumaS + lumaE + lumaW) * (1.0 / 6.0)
	                  + (lumaNW + lumaNE + lumaSW + lumaSE) * (1.0 / 12.0);

	float subpixelOffset = clamp(abs(lumaAverage - lumaM) / lumaRange, 0.0, 1.0);
	float subpixelBlend = (-2.0 * subpixelOffset + 3.0) * subpixelOffset * subpixelOffset;
	subpixelBlend = subpixelBlend * subpixelBlend * subpixelQuality;

	/* Step 10: Final FXAA blend. */
	float finalBlend = max(edgeBlend, subpixelBlend);

	vec2 finalUV = vUV;

	if (isHorizontal)
	{
		finalUV.y += finalBlend * stepLength;
	}
	else
	{
		finalUV.x += finalBlend * stepLength;
	}

	return texture(inputTex, finalUV).rgb;
}

/* ============================================================
 * CAS Sharpen — applied to the FXAA result.
 * Uses textureOffset on the FXAA output UV for the 5-tap cross.
 * Since we cannot re-sample the "FXAA output" (it's computed in
 * the same shader), we apply CAS to the FXAA-resolved color
 * and its cardinal neighbors from the input texture.
 * ============================================================ */
void main()
{
	/* Get FXAA anti-aliased center color. */
	vec3 center = fxaa();

	/* CAS 5-tap cross from the original texture (post-FXAA neighbors are
	 * the original texels — FXAA only shifts the center sample, not neighbors).
	 * This matches the standalone CAS behavior: sharpening around the center. */
	vec3 north = textureOffset(inputTex, vUV, ivec2( 0, -1)).rgb;
	vec3 south = textureOffset(inputTex, vUV, ivec2( 0,  1)).rgb;
	vec3 east  = textureOffset(inputTex, vUV, ivec2( 1,  0)).rgb;
	vec3 west  = textureOffset(inputTex, vUV, ivec2(-1,  0)).rgb;

	/* Luminance (Rec. 709). */
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
	FXAASharpen::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		/* Output is LDR (8-bit per channel). */
		if ( !m_outputTarget.create(renderer, width, height, VK_FORMAT_R8G8B8A8_UNORM, "FXAASharpenOutput") )
		{
			TraceError{TracerTag} << "Failed to create FXAA+Sharpen output target !";

			return false;
		}

		/* Compile shaders. */
		auto vertexModule = this->getFullscreenVertexShader();

		auto fragmentModule = renderer.shaderManager().getShaderModuleFromSourceCode(renderer.device(), "FXAASharpenFS", ShaderType::FragmentShader, FXAASharpenFragmentShader);

		if ( vertexModule == nullptr || fragmentModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile FXAA+Sharpen shaders !";

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

			/* Push constant range (32 bytes). */
			m_pipelineLayout = renderer.layoutManager().getPipelineLayout(sets, {
				VkPushConstantRange{
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					.offset = 0,
					.size = sizeof(FXAASharpenPushConstants)
				}
			});
		}

		if ( m_pipelineLayout == nullptr )
		{
			TraceError{TracerTag} << "Failed to create pipeline layout !";

			return false;
		}

		/* Graphics pipeline. */
		m_pipeline = this->createFullscreenPipeline(ClassId, "FXAASharpen", vertexModule, fragmentModule, m_pipelineLayout, m_outputTarget);

		if ( m_pipeline == nullptr )
		{
			TraceError{TracerTag} << "Failed to create FXAA+Sharpen pipeline !";

			return false;
		}

		/* Per-frame descriptor sets. */
		m_descriptorSets = this->createPerFrameDescriptorSets(descriptorSetLayout, ClassId, "FXAASharpenDescSet");

		if ( m_descriptorSets.empty() )
		{
			TraceError{TracerTag} << "Failed to create per-frame descriptor sets !";

			return false;
		}

		return true;
	}

	void
	FXAASharpen::destroy () noexcept
	{
		m_descriptorSets.clear();
		m_pipeline.reset();
		m_pipelineLayout.reset();
		m_outputTarget.destroy();
	}

	/* ---- Execute ---- */

	const TextureInterface &
	FXAASharpen::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, [[maybe_unused]] const TextureInterface * inputDepth, [[maybe_unused]] const TextureInterface * inputNormals, [[maybe_unused]] const TextureInterface * inputMaterialProperties, [[maybe_unused]] const Scenes::LightSet * lightSet, [[maybe_unused]] const PostProcessor::PushConstants & constants) noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Update the per-frame descriptor set with the current input. */
		static_cast< void >(m_descriptorSets[frameIndex]->writeCombinedImageSampler(0, inputColor));

		/* Build push constants with computed texel size and combined parameters. */
		const FXAASharpenPushConstants pc{
			.texelSizeX = 1.0F / static_cast< float >(m_outputTarget.width()),
			.texelSizeY = 1.0F / static_cast< float >(m_outputTarget.height()),
			.subpixelQuality = m_parameters.subpixelQuality,
			.edgeThreshold = m_parameters.edgeThreshold,
			.edgeThresholdMin = m_parameters.edgeThresholdMin,
			.sharpness = m_parameters.sharpness,
			.padding0 = 0.0F,
			.padding1 = 0.0F
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
