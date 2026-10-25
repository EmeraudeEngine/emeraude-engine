/*
 * src/Graphics/Effects/Shared/AtmosphereGLSL.hpp
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

#pragma once

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <string>

/**
 * @file
 * @brief The GLSL of the physical sky: the planet's atmosphere and the parametrisations of its lookup tables, shared by
 * the passes that build the tables (Graphics::SkyAtmosphere, raw compute sources) and the material that draws the sky
 * (Graphics::Material::SkyAtmosphereResource, Saphir declarations). ONE source per function, so the tables and the sky
 * can never read them differently.
 *
 * The model is S. Hillaire's, *A Scalable and Production Ready Sky and Atmosphere Rendering Technique*, EGSR 2020, after
 * his reference implementation (github.com/sebh/UnrealEngineSkyAtmosphere, MIT licence): a Rayleigh layer, a Mie layer
 * and an absorbing layer (Earth's ozone, a tent profile), single scattering ray-marched against a TRANSMITTANCE table and
 * multiple scattering through a MULTIPLE-SCATTERING table, the sky around the camera in a SKY-VIEW table. The
 * transmittance parametrisation is Bruneton & Neyret's (*Precomputed Atmospheric Scattering*, EGSR 2008).
 *
 * ⚠️ UNITS: kilometres for every distance, per kilometre for the coefficients. The planet's centre is the origin.
 * ⚠️ The sky-view table holds radiance PER LUX of the star's illuminance above the atmosphere — a few tenths at most,
 * safe in RGBA16F — and the material multiplies by the star's illuminance, colour included.
 */
namespace EmEn::Graphics::AtmosphereGLSL
{
	/** @brief The type and the instance name of the atmosphere's uniform block. */
	constexpr auto BlockType{"AtmosphereParameters"};
	constexpr auto BlockInstance{"atmosphere"};

	/** @brief The members of the block (std140, eight vec4), in order. */
	constexpr std::array< const char *, 8 > BlockMembers{
		"radii",                 /* x: ground radius, y: top radius, z: Rayleigh density exponent (-1/H), w: Mie density exponent (-1/H). */
		"rayleighScattering",    /* rgb: Rayleigh scattering at the ground, w: unused. */
		"mieScattering",         /* rgb: Mie scattering at the ground, w: absorbing layer width (km). */
		"mieExtinction",         /* rgb: Mie extinction at the ground, w: absorbing layer below: constant term. */
		"mieAbsorption",         /* rgb: Mie absorption, w: absorbing layer below: linear term (per km). */
		"absorptionExtinction",  /* rgb: the absorbing layer's peak extinction, w: absorbing layer above: constant term. */
		"groundAlbedo",          /* rgb: the ground's albedo, w: absorbing layer above: linear term (per km). */
		"miePhaseG"              /* rgb: the Mie phase anisotropy g per channel, w: unused. */
	};

	/** @brief One parameter of a shared function. */
	struct Parameter
	{
		const char * type{nullptr};
		const char * name{nullptr};
	};

	/** @brief A function shared by the raw compute sources and the Saphir-generated material. */
	struct FunctionSource
	{
		const char * returnType{nullptr};
		const char * name{nullptr};
		std::array< Parameter, 4 > parameters{};
		uint32_t parameterCount{0};
		const char * body{nullptr};
	};

	/** @brief The sub-texel remapping of a table's coordinate (Hillaire): the texel centres reach the borders. */
	constexpr FunctionSource FromUnitToSubUv{
		.returnType = "float",
		.name = "atmFromUnitToSubUv",
		.parameters = {{{.type = "float", .name = "u"}, {.type = "float", .name = "resolution"}, {}, {}}},
		.parameterCount = 2,
		.body = "\treturn (u + (0.5 / resolution)) * (resolution / (resolution + 1.0));\n"
	};

	/** @brief The nearest positive intersection of a ray with a sphere, -1 when there is none. */
	constexpr FunctionSource RaySphereNearest{
		.returnType = "float",
		.name = "atmRaySphereNearest",
		.parameters = {{{.type = "vec3", .name = "origin"}, {.type = "vec3", .name = "direction"}, {.type = "vec3", .name = "centre"}, {.type = "float", .name = "radius"}}},
		.parameterCount = 4,
		.body = "\tfloat a = dot(direction, direction);\n"
		"\tvec3 offset = origin - centre;\n"
		"\tfloat b = 2.0 * dot(direction, offset);\n"
		"\tfloat c = dot(offset, offset) - (radius * radius);\n"
		"\tfloat delta = (b * b) - (4.0 * a * c);\n"
		"\tif ( delta < 0.0 || a == 0.0 ) { return -1.0; }\n"
		"\tfloat root = sqrt(delta);\n"
		"\tfloat t0 = (-b - root) / (2.0 * a);\n"
		"\tfloat t1 = (-b + root) / (2.0 * a);\n"
		"\tif ( t0 < 0.0 && t1 < 0.0 ) { return -1.0; }\n"
		"\tif ( t0 < 0.0 ) { return max(0.0, t1); }\n"
		"\tif ( t1 < 0.0 ) { return max(0.0, t0); }\n"
		"\treturn max(0.0, min(t0, t1));\n"
	};

	/** @brief The transmittance table's coordinates for a height (km from the centre) and a zenith cosine. */
	constexpr FunctionSource TransmittanceParametersToUv{
		.returnType = "vec2",
		.name = "atmTransmittanceParametersToUv",
		.parameters = {{{.type = "float", .name = "viewHeight"}, {.type = "float", .name = "viewZenithCos"}, {}, {}}},
		.parameterCount = 2,
		.body = "\tfloat ground = atmosphere.radii.x;\n"
		"\tfloat top = atmosphere.radii.y;\n"
		"\tfloat H = sqrt(max((top * top) - (ground * ground), 0.0));\n"
		"\tfloat rho = sqrt(max((viewHeight * viewHeight) - (ground * ground), 0.0));\n"
		"\tfloat discriminant = (viewHeight * viewHeight * ((viewZenithCos * viewZenithCos) - 1.0)) + (top * top);\n"
		"\tfloat d = max(0.0, -(viewHeight * viewZenithCos) + sqrt(max(discriminant, 0.0)));\n"
		"\tfloat dMin = top - viewHeight;\n"
		"\tfloat dMax = rho + H;\n"
		"\treturn vec2((d - dMin) / max(dMax - dMin, 1e-6), rho / max(H, 1e-6));\n"
	};

	/** @brief The sky-view table's coordinates (Hillaire 2020 § 5.3): x the azimuth from the star, y the zenith, non-linear
	 * around the horizon where the sky changes fastest. The table is 192×108. */
	constexpr FunctionSource SkyViewParametersToUv{
		.returnType = "vec2",
		.name = "atmSkyViewParametersToUv",
		.parameters = {{{.type = "bool", .name = "intersectGround"}, {.type = "float", .name = "viewZenithCos"}, {.type = "float", .name = "lightViewCos"}, {.type = "float", .name = "viewHeight"}}},
		.parameterCount = 4,
		.body = "\tfloat ground = atmosphere.radii.x;\n"
		"\tfloat horizonDistance = sqrt(max((viewHeight * viewHeight) - (ground * ground), 0.0));\n"
		"\tfloat beta = acos(clamp(horizonDistance / viewHeight, -1.0, 1.0));\n"
		"\tfloat zenithHorizonAngle = 3.14159265358979 - beta;\n"
		"\tvec2 uv;\n"
		"\tif ( !intersectGround ) {\n"
		"\t\tfloat coord = acos(clamp(viewZenithCos, -1.0, 1.0)) / zenithHorizonAngle;\n"
		"\t\tuv.y = (1.0 - sqrt(max(1.0 - coord, 0.0))) * 0.5;\n"
		"\t} else {\n"
		"\t\tfloat coord = (acos(clamp(viewZenithCos, -1.0, 1.0)) - zenithHorizonAngle) / beta;\n"
		"\t\tuv.y = (sqrt(max(coord, 0.0)) * 0.5) + 0.5;\n"
		"\t}\n"
		"\tuv.x = sqrt(max((-lightViewCos * 0.5) + 0.5, 0.0));\n"
		"\treturn vec2(atmFromUnitToSubUv(uv.x, 192.0), atmFromUnitToSubUv(uv.y, 108.0));\n"
	};

	/** @brief The sky-view lookup of a world direction (Y up) for a star direction and a view height (km from the centre):
	 * xy the table's coordinates, z 1 when the direction meets the ground. The camera's frame of the table: the zenith is
	 * the world Y, the azimuth is measured from the star's, both projected on the horizontal plane (Hillaire's
	 * lightOnPlane); a direction or a star at the zenith has no azimuth, any value reads the same row. One source for the
	 * sky material and the sky's environment capture (Graphics::SkyEnvironment). */
	constexpr FunctionSource SkyViewLookup{
		.returnType = "vec3",
		.name = "atmSkyViewLookup",
		.parameters = {{{.type = "vec3", .name = "direction"}, {.type = "vec3", .name = "starDirection"}, {.type = "float", .name = "viewHeight"}, {}}},
		.parameterCount = 3,
		.body = "\tvec2 viewOnPlane = direction.xz;\n"
		"\tvec2 starOnPlane = starDirection.xz;\n"
		"\tfloat planeNorms = length(viewOnPlane) * length(starOnPlane);\n"
		"\tfloat lightViewCos = planeNorms > 1e-6 ? clamp(dot(viewOnPlane, starOnPlane) / planeNorms, -1.0, 1.0) : 1.0;\n"
		"\tbool hitsGround = atmRaySphereNearest(vec3(0.0, 0.0, viewHeight), vec3(viewOnPlane.x, viewOnPlane.y, direction.y), vec3(0.0), atmosphere.radii.x) >= 0.0;\n"
		"\treturn vec3(atmSkyViewParametersToUv(hitsGround, direction.y, lightViewCos, viewHeight), hitsGround ? 1.0 : 0.0);\n"
	};

	/** @brief The sky's artistic grading, neutral at its defaults: the saturation around the Rec.709 luminance, then the
	 * tint × the intensity (tintIntensity.rgb × tintIntensity.w). One source for the material and the capture. */
	constexpr FunctionSource Grade{
		.returnType = "vec3",
		.name = "atmGrade",
		.parameters = {{{.type = "vec3", .name = "radiance"}, {.type = "vec4", .name = "tintIntensity"}, {.type = "float", .name = "saturation"}, {}}},
		.parameterCount = 3,
		.body = "\tfloat luminance = dot(radiance, vec3(0.2126, 0.7152, 0.0722));\n"
		"\treturn max(mix(vec3(luminance), radiance, saturation), vec3(0.0)) * tintIntensity.rgb * tintIntensity.w;\n"
	};

	/**
	 * @brief Returns a shared function as raw GLSL (the compute sources).
	 * @param function A reference to the function.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	toGLSL (const FunctionSource & function) noexcept
	{
		std::string code{function.returnType};
		code += ' ';
		code += function.name;
		code += " (";

		uint32_t index = 0;

		for ( const auto & parameter : function.parameters )
		{
			if ( index == function.parameterCount )
			{
				break;
			}

			code += index > 0 ? ", " : "";
			code += parameter.type;
			code += ' ';
			code += parameter.name;

			++index;
		}

		code += ")\n{\n";
		code += function.body;
		code += "}\n\n";

		return code;
	}

	/**
	 * @brief Returns the atmosphere's uniform block as raw GLSL (the compute sources).
	 * @param set The descriptor set index.
	 * @param binding The binding.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	uniformBlockGLSL (uint32_t set, uint32_t binding) noexcept
	{
		std::string code{"layout(set = "};
		code += std::to_string(set) + ", binding = " + std::to_string(binding) + ", std140) uniform " + BlockType + "\n{\n";

		for ( const auto * member : BlockMembers )
		{
			code += "\tvec4 ";
			code += member;
			code += ";\n";
		}

		code += "} ";
		code += BlockInstance;
		code += ";\n\n";

		return code;
	}

	/**
	 * @brief Returns the shared functions as raw GLSL, in dependency order.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	sharedFunctionsGLSL () noexcept
	{
		return toGLSL(FromUnitToSubUv) + toGLSL(RaySphereNearest) + toGLSL(TransmittanceParametersToUv) + toGLSL(SkyViewParametersToUv) + toGLSL(SkyViewLookup) + toGLSL(Grade);
	}

	/** @brief The compute-only GLSL: the constants, the medium, the phase functions and the inverse parametrisations. */
	constexpr auto ComputeGLSL = R"GLSL(
const float AtmPi = 3.14159265358979;

/* Rays start this far above the ground, so a camera ON the ground is not inside it (Hillaire's PLANET_RADIUS_OFFSET). */
const float AtmPlanetRadiusOffset = 0.01;

const vec2 AtmTransmittanceSize = vec2(256.0, 64.0);
const float AtmMultipleScatteringSize = 32.0;
const vec2 AtmSkyViewSize = vec2(192.0, 108.0);

float atmGroundRadius () { return atmosphere.radii.x; }
float atmTopRadius () { return atmosphere.radii.y; }

float atmFromSubUvToUnit (float u, float resolution) { return (u - (0.5 / resolution)) * (resolution / (resolution - 1.0)); }

struct AtmMedium
{
	vec3 scattering;
	vec3 extinction;
	vec3 scatteringMie;
	vec3 scatteringRayleigh;
};

/* The medium at a point (km, planet centre at the origin). */
AtmMedium atmSampleMedium (vec3 position)
{
	float height = max(length(position) - atmGroundRadius(), 0.0);
	float densityMie = exp(atmosphere.radii.w * height);
	float densityRayleigh = exp(atmosphere.radii.z * height);
	float densityAbsorption = height < atmosphere.mieScattering.w
		? (atmosphere.mieAbsorption.w * height) + atmosphere.mieExtinction.w
		: (atmosphere.groundAlbedo.w * height) + atmosphere.absorptionExtinction.w;

	densityAbsorption = clamp(densityAbsorption, 0.0, 1.0);

	AtmMedium medium;
	medium.scatteringMie = densityMie * atmosphere.mieScattering.rgb;
	medium.scatteringRayleigh = densityRayleigh * atmosphere.rayleighScattering.rgb;
	medium.scattering = medium.scatteringMie + medium.scatteringRayleigh;
	medium.extinction = (densityMie * atmosphere.mieExtinction.rgb) + medium.scatteringRayleigh + (densityAbsorption * atmosphere.absorptionExtinction.rgb);

	return medium;
}

float atmRayleighPhase (float cosTheta)
{
	return (3.0 / (16.0 * AtmPi)) * (1.0 + (cosTheta * cosTheta));
}

/* Cornette-Shanks, peaking TOWARD the light: cosTheta = dot(view direction, direction toward the light). One anisotropy
 * per channel (a dust's forward peak narrows in the blue). */
vec3 atmMiePhase (float cosTheta)
{
	vec3 g = atmosphere.miePhaseG.rgb;
	vec3 k = (3.0 / (8.0 * AtmPi)) * (1.0 - (g * g)) / (2.0 + (g * g));

	return k * (1.0 + (cosTheta * cosTheta)) / pow(max(1.0 + (g * g) - (2.0 * g * cosTheta), vec3(1e-4)), vec3(1.5));
}

/* The transmittance table's inverse parametrisation (Bruneton & Neyret 2008). */
void atmTransmittanceUvToParameters (vec2 uv, out float viewHeight, out float viewZenithCos)
{
	float ground = atmGroundRadius();
	float top = atmTopRadius();
	float H = sqrt(max((top * top) - (ground * ground), 0.0));
	float rho = H * uv.y;

	viewHeight = sqrt((rho * rho) + (ground * ground));

	float dMin = top - viewHeight;
	float dMax = rho + H;
	float d = dMin + (uv.x * (dMax - dMin));

	viewZenithCos = d == 0.0 ? 1.0 : ((H * H) - (rho * rho) - (d * d)) / (2.0 * viewHeight * d);
	viewZenithCos = clamp(viewZenithCos, -1.0, 1.0);
}

/* The sky-view table's inverse parametrisation (Hillaire 2020 § 5.3). */
void atmSkyViewUvToParameters (vec2 uv, float viewHeight, out float viewZenithCos, out float lightViewCos)
{
	uv = vec2(atmFromSubUvToUnit(uv.x, AtmSkyViewSize.x), atmFromSubUvToUnit(uv.y, AtmSkyViewSize.y));

	float ground = atmGroundRadius();
	float horizonDistance = sqrt(max((viewHeight * viewHeight) - (ground * ground), 0.0));
	float beta = acos(clamp(horizonDistance / viewHeight, -1.0, 1.0));
	float zenithHorizonAngle = AtmPi - beta;

	if ( uv.y < 0.5 )
	{
		float coord = 1.0 - (2.0 * uv.y);
		coord = 1.0 - (coord * coord);
		viewZenithCos = cos(zenithHorizonAngle * coord);
	}
	else
	{
		float coord = (uv.y * 2.0) - 1.0;
		viewZenithCos = cos(zenithHorizonAngle + (beta * coord * coord));
	}

	float coord = uv.x * uv.x;
	lightViewCos = -((coord * 2.0) - 1.0);
}
)GLSL";
}
