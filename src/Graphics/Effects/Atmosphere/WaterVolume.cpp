/*
 * src/Graphics/Effects/Atmosphere/WaterVolume.cpp
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

#include "WaterVolume.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/Effects/Shared/ViewReconstructionGLSL.hpp"
#include "Graphics/FrameView.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/SeaImmersion.hpp"
#include "Graphics/SeaSurfaceWaves.hpp"
#include "Math/Vector.hpp"
#include "Saphir/Generator/OceanSurfaceHelper.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Saphir/Types.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/LightSet.hpp"
#include "Scenes/SeaLevelInterface.hpp"
#include "Scenes/WaterMedium.hpp"
#include "StaticVector.hpp"
#include "Tracer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/UniformBufferObject.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class DescriptorSetLayout;
}

namespace
{
	using namespace EmEn;

	/** @brief The scene colour and the depth: the samplers both variants bind first. */
	constexpr uint32_t SceneSamplerCount{2};

	/** @brief The moving variant's bindings after them: the ocean's displacement cascades, its shelter map, then —
	 * the uniform buffers follow the samplers — its uniform block and the view reconstruction. */
	constexpr uint32_t DisplacementBinding{2};
	constexpr uint32_t ShelterBinding{3};
	constexpr uint32_t MovingSamplerCount{4};
	constexpr uint32_t SurfaceUniformsBinding{4};
	constexpr uint32_t MovingViewBinding{5};

	constexpr auto Header = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;

layout(push_constant) uniform PushConstants
{
	/* xyz: extinction per channel (1/m), w: the sea level (world Y, m). */
	vec4 extinction;
	/* xyz: diffuse attenuation with depth per channel (1/m), w: the highest the surface stands above the level (m). */
	vec4 depthAttenuation;
	/* xyz: the pre-exposed scattering source just under the surface, per metre of path, w: the meniscus' half-width (px). */
	vec4 source;
} pc;
)GLSL";

	/* ---- The surface at rest: the level. ---- */
	constexpr auto SurfaceAtRest = R"GLSL(
float wvSurfaceRise (vec2 worldXZ)
{
	return 0.0;
}
)GLSL";

	/* ---- The moving surface: the mesh, where the OCEAN says it stands (Saphir::Generator::getOceanSurfaceStandingSource(),
	 * prepended to this). ---- */
	constexpr auto MovingSurface = R"GLSL(
float wvSurfaceRise (vec2 worldXZ)
{
	return ocSurfaceRiseAt(worldXZ);
}
)GLSL";

	constexpr auto FragmentShader = R"GLSL(
void main ()
{
	vec3 sceneColor = texture(sceneTex, vUV).rgb;

	/* The pixel's point on the NEAR plane (depth 0): where the drawn part of its ray starts. How deep it stands under
	 * the drawn surface says on which side of the water the pixel is. */
	vec3 eye = emView.cameraPosition.xyz;
	vec3 start = emRelativePosition(vUV, 0.0);
	vec3 startWorld = eye + start;
	float startDepth = (pc.extinction.w + wvSurfaceRise(startWorld.xz)) - startWorld.y;

	/* The meniscus: where the lens meets the water, on both sides of the line. */
	float lineDistance = abs(startDepth) / max(fwidth(startDepth), 1.0e-9);
	float meniscus = pc.source.w > 0.0 ? 1.0 - smoothstep(0.0, pc.source.w, lineDistance) : 0.0;
	float lensTransmission = mix(1.0, MENISCUS_TRANSMISSION, meniscus);

	if ( startDepth <= 0.0 )
	{
		outColor = vec4(sceneColor * lensTransmission, 1.0);

		return;
	}

	/* The first surface the frame drew along the ray: the seabed, or the sea surface seen from below (the
	 * translucent pass wrote its depth). The sky (depth 1) reconstructs the far plane in its direction. */
	vec3 surface = emRelativePosition(vUV, texture(depthTex, vUV).r);
	float surfaceDistance = length(surface);
	vec3 ray = surface / max(surfaceDistance, 1.0e-6);
	float startDistance = length(start);

	/* An upward ray that meets nothing drawn leaves the water where it can stand no higher. */
	float endDistance = surfaceDistance;

	if ( ray.y > 1.0e-6 )
	{
		endDistance = min(endDistance, (pc.extinction.w + pc.depthAttenuation.w - eye.y) / ray.y);
	}

	float path = max(endDistance - startDistance, 0.0);

	vec3 transmittance = exp(-pc.extinction.xyz * path);

	/* The light scattered toward the eye along the path. Its source falls off with the depth: exp(-Kd (z0 - wy t)),
	 * so the integrand is exp(-(sigma_t - Kd wy) t) — a rate that looking up can bring to zero or below (the path
	 * then runs toward the light faster than the water dims it), which the closed form holds as long as the division
	 * is kept away from zero. */
	vec3 rate = pc.extinction.xyz - pc.depthAttenuation.xyz * ray.y;
	bvec3 level = lessThan(abs(rate), vec3(1.0e-5));
	vec3 integral = mix((1.0 - exp(-rate * path)) / mix(rate, vec3(1.0), level), vec3(path), level);
	vec3 inscatter = pc.source.xyz * exp(-pc.depthAttenuation.xyz * startDepth) * integral;

	outColor = vec4(((sceneColor * transmittance) + inscatter) * lensTransmission, 1.0);
}
)GLSL";

	/** @brief The light that entered the water. */
	struct EnteringLight
	{
		/** @brief Its illuminance just under the surface, in lux per channel. */
		Base::Math::Vector< 3, float > illuminance{0.0F, 0.0F, 0.0F};
		/** @brief The cosine of the downwelling light's angle from the vertical (the refracted sun's, when there is one). */
		float downwellingCosine{Scenes::WaterMedium::MeanDownwellingCosine};
	};

	/**
	 * @brief Returns the light that entered the water: the sun through the surface, and the sky.
	 * @param context A reference to the frame's chain context.
	 * @return EnteringLight
	 */
	[[nodiscard]]
	EnteringLight
	enteringLight (const Graphics::IndirectPostProcessEffect::FrameContext & context) noexcept
	{
		using namespace Base::Math;
		using Graphics::Effects::Atmosphere::WaterVolume;

		EnteringLight light;

		/* The sun. A DISABLED main light lights nothing: a celestial clock leaves its last illuminance on the sun it
		 * switched off. Its emission direction runs from the sun to the scene: its opposite's height is the sun's. */
		const auto mainLight = context.lightSet != nullptr ? context.lightSet->mainDirectionalLight() : nullptr;
		const auto sunCosine = mainLight != nullptr && mainLight->isEnabled() ? std::clamp(-mainLight->direction().normalized()[Y], 0.0F, 1.0F) : 0.0F;

		if ( sunCosine > 0.0F )
		{
			/* Schlick's reflectance at the sun's incidence; what is not reflected enters. */
			const auto reflected = Scenes::WaterMedium::NormalReflectance + ((1.0F - Scenes::WaterMedium::NormalReflectance) * std::pow(1.0F - sunCosine, 5.0F));
			const auto illuminance = mainLight->illuminance() * sunCosine * (1.0F - reflected);
			/* Snell: the sun under the surface stands nearer to the vertical. */
			const auto refractedSine = std::sqrt(1.0F - (sunCosine * sunCosine)) / Scenes::WaterMedium::RefractiveIndex;

			light.illuminance = mainLight->emissionChromaticity() * illuminance;
			light.downwellingCosine = std::max(WaterVolume::MinDownwellingCosine, std::sqrt(1.0F - (refractedSine * refractedSine)));
		}

		/* The sky: the ambient the shading uses when it is a scalar, else the background's luminance as a uniform sky
		 * (E = π L). ⚠️ White in that second case: the sky's own colour is not carried by the frame context. */
		const auto ambient = context.frameView->ambientIlluminance();
		const auto skyIlluminance = ambient > 0.0F ? ambient : std::numbers::pi_v< float > * context.skyLuminance;
		const auto skyColor = ambient > 0.0F ? context.frameView->ambientChromaticity() : Vector< 3, float >{1.0F, 1.0F, 1.0F};

		light.illuminance = light.illuminance + (skyColor * (skyIlluminance * WaterVolume::SkyTransmission));

		return light;
	}
}

namespace EmEn::Graphics::Effects::Atmosphere
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	bool
	WaterVolume::createVariant (Variant & variant, bool movingSurface) noexcept
	{
		/* Scene colour and depth, then the view reconstruction; the moving variant binds the ocean's displacement, its
		 * shelter map and its uniform block in between. */
		const uint32_t samplerCount = movingSurface ? MovingSamplerCount : SceneSamplerCount;
		const uint32_t viewBinding = movingSurface ? MovingViewBinding : SceneSamplerCount;

		auto inputLayout = this->getInputLayout(samplerCount, movingSurface ? 2 : 1);

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
			variant.pipelineLayout = this->renderer().layoutManager().getPipelineLayout(sets, ranges);
		}

		if ( variant.pipelineLayout == nullptr )
		{
			return false;
		}

		const auto vertexModule = this->getFullscreenVertexShader();

		if ( vertexModule == nullptr )
		{
			TraceError{ClassId} << "Failed to compile the vertex shader !";

			return false;
		}

		/* Where the moving surface stands is the ocean's own GLSL, over the ocean's own resources. */
		const auto surface = movingSurface
			? Saphir::Generator::getOceanSurfaceStandingSource({.set = 0, .displacement = DisplacementBinding, .shelter = ShelterBinding, .uniforms = SurfaceUniformsBinding}) + MovingSurface
			: std::string{SurfaceAtRest};

		const auto source =
			std::string{Header} +
			"\n#define MENISCUS_TRANSMISSION " + std::to_string(MeniscusTransmission) + "\n" +
			surface +
			ViewReconstructionGLSL::sourceGLSL(0, viewBinding) +
			FragmentShader;

		const auto fragmentModule = this->renderer().shaderManager().getShaderModuleFromSourceCode(this->renderer().device(), movingSurface ? "WV_Moving_FS" : "WV_AtRest_FS", Saphir::ShaderType::FragmentShader, source);

		if ( fragmentModule == nullptr )
		{
			TraceError{ClassId} << "Failed to compile the fragment shader of the " << (movingSurface ? "moving" : "at rest") << " variant !";

			return false;
		}

		variant.pipeline = this->createFullscreenPipeline(ClassId, movingSurface ? "WV_Moving" : "WV_AtRest", vertexModule, fragmentModule, variant.pipelineLayout, m_outputTarget);

		if ( variant.pipeline == nullptr )
		{
			return false;
		}

		variant.perFrame = this->createPerFrameDescriptorSets(inputLayout, ClassId, movingSurface ? "WVMovingDescSet" : "WVAtRestDescSet");

		return !variant.perFrame.empty();
	}

	bool
	WaterVolume::create (uint32_t width, uint32_t height) noexcept
	{
		if ( !m_outputTarget.create(this->renderer(), width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "WV_Output") )
		{
			TraceError{ClassId} << "Failed to create the output target !";

			return false;
		}

		return this->createVariant(m_atRest, false) && this->createVariant(m_moving, true);
	}

	void
	WaterVolume::destroy () noexcept
	{
		m_moving = {};
		m_atRest = {};
		m_outputTarget.destroy();
	}

	const TextureInterface &
	WaterVolume::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		if ( this->record(commandBuffer, inputColor, context) )
		{
			return m_outputTarget;
		}

		/* No underwater pixel this frame, or nothing to draw them with: the chain colour passes through. */
		return inputColor;
	}

	bool
	WaterVolume::record (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		/* A sea that declares its water, and what a ray is reconstructed with. */
		const auto * sea = context.sea;
		const auto * water = sea != nullptr ? sea->waterMedium() : nullptr;

		if ( water == nullptr || context.depth == nullptr || context.view == nullptr )
		{
			return false;
		}

		/* Where the frame's near plane stands against the sea (FrameContext::seaImmersion, decided once a frame for
		 * every effect). Clear of the highest the surface can be: no pixel's ray starts in the water, the pass is not
		 * recorded. Submerged: every pixel's does, and none needs to be stood against the waves. */
		if ( context.seaImmersion == SeaImmersion::Clear )
		{
			return false;
		}

		const auto seaLevel = sea->getLevel();
		/* A surface that moves, once it has been drawn: where it stands, and how far from its level it can. */
		const auto * waves = sea->surfaceWaves();
		const auto surfaceReach = waves != nullptr ? waves->reach : 0.0F;

		const bool movingSurface =
			context.seaImmersion == SeaImmersion::Astride && waves != nullptr &&
			waves->displacement != nullptr && waves->sampler != nullptr &&
			waves->shelter != nullptr && waves->shelterSampler != nullptr && waves->uniforms != nullptr;
		const auto & variant = movingSurface ? m_moving : m_atRest;
		const auto frameIndex = this->renderer().currentFrameIndex();

		if ( frameIndex >= variant.perFrame.size() )
		{
			return false;
		}

		const auto light = enteringLight(context);
		const auto & scattering = water->scattering();
		const auto extinction = water->extinction();
		const auto depthAttenuation = water->diffuseAttenuation(light.downwellingCosine);
		const auto sourceScale = context.preExposure / (4.0F * std::numbers::pi_v< float >);

		const PushConstants pushConstants{
			.extinction = {extinction[X], extinction[Y], extinction[Z], seaLevel},
			.depthAttenuation = {depthAttenuation[X], depthAttenuation[Y], depthAttenuation[Z], surfaceReach},
			.source = {scattering[X] * light.illuminance[X] * sourceScale, scattering[Y] * light.illuminance[Y] * sourceScale, scattering[Z] * light.illuminance[Z] * sourceScale, MeniscusHalfWidth}
		};

		const auto & descriptorSet = *variant.perFrame[frameIndex];

		/* The ocean's resources of THIS frame (Graphics::SeaSurfaceWaves): what its own stages bind. */
		const auto surfaceBound = [&descriptorSet, waves] () noexcept {
			const VkDescriptorBufferInfo uniforms{
				.buffer = waves->uniforms->handle(),
				.offset = waves->uniformsOffset,
				.range = waves->uniformsRange
			};

			return
				descriptorSet.writeCombinedImageSampler(DisplacementBinding, *waves->displacement, *waves->sampler, VK_IMAGE_LAYOUT_GENERAL) &&
				descriptorSet.writeCombinedImageSampler(ShelterBinding, *waves->shelter, *waves->shelterSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) &&
				descriptorSet.writeUniformBuffer(SurfaceUniformsBinding, uniforms);
		};

		const bool written =
			descriptorSet.writeCombinedImageSampler(0, inputColor) &&
			descriptorSet.writeCombinedImageSampler(1, *context.depth) &&
			(!movingSurface || surfaceBound()) &&
			descriptorSet.writeUniformBufferObject(movingSurface ? MovingViewBinding : SceneSamplerCount, *context.view);

		if ( !written )
		{
			TraceError{ClassId} << "Unable to bind the inputs of the water volume !";

			return false;
		}

		recordFullscreenPass(commandBuffer, m_outputTarget, *variant.pipeline, *variant.pipelineLayout, descriptorSet, &pushConstants, sizeof(PushConstants));

		return true;
	}
}
