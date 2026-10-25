/*
 * src/Graphics/Effects/Atmosphere/StarField.cpp
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

#include "StarField.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <span>
#include <string>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/Effects/Shared/AtmosphereGLSL.hpp"
#include "Graphics/Photometry.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/SkyAtmosphere.hpp"
#include "Graphics/SkyStarField.hpp"
#include "Graphics/StarCatalogueResource.hpp"
#include "Graphics/StorageTexture2D.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"
#include "PixelFactory/Types.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Saphir/Types.hpp"
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

	/* The descriptor set of the star pass: every binding is read by the VERTEX stage. */
	constexpr uint32_t StarBufferBinding{0};
	constexpr uint32_t TransmittanceBinding{1};
	constexpr uint32_t FrameBinding{2};
	constexpr uint32_t AtmosphereBinding{3};

	/* The push constant block, shared by the two stages. */
	constexpr auto PushConstantsGLSL = R"GLSL(
layout(push_constant) uniform PushConstants
{
	mat4 viewProjection;
	vec4 axisX;
	vec4 axisY;
	vec4 axisZ;
	vec4 viewport;
} pc;

)GLSL";

	/* The VERTEX stage: one instance per star, six vertices per quad. A star that is not drawn (behind the camera, off
	 * the screen, below the horizon, behind the star's or a moon's disc) collapses its quad outside the clip volume. */
	constexpr auto VertexMain = R"GLSL(
layout(location = 0) out vec2 vOffset;
layout(location = 1) flat out vec3 vPeak;
layout(location = 2) flat out float vSigma;

const vec2 Corners[6] = vec2[6](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

/* The frame texture (SkyAtmosphere::FrameTexels): texel 0 the star's direction and the cos of its angular radius, 2 the
 * moon count, 4 + 3i each moon's direction and the cos of its angular radius. */
bool occulted (vec3 direction)
{
	const vec4 star = texelFetch(frameData, ivec2(0, 0), 0);

	if ( dot(direction, star.xyz) > star.w )
	{
		return true;
	}

	const int moonCount = int(clamp(texelFetch(frameData, ivec2(2, 0), 0).x, 0.0, float(MaxMoons)));

	for ( int index = 0; index < moonCount; ++index )
	{
		const vec4 moon = texelFetch(frameData, ivec2(4 + (3 * index), 0), 0);

		if ( dot(direction, moon.xyz) > moon.w )
		{
			return true;
		}
	}

	return false;
}

void main ()
{
	vOffset = vec2(0.0);
	vPeak = vec3(0.0);
	vSigma = 1.0;
	gl_Position = vec4(-2.0, -2.0, 0.0, 1.0);

	const StarData star = stars[gl_InstanceIndex];

	/* An empty planet slot. */
	if ( max(max(star.illuminance.r, star.illuminance.g), star.illuminance.b) <= 0.0 )
	{
		return;
	}

	const vec3 equatorial = star.direction.xyz;
	const vec3 world = normalize((pc.axisX.xyz * equatorial.x) + (pc.axisY.xyz * equatorial.y) + (pc.axisZ.xyz * equatorial.z));
	const vec4 clip = pc.viewProjection * vec4(world, 0.0);

	if ( clip.w <= 1e-6 )
	{
		return;
	}

	const vec2 ndc = clip.xy / clip.w;
	/* A planet's footprint widens to its disc (σ = half its radius in pixels, on the axis): a stand-in until the disc
	 * is drawn; a star's stays the least sigma. */
	const float sigma = max(pc.axisX.w, 0.5 * star.direction.w * pc.viewport.y / (2.0 * pc.viewport.w));
	const float radius = sigma * pc.axisY.w;
	const vec2 pixelToNdc = 2.0 / pc.viewport.xy;

	if ( any(greaterThan(abs(ndc), vec2(1.0) + (radius * pixelToNdc))) )
	{
		return;
	}

	/* Through the atmosphere from the camera (the sky's own lookup: world Y is the zenith); a ray into the planet sees
	 * no star. */
	const float viewHeight = texelFetch(frameData, ivec2(1, 0), 0).w;
	const float zenithCos = clamp(world.y, -1.0, 1.0);

	if ( atmRaySphereNearest(vec3(0.0, 0.0, viewHeight), vec3(sqrt(max(1.0 - (zenithCos * zenithCos), 0.0)), 0.0, zenithCos), vec3(0.0), atmosphere.radii.x) >= 0.0 || occulted(world) )
	{
		return;
	}

	const vec3 illuminance = star.illuminance.rgb * textureLod(transmittanceLut, atmTransmittanceParametersToUv(viewHeight, zenithCos), 0.0).rgb;

	/* The pixel's solid angle at the star: the axis pixel (2 tan(fov / 2) / size, per axis) × cos³ of the angle off
	 * the axis (a pinhole's tangent plane). The footprint's peak radiance makes Σ L Ω over the pixels the illuminance:
	 * a unit Gaussian per pixel² peaks at 1 / (2π σ²). In chain units (× the pre-exposure), bounded under a half
	 * float's range on the brightest channel (the hue is kept). */
	const vec2 tangent = ndc * pc.viewport.zw;
	const float cosTheta = inversesqrt(1.0 + dot(tangent, tangent));
	const float pixelSolidAngle = (2.0 * pc.viewport.z / pc.viewport.x) * (2.0 * pc.viewport.w / pc.viewport.y) * (cosTheta * cosTheta * cosTheta);
	vec3 peak = illuminance * pc.axisZ.w / (6.28318530718 * sigma * sigma * max(pixelSolidAngle, 1e-20));
	const float brightest = max(max(peak.r, peak.g), peak.b);

	if ( brightest > MaxChainRadiance )
	{
		peak *= MaxChainRadiance / brightest;
	}

	const vec2 corner = Corners[gl_VertexIndex];

	vOffset = corner * radius;
	vPeak = peak;
	vSigma = sigma;
	gl_Position = vec4(ndc + (corner * radius * pixelToNdc), 0.0, 1.0);
}
)GLSL";

	/* The FRAGMENT stage: the footprint at the pixel centre, in pixels from the star. */
	constexpr auto FragmentMain = R"GLSL(
layout(location = 0) in vec2 vOffset;
layout(location = 1) flat in vec3 vPeak;
layout(location = 2) flat in float vSigma;
layout(location = 0) out vec4 outColor;

void main ()
{
	outColor = vec4(vPeak * exp(-dot(vOffset, vOffset) / (2.0 * vSigma * vSigma)), 0.0);
}
)GLSL";

	/**
	 * @brief Returns the vertex shader's source: its inputs, the atmosphere's block and functions, then its body.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string
	vertexShaderSource ()
	{
		using namespace Graphics;

		std::string source{"#version 450\n\n"};

		source += "const int MaxMoons = " + std::to_string(SkyFrameInputs::MaxMoons) + ";\n";
		source += "const float MaxChainRadiance = " + std::to_string(Effects::Atmosphere::StarField::MaxChainRadiance) + ";\n\n";
		source += "struct StarData\n{\n\tvec4 direction;\n\tvec4 illuminance;\n};\n\n";
		source += "layout(std430, set = 0, binding = " + std::to_string(StarBufferBinding) + ") readonly buffer StarBuffer\n{\n\tStarData stars[];\n};\n\n";
		source += "layout(set = 0, binding = " + std::to_string(TransmittanceBinding) + ") uniform sampler2D transmittanceLut;\n";
		source += "layout(set = 0, binding = " + std::to_string(FrameBinding) + ") uniform sampler2D frameData;\n\n";
		source += AtmosphereGLSL::uniformBlockGLSL(0, AtmosphereBinding);
		source += PushConstantsGLSL;
		source += AtmosphereGLSL::toGLSL(AtmosphereGLSL::RaySphereNearest);
		source += AtmosphereGLSL::toGLSL(AtmosphereGLSL::TransmittanceParametersToUv);
		source += VertexMain;

		return source;
	}

	/**
	 * @brief Returns the fragment shader's source.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string
	fragmentShaderSource ()
	{
		return std::string{"#version 450\n\n"} + PushConstantsGLSL + FragmentMain;
	}

	/**
	 * @brief Returns the target format: an RGB float format the device blends into, the 32-bit packed one first (a
	 * full-resolution target, half the memory of RGBA16F: 30 MB at 3840×1990).
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

namespace EmEn::Graphics::Effects::Atmosphere
{
	using namespace Base;
	using namespace Vulkan;

	bool
	StarField::create (uint32_t width, uint32_t height) noexcept
	{
		auto & renderer = this->renderer();

		if ( !m_target.create(renderer, width, height, targetFormat(renderer), "SF_Stars") )
		{
			TraceError{ClassId} << "Failed to create the star target !";

			return false;
		}

		auto & layoutManager = renderer.layoutManager();

		m_inputLayout = layoutManager.getDescriptorSetLayout(ClassId);

		if ( m_inputLayout == nullptr )
		{
			auto layout = layoutManager.prepareNewDescriptorSetLayout(ClassId);
			layout->setIdentifier(ClassId, "Inputs", "DescriptorSetLayout");
			layout->declareStorageBuffer(StarBufferBinding, VK_SHADER_STAGE_VERTEX_BIT);
			layout->declareCombinedImageSampler(TransmittanceBinding, VK_SHADER_STAGE_VERTEX_BIT);
			layout->declareCombinedImageSampler(FrameBinding, VK_SHADER_STAGE_VERTEX_BIT);
			layout->declareUniformBuffer(AtmosphereBinding, VK_SHADER_STAGE_VERTEX_BIT);

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

		const auto vertexModule = shaderManager.getShaderModuleFromSourceCode(device, "SF_Stars_VS", Saphir::ShaderType::VertexShader, vertexShaderSource());
		const auto fragmentModule = shaderManager.getShaderModuleFromSourceCode(device, "SF_Stars_FS", Saphir::ShaderType::FragmentShader, fragmentShaderSource());

		if ( vertexModule == nullptr || fragmentModule == nullptr )
		{
			TraceError{ClassId} << "Failed to compile the star shaders !";

			return false;
		}

		auto pipeline = std::make_shared< GraphicsPipeline >(device);
		pipeline->setIdentifier(ClassId, "SF_Stars", "GraphicsPipeline");

		StaticVector< std::shared_ptr< ShaderModule >, 5 > shaderModules;
		shaderModules.emplace_back(vertexModule);
		shaderModules.emplace_back(fragmentModule);

		/* No vertex input: every vertex is pulled from the star buffer by gl_InstanceIndex. */
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
			TraceError{ClassId} << "Unable to create the star pipeline !";

			return false;
		}

		m_pipeline = std::move(pipeline);

		m_perFrame = this->createPerFrameDescriptorSets(m_inputLayout, ClassId, "SF_DescSet");

		return !m_perFrame.empty();
	}

	void
	StarField::destroy () noexcept
	{
		m_perFrame.clear();
		m_pipeline.reset();
		m_pipelineLayout.reset();
		m_inputLayout.reset();
		m_pointBuffers.clear();
		m_uploadedCatalogue.reset();
		m_starCount = 0;
		m_target.destroy();
	}

	bool
	StarField::uploadCatalogue (const std::shared_ptr< const StarCatalogueResource > & catalogue) noexcept
	{
		const auto & stars = catalogue->stars();

		std::vector< GPUStar > data;
		data.reserve(stars.size());

		std::ranges::transform(stars, std::back_inserter(data), [] (const StarCatalogueResource::Star & star) {
			/* The black body of the star's temperature at unit Rec.709 luminance, × its illuminance. */
			const auto linear = Photometry::linearColorFromTemperature(Photometry::temperatureFromColourIndex(star.colourIndex));
			const auto luminance = PixelFactory::Color< float >::computeLuminance(linear[0], linear[1], linear[2], PixelFactory::GrayscaleConversionMode::LumaRec709);
			const auto scale = luminance > 1.0e-6F ? Photometry::illuminanceFromMagnitude(star.visualMagnitude) / luminance : 0.0F;

			return GPUStar{
				.direction = {star.direction[Math::X], star.direction[Math::Y], star.direction[Math::Z], 0.0F},
				.illuminance = {linear[0] * scale, linear[1] * scale, linear[2] * scale, 0.0F}
			};
		});

		/* The catalogue refuses an empty star list nowhere: the renderer files no star field for one. */
		if ( data.empty() )
		{
			return false;
		}

		/* One buffer per frame in flight: the planets' slots (empty: zero illuminance) then the stars. */
		constexpr auto PlanetBytes = SkyStarField::MaxPlanets * sizeof(GPUStar);
		const std::array< GPUStar, SkyStarField::MaxPlanets > noPlanets{};
		const auto frameCount = this->renderer().framesInFlight();

		std::vector< std::unique_ptr< ShaderStorageBufferObject > > buffers;
		buffers.reserve(frameCount);

		for ( uint32_t frame = 0; frame < frameCount; ++frame )
		{
			auto buffer = std::make_unique< ShaderStorageBufferObject >(this->renderer().device(), PlanetBytes + (data.size() * sizeof(GPUStar)));
			buffer->setIdentifier(ClassId, catalogue->name() + "-F" + std::to_string(frame), "ShaderStorageBufferObject");

			if ( !buffer->createOnHardware() || !buffer->writeData(MemoryRegion{noPlanets.data(), PlanetBytes}) || !buffer->writeData(MemoryRegion{data.data(), data.size() * sizeof(GPUStar), PlanetBytes}) )
			{
				TraceError{ClassId} << "Unable to upload the " << data.size() << " stars of the catalogue '" << catalogue->name() << "' !";

				return false;
			}

			buffers.emplace_back(std::move(buffer));
		}

		/* The frames in flight still read the previous buffers. */
		for ( auto & previous : m_pointBuffers )
		{
			this->renderer().deferredDestructor().retireObject(std::move(previous));
		}

		m_pointBuffers = std::move(buffers);
		m_starCount = static_cast< uint32_t >(data.size());
		m_uploadedCatalogue = catalogue;

		TraceInfo{ClassId} << m_starCount << " stars of '" << catalogue->catalogueName() << "' uploaded (" << catalogue->credit() << ").";

		return true;
	}

	bool
	StarField::writePlanets (const ShaderStorageBufferObject & buffer, const SkyStarField & starField) noexcept
	{
		std::array< GPUStar, SkyStarField::MaxPlanets > planets{};
		const auto & frame = starField.equatorialFrame;

		/* planetCount <= MaxPlanets (the renderer's clamp). The shader turns the equatorial coordinates back into the
		 * world through the same axes: a world direction goes in as its projections on them. */
		std::ranges::transform(std::span{starField.planets}.first(std::min(starField.planetCount, SkyStarField::MaxPlanets)), planets.begin(), [&frame] (const SkyStarField::Point & planet) {
			return GPUStar{
				.direction = {
					Math::Vector< 3, float >::dotProduct(planet.direction, frame[0]),
					Math::Vector< 3, float >::dotProduct(planet.direction, frame[1]),
					Math::Vector< 3, float >::dotProduct(planet.direction, frame[2]),
					planet.angularRadius
				},
				.illuminance = {planet.illuminance[Math::X], planet.illuminance[Math::Y], planet.illuminance[Math::Z], 0.0F}
			};
		});

		return buffer.writeData(MemoryRegion{planets.data(), sizeof(planets)});
	}

	void
	StarField::recordStarPass (const CommandBuffer & commandBuffer, const DescriptorSet * descriptorSet, const PushConstants & pushConstants) const noexcept
	{
		const auto width = m_target.width();
		const auto height = m_target.height();

		/* The pass loads DONT_CARE: no clear value is read. */
		constexpr std::array< VkClearValue, 1 > clearValues{
			VkClearValue{.color = {{0.0F, 0.0F, 0.0F, 0.0F}}}
		};

		commandBuffer.beginRenderPass(m_target.framebuffer(), VkRect2D{.offset = {0, 0}, .extent = {width, height}}, clearValues, VK_SUBPASS_CONTENTS_INLINE);

		/* The render pass loads DONT_CARE: the target is cleared here, every frame, stars or not. */
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

		if ( descriptorSet != nullptr && m_starCount > 0 )
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

			commandBuffer.draw(6, SkyStarField::MaxPlanets + m_starCount);
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
	StarField::recordOverlayPasses (const CommandBuffer & commandBuffer, const TextureInterface & /*inputColor*/, const FrameContext & context) noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();

		/* No star field this frame (no catalogue, no clock, no submitted sky): the cleared target adds nothing. */
		const auto * starField = context.stars;
		const auto * atmosphere = context.atmosphere;

		PushConstants pushConstants{};
		const DescriptorSet * descriptorSet = nullptr;

		if ( starField != nullptr && starField->catalogue != nullptr && atmosphere != nullptr && atmosphere->parametersBuffer() != nullptr && frameIndex < m_perFrame.size() )
		{
			const bool uploaded = (m_uploadedCatalogue.lock() == starField->catalogue || this->uploadCatalogue(starField->catalogue)) && frameIndex < m_pointBuffers.size();

			auto & set = *m_perFrame[frameIndex];

			const bool written = uploaded &&
				StarField::writePlanets(*m_pointBuffers[frameIndex], *starField) &&
				set.writeStorageBuffer(StarBufferBinding, VkDescriptorBufferInfo{.buffer = m_pointBuffers[frameIndex]->handle(), .offset = 0, .range = VK_WHOLE_SIZE}) &&
				set.writeCombinedImageSampler(TransmittanceBinding, atmosphere->transmittance()) &&
				set.writeCombinedImageSampler(FrameBinding, atmosphere->frameData()) &&
				set.writeUniformBufferObject(AtmosphereBinding, *atmosphere->parametersBuffer());

			if ( written )
			{
				/* ⚠️ The UNJITTERED projection (after the temporal resolve, a star sits still) and the view ROTATION: a
				 * star is at infinity. The read state's matrices, the ones that produced the depth the combine masks by. */
				const auto readStateIndex = this->renderer().currentReadStateIndex();
				const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
				const auto & projection = viewMatrices.unjitteredProjectionMatrix(readStateIndex);
				const auto viewProjection = projection * viewMatrices.viewMatrix(readStateIndex, true, 0);
				const auto & frame = starField->equatorialFrame;

				std::copy_n(viewProjection.data(), pushConstants.viewProjection.size(), pushConstants.viewProjection.begin());

				pushConstants.axisX = {frame[0][Math::X], frame[0][Math::Y], frame[0][Math::Z], FootprintSigma};
				pushConstants.axisY = {frame[1][Math::X], frame[1][Math::Y], frame[1][Math::Z], FootprintExtent};
				pushConstants.axisZ = {frame[2][Math::X], frame[2][Math::Y], frame[2][Math::Z], context.preExposure};
				pushConstants.viewport = {
					static_cast< float >(m_target.width()),
					static_cast< float >(m_target.height()),
					1.0F / std::max(std::abs(projection(0, 0)), 1.0e-6F),
					1.0F / std::max(std::abs(projection(1, 1)), 1.0e-6F)
				};

				descriptorSet = &set;
			}
			else
			{
				TraceError{ClassId} << "Unable to bind the inputs of the star field !";
			}
		}

		this->recordStarPass(commandBuffer, descriptorSet, pushConstants);
	}

	IndirectPostProcessEffect::CombineContribution
	StarField::combineContribution (const FrameContext & /*context*/) const noexcept
	{
		CombineContribution contribution;
		contribution.prefix = "stars";
		contribution.needsDepth = true;
		contribution.samplers.emplace_back(CombineSamplerInput{.nameSuffix = "Tex", .texture = &m_target});

		/* ⚠️ The sky is the clear value, 1.0 exactly (the lens flare's probe, the aerial perspective). Behind a cloud a
		 * star is worth the cloud's transmittance: a second sampler, so a second combine variant (the signature counts
		 * the samplers). */
		if ( m_cloudTransmittance != nullptr )
		{
			contribution.samplers.emplace_back(CombineSamplerInput{.nameSuffix = "Clouds", .texture = m_cloudTransmittance});
			contribution.code =
				"\tif ( texture(emDepth, vUV).r >= 1.0 ) { em_Color.rgb += texture(starsTex, vUV).rgb * texture(starsClouds, vUV).r; }\n";
		}
		else
		{
			contribution.code =
				"\tif ( texture(emDepth, vUV).r >= 1.0 ) { em_Color.rgb += texture(starsTex, vUV).rgb; }\n";
		}

		return contribution;
	}
}
