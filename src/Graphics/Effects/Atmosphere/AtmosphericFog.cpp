/*
 * src/Graphics/Effects/Atmosphere/AtmosphericFog.cpp
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

#include "AtmosphericFog.hpp"

/* STL inclusions. */
#include <string>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/Effects/Shared/ViewReconstructionGLSL.hpp"
#include "Graphics/Renderer.hpp"
#include "Math/Vector.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Saphir/Types.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/LightSet.hpp"
#include "Scenes/ParticipatingMedium.hpp"
#include "StaticVector.hpp"
#include "Tracer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/LayoutManager.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class DescriptorSetLayout;
}

static constexpr auto TracerTag{"AtmosphericFogEffect"};

/* Compile-time size check. */
static_assert(sizeof(EmEn::Graphics::Effects::Atmosphere::AtmosphericFog::FogPushConstants) == 64, "FogPushConstants must be exactly 64 bytes !");

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ---- */

	/** @brief The view reconstruction's binding (after the three samplers: getInputLayout(3, 1)). */
	constexpr uint32_t ViewBinding{3};

	constexpr auto FogHeader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler2D materialPropsTex;
)GLSL";

	constexpr auto FogFragmentShader = R"GLSL(
layout(push_constant) uniform PushConstants
{
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

	/* The surface's offset from the camera, through the frame's inverse view-projection (ViewReconstructionGLSL): a sky
	 * pixel (depth 1) reconstructs the far plane in its direction, which is all a sky pixel needs. Until 2026-10-25 the
	 * distance came from clip planes frozen at 0.1 / 1000 (up to 2.1x too far with a wide lens) and the ray from a
	 * signed tanHalfFovY, right for a perspective camera only. */
	vec3 relative = emRelativePosition(vUV, depth);
	vec3 rayDir = normalize(relative);
	vec3 cameraPosition = emView.cameraPosition.xyz;
	float cameraPosY = cameraPosition.y;

	/* ⚠️ The sky is the clear value, 1.0 exactly: 0.9999 read every mountain past ~890 m as sky. */
	bool isSky = (depth >= 1.0);

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
		/* Geometry pixel: the Euclidean distance to the surface. */
		effectiveLength = min(length(relative), fogMaxDistance);
	}

	/* Exponential height fog. fogHeightFalloff is a POSITIVE decay rate, so the exponent it
	 * feeds must be NEGATED: density falls off going UP (+Y) in this Y-up world.
	 * The analytic integral below stays valid for either sign of k, which is exactly what made
	 * this defect silent: until Aug 2026 k was used unnegated, correct while +Y meant DOWN, and
	 * the Y-up flip turned it into fog that grew DENSER WITH ALTITUDE. */
	vec3 effectiveEnd = cameraPosition + rayDir * effectiveLength;
	float heightDiff = effectiveEnd.y - cameraPosY;

	float k = -fogHeightFalloff;
	float cameraOffset = cameraPosY - fogBaseHeight;

	float fogOpticalDepth;
	if (abs(k * heightDiff) > 0.001)
		fogOpticalDepth = fogDensity * exp(k * cameraOffset) * (exp(k * heightDiff) - 1.0) / (k * rayDir.y);
	else
		fogOpticalDepth = fogDensity * exp(k * cameraOffset) * effectiveLength;

	float fogAmount = 1.0 - exp(-max(fogOpticalDepth, 0.0));

	/* Modulate fog amount by per-pixel material fog response (A channel high nibble).
	 * fogResponse = 1.0 → full fog (default), fogResponse = 0.0 → immune to fog
	 * (e.g. HUD overlays, in-world icons, decals that must remain crisp). */
	vec4 mp = texture(materialPropsTex, vUV);
	uint aPacked = uint(mp.a * 255.0);
	float fogResponse = float(aPacked >> 4u) / 15.0;
	fogAmount *= fogResponse;

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

namespace EmEn::Graphics::Effects::Atmosphere
{
	using namespace Base;
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

		/* ---- Descriptor set layout (scene color + depth + material properties, the view reconstruction) ---- */
		auto tripleInputLayout = this->getInputLayout(3, 1);

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

			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
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

		const auto fogFragment = this->renderer().shaderManager().getShaderModuleFromSourceCode(this->renderer().device(), "AF_Fog_MatProps_FS", Saphir::ShaderType::FragmentShader, std::string{FogHeader} + ViewReconstructionGLSL::sourceGLSL(0, ViewBinding) + FogFragmentShader);

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

		return !m_fogPerFrame.empty();
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
	AtmosphericFog::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputMaterialProperties = context.materialProperties;
		const auto * lightSet = context.lightSet;

		/* The view reconstruction is made with the chain (PostProcessor::configure() fails without it). */
		if ( context.view == nullptr )
		{
			return inputColor;
		}

		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Read light direction and inscatter color from LightSet. */
		const auto mainLight = lightSet != nullptr ? lightSet->mainDirectionalLight() : nullptr;

		if ( mainLight == nullptr )
		{
			/* No directional light: the fog has nothing lighting it and no direction to scatter
			 * along. Passing the chain colour through untouched is the only honest answer —
			 * compositing the parameters raw is what used to paint the sky black. */
			return inputColor;
		}

		/* ⚠️ No medium, no fog. An atmospheric fog effect without an atmosphere is a pass-through,
		 * not a fog with invented parameters — the whole point of moving the medium to the scene is
		 * that there is exactly ONE place that describes it. A scene that adds this effect and
		 * forgets to declare a medium gets the chain colour untouched, and says so once. */
		const auto * medium = context.medium;

		if ( medium == nullptr )
		{
			if ( !m_missingMediumReported )
			{
				m_missingMediumReported = true;

				TraceWarning{ClassId} << "The scene declares no participating medium: the atmospheric fog is a no-op. Set one with Scene::setParticipatingMedium().";
			}

			return inputColor;
		}

		const auto lightDir = mainLight->direction().normalized();
		/* The emitted chromaticity (unit luminance): the override is normalised the same way. */
		const auto inscatterColor = m_inscatterColorOverride.has_value() ? m_inscatterColorOverride->unitLuminanceChromaticity() : mainLight->emissionChromaticity();

		/* PHOTOMETRIC SCALE. 'fogColor' and the light colour are CHROMATICITIES; the buffer this
		 * effect composites into holds ABSOLUTE LUMINANCE in nits. Multiplying the chromaticity in
		 * raw made the fog ~0.6 nits, which is black once the camera exposure is applied — and for
		 * a sky pixel the fog amount saturates (the fictive ray length is maxDistance), so the sky
		 * was REPLACED by that black. Same separation VolumetricLight already makes between
		 * mainLight->emissionChromaticity() and mainLight->intensity().
		 * Default derivation: L = E · ρ / π, the Lambertian relation used everywhere else in the
		 * engine, with E the illuminance in lux and ρ carried by the chromaticity itself. */
		/* PRE-EXPOSURE (step 6, D8): the composited buffer holds nits × E_pre, so the fog's own radiance is
		 * written at that scale too. */
		const auto fogLuminance = medium->resolveLuminance(mainLight->illuminance()) * context.preExposure;

		/* Update per-frame descriptor with scene color, depth, and material properties. */
		static_cast< void >(m_fogPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_fogPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputDepth));
		}

		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_fogPerFrame[frameIndex]->writeCombinedImageSampler(2, *inputMaterialProperties));
		}

		static_cast< void >(m_fogPerFrame[frameIndex]->writeUniformBufferObject(ViewBinding, *context.view));

		/* Build push constants. */
		const FogPushConstants fogPC{
			.fogDensity = medium->density(),
			.fogHeightFalloff = medium->heightFalloff(),
			.fogBaseHeight = medium->baseHeight(),
			.fogMaxDistance = medium->maxDistance(),
			.fogColorR = medium->scatteringAlbedo().red() * fogLuminance,
			.fogColorG = medium->scatteringAlbedo().green() * fogLuminance,
			.fogColorB = medium->scatteringAlbedo().blue() * fogLuminance,
			.lightDirX = lightDir.x(),
			.lightDirY = lightDir.y(),
			.lightDirZ = lightDir.z(),
			.inscatterExponent = m_parameters.inscatterExponent,
			.inscatterColorR = inscatterColor[Base::Math::X] * fogLuminance,
			.inscatterColorG = inscatterColor[Base::Math::Y] * fogLuminance,
			.inscatterColorB = inscatterColor[Base::Math::Z] * fogLuminance,
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
