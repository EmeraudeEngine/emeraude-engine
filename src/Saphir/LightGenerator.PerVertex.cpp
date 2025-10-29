/*
 * src/Saphir/LightGenerator.PerVertex.cpp
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
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Graphics;
	using namespace Saphir::Keys;
	using namespace Vulkan;

	bool
	LightGenerator::generateGouraudVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, LightType lightType, bool enableShadowMap) const noexcept
	{
		//TraceDebug{ClassId} << "Generating '" << to_string(lightType) << "' vertex shader [PerVertex][NormalMap:" << m_flags[UseNormalMapping] << "][ShadowMapSampler:" << enableShadowMap << "] ...";

		Declaration::OutputBlock lightBlock{LightBlock, generator.getNextShaderVariableLocation(2), ShaderVariable::Light};

		{
			if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionViewSpace, VariableScope::Local) )
			{
				return false;
			}

			if ( lightType == LightType::Directional )
			{
				vertexShader.addComment("Compute the light vector to vertex (L) [Directional].");

				/* NOTE: With directional light, the ray to vertex (L) is the same as the light direction in view space. */
				Code{vertexShader} << "const vec3 " << RayDirectionViewSpace << " = normalize((" << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << this->lightDirectionWorldSpace() << ").xyz);" << Line::End;
			}
			else
			{
				vertexShader.addComment("Compute the light vector to vertex (L) [Point+Spot].");

				Code{vertexShader} <<
					"const vec4 " << LightPositionViewSpace << " = " << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << this->lightPositionWorldSpace() << ';' << Line::End <<
					"const vec3 " << Distance << " = " << ShaderVariable::PositionViewSpace << ".xyz - " << LightPositionViewSpace << ".xyz;" << Line::End <<
					"const vec3 " << RayDirectionViewSpace << " = normalize(" << Distance << ");" << Line::End;
			}
		}

		Code{vertexShader} << "float " << LightFactor << " = 1.0;" << Line::End;

		if ( lightType != LightType::Directional )
		{
			vertexShader.addComment("Compute the radius influence over the light factor [Point+Spot].");

			Code{vertexShader} <<
				"if ( " << this->lightRadius() << " > 0.0 ) " << Line::End <<
				'{' << Line::End <<
				"	const vec3 DR = abs(" << Distance << ") / " << this->lightRadius() << ';' << Line::Blank <<

				"	" << LightFactor << " = max(1.0 - dot(DR, DR), 0.0);" << Line::End <<
				'}' << Line::End;
		}

		if ( lightType == LightType::Spot )
		{
			vertexShader.addComment("Compute the code influence over the light factor [Spot].");

			const auto innerCosAngle = this->lightInnerCosAngle();
			const auto outerCosAngle = this->lightOuterCosAngle();

			Code{vertexShader} <<
				"if ( " << LightFactor << " > 0.0 )" << Line::End <<
				'{' << Line::End <<
				"	const vec3 " << SpotLightDirectionViewSpace << " = normalize((" << MatrixPC(PushConstant::Component::ViewMatrix) << " * " << this->lightDirectionWorldSpace() << ").xyz);" << Line::End <<
				"	const float theta = dot(" << RayDirectionViewSpace << ", " << SpotLightDirectionViewSpace << ");" << Line::End <<
				"	const float epsilon = " << innerCosAngle << " - " << outerCosAngle << ";" << Line::End <<
				"	" << LightFactor << " *= clamp((theta - " << outerCosAngle << ") / epsilon, 0.0, 1.0);" << Line::End <<
				'}' << Line::End;
		}

		{
			vertexShader.addComment("Compute the diffuse factor.");

			lightBlock.addMember(Declaration::VariableType::Float, DiffuseFactor, GLSL::Smooth);

			if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalViewSpace, VariableScope::Local) )
			{
				return false;
			}

			Code{vertexShader} <<
				"if ( " << LightFactor << " > 0.0 )" << Line::End <<
				"	" << LightGenerator::variable(DiffuseFactor) << " = max(dot(-" << RayDirectionViewSpace << ", " << ShaderVariable::NormalViewSpace << "), 0.0) * " << LightFactor << ';' << Line::End <<
				"else" << Line::End <<
				"	" << LightGenerator::variable(DiffuseFactor) << " = 0.0;" << Line::End;
		}

		if ( !m_surfaceSpecularColor.empty() )
		{
			vertexShader.addComment("Compute the specular factor.");

			lightBlock.addMember(Declaration::VariableType::Float, SpecularFactor, GLSL::Smooth);

			if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionViewSpace, VariableScope::Local) )
			{
				return false;
			}

			Code{vertexShader} <<
				"if ( " << LightGenerator::variable(DiffuseFactor) << " > 0.0 ) " << Line::End <<
				'{' << Line::End <<
				"	const vec3 R = reflect(" << RayDirectionViewSpace << ", " << ShaderVariable::NormalViewSpace << ");" << Line::End <<
				"	const vec3 V = normalize(-" << ShaderVariable::PositionViewSpace << ".xyz);" << Line::End <<
				"	" << LightGenerator::variable(SpecularFactor) << " = pow(max(dot(R, V), 0.0), " << m_surfaceShininessAmount << ") * " << LightFactor << ';' << Line::End <<
				'}' << Line::End <<
				"else" << Line::End <<
				'{' << Line::End <<
				"	" << LightGenerator::variable(SpecularFactor) << " = 0.0;" << Line::End <<
				'}';
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
	LightGenerator::generateGouraudFragmentShader (Generator::Abstract & /*generator*/, FragmentShader & fragmentShader, LightType lightType, bool enableShadowMap) const noexcept
	{
		//TraceDebug{ClassId} << "Generating '" << to_string(lightType) << "' fragment shader [PerVertex][NormalMap:" << m_flags[UseNormalMapping] << "][ShadowMapSampler:" << enableShadowMap << "] ...";
		
		if ( m_discardUnlitFragment )
		{
			Code{fragmentShader} << "if ( " << LightGenerator::variable(DiffuseFactor) << " <= 0.0 ) { discard; }" << Line::End;
		}

		/* TODO: Test this code ! */
		if ( enableShadowMap )
		{
			fragmentShader.addComment("Compute the shadow influence over the light factor.");

			Code{fragmentShader} <<
				"if ( " << LightGenerator::variable(DiffuseFactor) << " > 0.0 )" << Line::End <<
				'{';

			switch ( lightType )
			{
				case LightType::Directional :
					//fragmentShader.declare(Declaration::Uniform{Declaration::VariableType::Sampler2DShadow, Uniform::ShadowMapSampler});

					Code{fragmentShader} << this->generate2DShadowMapCode(Uniform::ShadowMapSampler, LightGenerator::variable(ShaderBlock::Component::PositionLightSpace), DepthTextureFunction::TextureGather) << Line::End;
					break;

				case LightType::Point :
					/* FIXME: samplerCubeShadow bugs with GLSL texture(); */
					//gen.declare(Declaration::Uniform{Declaration::VariableType::SamplerCube/*SamplerCubeShadow*/, Uniform::ShadowMapSampler});
					//gen.declare(Declaration::Uniform{Declaration::VariableType::FloatVector2, Uniform::NearFar, "vec2(0.0, 1.0)"});

					// FIXME : NearFar must come from view UBO
					//Code{fragmentShader} << this->generate3DShadowMapCode(Uniform::ShadowMapSampler, Variable(ShaderBlock::Component::DirectionWorldSpace), Uniform::NearFar) << Line::End;
					break;

				case LightType::Spot :
					//gen.declare(Declaration::Uniform{Declaration::VariableType::Sampler2DShadow, Uniform::ShadowMapSampler});

					Code{fragmentShader} << this->generate2DShadowMapCode(Uniform::ShadowMapSampler, LightGenerator::variable(ShaderBlock::Component::PositionLightSpace), DepthTextureFunction::TextureProj) << Line::End;
					break;
			}

			Code{fragmentShader} << LightGenerator::variable(DiffuseFactor) << " *= shadowFactor;";

			if ( !m_surfaceSpecularColor.empty() )
			{
				Code{fragmentShader} << LightGenerator::variable(SpecularFactor) << " *= shadowFactor;";
			}

			Code{fragmentShader} << '}' << Line::End;
		}

		return this->generateFinalFragmentOutput(fragmentShader, LightGenerator::variable(DiffuseFactor), LightGenerator::variable(SpecularFactor));
	}
}
