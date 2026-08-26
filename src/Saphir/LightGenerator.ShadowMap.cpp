/*
 * src/Saphir/LightGenerator.ShadowMap.cpp
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

#include "LightGenerator.hpp"

/* Local inclusions. */
#include "Code.hpp"
#include "Generator/Abstract.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Graphics;
	using namespace Saphir::Keys;
	using namespace Vulkan;

	/**
	 * @brief Builds the GLSL condition stating that a projective light-space position falls INSIDE
	 * the 2D shadow map volume, on all three axes.
	 * @note A DIRECTIONAL light is a light at infinity: its map covers a finite box, and everything
	 * outside that box is NOT "in shadow", it is "unknown" — and the only physically defensible
	 * answer for a sun is LIT. Guarding all three axes here makes that semantic a property of the
	 * generated code instead of a property of the sampler's address mode: the border mode still
	 * handles the PCF taps that stray across the edge, but the centre sample can no longer be
	 * decided by whatever addressing the sampler cache happened to hand out.
	 * ⚠️ Z alone was guarded before, so lateral overflow fell through to the sampler — which,
	 * through a cache-key collision, was CLAMP_TO_EDGE: the edge texel ring shadowed the entire
	 * exterior and produced a broad black band past the coverage box.
	 * @note Shared with SPOT lights, and correct for them too: outside its cone a spot already has a
	 * zero cone factor, so answering LIT outside the map adds no light anywhere.
	 * @param fragmentPosition The GLSL expression naming the light-space position (xyzw, pre-divide).
	 * @return std::string
	 */
	[[nodiscard]]
	static
	std::string
	insideShadowVolumeCondition (const std::string & fragmentPosition)
	{
		std::string condition;
		condition.reserve(160 + (fragmentPosition.size() * 7));

		/* NOTE: The coordinates are still projective (the perspective divide happens in
		 * textureProj), so each axis is tested against w, not against 1.0. Requiring z >= 0 and
		 * z <= w also rules out w < 0, i.e. anything behind the light. */
		condition += fragmentPosition;
		condition += ".z >= 0.0 && ";
		condition += fragmentPosition;
		condition += ".z <= ";
		condition += fragmentPosition;
		condition += ".w && ";
		condition += fragmentPosition;
		condition += ".x >= 0.0 && ";
		condition += fragmentPosition;
		condition += ".x <= ";
		condition += fragmentPosition;
		condition += ".w && ";
		condition += fragmentPosition;
		condition += ".y >= 0.0 && ";
		condition += fragmentPosition;
		condition += ".y <= ";
		condition += fragmentPosition;
		condition += ".w";

		return condition;
	}

	bool
	LightGenerator::generateVertexShaderShadowMapCode (Generator::Abstract & generator, VertexShader & vertexShader, bool shadowCubemap) const noexcept
	{
		/* The shadow term MUST be evaluated at the SKINNED position: the shadow map holds the
		 * ANIMATED mesh depth (the shadow pass skins), so sampling it at the bind-pose vertex
		 * position made every animated pose self-occlude — the whole body flickered down to the
		 * ambient term on fast animation frames (measured on the reflexion-debug dragon). */
		std::string localPosition;

		if ( vertexShader.isSkinningEnabled() )
		{
			localPosition = "vec4(skinnedPosition, 1.0)";
		}
		else
		{
			localPosition.reserve(32);
			localPosition = "vec4(";
			localPosition += Attribute::Position;
			localPosition += ", 1.0)";
		}

		/* NOTE: For point light. */
		if ( shadowCubemap )
		{
			if ( !vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, "DirectionWorldSpace", GLSL::Smooth}) )
			{
				return false;
			}

			if ( vertexShader.isInstancingEnabled() )
			{
				if ( vertexShader.isBillBoardingEnabled() )
				{
					/* Billboard sprite: the model matrix is computed in-shader
					 * (SpriteModelMatrix) from the per-instance position/scaling, not
					 * supplied as the vaModelMatrix vertex attribute. */
					Code{vertexShader, Location::Output} << "DirectionWorldSpace = " << this->lightPositionWorldSpace() << " - " << ShaderVariable::SpriteModelMatrix << " * " << localPosition << ";";
				}
				else
				{
					/* Get the model matrix from VBO. */
					Code{vertexShader, Location::Output} << "DirectionWorldSpace = " << this->lightPositionWorldSpace() << " - " << Attribute::ModelMatrix << " * " << localPosition << ";";
				}
			}
			else if ( vertexShader.isInstanceTransformsEnabled() )
			{
				/* Get the model matrix from the InstanceTransforms SSBO entry.
				 * NOTE: The preparation is guaranteed requested by the MVP synthesis. */
				Code{vertexShader, Location::Output} << "DirectionWorldSpace = " << this->lightPositionWorldSpace() << " - " << ShaderVariable::InstanceModelMatrix << " * " << localPosition << ";";
			}
			else
			{
				/* Get the model matrix from the push constants. */
				Code{vertexShader, Location::Output} << "DirectionWorldSpace = " << this->lightPositionWorldSpace() << " - " << MatrixPC(PushConstant::Component::ModelMatrix) << " * " << localPosition << ";";
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
				if ( vertexShader.isBillBoardingEnabled() )
				{
					/* Billboard sprite: the model matrix is computed in-shader
					 * (SpriteModelMatrix) from the per-instance position/scaling, not
					 * supplied as the vaModelMatrix vertex attribute. */
					Code{vertexShader, Location::Output} << "PositionLightSpace = " << LightUB(UniformBlock::Component::ViewProjectionMatrix) << " * " << ShaderVariable::SpriteModelMatrix << " * " << localPosition << ";";
				}
				else
				{
					Code{vertexShader, Location::Output} << "PositionLightSpace = " << LightUB(UniformBlock::Component::ViewProjectionMatrix) << " * " << Attribute::ModelMatrix << " * " << localPosition << ";";
				}
			}
			else if ( vertexShader.isInstanceTransformsEnabled() )
			{
				/* Get the model matrix from the InstanceTransforms SSBO entry.
				 * NOTE: The preparation is guaranteed requested by the MVP synthesis. */
				Code{vertexShader, Location::Output} << "PositionLightSpace = " << LightUB(UniformBlock::Component::ViewProjectionMatrix) << " * " << ShaderVariable::InstanceModelMatrix << " * " << localPosition << ";";
			}
			else
			{
				Code{vertexShader, Location::Output} << "PositionLightSpace = " << LightUB(UniformBlock::Component::ViewProjectionMatrix) << " * " << MatrixPC(PushConstant::Component::ModelMatrix) << " * " << localPosition << ";";
			}
		}

		return true;
	}

	std::string
	LightGenerator::generate2DShadowMapCode (const std::string & shadowMap, const std::string & fragmentPosition) const noexcept
	{
		std::string code;
		code.reserve(320 + shadowMap.size() + (fragmentPosition.size() * 8));

		/* NOTE: Skip the shadow lookup entirely when the fragment falls outside the shadow map
		 * volume — laterally as well as in depth. shadowFactor keeps its 1.0 initial value, so
		 * "not covered by the map" reads as LIT. See insideShadowVolumeCondition(). */

		code +=
			"/* Shadow map 2D resolution. */" "\n\n"

			"float shadowFactor = 1.0;" "\n\n"

			"if ( ";
		code += insideShadowVolumeCondition(fragmentPosition);
		code +=
			" )" "\n"
			"{" "\n"
			"shadowFactor = textureProj(";
		code += shadowMap;
		code += ", ";
		code += fragmentPosition;
		code +=
			");" "\n\n"
			"}" "\n\n";

		if ( m_discardUnlitFragment )
		{
			code += "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code;
	}

	std::string
	LightGenerator::generate2DShadowMapPCFCode (const std::string & shadowMap, const std::string & fragmentPosition, const std::string & fragmentPositionWorldSpace) const noexcept
	{
		std::string code;
		code.reserve(1600 + (shadowMap.size() * 2) + (fragmentPosition.size() * 9));

		code += "/* Shadow map 2D resolution (PCF). */" "\n\n";

		code += "float shadowFactor = 1.0;" "\n\n";

		/* NOTE: Skip the shadow lookup entirely when the fragment falls outside the shadow map
		 * volume — laterally as well as in depth. shadowFactor keeps its 1.0 initial value, so
		 * "not covered by the map" reads as LIT. The individual PCF taps may still stray past the
		 * edge; those are absorbed by the sampler's border (opaque white = unoccluded), which is
		 * what keeps the transition smooth instead of stair-stepping at the coverage limit.
		 * See insideShadowVolumeCondition(). */
		code += "if ( ";
		code += insideShadowVolumeCondition(fragmentPosition);
		code +=
			" )" "\n"
			"{" "\n"
			"	const vec2 texelSize = 1.0 / vec2(textureSize(";
		code += shadowMap;
		code += ", 0));" "\n"
			"	const float filterRadius = ";
		code += LightUB(UniformBlock::Component::PCFRadius);
		code += ";" "\n\n";

		switch ( m_PCFMethod )
		{
			/* ==================== Grid Method (Legacy) ==================== */
			case PCFMethod::Grid :
			{
				code += GLSL::ConstInteger;
				code += " offset = ";
				code += std::to_string(m_PCFSample);
				code +=
					";" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( ";
				code += GLSL::Integer;
				code += " idy = -offset; idy <= offset; idy++ )" "\n"
					"	for ( ";
				code += GLSL::Integer;
				code += " idx = -offset; idx <= offset; idx++ )" "\n"
					"	{" "\n"
					"		vec4 offsetCoords = ";
				code += fragmentPosition;
				code +=
					";" "\n"
					"		offsetCoords.xy += vec2(float(idx), float(idy)) * texelSize * filterRadius * offsetCoords.w;" "\n"
					"		shadowFactor += textureProj(";
				code += shadowMap;
				code +=
					", offsetCoords);" "\n"
					"	}" "\n\n"
					"shadowFactor /= pow(float(offset) * 2.0 + 1.0, 2);" "\n";
			}
				break;

			/* ==================== Vogel Disk Method (Recommended) ==================== */
			case PCFMethod::VogelDisk :
			{
				/* Vogel disk sampling with per-fragment rotation to break up patterns.
				 * The golden angle (2.399963 rad) ensures optimal sample distribution. */
				const auto sampleCount = ((2U * m_PCFSample) + 1U) * ((2U * m_PCFSample) + 1U);

				code +=
					"/* Vogel disk PCF with per-fragment rotation. */" "\n"
					"const float goldenAngle = 2.399963;" "\n"
					"/* ⚠️⚠️ The kernel rotation is hashed from the fragment's WORLD position, never from" "\n"
					"   gl_FragCoord. A screen-space hash with no frame index mixed in is a noise field FIXED" "\n"
					"   IN SCREEN SPACE: surfaces slide THROUGH it as the camera moves, which reads as crawl," "\n"
					"   and a temporal filter is structurally unable to clean it — averaging a value that is" "\n"
					"   constant in time returns that constant. Anchored to the world, the rotation belongs to" "\n"
					"   the surface: it is the same every frame for a given point, so nothing crawls, and it" "\n"
					"   stays stable even when the LIGHT moves (a carried torch), which a light-space anchor" "\n"
					"   would not. */" "\n"
					"float rotationAngle = fract(sin(dot(" + fragmentPositionWorldSpace + ", vec3(12.9898, 78.233, 37.719))) * 43758.5453) * 6.283185;" "\n"
					"const float cosRot = cos(rotationAngle);" "\n"
					"const float sinRot = sin(rotationAngle);" "\n"
					"const int sampleCount = ";
				code += std::to_string(sampleCount);
				code +=
					";" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( int i = 0; i < sampleCount; i++ )" "\n"
					"{" "\n"
					"	float r = sqrt((float(i) + 0.5) / float(sampleCount));" "\n"
					"	float theta = float(i) * goldenAngle + rotationAngle;" "\n"
					"	vec2 offset = vec2(cos(theta), sin(theta)) * r * filterRadius;" "\n"
					"	vec4 offsetCoords = ";
				code += fragmentPosition;
				code +=
					";" "\n"
					"	offsetCoords.xy += offset * texelSize * offsetCoords.w;" "\n"
					"	shadowFactor += textureProj(";
				code += shadowMap;
				code +=
					", offsetCoords);" "\n"
					"}" "\n"
					"shadowFactor /= float(sampleCount);" "\n\n";
			}
				break;

			/* ==================== Poisson Disk Method ==================== */
			case PCFMethod::PoissonDisk :
			{
				/* Pre-computed 16-sample Poisson disk for high-quality soft shadows.
				 * These samples are carefully distributed to minimize clustering. */
				code +=
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
					"/* ⚠️⚠️ The kernel rotation is hashed from the fragment's WORLD position, never from" "\n"
					"   gl_FragCoord. A screen-space hash with no frame index mixed in is a noise field FIXED" "\n"
					"   IN SCREEN SPACE: surfaces slide THROUGH it as the camera moves, which reads as crawl," "\n"
					"   and a temporal filter is structurally unable to clean it — averaging a value that is" "\n"
					"   constant in time returns that constant. Anchored to the world, the rotation belongs to" "\n"
					"   the surface: it is the same every frame for a given point, so nothing crawls, and it" "\n"
					"   stays stable even when the LIGHT moves (a carried torch), which a light-space anchor" "\n"
					"   would not. */" "\n"
					"float rotationAngle = fract(sin(dot(" + fragmentPositionWorldSpace + ", vec3(12.9898, 78.233, 37.719))) * 43758.5453) * 6.283185;" "\n"
					"const float cosRot = cos(rotationAngle);" "\n"
					"const float sinRot = sin(rotationAngle);" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( int i = 0; i < 16; i++ )" "\n"
					"{" "\n"
					"	vec2 rotatedOffset = vec2(" "\n"
					"		poissonDisk[i].x * cosRot - poissonDisk[i].y * sinRot," "\n"
					"		poissonDisk[i].x * sinRot + poissonDisk[i].y * cosRot" "\n"
					"	) * filterRadius;" "\n"
					"	vec4 offsetCoords = ";
				code += fragmentPosition;
				code +=
					";" "\n"
					"	offsetCoords.xy += rotatedOffset * texelSize * offsetCoords.w;" "\n"
					"	shadowFactor += textureProj(";
				code += shadowMap;
				code +=
					", offsetCoords);" "\n"
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

				code +=
					"/* Optimized PCF using textureGather (4 samples per fetch). */" "\n"
					"const vec3 projCoords = ";
				code += fragmentPosition;
				code += ".xyz / ";
				code += fragmentPosition;
				code += ".w;" "\n"
					"const int gatherOffset = ";
				code += std::to_string(gatherCount);
				code +=
					";" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"float totalWeight = 0.0;" "\n"
					"for ( int gy = -gatherOffset; gy <= gatherOffset; gy++ )" "\n"
					"{" "\n"
					"	for ( int gx = -gatherOffset; gx <= gatherOffset; gx++ )" "\n"
					"	{" "\n"
					"		vec2 offsetUV = projCoords.xy + vec2(float(gx), float(gy)) * 2.0 * filterRadius;" "\n"
					"		vec4 gather = textureGather(";
				code += shadowMap;
				code +=
					", offsetUV, projCoords.z);" "\n"
					"		shadowFactor += gather.x + gather.y + gather.z + gather.w;" "\n"
					"		totalWeight += 4.0;" "\n"
					"	}" "\n"
					"}" "\n"
					"shadowFactor /= totalWeight;" "\n\n";
			}
				break;
		}

		/* Close the depth range check block. */
		code += "}" "\n\n";

		if ( m_discardUnlitFragment )
		{
			code += "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code;
	}

	std::string
	LightGenerator::generate3DShadowMapCode (const std::string & shadowMap, const std::string & directionWorldSpace, const std::string & nearFar) const noexcept
	{
		std::string code;
		code.reserve(384 + shadowMap.size() + (directionWorldSpace.size() * 3) + nearFar.size());

		/* Use max(bias, 0.005) to ensure minimum bias even if UBO value is 0. */
		code +=
			"/* Shadow map 3D (cubemap) resolution. */" "\n\n"

			"float shadowFactor = 1.0;" "\n\n"

			/* The direction from the LIGHT to the fragment, which is the plain negation of
			 * DirectionWorldSpace (fragment TO light). Nothing else: the cubemap render path —
			 * the face table, the undone projection Y flip and the inverted winding — already
			 * makes `texture(cube, d)` return what was rendered in direction `d`, and that path is
			 * SHARED with the reflection probes.
			 *
			 * ⚠️ It used to read `vec3(-x, y, z)`: an X-only negation dating from 0.8.5, i.e. a
			 * compensation for the Y-DOWN world that the Aug 2026 Y-up flip never revisited. The
			 * symptom was spectacular and took a while to be reported — on `global-illumination`,
			 * the walking paladin's shadow was cast on the CEILING, with no shadow at all under
			 * his feet. Negating Y alone brought it down to the floor but left it MIRRORED along
			 * the corridor; only the full negation puts it at his feet, pointing away from the
			 * light.
			 *
			 * ⚠️⚠️ The lookup is NOT shared with the reflection probes — the render is. So a
			 * cubemap change validated on probes alone says NOTHING about shadows, and vice
			 * versa: check both. */
			"const vec3 lookupVector = vec3(-";
		code += directionWorldSpace;
		code += ".x, -";
		code += directionWorldSpace;
		code += ".y, -";
		code += directionWorldSpace;
		code += ".z);" "\n"
			"const float smallestDepth = texture(";
		code += shadowMap;
		code += ", lookupVector).r * ";
		code += nearFar;
		code +=
			".y;" "\n"
			"const float depth = length(lookupVector);" "\n"
			"const float bias = max(";
		code += LightUB(UniformBlock::Component::ShadowBias);
		code +=
			", 0.005);" "\n\n"

			"if ( smallestDepth + bias < depth )" "\n"
			"{" "\n"
			"	shadowFactor = 0.0;" "\n"
			"}" "\n\n";

		if ( m_discardUnlitFragment )
		{
			code += "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code;
	}

	std::string
	LightGenerator::generate3DShadowMapPCFCode (const std::string & shadowMap, const std::string & directionWorldSpace, const std::string & nearFar, const std::string & fragmentPositionWorldSpace) const noexcept
	{
		std::string code;
		code.reserve(2048 + (shadowMap.size() * 4) + (directionWorldSpace.size() * 3) + (nearFar.size() * 4));

		code +=
			"/* Shadow map 3D (cubemap) resolution (PCF). */" "\n\n"

			"float shadowFactor = 1.0;" "\n\n"

			/* The direction from the LIGHT to the fragment, which is the plain negation of
			 * DirectionWorldSpace (fragment TO light). Nothing else: the cubemap render path —
			 * the face table, the undone projection Y flip and the inverted winding — already
			 * makes `texture(cube, d)` return what was rendered in direction `d`, and that path is
			 * SHARED with the reflection probes.
			 *
			 * ⚠️ It used to read `vec3(-x, y, z)`: an X-only negation dating from 0.8.5, i.e. a
			 * compensation for the Y-DOWN world that the Aug 2026 Y-up flip never revisited. The
			 * symptom was spectacular and took a while to be reported — on `global-illumination`,
			 * the walking paladin's shadow was cast on the CEILING, with no shadow at all under
			 * his feet. Negating Y alone brought it down to the floor but left it MIRRORED along
			 * the corridor; only the full negation puts it at his feet, pointing away from the
			 * light.
			 *
			 * ⚠️⚠️ The lookup is NOT shared with the reflection probes — the render is. So a
			 * cubemap change validated on probes alone says NOTHING about shadows, and vice
			 * versa: check both. */
			"const vec3 lookupVector = vec3(-";
		code += directionWorldSpace;
		code += ".x, -";
		code += directionWorldSpace;
		code += ".y, -";
		code += directionWorldSpace;
		code +=
			".z);" "\n"
			"const float depth = length(lookupVector);" "\n"
			"const vec3 lookupDir = normalize(lookupVector);" "\n"
			"const float bias = ";
		code += LightUB(UniformBlock::Component::ShadowBias);
		code +=
			";" "\n"
			"/* For cubemaps, use PCFRadius scaled by depth for world-space sampling radius. */" "\n"
			"const float filterRadius = depth * ";
		code += LightUB(UniformBlock::Component::PCFRadius);
		code += ";" "\n\n";

		switch ( m_PCFMethod )
		{
			/* ==================== Grid Method ==================== */
			case PCFMethod::Grid :
			{
				/* Grid sampling in 3D around the lookup direction. */
				const auto sampleCount = ((2U * m_PCFSample) + 1U) * ((2U * m_PCFSample) + 1U) * ((2U * m_PCFSample) + 1U);

				code +=
					"/* 3D Grid PCF sampling. */" "\n"
					"const int offset = ";
				code += std::to_string(m_PCFSample);
				code +=
					";" "\n"
					"const float step = filterRadius / float(offset);" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( int z = -offset; z <= offset; z++ )" "\n"
					"for ( int y = -offset; y <= offset; y++ )" "\n"
					"for ( int x = -offset; x <= offset; x++ )" "\n"
					"{" "\n"
					"	vec3 sampleDir = lookupVector + vec3(float(x), float(y), float(z)) * step;" "\n"
					"	float sampledDepth = texture(";
				code += shadowMap;
				code += ", sampleDir).r * ";
				code += nearFar;
				code +=
					".y;" "\n"
					"	if ( sampledDepth + bias >= depth ) { shadowFactor += 1.0; }" "\n"
					"}" "\n"
					"shadowFactor /= ";
				code += std::to_string(sampleCount);
				code += ".0;" "\n\n";
			}
				break;

			/* ==================== Vogel Sphere Method (Recommended for 3D) ==================== */
			case PCFMethod::VogelDisk :
			{
				/* Vogel sphere sampling (Fibonacci sphere distribution).
				 * Uses the golden ratio for optimal 3D sample distribution. */
				const auto sampleCount = ((2U * m_PCFSample) + 1U) * ((2U * m_PCFSample) + 1U);

				code +=
					"/* Vogel sphere PCF (Fibonacci sphere distribution). */" "\n"
					"const float goldenRatio = 1.618033988749895;" "\n"
					"const float pi = 3.14159265359;" "\n"
					"const int sampleCount = ";
				code += std::to_string(sampleCount);
				code +=
					";" "\n\n"

					"/* Per-fragment rotation to break up patterns. */" "\n"
					"/* ⚠️⚠️ The kernel rotation is hashed from the fragment's WORLD position, never from" "\n"
					"   gl_FragCoord. A screen-space hash with no frame index mixed in is a noise field FIXED" "\n"
					"   IN SCREEN SPACE: surfaces slide THROUGH it as the camera moves, which reads as crawl," "\n"
					"   and a temporal filter is structurally unable to clean it — averaging a value that is" "\n"
					"   constant in time returns that constant. Anchored to the world, the rotation belongs to" "\n"
					"   the surface: it is the same every frame for a given point, so nothing crawls, and it" "\n"
					"   stays stable even when the LIGHT moves (a carried torch), which a light-space anchor" "\n"
					"   would not. */" "\n"
					"float noise = fract(sin(dot(" + fragmentPositionWorldSpace + ", vec3(12.9898, 78.233, 37.719))) * 43758.5453);" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( int i = 0; i < sampleCount; i++ )" "\n"
					"{" "\n"
					"	/* Fibonacci sphere point distribution. */" "\n"
					"	float y = 1.0 - (float(i) / float(sampleCount - 1)) * 2.0;" "\n"
					"	float radiusAtY = sqrt(1.0 - y * y);" "\n"
					"	float theta = float(i) * 2.0 * pi / goldenRatio + noise * 2.0 * pi;" "\n"
					"	vec3 offset = vec3(cos(theta) * radiusAtY, y, sin(theta) * radiusAtY);" "\n\n"

					"	vec3 sampleDir = lookupVector + offset * filterRadius;" "\n"
					"	float sampledDepth = texture(";
				code += shadowMap;
				code += ", sampleDir).r * ";
				code += nearFar;
				code +=
					".y;" "\n"
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
				code +=
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
					"/* ⚠️⚠️ The kernel rotation is hashed from the fragment's WORLD position, never from" "\n"
					"   gl_FragCoord. A screen-space hash with no frame index mixed in is a noise field FIXED" "\n"
					"   IN SCREEN SPACE: surfaces slide THROUGH it as the camera moves, which reads as crawl," "\n"
					"   and a temporal filter is structurally unable to clean it — averaging a value that is" "\n"
					"   constant in time returns that constant. Anchored to the world, the rotation belongs to" "\n"
					"   the surface: it is the same every frame for a given point, so nothing crawls, and it" "\n"
					"   stays stable even when the LIGHT moves (a carried torch), which a light-space anchor" "\n"
					"   would not. */" "\n"
					"float noise = fract(sin(dot(" + fragmentPositionWorldSpace + ", vec3(12.9898, 78.233, 37.719))) * 43758.5453) * 6.283185;" "\n"
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
					"	float sampledDepth = texture(";
				code += shadowMap;
				code += ", sampleDir).r * ";
				code += nearFar;
				code +=
					".y;" "\n"
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
				code +=
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

					"/* ⚠️⚠️ The kernel rotation is hashed from the fragment's WORLD position, never from" "\n"
					"   gl_FragCoord. A screen-space hash with no frame index mixed in is a noise field FIXED" "\n"
					"   IN SCREEN SPACE: surfaces slide THROUGH it as the camera moves, which reads as crawl," "\n"
					"   and a temporal filter is structurally unable to clean it — averaging a value that is" "\n"
					"   constant in time returns that constant. Anchored to the world, the rotation belongs to" "\n"
					"   the surface: it is the same every frame for a given point, so nothing crawls, and it" "\n"
					"   stays stable even when the LIGHT moves (a carried torch), which a light-space anchor" "\n"
					"   would not. */" "\n"
					"float noise = fract(sin(dot(" + fragmentPositionWorldSpace + ", vec3(12.9898, 78.233, 37.719))) * 43758.5453) * 6.283185;" "\n"
					"float cosN = cos(noise);" "\n"
					"float sinN = sin(noise);" "\n"
					"mat3 rotation = mat3(cosN, sinN, 0.0, -sinN, cosN, 0.0, 0.0, 0.0, 1.0);" "\n\n"

					"shadowFactor = 0.0;" "\n"
					"for ( int i = 0; i < 20; i++ )" "\n"
					"{" "\n"
					"	vec3 offset = rotation * poissonSphere[i];" "\n"
					"	vec3 sampleDir = lookupVector + offset * filterRadius;" "\n"
					"	float sampledDepth = texture(";
				code += shadowMap;
				code += ", sampleDir).r * ";
				code += nearFar;
				code +=
					".y;" "\n"
					"	if ( sampledDepth + bias >= depth ) { shadowFactor += 1.0; }" "\n"
					"}" "\n"
					"shadowFactor /= 20.0;" "\n\n";
			}
				break;
		}

		if ( m_discardUnlitFragment )
		{
			code += "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code;
	}

	std::string
	LightGenerator::generateCSMShadowMapCode (const std::string & shadowMapArray, const std::string & fragmentPositionWorldSpace, const std::string & fragmentPositionViewSpace, const std::string & cascadeMatrices, const std::string & splitDistances, const std::string & cascadeCount, const std::string & shadowBias) const noexcept
	{
		std::string code;
		code.reserve(1280 + (shadowMapArray.size() * 3) + fragmentPositionWorldSpace.size() + fragmentPositionViewSpace.size() + cascadeMatrices.size() + splitDistances.size() + cascadeCount.size());

		code += "/* Cascaded Shadow Map resolution. */" "\n\n";

		code += "float shadowFactor = 1.0;" "\n\n";

		/* Compute view-space depth for cascade selection.
		 * NOTE: read straight off the interpolated view-space position. Re-deriving it from
		 * the world-space position would need the view matrix, which no fragment shader on
		 * the main render target can reach. */
		code += "/* Compute view-space depth for cascade selection. */" "\n"
			"const float viewDepth = abs(";
		code += fragmentPositionViewSpace;
		code += ".z);" "\n\n";

		/* Determine which cascade to use based on view-space depth. */
		code +=
			"/* Select the appropriate cascade based on depth. */" "\n"
			"int cascadeIndex = 0;" "\n"
			"const int numCascades = int(";
		code += cascadeCount;
		code +=
			");" "\n"
			"for ( int i = 0; i < numCascades; i++ )" "\n"
			"{" "\n"
			"	if ( viewDepth < ";
		code += splitDistances;
		code +=
			"[i] )" "\n"
			"	{" "\n"
			"		cascadeIndex = i;" "\n"
			"		break;" "\n"
			"	}" "\n"
			"	cascadeIndex = i;" "\n"
			"}" "\n\n";

		/* Transform fragment position to light space using the selected cascade matrix. */
		code +=
			"/* Transform to the selected cascade's light space. */" "\n"
			"const mat4 cascadeMatrix = ";
		code += cascadeMatrices;
		code +=
			"[cascadeIndex];" "\n"
			"vec4 posLightSpace = cascadeMatrix * vec4(";
		code += fragmentPositionWorldSpace;
		code +=
			", 1.0);" "\n"
			"vec3 projCoords = posLightSpace.xyz / posLightSpace.w;" "\n"
			"/* NOTE: Only X and Y need [-1,1] to [0,1] conversion for UV coordinates. */" "\n"
			"/* Z is already in [0,1] range from Vulkan orthographic projection. */" "\n"
			"projCoords.xy = projCoords.xy * 0.5 + 0.5;" "\n\n";

		/* ⚠️⚠️ PER-CASCADE DEPTH BIAS, and the reason it has to be derived rather than uploaded.
		 * `shadowBias` was uploaded by DirectionalLight and SpotLight, declared in their uniform
		 * blocks, and read by NOBODY on either 2D path — its only generated consumers were the
		 * point-light cubemap ones. Tuning it did nothing at all, silently.
		 * The rasterizer bias on the cast pass cannot replace it here: it is per-PIPELINE, and all
		 * cascades go through that one pipeline via multiview, so it cannot vary per cascade — while
		 * the cascades differ in texel size by more than an order of magnitude.
		 * The scale is read out of the cascade matrix instead of being uploaded, which keeps the
		 * uniform block untouched — that layout is described by hand in three separate places and a
		 * silent truncation has already shipped from editing one of them. After the bounding-sphere
		 * fit the orthographic X scale is exactly 1/radius and the light view is a rotation plus a
		 * translation, so `length(row0)` recovers 1/radius, and the depth range is 3 * radius by
		 * construction. `shadowBias` therefore means WORLD UNITS — metres — which is a quantity worth
		 * exposing, unlike an opaque NDC epsilon.
		 * @todo Normal-offset shadows would beat a pure depth bias on grazing surfaces, but they need
		 * the world-space normal, which no fragment shader here interpolates yet: the light pass only
		 * synthesizes the VIEW-space normal, and later than this block. That is its own change. */
		code +=
			"/* Per-cascade depth bias, expressed in WORLD units (see the note in the generator). */" "\n"
			"const float cascadeInverseRadius = length(vec3(cascadeMatrix[0][0], cascadeMatrix[1][0], cascadeMatrix[2][0]));" "\n"
			"projCoords.z -= ";
		code += shadowBias;
		code += " * cascadeInverseRadius / 3.0;" "\n\n";

		/* Skip shadow calculation if outside the shadow map's valid depth range. */
		code +=
			"if ( projCoords.z >= 0.0 && projCoords.z <= 1.0 )" "\n"
			"{" "\n";

		if ( m_PCFEnabled )
		{
			code += "	";
			code += GLSL::ConstInteger;
			code += " offset = ";
			code += std::to_string(m_PCFSample);
			code += ";" "\n\n";

			/* NOTE: Reset shadowFactor to 0.0 before accumulating PCF samples.
			 * The initial value of 1.0 is only for the non-shadow case (outside depth range). */
			code += "	shadowFactor = 0.0;" "\n\n";

			/* PCF sampling with sampler2DArrayShadow. */
			code += "	for ( ";
			code += GLSL::Integer;
			code += " idy = -offset; idy <= offset; idy++ )" "\n"
				"	{" "\n"
				"		for ( ";
			code += GLSL::Integer;
			code += " idx = -offset; idx <= offset; idx++ )" "\n"
				"		{" "\n"
				"			vec2 texelSize = 1.0 / vec2(textureSize(";
			code += shadowMapArray;
			code +=
				", 0).xy);" "\n"
				"			vec2 offsetUV = projCoords.xy + vec2(float(idx), float(idy)) * texelSize;" "\n"
				"			shadowFactor += texture(";
			code += shadowMapArray;
			code +=
				", vec4(offsetUV, float(cascadeIndex), projCoords.z));" "\n"
				"		}" "\n"
				"	}" "\n\n"

				"	shadowFactor /= pow(float(offset) * 2.0 + 1.0, 2);" "\n";
		}
		else
		{
			/* Single sample with sampler2DArrayShadow.
			 * The fourth component is the reference depth for comparison. */
			code += "	shadowFactor = texture(";
			code += shadowMapArray;
			code += ", vec4(projCoords.xy, float(cascadeIndex), projCoords.z));" "\n";
		}

		code += "}" "\n\n";

		if ( m_discardUnlitFragment )
		{
			code += "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code;
	}
}
