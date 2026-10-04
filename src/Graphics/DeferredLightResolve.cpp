/*
 * src/Graphics/DeferredLightResolve.cpp
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

#include "DeferredLightResolve.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

/* Local inclusions. */
#include "Graphics/Effects/Shared/LightFalloffGLSL.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/SceneRenderTarget.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Saphir/Types.hpp"
#include "Scenes/Component/PointLight.hpp"
#include "Scenes/Component/SpotLight.hpp"
#include "Scenes/LightSet.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/ShaderStorageBufferObject.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	namespace
	{
		/** @brief One light of the resolve buffer (std430, 64 bytes), in the VIEW space of the frame. */
		struct DeferredLightData
		{
			/* xyz = position (view space, metres), w = radius (0 = unbounded). */
			std::array< float, 4 > positionRadius;
			/* rgb = emission chromaticity, a = intensity (candela). */
			std::array< float, 4 > colorIntensity;
			/* xyz = spot direction (view space, normalised), w = type (1 point, 2 spot). */
			std::array< float, 4 > directionType;
			/* x = cos(inner angle), y = cos(outer angle). */
			std::array< float, 4 > cone;
		};

		static_assert(sizeof(DeferredLightData) == 64, "DeferredLightData must be 64 bytes (std430).");

		/** @brief Push constants of the resolve (80 bytes, the GLSL block below). */
		struct ResolvePushConstants
		{
			std::array< float, 16 > inverseProjection;
			float inverseExtentX;
			float inverseExtentY;
			uint32_t lightCount;
			uint32_t padding;
		};

		static_assert(sizeof(ResolvePushConstants) == 80, "ResolvePushConstants must be 80 bytes.");

		/** @brief The shared fullscreen triangle (IndirectPostProcessEffect's, same name: one module). */
		constexpr auto FullscreenVertexShader = R"GLSL(
#version 450

layout(location = 0) out vec2 vUV;

void main()
{
	vUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
)GLSL";

		/* The forward light pass's BRDF, term for term (Saphir/LightGenerator.PBR.cpp: a generated point-light pass
		 * of a plain metallic-roughness material reads exactly this), fed from the G-buffer. Only the pixels carrying
		 * the deferred-lighting bit are shaded: every other one keeps its forward passes. */
		constexpr auto ResolveFragmentShader = R"GLSL(
#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D normalsTex;
layout(set = 0, binding = 1) uniform sampler2D materialPropertiesTex;
layout(set = 0, binding = 2) uniform sampler2D albedoTex;
layout(set = 0, binding = 3) uniform sampler2D depthTex;

struct DeferredLight
{
	vec4 positionRadius;
	vec4 colorIntensity;
	vec4 directionType;
	vec4 cone;
};

layout(std430, set = 0, binding = 4) readonly buffer DeferredLights
{
	DeferredLight lights[];
};

layout(push_constant) uniform PushConstants
{
	mat4 inverseProjection;
	vec2 inverseExtent;
	uint lightCount;
	uint padding;
};

)GLSL" EMEN_LIGHT_FALLOFF_GLSL R"GLSL(

vec3 fresnelSchlick (float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float distributionGGX (vec3 N, vec3 H, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float NdotH2 = NdotH * NdotH;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	return a2 / (3.14159265 * denom * denom);
}

float geometrySchlickGGX (float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	return NdotV / (NdotV * (1.0 - k) + k);
}

void main()
{
	const ivec2 pixel = ivec2(gl_FragCoord.xy);

	/* The deferred-lighting bit: R low nibble, bit 0 (LightGenerator::materialPropertiesExpression()). */
	const uint materialBits = uint(texelFetch(materialPropertiesTex, pixel, 0).r * 255.0 + 0.5);

	if ( (materialBits & 1u) == 0u )
	{
		discard;
	}

	/* The view-space position, rebuilt with the frame's jittered projection (the one the depth was rasterised with). */
	const float depth = texelFetch(depthTex, pixel, 0).r;
	const vec4 ndc = vec4(gl_FragCoord.xy * inverseExtent * 2.0 - 1.0, depth, 1.0);
	const vec4 viewPosition = inverseProjection * ndc;
	const vec3 P = viewPosition.xyz / viewPosition.w;

	/* The G-buffer normal is ALREADY oriented toward the viewer by the ambient pass, from the GEOMETRIC side
	 * (LightGenerator.PBR.cpp, two-sided lighting). ⚠️ Never re-orient it here from the normal itself: a normal-map
	 * texel leaning away from a grazing view is legitimate on a front face, and turning it over lit the mortar
	 * grooves of Sponza's walls (measured 2026-10-04; docs/caution-points.md § Two-sided normals). */
	const vec4 normalRoughness = texelFetch(normalsTex, pixel, 0);
	const vec3 V = normalize(-P);
	const vec3 N = normalize(normalRoughness.xyz);

	/* normals.a = SAA roughness + round(metalness) * 2 (SceneRendering's G-buffer packing). */
	const float roughness = normalRoughness.a >= 1.5 ? normalRoughness.a - 2.0 : normalRoughness.a;

	/* albedo.a = the diffuse weight (1 - m)(1 - transmission); an eligible surface does not transmit. */
	const vec4 albedoWeight = texelFetch(albedoTex, pixel, 0);
	const vec3 albedo = albedoWeight.rgb;
	const float metalness = clamp(1.0 - albedoWeight.a, 0.0, 1.0);
	const vec3 F0 = mix(vec3(0.04), albedo, metalness);

	const float NdotV = max(dot(N, V), 0.0);
	const float geometryV = geometrySchlickGGX(NdotV, roughness);

	vec3 radianceSum = vec3(0.0);

	for ( uint index = 0u; index < lightCount; ++index )
	{
		const vec4 positionRadius = lights[index].positionRadius;
		const vec3 toLight = positionRadius.xyz - P;
		const float distanceSquared = dot(toLight, toLight);
		const float radius = positionRadius.w;

		/* Out of reach: the falloff window is exactly zero there. */
		if ( radius > 0.0 && distanceSquared >= radius * radius )
		{
			continue;
		}

		const float lightDistance = sqrt(distanceSquared);
		const vec3 L = toLight / max(lightDistance, 0.0001);

		float lightFactor = emLightFalloff(lightDistance, radius);

		/* The spot cone, with the forward pass's epsilon guard (a hard-edged spot has inner == outer). */
		if ( lights[index].directionType.w > 1.5 )
		{
			const vec4 cone = lights[index].cone;
			const float theta = dot(-L, lights[index].directionType.xyz);
			const float epsilon = max(cone.x - cone.y, 0.0001);

			lightFactor *= clamp((theta - cone.y) / epsilon, 0.0, 1.0);
		}

		const float NdotL = max(dot(N, L), 0.0);

		if ( lightFactor <= 0.0 || NdotL <= 0.0 )
		{
			continue;
		}

		const vec3 H = normalize(V + L);

		const float NDF = distributionGGX(N, H, roughness);
		const float G = geometryV * geometrySchlickGGX(NdotL, roughness);
		const vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

		const vec3 specular = (NDF * G * F) / (4.0 * NdotV * NdotL + 0.0001);
		const vec3 kD = (vec3(1.0) - F) * (1.0 - metalness);

		const vec4 colorIntensity = lights[index].colorIntensity;
		const vec3 radiance = colorIntensity.rgb * colorIntensity.a * lightFactor;

		radianceSum += (kD * albedo / 3.14159265 + specular) * radiance * NdotL;
	}

	outColor = vec4(radianceSum, 0.0);
}
)GLSL";

		/**
		 * @brief Writes one light of the snapshot, from its published block, in the frame's view space.
		 * @tparam positionOffset The block offset of the world position.
		 * @tparam colorOffset The block offset of the emission chromaticity.
		 * @tparam intensityOffset The block offset of the intensity.
		 * @tparam radiusOffset The block offset of the radius.
		 * @param block The light's published uniform block (render state slot of the frame).
		 * @param viewMatrix The frame's view matrix.
		 * @param output A reference to the GPU entry.
		 * @return void
		 */
		template< size_t positionOffset, size_t colorOffset, size_t intensityOffset, size_t radiusOffset >
		void
		writeCommonTerms (const std::array< float, Scenes::Component::AbstractLightEmitter::MaxUniformBlockElementCount > & block, const Matrix< 4, float > & viewMatrix, DeferredLightData & output) noexcept
		{
			static_assert(positionOffset + 2 < Scenes::Component::AbstractLightEmitter::MaxUniformBlockElementCount && colorOffset + 2 < Scenes::Component::AbstractLightEmitter::MaxUniformBlockElementCount, "Offset out of the published block.");
			static_assert(intensityOffset < Scenes::Component::AbstractLightEmitter::MaxUniformBlockElementCount && radiusOffset < Scenes::Component::AbstractLightEmitter::MaxUniformBlockElementCount, "Offset out of the published block.");

			const Vector< 4, float > worldPosition{block[positionOffset + 0], block[positionOffset + 1], block[positionOffset + 2], 1.0F};
			const auto viewPosition = viewMatrix * worldPosition;

			output.positionRadius[0] = viewPosition[X];
			output.positionRadius[1] = viewPosition[Y];
			output.positionRadius[2] = viewPosition[Z];
			output.positionRadius[3] = block[radiusOffset];
			output.colorIntensity[0] = block[colorOffset + 0];
			output.colorIntensity[1] = block[colorOffset + 1];
			output.colorIntensity[2] = block[colorOffset + 2];
			output.colorIntensity[3] = block[intensityOffset];
		}
	}

	DeferredLightResolve::DeferredLightResolve (Renderer & renderer) noexcept
		: m_renderer{renderer}
	{

	}

	DeferredLightResolve::~DeferredLightResolve ()
	{
		this->destroy();
	}

	bool
	DeferredLightResolve::hasGeometryBuffer (const SceneRenderTarget & sceneTarget) noexcept
	{
		return
			sceneTarget.colorImageView() != nullptr &&
			sceneTarget.normalsImageView() != nullptr &&
			sceneTarget.materialPropertiesImageView() != nullptr &&
			sceneTarget.albedoImageView() != nullptr &&
			sceneTarget.depthImageView() != nullptr;
	}

	bool
	DeferredLightResolve::createSharedResources () noexcept
	{
		/* Nearest, clamp-to-edge: every read is a texelFetch() of the pixel itself. */
		m_sampler = m_renderer.getSampler(ClassId, [] (Settings &, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.compareEnable = VK_FALSE;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = 1.0F;
		});

		if ( m_sampler == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the sampler !");

			return false;
		}

		auto & layoutManager = m_renderer.layoutManager();

		m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(ClassId);

		if ( m_descriptorSetLayout == nullptr )
		{
			auto newLayout = layoutManager.prepareNewDescriptorSetLayout(ClassId);
			newLayout->setIdentifier(ClassId, "Resolve", "DescriptorSetLayout");
			newLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);
			newLayout->declareCombinedImageSampler(1, VK_SHADER_STAGE_FRAGMENT_BIT);
			newLayout->declareCombinedImageSampler(2, VK_SHADER_STAGE_FRAGMENT_BIT);
			newLayout->declareCombinedImageSampler(3, VK_SHADER_STAGE_FRAGMENT_BIT);
			newLayout->declareStorageBuffer(4, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(newLayout) )
			{
				Tracer::error(ClassId, "Unable to create the descriptor set layout !");

				return false;
			}

			m_descriptorSetLayout = layoutManager.getDescriptorSetLayout(ClassId);

			if ( m_descriptorSetLayout == nullptr )
			{
				return false;
			}
		}

		/* One set and one light buffer per frame in flight: rewritten every frame, never while a frame in flight reads
		 * them (the frame's fence was waited before recording). */
		for ( uint32_t frameIndex = 0; frameIndex < m_renderer.framesInFlight(); ++frameIndex )
		{
			auto lightBuffer = std::make_unique< ShaderStorageBufferObject >(m_renderer.device(), static_cast< VkDeviceSize >(MaxLights * sizeof(DeferredLightData)));
			lightBuffer->setIdentifier(ClassId, "Lights", "ShaderStorageBufferObject");

			if ( !lightBuffer->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create a light buffer !");

				return false;
			}

			auto descriptorSet = std::make_unique< DescriptorSet >(m_renderer.descriptorPool(), m_descriptorSetLayout);
			descriptorSet->setIdentifier(ClassId, "Resolve", "DescriptorSet");

			if ( !descriptorSet->create() )
			{
				Tracer::error(ClassId, "Unable to create a descriptor set !");

				return false;
			}

			if ( !descriptorSet->writeStorageBuffer(4, lightBuffer->getDescriptorInfo(0, static_cast< uint32_t >(MaxLights * sizeof(DeferredLightData)))) )
			{
				Tracer::error(ClassId, "Unable to bind a light buffer !");

				return false;
			}

			m_lightBuffers.emplace_back(std::move(lightBuffer));
			m_descriptorSets.emplace_back(std::move(descriptorSet));
		}

		StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
		sets.emplace_back(m_descriptorSetLayout);

		m_pipelineLayout = layoutManager.getPipelineLayout(sets, {
			VkPushConstantRange{
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.offset = 0,
				.size = sizeof(ResolvePushConstants)
			}
		});

		if ( m_pipelineLayout == nullptr )
		{
			Tracer::error(ClassId, "Unable to create the pipeline layout !");

			return false;
		}

		m_sharedResourcesCreated = true;

		return true;
	}

	bool
	DeferredLightResolve::createTargetResources (const SceneRenderTarget & sceneTarget) noexcept
	{
		/* The previous target's objects may still be read by a frame in flight. */
		auto & deferredDestructor = m_renderer.deferredDestructor();

		if ( m_pipeline != nullptr )
		{
			deferredDestructor.retireObject(std::move(m_pipeline));
		}

		if ( m_framebuffer != nullptr )
		{
			deferredDestructor.retireObject(std::move(m_framebuffer));
		}

		if ( m_renderPass != nullptr )
		{
			deferredDestructor.retireObject(std::move(m_renderPass));
		}

		m_targetColorView.reset();

		/* The scene colour alone, LOADED and stored back: the resolve ADDS onto what the ambient and the forward light
		 * passes wrote. Same layouts as the scene target's own passes (COLOR_ATTACHMENT_OPTIMAL on both ends). */
		auto renderPass = std::make_shared< RenderPass >(m_renderer.device(), 0);
		renderPass->setIdentifier(ClassId, "Resolve", "RenderPass");

		RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

		renderPass->addAttachmentDescription(VkAttachmentDescription{
			.flags = 0,
			.format = sceneTarget.colorFormat(),
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		});

		subPass.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		renderPass->addSubPass(subPass);

		/* The opaque half wrote the colour before; the translucent half writes it after. */
		renderPass->addSubPassDependency(VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		});

		renderPass->addSubPassDependency(VkSubpassDependency{
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT,
			.dependencyFlags = 0
		});

		if ( !renderPass->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the render pass !");

			return false;
		}

		const auto colorView = sceneTarget.colorImageView();
		const auto & extent = sceneTarget.extent();

		auto framebuffer = std::make_shared< Framebuffer >(renderPass, VkExtent2D{extent.width, extent.height});
		framebuffer->setIdentifier(ClassId, "Resolve", "Framebuffer");
		framebuffer->addAttachment(colorView->handle());

		if ( !framebuffer->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the framebuffer !");

			return false;
		}

		auto & shaderManager = m_renderer.shaderManager();

		const auto vertexModule = shaderManager.getShaderModuleFromSourceCode(m_renderer.device(), "FullscreenPostProcessVS", Saphir::ShaderType::VertexShader, FullscreenVertexShader);
		const auto fragmentModule = shaderManager.getShaderModuleFromSourceCode(m_renderer.device(), "DeferredLightResolveFS", Saphir::ShaderType::FragmentShader, ResolveFragmentShader);

		if ( vertexModule == nullptr || fragmentModule == nullptr )
		{
			Tracer::error(ClassId, "Unable to compile the resolve shaders !");

			return false;
		}

		auto pipeline = std::make_shared< GraphicsPipeline >(m_renderer.device());
		pipeline->setIdentifier(ClassId, "Resolve", "GraphicsPipeline");

		StaticVector< std::shared_ptr< ShaderModule >, 5 > shaderModules;
		shaderModules.emplace_back(vertexModule);
		shaderModules.emplace_back(fragmentModule);

		if ( !pipeline->configureShaderStages(shaderModules) || !pipeline->configureEmptyVertexInputState() || !pipeline->configureInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) )
		{
			return false;
		}

		StaticVector< VkDynamicState, 16 > dynamicStates;
		dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);

		if ( !pipeline->configureDynamicStates(dynamicStates) || !pipeline->configureViewportState(extent.width, extent.height) )
		{
			return false;
		}

		VkPipelineRasterizationStateCreateInfo rasterization{};
		rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization.rasterizerDiscardEnable = VK_FALSE;
		rasterization.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization.cullMode = VK_CULL_MODE_NONE;
		rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterization.depthBiasEnable = VK_FALSE;
		rasterization.lineWidth = 1.0F;

		if ( !pipeline->configureRasterizationState(rasterization) || !pipeline->configureMultisampleState(1) )
		{
			return false;
		}

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		if ( !pipeline->configureDepthStencilState(depthStencil) )
		{
			return false;
		}

		/* Additive over the scene colour, RGB only: the alpha keeps what the ambient pass wrote (an opaque forward light
		 * pass rewrites the same albedo alpha, so leaving it is the identical result). */
		StaticVector< VkPipelineColorBlendAttachmentState, 8 > attachments;
		attachments.emplace_back(VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
		});

		VkPipelineColorBlendStateCreateInfo colorBlend{};
		colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlend.logicOpEnable = VK_FALSE;

		if ( !pipeline->configureColorBlendState(attachments, colorBlend) || !pipeline->finalize(renderPass, m_pipelineLayout, false, false) )
		{
			Tracer::error(ClassId, "Unable to create the resolve pipeline !");

			return false;
		}

		m_renderPass = std::move(renderPass);
		m_framebuffer = std::move(framebuffer);
		m_pipeline = std::move(pipeline);
		m_targetColorView = colorView;

		return true;
	}

	bool
	DeferredLightResolve::prepare (const SceneRenderTarget & sceneTarget, const Scenes::LightSet & lightSet, uint32_t readStateIndex, const ViewMatricesInterface & viewMatrices, bool shadowMapsEnabled) noexcept
	{
		m_lights.clear();

		if ( !hasGeometryBuffer(sceneTarget) )
		{
			return false;
		}

		/* Every resource is made HERE, before the scene skips a single forward pass: once the snapshot is handed to
		 * Scene::renderOpaque(), record() must not be able to fail. */
		if ( !m_sharedResourcesCreated && !this->createSharedResources() )
		{
			return false;
		}

		const auto & lightBuffer = *m_lightBuffers[m_renderer.currentFrameIndex() % m_lightBuffers.size()];
		auto * gpuData = lightBuffer.mapMemoryAs< DeferredLightData >();

		if ( gpuData == nullptr )
		{
			Tracer::error(ClassId, "Unable to map the light buffer !");

			return false;
		}

		const auto & viewMatrix = viewMatrices.viewMatrix(readStateIndex, false, 0);

		/* Snapshot and buffer are built in the SAME order, then the snapshot is sorted for the forward skip's lookup:
		 * the buffer order is free, the lookup only asks "is this light in it". */
		{
			const std::scoped_lock lock{lightSet.mutex()};

			for ( const auto & light : lightSet.pointLights() )
			{
				if ( m_lights.full() )
				{
					break;
				}

				if ( !Scenes::LightSet::isDeferredPunctualLight(*light, shadowMapsEnabled) )
				{
					continue;
				}

				using Scenes::Component::PointLight;

				const auto & block = light->publishedBlock(readStateIndex);
				auto & entry = gpuData[m_lights.size()];

				writeCommonTerms< PointLight::PositionOffset, PointLight::ColorOffset, PointLight::IntensityOffset, PointLight::RadiusOffset >(block, viewMatrix, entry);

				entry.directionType[0] = 0.0F;
				entry.directionType[1] = 0.0F;
				entry.directionType[2] = 0.0F;
				entry.directionType[3] = 1.0F;
				entry.cone[0] = 0.0F;
				entry.cone[1] = 0.0F;
				entry.cone[2] = 0.0F;
				entry.cone[3] = 0.0F;

				m_lights.emplace_back(light.get());
			}

			for ( const auto & light : lightSet.spotLights() )
			{
				if ( m_lights.full() )
				{
					break;
				}

				if ( !Scenes::LightSet::isDeferredPunctualLight(*light, shadowMapsEnabled) )
				{
					continue;
				}

				using Scenes::Component::SpotLight;

				const auto & block = light->publishedBlock(readStateIndex);
				auto & entry = gpuData[m_lights.size()];

				writeCommonTerms< SpotLight::PositionOffset, SpotLight::ColorOffset, SpotLight::IntensityOffset, SpotLight::RadiusOffset >(block, viewMatrix, entry);

				/* The direction is a vector: rotated, not translated, then normalised (the view matrix may carry a scale). */
				const Vector< 4, float > worldDirection{block[SpotLight::DirectionOffset + 0], block[SpotLight::DirectionOffset + 1], block[SpotLight::DirectionOffset + 2], 0.0F};
				const auto viewDirection4 = viewMatrix * worldDirection;
				auto viewDirection = Vector< 3, float >{viewDirection4[X], viewDirection4[Y], viewDirection4[Z]};
				viewDirection.normalize();

				entry.directionType[0] = viewDirection[X];
				entry.directionType[1] = viewDirection[Y];
				entry.directionType[2] = viewDirection[Z];
				entry.directionType[3] = 2.0F;
				entry.cone[0] = block[SpotLight::InnerCosAngleOffset];
				entry.cone[1] = block[SpotLight::OuterCosAngleOffset];
				entry.cone[2] = 0.0F;
				entry.cone[3] = 0.0F;

				m_lights.emplace_back(light.get());
			}
		}

		lightBuffer.unmapMemory();

		if ( m_lights.empty() )
		{
			return false;
		}

		/* The pipeline only for a frame that has something to resolve (a scene without lamp never compiles it). */
		if ( (m_pipeline == nullptr || m_targetColorView.lock() != sceneTarget.colorImageView()) && !this->createTargetResources(sceneTarget) )
		{
			m_lights.clear();

			return false;
		}

		std::ranges::sort(m_lights);

		return true;
	}

	bool
	DeferredLightResolve::record (const CommandBuffer & commandBuffer, const SceneRenderTarget & sceneTarget, uint32_t readStateIndex, const ViewMatricesInterface & viewMatrices) noexcept
	{
		/* prepare() made every resource for this target; a mismatch here is a call-order defect. */
		if ( m_lights.empty() || m_pipeline == nullptr || m_targetColorView.lock() != sceneTarget.colorImageView() )
		{
			return false;
		}

		const auto & normalsImage = *sceneTarget.normalsImage();
		const auto & materialPropertiesImage = *sceneTarget.materialPropertiesImage();
		const auto & albedoImage = *sceneTarget.albedoImage();
		const auto & depthImage = *sceneTarget.depthStencilImage();

		const auto & descriptorSet = *m_descriptorSets[m_renderer.currentFrameIndex() % m_descriptorSets.size()];

		static_cast< void >(descriptorSet.writeCombinedImageSampler(0, *sceneTarget.normalsImageView(), *m_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		static_cast< void >(descriptorSet.writeCombinedImageSampler(1, *sceneTarget.materialPropertiesImageView(), *m_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		static_cast< void >(descriptorSet.writeCombinedImageSampler(2, *sceneTarget.albedoImageView(), *m_sampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
		static_cast< void >(descriptorSet.writeCombinedImageSampler(3, *sceneTarget.depthImageView(), *m_sampler, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL));

		/* G-buffer: attachments → read-only, in one batch. */
		{
			const std::array< VkImageMemoryBarrier, 4 > barriers{
				Sync::ImageMemoryBarrier{normalsImage, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}.get(),
				Sync::ImageMemoryBarrier{materialPropertiesImage, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}.get(),
				Sync::ImageMemoryBarrier{albedoImage, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}.get(),
				Sync::ImageMemoryBarrier{depthImage, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT}.get()
			};

			commandBuffer.pipelineBarrier(barriers, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		const auto & extent = sceneTarget.extent();
		const auto inverseProjection = viewMatrices.projectionMatrix(readStateIndex).inverse();

		ResolvePushConstants pushConstants{};
		std::copy_n(inverseProjection.data(), pushConstants.inverseProjection.size(), pushConstants.inverseProjection.begin());
		pushConstants.inverseExtentX = 1.0F / static_cast< float >(extent.width);
		pushConstants.inverseExtentY = 1.0F / static_cast< float >(extent.height);
		pushConstants.lightCount = static_cast< uint32_t >(m_lights.size());
		pushConstants.padding = 0;

		const std::array< VkClearValue, 1 > clearValues{};

		commandBuffer.beginRenderPass(*m_framebuffer, sceneTarget.renderArea(), std::span< const VkClearValue >{clearValues.data(), clearValues.size()}, VK_SUBPASS_CONTENTS_INLINE);

		commandBuffer.bind(*m_pipeline);

		const VkViewport viewport{
			.x = 0.0F,
			.y = 0.0F,
			.width = static_cast< float >(extent.width),
			.height = static_cast< float >(extent.height),
			.minDepth = 0.0F,
			.maxDepth = 1.0F
		};
		vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

		const VkRect2D scissor = sceneTarget.renderArea();
		vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

		vkCmdPushConstants(commandBuffer.handle(), m_pipelineLayout->handle(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

		commandBuffer.bind(descriptorSet, *m_pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

		commandBuffer.draw(3, 1);

		commandBuffer.endRenderPass();

		/* G-buffer: read-only → attachments, for the translucent half (LOAD), in one batch. The resume pass carries no
		 * subpass dependency (it must stay render-pass compatible with the CLEAR pass), so the global memory barrier also
		 * publishes what this barrier batch does not transition — the colour, the velocity, the reactive mask written by
		 * the opaque half and the resolve — to the attachment reads and writes of the translucent half. */
		{
			const std::array< VkMemoryBarrier, 1 > memoryBarriers{
				VkMemoryBarrier{
					.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
					.pNext = nullptr,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
				}
			};

			const std::array< VkImageMemoryBarrier, 4 > barriers{
				Sync::ImageMemoryBarrier{normalsImage, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}.get(),
				Sync::ImageMemoryBarrier{materialPropertiesImage, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}.get(),
				Sync::ImageMemoryBarrier{albedoImage, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}.get(),
				Sync::ImageMemoryBarrier{depthImage, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT}.get()
			};

			commandBuffer.pipelineBarrier(memoryBarriers, {}, barriers, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
		}

		return true;
	}

	void
	DeferredLightResolve::destroy () noexcept
	{
		m_lights.clear();
		m_pipeline.reset();
		m_framebuffer.reset();
		m_renderPass.reset();
		m_targetColorView.reset();
		m_pipelineLayout.reset();
		m_descriptorSets.clear();
		m_lightBuffers.clear();
		m_descriptorSetLayout.reset();
		m_sampler.reset();

		m_sharedResourcesCreated = false;
	}
}
