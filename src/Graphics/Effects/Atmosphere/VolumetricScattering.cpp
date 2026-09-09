/*
 * src/Graphics/Effects/Atmosphere/VolumetricScattering.cpp
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

#include "VolumetricScattering.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstring>
#include <string>

/* Local inclusions. */
#include "Graphics/Effects/Shared/CSMSamplingGLSL.hpp"
#include "Graphics/Effects/Shared/MarchDitherGLSL.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesCascadedUBO.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/LightSet.hpp"
#include "Scenes/ParticipatingMedium.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/UniformBufferObject.hpp"

static constexpr auto TracerTag{"VolumetricScatteringEffect"};

/* Compile-time size check. */
static_assert(sizeof(EmEn::Graphics::Effects::Atmosphere::VolumetricScattering::ScatteringPushConstants) == 120, "ScatteringPushConstants must be exactly 120 bytes !");

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ---- */

	/* Bindings: 0 scene colour, 1 depth, 2 material properties, 3 cascaded shadow map,
	 * 4 the cascade block. The last two are declared by EMEN_CSM_SAMPLING_GLSL, which owns the
	 * whole sampling convention — never restate it here. */
	static constexpr auto ScatteringFragmentShader = R"GLSL(
#version 450

layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler2D materialPropsTex;

layout(push_constant) uniform PushConstants
{
	/* Camera basis. */
	float cameraPosX, cameraPosY, cameraPosZ;
	float cameraRightX, cameraRightY, cameraRightZ;
	float cameraForwardX, cameraForwardY, cameraForwardZ;
	/* Depth reconstruction. */
	float nearPlane, farPlane;
	float tanHalfFovY, aspectRatio;
	/* The medium. */
	float mediumDensity;
	float mediumHeightFalloff;
	float mediumBaseHeight;
	float mediumMaxDistance;
	float scatteringAlbedoR, scatteringAlbedoG, scatteringAlbedoB;
	float phaseAnisotropy;
	/* The light. */
	float lightDirX, lightDirY, lightDirZ;
	float lightIlluminanceR, lightIlluminanceG, lightIlluminanceB;
	/* March and shadow lookup. */
	float sampleCount;
	float cascadeCount;
	float shadowBias;
};
)GLSL" EMEN_CSM_SAMPLING_GLSL(3, 4) EMEN_MARCH_DITHER_GLSL R"GLSL(

const float EmPi = 3.14159265358979323846;

/* Henyey-Greenstein phase function, normalised over the sphere (integrates to 1 over 4*pi sr).
 * L. G. Henyey & J. L. Greenstein, "Diffuse radiation in the galaxy", 1941. */
float
henyeyGreenstein (float cosTheta, float anisotropy)
{
	float g = clamp(anisotropy, -0.95, 0.95);
	float g2 = g * g;
	float denominator = 1.0 + g2 - 2.0 * g * cosTheta;

	return (1.0 - g2) / (4.0 * EmPi * pow(max(denominator, 1.0e-4), 1.5));
}

void main()
{
	vec3 sceneColor = texture(sceneTex, vUV).rgb;
	float depth = texture(depthTex, vUV).r;

	vec2 ndc = vUV * 2.0 - 1.0;

	/* ⚠️ tanHalfFovY comes from the FrameContext and is NEVER recomputed here. It is a SIGNED
	 * contract (PostProcessor.cpp: `std::tan(...) * projectionYSign`, negative since the Y-up
	 * flip) and the sign is what carries the downward screen direction. Recomputing it locally
	 * throws the sign away and inverts rayDir.y — which, on a height-profiled medium, makes it
	 * grow DENSER with altitude. AtmosphericFog paid for exactly that. */
	float t = tanHalfFovY;

	vec3 cameraPosition = vec3(cameraPosX, cameraPosY, cameraPosZ);
	vec3 cameraRight = vec3(cameraRightX, cameraRightY, cameraRightZ);
	vec3 cameraForward = vec3(cameraForwardX, cameraForwardY, cameraForwardZ);
	/* Y-UP, right-handed: cross(right, forward) = cross(+X, -Z) = +Y, a genuine UP vector. */
	vec3 cameraUp = cross(cameraRight, cameraForward);

	vec3 rayDir = normalize(cameraRight * (ndc.x * abs(t) * aspectRatio) + cameraUp * (ndc.y * t) + cameraForward);

	/* How far to march: to the surface, or to the medium's own reach for a sky pixel.
	 * ⚠️ Unlike the screen-space god rays this replaces, there is NO off-screen gate: a
	 * world-space march scatters wherever the medium and the light both reach, including poses
	 * where the sun is behind the camera. Shafts appearing in such poses are correct, not a
	 * regression. */
	bool isSky = (depth >= 0.9999);
	float marchLength;

	if (isSky)
	{
		marchLength = mediumMaxDistance;
	}
	else
	{
		float z = depth * 2.0 - 1.0;
		float linearZ = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));

		float vx = ndc.x * abs(t) * aspectRatio * linearZ;
		float vy = ndc.y * t * linearZ;
		vec3 worldPosition = cameraPosition + cameraRight * vx + cameraUp * vy + cameraForward * linearZ;

		marchLength = min(length(worldPosition - cameraPosition), mediumMaxDistance);
	}

	/* ⚠️⚠️ Cap the march by the medium's OPTICAL reach, never by its maxDistance alone. The step
	 * size is marchLength / steps, and for a sky pixel marchLength IS maxDistance — 10 km on the
	 * reference scene. At 32 steps that is 312 m per step against a medium whose transmittance
	 * falls to 1/e in 67 m: the entire visible contribution lands inside the FIRST step, and the
	 * origin dither (a fraction of one step, by design) then scatters that single sample anywhere
	 * over 312 m per pixel. The result is full-amplitude per-pixel speckle — reported from the
	 * screen as "a pattern of black dots everywhere in the fog", and it is undersampling, not noise.
	 * Six extinction lengths leave 0.25 % of the light unaccounted for, which is far below what the
	 * tone mapper can show, and they bring the step back to a sane size (~15 m at the reference
	 * density) without touching the sample count. */
	float referenceDensity = mediumDensity * exp(-mediumHeightFalloff * (cameraPosY - mediumBaseHeight));

	if ( referenceDensity > 1.0e-6 )
	{
		marchLength = min(marchLength, 6.0 / referenceDensity);
	}

	int steps = int(max(sampleCount, 1.0));
	float stepLength = marchLength / float(steps);

	/* ⚠️ The march ORIGIN is dithered by a fraction of ONE step. Uniform steps do not produce
	 * noise, they produce coherent BANDING on any source smaller than a step, and banding is
	 * TAA-hostile — the rule and its measurements live in MarchDitherGLSL.hpp. */
	float ditherOffset = emInterleavedGradientNoise(gl_FragCoord.xy);

	/* Scattering angle: the light propagates along lightDir and leaves toward the camera, which is
	 * -rayDir, so cos(theta) = dot(lightDir, -rayDir). Forward scattering (g > 0) therefore peaks
	 * when looking TOWARD the source, which is the whole visual point of a shaft. */
	vec3 lightDir = vec3(lightDirX, lightDirY, lightDirZ);
	float cosTheta = -dot(lightDir, rayDir);
	float phase = henyeyGreenstein(cosTheta, phaseAnisotropy);

	vec3 scatteringAlbedo = vec3(scatteringAlbedoR, scatteringAlbedoG, scatteringAlbedoB);
	vec3 lightIlluminance = vec3(lightIlluminanceR, lightIlluminanceG, lightIlluminanceB);

	/* The radiance scattered toward the eye per unit of optical depth, before visibility. Units:
	 * lux / sr, so the accumulation below comes out in cd/m2 (nits) with no gain to calibrate. */
	vec3 scatteredRadiance = scatteringAlbedo * phase * lightIlluminance;

	int cascades = int(cascadeCount);

	float transmittance = 1.0;
	vec3 inscatter = vec3(0.0);

	for ( int index = 0; index < steps; ++index )
	{
		float distanceAlongRay = (float(index) + ditherOffset) * stepLength;
		vec3 samplePosition = cameraPosition + rayDir * distanceAlongRay;

		/* Extinction at this altitude. ⚠️ This is character-for-character the expression of
		 * Scenes::ParticipatingMedium::densityAt(), negation included: heightFalloff is a POSITIVE
		 * decay rate, so density must fall off going UP (+Y). The analytic form stays valid for
		 * either sign, which is exactly what made the same mistake silent in AtmosphericFog until
		 * the Y-up flip turned it into fog that thickened with altitude. */
		float sigmaT = mediumDensity * exp(-mediumHeightFalloff * (samplePosition.y - mediumBaseHeight));

		/* ⚠️ Cascade selection needs a VIEW-space depth and no fragment shader on the main render
		 * target can reach a view matrix (it travels as a VERTEX-stage push constant). It does not
		 * need one: the view depth of a point on the ray is its travelled distance projected onto
		 * the camera forward axis. */
		float viewDepth = distanceAlongRay * dot(rayDir, cameraForward);

		/* THE shadow lookup — the convention lives in CSMSamplingGLSL.hpp and nowhere else. This
		 * is what the closed-form fog is structurally unable to do: it knows sky from not-sky, and
		 * nothing about what occludes the volume. */
		float visibility = emCSMVisibility(samplePosition, viewDepth, cascades, shadowBias);

		/* Energy-conserving analytic integration of the source over the step (Hillaire, Frostbite
		 * 2015): (1 - stepTransmittance) is the fraction extinguished within the step, which is
		 * also what it scatters. Accumulating `source * stepLength` instead overshoots as the
		 * optical depth per step grows, and blows up outright on a dense medium. */
		float stepTransmittance = exp(-sigmaT * stepLength);

		inscatter += transmittance * scatteredRadiance * visibility * (1.0 - stepTransmittance);
		transmittance *= stepTransmittance;
	}

	/* Per-pixel medium response, from the material-properties A channel's high nibble — the SAME
	 * lane AtmosphericFog reads, so a surface authored immune to fog is immune to both integrators.
	 * Immunity means transmittance 1 and no in-scatter, not a half measure. */
	vec4 materialProperties = texture(materialPropsTex, vUV);
	uint packedAlpha = uint(materialProperties.a * 255.0);
	float mediumResponse = float(packedAlpha >> 4u) / 15.0;

	transmittance = mix(1.0, transmittance, mediumResponse);
	inscatter *= mediumResponse;

	/* ⚠️ The composite is a MULTIPLY on the chain colour plus an addition, not the pure add the
	 * screen-space god rays used. That is what single scattering IS: the medium both attenuates
	 * what is behind it and adds what it scatters. It is safe here because this effect owns both
	 * operands inside its own pass — it does not emit a combineContribution() snippet into the
	 * shared generated combine. */
	outColor = vec4(sceneColor * transmittance + inscatter, 1.0);
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
	VolumetricScattering::create (uint32_t width, uint32_t height) noexcept
	{
		if ( !m_outputTarget.create(this->renderer(), width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "VS_Output") )
		{
			TraceError{TracerTag} << "Failed to create output target !";

			return false;
		}

		/* ---- Descriptor set layout: 4 samplers (scene, depth, material properties, shadow map)
		 * plus 1 uniform buffer (the cascade block). Uniform buffers are laid out AFTER the
		 * samplers, so the block lands on binding 4 — which is what the shader declares. ---- */
		auto inputLayout = this->getInputLayout(4, 1);

		if ( inputLayout == nullptr )
		{
			return false;
		}

		/* ---- Pipeline layout ---- */
		{
			auto & layoutManager = this->renderer().layoutManager();

			const StaticVector< VkPushConstantRange, 4 > ranges{
				VkPushConstantRange{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ScatteringPushConstants)}
			};

			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(inputLayout);
			m_scatteringLayout = layoutManager.getPipelineLayout(sets, ranges);
		}

		if ( m_scatteringLayout == nullptr )
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

		const auto scatteringFragment = this->renderer().shaderManager().getShaderModuleFromSourceCode(this->renderer().device(), "VS_Scattering_FS", Saphir::ShaderType::FragmentShader, ScatteringFragmentShader);

		if ( scatteringFragment == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile scattering fragment shader !";

			return false;
		}

		/* ---- Create pipeline ---- */
		m_scatteringPipeline = this->createFullscreenPipeline(ClassId, "VS_Scattering", vertexModule, scatteringFragment, m_scatteringLayout, m_outputTarget);

		if ( m_scatteringPipeline == nullptr )
		{
			return false;
		}

		/* ---- Per-frame descriptor sets and cascade uniform buffers ---- */
		m_scatteringPerFrame = this->createPerFrameDescriptorSets(inputLayout, ClassId, "VSDescSet");

		if ( m_scatteringPerFrame.empty() )
		{
			return false;
		}

		m_cascadeUBOs = this->createPerFrameUniformBuffers(sizeof(CSMCascadeBlock), ClassId, "VS_Cascade_Frame_UBO");

		if ( m_cascadeUBOs.empty() )
		{
			return false;
		}

		return true;
	}

	void
	VolumetricScattering::destroy () noexcept
	{
		m_cascadeUBOs.clear();
		m_scatteringPerFrame.clear();
		m_scatteringPipeline.reset();
		m_scatteringLayout.reset();
		m_outputTarget.destroy();
	}

	/* ---- Execute ---- */

	const TextureInterface &
	VolumetricScattering::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto * inputDepth = context.depth;
		const auto * inputMaterialProperties = context.materialProperties;
		const auto * lightSet = context.lightSet;
		const auto & constants = context.constants;

		/* ⚠️ No medium, no scattering. A march through an atmosphere the scene never declared is a
		 * pass-through, not a march with invented parameters: there is exactly ONE place that
		 * describes the medium, and it is the scene. */
		const auto * medium = context.medium;

		if ( medium == nullptr )
		{
			if ( !m_missingMediumReported )
			{
				m_missingMediumReported = true;

				TraceWarning{TracerTag} << "The scene declares no participating medium: the volumetric scattering is a no-op. Set one with Scene::setParticipatingMedium().";
			}

			return inputColor;
		}

		/* ⚠️ This effect REQUIRES a CSM directional light, and passing through without one is the
		 * honest answer rather than a fallback. Its entire reason to exist over its slot sibling
		 * AtmosphericFog is the shadow lookup inside the volume; an unshadowed march is a more
		 * expensive way to compute what the closed-form fog already gives. */
		const auto mainLight = lightSet != nullptr ? lightSet->mainDirectionalLight() : nullptr;

		if ( mainLight == nullptr || !mainLight->usesCSM() || mainLight->shadowMap() == nullptr )
		{
			if ( !m_missingCSMLightReported )
			{
				m_missingCSMLightReported = true;

				TraceWarning{TracerTag} << "No cascaded-shadow-map directional light in the scene: the volumetric scattering is a no-op. Declare the sun with the CSM constructor, or use AtmosphericFog for an unshadowed medium.";
			}

			return inputColor;
		}

		const auto & shadowMap = mainLight->shadowMap();

		if ( !shadowMap->isReadyForRendering() )
		{
			return inputColor;
		}

		const auto frameIndex = this->renderer().currentFrameIndex();

		/* Camera basis. readStateIndex matches the view matrix that produced the depth buffer. */
		const auto readStateIndex = this->renderer().currentReadStateIndex();
		const auto & viewMatrices = this->renderer().mainRenderTarget()->viewMatrices();
		const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
		const auto & camPos = viewMatrices.position(readStateIndex);

		/* Right = row 0 of the view matrix; forward = negated row 2 (row 2 stores -forward). */
		const auto rX = viewMat(0, 0);
		const auto rY = viewMat(0, 1);
		const auto rZ = viewMat(0, 2);
		const auto fX = -viewMat(2, 0);
		const auto fY = -viewMat(2, 1);
		const auto fZ = -viewMat(2, 2);

		const auto lightDir = mainLight->direction().normalized();

		/* ⚠️⚠️ The source term takes the sun's ILLUMINANCE, in lux — NOT
		 * ParticipatingMedium::resolveLuminance(). That accessor is the contract of an effect that
		 * composites a LUMINANCE directly (AtmosphericFog: `mix(sceneColor, fogColor, fogAmount)`
		 * with fogColor = albedo * resolveLuminance), and with no authored luminance it returns
		 * E/pi. Feeding it to a radiative-transfer integral that already carries its own sigmaS and
		 * phase function is dimensionally wrong and under-reports by exactly pi. Measured before
		 * the fix on light-and-shadow-debug, pinned pose and exposure: mean luminance 188/255 with
		 * the E/pi form.
		 * ⚠️ Consequence to keep in mind rather than paper over: an authored
		 * ParticipatingMedium::setLuminance() has NO meaning for this integrator. It overrides a
		 * composited RESULT, and this pass computes the result instead of assuming it. Honouring it
		 * would mean inventing a convention the medium's contract does not define. */
		const auto lightIlluminance = mainLight->illuminance();
		const auto lightColor = mainLight->color();

		/* ---- Upload the cascade block. The four matrices are 256 bytes on their own, well past
		 * the 128-byte push-constant floor, so they travel in a per-frame UBO. ---- */
		{
			const auto & cascadedView = static_cast< ViewMatricesCascadedUBO & >(shadowMap->viewMatrices());
			const auto cascadeCount = std::min(mainLight->cascadeCount(), 4U);

			CSMCascadeBlock cascadeBlock;

			for ( uint32_t cascade = 0; cascade < cascadeCount; ++cascade )
			{
				const auto & matrix = cascadedView.cascadeViewProjectionMatrix(cascade);

				std::memcpy(&cascadeBlock.matrices[cascade * 16], matrix.data(), 16 * sizeof(float));

				cascadeBlock.splitDistances[cascade] = cascadedView.splitDistance(cascade);
			}

			if ( !updateUniformBufferData(*m_cascadeUBOs[frameIndex], &cascadeBlock, sizeof(cascadeBlock)) )
			{
				return inputColor;
			}
		}

		/* ---- Bind the inputs ---- */
		static_cast< void >(m_scatteringPerFrame[frameIndex]->writeCombinedImageSampler(0, inputColor));

		if ( inputDepth != nullptr )
		{
			static_cast< void >(m_scatteringPerFrame[frameIndex]->writeCombinedImageSampler(1, *inputDepth));
		}

		if ( inputMaterialProperties != nullptr )
		{
			static_cast< void >(m_scatteringPerFrame[frameIndex]->writeCombinedImageSampler(2, *inputMaterialProperties));
		}

		if ( !shadowMap->writeCombinedImageSampler(*m_scatteringPerFrame[frameIndex], 3) )
		{
			return inputColor;
		}

		static_cast< void >(m_scatteringPerFrame[frameIndex]->writeUniformBufferObject(4, *m_cascadeUBOs[frameIndex]));

		/* ---- Push constants ---- */
		const ScatteringPushConstants scatteringPC{
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
			.tanHalfFovY = constants.tanHalfFovY,
			.aspectRatio = this->renderer().mainRenderTarget()->viewMatrices().getAspectRatio(),
			.mediumDensity = medium->density(),
			.mediumHeightFalloff = medium->heightFalloff(),
			.mediumBaseHeight = medium->baseHeight(),
			.mediumMaxDistance = medium->maxDistance(),
			.scatteringAlbedoR = medium->scatteringAlbedo().red(),
			.scatteringAlbedoG = medium->scatteringAlbedo().green(),
			.scatteringAlbedoB = medium->scatteringAlbedo().blue(),
			.phaseAnisotropy = medium->phaseAnisotropy(),
			.lightDirX = lightDir.x(),
			.lightDirY = lightDir.y(),
			.lightDirZ = lightDir.z(),
			.lightIlluminanceR = lightColor.red() * lightIlluminance,
			.lightIlluminanceG = lightColor.green() * lightIlluminance,
			.lightIlluminanceB = lightColor.blue() * lightIlluminance,
			.sampleCount = static_cast< float >(m_parameters.sampleCount),
			.cascadeCount = static_cast< float >(std::min(mainLight->cascadeCount(), 4U)),
			.shadowBias = mainLight->shadowBias()
		};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			m_outputTarget,
			*m_scatteringPipeline,
			*m_scatteringLayout,
			*m_scatteringPerFrame[frameIndex],
			&scatteringPC,
			sizeof(ScatteringPushConstants)
		);

		return m_outputTarget;
	}
}
