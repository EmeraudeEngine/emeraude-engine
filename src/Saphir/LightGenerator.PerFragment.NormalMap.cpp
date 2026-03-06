/*
 * src/Saphir/LightGenerator.PerFragment.NormalMap.cpp
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
	
	bool
	LightGenerator::generatePhongBlinnWithNormalMapVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, LightType lightType, bool enableShadowMap, bool enableColorProjection) const noexcept
	{
		//TraceDebug{ClassId} << "Generating '" << to_string(lightType) << "' vertex shader [PerFragment][NormalMap:1][ShadowMapSampler:" << enableShadowMap << "] ...";

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

		/* NOTE: For all light types. */
		if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalViewSpace, VariableScope::ToNextStage) )
		{
			return false;
		}

		if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::ViewTBNMatrix, VariableScope::ToNextStage) )
		{
			return false;
		}

		/* NOTE: Another type of light already computes the position in view space. */
		if ( !m_surfaceSpecularColor.empty() && lightType == LightType::Directional )
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
	LightGenerator::generatePhongBlinnWithNormalMapFragmentShader (Generator::Abstract & generator, FragmentShader & fragmentShader, LightType lightType, [[maybe_unused]] bool enableShadowMap, [[maybe_unused]] bool enableColorProjection) const noexcept
	{
		//TraceDebug{ClassId} << "Generating '" << to_string(lightType) << "' fragment shader [PerFragment][NormalMap:1][ShadowMapSampler:" << enableShadowMap << "] ...";

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

		/* Discard backward normal sample code. */
		if ( !m_useStaticLighting )
		{
			Code{fragmentShader} << "if ( dot(-" << rayDirectionViewSpace << ", " << ShaderVariable::NormalViewSpace << ") < -0.33 ) { discard; }";
		}

		/* Get the ray direction in texture space */
		{
			fragmentShader.addComment("Compute the ray direction in texture space.");

			Code{fragmentShader} << "const vec3 RayDirectionTextureSpace = " << ShaderVariable::ViewTBNMatrix << " * " << rayDirectionViewSpace << ";";
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
			fragmentShader.addComment("Compute the code influence over the light factor [Spot].");

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

				Code{fragmentShader} <<
					"{ const float cpBoost = " << LightUB(UniformBlock::Component::ColorProjectionBoost) << ";" << Line::End <<
					"  if ( cpBoost > 0.0 ) { projectionColor = vec3(1.0) + projectionColor * cpBoost; } }" << Line::End;
			}
		}

		{
			fragmentShader.addComment("Compute the diffuse factor.");

			Code{fragmentShader} <<
				"float " << DiffuseFactor << " = 0.0;" << Line::Blank <<

				"if ( " << LightFactor << " > 0.0 )" << Line::End <<
				"	" << DiffuseFactor << " = max(dot(-RayDirectionTextureSpace, " << m_surfaceNormalVector << "), 0.0) * " << LightFactor << ';' << Line::End;
		}

		if ( !m_surfaceSpecularColor.empty() )
		{
			fragmentShader.addComment("Compute the specular factor.");

			Code{fragmentShader} <<
				"float " << SpecularFactor << " = 0.0;" << Line::Blank <<

				"if ( " << DiffuseFactor << " > 0.0 ) " << Line::End <<
				'{' << Line::End <<
				"	const vec3 positionTextureSpace = " << ShaderVariable::ViewTBNMatrix << " * " << ShaderVariable::PositionViewSpace << ".xyz;" << Line::End <<
				"	const vec3 R = reflect(RayDirectionTextureSpace, " << m_surfaceNormalVector << ");" << Line::End <<
				"	const vec3 V = normalize(-positionTextureSpace);" << Line::End <<
				"	" << SpecularFactor << " = pow(max(dot(R, V), 0.0), " << m_surfaceShininessAmount << ") * " << LightFactor << ';' << Line::End <<
				'}' << Line::End;
		}

		return this->generateFinalFragmentOutput(fragmentShader, DiffuseFactor, SpecularFactor);
	}
}
