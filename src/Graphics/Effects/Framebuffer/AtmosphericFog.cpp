/*
 * src/Graphics/Effects/Framebuffer/AtmosphericFog.cpp
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

#include "AtmosphericFog.hpp"

/* STL inclusions. */
#include <cmath>
#include <numbers>
#include <string>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Scenes/LightSet.hpp"
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
static constexpr auto TracerTag{"AtmosphericFogEffect"};
/* NOLINTEND(cert-err58-cpp) */

/* Compile-time size check. */
static_assert(sizeof(EmEn::Graphics::Effects::Framebuffer::AtmosphericFog::FogPushConstants) == 116, "FogPushConstants must be exactly 116 bytes !");

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ---- */

	static constexpr auto FogFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;

layout(push_constant) uniform PushConstants
{
	/* Camera basis. */
	float cameraPosX, cameraPosY, cameraPosZ;
	float cameraRightX, cameraRightY, cameraRightZ;
	float cameraForwardX, cameraForwardY, cameraForwardZ;
	/* Depth reconstruction. */
	float nearPlane, farPlane;
	float tanHalfFovY, aspectRatio;
	/* Fog parameters. */
	float fogDensity;
	float fogHeightFalloff;
	float fogBaseHeight;
	float fogMaxDistance;
	float fogColorR, fogColorG, fogColorB;
	/* Directional inscattering. */
	float lightDirX, lightDirY, lightDirZ;
	float inscatterExponent;
	float inscatterColorR, inscatterColorG, inscatterColorB;
	float inscatterIntensity;
	/* Sky fog option. */
	float skyFogEnabled;
};

void main()
{
	vec3 sceneColor = texture(sceneTex, vUV).rgb;
	float depth = texture(depthTex, vUV).r;

	/* Reconstruct ray direction (needed for both geometry and sky pixels). */
	vec2 ndc = vUV * 2.0 - 1.0;
	float t = abs(tanHalfFovY);

	vec3 cameraPosition = vec3(cameraPosX, cameraPosY, cameraPosZ);
	vec3 cameraRight = vec3(cameraRightX, cameraRightY, cameraRightZ);
	vec3 cameraForward = vec3(cameraForwardX, cameraForwardY, cameraForwardZ);
	/* cross(right, forward) = row 1 of view matrix in right-handed Y-DOWN. */
	vec3 cameraUp = cross(cameraRight, cameraForward);

	/* Unit ray direction in world space. */
	vec3 rawDir = cameraRight * (ndc.x * t * aspectRatio) + cameraUp * (ndc.y * t) + cameraForward;
	vec3 rayDir = normalize(rawDir);

	bool isSky = (depth >= 0.9999);

	if (isSky && skyFogEnabled < 0.5)
	{
		/* Sky skip: no fog on sky pixels when sky fog is disabled. */
		outColor = vec4(sceneColor, 1.0);
		return;
	}

	/* Compute effective ray length. */
	float effectiveLength;

	if (isSky)
	{
		/* Sky pixel: use fogMaxDistance as the fictive ray length. */
		effectiveLength = fogMaxDistance;
	}
	else
	{
		/* Geometry pixel: linearize depth to get actual distance. */
		float z = depth * 2.0 - 1.0;
		float linearZ = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));

		/* Reconstruct world position for accurate ray length. */
		float vx = ndc.x * t * aspectRatio * linearZ;
		float vy = ndc.y * t * linearZ;
		vec3 worldPos = cameraPosition + cameraRight * vx + cameraUp * vy + cameraForward * linearZ;
		effectiveLength = min(length(worldPos - cameraPosition), fogMaxDistance);
	}

	/* Exponential height fog (Y-DOWN: +Y = deeper into fog). */
	vec3 effectiveEnd = cameraPosition + rayDir * effectiveLength;
	float heightDiff = effectiveEnd.y - cameraPosY;

	float k = fogHeightFalloff;
	float cameraOffset = cameraPosY - fogBaseHeight;

	float fogOpticalDepth;
	if (abs(k * heightDiff) > 0.001)
		fogOpticalDepth = fogDensity * exp(k * cameraOffset) * (exp(k * heightDiff) - 1.0) / (k * rayDir.y);
	else
		fogOpticalDepth = fogDensity * exp(k * cameraOffset) * effectiveLength;

	float fogAmount = 1.0 - exp(-max(fogOpticalDepth, 0.0));

	/* Directional inscattering (simplified Henyey-Greenstein).
	 * lightDir is the emission direction (sun → scene), negate it
	 * to get the source direction so inscattering peaks when
	 * looking toward the sun. */
	vec3 lightDir = vec3(lightDirX, lightDirY, lightDirZ);
	float cosAngle = dot(rayDir, -lightDir);
	float inscatter = pow(clamp((cosAngle + 1.0) * 0.5, 0.0, 1.0), inscatterExponent);
	vec3 fogColor = vec3(fogColorR, fogColorG, fogColorB);
	vec3 inscatterColor = vec3(inscatterColorR, inscatterColorG, inscatterColorB);
	vec3 finalFogColor = mix(fogColor, inscatterColor * inscatterIntensity, inscatter);

	/* Composite. */
	outColor = vec4(mix(sceneColor, finalFogColor, fogAmount), 1.0);
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
	AtmosphericFog::create (uint32_t width, uint32_t height) noexcept
	{
		/* Create output target (full-res). */
		if ( !m_outputTarget.create(this->renderer(), width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "AF_Output") )
		{
			TraceError{TracerTag} << "Failed to create output target !";

			return false;
		}

		/* ---- Descriptor set layout (dual-input: scene color + depth) ---- */
		auto tripleInputLayout = this->getInputLayout(2);

		if ( tripleInputLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layout ---- */
		{
			auto & layoutManager = this->renderer().layoutManager();

			const StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FogPushConstants)}
			};

			StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > sets;
			sets.emplace_back(tripleInputLayout);
			m_fogLayout = layoutManager.getPipelineLayout(sets, ranges);
		}

		if ( m_fogLayout == nullptr )
		{
			return false;
		}

		/* ---- Compile shaders ---- */
		const auto vertexModule = this->getFullscreenVertexShader();

		if ( vertexModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile vertex shader !";

			return false;
		}

		const auto fogFragment = this->renderer().shaderManager().getShaderModuleFromSourceCode(this->renderer().device(), "AF_Fog_MatProps_FS", Saphir::ShaderType::FragmentShader, FogFragmentShader);

		if ( fogFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile fog fragment shader !";

			return false;
		}

		/* ---- Create pipeline ---- */
		m_fogPipeline = this->createFullscreenPipeline(ClassId, "AF_Fog", vertexModule, fogFragment, m_fogLayout, m_outputTarget);

		if ( m_fogPipeline == nullptr )
		{
			return false;
		}

		/* ---- Create descriptor sets ---- */
		m_fogPerFrame = this->createPerFrameDescriptorSets(tripleInputLayout, ClassId, "AFDescSet");

		if ( m_fogPerFrame.empty() )
		{
			return false;
		}

		return true;
	}

	void
	AtmosphericFog::destroy () noexcept
	{
		m_fogPerFrame.clear();
		m_fogPipeline.reset();
		m_fogLayout.reset();
		m_outputTarget.destroy();
	}

	/* ---- Execute ---- */

	const TextureInterface &
	AtmosphericFog::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const TextureInterface * inputDepth, [[maybe_unused]] const TextureInterface * inputNormals, [[maybe_unused]] const TextureInterface * inputMaterialProperties, const Scenes::LightSet * lightSet, const PostProcessor::PushConstants & constants) noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Extract camera basis from view matrix.
		 * Use readStateIndex to match the view matrix that produced the depth buffer. */
		const auto readStateIndex = this->renderer().currentReadStateIndex();
		const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
		const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
		const auto & camPos = viewMatrices.position(readStateIndex);

		/* Right = row 0 of view matrix. */
		const auto rX = viewMat(0, 0);
		const auto rY = viewMat(0, 1);
		const auto rZ = viewMat(0, 2);

		/* Forward = negated row 2 (row 2 stores -forward in this engine). */
		const auto fX = -viewMat(2, 0);
		const auto fY = -viewMat(2, 1);
		const auto fZ = -viewMat(2, 2);

		/* Compute tanHalfFovY and aspect ratio. */
		const auto fovDeg = this->renderer().mainRenderTarget()->viewMatrices().fieldOfView();
		const auto tanHalfFovY = std::tan(fovDeg * std::numbers::pi_v< float > / 360.0F);
		const auto aspectRatio = this->renderer().mainRenderTarget()->viewMatrices().getAspectRatio();

		/* Read light direction and inscatter color from LightSet. */
		const auto mainLight = lightSet->mainDirectionalLight();
		const auto lightDir = mainLight->direction().normalized();
		const auto inscatterColor = m_inscatterColorOverride.value_or(mainLight->color());

		/* Update per-frame descriptor with scene color, depth, and material properties. */
		static_cast< void >(m_fogPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_fogPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputDepth));
		}

		/* Build push constants. */
		const FogPushConstants fogPC{
			.cameraPosX = camPos[0],
			.cameraPosY = camPos[1],
			.cameraPosZ = camPos[2],
			.cameraRightX = rX,
			.cameraRightY = rY,
			.cameraRightZ = rZ,
			.cameraForwardX = fX,
			.cameraForwardY = fY,
			.cameraForwardZ = fZ,
			.nearPlane = constants.nearPlane,
			.farPlane = constants.farPlane,
			.tanHalfFovY = tanHalfFovY,
			.aspectRatio = aspectRatio,
			.fogDensity = m_parameters.density,
			.fogHeightFalloff = m_parameters.heightFalloff,
			.fogBaseHeight = m_parameters.baseHeight,
			.fogMaxDistance = m_parameters.maxDistance,
			.fogColorR = m_parameters.fogColor.red(),
			.fogColorG = m_parameters.fogColor.green(),
			.fogColorB = m_parameters.fogColor.blue(),
			.lightDirX = lightDir.x(),
			.lightDirY = lightDir.y(),
			.lightDirZ = lightDir.z(),
			.inscatterExponent = m_parameters.inscatterExponent,
			.inscatterColorR = inscatterColor.red(),
			.inscatterColorG = inscatterColor.green(),
			.inscatterColorB = inscatterColor.blue(),
			.inscatterIntensity = m_parameters.inscatterIntensity,
			.skyFogEnabled = m_parameters.skyFogEnabled ? 1.0F : 0.0F
		};

		/* Record single fullscreen pass. */
		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_outputTarget,
			*m_fogPipeline,
			*m_fogLayout,
			*m_fogPerFrame[frameIndex],
			&fogPC,
			sizeof(FogPushConstants)
		);

		return m_outputTarget;
	}
}
