/*
 * src/Graphics/Effects/Lighting/LightGlints.cpp
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

#include "LightGlints.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/Effects/Shared/ViewReconstructionGLSL.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"
#include "Mutex.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Saphir/Types.hpp"
#include "Scenes/Component/AbstractLightEmitter.hpp"
#include "Scenes/Component/PointLight.hpp"
#include "Scenes/Component/SpotLight.hpp"
#include "Scenes/LightSet.hpp"
#include "StaticVector.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DeferredDestructor.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/MemoryRegion.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/ShaderStorageBufferObject.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class ShaderModule;
}

namespace
{
	using namespace EmEn;

	/* The descriptor set of the glint pass: the glints for the VERTEX stage, the depth and the view for the FRAGMENT
	 * stage (the occlusion test, where the chain's barriers make the depth readable). */
	constexpr uint32_t GlintBufferBinding{0};
	constexpr uint32_t DepthBinding{1};
	constexpr uint32_t ViewBinding{2};

	/* The least capacity of the glint buffers. */
	constexpr uint32_t MinCapacity{256};

	/* The push constant block, shared by the two stages. */
	constexpr auto PushConstantsGLSL = R"GLSL(
layout(push_constant) uniform PushConstants
{
	mat4 viewProjection;
	vec4 viewport;
	vec4 footprint;
	vec4 occlusion;
} pc;

)GLSL";

	/* The VERTEX stage: one instance per glint, six vertices per quad. A glint that is not drawn (behind the camera,
	 * off the screen, a spot's face seen from behind) collapses its quad outside the clip volume. */
	constexpr auto VertexMain = R"GLSL(
layout(location = 0) out vec2 vOffset;
layout(location = 1) flat out vec3 vPeak;
layout(location = 2) flat out float vSigma;
layout(location = 3) flat out vec2 vCentre;
layout(location = 4) flat out float vDistance;

const vec2 Corners[6] = vec2[6](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

void main ()
{
	vOffset = vec2(0.0);
	vPeak = vec3(0.0);
	vSigma = 1.0;
	vCentre = vec2(0.0);
	vDistance = 0.0;
	gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);

	const GlintData glint = glints[gl_InstanceIndex];
	const vec3 relative = glint.position.xyz;
	const float distance = length(relative);

	if ( distance < 1e-3 )
	{
		return;
	}

	/* A spot's face shows I cos(alpha) toward the camera and its area × cos(alpha); a point light's sphere shows I and
	 * a quarter of its surface (its disc). */
	float facing = 1.0;
	float projectedArea = 0.25 * glint.position.w;

	if ( glint.axis.w > 0.5 )
	{
		facing = dot(glint.axis.xyz, -relative / distance);

		if ( facing <= 0.0 )
		{
			return;
		}

		projectedArea = glint.position.w * facing;
	}

	const vec4 clip = pc.viewProjection * vec4(relative, 1.0);

	if ( clip.w <= 1e-6 )
	{
		return;
	}

	const vec2 ndc = clip.xy / clip.w;
	/* The source's side in pixels (the axis pixel subtends 2 tan(fovY / 2) / height radians). */
	const float sidePixels = sqrt(projectedArea) / distance * pc.viewport.y / (2.0 * pc.viewport.w);
	const float sigma = clamp(FootprintPerSourcePixel * sidePixels, pc.footprint.x, pc.footprint.z);
	const float radius = sigma * pc.footprint.y;
	const vec2 pixelToNdc = 2.0 / pc.viewport.xy;

	if ( any(greaterThan(abs(ndc), vec2(1.0) + (radius * pixelToNdc))) )
	{
		return;
	}

	/* The illuminance at the camera, E = I / d² (lux), spread by a unit Gaussian over the pixels' solid angles: the
	 * stars' photometry (StarField). In chain units (× the pre-exposure), bounded under a half float's range on the
	 * brightest channel (the hue is kept). */
	const vec3 illuminance = glint.intensity.rgb * facing / (distance * distance);
	const vec2 tangent = ndc * pc.viewport.zw;
	const float cosTheta = inversesqrt(1.0 + dot(tangent, tangent));
	const float pixelSolidAngle = (2.0 * pc.viewport.z / pc.viewport.x) * (2.0 * pc.viewport.w / pc.viewport.y) * (cosTheta * cosTheta * cosTheta);
	vec3 peak = illuminance * pc.footprint.w / (6.28318530718 * sigma * sigma * max(pixelSolidAngle, 1e-20));
	const float brightest = max(max(peak.r, peak.g), peak.b);

	if ( brightest > MaxChainRadiance )
	{
		peak *= MaxChainRadiance / brightest;
	}

	const vec2 corner = Corners[gl_VertexIndex];

	vOffset = corner * radius;
	vPeak = peak;
	vSigma = sigma;
	vCentre = (ndc * 0.5) + 0.5;
	vDistance = distance;
	gl_Position = vec4(ndc + (corner * radius * pixelToNdc), 0.0, 1.0);
}
)GLSL";

	/* The FRAGMENT stage: the footprint at the pixel centre, in pixels from the glint, × the visible share of the
	 * glint's centre and its four neighbours (a surface nearer than the glint, beyond the bias, hides it). */
	constexpr auto FragmentMain = R"GLSL(
layout(location = 0) in vec2 vOffset;
layout(location = 1) flat in vec3 vPeak;
layout(location = 2) flat in float vSigma;
layout(location = 3) flat in vec2 vCentre;
layout(location = 4) flat in float vDistance;
layout(location = 0) out vec4 outColor;

const vec2 Taps[5] = vec2[5](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(-1.0, 0.0), vec2(0.0, 1.0), vec2(0.0, -1.0));

void main ()
{
	const float hiddenBeyond = (vDistance * (1.0 - pc.occlusion.y)) - pc.occlusion.x;
	float visible = 0.0;

	for ( int index = 0; index < 5; ++index )
	{
		const vec2 uv = clamp(vCentre + (Taps[index] / pc.viewport.xy), vec2(0.0), vec2(1.0));
		const float depth = texture(depthTexture, uv).r;

		/* The sky (the clear value, 1.0) hides nothing. */
		if ( depth >= 1.0 || emViewDistance(uv, depth) >= hiddenBeyond )
		{
			visible += 0.2;
		}
	}

	outColor = vec4(vPeak * visible * exp(-dot(vOffset, vOffset) / (2.0 * vSigma * vSigma)), 0.0);
}
)GLSL";

	/**
	 * @brief Returns the GLSL constants both stages share.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string
	constantsGLSL ()
	{
		using Graphics::Effects::Lighting::LightGlints;

		std::string source{"#version 450\n\n"};

		source += "const float MaxChainRadiance = " + std::to_string(LightGlints::MaxChainRadiance) + ";\n";
		source += "const float FootprintPerSourcePixel = " + std::to_string(LightGlints::FootprintPerSourcePixel) + ";\n\n";

		return source;
	}

	/**
	 * @brief Returns the vertex shader's source: its inputs, then its body.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string
	vertexShaderSource ()
	{
		std::string source = constantsGLSL();

		source += "struct GlintData\n{\n\tvec4 position;\n\tvec4 axis;\n\tvec4 intensity;\n};\n\n";
		source += "layout(std430, set = 0, binding = " + std::to_string(GlintBufferBinding) + ") readonly buffer GlintBuffer\n{\n\tGlintData glints[];\n};\n\n";
		source += PushConstantsGLSL;
		source += VertexMain;

		return source;
	}

	/**
	 * @brief Returns the fragment shader's source: the depth, the view reconstruction, then its body.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string
	fragmentShaderSource ()
	{
		std::string source = constantsGLSL();

		source += "layout(set = 0, binding = " + std::to_string(DepthBinding) + ") uniform sampler2D depthTexture;\n\n";
		source += Graphics::ViewReconstructionGLSL::sourceGLSL(0, ViewBinding);
		source += PushConstantsGLSL;
		source += FragmentMain;

		return source;
	}

	/**
	 * @brief Builds a glint from a light's published block: its position relative to the camera, its axis, I × the
	 * colour. The offsets are the light type's (PointLight / SpotLight), compile-time.
	 * @param block A reference to the published block.
	 * @param area The source's area, m².
	 * @param axis The spot's unit direction and 1, or zeros and 0 for a point light.
	 * @param camera The camera's world position.
	 * @return std::optional< Graphics::Effects::Lighting::LightGlints::GPUGlint > Nothing for a non-positive or
	 * non-finite intensity.
	 */
	template< size_t positionOffset, size_t colorOffset, size_t intensityOffset >
	[[nodiscard]]
	std::optional< Graphics::Effects::Lighting::LightGlints::GPUGlint >
	makeGlint (const std::array< float, Scenes::Component::AbstractLightEmitter::MaxUniformBlockElementCount > & block, float area, const std::array< float, 4 > & axis, const std::array< float, 3 > & camera) noexcept
	{
		static_assert(positionOffset + 2 < Scenes::Component::AbstractLightEmitter::MaxUniformBlockElementCount && colorOffset + 2 < Scenes::Component::AbstractLightEmitter::MaxUniformBlockElementCount, "Offset out of the published block.");
		static_assert(intensityOffset < Scenes::Component::AbstractLightEmitter::MaxUniformBlockElementCount, "Offset out of the published block.");

		const auto intensity = std::get< intensityOffset >(block);

		if ( !std::isfinite(intensity) || intensity <= 0.0F )
		{
			return std::nullopt;
		}

		return Graphics::Effects::Lighting::LightGlints::GPUGlint{
			.position = {std::get< positionOffset + 0 >(block) - camera[0], std::get< positionOffset + 1 >(block) - camera[1], std::get< positionOffset + 2 >(block) - camera[2], area},
			.axis = axis,
			.intensity = {std::get< colorOffset + 0 >(block) * intensity, std::get< colorOffset + 1 >(block) * intensity, std::get< colorOffset + 2 >(block) * intensity, 0.0F}
		};
	}

	/**
	 * @brief Returns the target format: an RGB float format the device blends into, the 32-bit packed one first (the
	 * stars' choice: a full-resolution target, half the memory of RGBA16F).
	 * @param renderer A reference to the renderer.
	 * @return VkFormat
	 */
	[[nodiscard]]
	VkFormat
	targetFormat (const Graphics::Renderer & renderer) noexcept
	{
		constexpr VkFormatFeatureFlags required = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
		const auto properties = renderer.device()->physicalDevice()->getFormatProperties(VK_FORMAT_B10G11R11_UFLOAT_PACK32);

		return (properties.optimalTilingFeatures & required) == required ? VK_FORMAT_B10G11R11_UFLOAT_PACK32 : VK_FORMAT_R16G16B16A16_SFLOAT;
	}
}

namespace EmEn::Graphics::Effects::Lighting
{
	using namespace Base;
	using namespace Vulkan;

	bool
	LightGlints::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		if ( !m_target.create(renderer, width, height, targetFormat(renderer), "LG_Glints") )
		{
			TraceError{ClassId} << "Failed to create the glint target !";

			return false;
		}

		auto & layoutManager = renderer.layoutManager();

		m_inputLayout = layoutManager.getDescriptorSetLayout(ClassId);

		if ( m_inputLayout == nullptr )
		{
			auto layout = layoutManager.prepareNewDescriptorSetLayout(ClassId);
			layout->setIdentifier(ClassId, "Inputs", "DescriptorSetLayout");
			layout->declareStorageBuffer(GlintBufferBinding, VK_SHADER_STAGE_VERTEX_BIT);
			layout->declareCombinedImageSampler(DepthBinding, VK_SHADER_STAGE_FRAGMENT_BIT);
			layout->declareUniformBuffer(ViewBinding, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(layout) )
			{
				TraceError{ClassId} << "Unable to create the descriptor set layout !";

				return false;
			}

			m_inputLayout = layoutManager.getDescriptorSetLayout(ClassId);

			if ( m_inputLayout == nullptr )
			{
				return false;
			}
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(m_inputLayout);

			m_pipelineLayout = layoutManager.getPipelineLayout(sets, {
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants)}
			});
		}

		if ( m_pipelineLayout == nullptr )
		{
			return false;
		}

		auto & shaderManager = renderer.shaderManager();
		const auto & device = renderer.device();

		const auto vertexModule = shaderManager.getShaderModuleFromSourceCode(device, "LG_Glints_VS", Saphir::ShaderType::VertexShader, vertexShaderSource());
		const auto fragmentModule = shaderManager.getShaderModuleFromSourceCode(device, "LG_Glints_FS", Saphir::ShaderType::FragmentShader, fragmentShaderSource());

		if ( vertexModule == nullptr || fragmentModule == nullptr )
		{
			TraceError{ClassId} << "Failed to compile the glint shaders !";

			return false;
		}

		auto pipeline = std::make_shared< GraphicsPipeline >(device);
		pipeline->setIdentifier(ClassId, "LG_Glints", "GraphicsPipeline");

		StaticVector< std::shared_ptr< ShaderModule >, 5 > shaderModules;
		shaderModules.emplace_back(vertexModule);
		shaderModules.emplace_back(fragmentModule);

		/* No vertex input: every vertex is pulled from the glint buffer by gl_InstanceIndex. */
		if ( !pipeline->configureShaderStages(shaderModules) || !pipeline->configureEmptyVertexInputState() || !pipeline->configureInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) )
		{
			return false;
		}

		StaticVector< VkDynamicState, 16 > dynamicStates;
		dynamicStates.emplace_back(VK_DYNAMIC_STATE_VIEWPORT);
		dynamicStates.emplace_back(VK_DYNAMIC_STATE_SCISSOR);

		if ( !pipeline->configureDynamicStates(dynamicStates) || !pipeline->configureViewportState(width, height) )
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

		/* ADDITIVE: overlapping footprints add their light. */
		StaticVector< VkPipelineColorBlendAttachmentState, 8 > attachments;
		attachments.emplace_back(VkPipelineColorBlendAttachmentState{
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		});

		VkPipelineColorBlendStateCreateInfo colorBlend{};
		colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlend.logicOpEnable = VK_FALSE;

		if ( !pipeline->configureColorBlendState(attachments, colorBlend) || !pipeline->finalize(m_target.framebuffer().renderPass(), m_pipelineLayout, false, false) )
		{
			TraceError{ClassId} << "Unable to create the glint pipeline !";

			return false;
		}

		m_pipeline = std::move(pipeline);

		m_perFrame = this->createPerFrameDescriptorSets(m_inputLayout, ClassId, "LG_DescSet");

		return !m_perFrame.empty();
	}

	void
	LightGlints::destroy () noexcept
	{
		m_perFrame.clear();
		m_pipeline.reset();
		m_pipelineLayout.reset();
		m_inputLayout.reset();
		m_glintBuffers.clear();
		m_glints.clear();
		m_capacity = 0;
		m_target.destroy();
	}

	void
	LightGlints::gatherGlints (const Scenes::LightSet & lightSet, uint32_t readStateIndex, const std::array< float, 3 > & cameraPosition) noexcept
	{
		using Scenes::Component::PointLight;
		using Scenes::Component::SpotLight;

		m_glints.clear();

		uint32_t skipped = 0;

		/* Appends a glint within the ceiling. */
		const auto append = [this, &skipped] (const std::optional< GPUGlint > & glint) {
			if ( !glint.has_value() )
			{
				return;
			}

			if ( m_glints.size() >= MaxGlints )
			{
				++skipped;

				return;
			}

			m_glints.emplace_back(glint.value());
		};

		{
			const MutexLock lock{lightSet.mutex()};

			for ( const auto & light : lightSet.pointLights() )
			{
				if ( const auto area = light->publishedVisibleSourceArea(readStateIndex); area > 0.0F && light->isEnabled() )
				{
					append(makeGlint< PointLight::PositionOffset, PointLight::ColorOffset, PointLight::IntensityOffset >(light->publishedBlock(readStateIndex), area, {0.0F, 0.0F, 0.0F, 0.0F}, cameraPosition));
				}
			}

			for ( const auto & light : lightSet.spotLights() )
			{
				if ( const auto area = light->publishedVisibleSourceArea(readStateIndex); area > 0.0F && light->isEnabled() )
				{
					const auto & block = light->publishedBlock(readStateIndex);
					Math::Vector< 3, float > direction{std::get< SpotLight::DirectionOffset + 0 >(block), std::get< SpotLight::DirectionOffset + 1 >(block), std::get< SpotLight::DirectionOffset + 2 >(block)};

					/* A spot without a direction shows no face. */
					if ( direction.lengthSquared() <= 0.0F )
					{
						continue;
					}

					direction.normalize();

					append(makeGlint< SpotLight::PositionOffset, SpotLight::ColorOffset, SpotLight::IntensityOffset >(block, area, {direction[Math::X], direction[Math::Y], direction[Math::Z], 1.0F}, cameraPosition));
				}
			}
		}

		/* Never a silent drop: said once each time the ceiling starts to cut. */
		if ( skipped > 0 && !m_overflowReported )
		{
			TraceWarning{ClassId} << skipped << " light glints over the " << MaxGlints << " a frame draws are skipped.";
		}

		m_overflowReported = skipped > 0;
	}

	bool
	LightGlints::uploadGlints (uint32_t frameIndex) noexcept
	{
		const auto count = static_cast< uint32_t >(m_glints.size());

		if ( count > m_capacity || m_glintBuffers.empty() )
		{
			/* count <= MaxGlints (gatherGlints()), a power of two: bit_ceil cannot overflow. */
			const auto capacity = std::max(MinCapacity, std::bit_ceil(count));
			const auto frameCount = this->renderer().framesInFlight();

			std::vector< std::unique_ptr< ShaderStorageBufferObject > > buffers;
			buffers.reserve(frameCount);

			for ( uint32_t frame = 0; frame < frameCount; ++frame )
			{
				auto buffer = std::make_unique< ShaderStorageBufferObject >(this->renderer().device(), static_cast< VkDeviceSize >(capacity) * sizeof(GPUGlint));
				buffer->setIdentifier(ClassId, "Glints-F" + std::to_string(frame), "ShaderStorageBufferObject");

				if ( !buffer->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create a buffer of " << capacity << " glints !";

					return false;
				}

				buffers.emplace_back(std::move(buffer));
			}

			/* The frames in flight still read the previous buffers. */
			for ( auto & previous : m_glintBuffers )
			{
				this->renderer().deferredDestructor().retireObject(std::move(previous));
			}

			m_glintBuffers = std::move(buffers);
			m_capacity = capacity;
		}

		if ( frameIndex >= m_glintBuffers.size() )
		{
			return false;
		}

		return m_glints.empty() || m_glintBuffers[frameIndex]->writeData(MemoryRegion{m_glints.data(), m_glints.size() * sizeof(GPUGlint)});
	}

	void
	LightGlints::recordGlintPass (const CommandBuffer & commandBuffer, const DescriptorSet * descriptorSet, const PushConstants & pushConstants) const noexcept
	{
		const auto width = m_target.width();
		const auto height = m_target.height();

		/* The pass loads DONT_CARE: no clear value is read. */
		constexpr std::array< VkClearValue, 1 > clearValues{
			VkClearValue{.color = {{0.0F, 0.0F, 0.0F, 0.0F}}}
		};

		commandBuffer.beginRenderPass(m_target.framebuffer(), VkRect2D{.offset = {0, 0}, .extent = {width, height}}, clearValues, VK_SUBPASS_CONTENTS_INLINE);

		/* The render pass loads DONT_CARE: the target is cleared here, every frame, glints or not. */
		const VkClearAttachment clear{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.colorAttachment = 0,
			.clearValue = VkClearValue{.color = {{0.0F, 0.0F, 0.0F, 0.0F}}}
		};
		const VkClearRect rect{
			.rect = VkRect2D{.offset = {0, 0}, .extent = {width, height}},
			.baseArrayLayer = 0,
			.layerCount = 1
		};

		vkCmdClearAttachments(commandBuffer.handle(), 1, &clear, 1, &rect);

		if ( descriptorSet != nullptr && !m_glints.empty() )
		{
			commandBuffer.bind(*m_pipeline);

			const VkViewport viewport{
				.x = 0.0F,
				.y = 0.0F,
				.width = static_cast< float >(width),
				.height = static_cast< float >(height),
				.minDepth = 0.0F,
				.maxDepth = 1.0F
			};
			vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

			const VkRect2D scissor{.offset = {0, 0}, .extent = {width, height}};
			vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);

			vkCmdPushConstants(commandBuffer.handle(), m_pipelineLayout->handle(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);

			commandBuffer.bind(*descriptorSet, *m_pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

			commandBuffer.draw(6, static_cast< uint32_t >(m_glints.size()));
		}

		commandBuffer.endRenderPass();

		/* The combine samples the target next (the explicit fence MoltenVK needs, recordFullscreenPass()). */
		const Sync::ImageMemoryBarrier barrier{
			*m_target.image(),
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	}

	void
	LightGlints::recordOverlayPasses (const CommandBuffer & commandBuffer, const TextureInterface & /*inputColor*/, const FrameContext & context) noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();
		const auto readStateIndex = this->renderer().currentReadStateIndex();
		const auto mainTarget = this->renderer().mainRenderTarget();

		PushConstants pushConstants{};
		const DescriptorSet * descriptorSet = nullptr;

		m_glints.clear();

		if ( context.lightSet != nullptr && context.depth != nullptr && context.view != nullptr && mainTarget != nullptr && frameIndex < m_perFrame.size() )
		{
			const auto & viewMatrices = mainTarget->viewMatrices();
			const auto & camera = viewMatrices.position(readStateIndex);

			this->gatherGlints(*context.lightSet, readStateIndex, {camera[Math::X], camera[Math::Y], camera[Math::Z]});

			if ( !m_glints.empty() )
			{
				auto & set = *m_perFrame[frameIndex];

				const bool written =
					this->uploadGlints(frameIndex) &&
					set.writeStorageBuffer(GlintBufferBinding, VkDescriptorBufferInfo{.buffer = m_glintBuffers[frameIndex]->handle(), .offset = 0, .range = VK_WHOLE_SIZE}) &&
					set.writeCombinedImageSampler(DepthBinding, *context.depth) &&
					set.writeUniformBufferObject(ViewBinding, *context.view);

				if ( written )
				{
					/* ⚠️ The UNJITTERED projection (after the temporal resolve, a glint sits still) and the view ROTATION:
					 * the glints are camera-relative. */
					const auto & projection = viewMatrices.unjitteredProjectionMatrix(readStateIndex);
					const auto viewProjection = projection * viewMatrices.viewMatrix(readStateIndex, true, 0);

					std::copy_n(viewProjection.data(), pushConstants.viewProjection.size(), pushConstants.viewProjection.begin());

					pushConstants.viewport = {
						static_cast< float >(m_target.width()),
						static_cast< float >(m_target.height()),
						1.0F / std::max(std::abs(projection(0, 0)), 1.0e-6F),
						1.0F / std::max(std::abs(projection(1, 1)), 1.0e-6F)
					};
					pushConstants.footprint = {FootprintSigma, FootprintExtent, MaxFootprintSigma, context.preExposure};
					pushConstants.occlusion = {OcclusionBias, OcclusionRelativeBias, 0.0F, 0.0F};

					descriptorSet = &set;
				}
				else
				{
					TraceError{ClassId} << "Unable to bind the inputs of the light glints !";
				}
			}
		}

		this->recordGlintPass(commandBuffer, descriptorSet, pushConstants);
	}

	IndirectPostProcessEffect::CombineContribution
	LightGlints::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		CombineContribution contribution;
		contribution.prefix = "glints";
		contribution.samplers.emplace_back(CombineSamplerInput{.nameSuffix = "Tex", .texture = &m_target});
		/* Everywhere: the glint pass already hid what the scene hides. */
		contribution.code = "\tem_Color.rgb += texture(glintsTex, vUV).rgb;\n";

		return contribution;
	}
}
