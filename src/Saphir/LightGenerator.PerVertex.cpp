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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "LightGenerator.hpp"

/* Local inclusions. */
#include "Code.hpp"
#include "Declaration/OutputBlock.hpp"
#include "Declaration/Sampler.hpp"
#include "Declaration/SpecializationConstant.hpp"
#include "Generator/Abstract.hpp"
#include "Graphics/BindlessTextureManager.hpp"

namespace EmEn::Saphir
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Graphics;
	using namespace Saphir::Keys;
	using namespace Vulkan;

	bool
	LightGenerator::generateGouraudVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, LightType lightType, bool enableShadowMap, bool enableColorProjection) const noexcept
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
				/* ⚠️ THE GUARD IS LOAD-BEARING: epsilon is ZERO for a HARD-EDGED spot.
				 * inner == outer is not a degenerate case, it is how a hard cone edge is
				 * expressed — and it is exactly what USD's `shaping:cone:softness = 0`
				 * means, which is what every fixture of a Kit export declares. Unguarded,
				 * the division is 0/0 for the fragments ON the edge and x/0 elsewhere;
				 * the result is driver-dependent and it came back as a PURE BLACK FRAME
				 * on 25 correctly-placed 3750 cd ceiling spots, with no error anywhere.
				 * The ray-traced path (RTR/RTGI) has always guarded it the same way. */
				"	const float epsilon = max(" << innerCosAngle << " - " << outerCosAngle << ", 0.0001);" << Line::End <<
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
				"	const vec3 V = normalize(-" << ShaderVariable::PositionViewSpace << ".xyz);" << Line::End <<
				/* BLINN-PHONG: the specular lobe is built on the HALF VECTOR
				 * H = normalize(L + V), not on the reflected ray. L is the direction TOWARDS the
				 * light, i.e. the opposite of the ray's travel direction, hence "V - rayDirection".
				 * Why the half vector: dot(N, H) keeps a continuous falloff at grazing angles where
				 * dot(R, V) goes negative and truncates the highlight along a hard edge, and its lobe
				 * stretches with the viewing angle instead of staying a disc — which is what a real
				 * specular reflection does on a flat surface (the sun's glitter path on water).
				 * It is also a microfacet normal distribution over H, the same family as the PBR
				 * path's GGX, so shininess and roughness can be related. Phong's lobe, parameterised
				 * around R, cannot be.
				 * ⚠️ dot(N, H) > dot(R, V) for the same geometry, so the highlight is WIDER than the
				 * Phong one it replaces at an equal exponent.
				 * ENERGY NORMALISATION (n+2)/(8.pi): without it the term was a raw multiple of the
				 * ILLUMINANCE and commensurable with nothing — a 0.5 grey specular under a 50000 lx
				 * sun returned 22350 nits, five times the sky's own luminance, which is what read as
				 * "flashy" on every Standard surface. With the normalisation AND the cos(theta) term
				 * below, the specular is a BRDF times an irradiance, exactly like the diffuse
				 * (albedo/pi * E * N.L), so the two terms are finally comparable to each other and to
				 * lights authored in lux/candela.
				 * The exponent itself comes from the material uniform, which the manifest parser fills
				 * through StandardResource::specularExponentFromGlossiness(). */
				"	const vec3 H = normalize(V - " << RayDirectionViewSpace << ");" << Line::End <<
				"	const float specularExponent = max(" << m_surfaceShininessAmount << ", 1.0);" << Line::End <<
				"	" << LightGenerator::variable(SpecularFactor) << " = pow(max(dot(" << ShaderVariable::NormalViewSpace << ", H), 0.0), specularExponent) * ((specularExponent + 2.0) / 25.132741228718345) * " << LightGenerator::variable(DiffuseFactor) << ';' << Line::End <<
				'}' << Line::End <<
				"else" << Line::End <<
				'{' << Line::End <<
				"	" << LightGenerator::variable(SpecularFactor) << " = 0.0;" << Line::End <<
				'}';
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
				/* NOTE: CSM computes the light-space position in the fragment shader, from the
				 * world-space position. The cascade itself is selected on VIEW-space depth, and
				 * the view matrix is not reachable from a fragment shader on the main render
				 * target: its view UBO does not carry one (it travels as a push constant, vertex
				 * stage only). Hence the view-space position is forwarded as well. */
				if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionWorldSpace, VariableScope::ToNextStage) )
				{
					return false;
				}

				if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::PositionViewSpace, VariableScope::ToNextStage) )
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
	LightGenerator::generateGouraudFragmentShader (Generator::Abstract & generator, FragmentShader & fragmentShader, LightType lightType, [[maybe_unused]] bool enableShadowMap, [[maybe_unused]] bool enableColorProjection) const noexcept
	{
		//TraceDebug{ClassId} << "Generating '" << to_string(lightType) << "' fragment shader [PerVertex][NormalMap:" << m_flags[UseNormalMapping] << "][ShadowMapSampler:" << enableShadowMap << "] ...";

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
						/* NOTE: CSM needs the world-space position for the cascade matrix, the
						 * view-space position for the cascade selection depth, and the cascade
						 * matrices / split distances from the light UBO. */
						Code{fragmentShader} << '\t' << this->generateCSMShadowMapCode(
							Uniform::ShadowMapSampler,
							std::string(ShaderVariable::PositionWorldSpace) + ".xyz",
							std::string(ShaderVariable::PositionViewSpace),
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
						"	projectionColor = texture(" << Bindless::Textures2D << "[" << GLSL::Functions::NonUniformEXT << "(cpIdx)], projCoords.xy).rgb; } }" << Line::End;
				}

				Code{fragmentShader} <<
					"{ const float cpBoost = " << LightUB(UniformBlock::Component::ColorProjectionBoost) << ";" << Line::End <<
					"  if ( cpBoost > 0.0 ) { projectionColor = vec3(1.0) + projectionColor * cpBoost; } }" << Line::End;
			}
		}

		return this->generateFinalFragmentOutput(fragmentShader, "diffuseFactor", !m_surfaceSpecularColor.empty() ? "specularFactor" : LightGenerator::variable(SpecularFactor));
	}
}
