/*
 * src/Graphics/SkyAtmosphere.cpp
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

#include "SkyAtmosphere.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <numbers>
#include <span>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/Effects/Shared/AtmosphereGLSL.hpp"
#include "Graphics/Renderer.hpp"
#include "Math/Astronomy.hpp"
#include "Saphir/ShaderManager.hpp"
#include "Saphir/Types.hpp"
#include "StaticVector.hpp"
#include "Tracer.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/PipelineLayout.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/ShaderModule.hpp"
#include "Vulkan/Sync/BufferMemoryBarrier.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"
#include "Vulkan/Types.hpp"
#include "Vulkan/UniformBufferObject.hpp"

namespace
{
	/* Descriptor bindings shared by the three passes (one layout, one set per pass, each writing what it uses). */
	constexpr uint32_t ParametersBinding{0};
	constexpr uint32_t TransmittanceBinding{1};
	constexpr uint32_t MultipleScatteringBinding{2};
	constexpr uint32_t OutputBinding{3};
	constexpr uint32_t FrameBinding{4};
	/* The sky-view pass only: the frame's moons (SkyAtmosphere::m_moonBuffer). */
	constexpr uint32_t MoonBinding{5};

	/** @brief The moon block (std140): a vec4 header (the moon count), then three vec4 per moon — the frame texture's
	 * texels 4 + 3i to 6 + 3i, copied as they are. */
	constexpr uint32_t MoonBlockVectors{1 + (3 * EmEn::Graphics::SkyFrameInputs::MaxMoons)};
	constexpr VkDeviceSize MoonBlockBytes{static_cast< VkDeviceSize >(MoonBlockVectors) * 4 * sizeof(float)};

	/** @brief The sky-view pass's push constants: the star and the camera of the frame. */
	struct SkyViewPushConstants
	{
		/* xyz: unit world vector toward the star (Y up), w: the camera's distance to the planet's centre, km. */
		std::array< float, 4 > sunDirectionHeight;
		/* rgb: the star's illuminance above the atmosphere × its colour, lux; w: cos of the star's angular radius. */
		std::array< float, 4 > sunIlluminanceCos;
	};

	static_assert(sizeof(SkyViewPushConstants) == 32, "The sky-view push constants are two vec4.");

	/** @brief The aerial perspective pass's push constants: the camera's frustum, the star and the sky's grading. */
	struct AerialPerspectivePushConstants
	{
		/* The inverse of projection × view ROTATION (camera-relative), column-major: screen → view ray. */
		std::array< float, 16 > invRelativeViewProj;
		/* xyz: unit world vector toward the star (Y up), w: the camera's distance to the planet's centre, km. */
		std::array< float, 4 > sunDirectionHeight;
		/* rgb: the star's illuminance above the atmosphere × its colour (lux), w: the grading's saturation. */
		std::array< float, 4 > sunIlluminanceSaturation;
		/* rgb: the grading's tint, w: its intensity. */
		std::array< float, 4 > tintIntensity;
	};

	static_assert(sizeof(AerialPerspectivePushConstants) == 112, "The aerial perspective push constants are a mat4 and three vec4.");
	static_assert(sizeof(AerialPerspectivePushConstants) <= 128, "Over the 128-byte Vulkan minimum guarantee of maxPushConstantsSize.");
	static_assert(sizeof(AerialPerspectivePushConstants) >= sizeof(SkyViewPushConstants), "The shared push constant range covers every pass.");

	/* ---- The shared marcher (Hillaire's IntegrateScatteredLuminance), in the planet frame (+Z up at the camera). ---- */
	constexpr auto IntegrationGLSL = R"GLSL(
#if ATM_READS_TRANSMITTANCE
layout(set = 0, binding = 1) uniform sampler2D transmittanceLut;

vec3 atmTransmittanceToTop (float height, float zenithCos)
{
	return textureLod(transmittanceLut, atmTransmittanceParametersToUv(height, zenithCos), 0.0).rgb;
}
#endif

#if ATM_READS_MULTIPLE_SCATTERING
layout(set = 0, binding = 2) uniform sampler2D multipleScatteringLut;

vec3 atmMultipleScattering (vec3 position, float sunZenithCos)
{
	vec2 uv = clamp(vec2((sunZenithCos * 0.5) + 0.5, (length(position) - atmGroundRadius()) / (atmTopRadius() - atmGroundRadius())), 0.0, 1.0);

	return textureLod(multipleScatteringLut, vec2(atmFromUnitToSubUv(uv.x, AtmMultipleScatteringSize), atmFromUnitToSubUv(uv.y, AtmMultipleScatteringSize)), 0.0).rgb;
}
#endif

struct AtmIntegration
{
	vec3 luminance;
	vec3 opticalDepth;
	vec3 multipleScatteringAs1;
};

/* Marches from `origin` along `direction` (unit) and integrates the in-scattered luminance PER LUX of the star.
 * phaseMode 0: uniform phase (the multiple-scattering table); 1: Rayleigh + Mie (the sky-view table). */
AtmIntegration atmIntegrate (vec3 origin, vec3 direction, vec3 sunDirection, bool withGround, float sampleCount, bool variableSampleCount, int phaseMode, float maxDistance)
{
	AtmIntegration result;
	result.luminance = vec3(0.0);
	result.opticalDepth = vec3(0.0);
	result.multipleScatteringAs1 = vec3(0.0);

	vec3 planetCentre = vec3(0.0);
	float tBottom = atmRaySphereNearest(origin, direction, planetCentre, atmGroundRadius());
	float tTop = atmRaySphereNearest(origin, direction, planetCentre, atmTopRadius());
	float tMax = 0.0;

	if ( tBottom < 0.0 )
	{
		if ( tTop < 0.0 )
		{
			return result;
		}

		tMax = tTop;
	}
	else
	{
		tMax = tTop > 0.0 ? min(tTop, tBottom) : tBottom;
	}

	/* maxDistance (km): the aerial perspective marches each froxel only to its slice. */
	tMax = min(tMax, min(maxDistance, 9000000.0));

	float count = sampleCount;
	float countFloor = sampleCount;
	float tMaxFloor = tMax;

	if ( variableSampleCount )
	{
		/* Hillaire's RayMarchMinMaxSPP (4, 14): more samples on long rays, quadratic spacing. */
		count = mix(4.0, 14.0, clamp(tMax * 0.01, 0.0, 1.0));
		countFloor = floor(count);
		tMaxFloor = tMax * countFloor / count;
	}

	float dt = tMax / count;
	float t = 0.0;
	/* The MIDPOINT of each step (owner decision 2026-10-25). Hillaire's reference samples at 0.3 of a step: with a full
	 * step the optical depth comes out +2 to +9 % (an exponential medium is denser at the step's start), and its fixed
	 * march (dt measured from the previous sample) -6 to -15 % (the first sample, which dominates the thin Mie layer,
	 * worth 0.3 of a step). The midpoint stays within 2 % of an exact integration for every ray of the tables. */
	const float SampleSegmentT = 0.5;
	const float UniformPhase = 1.0 / (4.0 * AtmPi);
	float cosTheta = dot(sunDirection, direction);
	vec3 miePhase = atmMiePhase(cosTheta);
	float rayleighPhase = atmRayleighPhase(cosTheta);
	vec3 throughput = vec3(1.0);

	for ( float s = 0.0; s < count; s += 1.0 )
	{
		if ( variableSampleCount )
		{
			float t0 = s / countFloor;
			float t1 = (s + 1.0) / countFloor;
			t0 = t0 * t0;
			t1 = t1 * t1;
			t0 = tMaxFloor * t0;
			t1 = t1 > 1.0 ? tMax : tMaxFloor * t1;
			t = t0 + ((t1 - t0) * SampleSegmentT);
			dt = t1 - t0;
		}
		else
		{
			/* ⚠️ Each sample is worth a FULL step (dt = tMax / count, set above). Hillaire's reference measures it from
			 * the previous sample (dt = newT - t from t = 0): the first sample is worth 0.3 of a step and the march
			 * covers (1 - 0.7 / count) of the ray. Found by measuring the aerial perspective against a CPU integration
			 * (2026-10-25: its optical depth ~4.5 % short at 1-9 km); fixed with the CPU twin (transmittanceToTop()). */
			t = tMax * (s + SampleSegmentT) / count;
		}

		vec3 position = origin + (t * direction);
		AtmMedium medium = atmSampleMedium(position);
		vec3 extinction = max(medium.extinction, vec3(1e-9));
		vec3 sampleOpticalDepth = medium.extinction * dt;
		vec3 sampleTransmittance = exp(-sampleOpticalDepth);

		result.opticalDepth += sampleOpticalDepth;

#if ATM_READS_TRANSMITTANCE
		float height = length(position);
		vec3 up = position / height;
		float sunZenithCos = dot(sunDirection, up);
		vec3 transmittanceToSun = atmTransmittanceToTop(height, sunZenithCos);
		/* The planet's own shadow (the ray toward the star hits the ground). */
		float earthShadow = atmRaySphereNearest(position, sunDirection, planetCentre + (AtmPlanetRadiusOffset * up), atmGroundRadius()) >= 0.0 ? 0.0 : 1.0;

		vec3 phaseTimesScattering = phaseMode == 1
			? (medium.scatteringMie * miePhase) + (medium.scatteringRayleigh * rayleighPhase)
			: medium.scattering * UniformPhase;

		vec3 multipleScattering = vec3(0.0);
#if ATM_READS_MULTIPLE_SCATTERING
		multipleScattering = atmMultipleScattering(position, sunZenithCos);
#endif

		vec3 S = (earthShadow * transmittanceToSun * phaseTimesScattering) + (multipleScattering * medium.scattering);
		vec3 MS = medium.scattering;

		/* Analytical integration of the sample over its segment (Hillaire 2020, § 5.4 / Frostbite 2015). */
		result.multipleScatteringAs1 += throughput * ((MS - (MS * sampleTransmittance)) / extinction);
		result.luminance += throughput * ((S - (S * sampleTransmittance)) / extinction);
#endif

		throughput *= sampleTransmittance;
	}

#if ATM_READS_TRANSMITTANCE
	/* The ground lit by the star (the multiple-scattering table's bounce). */
	if ( withGround && tMax == tBottom && tBottom > 0.0 )
	{
		vec3 position = origin + (tBottom * direction);
		float height = length(position);
		vec3 up = position / height;
		float sunZenithCos = dot(sunDirection, up);
		float NdotL = clamp(dot(up, sunDirection), 0.0, 1.0);

		result.luminance += atmTransmittanceToTop(height, sunZenithCos) * throughput * NdotL * atmosphere.groundAlbedo.rgb / AtmPi;
	}
#endif

	return result;
}
)GLSL";

	/* ---- Pass 1: TRANSMITTANCE to the top of the atmosphere, per (height, zenith). ---- */
	constexpr auto TransmittanceShader = R"GLSL(
#version 450

layout(local_size_x = 8, local_size_y = 8) in;

#define ATM_READS_TRANSMITTANCE 0
#define ATM_READS_MULTIPLE_SCATTERING 0
)GLSL";

	constexpr auto TransmittanceMain = R"GLSL(

void main ()
{
	ivec2 texel = ivec2(gl_GlobalInvocationID.xy);

	if ( any(greaterThanEqual(texel, ivec2(AtmTransmittanceSize))) )
	{
		return;
	}

	vec2 uv = (vec2(texel) + 0.5) / AtmTransmittanceSize;
	float viewHeight;
	float viewZenithCos;

	atmTransmittanceUvToParameters(uv, viewHeight, viewZenithCos);

	vec3 origin = vec3(0.0, 0.0, viewHeight);
	vec3 direction = vec3(0.0, sqrt(max(1.0 - (viewZenithCos * viewZenithCos), 0.0)), viewZenithCos);
	AtmIntegration result = atmIntegrate(origin, direction, direction, false, 40.0, false, 0, 9000000.0);

	imageStore(outputImage, texel, vec4(exp(-result.opticalDepth), 1.0));
}
)GLSL";

	/* ---- Pass 2: MULTIPLE SCATTERING, per (sun zenith, height): Hillaire 2020 § 5.5, the infinite series of an
	 * isotropic second-order transfer f_ms. ---- */
	constexpr auto MultipleScatteringShader = R"GLSL(
#version 450

layout(local_size_x = 8, local_size_y = 8) in;

#define ATM_READS_TRANSMITTANCE 1
#define ATM_READS_MULTIPLE_SCATTERING 0
)GLSL";

	constexpr auto MultipleScatteringMain = R"GLSL(

void main ()
{
	ivec2 texel = ivec2(gl_GlobalInvocationID.xy);

	if ( any(greaterThanEqual(texel, ivec2(AtmMultipleScatteringSize))) )
	{
		return;
	}

	vec2 uv = (vec2(texel) + 0.5) / AtmMultipleScatteringSize;
	uv = vec2(atmFromSubUvToUnit(uv.x, AtmMultipleScatteringSize), atmFromSubUvToUnit(uv.y, AtmMultipleScatteringSize));

	float sunZenithCos = (uv.x * 2.0) - 1.0;
	vec3 sunDirection = vec3(0.0, sqrt(clamp(1.0 - (sunZenithCos * sunZenithCos), 0.0, 1.0)), sunZenithCos);
	float viewHeight = atmGroundRadius() + clamp(uv.y + AtmPlanetRadiusOffset, 0.0, 1.0) * (atmTopRadius() - atmGroundRadius() - AtmPlanetRadiusOffset);
	vec3 origin = vec3(0.0, 0.0, viewHeight);

	/* 64 directions over the sphere (8 × 8, uniform in solid angle). */
	const float SqrtSampleCount = 8.0;
	const float SphereSolidAngle = 4.0 * AtmPi;
	const float IsotropicPhase = 1.0 / SphereSolidAngle;
	vec3 luminance = vec3(0.0);
	vec3 multipleScatteringAs1 = vec3(0.0);

	for ( float i = 0.0; i < SqrtSampleCount; i += 1.0 )
	{
		for ( float j = 0.0; j < SqrtSampleCount; j += 1.0 )
		{
			float randA = (i + 0.5) / SqrtSampleCount;
			float randB = (j + 0.5) / SqrtSampleCount;
			float theta = 2.0 * AtmPi * randA;
			float phi = acos(1.0 - (2.0 * randB));
			vec3 direction = vec3(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
			AtmIntegration result = atmIntegrate(origin, direction, sunDirection, true, 20.0, false, 0, 9000000.0);

			luminance += result.luminance * SphereSolidAngle / (SqrtSampleCount * SqrtSampleCount);
			multipleScatteringAs1 += result.multipleScatteringAs1 * SphereSolidAngle / (SqrtSampleCount * SqrtSampleCount);
		}
	}

	/* f_ms (eq. 7) and L_2nd (eq. 5), then the geometric series 1 / (1 - f_ms) (eq. 9, 10). */
	vec3 transfer = multipleScatteringAs1 * IsotropicPhase;
	vec3 inScattered = luminance * IsotropicPhase;
	vec3 psi = inScattered / max(vec3(1.0) - transfer, vec3(1e-4));

	imageStore(outputImage, texel, vec4(psi, 1.0));
}
)GLSL";

	/* ---- Pass 3: the SKY-VIEW table for this frame's star and camera altitude (Hillaire 2020 § 5.3), and the frame
	 * texture the material reads. ---- */
	constexpr auto SkyViewShader = R"GLSL(
#version 450

layout(local_size_x = 8, local_size_y = 8) in;

#define ATM_READS_TRANSMITTANCE 1
#define ATM_READS_MULTIPLE_SCATTERING 1
)GLSL";

	constexpr auto SkyViewMain = R"GLSL(
layout(set = 0, binding = 4, rgba32f) uniform writeonly image2D frameImage;

/* The frame's moons (SkyAtmosphere::recordFrame()): header.x the count, then three vec4 per moon, the frame texture's
 * layout (SkyAtmosphere::FrameTexels). */
layout(set = 0, binding = 5, std140) uniform SkyMoons
{
	vec4 header;
	vec4 texels[12];
} moons;

layout(push_constant) uniform SkyViewPushConstants
{
	vec4 sunDirectionHeight;
	vec4 sunIlluminanceCos;
} frame;

void main ()
{
	ivec2 texel = ivec2(gl_GlobalInvocationID.xy);
	float viewHeight = frame.sunDirectionHeight.w;

	/* The frame texture, once: the star (world frame) and the camera, for the material. */
	if ( texel == ivec2(0) )
	{
		imageStore(frameImage, ivec2(0, 0), vec4(frame.sunDirectionHeight.xyz, frame.sunIlluminanceCos.w));
		imageStore(frameImage, ivec2(1, 0), vec4(frame.sunIlluminanceCos.rgb, viewHeight));
		imageStore(frameImage, ivec2(2, 0), vec4(moons.header.x, 0.0, 0.0, 0.0));
		imageStore(frameImage, ivec2(3, 0), vec4(0.0));

		for ( int index = 0; index < 12; ++index )
		{
			imageStore(frameImage, ivec2(4 + index, 0), moons.texels[index]);
		}
	}

	if ( any(greaterThanEqual(texel, ivec2(AtmSkyViewSize))) )
	{
		return;
	}

	vec2 uv = (vec2(texel) + 0.5) / AtmSkyViewSize;
	float viewZenithCos;
	float lightViewCos;

	atmSkyViewUvToParameters(uv, viewHeight, viewZenithCos, lightViewCos);

	/* The camera's local frame (Hillaire's): +Z up, the star in the XZ plane. The engine is Y up: its zenith is the
	 * world Y component. */
	float sunZenithCos = clamp(frame.sunDirectionHeight.y, -1.0, 1.0);
	vec3 sunDirection = normalize(vec3(sqrt(max(1.0 - (sunZenithCos * sunZenithCos), 0.0)), 0.0, sunZenithCos));
	float viewZenithSin = sqrt(max(1.0 - (viewZenithCos * viewZenithCos), 0.0));
	vec3 direction = vec3(viewZenithSin * lightViewCos, viewZenithSin * sqrt(max(1.0 - (lightViewCos * lightViewCos), 0.0)), viewZenithCos);
	vec3 origin = vec3(0.0, 0.0, viewHeight);

	/* A camera above the atmosphere starts at its top. */
	if ( viewHeight > atmTopRadius() )
	{
		float tTop = atmRaySphereNearest(origin, direction, vec3(0.0), atmTopRadius());

		if ( tTop < 0.0 )
		{
			imageStore(outputImage, texel, vec4(0.0, 0.0, 0.0, 1.0));

			return;
		}

		origin += (direction * tTop) - (vec3(0.0, 0.0, 1.0) * AtmPlanetRadiusOffset);
	}

	/* WITH the ground (unlike Hillaire's sky-view pass, which leaves the planet to the scene): a direction below the
	 * horizon sees the planet's surface lit by the star through the air, not a black void under a small level. */
	AtmIntegration result = atmIntegrate(origin, direction, sunDirection, true, 30.0, true, 1, 9000000.0);

	imageStore(outputImage, texel, vec4(result.luminance, 1.0));
}
)GLSL";

	/* ---- Pass 4: the AERIAL PERSPECTIVE volume for this frame's camera and star (Hillaire 2020 § 5.4; his reference
	 * RenderCameraVolumePS): 32×32 froxels over the screen, 32 slices along the view ray in a SQUARED distribution of
	 * 4 km slices (0 to 128 km), each marched from the camera to its slice with 2 × (slice + 1) samples. The volume holds
	 * the in-scattered ABSOLUTE radiance (nits: the star's illuminance and the sky's grading applied, as the environment
	 * capture — the haze of a far ridge must meet the drawn horizon) and the RGB transmittance. ---- */
	constexpr auto AerialPerspectiveShader = R"GLSL(
#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

#define ATM_READS_TRANSMITTANCE 1
#define ATM_READS_MULTIPLE_SCATTERING 1
)GLSL";

	constexpr auto AerialPerspectiveMain = R"GLSL(
layout(set = 0, binding = 4, rgba16f) uniform writeonly image3D transmittanceVolume;

layout(push_constant) uniform AerialPerspectivePushConstants
{
	mat4 invRelativeViewProj;
	vec4 sunDirectionHeight;
	vec4 sunIlluminanceSaturation;
	vec4 tintIntensity;
} camera;

const float AtmAerialSize = 32.0;
const float AtmAerialKmPerSlice = 4.0;

void main ()
{
	ivec3 texel = ivec3(gl_GlobalInvocationID);

	if ( any(greaterThanEqual(texel, ivec3(AtmAerialSize))) )
	{
		return;
	}

	/* The froxel's view ray, through the screen point the application samples it at (vUV = this uv). */
	vec2 uv = (vec2(texel.xy) + 0.5) / AtmAerialSize;
	vec4 farPoint = camera.invRelativeViewProj * vec4(uv * 2.0 - 1.0, 0.5, 1.0);
	vec3 worldDirection = normalize(farPoint.xyz / farPoint.w);

	/* The planet frame (+Z up at the camera): the engine is Y up. The same mapping for the star keeps every dot product
	 * (the phases, the star's zenith along the ray). */
	vec3 direction = vec3(worldDirection.x, worldDirection.z, worldDirection.y);
	vec3 sunDirection = normalize(vec3(camera.sunDirectionHeight.x, camera.sunDirectionHeight.z, camera.sunDirectionHeight.y));
	vec3 origin = vec3(0.0, 0.0, camera.sunDirectionHeight.w);

	/* The slice's distance, km: 128 × ((slice + 0.5) / 32)², the application inverts it with a square root. */
	float slice = (float(texel.z) + 0.5) / AtmAerialSize;
	float distance = slice * slice * AtmAerialSize * AtmAerialKmPerSlice;

	AtmIntegration result = atmIntegrate(origin, direction, sunDirection, false, max(1.0, (float(texel.z) + 1.0) * 2.0), false, 1, distance);

	vec3 radiance = result.luminance * camera.sunIlluminanceSaturation.rgb;

	imageStore(outputVolume, texel, vec4(atmGrade(radiance, camera.tintIntensity, camera.sunIlluminanceSaturation.w), 1.0));
	imageStore(transmittanceVolume, texel, vec4(exp(-result.opticalDepth), 1.0));
}
)GLSL";
}

namespace EmEn::Graphics
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Vulkan;

	AtmosphereParameters
	AtmosphereParameters::earth () noexcept
	{
		return {};
	}

	AtmosphereParameters
	AtmosphereParameters::mars () noexcept
	{
		AtmosphereParameters parameters;

		parameters.groundRadius = 3389.5F;
		parameters.topRadius = 3389.5F + 80.0F;
		/* A thin CO2 atmosphere: ~1.5 % of Earth's molecular scattering. */
		parameters.rayleighScattering = {0.087e-3F, 0.203e-3F, 0.497e-3F};
		parameters.rayleighScaleHeight = 11.1F;
		/* The dust: an optical depth of ~0.5, a single-scattering albedo that falls in the blue (it reddens the sky). */
		parameters.mieScattering = {38.0e-3F, 30.0e-3F, 19.0e-3F};
		parameters.mieAbsorption = {1.5e-3F, 7.0e-3F, 17.0e-3F};
		parameters.mieScaleHeight = 11.1F;
		/* The forward peak narrows with the wavelength (diffraction by ~1.5 µm grains): the blue aureole of the sunsets. */
		parameters.miePhaseG = {0.50F, 0.66F, 0.88F};
		/* No ozone. */
		parameters.absorptionExtinction = {0.0F, 0.0F, 0.0F};
		parameters.groundAlbedo = {0.25F, 0.15F, 0.08F};
		parameters.planet = "Mars";

		return parameters;
	}

	std::optional< AtmosphereParameters >
	AtmosphereParameters::preset (const std::string & name) noexcept
	{
		if ( name == "Earth" )
		{
			return AtmosphereParameters::earth();
		}

		if ( name == "Mars" )
		{
			return AtmosphereParameters::mars();
		}

		return std::nullopt;
	}

	Vector< 3, float >
	AtmosphereParameters::transmittanceToTop (float height, float zenithCos) const noexcept
	{
		if ( !std::isfinite(height) || !std::isfinite(zenithCos) )
		{
			return {0.0F, 0.0F, 0.0F};
		}

		/* Kilometres from the planet's centre, the ray in the (horizontal, vertical) plane: the table's frame. */
		const auto ground = static_cast< double >(groundRadius);
		const auto top = std::max(static_cast< double >(topRadius), ground);
		const auto radius = ground + std::clamp(static_cast< double >(height), 0.0, top - ground);
		const auto mu = std::clamp(static_cast< double >(zenithCos), -1.0, 1.0);
		const auto sinZenith = std::sqrt(std::max(1.0 - (mu * mu), 0.0));

		/* The ground hides a ray going down below the horizon of this height. */
		if ( mu < 0.0 && (radius * radius * (1.0 - (mu * mu))) < (ground * ground) )
		{
			return {0.0F, 0.0F, 0.0F};
		}

		/* The distance to the top along the ray (the far root: the point is inside the top sphere). */
		const auto distance = (-radius * mu) + std::sqrt(std::max((radius * radius * ((mu * mu) - 1.0)) + (top * top), 0.0));

		/* packParameters()' scale heights, clamped alike, and the medium of AtmosphereGLSL's atmSampleMedium(). */
		const auto rayleighRate = -1.0 / static_cast< double >(std::max(rayleighScaleHeight, 1e-3F));
		const auto mieRate = -1.0 / static_cast< double >(std::max(mieScaleHeight, 1e-3F));
		const auto mieExtinction = mieScattering + mieAbsorption;

		std::array< double, 3 > opticalDepth{0.0, 0.0, 0.0};

		/* The table's fixed-step march: a sample at the MIDPOINT of each step, each worth a full step (the table's own
		 * march, fixed with it on 2026-10-25: Hillaire's reference sampled at 0.3 of a step and measured a step from the
		 * previous sample — optical depth -6 to -15 % against an exact integration; the midpoint is within 2 %). */
		const auto dt = distance / static_cast< double >(TransmittanceSampleCount);

		for ( int sample = 0; sample < TransmittanceSampleCount; ++sample )
		{
			const auto t = distance * (static_cast< double >(sample) + 0.5) / static_cast< double >(TransmittanceSampleCount);

			const auto x = t * sinZenith;
			const auto z = radius + (t * mu);
			const auto sampleHeight = std::max(std::sqrt((x * x) + (z * z)) - ground, 0.0);
			const auto densityMie = std::exp(mieRate * sampleHeight);
			const auto densityRayleigh = std::exp(rayleighRate * sampleHeight);
			const auto densityAbsorption = std::clamp(sampleHeight < static_cast< double >(absorptionLayerWidth)
				? (static_cast< double >(absorptionBelowLinear) * sampleHeight) + static_cast< double >(absorptionBelowConstant)
				: (static_cast< double >(absorptionAboveLinear) * sampleHeight) + static_cast< double >(absorptionAboveConstant), 0.0, 1.0);

			const auto extinction = [&] (size_t channel) {
				return (densityMie * static_cast< double >(mieExtinction[channel])) + (densityRayleigh * static_cast< double >(rayleighScattering[channel])) + (densityAbsorption * static_cast< double >(absorptionExtinction[channel]));
			};

			opticalDepth[0] += extinction(0) * dt;
			opticalDepth[1] += extinction(1) * dt;
			opticalDepth[2] += extinction(2) * dt;
		}

		return {
			static_cast< float >(std::exp(-opticalDepth[0])),
			static_cast< float >(std::exp(-opticalDepth[1])),
			static_cast< float >(std::exp(-opticalDepth[2]))
		};
	}

	Vector< 3, float >
	AtmosphereParameters::discTransmittance (float height, float elevation, float angularRadius) const noexcept
	{
		if ( !std::isfinite(height) || !std::isfinite(elevation) || !std::isfinite(angularRadius) )
		{
			return {0.0F, 0.0F, 0.0F};
		}

		const auto radius = std::clamp(static_cast< double >(angularRadius), 0.0, std::numbers::pi / 4.0);

		std::array< double, 3 > sum{0.0, 0.0, 0.0};
		double weights = 0.0;

		for ( int strip = 0; strip < DiscStripCount; ++strip )
		{
			/* The strip's centre, from −1 (the disc's bottom) to +1 (its top), and its chord. */
			const auto offset = -1.0 + ((2.0 * static_cast< double >(strip) + 1.0) / static_cast< double >(DiscStripCount));
			const auto weight = std::sqrt(std::max(1.0 - (offset * offset), 0.0));
			const auto stripElevation = static_cast< double >(elevation) + (offset * radius);
			const auto transmittance = this->transmittanceToTop(height, static_cast< float >(std::sin(stripElevation)));

			sum[0] += weight * static_cast< double >(transmittance[0]);
			sum[1] += weight * static_cast< double >(transmittance[1]);
			sum[2] += weight * static_cast< double >(transmittance[2]);

			weights += weight;
		}

		return {
			static_cast< float >(sum[0] / weights),
			static_cast< float >(sum[1] / weights),
			static_cast< float >(sum[2] / weights)
		};
	}

	SkyAtmosphere::SkyAtmosphere (std::string name, AtmosphereParameters parameters) noexcept
		: m_name{std::move(name)},
		m_parameters{std::move(parameters)},
		m_transmittance{m_name + "Transmittance", {TransmittanceWidth, TransmittanceHeight}, VK_FORMAT_R16G16B16A16_SFLOAT},
		m_multipleScattering{m_name + "MultipleScattering", {MultipleScatteringSize, MultipleScatteringSize}, VK_FORMAT_R16G16B16A16_SFLOAT},
		m_skyView{m_name + "SkyView", {SkyViewWidth, SkyViewHeight}, VK_FORMAT_R16G16B16A16_SFLOAT},
		m_frameData{m_name + "Frame", {FrameTexels, 1}, VK_FORMAT_R32G32B32A32_SFLOAT},
		m_aerialInscatter{m_name + "AerialInscatter", {AerialPerspectiveSize, AerialPerspectiveSize, AerialPerspectiveSize}, VK_FORMAT_R16G16B16A16_SFLOAT},
		m_aerialTransmittance{m_name + "AerialTransmittance", {AerialPerspectiveSize, AerialPerspectiveSize, AerialPerspectiveSize}, VK_FORMAT_R16G16B16A16_SFLOAT},
		m_environment{std::make_unique< SkyEnvironment >(m_name)}
	{

	}

	SkyAtmosphere::~SkyAtmosphere () = default;

	std::array< float, 32 >
	SkyAtmosphere::packParameters () const noexcept
	{
		const auto & p = m_parameters;
		const auto mieExtinction = p.mieScattering + p.mieAbsorption;

		return {
			/* radii */
			p.groundRadius, p.topRadius, -1.0F / std::max(p.rayleighScaleHeight, 1e-3F), -1.0F / std::max(p.mieScaleHeight, 1e-3F),
			/* rayleighScattering, unused */
			p.rayleighScattering.x(), p.rayleighScattering.y(), p.rayleighScattering.z(), 0.0F,
			/* mieScattering, absorbing layer width */
			p.mieScattering.x(), p.mieScattering.y(), p.mieScattering.z(), p.absorptionLayerWidth,
			/* mieExtinction, absorbing layer below: constant */
			mieExtinction.x(), mieExtinction.y(), mieExtinction.z(), p.absorptionBelowConstant,
			/* mieAbsorption, absorbing layer below: linear */
			p.mieAbsorption.x(), p.mieAbsorption.y(), p.mieAbsorption.z(), p.absorptionBelowLinear,
			/* absorptionExtinction, absorbing layer above: constant */
			p.absorptionExtinction.x(), p.absorptionExtinction.y(), p.absorptionExtinction.z(), p.absorptionAboveConstant,
			/* groundAlbedo, absorbing layer above: linear */
			p.groundAlbedo.x(), p.groundAlbedo.y(), p.groundAlbedo.z(), p.absorptionAboveLinear,
			/* Mie g per channel, unused */
			std::clamp(p.miePhaseG.x(), -0.999F, 0.999F), std::clamp(p.miePhaseG.y(), -0.999F, 0.999F), std::clamp(p.miePhaseG.z(), -0.999F, 0.999F), 0.0F
		};
	}

	bool
	SkyAtmosphere::isCreated () const noexcept
	{
		const MutexLock lock{m_creationAccess};

		return m_created;
	}

	bool
	SkyAtmosphere::createOnHardware (Renderer & renderer) noexcept
	{
		const MutexLock lock{m_creationAccess};

		if ( m_created )
		{
			return true;
		}

		m_tableFormat = SkyAtmosphere::tableFormat(renderer);

		if ( !m_transmittance.setFormat(m_tableFormat) || !m_multipleScattering.setFormat(m_tableFormat) || !m_skyView.setFormat(m_tableFormat) || !m_aerialInscatter.setFormat(m_tableFormat) )
		{
			return false;
		}

		if ( !m_transmittance.create(renderer) || !m_multipleScattering.create(renderer) || !m_skyView.create(renderer) || !m_frameData.create(renderer) || !m_aerialInscatter.create(renderer) || !m_aerialTransmittance.create(renderer) )
		{
			TraceError{ClassId} << "Unable to create the lookup tables of the sky '" << m_name << "' !";

			return false;
		}

		if ( !this->createPipelines(renderer) || !this->bake(renderer) )
		{
			return false;
		}

		m_created = true;

		TraceSuccess{ClassId} << "Sky atmosphere '" << m_name << "': transmittance (" << TransmittanceWidth << "×" << TransmittanceHeight << ") and multiple scattering (" << MultipleScatteringSize << "²) baked in " << (m_tableFormat == VK_FORMAT_R32G32B32A32_SFLOAT ? "RGBA32F" : "RGBA16F") << "; ground radius " << m_parameters.groundRadius << " km, top " << m_parameters.topRadius << " km.";

		return true;
	}

	VkFormat
	SkyAtmosphere::tableFormat (Renderer & renderer) noexcept
	{
		constexpr VkFormatFeatureFlags Needs = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

		if ( const auto properties = renderer.device()->physicalDevice()->getFormatProperties(VK_FORMAT_R32G32B32A32_SFLOAT); (properties.optimalTilingFeatures & Needs) == Needs )
		{
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		}

		TraceWarning{ClassId} << "The device cannot filter RGBA32F storage images: the sky's tables fall back to RGBA16F, and the night sky below a −12° sun flushes to zero.";

		return VK_FORMAT_R16G16B16A16_SFLOAT;
	}

	bool
	SkyAtmosphere::createPipelines (Renderer & renderer) noexcept
	{
		const auto & device = renderer.device();

		/* ---- The atmosphere's uniform buffer, written once. ---- */
		m_parametersBuffer = std::make_unique< UniformBufferObject >(device, parametersBlockSize());
		m_parametersBuffer->setIdentifier(ClassId, m_name, "UniformBufferObject");

		if ( !m_parametersBuffer->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the uniform buffer of the sky '" << m_name << "' !";

			return false;
		}

		{
			const auto packed = this->packParameters();
			auto * pointer = m_parametersBuffer->mapMemoryAs< uint8_t >(0, VK_WHOLE_SIZE);

			if ( pointer == nullptr )
			{
				return false;
			}

			std::memcpy(pointer, packed.data(), packed.size() * sizeof(float));

			m_parametersBuffer->unmapMemory();
		}

		/* ---- The frame's moons: written in each frame's command stream (recordFrame()), read by the sky-view pass. ---- */
		m_moonBuffer = std::make_unique< Buffer >(device, 0, MoonBlockBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);
		m_moonBuffer->setIdentifier(ClassId, m_name + "Moons", "Buffer");

		if ( !m_moonBuffer->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the moon buffer of the sky '" << m_name << "' !";

			return false;
		}

		/* ---- One layout for the three passes. ---- */
		m_descriptorSetLayout = std::make_shared< DescriptorSetLayout >(device, m_name + "SkyDSLayout");

		const auto declare = [this] (uint32_t binding, VkDescriptorType type) {
			VkDescriptorSetLayoutBinding layoutBinding{};
			layoutBinding.binding = binding;
			layoutBinding.descriptorType = type;
			layoutBinding.descriptorCount = 1;
			layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			m_descriptorSetLayout->declare(layoutBinding);
		};

		declare(ParametersBinding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		declare(TransmittanceBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		declare(MultipleScatteringBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
		declare(OutputBinding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		declare(FrameBinding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		declare(MoonBinding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

		if ( !m_descriptorSetLayout->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the descriptor set layout of the sky '" << m_name << "' !";

			return false;
		}

		m_pipelineLayout = std::make_shared< PipelineLayout >(
			device, m_name + "SkyPipelineLayout",
			StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 >{m_descriptorSetLayout},
			/* One range for every pass: the largest block (the aerial perspective's) covers the others. */
			StaticVector< VkPushConstantRange, 4 >{VkPushConstantRange{.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = sizeof(AerialPerspectivePushConstants)}}
		);

		if ( !m_pipelineLayout->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the pipeline layout of the sky '" << m_name << "' !";

			return false;
		}

		/* ---- The three pipelines. ---- */
		auto & shaderManager = renderer.shaderManager();

		const auto makePipeline = [&] (const char * label, const std::string & source, std::unique_ptr< ComputePipeline > & pipeline) {
			const auto module = shaderManager.getShaderModuleFromSourceCode(device, label, Saphir::ShaderType::ComputeShader, source);

			if ( module == nullptr )
			{
				TraceError{ClassId} << "Unable to compile the sky shader '" << label << "' !";

				return false;
			}

			pipeline = std::make_unique< ComputePipeline >(m_pipelineLayout);
			pipeline->setShaderModule(module->handle());

			if ( !pipeline->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the sky pipeline '" << label << "' !";

				return false;
			}

			return true;
		};

		/* The atmosphere's block and functions, the same source the sky material declares through Saphir. */
		const auto atmosphereGLSL = AtmosphereGLSL::uniformBlockGLSL(0, ParametersBinding) + AtmosphereGLSL::sharedFunctionsGLSL() + AtmosphereGLSL::ComputeGLSL;
		/* The table each pass writes, in the format tableFormat() chose (the image qualifier must match it). */
		const auto outputGLSL = std::string{"layout(set = 0, binding = "} + std::to_string(OutputBinding) + (m_tableFormat == VK_FORMAT_R32G32B32A32_SFLOAT ? ", rgba32f" : ", rgba16f") + ") uniform writeonly image2D outputImage;\n";

		if ( !makePipeline("SkyTransmittanceCS", std::string{TransmittanceShader} + atmosphereGLSL + IntegrationGLSL + outputGLSL + TransmittanceMain, m_transmittancePipeline) ||
			 !makePipeline("SkyMultipleScatteringCS", std::string{MultipleScatteringShader} + atmosphereGLSL + IntegrationGLSL + outputGLSL + MultipleScatteringMain, m_multipleScatteringPipeline) ||
			 !makePipeline("SkyViewCS", std::string{SkyViewShader} + atmosphereGLSL + IntegrationGLSL + outputGLSL + SkyViewMain, m_skyViewPipeline) )
		{
			return false;
		}

		/* The aerial perspective: its in-scattering volume in the table format (a night's haze, ~1e-6 nits, would
		 * lose its precision in half floats), its transmittance in RGBA16F. */
		const auto volumeGLSL = std::string{"layout(set = 0, binding = "} + std::to_string(OutputBinding) + (m_tableFormat == VK_FORMAT_R32G32B32A32_SFLOAT ? ", rgba32f" : ", rgba16f") + ") uniform writeonly image3D outputVolume;\n";

		if ( !makePipeline("SkyAerialPerspectiveCS", std::string{AerialPerspectiveShader} + atmosphereGLSL + IntegrationGLSL + volumeGLSL + AerialPerspectiveMain, m_aerialPerspectivePipeline) )
		{
			return false;
		}

		/* ---- One set per pass. ⚠️ FREE_DESCRIPTOR_SET_BIT: DescriptorSet frees its set in its destructor.
		 * ⚠️ A set allocates EVERY binding of its layout, written or not: four sets of 2 uniform buffers, 2 samplers and 2
		 * storage images. The pool asked for 4 / 5 / 6 until 2026-10-25 — one layout of 1 / 2 / 2 already needed 4 / 8 / 8;
		 * an implementation that enforces the counts returns VK_ERROR_OUT_OF_POOL_MEMORY on the third set. ---- */
		constexpr uint32_t SetCount{4};

		const std::vector< VkDescriptorPoolSize > poolSizes{
			{.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = SetCount * 2},
			{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = SetCount * 2},
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = SetCount * 2}
		};

		m_descriptorPool = std::make_shared< DescriptorPool >(device, poolSizes, SetCount, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

		if ( !m_descriptorPool->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the descriptor pool of the sky '" << m_name << "' !";

			return false;
		}

		const auto makeSet = [this] (std::unique_ptr< DescriptorSet > & set) {
			set = std::make_unique< DescriptorSet >(m_descriptorPool, m_descriptorSetLayout);

			return set->create() && set->writeUniformBufferObject(ParametersBinding, *m_parametersBuffer);
		};

		if ( !makeSet(m_transmittanceSet) || !makeSet(m_multipleScatteringSet) || !makeSet(m_skyViewSet) || !makeSet(m_aerialPerspectiveSet) )
		{
			TraceError{ClassId} << "Unable to create the descriptor sets of the sky '" << m_name << "' !";

			return false;
		}

		/* Sampled tables are read in SHADER_READ_ONLY; outputs are written in GENERAL. */
		const auto sampled = [] (const DescriptorSet & set, uint32_t binding, const StorageTexture2D & texture) {
			return set.writeCombinedImageSampler(binding, *texture.imageView(), *texture.sampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		};

		const bool written =
			m_transmittanceSet->writeStorageImage(OutputBinding, *m_transmittance.storageView()) &&
			sampled(*m_multipleScatteringSet, TransmittanceBinding, m_transmittance) &&
			m_multipleScatteringSet->writeStorageImage(OutputBinding, *m_multipleScattering.storageView()) &&
			sampled(*m_skyViewSet, TransmittanceBinding, m_transmittance) &&
			sampled(*m_skyViewSet, MultipleScatteringBinding, m_multipleScattering) &&
			m_skyViewSet->writeStorageImage(OutputBinding, *m_skyView.storageView()) &&
			m_skyViewSet->writeStorageImage(FrameBinding, *m_frameData.storageView()) &&
			m_skyViewSet->writeUniformBuffer(MoonBinding, m_moonBuffer->getDescriptorInfo(0, static_cast< uint32_t >(MoonBlockBytes))) &&
			sampled(*m_aerialPerspectiveSet, TransmittanceBinding, m_transmittance) &&
			sampled(*m_aerialPerspectiveSet, MultipleScatteringBinding, m_multipleScattering) &&
			m_aerialPerspectiveSet->writeStorageImage(OutputBinding, *m_aerialInscatter.storageView()) &&
			m_aerialPerspectiveSet->writeStorageImage(FrameBinding, *m_aerialTransmittance.storageView());

		if ( !written )
		{
			TraceError{ClassId} << "Unable to write the descriptor sets of the sky '" << m_name << "' !";

			return false;
		}

		return true;
	}

	bool
	SkyAtmosphere::bake (Renderer & renderer) noexcept
	{
		const auto & device = renderer.device();
		const auto commandPool = std::make_shared< CommandPool >(device, device->getGraphicsFamilyIndex(), true, true, false);

		if ( !commandPool->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the bake command pool of the sky '" << m_name << "' !";

			return false;
		}

		const auto commandBuffer = std::make_unique< CommandBuffer >(commandPool, true);

		if ( !commandBuffer->isCreated() || !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			TraceError{ClassId} << "Unable to begin the bake command buffer of the sky '" << m_name << "' !";

			return false;
		}

		const auto toGeneral = [&commandBuffer] (const StorageTexture2D & texture) {
			commandBuffer->pipelineBarrier(
				Sync::ImageMemoryBarrier{*texture.image(), 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL},
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
			);
		};

		const auto toSampled = [&commandBuffer] (const StorageTexture2D & texture) {
			commandBuffer->pipelineBarrier(
				Sync::ImageMemoryBarrier{*texture.image(), VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			);
		};

		/* Transmittance, then the multiple scattering that reads it. */
		toGeneral(m_transmittance);
		commandBuffer->bind(*m_transmittancePipeline);
		commandBuffer->bind(*m_transmittanceSet, *m_pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
		commandBuffer->dispatch((TransmittanceWidth + 7) / 8, (TransmittanceHeight + 7) / 8, 1);
		toSampled(m_transmittance);

		toGeneral(m_multipleScattering);
		commandBuffer->bind(*m_multipleScatteringPipeline);
		commandBuffer->bind(*m_multipleScatteringSet, *m_pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
		commandBuffer->dispatch((MultipleScatteringSize + 7) / 8, (MultipleScatteringSize + 7) / 8, 1);
		toSampled(m_multipleScattering);

		/* No moon until a frame says otherwise (recordFrame() rewrites the block before every sky-view dispatch). */
		commandBuffer->fill(*m_moonBuffer, 0, MoonBlockBytes, 0U);
		commandBuffer->pipelineBarrier(
			Sync::BufferMemoryBarrier{*m_moonBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT},
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		);

		/* The per-frame textures start in their SAMPLED layout (empty content): a material created before the first
		 * frame records may bind them without reading an UNDEFINED image. The star field reads the frame texture in its
		 * VERTEX stage (Effects::Atmosphere::StarField). */
		for ( const auto * texture : {&m_skyView, &m_frameData} )
		{
			commandBuffer->pipelineBarrier(
				Sync::ImageMemoryBarrier{*texture->image(), 0, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			);
		}

		if ( !commandBuffer->end() )
		{
			TraceError{ClassId} << "Unable to end the bake command buffer of the sky '" << m_name << "' !";

			return false;
		}

		auto * queue = device->getGraphicsQueue(QueuePriority::High);

		if ( queue == nullptr || !queue->submit(*commandBuffer) || !queue->waitIdle() )
		{
			TraceError{ClassId} << "Unable to submit the bake of the sky '" << m_name << "' !";

			return false;
		}

		for ( const auto * texture : {&m_transmittance, &m_multipleScattering, &m_skyView, &m_frameData} )
		{
			texture->image()->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return true;
	}

	bool
	SkyAtmosphere::recordFrame (const CommandBuffer & commandBuffer, const SkyFrameInputs & inputs) noexcept
	{
		if ( !this->isCreated() )
		{
			return false;
		}

		/* The previous frame's reads (the sky's fragments, the star field's vertices, the environment's compute) before
		 * this frame's writes (compute): the barrier's first scope covers the commands submitted earlier on the queue. */
		for ( const auto * texture : {&m_skyView, &m_frameData} )
		{
			commandBuffer.pipelineBarrier(
				Sync::ImageMemoryBarrier{*texture->image(), VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL},
				VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
			);
		}

		const auto direction = inputs.sunDirection.lengthSquared() > 0.0F ? inputs.sunDirection.normalized() : Vector< 3, float >{0.0F, 1.0F, 0.0F};
		/* The camera stands on the planet's ground at the world's Y = 0, never under it nor exactly on it (a ray along
		 * the horizon would graze the ground sphere): Hillaire's planet-radius offset, 10 m, is the floor. */
		const auto viewHeight = m_parameters.groundRadius + std::max(inputs.cameraAltitude * 0.001F, 0.01F);
		const auto angularRadius = 0.5F * inputs.sunAngularDiameter * (std::numbers::pi_v< float > / 180.0F);

		const SkyViewPushConstants pushConstants{
			.sunDirectionHeight = {direction.x(), direction.y(), direction.z(), viewHeight},
			.sunIlluminanceCos = {inputs.sunIlluminance.x(), inputs.sunIlluminance.y(), inputs.sunIlluminance.z(), std::cos(angularRadius)}
		};

		/* ---- The frame's moons (graphics doc 42 § "The moons"): the block the sky-view pass copies into the frame
		 * texture, written in this frame's command stream — the previous frame's reads first. ---- */
		{
			std::array< float, static_cast< size_t >(MoonBlockVectors) * 4 > block{};

			const auto count = std::min(inputs.moonCount, SkyFrameInputs::MaxMoons);
			/* The lunar reflectance of a FULL disc is uniform (no limb darkening): a disc of radiance
			 * scale × that reflectance over its solid angle pours exactly fullIlluminance. */
			const auto fullReflectance = static_cast< float >(Astronomy::lunarReflectance(1.0, 1.0, 0.0));

			block[0] = static_cast< float >(count);

			/* After the header vector: count ≤ MaxMoons blocks of 12 floats fill it exactly. */
			auto destination = std::span{block}.subspan(4).begin();

			for ( const auto & moon : std::span{inputs.moons}.first(count) )
			{
				const auto moonDirection = moon.direction.lengthSquared() > 0.0F ? moon.direction.normalized() : Vector< 3, float >{0.0F, 1.0F, 0.0F};
				const auto pole = moon.pole.lengthSquared() > 0.0F ? moon.pole.normalized() : Vector< 3, float >{0.0F, 1.0F, 0.0F};
				const auto moonRadius = 0.5F * std::clamp(moon.angularDiameter, 0.0F, 90.0F) * (std::numbers::pi_v< float > / 180.0F);
				const auto cosRadius = std::cos(moonRadius);
				const auto solidAngle = 2.0F * std::numbers::pi_v< float > * (1.0F - cosRadius);
				const auto scale = solidAngle > 0.0F && std::isfinite(moon.fullIlluminance) ? std::max(0.0F, moon.fullIlluminance) / (fullReflectance * solidAngle) : 0.0F;
				const auto earthshine = std::isfinite(moon.earthshine) ? std::max(0.0F, moon.earthshine) : 0.0F;

				const std::array< float, 12 > texels{
					moonDirection.x(), moonDirection.y(), moonDirection.z(), cosRadius,
					moon.color.x() * scale, moon.color.y() * scale, moon.color.z() * scale, earthshine,
					pole.x(), pole.y(), pole.z(), 0.0F
				};

				destination = std::ranges::copy(texels, destination).out;
			}

			commandBuffer.pipelineBarrier(
				Sync::BufferMemoryBarrier{*m_moonBuffer, VK_ACCESS_UNIFORM_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT},
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
			);
			commandBuffer.update(*m_moonBuffer, 0, MoonBlockBytes, block.data());
			commandBuffer.pipelineBarrier(
				Sync::BufferMemoryBarrier{*m_moonBuffer, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_UNIFORM_READ_BIT},
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
			);
		}

		commandBuffer.bind(*m_skyViewPipeline);
		commandBuffer.bind(*m_skyViewSet, *m_pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
		vkCmdPushConstants(commandBuffer.handle(), m_pipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
		commandBuffer.dispatch((SkyViewWidth + 7) / 8, (SkyViewHeight + 7) / 8, 1);

		/* Read by the sky's fragments, the star field's vertices and the environment's compute. */
		for ( const auto * texture : {&m_skyView, &m_frameData} )
		{
			commandBuffer.pipelineBarrier(
				Sync::ImageMemoryBarrier{*texture->image(), VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
			);
		}

		return true;
	}

	bool
	SkyAtmosphere::recordAerialPerspective (const CommandBuffer & commandBuffer, const SkyFrameInputs & inputs, const SkyEnvironment::Grading & grading) noexcept
	{
		if ( !this->isCreated() )
		{
			return false;
		}

		/* The previous frames' reads before this frame's writes (the barrier's first scope covers the commands submitted
		 * earlier on the queue); the content is rebuilt whole, so the old layout is discarded. */
		for ( const auto * volume : {&m_aerialInscatter, &m_aerialTransmittance} )
		{
			commandBuffer.pipelineBarrier(
				Sync::ImageMemoryBarrier{*volume->image(), 0, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL},
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
			);
		}

		const auto direction = inputs.sunDirection.lengthSquared() > 0.0F ? inputs.sunDirection.normalized() : Vector< 3, float >{0.0F, 1.0F, 0.0F};
		/* The sky-view's own height (recordFrame()): the same camera, the same 10 m floor. */
		const auto viewHeight = m_parameters.groundRadius + std::max(inputs.cameraAltitude * 0.001F, 0.01F);

		const AerialPerspectivePushConstants pushConstants{
			.invRelativeViewProj = inputs.invRelativeViewProj,
			.sunDirectionHeight = {direction.x(), direction.y(), direction.z(), viewHeight},
			.sunIlluminanceSaturation = {inputs.sunIlluminance.x(), inputs.sunIlluminance.y(), inputs.sunIlluminance.z(), grading.saturation},
			.tintIntensity = {grading.tint.x(), grading.tint.y(), grading.tint.z(), grading.intensity}
		};

		commandBuffer.bind(*m_aerialPerspectivePipeline);
		commandBuffer.bind(*m_aerialPerspectiveSet, *m_pipelineLayout, VK_PIPELINE_BIND_POINT_COMPUTE, 0);
		vkCmdPushConstants(commandBuffer.handle(), m_pipelineLayout->handle(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);
		commandBuffer.dispatch((AerialPerspectiveSize + 7) / 8, (AerialPerspectiveSize + 7) / 8, AerialPerspectiveSize);

		for ( const auto * volume : {&m_aerialInscatter, &m_aerialTransmittance} )
		{
			commandBuffer.pipelineBarrier(
				Sync::ImageMemoryBarrier{*volume->image(), VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
			);

			/* The tracked layout a descriptor describes the volume with (the barrier above does not update it). */
			volume->image()->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		return true;
	}

	void
	SkyAtmosphere::recordEnvironment (Renderer & renderer, const CommandBuffer & commandBuffer, const SkyFrameInputs & inputs, const SkyEnvironment::Grading & grading) noexcept
	{
		if ( !this->isCreated() )
		{
			return;
		}

		m_environment->recordStage(renderer, commandBuffer, *this, {.sunDirection = inputs.sunDirection, .sunIlluminance = inputs.sunIlluminance, .grading = grading});
	}
}
