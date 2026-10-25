/*
 * src/Graphics/Effects/Atmosphere/AerialPerspective.cpp
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

#include "AerialPerspective.hpp"

/* STL inclusions. */
#include <algorithm>
#include <string>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/SkyAtmosphere.hpp"
#include "Graphics/StorageTexture3D.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Math/Matrix.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Saphir/Types.hpp"
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

	/* ---- The application of the volume (Hillaire's RenderRaymarchingPS, its aerial perspective branch). ---- */
	constexpr auto FragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler2D materialPropsTex;
/* Graded absolute radiance (nits) and RGB transmittance, from the camera to each froxel's slice. */
layout(set = 0, binding = 3) uniform sampler3D inscatterVolume;
layout(set = 0, binding = 4) uniform sampler3D transmittanceVolume;

layout(push_constant) uniform PushConstants
{
	mat4 invRelativeViewProj;
	/* x: pre-exposure, y: km per slice, z: slice count. */
	vec4 parameters;
} pc;

void main ()
{
	vec3 sceneColor = texture(sceneTex, vUV).rgb;
	float depth = texture(depthTex, vUV).r;

	/* ⚠️ The sky is the clear value, 1.0 exactly: the drawn sky already is the whole atmosphere. */
	if ( depth >= 1.0 )
	{
		outColor = vec4(sceneColor, 1.0);

		return;
	}

	/* The distance from the camera, reconstructed through the matrix the froxels were placed with — never a scalar
	 * depth linearisation. */
	vec4 surface = pc.invRelativeViewProj * vec4(vUV * 2.0 - 1.0, depth, 1.0);
	float distanceKm = length(surface.xyz / surface.w) * 0.001;

	/* The slice, in slice units (0 to the slice count); under half a slice the first sampled one fades linearly to 0
	 * at the camera (the in-scattering of a thin path grows with its length). */
	float slice = distanceKm / pc.parameters.y;
	float weight = 1.0;

	if ( slice < 0.5 )
	{
		weight = clamp(slice * 2.0, 0.0, 1.0);
		slice = 0.5;
	}

	/* The fog response (alpha's high nibble): a surface immune to the fog is immune to the haze. */
	uint aPacked = uint(texture(materialPropsTex, vUV).a * 255.0);
	weight *= float(aPacked >> 4u) / 15.0;

	/* The squared slice distribution, inverted. */
	vec3 uvw = vec3(vUV, sqrt(slice / pc.parameters.z));
	vec3 inscatter = textureLod(inscatterVolume, uvw, 0.0).rgb * weight;
	vec3 transmittance = 1.0 - (weight * (1.0 - textureLod(transmittanceVolume, uvw, 0.0).rgb));

	outColor = vec4((sceneColor * transmittance) + (inscatter * pc.parameters.x), 1.0);
}
)GLSL";
}

namespace EmEn::Graphics::Effects::Atmosphere
{
	using namespace Base;
	using namespace Vulkan;

	bool
	AerialPerspective::create (uint32_t width, uint32_t height) noexcept
	{
		if ( !m_outputTarget.create(this->renderer(), width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "AP_Output") )
		{
			TraceError{ClassId} << "Failed to create the output target !";

			return false;
		}

		/* Scene colour, depth, material properties, the in-scattering and the transmittance volumes. */
		auto inputLayout = this->getInputLayout(5);

		if ( inputLayout == nullptr )
		{
			return false;
		}

		{
			const StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants)}
			};

			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(inputLayout);
			m_pipelineLayout = this->renderer().layoutManager().getPipelineLayout(sets, ranges);
		}

		if ( m_pipelineLayout == nullptr )
		{
			return false;
		}

		const auto vertexModule = this->getFullscreenVertexShader();

		if ( vertexModule == nullptr )
		{
			TraceError{ClassId} << "Failed to compile the vertex shader !";

			return false;
		}

		const auto fragmentModule = this->renderer().shaderManager().getShaderModuleFromSourceCode(this->renderer().device(), "AP_Apply_FS", Saphir::ShaderType::FragmentShader, FragmentShader);

		if ( fragmentModule == nullptr )
		{
			TraceError{ClassId} << "Failed to compile the fragment shader !";

			return false;
		}

		m_pipeline = this->createFullscreenPipeline(ClassId, "AP_Apply", vertexModule, fragmentModule, m_pipelineLayout, m_outputTarget);

		if ( m_pipeline == nullptr )
		{
			return false;
		}

		m_perFrame = this->createPerFrameDescriptorSets(inputLayout, ClassId, "APDescSet");

		return !m_perFrame.empty();
	}

	void
	AerialPerspective::destroy () noexcept
	{
		m_perFrame.clear();
		m_pipeline.reset();
		m_pipelineLayout.reset();
		m_outputTarget.destroy();
	}

	const TextureInterface &
	AerialPerspective::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		/* No atmosphere submitted this frame (no physical sky, or its submission failed): its volume holds nothing of
		 * this frame. */
		const auto * atmosphere = context.atmosphere;

		if ( atmosphere == nullptr || context.depth == nullptr || context.materialProperties == nullptr )
		{
			return inputColor;
		}

		const auto frameIndex = this->renderer().currentFrameIndex();

		if ( frameIndex >= m_perFrame.size() )
		{
			return inputColor;
		}

		auto & descriptorSet = *m_perFrame[frameIndex];

		const bool written =
			descriptorSet.writeCombinedImageSampler(0, inputColor) &&
			descriptorSet.writeCombinedImageSampler(1, *context.depth) &&
			descriptorSet.writeCombinedImageSampler(2, *context.materialProperties) &&
			descriptorSet.writeCombinedImageSampler(3, atmosphere->aerialPerspectiveInscatter()) &&
			descriptorSet.writeCombinedImageSampler(4, atmosphere->aerialPerspectiveTransmittance());

		if ( !written )
		{
			TraceError{ClassId} << "Unable to bind the inputs of the aerial perspective !";

			return inputColor;
		}

		/* ⚠️ The read state's matrices: the ones that produced the depth buffer, and the ones the volume was built with
		 * (Renderer::renderSkyAtmosphere()). */
		const auto readStateIndex = this->renderer().currentReadStateIndex();
		const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
		const auto invRelativeViewProj = (viewMatrices.projectionMatrix(readStateIndex) * viewMatrices.viewMatrix(readStateIndex, true, 0)).inverse();

		PushConstants pushConstants{
			.invRelativeViewProj = {},
			.parameters = {context.preExposure, SkyAtmosphere::AerialPerspectiveKmPerSlice, static_cast< float >(SkyAtmosphere::AerialPerspectiveSize), 0.0F}
		};

		std::copy_n(invRelativeViewProj.data(), pushConstants.invRelativeViewProj.size(), pushConstants.invRelativeViewProj.begin());

		recordFullscreenPass(commandBuffer, m_outputTarget, *m_pipeline, *m_pipelineLayout, descriptorSet, &pushConstants, sizeof(PushConstants));

		return m_outputTarget;
	}
}
