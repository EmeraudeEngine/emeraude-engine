/*
 * src/Graphics/Effects/Framebuffer/SSAO.cpp
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
	using namespace EmEn;

	static constexpr auto SSAOComputeFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out float outAO;

layout(set = 0, binding = 0) uniform sampler2D depthTex;
layout(set = 0, binding = 1) uniform sampler2D normalTex;

layout(push_constant) uniform PushConstants
{
	float texelSizeX;
	float texelSizeY;
	float radius;
	float intensity;
	float bias;
	float nearPlane;
	float farPlane;
	float tanHalfFovY;
	float aspectRatio;
	uint sampleCount;
};

/* Linearize depth from [0,1] range. */
float linearizeDepth (float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

/* Reconstruct view-space position from depth and UV.
 * NOTE: tanHalfFovY is SIGNED and carries the projection's Y direction (see PostProcessor).
 * Do NOT wrap it in abs(): that discards exactly the information it looks like it protects. */
vec3 reconstructPosition (vec2 uv, float depth)
{
	float linearZ = linearizeDepth(depth);
	vec2 ndc = uv * 2.0 - 1.0;
	float t = tanHalfFovY;
	return vec3(ndc * vec2(t * aspectRatio, t) * linearZ, linearZ);
}

/* Hash function for pseudo-random sampling. */
float hash (vec2 p)
{
	return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
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

	/* Generate per-pixel random rotation. */
	vec2 noiseVec = vec2(hash(vUV), hash(vUV * 2.37));

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

		/* Project sample back to screen space. */
		float t = tanHalfFovY;
		vec2 sampleUV = samplePos.xy / (samplePos.z * vec2(t * aspectRatio, t)) * 0.5 + 0.5;

		/* Samples projected outside the frame have no depth information: treat them as
		 * unoccluded. The clamp-to-edge sampler used to recycle the border depth, turning
		 * every screen edge into a false full-occlusion band (solid black strip at the
		 * bottom of the frame on close grazing floors). */
		if (any(lessThan(sampleUV, vec2(0.0))) || any(greaterThan(sampleUV, vec2(1.0))))
		{
			continue;
		}

		/* Sample depth at projected position. */
		float sampleDepth = linearizeDepth(texture(depthTex, sampleUV).r);

		/* Range check and accumulate. */
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(centerPos.z - sampleDepth));
		occlusion += (sampleDepth <= samplePos.z - bias ? 1.0 : 0.0) * rangeCheck;
	}

	/* Pure visibility term — the user-facing intensity is applied ONCE, in the apply pass.
	 * It used to be multiplied here AND fed to an extrapolating mix() there (t > 1), a
	 * double application that made default SSAO far too dark. */
	occlusion = 1.0 - (occlusion / float(sampleCount));
	outAO = clamp(occlusion, 0.0, 1.0);
}
)GLSL";

}

namespace EmEn::Graphics::Effects::Framebuffer
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
		m_parameters.radius = settings.getOrSetDefault< float >(GraphicsScreenSpaceAORadiusKey, DefaultGraphicsScreenSpaceAORadius);
		m_parameters.intensity = settings.getOrSetDefault< float >(GraphicsScreenSpaceAOIntensityKey, DefaultGraphicsScreenSpaceAOIntensity);
		m_parameters.bias = settings.getOrSetDefault< float >(GraphicsScreenSpaceAOBiasKey, DefaultGraphicsScreenSpaceAOBias);
		m_parameters.sampleCount = settings.getOrSetDefault< uint32_t >(GraphicsScreenSpaceAOSampleCountKey, DefaultGraphicsScreenSpaceAOSampleCount);

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
		auto dualLayout = this->getInputLayout(2);

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
		const auto aoFragment = shaderManager.getShaderModuleFromSourceCode(device, "SSAO_AO_FS", Saphir::ShaderType::FragmentShader, SSAOComputeFragmentShader);

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

		if ( m_aoPerFrame.empty() )
		{
			return false;
		}

		return true;
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
		const auto & constants = context.constants;


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

		/* ---- Pass 1: AO Computation ---- */
		{
			const SSAOPushConstants pc{
				.texelSizeX = 1.0F / static_cast< float >(m_aoTarget.width()),
				.texelSizeY = 1.0F / static_cast< float >(m_aoTarget.height()),
				.radius = m_parameters.radius,
				.intensity = m_parameters.intensity,
				.bias = m_parameters.bias,
				.nearPlane = constants.nearPlane,
				.farPlane = constants.farPlane,
				.tanHalfFovY = constants.tanHalfFovY,
				.aspectRatio = constants.frameWidth / constants.frameHeight,
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
	SSAO::denoiseContribution (const FrameContext & /*context*/) const noexcept
	{
		DenoiseContribution contribution;
		contribution.prefix = "ssao";
		contribution.source = &m_aoTarget;
		contribution.targetH = const_cast< IntermediateRenderTarget * >(&m_blurHTarget);
		contribution.targetV = const_cast< IntermediateRenderTarget * >(&m_blurVTarget);

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
		contribution.samplers.emplace_back(CombineSamplerInput{"Tex", &m_blurVTarget});
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
