/*
 * src/Saphir/LightGenerator.PerVertex.cpp
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

		/* NOTE: In cubemap mode, the view matrix comes from the UBO indexed by gl_ViewIndex,
		 * not from the push constant. */
		const auto viewMatrixSource = vertexShader.isCubemapModeEnabled() ?
			ViewUB(UniformBlock::Component::ViewMatrix, true) :
			MatrixPC(PushConstant::Component::ViewMatrix);

		{
			if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionViewSpace, VariableScope::Local) )
			{
				return false;
			}

			if ( lightType == LightType::Directional )
			{
				vertexShader.addComment("Compute the light vector to vertex (L) [Directional].");

				/* NOTE: With directional light, the ray to vertex (L) is the same as the light direction in view space. */
				Code{vertexShader} << "const vec3 " << RayDirectionViewSpace << " = normalize((" << viewMatrixSource << " * " << this->lightDirectionWorldSpace() << ").xyz);" << Line::End;
			}
			else
			{
				vertexShader.addComment("Compute the light vector to vertex (L) [Point+Spot].");

				Code{vertexShader} <<
					"const vec4 " << LightPositionViewSpace << " = " << viewMatrixSource << " * " << this->lightPositionWorldSpace() << ';' << Line::End <<
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
				"	const vec3 " << SpotLightDirectionViewSpace << " = normalize((" << viewMatrixSource << " * " << this->lightDirectionWorldSpace() << ").xyz);" << Line::End <<
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

		/* NOTE: Shadow map prerequisites must be generated based on actual UBO structure.
		 * The UBO only contains viewProjectionMatrix when shadow mapping is enabled.
		 * Point lights use cubemap shadow maps requiring direction output for 3D lookup.
		 * CSM mode requires PositionWorldSpace instead of PositionLightSpace. */
		if ( enableShadowMap )
		{
			const bool useCSM = (m_renderPassType == RenderPassType::DirectionalLightPassCSM);

			vertexShader.addComment("Compute the shadow map prerequisites for next stage.");

			if ( useCSM )
			{
				/* NOTE: CSM computes light-space position in the fragment shader.
				 * We only need to pass the world-space position. */
				if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionWorldSpace, VariableScope::ToNextStage) )
				{
					return false;
				}
			}
			else
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
	LightGenerator::generateGouraudFragmentShader (Generator::Abstract & generator, FragmentShader & fragmentShader, LightType lightType, [[maybe_unused]] bool enableShadowMap) const noexcept
	{
		//TraceDebug{ClassId} << "Generating '" << to_string(lightType) << "' fragment shader [PerVertex][NormalMap:" << m_flags[UseNormalMapping] << "][ShadowMapSampler:" << enableShadowMap << "] ...";

		const auto lightSetIndex = generator.shaderProgram()->setIndex(SetType::PerLight);

		const bool useCSM = (m_renderPassType == RenderPassType::DirectionalLightPassCSM);

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

		if ( m_discardUnlitFragment )
		{
			Code{fragmentShader} << "if ( " << LightGenerator::variable(DiffuseFactor) << " <= 0.0 ) { discard; }" << Line::End;
		}

		/* NOTE: Create local copies of the light factors to allow modification.
		 * Shader inputs (from vertex shader) are read-only in GLSL. */
		Code{fragmentShader} << "float diffuseFactor = " << LightGenerator::variable(DiffuseFactor) << ";" << Line::End;

		if ( !m_surfaceSpecularColor.empty() )
		{
			Code{fragmentShader} << "float specularFactor = " << LightGenerator::variable(SpecularFactor) << ";" << Line::End;
		}

		/* NOTE: Shadow map influence is computed when shadow mapping is enabled. */
		if ( enableShadowMap )
		{
			fragmentShader.addComment("Compute the shadow influence over the light factor.");

			Code{fragmentShader} <<
				"if ( diffuseFactor > 0.0 )" << Line::End <<
				'{' << Line::End;

			switch ( lightType )
			{
				case LightType::Directional :
					if ( useCSM )
					{
						/* NOTE: CSM requires world-space position, view matrix for depth calculation,
						 * and cascade matrices/split distances from the light UBO. */
						Code{fragmentShader} << '\t' << this->generateCSMShadowMapCode(
							Uniform::ShadowMapSampler,
							std::string(ShaderVariable::PositionWorldSpace) + ".xyz",
							ViewUB(UniformBlock::Component::ViewMatrix, false),
							LightUB(UniformBlock::Component::CascadeViewProjectionMatrices),
							LightUB(UniformBlock::Component::CascadeSplitDistances),
							LightUB(UniformBlock::Component::CascadeCount)
						) << Line::End;
					}
					else if ( m_PCFEnabled )
					{
						Code{fragmentShader} << '\t' << this->generate2DShadowMapPCFCode(Uniform::ShadowMapSampler, ShaderVariable::PositionLightSpace) << Line::End;
					}
					else
					{
						Code{fragmentShader} << '\t' << this->generate2DShadowMapCode(Uniform::ShadowMapSampler, ShaderVariable::PositionLightSpace) << Line::End;
					}
					break;

				case LightType::Point :
					/* NOTE: Point lights use cubemap shadow maps. The direction from fragment to light
					 * is used as the lookup vector. Depth is linearized using the light radius as far plane.
					 * nearFar.y = radius is used to convert stored depth to world-space distance. */
					if ( m_PCFEnabled )
					{
						Code{fragmentShader} << '\t' << this->generate3DShadowMapPCFCode(Uniform::ShadowMapSampler, "DirectionWorldSpace", "vec2(0.1, " + this->lightRadius() + ")") << Line::End;
					}
					else
					{
						Code{fragmentShader} << '\t' << this->generate3DShadowMapCode(Uniform::ShadowMapSampler, "DirectionWorldSpace", "vec2(0.1, " + this->lightRadius() + ")") << Line::End;
					}
					break;

				case LightType::Spot :
					if ( m_PCFEnabled )
					{
						Code{fragmentShader} << '\t' << this->generate2DShadowMapPCFCode(Uniform::ShadowMapSampler, ShaderVariable::PositionLightSpace) << Line::End;
					}
					else
					{
						Code{fragmentShader} << '\t' << this->generate2DShadowMapCode(Uniform::ShadowMapSampler, ShaderVariable::PositionLightSpace) << Line::End;
					}
					break;
			}

			Code{fragmentShader} << '\t' << "diffuseFactor *= shadowFactor;" << Line::End;

			if ( !m_surfaceSpecularColor.empty() )
			{
				Code{fragmentShader} << '\t' << "specularFactor *= shadowFactor;" << Line::End;
			}

			Code{fragmentShader} << '}' << Line::End;
		}

		return this->generateFinalFragmentOutput(fragmentShader, "diffuseFactor", !m_surfaceSpecularColor.empty() ? "specularFactor" : LightGenerator::variable(SpecularFactor));
	}
}
