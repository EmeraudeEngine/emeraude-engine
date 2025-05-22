/*
 * src/Saphir/LightGenerator.PerFragment.NormalMap.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include "Generator/Abstract.hpp"
#include "Code.hpp"

namespace EmEn::Saphir
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;
	using namespace EmEn::Graphics;
	using namespace EmEn::Saphir::Keys;
	using namespace EmEn::Vulkan;
	
	bool
	LightGenerator::generatePhongBlinnWithNormalMapVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, LightType lightType, bool enableShadowMap) const noexcept
	{
		//TraceDebug{ClassId} << "Generating '" << to_string(lightType) << "' vertex shader [PerFragment][NormalMap:1][ShadowMapSampler:" << enableShadowMap << "] ...";

		Declaration::OutputBlock lightBlock{LightBlock, generator.getNextShaderVariableLocation(lightType == LightType::Spot ? 2 : 1), ShaderVariable::Light};

		if ( lightType == LightType::Directional )
		{
			vertexShader.addComment("Compute the light direction in view space (Normalized vector).");

			lightBlock.addMember(Declaration::VariableType::FloatVector3, RayDirectionViewSpace, GLSL::Smooth);

			Code{vertexShader, Location::Output} << Variable(RayDirectionViewSpace) << " = normalize((" << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << this->lightDirectionWorldSpace() << ").xyz);";
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

			Code{vertexShader, Location::Output} << Variable(Distance) << " = " << ShaderVariable::PositionViewSpace << ".xyz - " << LightPositionViewSpace << ".xyz;";
		}

		if ( lightType == LightType::Spot )
		{
			lightBlock.addMember(Declaration::VariableType::FloatVector3, SpotLightDirectionViewSpace, GLSL::Smooth);

			Code{vertexShader, Location::Output} << Variable(SpotLightDirectionViewSpace) << " = normalize((" << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << this->lightDirectionWorldSpace() << ").xyz);";
		}

		/* NOTE: For all light type. */
		if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalViewSpace, VariableScope::ToNextStage) )
		{
			return false;
		}

		if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::ViewTBNMatrix, VariableScope::ToNextStage) )
		{
			return false;
		}

		/* NOTE: Other type of light already compute the position in view space. */
		if ( !m_surfaceSpecularColor.empty() && lightType == LightType::Directional )
		{
			if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionViewSpace, VariableScope::ToNextStage) )
			{
				return false;
			}
		}

		/* TODO: Test this code ! */
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
	LightGenerator::generatePhongBlinnWithNormalMapFragmentShader (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader, LightType lightType, bool enableShadowMap) const noexcept
	{
		//TraceDebug{ClassId} << "Generating '" << to_string(lightType) << "' fragment shader [PerFragment][NormalMap:1][ShadowMapSampler:" << enableShadowMap << "] ...";

		std::string rayDirectionViewSpace;

		if ( lightType != LightType::Directional )
		{
			fragmentShader.addComment("Compute the ray direction in view space.");

			Code{fragmentShader} << "const vec3 " << RayDirectionViewSpace << " = normalize(" << Variable(Distance) << ");";

			rayDirectionViewSpace = RayDirectionViewSpace;
		}
		else
		{
			rayDirectionViewSpace = Variable(RayDirectionViewSpace);
		}

		/* Discard backward normal sample code. */
		if ( !m_flags[UseStaticLighting] )
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
				"	const vec3 DR = abs(" << Variable(Distance) << ") / " << this->lightRadius() << ';' << Line::Blank <<

				"	" << LightFactor << " *= max(1.0 - dot(DR, DR), 0.0);" << Line::End <<
				'}' << Line::End;

			if ( m_flags[DiscardUnlitFragment] )
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
				"	const float theta = dot(" << rayDirectionViewSpace << ", " << Variable(SpotLightDirectionViewSpace) << ");" << Line::End <<
				"	const float epsilon = " << innerCosAngle << " - " << outerCosAngle << ";" << Line::End <<
				"	const float spotFactor = clamp((theta - " << outerCosAngle << ") / epsilon, 0.0, 1.0);" << Line::End <<
				"	" << LightFactor << " *= spotFactor;" << Line::End <<
				'}' << Line::End;

			if ( m_flags[DiscardUnlitFragment] )
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
					//gen.declare(Declaration::Uniform{Declaration::VariableType::Sampler2DShadow, Uniform::ShadowMapSampler});

					Code{fragmentShader} << this->generate2DShadowMapCode(Uniform::ShadowMapSampler, Variable(ShaderBlock::Component::PositionLightSpace), DepthTextureFunction::TextureGather) << Line::Blank;
					break;

				case LightType::Point :
					//gen.declare(Declaration::Uniform{Declaration::VariableType::Sampler2DShadow, Uniform::ShadowMapSampler});

					// FIXME: NearFar must comme from the view UBO
					//Code{fragmentShader} << this->generate3DShadowMapCode(Uniform::ShadowMapSampler, Variable(ShaderBlock::Component::DirectionWorldSpace), Uniform::NearFar) << Line::End;
					break;

				case LightType::Spot :
					//gen.declare(Declaration::Uniform{Declaration::VariableType::Sampler2DShadow, Uniform::ShadowMapSampler});

					Code{fragmentShader} << this->generate2DShadowMapCode(Uniform::ShadowMapSampler, Variable(ShaderBlock::Component::PositionLightSpace), DepthTextureFunction::TextureProj) << Line::End;
					break;
			}

			Code{fragmentShader} << LightFactor << " *= shadowFactor;" << Line::End;
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
