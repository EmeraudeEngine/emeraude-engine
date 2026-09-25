/*
 * src/Graphics/Effects/Atmosphere/VolumetricClouds.cpp
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

#include "VolumetricClouds.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>

/* Local inclusions. */
#include "Algorithms/WorleyNoise.hpp"
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/CloudShapeResource.hpp"
#include "Graphics/Effects/Shared/CSMSamplingGLSL.hpp"
#include "Graphics/Effects/Shared/CloudVolumeGLSL.hpp"
#include "Graphics/Effects/Shared/MarchDitherGLSL.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesCascadedUBO.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Scenes/CloudSet.hpp"
#include "Scenes/Component/CloudVolume.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/LightSet.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/MemoryRegion.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/UniformBufferObject.hpp"

static constexpr auto TracerTag{"VolumetricCloudsEffect"};

namespace
{
	using namespace EmEn;

	/* ---- GLSL Shader Sources ----
	 * The body is shared by the two variants; the C++ side prepends `#version`, the extension and the
	 * variant defines. Bindings, set 0:
	 *   with cascades:    0 scene, 1 depth, 2 detail noise, 3 cascaded shadow map, 4 frame block, 5 cascade block
	 *   without cascades: 0 scene, 1 depth, 2 detail noise,                        3 frame block
	 * Set 1 is the bindless table: the cloud SHAPES live in its 3D array, the baked irradiance cubemap
	 * in its reserved cube slot 1. */
	constexpr auto CloudsFragmentShaderBody = R"GLSL(
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;
/* The view transmittance through the clouds (R16F): read by the light shafts and the lens flare. */
layout(location = 1) out vec4 outTransmittance;

layout(set = 0, binding = 0) uniform sampler2D sceneTex;
layout(set = 0, binding = 1) uniform sampler2D depthTex;
layout(set = 0, binding = 2) uniform sampler3D detailNoiseTex;

layout(set = 1, binding = 2) uniform sampler3D textures3D[];
layout(set = 1, binding = 3) uniform samplerCube texturesCube[];
)GLSL" EMEN_CLOUD_VOLUME_GLSL R"GLSL(

/* Reserved bindless slot of the scene's baked irradiance cubemap (BindlessTextureManager). It stores
 * E/pi, normalized; the sky luminance scales it to nits and doubles as the "there is a sky" flag. */
const uint IrradianceCubemapSlot = 1u;

const int MaxHits = 8;
/* Must match VolumetricClouds::DetailNoiseCells. */
const float DetailNoiseCells = 4.0;
/* Below this transmittance nothing behind the cloud can show through the tone mapper. */
const float MinTransmittance = 0.01;
/* The view step grows with the distance, 1.5 % of it, from an eighth of the base step up to FarStepScale base
 * steps, and a step k base steps long samples the shape's mip log2(k) (viewStepLod()): the DISTANCE LOD, -9 %
 * toward the horizon on `terrain` (2026-09-25). Band-limited to the step, the field keeps the base step's ratio
 * to its voxels at any distance. ⚠️ 4x saved no more and showed a dither weave (the dither spans the step).
 * ⚠️⚠️ The contour rings the first attempt drew were NOT the step: an UNDITHERED box entry inside a step (see the
 * march) re-aligned every pixel.
 * docs/caution-points.md § The cloud march: a distance LOD that works. */
const float DistanceStepFactor = 0.015;
const float FarStepScale = 2.0;
/* ⚠️ In the dense part the step follows the OPTICAL depth instead: up to 0.4 per step. Measured on
 * `forest` (2026-09-24), a camera inside a 100 m cloud marched 0.28 m steps with six sun samples
 * each: 13.3 ms for a uniform white-out. The energy-conserving step is exact for a constant source,
 * so a thick step inside the uniform core costs nothing visible; the shell, where the density varies,
 * keeps its distance-driven fine steps (its extinction is low there). */
const float MaxStepOpticalDepth = 0.4;
const float EmPi = 3.14159265358979323846;

layout(set = 0, binding = EMEN_CLOUDS_FRAME_BINDING, std140) uniform EmCloudFrame
{
	mat4 inverseRelativeViewProjection;	/* Projection x view ROTATION, inverted: eye-relative, never world. */
	vec4 cameraPosition;	/* w = cloud count. */
	vec4 cameraForward;		/* w = step count. */
	vec4 sunDirection;		/* xyz = direction of PROPAGATION, w = light step count. */
	vec4 sunIlluminance;	/* rgb = colour x lux, 0 = no sun; w = shadow bias. */
	vec4 ambient;			/* x = sky luminance (nits), y = ground albedo, z = cascade count. */
	EmCloud clouds[MaxClouds];
} emFrame;

#ifdef EMEN_CLOUDS_CASCADES
)GLSL" EMEN_CSM_SAMPLING_GLSL(3, 5) R"GLSL(
#endif
)GLSL" EMEN_MARCH_DITHER_GLSL R"GLSL(

/* The view step at a distance t along the ray, for a cloud whose base step is baseStep. */
float
viewStep (float t, float baseStep)
{
	return clamp(t * DistanceStepFactor, baseStep * 0.125, baseStep * FarStepScale);
}

/* The shape mip a view step samples: 0 up to the base step, log2 of how many base steps it spans beyond. */
float
viewStepLod (float step, float baseStep)
{
	return log2(max(step / baseStep, 1.0));
}

/* Henyey-Greenstein phase function, normalised over the sphere.
 * L. G. Henyey & J. L. Greenstein, "Diffuse radiation in the galaxy", 1941. */
float
henyeyGreenstein (float cosTheta, float anisotropy)
{
	float g = clamp(anisotropy, -0.95, 0.95);
	float g2 = g * g;
	float denominator = 1.0 + g2 - 2.0 * g * cosTheta;

	return (1.0 - g2) / (4.0 * EmPi * pow(max(denominator, 1.0e-4), 1.5));
}

/* A strong forward lobe (the silver lining toward the sun) and a weak backward one (the glow of a
 * cloud lit from behind the viewer). 'scale' shrinks both anisotropies: the octaves of multiple
 * scattering are less directional than single scattering. */
float
cloudPhase (float cosTheta, float scale)
{
	return mix(henyeyGreenstein(cosTheta, -0.3 * scale), henyeyGreenstein(cosTheta, 0.8 * scale), 0.7);
}

/* The shape density eroded by the detail noise at the shell (Schneider & Vos 2015 remap): a voxel at
 * full density is untouched, a voxel of the ramp loses what the noise takes. The noise coordinate is
 * in SHAPE units, so it scales with the cloud — the look is kept. It drifts UPWARD: convection. */
float
erodedDensity (EmCloud cloud, vec3 box, float base)
{
	vec3 shapePosition = box * cloud.shape.xyz;
	vec3 noisePosition = shapePosition * (cloud.look.y / (2.0 * DetailNoiseCells)) - vec3(0.0, cloud.look.z, 0.0);
	float erosion = texture(detailNoiseTex, noisePosition).r * cloud.look.x;

	return clamp((base - erosion) / max(1.0 - erosion, 1.0e-3), 0.0, 1.0);
}

/* The radiance a cloud sample scatters toward the eye, per unit of extinction and before its albedo:
 * the sun seen through THIS cloud and past what occludes it outside (the cascades), through the
 * multiple-scattering octaves, plus the isotropic ambient. */
vec3
cloudLightRadiance (EmCloud cloud, uint shapeIndex, vec3 box, vec3 worldPosition, float viewDepth, bool hasSun, vec3 toSun, vec3 sunIlluminance, int lightStepCount, float octavePhase[4], vec3 groundRadiance, vec3 skyRadiance)
{
	vec3 lightRadiance = vec3(0.0);

	if ( hasSun )
	{
		/* The optical depth toward the sun INSIDE this cloud, on a coarser, smoother mip. */
		vec3 boxToSun = toBoxDirection(cloud, toSun);
		float extinction = cloud.centerAndExtinction.w;
		float sunExit = max(intersectBox(box, boxToSun).y, 0.0);
		float lightStep = sunExit / float(lightStepCount);
		float sunOpticalDepth = 0.0;

		for ( int lightIndex = 0; lightIndex < lightStepCount; ++lightIndex )
		{
			vec3 lightBox = box + boxToSun * (lightStep * (float(lightIndex) + 0.5));

			sunOpticalDepth += textureLod(textures3D[nonuniformEXT(shapeIndex)], lightBox * 0.5 + 0.5, 1.5).r * extinction * lightStep;
		}

		float visibility = 1.0;

#ifdef EMEN_CLOUDS_CASCADES
		/* What occludes the sun OUTSIDE the cloud: the trees, the terrain. */
		visibility = emCSMVisibility(worldPosition, viewDepth, int(emFrame.ambient.z), emFrame.sunIlluminance.w);
#endif

		/* Multiple scattering, Wrenninge et al. 2013: every octave halves the energy, the extinction
		 * seen toward the light and the anisotropy (a <= b keeps it conservative). */
		float sunScattering = 0.0;
		float octaveWeight = 1.0;
		float octaveExtinction = 1.0;

		for ( int octave = 0; octave < 4; ++octave )
		{
			sunScattering += octaveWeight * exp(-sunOpticalDepth * octaveExtinction) * octavePhase[octave];
			octaveWeight *= 0.5;
			octaveExtinction *= 0.5;
		}

		lightRadiance += sunIlluminance * (visibility * sunScattering);
	}

	/* The isotropic ambient integrates to itself over the sphere. ⚠️ The sky is read at the ZENITH on purpose:
	 * a directional read (the dome each side of the cloud faces, 2026-09-25) changed NOTHING on `terrain` (cloud
	 * colour identical to 0.1/255) for +0.6 ms — a store sky's hue barely varies with the direction, and the sun
	 * dominates a lit cloud ~7:1. docs/caution-points.md § The cloud march: a distance LOD that works. */
	return lightRadiance + mix(groundRadiance, skyRadiance, clamp(box.y * 0.5 + 0.5, 0.0, 1.0));
}

void main()
{
	vec3 sceneColor = texture(sceneTex, vUV).rgb;
	float depth = texture(depthTex, vUV).r;

	/* ⚠️ The matrix form: the inverse view-projection of the frame that produced this depth buffer
	 * (read state), so the Y flip of the projection comes for free — no signed tanHalfFovY here.
	 * ⚠️⚠️ CAMERA-RELATIVE (view ROTATION only): the unprojected points are offsets from the eye,
	 * never world positions. The inverse of the full view-projection, in float, carried the camera
	 * translation into a near/far = 1e5 projection: the far point's w (1/far) is the difference of
	 * two ±1/near terms, and its error moved the reconstructed rays by ±1-2 px at 1.3 km from the
	 * world origin, ±5-13 px at 7.8 km, differently at every pose — the `terrain` clouds slid
	 * against the relief whenever the camera turned or moved (2026-09-25). */
	vec2 ndc = vUV * 2.0 - 1.0;
	vec3 cameraPosition = emFrame.cameraPosition.xyz;
	vec4 farPoint = emFrame.inverseRelativeViewProjection * vec4(ndc, 1.0, 1.0);
	vec3 rayDirection = normalize(farPoint.xyz / farPoint.w);

	/* The march stops at the surface: a tree inside a cloud is buried in it, not drawn over it.
	 * ⚠️⚠️ The sky is the CLEAR value, 1.0 exactly (the background writes no depth). A threshold
	 * such as 0.9999 is a DISTANCE in disguise: the depth is conventional, so 1 - depth ≈ near/z,
	 * and with the 0.089 m near plane everything past ~890 m read as sky — the clouds of `terrain`
	 * were drawn over every mountain they stood behind (2026-09-25). */
	float sceneDistance = 3.0e38;

	if ( depth < 1.0 )
	{
		vec4 surface = emFrame.inverseRelativeViewProjection * vec4(ndc, depth, 1.0);

		sceneDistance = length(surface.xyz / surface.w);
	}

	/* ---- The clouds this ray crosses, sorted front to back. ---- */
	float hitEnter[MaxHits];
	float hitExit[MaxHits];
	int hitCloud[MaxHits];
	int hitCount = 0;

	int cloudCount = min(int(emFrame.cameraPosition.w), MaxClouds);

	for ( int cloudIndex = 0; cloudIndex < cloudCount; ++cloudIndex )
	{
		vec2 range = intersectBox(toBox(emFrame.clouds[cloudIndex], cameraPosition), toBoxDirection(emFrame.clouds[cloudIndex], rayDirection));

		range.x = max(range.x, 0.0);
		range.y = min(range.y, sceneDistance);

		if ( range.y <= range.x )
		{
			continue;
		}

		/* Full: keep the nearest ones, the farthest is the least visible. */
		if ( hitCount == MaxHits )
		{
			if ( range.x >= hitEnter[MaxHits - 1] )
			{
				continue;
			}

			hitCount = MaxHits - 1;
		}

		int slot = hitCount;

		while ( slot > 0 && hitEnter[slot - 1] > range.x )
		{
			hitEnter[slot] = hitEnter[slot - 1];
			hitExit[slot] = hitExit[slot - 1];
			hitCloud[slot] = hitCloud[slot - 1];
			--slot;
		}

		hitEnter[slot] = range.x;
		hitExit[slot] = range.y;
		hitCloud[slot] = cloudIndex;
		++hitCount;
	}

	if ( hitCount == 0 )
	{
		outColor = vec4(sceneColor, 1.0);
		outTransmittance = vec4(1.0);

		return;
	}

	/* ---- Per-pixel constants. ---- */
	vec3 sunDirection = emFrame.sunDirection.xyz;
	vec3 toSun = -sunDirection;
	vec3 sunIlluminance = emFrame.sunIlluminance.rgb;
	bool hasSun = max(max(sunIlluminance.r, sunIlluminance.g), sunIlluminance.b) > 0.0;

	/* The light propagates along sunDirection and leaves toward the camera (-rayDirection). */
	float cosTheta = -dot(sunDirection, rayDirection);

	/* The phase of each multiple-scattering octave depends on the pixel only. */
	float octavePhase[4];
	float anisotropyScale = 1.0;

	for ( int octave = 0; octave < 4; ++octave )
	{
		octavePhase[octave] = cloudPhase(cosTheta, anisotropyScale);
		anisotropyScale *= 0.5;
	}

	/* The ambient: the sky over the cloud, the ground bounce under it (L = albedo . E / pi). */
	float skyLuminance = emFrame.ambient.x;
	vec3 skyRadiance = skyLuminance > 0.0 ? textureLod(texturesCube[IrradianceCubemapSlot], vec3(0.0, 1.0, 0.0), 0.0).rgb * skyLuminance : vec3(0.0);
	vec3 groundIrradiance = sunIlluminance * max(-sunDirection.y, 0.0) + skyRadiance * EmPi;

	/* The sky's HUE, normalised by its brightest channel so a tint never boosts one (Look::skyTint). */
	float skyPeak = max(max(skyRadiance.r, skyRadiance.g), skyRadiance.b);
	vec3 skyHue = skyPeak > 0.0 ? skyRadiance / skyPeak : vec3(1.0);
	vec3 groundRadiance = emFrame.ambient.y * groundIrradiance / EmPi;

	int stepCount = max(int(emFrame.cameraForward.w), 1);
	int lightStepCount = max(int(emFrame.sunDirection.w), 1);
	float viewDepthPerMetre = dot(rayDirection, emFrame.cameraForward.xyz);

	/* ⚠️ Static per pixel, one step's worth — the engine rule (MarchDitherGLSL.hpp). */
	float dither = emInterleavedGradientNoise(gl_FragCoord.xy);

	/* ---- ONE march over the UNION of the crossed boxes. ----
	 * ⚠️⚠️ Overlapping clouds are ONE medium: at every step, every cloud whose interval holds the sample
	 * adds its extinction and its source. The first version marched the sorted clouds one after the
	 * other, and two overlapping clouds swapped their compositing order along the line where their box
	 * entries meet — a STRAIGHT SEAM through both, seen on `forest` once the clouds were made 70-130 m
	 * wide and started to overlap (2026-09-24). */
	float hitBaseStep[MaxHits];
	float tEnd = 0.0;

	for ( int hit = 0; hit < hitCount; ++hit )
	{
		EmCloud cloud = emFrame.clouds[hitCloud[hit]];

		hitBaseStep[hit] = 2.0 * length(vec3(cloud.axisX.w, cloud.axisY.w, cloud.axisZ.w)) / float(stepCount);
		tEnd = max(tEnd, hitExit[hit]);
	}

	float transmittance = 1.0;
	vec3 inscatter = vec3(0.0);

	float t = hitEnter[0] + dither * viewStep(hitEnter[0], hitBaseStep[0]);

	for ( int iteration = 0; iteration < stepCount * 6 && t < tEnd; ++iteration )
	{
		vec3 samplePosition = cameraPosition + rayDirection * t;

		float stepLength = 3.0e38;
		float nextEnter = tEnd;
		float nextEnterStep = 0.0;
		float sigmaT = 0.0;
		/* Sum of sigma_s . L over the clouds holding the sample, in nits per metre. */
		vec3 source = vec3(0.0);
		bool inside = false;

		for ( int hit = 0; hit < hitCount; ++hit )
		{
			if ( t < hitEnter[hit] )
			{
				if ( hitEnter[hit] < nextEnter )
				{
					nextEnter = hitEnter[hit];
					nextEnterStep = hitBaseStep[hit];
				}

				continue;
			}

			if ( t >= hitExit[hit] )
			{
				continue;
			}

			inside = true;

			EmCloud cloud = emFrame.clouds[hitCloud[hit]];
			uint shapeIndex = uint(cloud.shape.w + 0.5);
			float cloudStep = viewStep(t, hitBaseStep[hit]);
			float shapeLod = viewStepLod(cloudStep, hitBaseStep[hit]);
			vec3 box = toBox(cloud, samplePosition);

			vec2 shape = textureLod(textures3D[nonuniformEXT(shapeIndex)], box * 0.5 + 0.5, shapeLod).rg;

			if ( shape.r <= 0.0 )
			{
				/* ⚠️ The skip channel at mip 0 ONLY: an average of distances is not a distance. Empty at a coarse mip is
				 * empty at mip 0 (every child is), so only the distance is read again, and only in the far field. */
				float skip = shapeLod > 0.0 ? textureLod(textures3D[nonuniformEXT(shapeIndex)], box * 0.5 + 0.5, 0.0).g : shape.g;

				stepLength = min(stepLength, max(skip * MaxSkipDistance * cloud.look.w, cloudStep));

				continue;
			}

			float density = erodedDensity(cloud, box, shape.r);

			if ( density <= 0.0 )
			{
				stepLength = min(stepLength, cloudStep);

				continue;
			}

			float cloudSigmaT = density * cloud.centerAndExtinction.w;

			stepLength = min(stepLength, min(max(cloudStep, MaxStepOpticalDepth / cloudSigmaT), max(cloudStep, hitBaseStep[hit])));

			sigmaT += cloudSigmaT;
			source += cloudSigmaT * cloud.albedo.rgb * mix(vec3(1.0), skyHue, cloud.albedo.w) * cloudLightRadiance(cloud, shapeIndex, box, samplePosition, t * viewDepthPerMetre, hasSun, toSun, sunIlluminance, lightStepCount, octavePhase, groundRadiance, skyRadiance);
		}

		/* Between two boxes: jump to the next one, dithered like the first entry. */
		if ( !inside )
		{
			t = nextEnter + dither * viewStep(nextEnter, nextEnterStep);

			continue;
		}

		/* A box starting inside this step would lose its front: stop at its entry — DITHERED like every other entry.
		 * ⚠️ Stopping exactly on it re-aligned every pixel on the same sample positions from there on: the dither is a
		 * phase along the ray, and overlapping boxes (every cloud at the horizon) lost it at each entry, which drew
		 * contour rings one step apart on the far clouds (2026-09-25). */
		if ( nextEnter > t )
		{
			stepLength = min(stepLength, max(nextEnter - t + dither * viewStep(nextEnter, nextEnterStep), 1.0e-3));
		}

		if ( sigmaT > 0.0 )
		{
			/* Energy-conserving integration of the source over the step (Hillaire 2016). */
			float stepTransmittance = exp(-sigmaT * stepLength);

			inscatter += transmittance * (source / sigmaT) * (1.0 - stepTransmittance);
			transmittance *= stepTransmittance;

			if ( transmittance <= MinTransmittance )
			{
				break;
			}
		}

		t += stepLength;
	}

	outColor = vec4(sceneColor * transmittance + inscatter, 1.0);
	outTransmittance = vec4(transmittance);
}
)GLSL";

	/**
	 * @brief Assembles one variant of the fragment shader.
	 * @param withCascades Whether the variant samples the cascaded shadow map.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string
	cloudsFragmentShader (bool withCascades) noexcept
	{
		std::string source{"#version 450\n#extension GL_EXT_nonuniform_qualifier : require\n"};

		if ( withCascades )
		{
			source += "#define EMEN_CLOUDS_CASCADES 1\n#define EMEN_CLOUDS_FRAME_BINDING 4\n";
		}
		else
		{
			source += "#define EMEN_CLOUDS_FRAME_BINDING 3\n";
		}

		source += CloudsFragmentShaderBody;

		return source;
	}
}

namespace EmEn::Graphics::Effects::Atmosphere
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	/* ---- Lifecycle ---- */

	bool
	VolumetricClouds::create (uint32_t width, uint32_t height) noexcept
	{
		if ( this->renderer().bindlessTextureManager().descriptorSetLayout() == nullptr )
		{
			TraceError{TracerTag} << "The bindless texture table is not available: the cloud shapes cannot be read !";

			return false;
		}

		if ( !m_outputTarget.create(this->renderer(), width, height, VK_FORMAT_R16G16B16A16_SFLOAT, "VC_Output") )
		{
			TraceError{TracerTag} << "Failed to create output target !";

			return false;
		}

		if ( !m_transmittanceTarget.create(this->renderer(), width, height, VK_FORMAT_R16_SFLOAT, "VC_Transmittance") )
		{
			TraceError{TracerTag} << "Failed to create the transmittance target !";

			return false;
		}

		if ( !this->createTargetsPass() )
		{
			return false;
		}

		if ( !this->createDetailNoise() )
		{
			return false;
		}

		if ( !this->createVariant(m_withCascades, true) || !this->createVariant(m_withoutCascades, false) )
		{
			return false;
		}

		m_frameUBOs = this->createPerFrameUniformBuffers(sizeof(FrameBlock), ClassId, "VC_Frame_UBO");
		m_cascadeUBOs = this->createPerFrameUniformBuffers(sizeof(CSMCascadeBlock), ClassId, "VC_Cascade_UBO");

		return !m_frameUBOs.empty() && !m_cascadeUBOs.empty();
	}

	bool
	VolumetricClouds::createVariant (Variant & variant, bool withCascades) noexcept
	{
		/* Samplers first, then the uniform buffers: 3 or 4 samplers, 1 or 2 blocks. */
		auto inputLayout = withCascades ? this->getInputLayout(4, 2) : this->getInputLayout(3, 1);

		if ( inputLayout == nullptr )
		{
			return false;
		}

		{
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
			sets.emplace_back(inputLayout);
			/* Set 1 = the bindless table, where recordFullscreenPass() binds it. */
			sets.emplace_back(this->renderer().bindlessTextureManager().descriptorSetLayout());

			variant.layout = this->renderer().layoutManager().getPipelineLayout(sets, {});
		}

		if ( variant.layout == nullptr )
		{
			return false;
		}

		const auto vertexModule = this->getFullscreenVertexShader();

		if ( vertexModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile vertex shader !";

			return false;
		}

		const auto name = std::string{withCascades ? "VC_Clouds_CSM" : "VC_Clouds"};
		const auto fragmentModule = this->renderer().shaderManager().getShaderModuleFromSourceCode(this->renderer().device(), name + "_FS", Saphir::ShaderType::FragmentShader, cloudsFragmentShader(withCascades));

		if ( fragmentModule == nullptr )
		{
			TraceError{TracerTag} << "Failed to compile the '" << name << "' fragment shader !";

			return false;
		}

		/* Two attachments: the colour and the transmittance. */
		variant.pipeline = this->createFullscreenPipeline(ClassId, name, vertexModule, fragmentModule, variant.layout, m_targetsRenderPass, m_outputTarget.width(), m_outputTarget.height(), 2);

		if ( variant.pipeline == nullptr )
		{
			return false;
		}

		variant.perFrame = this->createPerFrameDescriptorSets(inputLayout, ClassId, name + "_DescSet");

		return !variant.perFrame.empty();
	}

	bool
	VolumetricClouds::createTargetsPass () noexcept
	{
		auto & renderer = this->renderer();

		auto renderPass = std::make_shared< RenderPass >(renderer.device());
		renderPass->setIdentifier(ClassId, "VC_Targets", "RenderPass");

		RenderSubPass subPass;

		uint32_t attachmentIndex = 0;

		for ( const auto * target : {&m_outputTarget, &m_transmittanceTarget} )
		{
			renderPass->addAttachmentDescription(VkAttachmentDescription{
				.flags = 0,
				.format = target->format(),
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			});

			subPass.addColorAttachment(attachmentIndex++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		}

		renderPass->addSubPass(subPass);

		/* ⚠️ NOT by-region, either way: the next effect samples the colour non-locally, the light
		 * shafts gather the transmittance at half resolution and the flare probes it around the sun
		 * (IntermediateRenderTarget::createRenderPass() holds the full reasoning). */
		renderPass->addSubPassDependency(VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		});

		renderPass->addSubPassDependency(VkSubpassDependency{
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = 0
		});

		if ( !renderPass->createOnHardware() )
		{
			TraceError{TracerTag} << "Failed to create the colour + transmittance render pass !";

			return false;
		}

		auto framebuffer = std::make_shared< Framebuffer >(renderPass, VkExtent2D{m_outputTarget.width(), m_outputTarget.height()});
		framebuffer->setIdentifier(ClassId, "VC_Targets", "Framebuffer");
		framebuffer->addAttachment(m_outputTarget.imageView()->handle());
		framebuffer->addAttachment(m_transmittanceTarget.imageView()->handle());

		if ( !framebuffer->createOnHardware() )
		{
			TraceError{TracerTag} << "Failed to create the colour + transmittance framebuffer !";

			return false;
		}

		m_targetsRenderPass = std::move(renderPass);
		m_framebuffer = std::move(framebuffer);

		return true;
	}

	bool
	VolumetricClouds::createDetailNoise () noexcept
	{
		/* The billowy cellular noise of the shell erosion, TILEABLE (WorleyNoise wraps its lattice),
		 * baked once per effect: 32³ voxels holding three octaves over DetailNoiseCells cells. */
		constexpr auto Size = DetailNoiseSize;
		constexpr auto Octaves = 3U;

		const Algorithms::WorleyNoise< float > noise{0xC10D5EEDU, DetailNoiseCells};

		std::vector< uint8_t > voxels(static_cast< size_t >(Size) * Size * Size);

		const auto cellsPerVoxel = static_cast< float >(DetailNoiseCells) / static_cast< float >(Size);

		for ( uint32_t z = 0; z < Size; ++z )
		{
			for ( uint32_t y = 0; y < Size; ++y )
			{
				for ( uint32_t x = 0; x < Size; ++x )
				{
					const auto value = noise.generateBillows(
						(static_cast< float >(x) + 0.5F) * cellsPerVoxel,
						(static_cast< float >(y) + 0.5F) * cellsPerVoxel,
						(static_cast< float >(z) + 0.5F) * cellsPerVoxel,
						Octaves
					);

					voxels[(static_cast< size_t >(z) * Size + y) * Size + x] = static_cast< uint8_t >(std::lround(std::clamp(value, 0.0F, 1.0F) * 255.0F));
				}
			}
		}

		auto & renderer = this->renderer();

		const auto mipLevels = static_cast< uint32_t >(std::floor(std::log2(static_cast< float >(Size)))) + 1U;

		m_detailNoiseImage = std::make_shared< Image >(
			renderer.device(),
			VK_IMAGE_TYPE_3D,
			VK_FORMAT_R8_UNORM,
			VkExtent3D{Size, Size, Size},
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			0,
			mipLevels
		);
		m_detailNoiseImage->setIdentifier(ClassId, "DetailNoise", "Image");

		if ( !m_detailNoiseImage->createOnHardware() || !m_detailNoiseImage->writeData(renderer.transferManager(), MemoryRegion{voxels.data(), voxels.size()}) )
		{
			TraceError{TracerTag} << "Unable to create the detail noise texture !";

			return false;
		}

		m_detailNoiseView = std::make_shared< ImageView >(
			m_detailNoiseImage,
			VK_IMAGE_VIEW_TYPE_3D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = mipLevels,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		);
		m_detailNoiseView->setIdentifier(ClassId, "DetailNoise", "ImageView");

		if ( !m_detailNoiseView->createOnHardware() )
		{
			TraceError{TracerTag} << "Unable to create the detail noise view !";

			return false;
		}

		/* REPEAT: the noise tiles, and the cloud samples it far outside [0, 1]. */
		m_detailNoiseSampler = renderer.getSampler("CloudDetailNoise", [] (Settings & /*settings*/, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.mipLodBias = 0.0F;
			createInfo.anisotropyEnable = VK_FALSE;
			createInfo.maxAnisotropy = 1.0F;
			createInfo.compareEnable = VK_FALSE;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = VK_LOD_CLAMP_NONE;
		});

		return m_detailNoiseSampler != nullptr;
	}

	void
	VolumetricClouds::destroy () noexcept
	{
		m_frameUBOs.clear();
		m_cascadeUBOs.clear();

		for ( auto * variant : {&m_withCascades, &m_withoutCascades} )
		{
			variant->perFrame.clear();
			variant->pipeline.reset();
			variant->layout.reset();
		}

		if ( m_detailNoiseView != nullptr )
		{
			m_detailNoiseView->destroyFromHardware();
			m_detailNoiseView.reset();
		}

		if ( m_detailNoiseImage != nullptr )
		{
			m_detailNoiseImage->destroyFromHardware();
			m_detailNoiseImage.reset();
		}

		/* NOTE: The sampler belongs to the renderer's shared cache — only release our reference. */
		m_detailNoiseSampler.reset();

		m_framebuffer.reset();
		m_targetsRenderPass.reset();
		m_transmittanceTarget.destroy();
		m_outputTarget.destroy();
	}

	VolumetricClouds::Census
	VolumetricClouds::gatherClouds (const Scenes::CloudSet & clouds, uint32_t readStateIndex, float time, std::array< CloudBlock, MaxClouds > & blocks) noexcept
	{
		Census census;

		clouds.forEach([&] (const Scenes::Component::CloudVolume & cloud) {
			const auto shapeIndex = cloud.shapeBindlessIndex();
			const auto shape = cloud.shape();

			/* Still growing on the thread pool, or failed: not drawn this frame. */
			if ( shapeIndex == Scenes::Component::CloudVolume::NoShapeIndex )
			{
				++census.withoutShapeSlot;

				return;
			}

			if ( shape == nullptr || !shape->isCreated() )
			{
				++census.shapeNotOnGPU;

				return;
			}

			if ( census.drawn >= MaxClouds )
			{
				census.overflow = true;

				return;
			}

			const auto & state = cloud.renderState(readStateIndex);
			const auto & frame = cloud.parentEntity().getWorldCoordinatesStateForRendering(readStateIndex);
			const auto & scale = frame.scalingFactor();

			const Vector< 3, float > worldHalfExtents{
				state.halfExtents[X] * std::abs(scale[X]),
				state.halfExtents[Y] * std::abs(scale[Y]),
				state.halfExtents[Z] * std::abs(scale[Z])
			};

			if ( worldHalfExtents[X] <= 1.0e-4F || worldHalfExtents[Y] <= 1.0e-4F || worldHalfExtents[Z] <= 1.0e-4F )
			{
				++census.degenerateBox;

				return;
			}

			const auto & position = frame.position();
			const auto & axisX = frame.rightVector();
			const auto & axisY = frame.localYAxis();
			const auto & axisZ = frame.backwardVector();
			const auto proportions = shape->proportions();

			auto & gpuCloud = blocks[census.drawn];

			/* ⚠️⚠️ THE "KEEP THE LOOK" RULE (owner decision, 2026-09-24): the extinction follows the
			 * CURRENT world height, so the vertical optical depth stays opticalThickness whatever the
			 * scale. A fixed extinction in 1/m would turn a shrunk cumulus into mist. */
			gpuCloud.centerAndExtinction = {position[X], position[Y], position[Z], state.look.opticalThickness / (2.0F * worldHalfExtents[Y])};
			gpuCloud.axisX = {axisX[X], axisX[Y], axisX[Z], worldHalfExtents[X]};
			gpuCloud.axisY = {axisY[X], axisY[Y], axisY[Z], worldHalfExtents[Y]};
			gpuCloud.axisZ = {axisZ[X], axisZ[Y], axisZ[Z], worldHalfExtents[Z]};
			gpuCloud.shape = {proportions[X], proportions[Y], proportions[Z], static_cast< float >(shapeIndex)};

			/* The boiling offset is a PRODUCT of the frame time, never an accumulation, wrapped on the
			 * noise period so the float keeps its precision for hours. */
			const auto boilingOffset = std::fmod(time * state.look.boilingSpeed / static_cast< float >(DetailNoiseCells), 1.0F);

			const auto metresPerUnit = std::min({
				worldHalfExtents[X] / proportions[X],
				worldHalfExtents[Y] / proportions[Y],
				worldHalfExtents[Z] / proportions[Z]
			});

			gpuCloud.look = {std::clamp(state.look.erosion, 0.0F, 1.0F), state.look.detailFrequency, boilingOffset, metresPerUnit};
			gpuCloud.albedo = {state.look.scatteringAlbedo.red(), state.look.scatteringAlbedo.green(), state.look.scatteringAlbedo.blue(), std::clamp(state.look.skyTint, 0.0F, 1.0F)};

			++census.drawn;
		});

		return census;
	}

	/* ---- Execute ---- */

	const TextureInterface &
	VolumetricClouds::execute (const CommandBuffer & commandBuffer, const TextureInterface & inputColor, const FrameContext & context) noexcept
	{
		const auto * clouds = context.clouds;

		if ( clouds == nullptr || clouds->empty() || context.depth == nullptr )
		{
			return inputColor;
		}

		auto & renderer = this->renderer();
		const auto frameIndex = renderer.currentFrameIndex();

		/* ⚠️ readStateIndex everywhere: the view matrices that produced the depth buffer, and the
		 * entity frames the logic PUBLISHED for this frame — never the live logic state. */
		const auto readStateIndex = renderer.currentReadStateIndex();
		const auto & viewMatrices = renderer.mainRenderTarget()->viewMatrices();
		const auto & viewMatrix = viewMatrices.viewMatrix(readStateIndex, false, 0);
		const auto & projectionMatrix = viewMatrices.projectionMatrix(readStateIndex);
		/* ⚠️⚠️ The INFINITY view: the same rotation, no translation. See the shader's ray setup —
		 * the full view-projection inverted in float made the rays swim by up to 13 px. */
		const auto inverseRelativeViewProjection = (projectionMatrix * viewMatrices.viewMatrix(readStateIndex, true, 0)).inverse();
		const auto & cameraPosition = viewMatrices.position(readStateIndex);

		FrameBlock block{};

		std::memcpy(block.inverseRelativeViewProjection.data(), inverseRelativeViewProjection.data(), block.inverseRelativeViewProjection.size() * sizeof(float));

		/* Forward = the negated row 2 of the view matrix (row 2 stores -forward). */
		block.cameraForward = {-viewMatrix(2, 0), -viewMatrix(2, 1), -viewMatrix(2, 2), static_cast< float >(m_parameters.stepCount)};

		/* ---- The clouds, from their PUBLISHED state. ---- */
		const auto census = gatherClouds(*clouds, readStateIndex, context.constants.time, block.clouds);
		const auto cloudCount = census.drawn;

		if ( census.overflow && !m_tooManyCloudsReported )
		{
			m_tooManyCloudsReported = true;

			TraceWarning{TracerTag} << "The scene holds more than " << MaxClouds << " clouds: the extra ones are not drawn.";
		}

		/* ⚠️ A scene whose clouds never show says so HERE or nowhere: the pass is silent by
		 * construction (a skipped cloud is the normal state while its shape grows). Traced when the
		 * census changes, never per frame. */
		if ( const std::array< uint32_t, 4 > counts{census.drawn, census.withoutShapeSlot, census.shapeNotOnGPU, census.degenerateBox}; counts != m_lastCensus )
		{
			m_lastCensus = counts;

			TraceInfo{TracerTag} <<
				"Clouds drawn: " << census.drawn << " of " << clouds->count() << " (" <<
				census.withoutShapeSlot << " without a bindless shape slot yet, " <<
				census.shapeNotOnGPU << " whose shape is not on the GPU, " <<
				census.degenerateBox << " with a degenerate box).";
		}

		/* ⚠️ NO early-out on zero clouds drawn (their shapes still growing): the pass runs anyway and
		 * writes a transmittance of 1. Paired consumers (the light shafts, the lens flare) read that
		 * target on every frame the scene holds clouds; skipped here, they would read an image
		 * nobody wrote. */
		block.cameraPosition = {cameraPosition[X], cameraPosition[Y], cameraPosition[Z], static_cast< float >(cloudCount)};

		/* ---- The sun and the sky. A scene without a sun still has lit clouds: the sky lights them. ---- */
		const auto mainLight = context.lightSet != nullptr ? context.lightSet->mainDirectionalLight() : nullptr;
		auto withCascades = false;

		block.sunDirection = {0.0F, -1.0F, 0.0F, static_cast< float >(m_parameters.lightStepCount)};

		if ( mainLight != nullptr && mainLight->isEnabled() )
		{
			const auto direction = mainLight->direction().normalized();
			const auto illuminance = mainLight->illuminance();
			const auto & color = mainLight->emissionChromaticity();

			block.sunDirection = {direction[X], direction[Y], direction[Z], static_cast< float >(m_parameters.lightStepCount)};
			block.sunIlluminance = {color[X] * illuminance, color[Y] * illuminance, color[Z] * illuminance, mainLight->shadowBias()};

			const auto & shadowMap = mainLight->shadowMap();

			withCascades = mainLight->usesCSM() && shadowMap != nullptr && shadowMap->isReadyForRendering();

			if ( withCascades )
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

				block.ambient[2] = static_cast< float >(cascadeCount);

				if ( !updateUniformBufferData(*m_cascadeUBOs[frameIndex], &cascadeBlock, sizeof(cascadeBlock)) )
				{
					withCascades = false;
				}
			}
		}

		block.ambient[0] = context.skyLuminance;
		block.ambient[1] = std::clamp(m_parameters.groundAlbedo, 0.0F, 1.0F);

		if ( !updateUniformBufferData(*m_frameUBOs[frameIndex], &block, sizeof(block)) )
		{
			return inputColor;
		}

		/* ---- Bind and record. ---- */
		auto & variant = withCascades ? m_withCascades : m_withoutCascades;
		auto & descriptorSet = *variant.perFrame[frameIndex];

		static_cast< void >(descriptorSet.writeCombinedImageSampler(0, inputColor));
		static_cast< void >(descriptorSet.writeCombinedImageSampler(1, *context.depth));
		static_cast< void >(descriptorSet.writeCombinedImageSampler(2, *m_detailNoiseImage, *m_detailNoiseView, *m_detailNoiseSampler));

		if ( withCascades )
		{
			if ( !mainLight->shadowMap()->writeCombinedImageSampler(descriptorSet, 3) )
			{
				return inputColor;
			}

			static_cast< void >(descriptorSet.writeUniformBufferObject(4, *m_frameUBOs[frameIndex]));
			static_cast< void >(descriptorSet.writeUniformBufferObject(5, *m_cascadeUBOs[frameIndex]));
		}
		else
		{
			static_cast< void >(descriptorSet.writeUniformBufferObject(3, *m_frameUBOs[frameIndex]));
		}

		const std::array< const IntermediateRenderTarget *, 2 > targets{&m_outputTarget, &m_transmittanceTarget};

		IndirectPostProcessEffect::recordFullscreenPass(
			commandBuffer,
			*m_framebuffer,
			targets,
			*variant.pipeline,
			*variant.layout,
			descriptorSet,
			nullptr,
			0,
			renderer.bindlessTextureManager().descriptorSet()
		);

		return m_outputTarget;
	}
}
