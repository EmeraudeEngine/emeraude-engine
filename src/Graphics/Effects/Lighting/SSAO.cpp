/*
 * src/Graphics/Effects/Lighting/SSAO.cpp
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

#include "SSAO.hpp"

/* STL inclusions. */
#include <cstdint>
#include <string>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/Effects/Shared/MarchDitherGLSL.hpp"
#include "Graphics/Effects/Shared/ViewReconstructionGLSL.hpp"
#include "Graphics/Renderer.hpp"
#include "Math/Vector.hpp"
#include "PrimaryServices.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Saphir/Types.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "StaticVector.hpp"
#include "Tracer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/LayoutManager.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class DescriptorSetLayout;
}

namespace
{
	using namespace EmEn;

	/** @brief The view reconstruction's binding (after the three samplers: getInputLayout(3, 1)). */
	constexpr uint32_t ViewBinding{3};

	constexpr auto SSAOHeader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out float outAO;

layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D normalTex;
layout(set = 0, binding = 2) uniform sampler2D materialPropsTex;
)GLSL";

	constexpr auto SSAOComputeFragmentShader = R"GLSL(
layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float radius;
	float intensity;
	float bias;
	uint sampleCount;
};

/* The "reconstruction space" of this pass: view-space X and Y, and the distance along the view axis as a POSITIVE Z.
 * Through the frame's inverse projection (ViewReconstructionGLSL), never a scalar linearisation: the chain's clip
 * planes were frozen at 0.1 / 1000, so with a wide lens this pass read 2.11 m for 1 m (2026-10-25). */
vec3 reconstructPosition (vec2 uv, float depth)
{
	vec3 viewPosition = emViewPosition(uv, depth);

	return vec3(viewPosition.xy, -viewPosition.z);
}

/* The depth buffer's own resolution at this pixel, in metres along the view axis: the distance one step of the stored
 * value spans (one float ulp — D32, conventional, so the step grows as the square of the distance: ~6 mm at 100 m, ~2.4 m
 * at 2 km with a 0.1 m near plane). Through the same reconstruction as the samples: projection-agnostic. */
float depthStepMetres (vec2 uv, float depth, float viewDepth)
{
	const float ulp = exp2(floor(log2(max(depth, 1.0e-30))) - 23.0);

	return abs(-emViewPosition(uv, min(depth + ulp, 1.0)).z - viewDepth);
}

/* Generate a sample in a hemisphere using noise-based rotation. */
vec3 hemispherePoint (uint i, vec2 noise)
{
	float fi = float(i);
	float angle = fi * 2.399963 + noise.x * 6.283185;
	float r = sqrt((fi + 0.5) / float(sampleCount));
	float z = sqrt(1.0 - r * r);
	return vec3(cos(angle) * r, sin(angle) * r, z);
}

void main()
{
	float centerDepth = texture(depthTex, vUV).r;

	/* Skip far-plane fragments. */
	if (centerDepth >= 1.0)
	{
		outAO = 1.0;
		return;
	}

	/* A surface that takes no ambient occlusion (aoResponse 0: an unlit one, whose normal is a placeholder) gets none
	 * here, so the blur cannot spread a value computed around that placeholder onto its neighbours. */
	if ((uint(texture(materialPropsTex, vUV).g * 255.0) >> 4u) == 0u)
	{
		outAO = 1.0;
		return;
	}

	vec3 centerPos = reconstructPosition(vUV, centerDepth);

	/* Read view-space normal from MRT normal buffer and convert to reconstruction space.
	 * Reconstruction space matches view space for X and Y, but Z is negated
	 * (linearDepth is positive, view-space Z is negative for objects in front of the camera). */
	vec3 rawN = texture(normalTex, vUV).rgb;

	if (dot(rawN, rawN) < 0.0001)
	{
		outAO = 1.0;
		return;
	}

	vec3 normal = normalize(vec3(rawN.x, rawN.y, -rawN.z));

	/* DEPTH PRECISION (2026-10-27): a sample is "behind" only beyond what the depth buffer can tell apart — two steps,
	 * the centre's and the sample's. A constant bias under the step turned the quantization into occlusion: the open
	 * sea far away read ~50 % occluded, in row stripes (engine item ssao-darkens-open-sea-at-distance). */
	const float effectiveBias = max(bias, 2.0 * depthStepMetres(vUV, centerDepth, centerPos.z));

	/* Per-pixel random rotation: the engine's interleaved gradient noise on the PIXEL coordinates (2026-10-27). It was
	 * fract(sin(dot(vUV, …)) * 43758.5453) on the UV: the precision of sin() over such arguments is the GPU's own, and
	 * Intel drew static 13-pixel row stripes with it (engine item ssao-darkens-open-sea-at-distance). Two decorrelated
	 * draws: the second at an offset of the same pixel lattice. */
	vec2 noiseVec = vec2(emInterleavedGradientNoise(gl_FragCoord.xy), emInterleavedGradientNoise(gl_FragCoord.xy + vec2(47.0, 17.0)));

	/* Build a tangent-space basis. */
	vec3 tangent = normalize(vec3(noiseVec.x, noiseVec.y, 0.0) - normal * dot(vec3(noiseVec.x, noiseVec.y, 0.0), normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	/* Accumulate occlusion. */
	float occlusion = 0.0;

	for (uint i = 0u; i < sampleCount; ++i)
	{
		vec3 sampleDir = TBN * hemispherePoint(i, noiseVec);
		vec3 samplePos = centerPos + sampleDir * radius;

		/* Project sample back to screen space, through the frame's projection. */
		vec2 sampleUV = emUVFromView(vec3(samplePos.xy, -samplePos.z));

		/* Samples projected outside the frame have no depth information: treat them as
		 * unoccluded. The clamp-to-edge sampler used to recycle the border depth, turning
		 * every screen edge into a false full-occlusion band (solid black strip at the
		 * bottom of the frame on close grazing floors). */
		if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
		{
			continue;
		}

		/* Sample depth at projected position. */
		float sampleDepth = emViewDepth(sampleUV, texture(depthTex, sampleUV).r);

		/* Range check and accumulate. */
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(centerPos.z - sampleDepth));
		occlusion += (sampleDepth <= samplePos.z - effectiveBias ? 1.0 : 0.0) * rangeCheck;
	}

	/* Pure visibility term — the user-facing intensity is applied ONCE, in the apply pass.
	 * It used to be multiplied here AND fed to an extrapolating mix() there (t > 1), a
	 * double application that made default SSAO far too dark. */
	occlusion = 1.0 - (occlusion / float(sampleCount));
	outAO = clamp(occlusion, 0.0, 1.0);
}
)GLSL";

}

namespace EmEn::Graphics::Effects::Lighting
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	SSAO::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		auto & settings = renderer.primaryServices().settings();

		/* User-facing parameters, engine-wide and persisted in the settings file.
		 * These override any constructor-provided values. */
		m_parameters.radius = settings.getOrSetDefault< float >(GraphicsPPAmbientOcclusionSSRadiusKey, DefaultGraphicsPPAmbientOcclusionSSRadius);
		m_parameters.intensity = settings.getOrSetDefault< float >(GraphicsPPAmbientOcclusionIntensityKey, DefaultGraphicsPPAmbientOcclusionIntensity);
		m_parameters.bias = settings.getOrSetDefault< float >(GraphicsPPAmbientOcclusionSSBiasKey, DefaultGraphicsPPAmbientOcclusionSSBias);
		m_parameters.sampleCount = settings.getOrSetDefaultInRange< uint32_t >(GraphicsPPAmbientOcclusionSSSampleCountKey, DefaultGraphicsPPAmbientOcclusionSSSampleCount, MinGraphicsPPAmbientOcclusionSSSampleCount, MaxGraphicsPPAmbientOcclusionSSSampleCount);

		const auto halfW = (width > 1) ? width / 2 : 1U;
		const auto halfH = (height > 1) ? height / 2 : 1U;

		/* AO computation target (half-res, single channel). */
		if ( !m_aoTarget.create(renderer, halfW, halfH, VK_FORMAT_R8_UNORM, "SSAO_AO") )
		{
			TraceError{ClassId} << "Failed to create SSAO AO target !";

			return false;
		}

		/* Blur targets (half-res, single channel). */
		if ( !m_blurHTarget.create(renderer, halfW, halfH, VK_FORMAT_R8_UNORM, "SSAO_BlurH") )
		{
			TraceError{ClassId} << "Failed to create SSAO blur H target !";

			return false;
		}

		if ( !m_blurVTarget.create(renderer, halfW, halfH, VK_FORMAT_R8_UNORM, "SSAO_BlurV") )
		{
			TraceError{ClassId} << "Failed to create SSAO blur V target !";

			return false;
		}

		/* ---- Descriptor set layouts (shared) ---- */
		auto singleLayout = this->getInputLayout(1);
		/* Depth, normals, material properties and the view reconstruction. */
		auto dualLayout = this->getInputLayout(3, 1);

		if ( singleLayout == nullptr || dualLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layouts ---- */
		auto & layoutManager = renderer.layoutManager();

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(dualLayout);

			m_aoLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(SSAOPushConstants)}
			});
		}

		if ( m_aoLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = this->getFullscreenVertexShader();
		const auto aoFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSAO_AO_FS", Saphir::ShaderType::FragmentShader, std::string{SSAOHeader} + ViewReconstructionGLSL::sourceGLSL(0, ViewBinding) + EMEN_MARCH_DITHER_GLSL + SSAOComputeFragmentShader);

		if ( vertexModule == nullptr || aoFragment == nullptr )
		{
			TraceError{ClassId} << "Failed to compile SSAO shaders !";

			return false;
		}

		/* ---- Create pipelines ---- */
		m_aoPipeline = this->createFullscreenPipeline(ClassId, "SSAO_AO", vertexModule, aoFragment, m_aoLayout, m_aoTarget);

		if ( m_aoPipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		/* AO computation: reads depth + normals (updated per-frame). */
		m_aoPerFrame = this->createPerFrameDescriptorSets(dualLayout, ClassId, "AO_DescSet");

		return !m_aoPerFrame.empty();
	}

	void
	SSAO::destroy () noexcept
	{
		m_aoPerFrame.clear();
		
		m_aoPipeline.reset();
		m_aoLayout.reset();

		m_blurVTarget.destroy();
		m_blurHTarget.destroy();
		m_aoTarget.destroy();
	}

	void
	SSAO::recordPreDenoisePasses (const CommandBuffer & commandBuffer, const TextureInterface & /*inputColor*/, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputNormals = context.normals;

		/* The view reconstruction is made with the chain (PostProcessor::configure() fails without it); the material
		 * properties come with requiresMaterialProperties(). */
		if ( context.view == nullptr || context.materialProperties == nullptr )
		{
			return;
		}

		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Update depth + normals descriptors for this frame's AO pass. */
		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_aoPerFrame[frameIndex]->writeCombinedImageSampler(0, *inputDepth));
		}

		if ( inputNormals != nullptr )
		{
			static_cast< void >(m_aoPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputNormals));
		}

		static_cast< void >(m_aoPerFrame[frameIndex]->writeCombinedImageSampler(2, *context.materialProperties));
		static_cast< void >(m_aoPerFrame[frameIndex]->writeUniformBufferObject(ViewBinding, *context.view));

		/* ---- Pass 1: AO Computation ---- */
		{
			const SSAOPushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_aoTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_aoTarget.height()),
				.radius = m_parameters.radius,
				.intensity = m_parameters.intensity,
				.bias = m_parameters.bias,
				.sampleCount = m_parameters.sampleCount
			};

			IndirectPostProcessEffect::recordFullscreenPass(
				commandBuffer,
				m_aoTarget,
				*m_aoPipeline,
				*m_aoLayout,
				*m_aoPerFrame[frameIndex],
				&pc,
				sizeof(pc)
			);
		}
	}

	IndirectPostProcessEffect::DenoiseContribution
	SSAO::denoiseContribution (const FrameContext & /*context*/) noexcept
	{
		DenoiseContribution contribution;
		contribution.prefix = "ssao";
		contribution.source = &m_aoTarget;
		contribution.targetH = &m_blurHTarget;
		contribution.targetV = &m_blurVTarget;

		/* Same 5-tap gaussian as the retired SSAO_Blur_FS pass (no guides). */
		contribution.code =
			"\tvec2 ssaoTexel = 1.0 / vec2(textureSize(ssaoSrc, 0));\n"
			"\tfloat ssaoBlur = 0.0;\n"
			"\tssaoBlur += texture(ssaoSrc, vUV - 2.0 * emDenoiseDir * ssaoTexel).r * 0.06136;\n"
			"\tssaoBlur += texture(ssaoSrc, vUV - 1.0 * emDenoiseDir * ssaoTexel).r * 0.24477;\n"
			"\tssaoBlur += texture(ssaoSrc, vUV).r * 0.38774;\n"
			"\tssaoBlur += texture(ssaoSrc, vUV + 1.0 * emDenoiseDir * ssaoTexel).r * 0.24477;\n"
			"\tssaoBlur += texture(ssaoSrc, vUV + 2.0 * emDenoiseDir * ssaoTexel).r * 0.06136;\n"
			"\tvec4 ssaoResult = vec4(ssaoBlur, 0.0, 0.0, 1.0);\n";

		return contribution;
	}

	IndirectPostProcessEffect::CombineContribution
	SSAO::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		CombineContribution contribution;
		contribution.prefix = "ssao";
		contribution.samplers.emplace_back(CombineSamplerInput{.nameSuffix = "Tex", .texture = &m_blurVTarget});
		contribution.needsMaterialProperties = true;
		contribution.dynamics.emplace_back(Base::Math::Vector< 4, float >{m_parameters.intensity, 0.0F, 0.0F, 0.0F});

		/* Same math as the retired SSAO_Apply_FS pass: user intensity (clamped mix, an
		 * intensity above 1 saturates rather than extrapolating), then the material
		 * aoResponse with emissive rejection. */
		contribution.code =
			"\tfloat ssaoAO = texture(ssaoTex, vUV).r;\n"
			"\tvec4 ssaoMp = texture(emMaterialProps, vUV);\n"
			"\tfloat ssaoResponse = float(uint(ssaoMp.g * 255.0) >> 4u) / 15.0;\n"
			"\tfloat ssaoEmissive = float(uint(ssaoMp.b * 255.0) & 0xFu) / 15.0;\n"
			"\tssaoAO = clamp(mix(1.0, ssaoAO, emDyn.ssaoDynamics0.x), 0.0, 1.0);\n"
			"\tssaoAO = mix(1.0, ssaoAO, ssaoResponse * (1.0 - ssaoEmissive));\n"
			"\tem_Color.rgb *= ssaoAO;\n";

		return contribution;
	}
}
