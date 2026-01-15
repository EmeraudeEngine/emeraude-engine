/*
 * src/Saphir/LightGenerator.PBR.cpp
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
#include "Declaration/OutputBlock.hpp"
#include "Declaration/Function.hpp"
#include "Generator/Abstract.hpp"
#include "Code.hpp"

namespace EmEn::Saphir
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Graphics;
	using namespace Saphir::Keys;
	using namespace Vulkan;

	void
	LightGenerator::generatePBRBRDFFunctions (FragmentShader & fragmentShader) const noexcept
	{
		/* Fresnel-Schlick approximation. */
		{
			Declaration::Function fresnelSchlick{"fresnelSchlick", GLSL::FloatVector3};
			fresnelSchlick.addInParameter(GLSL::Float, "cosTheta");
			fresnelSchlick.addInParameter(GLSL::FloatVector3, "F0");
			Code{fresnelSchlick, Location::Output} << "return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);";

			fragmentShader.declare(fresnelSchlick);
		}

		/* Normal Distribution Function (GGX/Trowbridge-Reitz). */
		{
			Declaration::Function distributionGGX{"distributionGGX", GLSL::Float};
			distributionGGX.addInParameter(GLSL::FloatVector3, "N");
			distributionGGX.addInParameter(GLSL::FloatVector3, "H");
			distributionGGX.addInParameter(GLSL::Float, "roughness");
			Code{distributionGGX, Location::Output} <<
				"float a = roughness * roughness;" << Line::End <<
				"float a2 = a * a;" << Line::End <<
				"float NdotH = max(dot(N, H), 0.0);" << Line::End <<
				"float NdotH2 = NdotH * NdotH;" << Line::End <<
				"float denom = (NdotH2 * (a2 - 1.0) + 1.0);" << Line::End <<
				"return a2 / (3.14159265 * denom * denom);";

			fragmentShader.declare(distributionGGX);
		}

		/* Geometry function (Schlick-GGX). */
		{
			Declaration::Function geometrySchlickGGX{"geometrySchlickGGX", GLSL::Float};
			geometrySchlickGGX.addInParameter(GLSL::Float, "NdotV");
			geometrySchlickGGX.addInParameter(GLSL::Float, "roughness");
			Code{geometrySchlickGGX, Location::Output} <<
				"float r = roughness + 1.0;" << Line::End <<
				"float k = (r * r) / 8.0;" << Line::End <<
				"return NdotV / (NdotV * (1.0 - k) + k);";

			fragmentShader.declare(geometrySchlickGGX);
		}

		/* Geometry function (Smith's method). */
		{
			Declaration::Function geometrySmith{"geometrySmith", GLSL::Float};
			geometrySmith.addInParameter(GLSL::FloatVector3, "N");
			geometrySmith.addInParameter(GLSL::FloatVector3, "V");
			geometrySmith.addInParameter(GLSL::FloatVector3, "L");
			geometrySmith.addInParameter(GLSL::Float, "roughness");
			Code{geometrySmith, Location::Output} <<
				"float NdotV = max(dot(N, V), 0.0);" << Line::End <<
				"float NdotL = max(dot(N, L), 0.0);" << Line::End <<
				"return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);";

			fragmentShader.declare(geometrySmith);
		}
	}

	bool
	LightGenerator::generatePBRVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, LightType lightType, bool enableShadowMap) const noexcept
	{
		Declaration::OutputBlock lightBlock{LightBlock, generator.getNextShaderVariableLocation(lightType == LightType::Spot ? 2 : 1), ShaderVariable::Light};

		if ( lightType == LightType::Directional )
		{
			vertexShader.addComment("Compute the light direction in view space (Normalized vector).");

			lightBlock.addMember(Declaration::VariableType::FloatVector3, RayDirectionViewSpace, GLSL::Smooth);

			Code{vertexShader, Location::Output} << LightGenerator::variable(RayDirectionViewSpace) << " = normalize((" << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << this->lightDirectionWorldSpace() << ").xyz);";
		}
		else
		{
			vertexShader.addComment("Compute the light direction in view space (Distance vector).");

			lightBlock.addMember(Declaration::VariableType::FloatVector3, Distance, GLSL::Smooth);

			if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionViewSpace, VariableScope::Both) )
			{
				return false;
			}

			Code{vertexShader} << "const vec4 " << LightPositionViewSpace << " = " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << this->lightPositionWorldSpace() << ';';

			Code{vertexShader, Location::Output} << LightGenerator::variable(Distance) << " = " << ShaderVariable::PositionViewSpace << ".xyz - " << LightPositionViewSpace << ".xyz;";
		}

		if ( lightType == LightType::Spot )
		{
			lightBlock.addMember(Declaration::VariableType::FloatVector3, SpotLightDirectionViewSpace, GLSL::Smooth);

			Code{vertexShader, Location::Output} << LightGenerator::variable(SpotLightDirectionViewSpace) << " = normalize((" << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << this->lightDirectionWorldSpace() << ").xyz);";
		}

		/* NOTE: For all light types - PBR needs normal in view space. */
		if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalViewSpace, VariableScope::ToNextStage) )
		{
			return false;
		}

		/* NOTE: If normal mapping is used, we need the TBN matrix to transform the
		 * tangent-space normal to view space in the fragment shader. */
		if ( m_useNormalMapping )
		{
			if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::ViewTBNMatrix, VariableScope::ToNextStage) )
			{
				return false;
			}
		}

		/* NOTE: PBR always needs position for view direction calculation. */
		if ( lightType == LightType::Directional )
		{
			if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionViewSpace, VariableScope::ToNextStage) )
			{
				return false;
			}
		}

		/* NOTE: Handle shadow mapping if enabled. */
		if ( enableShadowMap )
		{
			vertexShader.addComment("Compute the shadow map prerequisites for next stage.");

			if ( !this->generateVertexShaderShadowMapCode(generator, vertexShader, lightType == LightType::Point) )
			{
				return false;
			}
		}

		return vertexShader.declare(lightBlock);
	}

	bool
	LightGenerator::generatePBRFragmentShader (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader, LightType lightType, bool enableShadowMap) const noexcept
	{
		/* Generate the PBR BRDF helper functions. */
		this->generatePBRBRDFFunctions(fragmentShader);

		std::string rayDirectionViewSpace;

		if ( lightType != LightType::Directional )
		{
			fragmentShader.addComment("Compute the ray direction in view space.");

			Code{fragmentShader} << "const vec3 " << RayDirectionViewSpace << " = normalize(" << LightGenerator::variable(Distance) << ");";

			rayDirectionViewSpace = RayDirectionViewSpace;
		}
		else
		{
			rayDirectionViewSpace = LightGenerator::variable(RayDirectionViewSpace);
		}

		Code{fragmentShader} << "float " << LightFactor << " = 1.0;" << Line::End;

		/* NOTE: Check the radius influence. */
		if ( lightType != LightType::Directional )
		{
			fragmentShader.addComment("Compute the radius influence over the light factor [Point+Spot].");

			Code{fragmentShader} <<
				"if ( " << this->lightRadius() << " > 0.0 ) " << Line::End <<
				'{' << Line::End <<
				"	const vec3 DR = abs(" << LightGenerator::variable(Distance) << ") / " << this->lightRadius() << ';' << Line::Blank <<

				"	" << LightFactor << " *= max(1.0 - dot(DR, DR), 0.0);" << Line::End <<
				'}' << Line::End;

			if ( m_discardUnlitFragment )
			{
				Code{fragmentShader} << "if ( " << LightFactor << " <= 0.0 ) { discard; }" << Line::End;
			}
		}

		/* NOTE: Check the spot-light influence. */
		if ( lightType == LightType::Spot )
		{
			fragmentShader.addComment("Compute the cone influence over the light factor [Spot].");

			const auto innerCosAngle = this->lightInnerCosAngle();
			const auto outerCosAngle = this->lightOuterCosAngle();

			Code{fragmentShader} <<
				"if ( " << LightFactor << " > 0.0 )" << Line::End <<
				'{' << Line::End <<
				"	const float theta = dot(" << rayDirectionViewSpace << ", " << LightGenerator::variable(SpotLightDirectionViewSpace) << ");" << Line::End <<
				"	const float epsilon = " << innerCosAngle << " - " << outerCosAngle << ";" << Line::End <<
				"	const float spotFactor = clamp((theta - " << outerCosAngle << ") / epsilon, 0.0, 1.0);" << Line::End <<
				"	" << LightFactor << " *= spotFactor;" << Line::End <<
				'}' << Line::End;

			if ( m_discardUnlitFragment )
			{
				Code{fragmentShader} << "if ( " << LightFactor << " <= 0.0 ) { discard; }" << Line::End;
			}
		}

		/* NOTE: Check the shadow map influence. */
		if ( enableShadowMap )
		{
			fragmentShader.addComment("Compute the shadow influence over the light factor.");

			switch ( lightType )
			{
				case LightType::Directional :
					Code{fragmentShader} << this->generate2DShadowMapCode(Uniform::ShadowMapSampler, LightGenerator::variable(ShaderBlock::Component::PositionLightSpace), DepthTextureFunction::TextureGather) << Line::Blank;
					break;

				case LightType::Point :
					/* TODO: Implement point light shadow mapping for PBR. */
					break;

				case LightType::Spot :
					Code{fragmentShader} << this->generate2DShadowMapCode(Uniform::ShadowMapSampler, LightGenerator::variable(ShaderBlock::Component::PositionLightSpace), DepthTextureFunction::TextureProj) << Line::End;
					break;
			}

			Code{fragmentShader} << LightFactor << " *= shadowFactor;" << Line::End;
		}

		/* PBR Cook-Torrance BRDF computation. */
		fragmentShader.addComment("PBR Cook-Torrance BRDF computation.");

		/* Get the surface properties. */
		const std::string albedo = m_surfaceAlbedo.empty() ? "vec3(1.0)" : m_surfaceAlbedo + ".rgb";
		const std::string roughness = m_surfaceRoughness.empty() ? "0.5" : m_surfaceRoughness;
		const std::string metalness = m_surfaceMetalness.empty() ? "0.0" : m_surfaceMetalness;

		/* Compute the view direction and normal.
		 * If normal mapping is used, transform the tangent-space normal to view space.
		 * transpose(ViewTBNMatrix) transforms from tangent space to view space. */
		if ( m_useNormalMapping && !m_surfaceNormalVector.empty() )
		{
			Code{fragmentShader} <<
				"const vec3 N = normalize(transpose(" << ShaderVariable::ViewTBNMatrix << ") * " << m_surfaceNormalVector << ");" << Line::End <<
				"const vec3 V = normalize(-" << ShaderVariable::PositionViewSpace << ".xyz);" << Line::End <<
				"const vec3 L = -" << rayDirectionViewSpace << ";" << Line::End <<
				"const vec3 H = normalize(V + L);" << Line::Blank;
		}
		else
		{
			Code{fragmentShader} <<
				"const vec3 N = normalize(" << ShaderVariable::NormalViewSpace << ");" << Line::End <<
				"const vec3 V = normalize(-" << ShaderVariable::PositionViewSpace << ".xyz);" << Line::End <<
				"const vec3 L = -" << rayDirectionViewSpace << ";" << Line::End <<
				"const vec3 H = normalize(V + L);" << Line::Blank;
		}

		/* Compute F0 (reflectance at normal incidence). */
		Code{fragmentShader} <<
			"/* F0: reflectance at normal incidence. Dielectrics use 0.04, metals use albedo. */" << Line::End <<
			"const vec3 F0 = mix(vec3(0.04), " << albedo << ", " << metalness << ");" << Line::Blank;

		/* Compute the Cook-Torrance BRDF components. */
		Code{fragmentShader} <<
			"/* Cook-Torrance BRDF components. */" << Line::End <<
			"const float NDF = distributionGGX(N, H, " << roughness << ");" << Line::End <<
			"const float G = geometrySmith(N, V, L, " << roughness << ");" << Line::End <<
			"const vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);" << Line::Blank;

		/* Compute the specular contribution. */
		Code{fragmentShader} <<
			"/* Specular contribution. */" << Line::End <<
			"const vec3 numerator = NDF * G * F;" << Line::End <<
			"const float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;" << Line::End <<
			"const vec3 specular = numerator / denominator;" << Line::Blank;

		/* Compute the diffuse contribution with energy conservation. */
		Code{fragmentShader} <<
			"/* Energy conservation: kD + kS = 1.0, metals have no diffuse. */" << Line::End <<
			"const vec3 kS = F;" << Line::End <<
			"const vec3 kD = (vec3(1.0) - kS) * (1.0 - " << metalness << ");" << Line::Blank;

		/* Compute the final lighting. */
		Code{fragmentShader} <<
			"/* Final NdotL factor. */" << Line::End <<
			"const float NdotL = max(dot(N, L), 0.0);" << Line::Blank;

		/* Compute radiance and final output. */
		Code{fragmentShader} <<
			"/* Light radiance. */" << Line::End <<
			"const vec3 radiance = " << this->lightColor() << ".rgb * " << this->lightIntensity() << " * " << LightFactor << ";" << Line::Blank;

		/* Fragment color output. */
		if ( m_fragmentColor.empty() )
		{
			return false;
		}

		if ( m_useOpacity )
		{
			Code{fragmentShader, Location::Top} << "vec4 " << m_fragmentColor << " = vec4(0.0, 0.0, 0.0, " << m_surfaceOpacityAmount << ");";
		}
		else
		{
			Code{fragmentShader, Location::Top} << "vec4 " << m_fragmentColor << " = vec4(0.0, 0.0, 0.0, 1.0);";
		}

		if ( m_useStaticLighting )
		{
			this->generateAmbientFragmentShader(fragmentShader);
		}

		/* Apply diffuse and specular contributions. */
		Code{fragmentShader} <<
			"/* Apply PBR lighting. */" << Line::End <<
			m_fragmentColor << ".rgb += (kD * " << albedo << " / 3.14159265 + specular) * radiance * NdotL;";

		/* NOTE: IBL (reflection/refraction from environment cubemap) is handled ONLY in the ambient pass.
		 * This prevents IBL accumulation when multiple lights are present.
		 * Direct light passes only compute Cook-Torrance BRDF for specular highlights from each light source.
		 * The IBLIntensity parameter in the ambient pass controls the cubemap contribution. */

		return true;
	}
}
