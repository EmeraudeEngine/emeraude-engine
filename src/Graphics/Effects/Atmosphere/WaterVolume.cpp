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
#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <numbers>
#include <string>
#include <utility>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/Effects/Shared/CSMSamplingGLSL.hpp"
#include "Graphics/Effects/Shared/MarchDitherGLSL.hpp"
#include "Graphics/Effects/Shared/SeaCausticsGLSL.hpp"
#include "Graphics/Effects/Shared/ViewReconstructionGLSL.hpp"
#include "Graphics/FrameView.hpp"
#include "Graphics/OceanWaves.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/SeaCaustics.hpp"
#include "Graphics/SeaImmersion.hpp"
#include "Graphics/SeaSurfaceWaves.hpp"
#include "Graphics/ViewMatricesCascadedUBO.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Math/Matrix.hpp"
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

	/**
	 * @brief The bindings of a variant that marches the sun's shafts. The samplers first — the scene colour and the
	 * depth, the moving surface's two, then the sun's shadow cascades, the three curvatures and the shelter map —, the
	 * uniform blocks after them: the moving surface's, the cascades', the shafts', the view reconstruction.
	 * ⚠️ The cascades' two bindings are LITERALS of EMEN_CSM_SAMPLING_GLSL below: 2 and 7 at rest, 4 and 10 moving.
	 */
	struct ShaftBindings
	{
		uint32_t shadow;
		uint32_t curvature;
		uint32_t shelter;
		uint32_t samplerCount;
		uint32_t cascades;
		uint32_t shafts;
		uint32_t view;
	};

	/**
	 * @brief Where a variant binds what all of them read: how many samplers it has, its view reconstruction, the moving
	 * surface's uniform block.
	 */
	struct VariantBindings
	{
		uint32_t samplerCount;
		uint32_t view;
		uint32_t surfaceUniforms;
	};

	constexpr ShaftBindings ShaftsAtRest{.shadow = 2, .curvature = 3, .shelter = 6, .samplerCount = 7, .cascades = 7, .shafts = 8, .view = 9};
	constexpr ShaftBindings ShaftsMoving{.shadow = 4, .curvature = 5, .shelter = 8, .samplerCount = 9, .cascades = 10, .shafts = 11, .view = 12};
	/** @brief The moving surface's uniform block in the marched moving variant (after its nine samplers). */
	constexpr uint32_t ShaftsMovingSurfaceUniformsBinding{9};

	/**
	 * @brief Returns where a variant binds what all of them read.
	 * @param movingSurface Whether the variant stands a pixel against the ocean's mesh.
	 * @param shafts Whether the variant marches the sun's shafts.
	 * @return VariantBindings
	 */
	[[nodiscard]]
	constexpr
	VariantBindings
	variantBindings (bool movingSurface, bool shafts) noexcept
	{
		if ( shafts )
		{
			const auto & bindings = movingSurface ? ShaftsMoving : ShaftsAtRest;

			return {.samplerCount = bindings.samplerCount, .view = bindings.view, .surfaceUniforms = ShaftsMovingSurfaceUniformsBinding};
		}

		if ( movingSurface )
		{
			return {.samplerCount = MovingSamplerCount, .view = MovingViewBinding, .surfaceUniforms = SurfaceUniformsBinding};
		}

		return {.samplerCount = SceneSamplerCount, .view = SceneSamplerCount, .surfaceUniforms = SurfaceUniformsBinding};
	}

	constexpr auto ShadowCascadesAtRest = EMEN_CSM_SAMPLING_GLSL(2, 7);
	constexpr auto ShadowCascadesMoving = EMEN_CSM_SAMPLING_GLSL(4, 10);

	/* ---- The sun's shafts: what the sun's light has become at a point of the water. ----
	 * The light that reaches a point under the sea crossed the surface up-sun of it: the waves there focused it or
	 * spread it (the caustics' gain, the one the sun's light pass draws on the seabed — SeaCausticsGLSL.hpp), and
	 * something may shadow that point (the sun's cascaded shadow map). 1 = the even light the closed form assumes.
	 * ⚠️ The shadow map is read along the sun's direction in the AIR: the bend of the light at the surface is not in it. */
	constexpr auto ShaftsFunctions = R"GLSL(
const float WvPi = 3.14159265358979323846;

float wvSunModulation (vec3 world, float depth, float blurFloor, float viewDepth)
{
	const vec3 travel = wvShafts.sunDirection.xyz;
	const float cosRefracted = max(-travel.y, 0.05);
	const float path = depth / cosRefracted;
	const vec2 crossing = world.xz - (travel.xz / cosRefracted) * depth;
	const float open = textureLod(wvShelter, (crossing - wvShafts.shelterRectangle.xy) * wvShafts.shelterRectangle.zw, 0.0).r;

	vec3 curvature = vec3(0.0);

	{
		const float weight = mix(wvShafts.shelteredShares.x, 1.0, open);
		curvature += weight * textureLod(wvCurvature0, crossing / wvShafts.cascadeSizes.x, emSeaCurvatureLevel(max(wvShafts.shelteredShares.w * depth * weight, blurFloor), WV_CURVATURE_TEXELS, wvShafts.cascadeSizes.x)).xyz;
	}
	{
		const float weight = mix(wvShafts.shelteredShares.y, 1.0, open);
		curvature += weight * textureLod(wvCurvature1, crossing / wvShafts.cascadeSizes.y, emSeaCurvatureLevel(max(wvShafts.shelteredShares.w * depth * weight, blurFloor), WV_CURVATURE_TEXELS, wvShafts.cascadeSizes.y)).xyz;
	}
	{
		const float weight = mix(wvShafts.shelteredShares.z, 1.0, open);
		curvature += weight * textureLod(wvCurvature2, crossing / wvShafts.cascadeSizes.z, emSeaCurvatureLevel(max(wvShafts.shelteredShares.w * depth * weight, blurFloor), WV_CURVATURE_TEXELS, wvShafts.cascadeSizes.z)).xyz;
	}

	const float drawn = clamp(wvShafts.cascadeSizes.w, 0.0, 1.0) * exp(-depth / max(wvShafts.tuning.x, 0.01));
	const float visibility = emCSMVisibility(world, viewDepth, int(wvShafts.cameraForward.w), wvShafts.tuning.y);

	return visibility * mix(1.0, emSeaCausticGain(curvature, path), drawn);
}
)GLSL";

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

#ifdef WV_SHAFTS
	/* The angle a pixel spans, taken before any branch (a derivative inside one is undefined): the finest detail of
	 * the shafts a pixel can show at a distance. */
	vec3 wvDirection = normalize(emRelativePosition(vUV, 1.0));
	float wvPixelAngle = max(length(dFdx(wvDirection)), length(dFdy(wvDirection)));
#endif

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

#ifdef WV_SHAFTS
	/* The SUN's share, marched (pc.source holds the sky's alone in this variant): the closed form above assumes the
	 * same light at every point of a depth; the march adds what the waves and the shadows change of it,
	 * sum over the steps of (M - 1) x the step's exact share of the integral — so a water nothing modulates gives the
	 * closed form to the bit, and the part of a long path beyond the march keeps it. */
	{
		const float steps = max(wvShafts.sunSource.w, 1.0);
		const float reach = 6.0 / max(min(pc.extinction.x, min(pc.extinction.y, pc.extinction.z)), 1.0e-4);
		const float stepLength = min(path, reach) / steps;
		const float dither = emInterleavedGradientNoise(gl_FragCoord.xy);
		const float forward = dot(ray, wvShafts.cameraForward.xyz);

		vec3 deviation = vec3(0.0);

		for ( float index = 0.0; index < steps; index += 1.0 )
		{
			const float from = index * stepLength;
			const vec3 share = mix((exp(-rate * from) - exp(-rate * (from + stepLength))) / mix(rate, vec3(1.0), level), vec3(stepLength), level);
			const float travelled = startDistance + from + (dither * stepLength);
			const vec3 world = eye + (ray * travelled);
			const float depth = pc.extinction.w - world.y;

			if ( depth > 0.0 )
			{
				/* Never finer than a pixel, nor than half a step: what the march cannot resolve is averaged, not aliased. */
				const float blurFloor = max(travelled * wvPixelAngle, 0.5 * stepLength);

				deviation += (wvSunModulation(world, depth, blurFloor, travelled * forward) - 1.0) * share;
			}
		}

		/* The sun's light scattered toward the eye: Henyey-Greenstein over the isotropic 1 / 4 pi the source holds
		 * (1 everywhere at g = 0). The light travels along sunDirection and leaves toward the eye, -ray. */
		const float g = wvShafts.sunDirection.w;
		const float cosTheta = -dot(wvShafts.sunDirection.xyz, ray);
		const float phase = (1.0 - g * g) / pow(max(1.0 + g * g - 2.0 * g * cosTheta, 1.0e-4), 1.5);

		inscatter += phase * wvShafts.sunSource.xyz * exp(-pc.depthAttenuation.xyz * startDepth) * max(integral + (wvShafts.tuning.z * deviation), vec3(0.0));
	}
#endif

	outColor = vec4(((sceneColor * transmittance) + inscatter) * lensTransmission, 1.0);
}
)GLSL";

	/** @brief The light that entered the water. */
	struct EnteringLight
	{
		/** @brief The SUN's illuminance just under the surface, in lux per channel. */
		Base::Math::Vector< 3, float > sun{0.0F, 0.0F, 0.0F};
		/** @brief The SKY's illuminance just under the surface, in lux per channel. */
		Base::Math::Vector< 3, float > sky{0.0F, 0.0F, 0.0F};
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

			light.sun = mainLight->emissionChromaticity() * illuminance;
			light.downwellingCosine = std::max(WaterVolume::MinDownwellingCosine, std::sqrt(1.0F - (refractedSine * refractedSine)));
		}

		/* The sky: the ambient the shading uses when it is a scalar, else the background's luminance as a uniform sky
		 * (E = π L). ⚠️ White in that second case: the sky's own colour is not carried by the frame context. */
		const auto ambient = context.frameView->ambientIlluminance();
		const auto skyIlluminance = ambient > 0.0F ? ambient : std::numbers::pi_v< float > * context.skyLuminance;
		const auto skyColor = ambient > 0.0F ? context.frameView->ambientChromaticity() : Vector< 3, float >{1.0F, 1.0F, 1.0F};

		light.sky = skyColor * (skyIlluminance * WaterVolume::SkyTransmission);

		return light;
	}
}

namespace EmEn::Graphics::Effects::Atmosphere
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	bool
	WaterVolume::createVariant (Variant & variant, bool movingSurface, bool shafts) noexcept
	{
		/* Scene colour and depth, then the view reconstruction; the moving variant binds the ocean's displacement, its
		 * shelter map and its uniform block in between; a marched variant, the sun's shadow cascades, the caustics'
		 * curvature and shelter map, and its two blocks (ShaftBindings). */
		const auto & shaftBindings = movingSurface ? ShaftsMoving : ShaftsAtRest;
		const auto common = variantBindings(movingSurface, shafts);
		const uint32_t samplerCount = common.samplerCount;
		const uint32_t viewBinding = common.view;
		const uint32_t surfaceUniformsBinding = common.surfaceUniforms;
		const uint32_t blockCount = (movingSurface ? 2U : 1U) + (shafts ? 2U : 0U);

		auto inputLayout = this->getInputLayout(samplerCount, blockCount);

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
			? Saphir::Generator::getOceanSurfaceStandingSource({.set = 0, .displacement = DisplacementBinding, .shelter = ShelterBinding, .uniforms = surfaceUniformsBinding}) + MovingSurface
			: std::string{SurfaceAtRest};

		/* What a marched variant declares: the caustics' textures and the shafts' block at its bindings, the shadow
		 * cascades (their bindings are literals of the macro), the march's dither, the caustics' gain. */
		std::string shaftsSource;

		if ( shafts )
		{
			const auto binding = [] (uint32_t index) {
				return "layout(set = 0, binding = " + std::to_string(index) + ") ";
			};

			shaftsSource =
				"\n#define WV_SHAFTS\n#define WV_CURVATURE_TEXELS " + std::to_string(OceanWaves::Resolution) + ".0\n" +
				binding(shaftBindings.curvature) + "uniform sampler2D wvCurvature0;\n" +
				binding(shaftBindings.curvature + 1U) + "uniform sampler2D wvCurvature1;\n" +
				binding(shaftBindings.curvature + 2U) + "uniform sampler2D wvCurvature2;\n" +
				binding(shaftBindings.shelter) + "uniform sampler2D wvShelter;\n" +
				binding(shaftBindings.shafts) + "uniform WvShaftBlock\n{\n\tvec4 sunSource;\n\tvec4 sunDirection;\n\tvec4 cameraForward;\n\tvec4 cascadeSizes;\n\tvec4 shelterRectangle;\n\tvec4 shelteredShares;\n\tvec4 tuning;\n} wvShafts;\n" +
				(movingSurface ? ShadowCascadesMoving : ShadowCascadesAtRest) +
				EMEN_MARCH_DITHER_GLSL +
				SeaCausticsGLSL::sourceGLSL() +
				ShaftsFunctions;
		}

		const auto source =
			std::string{Header} +
			"\n#define MENISCUS_TRANSMISSION " + std::to_string(MeniscusTransmission) + "\n" +
			surface +
			shaftsSource +
			ViewReconstructionGLSL::sourceGLSL(0, viewBinding) +
			FragmentShader;

		const std::string label = std::string{movingSurface ? "WV_Moving" : "WV_AtRest"} + (shafts ? "_Shafts" : "");

		const auto fragmentModule = this->renderer().shaderManager().getShaderModuleFromSourceCode(this->renderer().device(), label + "_FS", Saphir::ShaderType::FragmentShader, source);

		if ( fragmentModule == nullptr )
		{
			TraceError{ClassId} << "Failed to compile the fragment shader of the variant " << label << " !";

			return false;
		}

		variant.pipeline = this->createFullscreenPipeline(ClassId, label, vertexModule, fragmentModule, variant.pipelineLayout, m_outputTarget);

		if ( variant.pipeline == nullptr )
		{
			return false;
		}

		variant.perFrame = this->createPerFrameDescriptorSets(inputLayout, ClassId, label + "DescSet");

		if ( variant.perFrame.empty() )
		{
			return false;
		}

		if ( shafts )
		{
			variant.cascadeBlocks = this->createPerFrameUniformBuffers(sizeof(CSMCascadeBlock), ClassId, label + "_Cascades_UBO");
			variant.shaftBlocks = this->createPerFrameUniformBuffers(sizeof(ShaftBlock), ClassId, label + "_Shafts_UBO");

			return !variant.cascadeBlocks.empty() && !variant.shaftBlocks.empty();
		}

		return true;
	}

	bool
	WaterVolume::create (uint32_t width, uint32_t height) noexcept
	{
		if ( !m_outputTarget.create(this->renderer(), width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "WV_Output") )
		{
			TraceError{ClassId} << "Failed to create the output target !";

			return false;
		}

		return
			this->createVariant(m_atRest, false, false) && this->createVariant(m_moving, true, false) &&
			this->createVariant(m_atRestShafts, false, true) && this->createVariant(m_movingShafts, true, true);
	}

	void
	WaterVolume::destroy () noexcept
	{
		m_movingShafts = {};
		m_atRestShafts = {};
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
		const auto frameIndex = this->renderer().currentFrameIndex();

		const auto light = enteringLight(context);
		const auto & scattering = water->scattering();
		const auto extinction = water->extinction();
		const auto depthAttenuation = water->diffuseAttenuation(light.downwellingCosine);
		const auto sourceScale = context.preExposure / (4.0F * std::numbers::pi_v< float >);
		const std::array< float, 3 > sunSource{scattering[X] * light.sun[X] * sourceScale, scattering[Y] * light.sun[Y] * sourceScale, scattering[Z] * light.sun[Z] * sourceScale};
		const std::array< float, 3 > skySource{scattering[X] * light.sky[X] * sourceScale, scattering[Y] * light.sky[Y] * sourceScale, scattering[Z] * light.sky[Z] * sourceScale};

		/* The sun's shafts: the marched variant when the frame has what it reads, else the closed form with the sun in
		 * the one source. */
		const auto & marched = movingSurface ? m_movingShafts : m_atRestShafts;
		const bool shafts = shaftsWanted(context) && frameIndex < marched.perFrame.size() && this->bindShafts(marched, movingSurface, context, sunSource);
		const auto & closedForm = movingSurface ? m_moving : m_atRest;
		const auto & variant = shafts ? marched : closedForm;

		if ( frameIndex >= variant.perFrame.size() )
		{
			return false;
		}

		const PushConstants pushConstants{
			.extinction = {extinction[X], extinction[Y], extinction[Z], seaLevel},
			.depthAttenuation = {depthAttenuation[X], depthAttenuation[Y], depthAttenuation[Z], surfaceReach},
			.source = {skySource[0] + (shafts ? 0.0F : sunSource[0]), skySource[1] + (shafts ? 0.0F : sunSource[1]), skySource[2] + (shafts ? 0.0F : sunSource[2]), MeniscusHalfWidth}
		};

		const auto & descriptorSet = *variant.perFrame[frameIndex];
		const auto common = variantBindings(movingSurface, shafts);
		const auto surfaceUniformsBinding = common.surfaceUniforms;
		const auto viewBinding = common.view;

		/* The ocean's resources of THIS frame (Graphics::SeaSurfaceWaves): what its own stages bind. */
		const auto surfaceBound = [&descriptorSet, waves, surfaceUniformsBinding] () noexcept {
			const VkDescriptorBufferInfo uniforms{
				.buffer = waves->uniforms->handle(),
				.offset = waves->uniformsOffset,
				.range = waves->uniformsRange
			};

			return
				descriptorSet.writeCombinedImageSampler(DisplacementBinding, *waves->displacement, *waves->sampler, VK_IMAGE_LAYOUT_GENERAL) &&
				descriptorSet.writeCombinedImageSampler(ShelterBinding, *waves->shelter, *waves->shelterSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) &&
				descriptorSet.writeUniformBuffer(surfaceUniformsBinding, uniforms);
		};

		const bool written =
			descriptorSet.writeCombinedImageSampler(0, inputColor) &&
			descriptorSet.writeCombinedImageSampler(1, *context.depth) &&
			(!movingSurface || surfaceBound()) &&
			descriptorSet.writeUniformBufferObject(viewBinding, *context.view);

		if ( !written )
		{
			TraceError{ClassId} << "Unable to bind the inputs of the water volume !";

			return false;
		}

		recordFullscreenPass(commandBuffer, m_outputTarget, *variant.pipeline, *variant.pipelineLayout, descriptorSet, &pushConstants, sizeof(PushConstants));

		return true;
	}

	bool
	WaterVolume::shaftsWanted (const FrameContext & context) noexcept
	{
		const auto * caustics = context.sea != nullptr ? context.sea->caustics() : nullptr;

		if ( caustics == nullptr || !(caustics->shaftStrength > 0.0F) || caustics->shelter == nullptr )
		{
			return false;
		}

		for ( const auto & curvature : caustics->curvature )
		{
			if ( curvature == nullptr )
			{
				return false;
			}
		}

		/* The sun: up, lit, and casting the cascaded shadows the march reads. */
		const auto mainLight = context.lightSet != nullptr ? context.lightSet->mainDirectionalLight() : nullptr;

		if ( mainLight == nullptr || !mainLight->isEnabled() || !mainLight->usesCSM() || !(mainLight->direction().normalized()[Y] < 0.0F) )
		{
			return false;
		}

		const auto shadowMap = mainLight->shadowMap();

		return shadowMap != nullptr && shadowMap->isReadyForRendering() && context.frameView != nullptr;
	}

	bool
	WaterVolume::bindShafts (const Variant & variant, bool movingSurface, const FrameContext & context, const std::array< float, 3 > & sunSource) const noexcept
	{
		const auto frameIndex = this->renderer().currentFrameIndex();

		if ( frameIndex >= variant.cascadeBlocks.size() || frameIndex >= variant.shaftBlocks.size() )
		{
			return false;
		}

		const auto & bindings = movingSurface ? ShaftsMoving : ShaftsAtRest;
		const auto & caustics = *context.sea->caustics();
		const auto mainLight = context.lightSet->mainDirectionalLight();
		const auto shadowMap = mainLight->shadowMap();
		const auto cascadeCount = std::min(mainLight->cascadeCount(), 4U);

		/* The sun's shadow cascades (Graphics/Effects/Shared/CSMSamplingGLSL.hpp owns the layout). */
		{
			/* A sun that says it casts cascaded shadows and whose map holds no cascade is refused, not trusted. */
			const auto * cascadedView = dynamic_cast< const ViewMatricesCascadedUBO * >(&std::as_const(*shadowMap).viewMatrices());

			if ( cascadedView == nullptr )
			{
				return false;
			}

			CSMCascadeBlock cascadeBlock;
			auto * splitDistance = cascadeBlock.splitDistances.data();

			for ( uint32_t cascade = 0; cascade < cascadeCount; ++cascade )
			{
				const auto & matrix = cascadedView->cascadeViewProjectionMatrix(cascade);

				std::memcpy(std::next(cascadeBlock.matrices.data(), static_cast< std::ptrdiff_t >(cascade) * 16), matrix.data(), 16 * sizeof(float));

				*splitDistance = cascadedView->splitDistance(cascade);
				splitDistance = std::next(splitDistance);
			}

			if ( !updateUniformBufferData(*variant.cascadeBlocks[frameIndex], &cascadeBlock, sizeof(cascadeBlock)) )
			{
				return false;
			}
		}

		/* The direction the sun's light travels in the water: Snell at a level surface, the horizontal part divided
		 * by the water's index. The forward axis of the frame's view: the negated third row of its matrix. */
		const auto inAir = mainLight->direction().normalized();
		const auto refractedX = inAir[X] / Scenes::WaterMedium::RefractiveIndex;
		const auto refractedZ = inAir[Z] / Scenes::WaterMedium::RefractiveIndex;
		const auto refractedY = -std::sqrt(std::max(1.0F - ((refractedX * refractedX) + (refractedZ * refractedZ)), 0.0F));
		const auto & viewMatrix = context.frameView->viewMatrix();

		const ShaftBlock block{
			.sunSource = {sunSource[0], sunSource[1], sunSource[2], static_cast< float >(ShaftSampleCount)},
			.sunDirection = {refractedX, refractedY, refractedZ, caustics.shaftAnisotropy},
			.cameraForward = {-viewMatrix(2, 0), -viewMatrix(2, 1), -viewMatrix(2, 2), static_cast< float >(cascadeCount)},
			.cascadeSizes = {caustics.cascadeSizes[0], caustics.cascadeSizes[1], caustics.cascadeSizes[2], caustics.strength},
			.shelterRectangle = caustics.shelterRectangle,
			.shelteredShares = {caustics.shelteredShares[0], caustics.shelteredShares[1], caustics.shelteredShares[2], caustics.softening},
			.tuning = {caustics.fadeDepth, mainLight->shadowBias(), caustics.shaftStrength, 0.0F}
		};

		if ( !updateUniformBufferData(*variant.shaftBlocks[frameIndex], &block, sizeof(block)) )
		{
			return false;
		}

		const auto & descriptorSet = *variant.perFrame[frameIndex];

		return
			shadowMap->writeCombinedImageSampler(descriptorSet, bindings.shadow) &&
			descriptorSet.writeCombinedImageSampler(bindings.curvature, *caustics.curvature[0]) &&
			descriptorSet.writeCombinedImageSampler(bindings.curvature + 1U, *caustics.curvature[1]) &&
			descriptorSet.writeCombinedImageSampler(bindings.curvature + 2U, *caustics.curvature[2]) &&
			descriptorSet.writeCombinedImageSampler(bindings.shelter, *caustics.shelter) &&
			descriptorSet.writeUniformBufferObject(bindings.cascades, *variant.cascadeBlocks[frameIndex]) &&
			descriptorSet.writeUniformBufferObject(bindings.shafts, *variant.shaftBlocks[frameIndex]);
	}
}
