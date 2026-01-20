/*
* src/Saphir/LightGenerator.cpp
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

#include "LightGenerator.hpp"

/* Local inclusions. */
#include "Generator/Abstract.hpp"
#include "Code.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Graphics;
	using namespace Saphir::Keys;
	using namespace Vulkan;

	bool
	LightGenerator::generateVertexShaderShadowMapCode (Generator::Abstract & generator, VertexShader & vertexShader, bool shadowCubemap) const noexcept
	{
		/* NOTE: For point light. */
		if ( shadowCubemap )
		{
			if ( !vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, "DirectionWorldSpace", GLSL::Smooth}) )
			{
				return false;
			}

			if ( vertexShader.isInstancingEnabled() )
			{
				/* Get the model matrix from VBO. */
				Code{vertexShader, Location::Output} << "DirectionWorldSpace = " << this->lightPositionWorldSpace() << " - " << Attribute::ModelMatrix << " * vec4(" << Attribute::Position << ", 1.0);";
			}
			else
			{
				/* Get the model matrix from UBO. */
				Code{vertexShader, Location::Output} << "DirectionWorldSpace = " << this->lightPositionWorldSpace() << " - " << MatrixPC(PushConstant::Component::ModelMatrix) << " * vec4(" << Attribute::Position << ", 1.0);";
			}
		}
		/* NOTE: For directional and spot-light. */
		else
		{
			if ( !vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, "PositionLightSpace", GLSL::Smooth}) )
			{
				return false;
			}

			if ( vertexShader.isInstancingEnabled() )
			{
				Code{vertexShader, Location::Output} << "PositionLightSpace = " << LightUB(UniformBlock::Component::ViewProjectionMatrix) << " * " << Attribute::ModelMatrix << " * vec4(" << Attribute::Position << ", 1.0);";
			}
			else
			{
				Code{vertexShader, Location::Output} << "PositionLightSpace = " << LightUB(UniformBlock::Component::ViewProjectionMatrix) << " * " << MatrixPC(PushConstant::Component::ModelMatrix) << " * vec4(" << Attribute::Position << ", 1.0);";
			}
		}

		return true;
	}

	std::string
	LightGenerator::generate2DShadowMapCode (const std::string & shadowMap, const std::string & fragmentPosition) const noexcept
	{
		std::stringstream code{};

		/* NOTE: Skip shadow calculation if outside the shadow map's valid depth range.
		 * In clip space, z is in [0, w] range (Vulkan depth [0,1]).
		 * z < 0 means before the near plane, z > w means beyond the far plane.
		 * In both cases, the fragment is not covered by the shadow map, so no shadow. */

		code <<
			"/* Shadow map 2D resolution. */" "\n\n"

			"float shadowFactor = 1.0;" "\n\n"

			"if ( " << fragmentPosition << ".z >= 0.0 && " << fragmentPosition << ".z <= " << fragmentPosition << ".w )" "\n"
			"{" "\n"
			"shadowFactor = textureProj(" << shadowMap << ", " << fragmentPosition << ");" "\n\n"
			"}" "\n\n";

		if ( m_discardUnlitFragment )
		{
			code << "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code.str();
	}

	std::string
	LightGenerator::generate2DShadowMapPCFCode (const std::string & shadowMap, const std::string & fragmentPosition) const noexcept
	{
		std::stringstream code{};

		code << "/* Shadow map 2D resolution (PCF). */" "\n\n";

		code << "float shadowFactor = 1.0;" "\n\n";

		/* NOTE: Skip shadow calculation if outside the shadow map's valid depth range.
		 * In clip space, z is in [0, w] range (Vulkan depth [0,1]).
		 * z < 0 means before the near plane, z > w means beyond the far plane.
		 * In both cases, the fragment is not covered by the shadow map, so no shadow. */
		code <<
			"if ( " << fragmentPosition << ".z >= 0.0 && " << fragmentPosition << ".z <= " << fragmentPosition << ".w )" "\n"
			"{" "\n"
			"	const vec2 texelSize = 1.0 / vec2(textureSize(" << shadowMap << ", 0));" "\n"
			"	const float filterRadius = " << LightUB(UniformBlock::Component::PCFRadius) << ";" "\n\n";

		switch ( m_PCFMethod )
		{
			/* ==================== Grid Method (Legacy) ==================== */
			case PCFMethod::Grid :
			{
				code << 
					GLSL::ConstInteger << " offset = " << m_PCFSample << ";" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( " << GLSL::Integer << " idy = -offset; idy <= offset; idy++ )" "\n"
					"	for ( " << GLSL::Integer << " idx = -offset; idx <= offset; idx++ )" "\n"
					"	{" "\n"
					"		vec4 offsetCoords = " << fragmentPosition << ";" "\n"
					"		offsetCoords.xy += vec2(float(idx), float(idy)) * texelSize * filterRadius * offsetCoords.w;" "\n"
					"		shadowFactor += textureProj(" << shadowMap << ", offsetCoords);" "\n"
					"	}" "\n\n"
					"shadowFactor /= pow(float(offset) * 2.0 + 1.0, 2);" "\n";
			}
				break;

			/* ==================== Vogel Disk Method (Recommended) ==================== */
			case PCFMethod::VogelDisk :
			{
				/* Vogel disk sampling with per-fragment rotation to break up patterns.
				 * The golden angle (2.399963 rad) ensures optimal sample distribution. */
				const auto sampleCount = (2 * m_PCFSample + 1) * (2 * m_PCFSample + 1);

				code <<
					"/* Vogel disk PCF with per-fragment rotation. */" "\n"
					"const float goldenAngle = 2.399963;" "\n"
					"const float rotationAngle = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 6.283185;" "\n"
					"const float cosRot = cos(rotationAngle);" "\n"
					"const float sinRot = sin(rotationAngle);" "\n"
					"const int sampleCount = " << sampleCount << ";" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( int i = 0; i < sampleCount; i++ )" "\n"
					"{" "\n"
					"	float r = sqrt((float(i) + 0.5) / float(sampleCount));" "\n"
					"	float theta = float(i) * goldenAngle + rotationAngle;" "\n"
					"	vec2 offset = vec2(cos(theta), sin(theta)) * r * filterRadius;" "\n"
					"	vec4 offsetCoords = " << fragmentPosition << ";" "\n"
					"	offsetCoords.xy += offset * texelSize * offsetCoords.w;" "\n"
					"	shadowFactor += textureProj(" << shadowMap << ", offsetCoords);" "\n"
					"}" "\n"
					"shadowFactor /= float(sampleCount);" "\n\n";
			}
				break;

			/* ==================== Poisson Disk Method ==================== */
			case PCFMethod::PoissonDisk :
			{
				/* Pre-computed 16-sample Poisson disk for high-quality soft shadows.
				 * These samples are carefully distributed to minimize clustering. */
				code <<
					"/* Poisson disk PCF with 16 pre-computed samples. */" "\n"
					"const vec2 poissonDisk[16] = vec2[](" "\n"
					"	vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725)," "\n"
					"	vec2(-0.09418410, -0.92938870), vec2(0.34495938, 0.29387760)," "\n"
					"	vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464)," "\n"
					"	vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379)," "\n"
					"	vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420)," "\n"
					"	vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188)," "\n"
					"	vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590)," "\n"
					"	vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)" "\n"
					");" "\n"
					"const float rotationAngle = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 6.283185;" "\n"
					"const float cosRot = cos(rotationAngle);" "\n"
					"const float sinRot = sin(rotationAngle);" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( int i = 0; i < 16; i++ )" "\n"
					"{" "\n"
					"	vec2 rotatedOffset = vec2(" "\n"
					"		poissonDisk[i].x * cosRot - poissonDisk[i].y * sinRot," "\n"
					"		poissonDisk[i].x * sinRot + poissonDisk[i].y * cosRot" "\n"
					"	) * filterRadius;" "\n"
					"	vec4 offsetCoords = " << fragmentPosition << ";" "\n"
					"	offsetCoords.xy += rotatedOffset * texelSize * offsetCoords.w;" "\n"
					"	shadowFactor += textureProj(" << shadowMap << ", offsetCoords);" "\n"
					"}" "\n"
					"shadowFactor /= 16.0;" "\n\n";
			}
				break;

			/* ==================== Optimized Gather Method ==================== */
			case PCFMethod::OptimizedGather :
			{
				/* Uses textureGather to fetch 4 samples per call, reducing texture fetches by 4x.
				 * Each textureGather returns a 2x2 quad of comparison results.
				 * NOTE: textureGather does NOT perform perspective division, so we must do it manually.
				 * NOTE: filterRadius is already in UV space (1/resolution), so we use it directly
				 * without multiplying by texelSize. The 2.0 factor accounts for the 2x2 texel block. */
				const auto gatherCount = m_PCFSample + 1; /* Number of gather calls per axis */

				code <<
					"/* Optimized PCF using textureGather (4 samples per fetch). */" "\n"
					"const vec3 projCoords = " << fragmentPosition << ".xyz / " << fragmentPosition << ".w;" "\n"
					"const int gatherOffset = " << gatherCount << ";" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"float totalWeight = 0.0;" "\n"
					"for ( int gy = -gatherOffset; gy <= gatherOffset; gy++ )" "\n"
					"{" "\n"
					"	for ( int gx = -gatherOffset; gx <= gatherOffset; gx++ )" "\n"
					"	{" "\n"
					"		vec2 offsetUV = projCoords.xy + vec2(float(gx), float(gy)) * 2.0 * filterRadius;" "\n"
					"		vec4 gather = textureGather(" << shadowMap << ", offsetUV, projCoords.z);" "\n"
					"		shadowFactor += gather.x + gather.y + gather.z + gather.w;" "\n"
					"		totalWeight += 4.0;" "\n"
					"	}" "\n"
					"}" "\n"
					"shadowFactor /= totalWeight;" "\n\n";
			}
				break;
		}

		/* Close the depth range check block. */
		code << "}" "\n\n";

		if ( m_discardUnlitFragment )
		{
			code << "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code.str();
	}

	std::string
	LightGenerator::generate3DShadowMapCode (const std::string & shadowMap, const std::string & directionWorldSpace, const std::string & nearFar) const noexcept
	{
		std::stringstream code{};

		/* Use max(bias, 0.005) to ensure minimum bias even if UBO value is 0. */
		code <<
			"/* Shadow map 3D (cubemap) resolution. */" "\n\n"

			"float shadowFactor = 1.0;" "\n\n"

			"const vec3 lookupVector = vec3(-" << directionWorldSpace << ".x, " << directionWorldSpace << ".y, " << directionWorldSpace << ".z);" "\n"
			"const float smallestDepth = texture(" << shadowMap << ", lookupVector).r * " << nearFar << ".y;" "\n"
			"const float depth = length(lookupVector);" "\n"
			"const float bias = max(" << LightUB(UniformBlock::Component::ShadowBias) << ", 0.005);" "\n\n"

			"if ( smallestDepth + bias < depth )" "\n"
			"{" "\n"
			"	shadowFactor = 0.0;" "\n"
			"}" "\n\n";

		if ( m_discardUnlitFragment )
		{
			code << "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code.str();
	}

	std::string
	LightGenerator::generate3DShadowMapPCFCode (const std::string & shadowMap, const std::string & directionWorldSpace, const std::string & nearFar) const noexcept
	{
		std::stringstream code{};

		code <<
			"/* Shadow map 3D (cubemap) resolution (PCF). */" "\n\n"

			"float shadowFactor = 1.0;" "\n\n"

			"const vec3 lookupVector = vec3(-" << directionWorldSpace << ".x, " << directionWorldSpace << ".y, " << directionWorldSpace << ".z);" "\n"
			"const float depth = length(lookupVector);" "\n"
			"const vec3 lookupDir = normalize(lookupVector);" "\n"
			"const float bias = " << LightUB(UniformBlock::Component::ShadowBias) << ";" "\n"
			"/* For cubemaps, use PCFRadius scaled by depth for world-space sampling radius. */" "\n"
			"const float filterRadius = depth * " << LightUB(UniformBlock::Component::PCFRadius) << ";" "\n\n";

		switch ( m_PCFMethod )
		{
			/* ==================== Grid Method ==================== */
			case PCFMethod::Grid :
			{
				/* Grid sampling in 3D around the lookup direction. */
				const auto sampleCount = (2 * m_PCFSample + 1) * (2 * m_PCFSample + 1) * (2 * m_PCFSample + 1);

				code <<
					"/* 3D Grid PCF sampling. */" "\n"
					"const int offset = " << m_PCFSample << ";" "\n"
					"const float step = filterRadius / float(offset);" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( int z = -offset; z <= offset; z++ )" "\n"
					"for ( int y = -offset; y <= offset; y++ )" "\n"
					"for ( int x = -offset; x <= offset; x++ )" "\n"
					"{" "\n"
					"	vec3 sampleDir = lookupVector + vec3(float(x), float(y), float(z)) * step;" "\n"
					"	float sampledDepth = texture(" << shadowMap << ", sampleDir).r * " << nearFar << ".y;" "\n"
					"	if ( sampledDepth + bias >= depth ) { shadowFactor += 1.0; }" "\n"
					"}" "\n"
					"shadowFactor /= " << sampleCount << ".0;" "\n\n";
			}
				break;

			/* ==================== Vogel Sphere Method (Recommended for 3D) ==================== */
			case PCFMethod::VogelDisk :
			{
				/* Vogel sphere sampling (Fibonacci sphere distribution).
				 * Uses the golden ratio for optimal 3D sample distribution. */
				const auto sampleCount = (2 * m_PCFSample + 1) * (2 * m_PCFSample + 1);

				code <<
					"/* Vogel sphere PCF (Fibonacci sphere distribution). */" "\n"
					"const float goldenRatio = 1.618033988749895;" "\n"
					"const float pi = 3.14159265359;" "\n"
					"const int sampleCount = " << sampleCount << ";" "\n\n"

					"/* Per-fragment rotation to break up patterns. */" "\n"
					"float noise = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453);" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( int i = 0; i < sampleCount; i++ )" "\n"
					"{" "\n"
					"	/* Fibonacci sphere point distribution. */" "\n"
					"	float y = 1.0 - (float(i) / float(sampleCount - 1)) * 2.0;" "\n"
					"	float radiusAtY = sqrt(1.0 - y * y);" "\n"
					"	float theta = float(i) * 2.0 * pi / goldenRatio + noise * 2.0 * pi;" "\n"
					"	vec3 offset = vec3(cos(theta) * radiusAtY, y, sin(theta) * radiusAtY);" "\n\n"

					"	vec3 sampleDir = lookupVector + offset * filterRadius;" "\n"
					"	float sampledDepth = texture(" << shadowMap << ", sampleDir).r * " << nearFar << ".y;" "\n"
					"	if ( sampledDepth + bias >= depth ) { shadowFactor += 1.0; }" "\n"
					"}" "\n"
					"shadowFactor /= float(sampleCount);" "\n\n";
			}
				break;

			/* ==================== Poisson Sphere Method ==================== */
			case PCFMethod::PoissonDisk :
			{
				/* Pre-computed 20-point Poisson sphere distribution.
				 * These points are uniformly distributed on a unit sphere. */
				code <<
					"/* Poisson sphere PCF with 20 pre-computed samples. */" "\n"
					"const vec3 poissonSphere[20] = vec3[](" "\n"
					"	vec3( 0.5381, 0.1856,-0.4319), vec3( 0.1379, 0.2486, 0.4430)," "\n"
					"	vec3( 0.3371, 0.5679,-0.0057), vec3(-0.6999,-0.0451,-0.0019)," "\n"
					"	vec3( 0.0689,-0.1598,-0.8547), vec3( 0.0560, 0.0069,-0.1843)," "\n"
					"	vec3(-0.0146, 0.1402, 0.0762), vec3( 0.0100,-0.1924,-0.0344)," "\n"
					"	vec3(-0.3577,-0.5301,-0.4358), vec3(-0.3169, 0.1063, 0.0158)," "\n"
					"	vec3( 0.0103,-0.5869, 0.0046), vec3(-0.0897,-0.4940, 0.3287)," "\n"
					"	vec3( 0.7119,-0.0154,-0.0918), vec3(-0.0533, 0.0596,-0.5411)," "\n"
					"	vec3( 0.0352,-0.0631, 0.5460), vec3(-0.4776, 0.2847,-0.0271)," "\n"
					"	vec3(-0.2420, 0.5763, 0.3370), vec3( 0.5765, 0.3331, 0.5170)," "\n"
					"	vec3(-0.5836,-0.3541, 0.2407), vec3( 0.2890, 0.7152,-0.2167)" "\n"
					");" "\n\n"

					"/* Per-fragment rotation matrix to break up patterns. */" "\n"
					"float noise = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 6.283185;" "\n"
					"float cosN = cos(noise);" "\n"
					"float sinN = sin(noise);" "\n"
					"mat3 rotation = mat3(" "\n"
					"	cosN, sinN, 0.0," "\n"
					"	-sinN, cosN, 0.0," "\n"
					"	0.0, 0.0, 1.0" "\n"
					");" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( int i = 0; i < 20; i++ )" "\n"
					"{" "\n"
					"	vec3 offset = rotation * poissonSphere[i];" "\n"
					"	vec3 sampleDir = lookupVector + offset * filterRadius;" "\n"
					"	float sampledDepth = texture(" << shadowMap << ", sampleDir).r * " << nearFar << ".y;" "\n"
					"	if ( sampledDepth + bias >= depth ) { shadowFactor += 1.0; }" "\n"
					"}" "\n"
					"shadowFactor /= 20.0;" "\n\n";
			}
				break;

			/* ==================== Optimized Gather (fallback to Poisson for cubemaps) ==================== */
			case PCFMethod::OptimizedGather :
			{
				/* textureGather doesn't work with cubemaps in the same way,
				 * so we fall back to Poisson sphere sampling. */
				code <<
					"/* OptimizedGather not available for cubemaps, using Poisson sphere. */" "\n"
					"const vec3 poissonSphere[20] = vec3[](" "\n"
					"	vec3( 0.5381, 0.1856,-0.4319), vec3( 0.1379, 0.2486, 0.4430)," "\n"
					"	vec3( 0.3371, 0.5679,-0.0057), vec3(-0.6999,-0.0451,-0.0019)," "\n"
					"	vec3( 0.0689,-0.1598,-0.8547), vec3( 0.0560, 0.0069,-0.1843)," "\n"
					"	vec3(-0.0146, 0.1402, 0.0762), vec3( 0.0100,-0.1924,-0.0344)," "\n"
					"	vec3(-0.3577,-0.5301,-0.4358), vec3(-0.3169, 0.1063, 0.0158)," "\n"
					"	vec3( 0.0103,-0.5869, 0.0046), vec3(-0.0897,-0.4940, 0.3287)," "\n"
					"	vec3( 0.7119,-0.0154,-0.0918), vec3(-0.0533, 0.0596,-0.5411)," "\n"
					"	vec3( 0.0352,-0.0631, 0.5460), vec3(-0.4776, 0.2847,-0.0271)," "\n"
					"	vec3(-0.2420, 0.5763, 0.3370), vec3( 0.5765, 0.3331, 0.5170)," "\n"
					"	vec3(-0.5836,-0.3541, 0.2407), vec3( 0.2890, 0.7152,-0.2167)" "\n"
					");" "\n\n"

					"float noise = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 6.283185;" "\n"
					"float cosN = cos(noise);" "\n"
					"float sinN = sin(noise);" "\n"
					"mat3 rotation = mat3(cosN, sinN, 0.0, -sinN, cosN, 0.0, 0.0, 0.0, 1.0);" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( int i = 0; i < 20; i++ )" "\n"
					"{" "\n"
					"	vec3 offset = rotation * poissonSphere[i];" "\n"
					"	vec3 sampleDir = lookupVector + offset * filterRadius;" "\n"
					"	float sampledDepth = texture(" << shadowMap << ", sampleDir).r * " << nearFar << ".y;" "\n"
					"	if ( sampledDepth + bias >= depth ) { shadowFactor += 1.0; }" "\n"
					"}" "\n"
					"shadowFactor /= 20.0;" "\n\n";
			}
				break;
		}

		if ( m_discardUnlitFragment )
		{
			code << "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code.str();
	}

	std::string
	LightGenerator::generateCSMShadowMapCode (const std::string & shadowMapArray, const std::string & fragmentPositionWorldSpace, const std::string & viewMatrix, const std::string & cascadeMatrices, const std::string & splitDistances, const std::string & cascadeCount) const noexcept
	{
		std::stringstream code{};

		code << "/* Cascaded Shadow Map resolution. */" "\n\n";

		code << "float shadowFactor = 1.0;" "\n\n";

		/* Compute view-space depth for cascade selection. */
		code <<
			"/* Compute view-space depth for cascade selection. */" "\n"
			"const float viewDepth = abs((" << viewMatrix << " * vec4(" << fragmentPositionWorldSpace << ", 1.0)).z);" "\n\n";

		/* Determine which cascade to use based on view-space depth. */
		code <<
			"/* Select the appropriate cascade based on depth. */" "\n"
			"int cascadeIndex = 0;" "\n"
			"const int numCascades = int(" << cascadeCount << ");" "\n"
			"for ( int i = 0; i < numCascades; i++ )" "\n"
			"{" "\n"
			"	if ( viewDepth < " << splitDistances << "[i] )" "\n"
			"	{" "\n"
			"		cascadeIndex = i;" "\n"
			"		break;" "\n"
			"	}" "\n"
			"	cascadeIndex = i;" "\n"
			"}" "\n\n";

		/* Transform fragment position to light space using the selected cascade matrix. */
		code <<
			"/* Transform to the selected cascade's light space. */" "\n"
			"vec4 posLightSpace = " << cascadeMatrices << "[cascadeIndex] * vec4(" << fragmentPositionWorldSpace << ", 1.0);" "\n"
			"vec3 projCoords = posLightSpace.xyz / posLightSpace.w;" "\n"
			"/* NOTE: Only X and Y need [-1,1] to [0,1] conversion for UV coordinates. */" "\n"
			"/* Z is already in [0,1] range from Vulkan orthographic projection. */" "\n"
			"projCoords.xy = projCoords.xy * 0.5 + 0.5;" "\n\n";

		/* Skip shadow calculation if outside the shadow map's valid depth range. */
		code <<
			"if ( projCoords.z >= 0.0 && projCoords.z <= 1.0 )" "\n"
			"{" "\n";

		if ( m_PCFEnabled )
		{
			code << "	" << GLSL::ConstInteger << " offset = " << m_PCFSample << ";" "\n\n";

			/* NOTE: Reset shadowFactor to 0.0 before accumulating PCF samples.
			 * The initial value of 1.0 is only for the non-shadow case (outside depth range). */
			code << "	shadowFactor = 0.0;" "\n\n";

			/* PCF sampling with sampler2DArrayShadow. */
			code <<
				"	for ( " << GLSL::Integer << " idy = -offset; idy <= offset; idy++ )" "\n"
				"	{" "\n"
				"		for ( " << GLSL::Integer << " idx = -offset; idx <= offset; idx++ )" "\n"
				"		{" "\n"
				"			vec2 texelSize = 1.0 / vec2(textureSize(" << shadowMapArray << ", 0).xy);" "\n"
				"			vec2 offsetUV = projCoords.xy + vec2(float(idx), float(idy)) * texelSize;" "\n"
				"			shadowFactor += texture(" << shadowMapArray << ", vec4(offsetUV, float(cascadeIndex), projCoords.z));" "\n"
				"		}" "\n"
				"	}" "\n\n"

				"	shadowFactor /= pow(float(offset) * 2.0 + 1.0, 2);" "\n";
		}
		else
		{
			/* Single sample with sampler2DArrayShadow.
			 * The fourth component is the reference depth for comparison. */
			code <<
				"	shadowFactor = texture(" << shadowMapArray << ", vec4(projCoords.xy, float(cascadeIndex), projCoords.z));" "\n";
		}

		code << "}" "\n\n";

		if ( m_discardUnlitFragment )
		{
			code << "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code.str();
	}
}
