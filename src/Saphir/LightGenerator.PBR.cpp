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
#include "Declaration/Sampler.hpp"
#include "Declaration/SpecializationConstant.hpp"
#include "Generator/Abstract.hpp"
#include "Code.hpp"
#include "Graphics/BindlessTextureManager.hpp"

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

		/* Charlie Normal Distribution Function (for sheen/cloth). */
		if ( m_useSheen )
		{
			Declaration::Function distributionCharlie{"distributionCharlie", GLSL::Float};
			distributionCharlie.addInParameter(GLSL::FloatVector3, "N");
			distributionCharlie.addInParameter(GLSL::FloatVector3, "H");
			distributionCharlie.addInParameter(GLSL::Float, "roughness");
			Code{distributionCharlie, Location::Output} <<
				"float NdotH = max(dot(N, H), 0.0);" << Line::End <<
				"float invAlpha = 1.0 / roughness;" << Line::End <<
				"float cos2h = NdotH * NdotH;" << Line::End <<
				"float sin2h = max(1.0 - cos2h, 0.0078125);" << Line::End <<
				"return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * 3.14159265);";

			fragmentShader.declare(distributionCharlie);
		}

		/* Ashikhmin visibility function (for sheen/cloth). */
		if ( m_useSheen )
		{
			Declaration::Function visibilityAshikhmin{"visibilityAshikhmin", GLSL::Float};
			visibilityAshikhmin.addInParameter(GLSL::Float, "NdotV");
			visibilityAshikhmin.addInParameter(GLSL::Float, "NdotL");
			Code{visibilityAshikhmin, Location::Output} <<
				"return 1.0 / (4.0 * (NdotL + NdotV - NdotL * NdotV));";

			fragmentShader.declare(visibilityAshikhmin);
		}

		/* Anisotropic GGX Normal Distribution Function. */
		if ( m_useAnisotropy )
		{
			Declaration::Function distributionGGXAniso{"distributionGGXAniso", GLSL::Float};
			distributionGGXAniso.addInParameter(GLSL::FloatVector3, "N");
			distributionGGXAniso.addInParameter(GLSL::FloatVector3, "H");
			distributionGGXAniso.addInParameter(GLSL::FloatVector3, "T");
			distributionGGXAniso.addInParameter(GLSL::FloatVector3, "B");
			distributionGGXAniso.addInParameter(GLSL::Float, "at");
			distributionGGXAniso.addInParameter(GLSL::Float, "ab");
			Code{distributionGGXAniso, Location::Output} <<
				"float TdotH = dot(T, H);" << Line::End <<
				"float BdotH = dot(B, H);" << Line::End <<
				"float NdotH = max(dot(N, H), 0.0);" << Line::End <<
				"float f = (TdotH * TdotH) / (at * at) + (BdotH * BdotH) / (ab * ab) + NdotH * NdotH;" << Line::End <<
				"return 1.0 / (3.14159265 * at * ab * f * f);";

			fragmentShader.declare(distributionGGXAniso);
		}

		/* Anisotropic Smith-GGX height-correlated visibility function.
		 * Includes the BRDF denominator 1/(4*NdotV*NdotL).
		 * Reference: Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs". */
		if ( m_useAnisotropy )
		{
			Declaration::Function visibilityAniso{"visibilityAniso", GLSL::Float};
			visibilityAniso.addInParameter(GLSL::FloatVector3, "T");
			visibilityAniso.addInParameter(GLSL::FloatVector3, "B");
			visibilityAniso.addInParameter(GLSL::FloatVector3, "N");
			visibilityAniso.addInParameter(GLSL::FloatVector3, "V");
			visibilityAniso.addInParameter(GLSL::FloatVector3, "L");
			visibilityAniso.addInParameter(GLSL::Float, "at");
			visibilityAniso.addInParameter(GLSL::Float, "ab");
			Code{visibilityAniso, Location::Output} <<
				"float TdotV = dot(T, V);" << Line::End <<
				"float BdotV = dot(B, V);" << Line::End <<
				"float NdotV = max(dot(N, V), 0.001);" << Line::End <<
				"float TdotL = dot(T, L);" << Line::End <<
				"float BdotL = dot(B, L);" << Line::End <<
				"float NdotL = max(dot(N, L), 0.001);" << Line::End <<
				"float lambdaV = NdotL * sqrt(at * at * TdotV * TdotV + ab * ab * BdotV * BdotV + NdotV * NdotV);" << Line::End <<
				"float lambdaL = NdotV * sqrt(at * at * TdotL * TdotL + ab * ab * BdotL * BdotL + NdotL * NdotL);" << Line::End <<
				"return 0.5 / (lambdaV + lambdaL + 0.0001);";

			fragmentShader.declare(visibilityAniso);
		}

		/* Thin-film iridescence (Airy equation for 3 RGB wavelengths). */
		if ( m_useIridescence )
		{
			Declaration::Function evalIridescence{"evalIridescence", GLSL::FloatVector3};
			evalIridescence.addInParameter(GLSL::Float, "outsideIOR");
			evalIridescence.addInParameter(GLSL::Float, "iridescenceIOR");
			evalIridescence.addInParameter(GLSL::Float, "cosTheta1");
			evalIridescence.addInParameter(GLSL::Float, "thickness");
			evalIridescence.addInParameter(GLSL::FloatVector3, "baseF0");
			Code{evalIridescence, Location::Output} <<
				"float eta = outsideIOR / iridescenceIOR;" << Line::End <<
				"float sinTheta2Sq = eta * eta * (1.0 - cosTheta1 * cosTheta1);" << Line::End <<
				"float cosTheta2 = sqrt(max(1.0 - sinTheta2Sq, 0.0));" << Line::End <<
				"float R0_12 = pow((outsideIOR - iridescenceIOR) / (outsideIOR + iridescenceIOR), 2.0);" << Line::End <<
				"float R12 = R0_12 + (1.0 - R0_12) * pow(1.0 - cosTheta1, 5.0);" << Line::End <<
				"float OPD = 2.0 * iridescenceIOR * thickness * cosTheta2;" << Line::End <<
				"vec3 phi = 2.0 * 3.14159265 * OPD / vec3(630.0, 530.0, 460.0);" << Line::End <<
				"vec3 R23 = baseF0;" << Line::End <<
				"vec3 sqrtR12 = vec3(sqrt(R12));" << Line::End <<
				"vec3 sqrtR23 = sqrt(R23);" << Line::End <<
				"vec3 cosPhi = cos(phi);" << Line::End <<
				"vec3 num = vec3(R12) + R23 + 2.0 * sqrtR12 * sqrtR23 * cosPhi;" << Line::End <<
				"vec3 den = vec3(1.0) + vec3(R12) * R23 + 2.0 * sqrtR12 * sqrtR23 * cosPhi;" << Line::End <<
				"return clamp(num / den, vec3(0.0), vec3(1.0));";

			fragmentShader.declare(evalIridescence);
		}
	}

	bool
	LightGenerator::generatePBRVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, LightType lightType, bool enableShadowMap, bool enableColorProjection) const noexcept
	{
		Declaration::OutputBlock lightBlock{LightBlock, generator.getNextShaderVariableLocation(lightType == LightType::Spot ? 2 : 1), ShaderVariable::Light};

		/* NOTE: In cubemap mode, the view matrix comes from the UBO indexed by gl_ViewIndex,
		 * not from the push constant. */
		const auto viewMatrixSource = vertexShader.isCubemapModeEnabled() ?
			ViewUB(UniformBlock::Component::ViewMatrix, true) :
			MatrixPC(PushConstant::Component::ViewMatrix);

		if ( lightType == LightType::Directional )
		{
			vertexShader.addComment("Compute the light direction in view space (Normalized vector).");

			lightBlock.addMember(Declaration::VariableType::FloatVector3, RayDirectionViewSpace, GLSL::Smooth);

			Code{vertexShader, Location::Output} << LightGenerator::variable(RayDirectionViewSpace) << " = normalize((" << viewMatrixSource << " * " << this->lightDirectionWorldSpace() << ").xyz);";
		}
		else
		{
			vertexShader.addComment("Compute the light direction in view space (Distance vector).");

			lightBlock.addMember(Declaration::VariableType::FloatVector3, Distance, GLSL::Smooth);

			if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionViewSpace, VariableScope::Both) )
			{
				return false;
			}

			Code{vertexShader} << "const vec4 " << LightPositionViewSpace << " = " << viewMatrixSource << " * " << this->lightPositionWorldSpace() << ';';

			Code{vertexShader, Location::Output} << LightGenerator::variable(Distance) << " = " << ShaderVariable::PositionViewSpace << ".xyz - " << LightPositionViewSpace << ".xyz;";
		}

		if ( lightType == LightType::Spot )
		{
			lightBlock.addMember(Declaration::VariableType::FloatVector3, SpotLightDirectionViewSpace, GLSL::Smooth);

			Code{vertexShader, Location::Output} << LightGenerator::variable(SpotLightDirectionViewSpace) << " = normalize((" << viewMatrixSource << " * " << this->lightDirectionWorldSpace() << ").xyz);";
		}

		/* NOTE: For all light types - PBR needs normal in view space. */
		if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalViewSpace, VariableScope::ToNextStage) )
		{
			return false;
		}

		/* NOTE: If normal mapping is used, we need the TBN matrix
		 * to transform tangent-space normals to view space in the fragment shader.
		 * NOTE: Clear coat normal WITHOUT base normal mapping computes T/B from N
		 * in the fragment shader, just like anisotropy does. */
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

		/* NOTE: Projection coordinates are needed for shadow mapping AND/OR color projection.
		 * The UBO contains viewProjectionMatrix when shadow mapping or color projection is enabled.
		 * Point lights use cubemap direction for 3D lookup (shadow and color projection).
		 * CSM mode requires PositionWorldSpace (shadow-only, CSM-specific). */
		if ( enableShadowMap || enableColorProjection )
		{
			const bool useCSM = renderPassUsesCSM(m_renderPassType);

			vertexShader.addComment("Compute the projection coordinates for next stage.");

			if ( enableShadowMap && useCSM )
			{
				/* NOTE: CSM computes light-space position in the fragment shader.
				 * We only need to pass the world-space position. */
				if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionWorldSpace, VariableScope::ToNextStage) )
				{
					return false;
				}
			}

			if ( !useCSM )
			{
				/* NOTE: Point lights use cubemap shadow maps (true = cubemap mode).
				 * Other light types use 2D shadow maps with viewProjectionMatrix. */
				if ( !this->generateVertexShaderShadowMapCode(generator, vertexShader, lightType == LightType::Point) )
				{
					return false;
				}
			}
		}

		return vertexShader.declare(lightBlock);
	}

	bool
	LightGenerator::generatePBRFragmentShader (const Generator::Abstract & generator, FragmentShader & fragmentShader, LightType lightType, [[maybe_unused]] bool enableShadowMap, [[maybe_unused]] bool enableColorProjection) const noexcept
	{
		const auto lightSetIndex = generator.shaderProgram()->setIndex(SetType::PerLight);

		const bool useCSM = renderPassUsesCSM(m_renderPassType);

		/* NOTE: Shadow sampler is declared when shadow mapping is enabled.
		 * Point lights use cubemap shadow maps (samplerCube) for omnidirectional shadows.
		 * Directional lights with CSM use 2D array shadow maps (sampler2DArrayShadow).
		 * Other light types use 2D shadow maps (sampler2DShadow) with hardware comparison. */
		if ( enableShadowMap )
		{
			if ( lightType == LightType::Point )
			{
				/* NOTE: Point lights use a cubemap sampler for omnidirectional shadow lookup. */
				if ( !fragmentShader.declare(Declaration::Sampler{lightSetIndex, 1, GLSL::SamplerCube, Uniform::ShadowMapSampler}) )
				{
					return false;
				}
			}
			else if ( useCSM )
			{
				/* NOTE: CSM uses a 2D array shadow sampler for cascade layers. */
				if ( !fragmentShader.declare(Declaration::Sampler{lightSetIndex, 1, GLSL::Sampler2DArrayShadow, Uniform::ShadowMapSampler}) )
				{
					return false;
				}
			}
			else
			{
				if ( !fragmentShader.declare(Declaration::Sampler{lightSetIndex, 1, GLSL::Sampler2DShadow, Uniform::ShadowMapSampler}) )
				{
					return false;
				}
			}
		}

		/* NOTE: Color projection uses bindless textures. When enabled, declare the appropriate
		 * bindless sampler array and enable the nonuniform qualifier extension. */
		if ( enableColorProjection )
		{
			const auto bindlessSetIndex = generator.shaderProgram()->setIndex(SetType::PerBindless);

			fragmentShader.setExtensionBehavior(GLSL::Extension::NonUniformQualifier, GLSL::Extension::Require);

			if ( lightType == LightType::Point )
			{
				if ( !fragmentShader.declare(Declaration::Sampler{bindlessSetIndex, BindlessTextureManager::TextureCubeBinding, GLSL::SamplerCube, Bindless::TexturesCube, Declaration::Sampler::UnboundedArray}) )
				{
					return false;
				}

				if ( !fragmentShader.declare(Declaration::Sampler{bindlessSetIndex, BindlessTextureManager::TextureCubeArrayBinding, GLSL::SamplerCubeArray, Bindless::TexturesCubeArray, Declaration::Sampler::UnboundedArray}) )
				{
					return false;
				}
			}
			else
			{
				if ( !fragmentShader.declare(Declaration::Sampler{bindlessSetIndex, BindlessTextureManager::Texture2DBinding, GLSL::Sampler2D, Bindless::Textures2D, Declaration::Sampler::UnboundedArray}) )
				{
					return false;
				}
			}
		}

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

		/* NOTE: Shadow map influence is computed when shadow mapping is enabled. */
		if ( enableShadowMap )
		{
			fragmentShader.addComment("Compute the shadow influence over the light factor.");

			switch ( lightType )
			{
				case LightType::Directional :
					if ( useCSM )
					{
						/* NOTE: CSM requires world-space position, view matrix for depth calculation,
						 * and cascade matrices/split distances from the light UBO. */
						Code{fragmentShader} << this->generateCSMShadowMapCode(
							Uniform::ShadowMapSampler,
							std::string(ShaderVariable::PositionWorldSpace) + ".xyz",
							ViewUB(UniformBlock::Component::ViewMatrix, false),
							LightUB(UniformBlock::Component::CascadeViewProjectionMatrices),
							LightUB(UniformBlock::Component::CascadeSplitDistances),
							LightUB(UniformBlock::Component::CascadeCount)
						) << Line::Blank;
					}
					else if ( m_PCFEnabled )
					{
						Code{fragmentShader} << this->generate2DShadowMapPCFCode(Uniform::ShadowMapSampler, ShaderVariable::PositionLightSpace) << Line::Blank;
					}
					else
					{
						Code{fragmentShader} << this->generate2DShadowMapCode(Uniform::ShadowMapSampler, ShaderVariable::PositionLightSpace) << Line::Blank;
					}
					break;

				case LightType::Point :
					/* NOTE: Point lights use cubemap shadow maps. The direction from fragment to light
					 * is used as the lookup vector. Depth is linearized using the light radius as far plane.
					 * nearFar.y = radius is used to convert stored depth to world-space distance. */
					if ( m_PCFEnabled )
					{
						Code{fragmentShader} << this->generate3DShadowMapPCFCode(Uniform::ShadowMapSampler, "DirectionWorldSpace", "vec2(0.1, " + this->lightRadius() + ")") << Line::End;
					}
					else
					{
						Code{fragmentShader} << this->generate3DShadowMapCode(Uniform::ShadowMapSampler, "DirectionWorldSpace", "vec2(0.1, " + this->lightRadius() + ")") << Line::End;
					}
					break;

				case LightType::Spot :
					if ( m_PCFEnabled )
					{
						Code{fragmentShader} << this->generate2DShadowMapPCFCode(Uniform::ShadowMapSampler, ShaderVariable::PositionLightSpace) << Line::End;
					}
					else
					{
						Code{fragmentShader} << this->generate2DShadowMapCode(Uniform::ShadowMapSampler, ShaderVariable::PositionLightSpace) << Line::End;
					}
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
		if ( m_useMaterialIOR )
		{
			if ( m_useKHRSpecular )
			{
				Code{fragmentShader} <<
					"/* F0: reflectance at normal incidence from material IOR + KHR_materials_specular. */" << Line::End <<
					"const float materialIOR = " << m_surfaceMaterialIOR << ";" << Line::End <<
					"const float dielectricF0 = pow((materialIOR - 1.0) / (materialIOR + 1.0), 2.0);" << Line::End <<
					"const vec3 F0 = mix(min(vec3(dielectricF0) * " << m_surfaceKHRSpecularColor << ".rgb * " << m_surfaceKHRSpecularFactor << ", vec3(1.0)), " << albedo << ", " << metalness << ");" << Line::Blank;
			}
			else
			{
				Code{fragmentShader} <<
					"/* F0: reflectance at normal incidence from material IOR (KHR_materials_ior). */" << Line::End <<
					"const float materialIOR = " << m_surfaceMaterialIOR << ";" << Line::End <<
					"const float dielectricF0 = pow((materialIOR - 1.0) / (materialIOR + 1.0), 2.0);" << Line::End <<
					"const vec3 F0 = mix(vec3(dielectricF0), " << albedo << ", " << metalness << ");" << Line::Blank;
			}
		}
		else
		{
			Code{fragmentShader} <<
				"/* F0: reflectance at normal incidence. Dielectrics use 0.04, metals use albedo. */" << Line::End <<
				"const vec3 F0 = mix(vec3(0.04), " << albedo << ", " << metalness << ");" << Line::Blank;
		}

		/* Declare iridescence local variables if enabled. */
		if ( m_useIridescence )
		{
			Code{fragmentShader} <<
				"/* Iridescence: thin-film interference parameters. */" << Line::End <<
				"const float iridescenceFactor = " << m_surfaceIridescenceFactor << ";" << Line::End <<
				"const float iridescenceIOR = " << m_surfaceIridescenceIOR << ";" << Line::End <<
				"const float iridescenceThicknessMin = " << m_surfaceIridescenceThicknessMin << ";" << Line::End <<
				"const float iridescenceThicknessMax = " << m_surfaceIridescenceThicknessMax << ";" << Line::End <<
				"const float iridescenceThickness = mix(iridescenceThicknessMin, iridescenceThicknessMax, 0.5);" << Line::Blank;
		}

		if ( m_useAnisotropy )
		{
			/* Anisotropic BRDF: build tangent frame, apply rotation, compute directional roughness. */
			/* Derive T/B from the shading normal N.
			 * This gives a smooth tangent frame everywhere, avoiding per-vertex
			 * tangent interpolation artifacts (UV seam discontinuities). */
			if ( !m_surfaceAnisotropyDirection.empty() )
			{
				/* Per-pixel direction from KHR_materials_anisotropy texture (RG = direction, B = strength). */
				Code{fragmentShader} <<
					"/* Anisotropy: build tangent frame from N with per-pixel direction (KHR_materials_anisotropy). */" << Line::End <<
					"const float anisoValue = " << m_surfaceAnisotropy << ";" << Line::End <<
					"const vec2 anisoDir = " << m_surfaceAnisotropyDirection << ";" << Line::End <<
					"const float texRotation = atan(anisoDir.y, anisoDir.x);" << Line::End <<
					"const float anisoRotation = texRotation + " << m_surfaceAnisotropyRotation << " * 2.0 * 3.14159265;" << Line::End <<
					"const vec3 T0 = abs(N.y) < 0.999 ? normalize(cross(N, vec3(0.0, 1.0, 0.0))) : normalize(cross(N, vec3(1.0, 0.0, 0.0)));" << Line::End <<
					"const vec3 B0 = cross(N, T0);" << Line::End <<
					"const float cosR = cos(anisoRotation);" << Line::End <<
					"const float sinR = sin(anisoRotation);" << Line::End <<
					"const vec3 T = T0 * cosR + B0 * sinR;" << Line::End <<
					"const vec3 B = -T0 * sinR + B0 * cosR;" << Line::End <<
					"const float alphaRoughness = " << roughness << " * " << roughness << ";" << Line::End <<
					"const float at = max(alphaRoughness * (1.0 + anisoValue), 0.001);" << Line::End <<
					"const float ab = max(alphaRoughness * (1.0 - anisoValue), 0.001);" << Line::Blank;
			}
			else
			{
				/* Uniform rotation from UBO only (no direction texture). */
				Code{fragmentShader} <<
					"/* Anisotropy: build tangent frame from N and apply rotation. */" << Line::End <<
					"const float anisoValue = " << m_surfaceAnisotropy << ";" << Line::End <<
					"const float anisoRotation = " << m_surfaceAnisotropyRotation << " * 2.0 * 3.14159265;" << Line::End <<
					"const vec3 T0 = abs(N.y) < 0.999 ? normalize(cross(N, vec3(0.0, 1.0, 0.0))) : normalize(cross(N, vec3(1.0, 0.0, 0.0)));" << Line::End <<
					"const vec3 B0 = cross(N, T0);" << Line::End <<
					"const float cosR = cos(anisoRotation);" << Line::End <<
					"const float sinR = sin(anisoRotation);" << Line::End <<
					"const vec3 T = T0 * cosR + B0 * sinR;" << Line::End <<
					"const vec3 B = -T0 * sinR + B0 * cosR;" << Line::End <<
					"const float alphaRoughness = " << roughness << " * " << roughness << ";" << Line::End <<
					"const float at = max(alphaRoughness * (1.0 + anisoValue), 0.001);" << Line::End <<
					"const float ab = max(alphaRoughness * (1.0 - anisoValue), 0.001);" << Line::Blank;
			}

			/* Compute Cook-Torrance BRDF with anisotropic NDF and visibility. */
			if ( m_useIridescence )
			{
				Code{fragmentShader} <<
					"/* Cook-Torrance BRDF components (anisotropic + iridescence). */" << Line::End <<
					"const float NDF = distributionGGXAniso(N, H, T, B, at, ab);" << Line::End <<
					"const float V_aniso = visibilityAniso(T, B, N, V, L, at, ab);" << Line::End <<
					"const vec3 F_base = fresnelSchlick(max(dot(H, V), 0.0), F0);" << Line::End <<
					"const vec3 F_iridescence = evalIridescence(1.0, iridescenceIOR, max(dot(H, V), 0.0), iridescenceThickness, F0);" << Line::End <<
					"const vec3 F = mix(F_base, F_iridescence, iridescenceFactor);" << Line::Blank;
			}
			else
			{
				Code{fragmentShader} <<
					"/* Cook-Torrance BRDF components (anisotropic). */" << Line::End <<
					"const float NDF = distributionGGXAniso(N, H, T, B, at, ab);" << Line::End <<
					"const float V_aniso = visibilityAniso(T, B, N, V, L, at, ab);" << Line::End <<
					"const vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);" << Line::Blank;
			}

			/* Specular contribution (visibility already includes 1/(4*NdotV*NdotL)). */
			Code{fragmentShader} <<
				"/* Specular contribution (anisotropic). */" << Line::End <<
				"const vec3 specular = NDF * V_aniso * F;" << Line::Blank;
		}
		else
		{
			/* Compute the Cook-Torrance BRDF components. */
			if ( m_useIridescence )
			{
				Code{fragmentShader} <<
					"/* Cook-Torrance BRDF components (iridescence). */" << Line::End <<
					"const float NDF = distributionGGX(N, H, " << roughness << ");" << Line::End <<
					"const float G = geometrySmith(N, V, L, " << roughness << ");" << Line::End <<
					"const vec3 F_base = fresnelSchlick(max(dot(H, V), 0.0), F0);" << Line::End <<
					"const vec3 F_iridescence = evalIridescence(1.0, iridescenceIOR, max(dot(H, V), 0.0), iridescenceThickness, F0);" << Line::End <<
					"const vec3 F = mix(F_base, F_iridescence, iridescenceFactor);" << Line::Blank;
			}
			else
			{
				Code{fragmentShader} <<
					"/* Cook-Torrance BRDF components. */" << Line::End <<
					"const float NDF = distributionGGX(N, H, " << roughness << ");" << Line::End <<
					"const float G = geometrySmith(N, V, L, " << roughness << ");" << Line::End <<
					"const vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);" << Line::Blank;
			}

			/* Compute the specular contribution. */
			Code{fragmentShader} <<
				"/* Specular contribution. */" << Line::End <<
				"const vec3 numerator = NDF * G * F;" << Line::End <<
				"const float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;" << Line::End <<
				"const vec3 specular = numerator / denominator;" << Line::Blank;
		}

		/* Compute the diffuse contribution with energy conservation. */
		Code{fragmentShader} <<
			"/* Energy conservation: kD + kS = 1.0, metals have no diffuse. */" << Line::End <<
			"const vec3 kS = F;" << Line::End <<
			"const vec3 kD = (vec3(1.0) - kS) * (1.0 - " << metalness << ");" << Line::Blank;

		/* Compute the final lighting. */
		Code{fragmentShader} <<
			"/* Final NdotL factor. */" << Line::End <<
			"const float NdotL = max(dot(N, L), 0.0);" << Line::Blank;

		/* NOTE: Color projection. Default vec3(1.0) is a no-op on multiply.
		 * When enabled, the texture is sampled using projection coordinates. */
		{
			Code{fragmentShader} << "vec3 projectionColor = vec3(1.0);" << Line::End;

			if ( enableColorProjection )
			{
				fragmentShader.addComment("Sample the color projection texture from the bindless array.");

				if ( lightType == LightType::Point )
				{
					Code{fragmentShader} <<
						"{ const uint cpIdx = floatBitsToUint(" << LightUB(UniformBlock::Component::ColorProjectionIndex) << ");" << Line::End <<
						"  const uint cpFrameBits = floatBitsToUint(" << LightUB(UniformBlock::Component::ColorProjectionFrameIndex) << ");" << Line::End <<
						"  if ( cpIdx != 0xFFFFFFFFu && cpFrameBits == 0xFFFFFFFFu ) { projectionColor = texture(" << Bindless::TexturesCube << "[" << GLSL::Functions::NonUniformEXT << "(cpIdx)], DirectionWorldSpace.xyz).rgb; }" << Line::End <<
						"  if ( cpIdx != 0xFFFFFFFFu && cpFrameBits != 0xFFFFFFFFu ) { projectionColor = texture(" << Bindless::TexturesCubeArray << "[" << GLSL::Functions::NonUniformEXT << "(cpIdx)], vec4(DirectionWorldSpace.xyz, float(cpFrameBits))).rgb; } }" << Line::End;
				}
				else if ( !useCSM )
				{
					Code{fragmentShader} <<
						"{ const uint cpIdx = floatBitsToUint(" << LightUB(UniformBlock::Component::ColorProjectionIndex) << ");" << Line::End <<
						"  if ( cpIdx != 0xFFFFFFFFu )" << Line::End <<
						"  { const vec3 projCoords = " << ShaderVariable::PositionLightSpace << ".xyz / " << ShaderVariable::PositionLightSpace << ".w;" << Line::End <<
						"    projectionColor = texture(" << Bindless::Textures2D << "[" << GLSL::Functions::NonUniformEXT << "(cpIdx)], projCoords.xy).rgb; } }" << Line::End;
				}
			}
		}

		/* Compute radiance and final output. */
		Code{fragmentShader} <<
			"/* Light radiance. */" << Line::End <<
			"const vec3 radiance = " << this->lightColor() << ".rgb * projectionColor * " << this->lightIntensity() << " * " << LightFactor << ";" << Line::Blank;

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
		if ( m_useClearCoat && m_useSubsurface )
		{
			/* Compute clear coat normal (Ncc). */
			if ( !m_surfaceClearCoatNormal.empty() )
			{
				Code{fragmentShader} <<
					"/* Clear coat normal from dedicated normal map, transformed by fragment-local TBN from N. */" << Line::End <<
					"const vec3 ccT = abs(N.y) < 0.999 ? normalize(cross(N, vec3(0.0, 1.0, 0.0))) : normalize(cross(N, vec3(1.0, 0.0, 0.0)));" << Line::End <<
					"const vec3 ccB = cross(N, ccT);" << Line::End <<
					"const vec3 Ncc = normalize(ccT * " << m_surfaceClearCoatNormal << ".x + ccB * " << m_surfaceClearCoatNormal << ".y + N * " << m_surfaceClearCoatNormal << ".z);" << Line::End;
			}
			else
			{
				Code{fragmentShader} <<
					"/* No separate clear coat normal: use surface normal. */" << Line::End <<
					"const vec3 Ncc = N;" << Line::End;
			}

			Code{fragmentShader} <<
				"/* Clear coat second specular lobe. */" << Line::End <<
				"const float ccRoughness = " << m_surfaceClearCoatRoughness << ";" << Line::End <<
				"const float ccFactor = " << m_surfaceClearCoatFactor << ";" << Line::End <<
				"const vec3 Fc = fresnelSchlick(max(dot(H, V), 0.0), vec3(0.04));" << Line::End <<
				"const float Dc = distributionGGX(Ncc, H, ccRoughness);" << Line::End <<
				"const float Gc = geometrySmith(Ncc, V, L, ccRoughness);" << Line::End <<
				"const vec3 ccSpec = (Dc * Gc * Fc) / (4.0 * max(dot(Ncc, V), 0.0) * max(dot(Ncc, L), 0.0) + 0.0001);" << Line::Blank <<
				"/* SSS wrap diffuse with clear coat energy conservation. */" << Line::End <<
				"const float sssIntensity = " << m_surfaceSubsurfaceIntensity << ";" << Line::End <<
				"const vec3 sssColor = " << m_surfaceSubsurfaceColor << ".rgb;" << Line::End <<
				"const float sssWrap = sssIntensity;" << Line::End <<
				"const float NdotLWrap = max((dot(N, L) + sssWrap) / (1.0 + sssWrap), 0.0);" << Line::End <<
				"const float wrapFactor = smoothstep(0.0, sssWrap, NdotLWrap) * (1.0 - smoothstep(sssWrap, 1.0, NdotLWrap));" << Line::End <<
				"vec3 sssDiffuse = kD * " << albedo << " / 3.14159265 * NdotLWrap;" << Line::End <<
				"sssDiffuse = mix(sssDiffuse, sssDiffuse * sssColor, wrapFactor * sssWrap);" << Line::End <<
				"const vec3 baseLighting = (sssDiffuse + specular) * (vec3(1.0) - ccFactor * Fc);" << Line::End <<
				m_fragmentColor << ".rgb += (baseLighting + ccFactor * ccSpec) * radiance;";
		}
		else if ( m_useSubsurface )
		{
			Code{fragmentShader} <<
				"/* SSS wrap diffuse replaces standard Lambertian. */" << Line::End <<
				"const float sssIntensity = " << m_surfaceSubsurfaceIntensity << ";" << Line::End <<
				"const vec3 sssColor = " << m_surfaceSubsurfaceColor << ".rgb;" << Line::End <<
				"const float sssWrap = sssIntensity;" << Line::End <<
				"const float NdotLWrap = max((dot(N, L) + sssWrap) / (1.0 + sssWrap), 0.0);" << Line::End <<
				"const float wrapFactor = smoothstep(0.0, sssWrap, NdotLWrap) * (1.0 - smoothstep(sssWrap, 1.0, NdotLWrap));" << Line::End <<
				"vec3 sssDiffuse = kD * " << albedo << " / 3.14159265 * NdotLWrap;" << Line::End <<
				"sssDiffuse = mix(sssDiffuse, sssDiffuse * sssColor, wrapFactor * sssWrap);" << Line::End <<
				m_fragmentColor << ".rgb += (sssDiffuse + specular) * radiance;";
		}
		else if ( m_useClearCoat )
		{
			/* Compute clear coat normal (Ncc). */
			if ( !m_surfaceClearCoatNormal.empty() )
			{
				Code{fragmentShader} <<
					"/* Clear coat normal from dedicated normal map, transformed by fragment-local TBN from N. */" << Line::End <<
					"const vec3 ccT = abs(N.y) < 0.999 ? normalize(cross(N, vec3(0.0, 1.0, 0.0))) : normalize(cross(N, vec3(1.0, 0.0, 0.0)));" << Line::End <<
					"const vec3 ccB = cross(N, ccT);" << Line::End <<
					"const vec3 Ncc = normalize(ccT * " << m_surfaceClearCoatNormal << ".x + ccB * " << m_surfaceClearCoatNormal << ".y + N * " << m_surfaceClearCoatNormal << ".z);" << Line::End;
			}
			else
			{
				Code{fragmentShader} <<
					"/* No separate clear coat normal: use surface normal. */" << Line::End <<
					"const vec3 Ncc = N;" << Line::End;
			}

			Code{fragmentShader} <<
				"/* Clear coat second specular lobe. */" << Line::End <<
				"const float ccRoughness = " << m_surfaceClearCoatRoughness << ";" << Line::End <<
				"const float ccFactor = " << m_surfaceClearCoatFactor << ";" << Line::End <<
				"const vec3 Fc = fresnelSchlick(max(dot(H, V), 0.0), vec3(0.04));" << Line::End <<
				"const float Dc = distributionGGX(Ncc, H, ccRoughness);" << Line::End <<
				"const float Gc = geometrySmith(Ncc, V, L, ccRoughness);" << Line::End <<
				"const vec3 ccSpec = (Dc * Gc * Fc) / (4.0 * max(dot(Ncc, V), 0.0) * max(dot(Ncc, L), 0.0) + 0.0001);" << Line::Blank <<
				"/* Apply PBR lighting with clear coat energy conservation. */" << Line::End <<
				"const vec3 baseLighting = (kD * " << albedo << " / 3.14159265 + specular) * (vec3(1.0) - ccFactor * Fc);" << Line::End <<
				m_fragmentColor << ".rgb += (baseLighting + ccFactor * ccSpec) * radiance * NdotL;";
		}
		else
		{
			Code{fragmentShader} <<
				"/* Apply PBR lighting. */" << Line::End <<
				m_fragmentColor << ".rgb += (kD * " << albedo << " / 3.14159265 + specular) * radiance * NdotL;";
		}

		/* SSS back-lit transmittance (applies after main diffuse/specular regardless of clear coat). */
		if ( m_useSubsurface )
		{
			if ( m_useSubsurfaceThicknessMap )
			{
				Code{fragmentShader} <<
					Line::Blank <<
					"/* SSS back-lit transmittance with thickness map. */" << Line::End <<
					"const float sssRadius = " << m_surfaceSubsurfaceRadius << ";" << Line::End <<
					"const float thickness = " << m_surfaceSubsurfaceThickness << ";" << Line::End <<
					"const float NdotLBack = max(dot(-N, L), 0.0);" << Line::End <<
					"const vec3 sssTransmittance = " << m_surfaceSubsurfaceColor << ".rgb * exp(-thickness / sssRadius) * NdotLBack * " << m_surfaceSubsurfaceIntensity << ";" << Line::End <<
					m_fragmentColor << ".rgb += sssTransmittance * radiance;";
			}
			else
			{
				Code{fragmentShader} <<
					Line::Blank <<
					"/* SSS back-lit transmittance (no thickness map, uniform thin assumption). */" << Line::End <<
					"const float sssRadius = " << m_surfaceSubsurfaceRadius << ";" << Line::End <<
					"const float NdotLBack = max(dot(-N, L), 0.0);" << Line::End <<
					"const vec3 sssTransmittance = " << m_surfaceSubsurfaceColor << ".rgb * exp(-0.5 / sssRadius) * NdotLBack * " << m_surfaceSubsurfaceIntensity << ";" << Line::End <<
					m_fragmentColor << ".rgb += sssTransmittance * radiance;";
			}
		}

		/* Sheen lobe (applies after main lighting for fabric-like materials). */
		if ( m_useSheen )
		{
			Code{fragmentShader} <<
				Line::Blank <<
				"/* Sheen lobe (Charlie NDF + Ashikhmin visibility). */" << Line::End <<
				"const vec3 sheenColor = " << m_surfaceSheenColor << ".rgb;" << Line::End <<
				"const float sheenRoughness = " << m_surfaceSheenRoughness << ";" << Line::End <<
				"const float Dsheen = distributionCharlie(N, H, sheenRoughness);" << Line::End <<
				"const float Vsheen = visibilityAshikhmin(max(dot(N, V), 0.0), NdotL);" << Line::End <<
				"const vec3 sheenSpec = sheenColor * Dsheen * Vsheen;" << Line::End <<
				"/* Energy conservation: approximate directional albedo of the sheen layer. */" << Line::End <<
				"const float sheenDFG = 0.157 * sheenRoughness + 0.04;" << Line::End <<
				"const float sheenScaling = 1.0 - max(max(sheenColor.r, sheenColor.g), sheenColor.b) * sheenDFG;" << Line::End <<
				m_fragmentColor << ".rgb = " << m_fragmentColor << ".rgb * sheenScaling + sheenSpec * radiance * NdotL;";
		}

		/* Transmission back-lit (light passes through thin/translucent surfaces from behind).
		 * Beer's law provides wavelength-dependent absorption for colored glass. */
		if ( m_useTransmission )
		{
			const auto albedoB = m_surfaceAlbedo.empty() ? "vec3(1.0)" : m_surfaceAlbedo + ".rgb";

			/* NOTE: NdotLBack may already be declared by SSS. Use a different name to avoid redeclaration. */
			Code{fragmentShader} <<
				Line::Blank <<
				"/* Transmission back-lit - light passing through the surface. */" << Line::End <<
				"const float transNdotLBack = max(dot(-N, L), 0.0);" << Line::End <<
				"const vec3 transBackAbsorption = exp(log(max(" << m_surfaceAttenuationColor << ".rgb, vec3(0.001))) / max(" << m_surfaceAttenuationDistance << ", 0.0001) * " << m_surfaceThicknessFactor << ");" << Line::End <<
				"const vec3 transmitted = " << albedoB << " * transBackAbsorption * transNdotLBack * " << m_surfaceTransmissionFactor << ";" << Line::End <<
				m_fragmentColor << ".rgb += transmitted * radiance;";
		}

		/* NOTE: IBL (reflection/refraction from environment cubemap) is handled ONLY in the ambient pass.
		 * This prevents IBL accumulation when multiple lights are present.
		 * Direct light passes only compute Cook-Torrance BRDF for specular highlights from each light source.
		 * The IBLIntensity parameter in the ambient pass controls the cubemap contribution. */

		return true;
	}
}
