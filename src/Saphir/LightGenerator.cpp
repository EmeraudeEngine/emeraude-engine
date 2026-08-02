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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "LightGenerator.hpp"

/* Local inclusions. */
#include "Code.hpp"
#include "Declaration/Function.hpp"
#include "Declaration/Sampler.hpp"
#include "Generator/Abstract.hpp"
#include "Graphics/BindlessTextureManager.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::PixelFactory;
	using namespace Graphics;
	using namespace Saphir::Keys;
	using namespace Vulkan;

	std::string
	LightGenerator::roughnessShaderExpression () const noexcept
	{
		if ( !m_surfaceRoughness.empty() )
		{
			return m_surfaceRoughness;
		}

		if ( !m_surfaceShininessAmount.empty() )
		{
			return "sqrt(2.0 / (" + m_surfaceShininessAmount + " + 2.0))";
		}

		return "0.5";
	}

	std::string
	LightGenerator::metalnessShaderExpression () const noexcept
	{
		if ( !m_surfaceMetalness.empty() )
		{
			return m_surfaceMetalness;
		}

		return "0.0";
	}

	std::string
	LightGenerator::albedoShaderExpression () const noexcept
	{
		if ( !m_surfaceAlbedo.empty() )
		{
			return m_surfaceAlbedo;
		}

		if ( !m_surfaceDiffuseColor.empty() )
		{
			return m_surfaceDiffuseColor;
		}

		return "vec4(1.0, 1.0, 1.0, 1.0)";
	}

	std::string
	LightGenerator::finalNormalViewSpaceExpression () const noexcept
	{
		/* When normal mapping is active, the PBR lighting code declares
		 * 'const vec3 N = normalize(transpose(ViewTBNMatrix) * surfaceNormalVector)'
		 * which is the perturbed normal in view space. Use it for the MRT output
		 * so that post-process effects (RTR, SSR, SSAO, RTAO) see the normal-mapped surface. */
		if ( m_useNormalMapping && !m_surfaceNormalVector.empty() )
		{
			return "N";
		}

		/* NOTE: reserve() forces a heap buffer up-front. Without it, GCC 14's value-range
		 * analysis infers a length (~28) that exceeds the 15-byte SSO capacity yet still
		 * believes the data could live in the inline buffer, then flags the move-construct's
		 * char_traits::copy as a -Wstringop-overread overflow (a known false positive that
		 * only surfaces once the PCH shifts the STL inlining context). A guaranteed-heap
		 * string removes the ambiguity. */
		std::string expression;
		expression.reserve(sizeof("normalize(") + sizeof(Keys::ShaderVariable::NormalViewSpace));
		expression += "normalize(";
		expression += Keys::ShaderVariable::NormalViewSpace;
		expression += ')';

		return expression;
	}

	std::string
	LightGenerator::materialPropertiesExpression () const noexcept
	{
		/* Reflectivity (R high nibble):
		 * Priority 1: Dedicated reflectivity map (per-pixel, artist-controlled).
		 * Priority 2: PBR with IBL — iblIntensity modulated by smoothness.
		 * Priority 3: PBR metalness-derived reflectivity.
		 * Priority 4: Standard reflectionAmount.
		 * Fallback: 0 (no reflection). */
		std::string reflectivity;

		if ( m_useReflectivityMap && !m_surfaceReflectivityMap.empty() )
		{
			/* Dedicated reflectivity map: per-pixel control, highest priority. */
			reflectivity = "clamp(" + m_surfaceReflectivityMap + ", 0.0, 1.0)";
		}
		else if ( m_reflectionArtistic )
		{
			/* Artistic reflection (explicit cubemap texture): the author's look, never
			 * replaced by the post-process reflections — zero reflectivity published. */
			reflectivity = "0.0";
		}
		else if ( m_usePBRMode && !m_surfaceIBLIntensity.empty() && m_useReflection )
		{
			/* PBR with environment reflection: IBL intensity scaled by smoothness.
			 * Metallic surfaces always get high reflectivity. */
			if ( !m_surfaceMetalness.empty() && !m_surfaceRoughness.empty() )
			{
				reflectivity = "clamp(max(" + m_surfaceIBLIntensity + " * (1.0 - " + m_surfaceRoughness + "), " + m_surfaceMetalness + "), 0.0, 1.0)";
			}
			else if ( !m_surfaceRoughness.empty() )
			{
				reflectivity = "clamp(" + m_surfaceIBLIntensity + " * (1.0 - " + m_surfaceRoughness + "), 0.0, 1.0)";
			}
			else
			{
				reflectivity = "clamp(" + m_surfaceIBLIntensity + ", 0.0, 1.0)";
			}
		}
		else if ( m_usePBRMode && !m_surfaceMetalness.empty() && !m_surfaceRoughness.empty() )
		{
			/* PBR without explicit reflection: derive from metalness and smoothness. */
			reflectivity = "clamp(" + m_surfaceMetalness + " * (1.0 - " + m_surfaceRoughness + "), 0.0, 1.0)";
		}
		else if ( m_useReflection && !m_surfaceReflectionAmount.empty() )
		{
			/* Standard material: use the explicit reflection amount. */
			reflectivity = "clamp(" + m_surfaceReflectionAmount + ", 0.0, 1.0)";
		}
		else
		{
			reflectivity = "0.0";
		}

		/* AO response (G high nibble):
		 * PBR with AO: aoIntensity (0=no AO effect, 1=full AO).
		 * Others: 1.0 (full AO response). */
		std::string aoResponse;

		if ( m_useAmbientOcclusion && !m_surfaceAOIntensity.empty() )
		{
			aoResponse = "clamp(" + m_surfaceAOIntensity + ", 0.0, 1.0)";
		}
		else
		{
			aoResponse = "1.0";
		}

		/* Emissive mask (B low nibble):
		 * Any material with autoIllumination: amount (0=not emissive, 1=fully emissive).
		 * Others: 0 (not emissive). */
		std::string emissiveMask;

		if ( m_useAutoIllumination && !m_surfaceAutoIlluminationAmount.empty() )
		{
			emissiveMask = "clamp(" + m_surfaceAutoIlluminationAmount + ", 0.0, 1.0)";
		}
		else
		{
			emissiveMask = "0.0";
		}

		/* Encode nibble-packed vec4:
		 * R = (reflectivity << 4 | reserved) / 255
		 * G = (aoResponse << 4 | shadowResponse) / 255	— shadowResponse=15 (full)
		 * B = (bloomContrib << 4 | emissiveMask) / 255	 — bloomContrib=15 (full)
		 * A = (fogResponse << 4 | dofMask) / 255		   — both 15 (full) */
		return "vec4("
			"float(uint(" + reflectivity + " * 15.0) << 4u) / 255.0, "
			"float((uint(" + aoResponse + " * 15.0) << 4u) | 15u) / 255.0, "
			"float((15u << 4u) | uint(" + emissiveMask + " * 15.0)) / 255.0, "
			"1.0)";
	}

	std::string
	LightGenerator::lightPositionWorldSpace () const noexcept
	{
		return LightUB(UniformBlock::Component::PositionWorldSpace);
	}

	std::string
	LightGenerator::lightDirectionWorldSpace () const noexcept
	{
		return LightUB(UniformBlock::Component::DirectionWorldSpace);
	}

	std::string
	LightGenerator::ambientLightColor () const noexcept
	{
		return ViewUB(UniformBlock::Component::AmbientLightColor, false);
	}

	std::string
	LightGenerator::ambientLightIntensity () const noexcept
	{
		return ViewUB(UniformBlock::Component::AmbientLightIntensity, false);
	}

	std::string
	LightGenerator::lightIntensity () const noexcept
	{
		return LightUB(UniformBlock::Component::Intensity);
	}

	std::string
	LightGenerator::lightRadius () const noexcept
	{
		return LightUB(UniformBlock::Component::Radius);
	}

	std::string
	LightGenerator::lightInnerCosAngle () const noexcept
	{
		return LightUB(UniformBlock::Component::InnerCosAngle);
	}

	std::string
	LightGenerator::lightOuterCosAngle () const noexcept
	{
		return LightUB(UniformBlock::Component::OuterCosAngle);
	}

	std::string
	LightGenerator::lightColor () const noexcept
	{
		return LightUB(UniformBlock::Component::Color);
	}

	std::string
	LightGenerator::variable (const char * componentName) noexcept
	{
		std::stringstream output;
		output << ShaderVariable::Light << '.' << componentName;

		return output.str();
	}

	Declaration::UniformBlock
	LightGenerator::getUniformBlock (uint32_t set, uint32_t binding, LightType lightType, bool useShadowMap, bool useColorProjection) noexcept
	{
		switch ( lightType )
		{
			case LightType::Directional :
			{
				Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::DirectionalLight, UniformBlock::Light};
				block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::Color);
				block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::DirectionWorldSpace);
				block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Intensity);

				if ( useColorProjection )
				{
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ColorProjectionIndex);
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ColorProjectionBoost);
				}

				if ( useShadowMap || useColorProjection )
				{
					block.addMember(Declaration::VariableType::Matrix4, UniformBlock::Component::ViewProjectionMatrix);
				}

				if ( useShadowMap || useColorProjection )
				{
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::PCFRadius);
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ShadowBias);
				}

				return block;
			}

			case LightType::Point :
			{
				Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::PointLight, UniformBlock::Light};
				block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::Color);
				block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::PositionWorldSpace);
				block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Intensity);
				block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Radius);

				if ( useShadowMap || useColorProjection )
				{
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::PCFRadius);
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ShadowBias);
				}

				if ( useColorProjection )
				{
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ColorProjectionIndex);
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ColorProjectionFrameIndex);
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ColorProjectionBoost);
				}

				return block;
			}

			case LightType::Spot :
			{
				Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::SpotLight, UniformBlock::Light};
				block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::Color);
				block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::PositionWorldSpace);
				block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::DirectionWorldSpace);
				block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Intensity);
				block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Radius);
				block.addMember(Declaration::VariableType::Float, UniformBlock::Component::InnerCosAngle);
				block.addMember(Declaration::VariableType::Float, UniformBlock::Component::OuterCosAngle);

				if ( useShadowMap || useColorProjection )
				{
					block.addMember(Declaration::VariableType::Matrix4, UniformBlock::Component::ViewProjectionMatrix);
				}

				if ( useShadowMap || useColorProjection )
				{
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::PCFRadius);
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ShadowBias);
				}

				if ( useColorProjection )
				{
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ColorProjectionIndex);
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ColorProjectionBoost);
				}

				return block;
			}

			default:
				/* TODO: Fix this! */
				return {0, 0, Declaration::MemoryLayout::Std140, nullptr, nullptr};
		}
	}

	Declaration::UniformBlock
	LightGenerator::getUniformBlockCSM (uint32_t set, uint32_t binding, uint32_t cascadeCount) noexcept
	{
		/*
		 * CSM UBO Layout (std140):
		 * mat4[4] cascadeViewProjectionMatrices (256 bytes)
		 * vec4 cascadeSplitDistances (16 bytes)
		 * vec4 (cascadeCount, shadowBias, reserved, reserved) (16 bytes)
		 * vec4 color (16 bytes)
		 * vec4 directionWorldSpace (16 bytes)
		 * float intensity (4 bytes + padding to 16)
		 */
		Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::DirectionalLightCSM, UniformBlock::Light};

		/* Array of cascade view-projection matrices. */
		block.addArrayMember(Declaration::VariableType::Matrix4, UniformBlock::Component::CascadeViewProjectionMatrices, cascadeCount);

		/* Cascade split distances (view-space depths where cascades transition). */
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::CascadeSplitDistances);

		/* Cascade count and shadow bias packed into a vec4. */
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::CascadeCount);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ShadowBias);

		/* Standard directional light properties. */
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::Color);
		block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::DirectionWorldSpace);
		block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Intensity);

		return block;
	}

	bool
	LightGenerator::generateVertexShaderCode (Generator::Abstract & generator, VertexShader & vertexShader) const noexcept
	{
		const auto lightSetIndex = generator.shaderProgram()->setIndex(SetType::PerLight);

		auto lightType = LightType::Directional;
		bool enableShadowMap = false;
		bool enableColorProjection = false;

		switch ( m_renderPassType )
		{
			case RenderPassType::AmbientPass :
				/* Request ViewTBNMatrix for the MRT normals output when normal mapping is active.
				 * Post-process effects (RTR, SSR, SSAO, RTAO) need the perturbed normal in view space. */
				if ( m_useNormalMapping )
				{
					if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::ViewTBNMatrix, VariableScope::ToNextStage) )
					{
						Tracer::error(ClassId, "Unable to synthesize ViewTBNMatrix for ambient pass !");

						return false;
					}
				}

				/* World-space normal for the IBL diffuse irradiance lookup (see
				 * generateAmbientFragmentShader). Only when the program carries the
				 * bindless set — the legacy scalar ambient needs no normal. */
				if ( generator.bindlessTexturesEnabled() )
				{
					if ( !vertexShader.requestSynthesizeInstruction(ShaderVariable::NormalWorldSpace, VariableScope::ToNextStage) )
					{
						Tracer::error(ClassId, "Unable to synthesize NormalWorldSpace for the ambient pass IBL !");

						return false;
					}
				}

				return true;

			case RenderPassType::DirectionalLightPassFullCSM :
				enableColorProjection = true;
				[[fallthrough]];
			case RenderPassType::DirectionalLightPassCSM :
				enableShadowMap = true;
				lightType = LightType::Directional;
				break;

			case RenderPassType::DirectionalLightPassFull :
				enableColorProjection = true;
				[[fallthrough]];
			case RenderPassType::DirectionalLightPassShadowMap :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::DirectionalLightPassColorMap :
				if ( !enableShadowMap ) { enableColorProjection = true; }
				[[fallthrough]];
			case RenderPassType::DirectionalLightPass :
				lightType = LightType::Directional;
				break;

			case RenderPassType::PointLightPassFull :
				enableColorProjection = true;
				[[fallthrough]];
			case RenderPassType::PointLightPassShadowMap :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::PointLightPassColorMap :
				if ( !enableShadowMap ) { enableColorProjection = true; }
				[[fallthrough]];
			case RenderPassType::PointLightPass :
				lightType = LightType::Point;
				break;

			case RenderPassType::SpotLightPassFull :
				enableColorProjection = true;
				[[fallthrough]];
			case RenderPassType::SpotLightPassShadowMap :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::SpotLightPassColorMap :
				if ( !enableShadowMap ) { enableColorProjection = true; }
				[[fallthrough]];
			case RenderPassType::SpotLightPass :
				lightType = LightType::Spot;
				break;

			case RenderPassType::None :
			case RenderPassType::SimplePass :
			default:

				Tracer::error(ClassId, "Calling the light code generation render pass set to 'None' !");
				return false;
		}

		/* CSM uses a specialized uniform block. */
		const bool useCSM = renderPassUsesCSM(m_renderPassType);

		if ( useCSM )
		{
			if ( !vertexShader.declare(LightGenerator::getUniformBlockCSM(lightSetIndex, 0)) )
			{
				return false;
			}
		}
		else if ( !vertexShader.declare(LightGenerator::getUniformBlock(lightSetIndex, 0, lightType, enableShadowMap, enableColorProjection)) )
		{
			return false;
		}


		if ( generator.highQualityEnabled() )
		{
			/* PBR mode uses Cook-Torrance BRDF. */
			if ( m_usePBRMode )
			{
				return this->generatePBRVertexShader(generator, vertexShader, lightType, enableShadowMap, enableColorProjection);
			}

			if ( m_useNormalMapping )
			{
				return this->generatePhongBlinnWithNormalMapVertexShader(generator, vertexShader, lightType, enableShadowMap, enableColorProjection);
			}

			return this->generatePhongBlinnVertexShader(generator, vertexShader, lightType, enableShadowMap, enableColorProjection);
		}

		return this->generateGouraudVertexShader(generator, vertexShader, lightType, enableShadowMap, enableColorProjection);
	}

	bool
	LightGenerator::generateFragmentShaderCode (Generator::Abstract & generator, FragmentShader & fragmentShader) const noexcept
	{
		const auto lightSetIndex = generator.shaderProgram()->setIndex(SetType::PerLight);

		auto lightType = LightType::Directional;
		bool enableShadowMap = false;
		bool enableColorProjection = false;

		/* Declare the perturbed normal in view space ("N") for the MRT normals output.
		 * Post-process effects (RTR, SSR, SSAO, RTAO, RTGI) need it, and
		 * finalNormalViewSpaceExpression() returns "N" whenever normal mapping is active.
		 * SceneRendering writes that output for the AmbientPass, which never reaches a
		 * light-pass generator (it returns after the ambient shader), so no generator
		 * declares "N" for it — we must declare it here. The surface normal vector is
		 * declared earlier by the material code (Location::Top), so this Location::Main
		 * statement always sees it. */
		if ( m_renderPassType == RenderPassType::AmbientPass && m_useNormalMapping && !m_surfaceNormalVector.empty() )
		{
			Code{fragmentShader} << "const vec3 N = normalize(transpose(" << ShaderVariable::ViewTBNMatrix << ") * " << m_surfaceNormalVector << ");";
		}

		switch ( m_renderPassType )
		{
			case RenderPassType::AmbientPass :
			{
				if ( m_fragmentColor.empty() )
				{
					TraceError{ClassId} << "There is no name for the fragment color output !";

					return false;
				}

				Code{fragmentShader, Location::Top} << "vec4 " << m_fragmentColor << " = vec4(0.0, 0.0, 0.0, 1.0);";

				/* Note: the perturbed view-space normal "N" needed by the MRT normals output
				 * is declared once at the top of this function. */

				this->generateAmbientFragmentShader(generator, fragmentShader);

				if ( m_useOpacity )
				{
					Code{fragmentShader, Location::Output} << m_fragmentColor << ".a = " << m_surfaceOpacityAmount << ';';
				}
				/*else
				{
					Code{fragmentShader, Location::Output} << m_fragmentColor << ".a = 1.0;";
				}*/
			}
				return true;

			case RenderPassType::DirectionalLightPassFullCSM :
				enableColorProjection = true;
				[[fallthrough]];
			case RenderPassType::DirectionalLightPassCSM :
				enableShadowMap = true;
				lightType = LightType::Directional;
				break;

			case RenderPassType::DirectionalLightPassFull :
				enableColorProjection = true;
				[[fallthrough]];
			case RenderPassType::DirectionalLightPassShadowMap :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::DirectionalLightPassColorMap :
				if ( !enableShadowMap ) { enableColorProjection = true; }
				[[fallthrough]];
			case RenderPassType::DirectionalLightPass :
				lightType = LightType::Directional;
				break;

			case RenderPassType::PointLightPassFull :
				enableColorProjection = true;
				[[fallthrough]];
			case RenderPassType::PointLightPassShadowMap :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::PointLightPassColorMap :
				if ( !enableShadowMap ) { enableColorProjection = true; }
				[[fallthrough]];
			case RenderPassType::PointLightPass :
				lightType = LightType::Point;
				break;

			case RenderPassType::SpotLightPassFull :
				enableColorProjection = true;
				[[fallthrough]];
			case RenderPassType::SpotLightPassShadowMap :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::SpotLightPassColorMap :
				if ( !enableShadowMap ) { enableColorProjection = true; }
				[[fallthrough]];
			case RenderPassType::SpotLightPass :
				lightType = LightType::Spot;
				break;

			default :
				Tracer::error(ClassId, "Invalid render pass for lighting !");

				return false;
		}

		/* CSM uses a specialized uniform block. */
		const bool useCSM = renderPassUsesCSM(m_renderPassType);

		if ( useCSM )
		{
			if ( !fragmentShader.declare(LightGenerator::getUniformBlockCSM(lightSetIndex, 0)) )
			{
				return false;
			}
		}
		else if ( !fragmentShader.declare(LightGenerator::getUniformBlock(lightSetIndex, 0, lightType, enableShadowMap, enableColorProjection)) )
		{
			return false;
		}


		if ( generator.highQualityEnabled() )
		{
			/* PBR mode uses Cook-Torrance BRDF. */
			if ( m_usePBRMode )
			{
				return this->generatePBRFragmentShader(generator, fragmentShader, lightType, enableShadowMap, enableColorProjection);
			}

			if ( m_useNormalMapping )
			{
				return this->generatePhongBlinnWithNormalMapFragmentShader(generator, fragmentShader, lightType, enableShadowMap, enableColorProjection);
			}

			return this->generatePhongBlinnFragmentShader(generator, fragmentShader, lightType, enableShadowMap, enableColorProjection);
		}

		return this->generateGouraudFragmentShader(generator, fragmentShader, lightType, enableShadowMap, enableColorProjection);
	}

	void
	LightGenerator::generateAmbientFragmentShader (Generator::Abstract & generator, FragmentShader & fragmentShader) const noexcept
	{
		using Graphics::BindlessTextureManager;

		/* Declare evalIridescence function if needed for ambient/IBL pass. */
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

		std::string surfaceColor{};
		std::string intensity{};

		/* The raw base color (albedo/diffuse, WITHOUT the Lambert 1/pi) — the IBL diffuse
		 * irradiance term needs it as-is, since the irradiance cubemap stores E/pi. */
		const auto & iblBaseColor = m_usePBRMode && !m_surfaceAlbedo.empty() ? m_surfaceAlbedo : m_surfaceDiffuseColor;

		if ( m_surfaceAmbientColor.empty() )
		{
			/* PHOTOMETRIC AMBIENT. The ambient intensity is an ILLUMINANCE in lux (the sky and
			 * bounce light reaching the surface), so the outgoing luminance of a Lambertian
			 * surface is `albedo * E / pi` — hence the 1/pi normalization here, not an arbitrary
			 * scale. It used to be a hard-coded 0.05 ("5% of the albedo"), which was a purely
			 * artistic factor and made the ambient term incomparable with a light in candela.
			 * In PBR mode, use albedo instead of diffuse. */
			surfaceColor = (std::stringstream{} << "(" << iblBaseColor << " * 0.3183098862)").str();
		}
		else
		{
			surfaceColor = m_surfaceAmbientColor;
		}

		if ( m_enableAmbientNoise )
		{
			Declaration::Function random{"random", GLSL::Float};
			random.addInParameter(GLSL::FloatVector2, "st");
			Code{random, Location::Output} << "return fract(sin(dot(st, vec2(12.9898, 78.233))) * 43758.5453123);";

			if ( !fragmentShader.declare(random) )
			{
				return;
			}

			intensity = (std::stringstream{} << "(" << this->ambientLightIntensity() << " * random(gl_FragCoord.xy))").str();
		}
		else
		{
			intensity = this->ambientLightIntensity();
		}

		/* IBL AMBIENT (diffuse irradiance + split-sum specular). Available when the program
		 * carries the bindless set: reserved cube slot 1 holds the scene's baked irradiance
		 * (E/pi, parked on the default BLACK cubemap when the scene does not derive its
		 * ambient from the sky — the term then contributes nothing), slot 2 the prefiltered
		 * environment, 2D slot 3 the split-sum BRDF LUT. */
		const bool useIBL = generator.bindlessTexturesEnabled();

		if ( useIBL )
		{
			const auto bindlessSetIndex = generator.shaderProgram()->setIndex(SetType::PerBindless);

			fragmentShader.setExtensionBehavior(GLSL::Extension::NonUniformQualifier, GLSL::Extension::Require);

			if ( !fragmentShader.declare(Declaration::Sampler{bindlessSetIndex, BindlessTextureManager::TextureCubeBinding, GLSL::SamplerCube, Bindless::TexturesCube, Declaration::Sampler::UnboundedArray}) )
			{
				return;
			}

			if ( !fragmentShader.declare(Declaration::Sampler{bindlessSetIndex, BindlessTextureManager::Texture2DBinding, GLSL::Sampler2D, Bindless::Textures2D, Declaration::Sampler::UnboundedArray}) )
			{
				return;
			}

			/* The GEOMETRIC world normal is enough for the irradiance lookup: a 32² cosine
			 * convolved cubemap carries no frequency a normal map could reveal.
			 * ENGINE CUBEMAP CONVENTION: world direction D samples at (D.x, -D.y, D.z). */
			Code{fragmentShader} <<
				"const vec3 iblAmbientNormal = normalize(" << ShaderVariable::NormalWorldSpace << ");" << Line::End <<
				"const vec3 iblIrradiance = texture(" << Bindless::TexturesCube << "[" << BindlessTextureManager::IrradianceCubemapSlot << "], vec3(iblAmbientNormal.x, -iblAmbientNormal.y, iblAmbientNormal.z)).rgb;";
		}

		/* The tint of the IBL diffuse irradiance term is ALWAYS the raw base color
		 * (albedo/diffuse): the irradiance is a physical light lighting the Lambertian
		 * reflectance. The Phong ambient component (m_surfaceAmbientColor) is an artistic
		 * constant-ambient hack — it stays on the legacy scalar path only (a light-grey
		 * ka under a 17k lx sky would wash every material out). And NEVER the 1/pi-scaled
		 * photometric surfaceColor — the irradiance cubemap already stores E/pi. */
		const auto & iblDiffuseTint = iblBaseColor;

		/* Baked ambient occlusion factor: it modulates the DIFFUSE ambient terms only —
		 * occluding the specular IBL or the emission with the same cavity term is wrong
		 * (the old global multiply did, and also darkened the emissive). */
		std::string aoFactor{};

		if ( m_useAmbientOcclusion && !m_surfaceAmbientOcclusion.empty() )
		{
			const auto aoIntensity = m_surfaceAOIntensity.empty() ? "1.0" : m_surfaceAOIntensity;

			Code{fragmentShader} << "const float iblAmbientAO = mix(1.0, " << m_surfaceAmbientOcclusion << ", " << aoIntensity << ");";

			aoFactor = " * iblAmbientAO";
		}

		if ( m_usePBRMode && m_useReflection && m_useRefraction && this->highQualityEnabled() )
		{
			/* NOTE: PBR Glass/transparent materials with both reflection and refraction.
			 * The Fresnel effect determines the blend between reflection and refraction.
			 * IBL is the main contribution for glass - it shows the environment, not ambient light.
			 * IBLIntensity allows dynamic control over the cubemap contribution.
			 * Requires high-quality mode for reflectionNormal and reflectionI variables. */
			/* NOTE: The reflected/refracted legs are pre-scaled at their definition by
			 * reflectionIntensity()/refractionIntensity(): a normalized cubemap source gets the
			 * environment luminance, a render-target source (probe/mirror) is already an
			 * absolute luminance and only takes the artistic weight. */
			const auto code = (std::stringstream{} <<
				"/* PBR Glass IBL - Fresnel-Schlick approximation. */" "\n"
				"const float NdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n"
				"const float fresnelFactor = 0.04 + (1.0 - 0.04) * pow(1.0 - NdotV, 5.0);" "\n"
				"const vec3 reflectedColor = " << m_surfaceReflectionColor << ".rgb * " << m_surfaceReflectionAmount << " * " << this->reflectionIntensity() << ";" "\n"
				"vec3 refractedColor = " << m_surfaceRefractionColor << ".rgb * " << m_surfaceRefractionAmount << " * " << this->refractionIntensity() << ";" "\n").str();

			Code{fragmentShader, Location::Output} << code;

			/* Transmission with refraction - apply Beer's law absorption to refracted color. */
			if ( m_useTransmission )
			{
				const auto transmissionCode = (std::stringstream{} <<
					"/* Beer's law absorption for colored glass transmission. */" "\n"
					"const vec3 beerAbsorption = exp(log(max(" << m_surfaceAttenuationColor << ".rgb, vec3(0.001))) / max(" << m_surfaceAttenuationDistance << ", 0.0001) * " << m_surfaceThicknessFactor << ");" "\n"
					"refractedColor *= beerAbsorption * " << m_surfaceTransmissionFactor << " + (1.0 - " << m_surfaceTransmissionFactor << ");").str();
				Code{fragmentShader, Location::Output} << transmissionCode;
			}

			{
				const auto blendCode = (std::stringstream{} <<
					"/* Blend reflection and refraction based on Fresnel, modulated by IBL intensity. */" "\n" <<
					m_fragmentColor << ".rgb += mix(refractedColor, reflectedColor, fresnelFactor);").str();
				Code{fragmentShader, Location::Output} << blendCode;
			}

			/* Clear coat IBL - energy conservation + coat reflection (HQ). */
			if ( m_useClearCoat )
			{
				const auto ccCode = (std::stringstream{} <<
					"/* Clear coat IBL - energy conservation + coat reflection. */" "\n"
					"const float ccFactor = " << m_surfaceClearCoatFactor << ";" "\n"
					"const float ccNdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n"
					"const vec3 ccFresnel = vec3(0.04) + (vec3(1.0) - vec3(0.04)) * pow(1.0 - ccNdotV, 5.0);" "\n" <<
					m_fragmentColor << ".rgb *= (vec3(1.0) - ccFactor * ccFresnel);" "\n" <<
					m_fragmentColor << ".rgb += reflectedColor * ccFactor * ccFresnel;").str();
				Code{fragmentShader, Location::Output} << ccCode;
			}
		}
		else if ( m_usePBRMode && m_useReflection && m_useTransmission && !m_useRefraction && this->highQualityEnabled() )
		{
			/* NOTE: PBR Reflection + Transmission (glass-like dielectric).
			 * Energy conservation: Fresnel with dielectric F0=0.04 splits light between
			 * reflection (F) and transmission (1-F). This ensures the two effects
			 * share the same energy budget rather than being additive.
			 * Beer's law absorption colors the transmitted light.
			 * Requires high-quality mode for reflectionNormal and reflectionI variables. */
			/* Scaled by the environment luminance: the cubemap is a normalized [0,1] source, so
			 * without this the reflections contribute a fraction of a nit to a scene lit in
			 * thousands. The surface's own IBLIntensity stays the artistic weight. */
			const auto iblIntensity = this->scaledIBLIntensity();

			/* ⚠️ The transmitted light does NOT always share the reflection's unit. The reflection is
			 * a cubemap texel in [0,1] and genuinely needs the sky luminance to become a luminance.
			 * A GRAB-PASS transmission is a copy of the rendered scene, already in nits: scaling it
			 * too multiplied the scene by the sky luminance a SECOND time, which is what turned the
			 * shallow water — where Beer's law absorbs almost nothing, so the sunlit sand comes
			 * through undimmed — into a solid white blowout. */
			const auto transmissionScale = m_transmissionIsSceneRadiance
				? std::string{}
				: " * " + iblIntensity;

			const auto code = (std::stringstream{} <<
				"/* PBR Reflection + Transmission - energy-conserving Fresnel blend. */" "\n"
				"const float NdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n" <<
				(m_useMaterialIOR
					? (m_useKHRSpecular
						? "/* Dielectric F0 from material IOR + KHR_materials_specular. */\n"
						  "const float materialF0raw = pow((" + m_surfaceMaterialIOR + " - 1.0) / (" + m_surfaceMaterialIOR + " + 1.0), 2.0);\n"
						  "const float materialF0 = min(materialF0raw * " + m_surfaceKHRSpecularFactor + ", 1.0);\n"
						  "const float fresnelDielectric = materialF0 + (1.0 - materialF0) * pow(1.0 - NdotV, 5.0);\n"
						: "/* Dielectric F0 from material IOR (KHR_materials_ior). */\n"
						  "const float materialF0 = pow((" + m_surfaceMaterialIOR + " - 1.0) / (" + m_surfaceMaterialIOR + " + 1.0), 2.0);\n"
						  "const float fresnelDielectric = materialF0 + (1.0 - materialF0) * pow(1.0 - NdotV, 5.0);\n")
					: "/* Dielectric F0=0.04 for accurate glass Fresnel split. */\n"
					  "const float fresnelDielectric = 0.04 + 0.96 * pow(1.0 - NdotV, 5.0);\n") <<
				"const vec3 reflectedColor = " << m_surfaceReflectionColor << ".rgb * " << m_surfaceReflectionAmount << " * " << this->reflectionIntensity() << ";" "\n"
				"/* Beer's law absorption for transmission. */" "\n"
				"const vec3 transAbsorption = exp(log(max(" << m_surfaceAttenuationColor << ".rgb, vec3(0.001))) / max(" << m_surfaceAttenuationDistance << ", 0.0001) * " << m_surfaceThicknessFactor << ");" "\n"
				"const vec3 transmittedLight = " << m_surfaceTransmissionColor << " * transAbsorption;" "\n"
				"/* F = reflection, (1-F)*transmissionFactor = transmission. */" "\n" <<
				m_fragmentColor << ".rgb += reflectedColor * fresnelDielectric;" "\n" <<
				m_fragmentColor << ".rgb += transmittedLight * " << m_surfaceTransmissionFactor << " * (1.0 - fresnelDielectric)" << transmissionScale << ";").str();

			Code{fragmentShader, Location::Output} << code;

			/* Clear coat IBL on transmissive glass. */
			if ( m_useClearCoat )
			{
				const auto ccCode = (std::stringstream{} <<
					"/* Clear coat IBL - energy conservation + coat reflection. */" "\n"
					"const float ccFactor = " << m_surfaceClearCoatFactor << ";" "\n"
					"const float ccNdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n"
					"const vec3 ccFresnel = vec3(0.04) + (vec3(1.0) - vec3(0.04)) * pow(1.0 - ccNdotV, 5.0);" "\n" <<
					m_fragmentColor << ".rgb *= (vec3(1.0) - ccFactor * ccFresnel);" "\n" <<
					m_fragmentColor << ".rgb += reflectedColor * ccFactor * ccFresnel;").str();
				Code{fragmentShader, Location::Output} << ccCode;
			}
		}
		else if ( m_usePBRMode && m_useReflection && this->highQualityEnabled() )
		{
			/* NOTE: PBR Metal/reflective materials (no transmission).
			 * IBL is modulated by Fresnel (with proper F0 based on metalness) and IBLIntensity.
			 * For metals (metalness=1), F0 = albedo color, giving strong colored reflections.
			 * For dielectrics (metalness=0), F0 from IOR when available, otherwise 0.5 (boosted for visibility).
			 * Requires high-quality mode for reflectionNormal and reflectionI variables. */
			/* Scaled by the environment luminance: the cubemap is a normalized [0,1] source, so
			 * without this the reflections contribute a fraction of a nit to a scene lit in
			 * thousands. The surface's own IBLIntensity stays the artistic weight. */
			const auto iblIntensity = this->scaledIBLIntensity();
			const auto albedo = m_surfaceAlbedo.empty() ? "vec3(1.0)" : m_surfaceAlbedo + ".rgb";
			const auto metalness = m_surfaceMetalness.empty() ? "0.0" : m_surfaceMetalness;

			/* Compute the dielectric F0 expression for IBL. */
			std::string iblF0Computation;
			if ( m_useMaterialIOR && m_useKHRSpecular )
			{
				iblF0Computation = (std::stringstream{} <<
					"const float iblDielectricF0 = pow((" << m_surfaceMaterialIOR << " - 1.0) / (" << m_surfaceMaterialIOR << " + 1.0), 2.0);" "\n"
					"const vec3 iblF0 = mix(min(vec3(iblDielectricF0) * " << m_surfaceKHRSpecularColor << ".rgb * " << m_surfaceKHRSpecularFactor << ", vec3(1.0)), " << albedo << ", " << metalness << ");").str();
			}
			else if ( m_useMaterialIOR )
			{
				iblF0Computation = (std::stringstream{} <<
					"const float iblDielectricF0 = pow((" << m_surfaceMaterialIOR << " - 1.0) / (" << m_surfaceMaterialIOR << " + 1.0), 2.0);" "\n"
					"const vec3 iblF0 = mix(vec3(iblDielectricF0), " << albedo << ", " << metalness << ");").str();
			}
			else
			{
				iblF0Computation = (std::stringstream{} <<
					"const vec3 iblF0 = mix(vec3(0.5), " << albedo << ", " << metalness << ");").str();
			}

			if ( m_useIridescence )
			{
				const auto code = (std::stringstream{} <<
					"/* PBR IBL - Fresnel-Schlick with iridescence. */" "\n" <<
					iblF0Computation << "\n"
					"const float NdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n"
					"const vec3 fresnelIBL_base = iblF0 + (1.0 - iblF0) * pow(1.0 - NdotV, 5.0);" "\n"
					"const float iridescenceThickness = mix(" << m_surfaceIridescenceThicknessMin << ", " << m_surfaceIridescenceThicknessMax << ", 0.5);" "\n"
					"const vec3 fresnelIBL_iridescence = evalIridescence(1.0, " << m_surfaceIridescenceIOR << ", NdotV, iridescenceThickness, iblF0);" "\n"
					"const vec3 fresnelIBL = mix(fresnelIBL_base, fresnelIBL_iridescence, " << m_surfaceIridescenceFactor << ");" "\n"
					"const vec3 reflectedColor = " << m_surfaceReflectionColor << ".rgb * " << m_surfaceReflectionAmount << " * " << this->reflectionIntensity() << ";" "\n"
					"/* IBL contribution modulated by Fresnel and IBL intensity. */" "\n" <<
					m_fragmentColor << ".rgb += reflectedColor * fresnelIBL;").str();

				Code{fragmentShader, Location::Output} << code;

				/* Diffuse irradiance: what the iridescent Fresnel does not reflect feeds
				 * the Lambertian lobe of the dielectric part. */
				if ( useIBL )
				{
					const auto diffuseCode = (std::stringstream{} <<
						"/* IBL diffuse irradiance (iridescence: energy left by the Fresnel). */" "\n" <<
						m_fragmentColor << ".rgb += " << albedo << " * (1.0 - " << metalness << ") * (vec3(1.0) - fresnelIBL) * iblIrradiance * " << iblIntensity << aoFactor << ";").str();
					Code{fragmentShader, Location::Output} << diffuseCode;
				}
			}
			else if ( useIBL )
			{
				/* Split-sum reconstruction (Karis 2013): the prefiltered radiance times the
				 * BRDF LUT (scale/bias on F0), completed by the Fdez-Agüera 2019 multi-scatter
				 * energy compensation — no extra resource, the same two LUT channels. The
				 * diffuse lobe takes what the specular did not (energy conservation). */
				const auto roughness = m_surfaceRoughness.empty() ? "0.5" : m_surfaceRoughness;

				const auto code = (std::stringstream{} <<
					"/* PBR IBL - split-sum + multi-scatter energy compensation. */" "\n" <<
					iblF0Computation << "\n"
					"const float NdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n"
					"const vec3 reflectedColor = " << m_surfaceReflectionColor << ".rgb * " << m_surfaceReflectionAmount << " * " << this->reflectionIntensity() << ";" "\n"
					"const vec2 iblEnvBRDF = texture(" << Bindless::Textures2D << "[" << Graphics::BindlessTextureManager::BRDFLutSlot << "], vec2(NdotV, clamp(" << roughness << ", 0.0, 1.0))).rg;" "\n"
					"const vec3 iblFssEss = iblF0 * iblEnvBRDF.x + iblEnvBRDF.y;" "\n"
					"const float iblEms = 1.0 - (iblEnvBRDF.x + iblEnvBRDF.y);" "\n"
					"const vec3 iblFavg = iblF0 + (vec3(1.0) - iblF0) / 21.0;" "\n"
					"const vec3 iblFmsEms = iblEms * iblFssEss * iblFavg / (vec3(1.0) - iblFavg * iblEms);" "\n"
					"const vec3 iblKD = " << albedo << " * (1.0 - " << metalness << ") * max(vec3(1.0) - iblFssEss - iblFmsEms, vec3(0.0));" "\n" <<
					m_fragmentColor << ".rgb += iblFssEss * reflectedColor;" "\n" <<
					m_fragmentColor << ".rgb += (iblFmsEms + iblKD" << aoFactor << ") * iblIrradiance * " << iblIntensity << ";").str();

				Code{fragmentShader, Location::Output} << code;
			}
			else
			{
				const auto code = (std::stringstream{} <<
					"/* PBR IBL - Fresnel-Schlick with proper F0 for metals. */" "\n" <<
					iblF0Computation << "\n"
					"const float NdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n"
					"const vec3 fresnelIBL = iblF0 + (1.0 - iblF0) * pow(1.0 - NdotV, 5.0);" "\n"
					"const vec3 reflectedColor = " << m_surfaceReflectionColor << ".rgb * " << m_surfaceReflectionAmount << " * " << this->reflectionIntensity() << ";" "\n"
					"/* IBL contribution modulated by Fresnel and IBL intensity. */" "\n" <<
					m_fragmentColor << ".rgb += reflectedColor * fresnelIBL;").str();

				Code{fragmentShader, Location::Output} << code;
			}

			/* Clear coat IBL - energy conservation + coat reflection (HQ). */
			if ( m_useClearCoat )
			{
				const auto ccCode = (std::stringstream{} <<
					"/* Clear coat IBL - energy conservation + coat reflection. */" "\n"
					"const float ccFactor = " << m_surfaceClearCoatFactor << ";" "\n"
					"const float ccNdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n"
					"const vec3 ccFresnel = vec3(0.04) + (vec3(1.0) - vec3(0.04)) * pow(1.0 - ccNdotV, 5.0);" "\n" <<
					m_fragmentColor << ".rgb *= (vec3(1.0) - ccFactor * ccFresnel);" "\n" <<
					m_fragmentColor << ".rgb += reflectedColor * ccFactor * ccFresnel;").str();
				Code{fragmentShader, Location::Output} << ccCode;
			}
		}
		else if ( m_usePBRMode && m_useReflection )
		{
			/* NOTE: PBR low-quality fallback - simplified IBL without per-fragment Fresnel.
			 * When high-quality reflection is disabled, reflectionNormal and reflectionI
			 * are not available. We approximate F0 using metalness:
			 * - Dielectrics (metalness=0): F0 from IOR+specular when available, else LowQualityDielectricF0
			 * - Metals (metalness=1): F0 = albedo (colored reflections) */
			/* Scaled by the environment luminance: the cubemap is a normalized [0,1] source, so
			 * without this the reflections contribute a fraction of a nit to a scene lit in
			 * thousands. The surface's own IBLIntensity stays the artistic weight. */
			const auto iblIntensity = this->scaledIBLIntensity();
			const auto albedo = m_surfaceAlbedo.empty() ? "vec3(1.0)" : m_surfaceAlbedo + ".rgb";
			const auto metalness = m_surfaceMetalness.empty() ? "0.0" : m_surfaceMetalness;

			std::string lqF0Code;
			if ( m_useMaterialIOR && m_useKHRSpecular )
			{
				lqF0Code = (std::stringstream{} <<
					"const float lqDielectricF0 = pow((" << m_surfaceMaterialIOR << " - 1.0) / (" << m_surfaceMaterialIOR << " + 1.0), 2.0);" "\n"
					"const vec3 lqF0 = mix(min(vec3(lqDielectricF0) * " << m_surfaceKHRSpecularColor << ".rgb * " << m_surfaceKHRSpecularFactor << ", vec3(1.0)), " << albedo << ", " << metalness << ");").str();
			}
			else if ( m_useMaterialIOR )
			{
				lqF0Code = (std::stringstream{} <<
					"const float lqDielectricF0 = pow((" << m_surfaceMaterialIOR << " - 1.0) / (" << m_surfaceMaterialIOR << " + 1.0), 2.0);" "\n"
					"const vec3 lqF0 = mix(vec3(lqDielectricF0), " << albedo << ", " << metalness << ");").str();
			}
			else
			{
				lqF0Code = (std::stringstream{} <<
					"const vec3 lqF0 = mix(vec3(" << LowQualityDielectricF0 << "), " << albedo << ", " << metalness << ");").str();
			}

			const auto code = (std::stringstream{} <<
				"/* Low-quality PBR IBL - F0 approximation without Fresnel. */" "\n" <<
				lqF0Code << "\n" <<
				m_fragmentColor << ".rgb += " << m_surfaceReflectionColor << ".rgb * lqF0 * " << m_surfaceReflectionAmount << " * " << this->reflectionIntensity() << ";").str();
			Code{fragmentShader, Location::Output} << code;

			/* Diffuse irradiance (LQ: plain energy split, no LUT). */
			if ( useIBL )
			{
				const auto diffuseCode = (std::stringstream{} <<
					"/* IBL diffuse irradiance (LQ). */" "\n" <<
					m_fragmentColor << ".rgb += " << albedo << " * (1.0 - " << metalness << ") * (vec3(1.0) - lqF0) * iblIrradiance * " << iblIntensity << aoFactor << ";").str();
				Code{fragmentShader, Location::Output} << diffuseCode;
			}

			/* Clear coat IBL - simplified constant attenuation (LQ, no reflectionNormal available). */
			if ( m_useClearCoat )
			{
				const auto ccCode = (std::stringstream{} <<
					"/* Clear coat IBL - simplified constant attenuation (LQ). */" "\n"
					"const float ccFactor = " << m_surfaceClearCoatFactor << ";" "\n" <<
					m_fragmentColor << ".rgb *= (1.0 - ccFactor * 0.04);" "\n" <<
					m_fragmentColor << ".rgb += " << m_surfaceReflectionColor << ".rgb * ccFactor * 0.04 * " << m_surfaceReflectionAmount << " * " << this->reflectionIntensity() << ";").str();
				Code{fragmentShader, Location::Output} << ccCode;
			}
		}
		else if ( m_usePBRMode && m_useRefraction )
		{
			/* NOTE: PBR low-quality fallback for refraction-only materials.
			 * Refraction is less affected by F0 - use a subtle blend. */
			/* NOTE: The refracted leg takes refractionIntensity(): environment luminance for a
			 * normalized cubemap source, artistic weight alone for a render-target source. */
			if ( m_useTransmission )
			{
				const auto code = (std::stringstream{} <<
					"/* PBR refraction with Beer's law absorption. */" "\n"
					"const vec3 beerAbsorption = exp(log(max(" << m_surfaceAttenuationColor << ".rgb, vec3(0.001))) / max(" << m_surfaceAttenuationDistance << ", 0.0001) * " << m_surfaceThicknessFactor << ");" "\n"
					"vec3 refrRefractedColor = " << m_surfaceRefractionColor << ".rgb * " << m_surfaceRefractionAmount << " * 0.96;" "\n"
					"refrRefractedColor *= beerAbsorption * " << m_surfaceTransmissionFactor << " + (1.0 - " << m_surfaceTransmissionFactor << ");" "\n" <<
					m_fragmentColor << ".rgb += refrRefractedColor * " << this->refractionIntensity() << ";").str();
				Code{fragmentShader} << code;
			}
			else
			{
				Code{fragmentShader} << m_fragmentColor << ".rgb += " << m_surfaceRefractionColor << ".rgb * " << m_surfaceRefractionAmount << " * 0.96 * " << this->refractionIntensity() << ";";
			}
		}
		else if ( m_useReflection && m_useRefraction )
		{
			/* NOTE: Non-PBR Glass - legacy behavior.
			 * The fresnelFactor variable is already declared by the material (StandardResource).
			 * We just use it here to blend reflection and refraction in the ambient pass. */
			const auto code = (std::stringstream{} <<
				"/* Glass ambient pass - uses fresnelFactor from material. */" "\n"
				"const vec3 ambientReflectedColor = " << m_surfaceReflectionColor << ".rgb * " << m_surfaceReflectionAmount << ";" "\n"
				"const vec3 ambientRefractedColor = " << m_surfaceRefractionColor << ".rgb * " << m_surfaceRefractionAmount << ";" "\n"
				"/* Blend reflection and refraction based on Fresnel, with subtle tint from albedo. */" "\n" <<
				m_fragmentColor << ".rgb += mix(ambientRefractedColor, ambientReflectedColor, fresnelFactor) * " << surfaceColor << ".rgb;").str();

			Code{fragmentShader, Location::Output} << code;
		}
		else if ( m_useReflection )
		{
			Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << surfaceColor << ", " << m_surfaceReflectionColor << ", " << m_surfaceReflectionAmount << ").rgb * (" << this->ambientLightColor() << ".rgb * " << intensity << ")" << aoFactor << ";";

			/* IBL (non-PBR reflective, e.g. Standard): the same diffuse/reflection mix, lit
			 * by the irradiance instead of the scalar — the reflection color is a prefiltered
			 * sample in [0,1], the environment luminance turns both into nits. */
			if ( useIBL )
			{
				if ( m_reflectionSourceAbsolute )
				{
					/* Render-target reflection: already an absolute luminance — only the
					 * sky-derived diffuse leg takes the environment luminance. */
					Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << iblDiffuseTint << ".rgb * iblIrradiance" << aoFactor << " * " << ViewUB(Keys::UniformBlock::Component::EnvironmentLuminance, false) << ", " << m_surfaceReflectionColor << ".rgb, " << m_surfaceReflectionAmount << ");";
				}
				else
				{
					Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << iblDiffuseTint << ".rgb * iblIrradiance" << aoFactor << ", " << m_surfaceReflectionColor << ".rgb, " << m_surfaceReflectionAmount << ") * " << ViewUB(Keys::UniformBlock::Component::EnvironmentLuminance, false) << ";";
				}
			}
		}
		else if ( m_useRefraction )
		{
			Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << surfaceColor << ", " << m_surfaceRefractionColor << ", " << m_surfaceRefractionAmount << ").rgb * (" << this->ambientLightColor() << ".rgb * " << intensity << ")" << aoFactor << ";";

			if ( useIBL )
			{
				if ( m_refractionSourceAbsolute )
				{
					/* Render-target refraction: already an absolute luminance — only the
					 * sky-derived diffuse leg takes the environment luminance. */
					Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << iblDiffuseTint << ".rgb * iblIrradiance" << aoFactor << " * " << ViewUB(Keys::UniformBlock::Component::EnvironmentLuminance, false) << ", " << m_surfaceRefractionColor << ".rgb, " << m_surfaceRefractionAmount << ");";
				}
				else
				{
					Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << iblDiffuseTint << ".rgb * iblIrradiance" << aoFactor << ", " << m_surfaceRefractionColor << ".rgb, " << m_surfaceRefractionAmount << ") * " << ViewUB(Keys::UniformBlock::Component::EnvironmentLuminance, false) << ";";
				}
			}
		}
		else
		{
			Code{fragmentShader} << m_fragmentColor << ".rgb += " << surfaceColor << ".rgb * (" << this->ambientLightColor() << ".rgb * " << intensity << ")" << aoFactor << ";";

			/* IBL diffuse irradiance: the cubemap stores E/pi, so the raw base color
			 * (no 1/pi) times the sample times the environment luminance is the outgoing
			 * luminance of the Lambertian surface — it matches the scalar path exactly on
			 * a uniform sky. The scalar term above is zeroed by the scene when the sky
			 * drives the ambient (see Scene::refreshAmbientLightProperties). */
			if ( useIBL )
			{
				Code{fragmentShader} << m_fragmentColor << ".rgb += " << iblDiffuseTint << ".rgb * iblIrradiance * " << ViewUB(Keys::UniformBlock::Component::EnvironmentLuminance, false) << aoFactor << ";";
			}
		}

		/* Auto-Illumination (emissive) support. */
		if ( !m_surfaceAutoIlluminationAmount.empty() )
		{
			if ( m_useAutoIllumination && !m_surfaceAutoIlluminationColor.empty() )
			{
				/* PBR mode: use explicit emissive color. */
				if ( !m_surfaceEmissiveStrength.empty() )
				{
					Code{fragmentShader} << m_fragmentColor << ".rgb += " << m_surfaceAutoIlluminationColor << ".rgb * " << m_surfaceAutoIlluminationAmount << " * " << m_surfaceEmissiveStrength << ";";
				}
				else
				{
					Code{fragmentShader} << m_fragmentColor << ".rgb += " << m_surfaceAutoIlluminationColor << ".rgb * " << m_surfaceAutoIlluminationAmount << ";";
				}
			}
			else
			{
				/* Legacy/Phong mode: use diffuse color as emissive base. */
				if ( !m_surfaceEmissiveStrength.empty() )
				{
					Code{fragmentShader} << m_fragmentColor << ".rgb += " << m_surfaceDiffuseColor << ".rgb * " << m_surfaceAutoIlluminationAmount << " * " << m_surfaceEmissiveStrength << ";";
				}
				else
				{
					Code{fragmentShader} << m_fragmentColor << ".rgb += " << m_surfaceDiffuseColor << ".rgb * " << m_surfaceAutoIlluminationAmount << ";";
				}
			}
		}

		/* NOTE: The baked ambient occlusion no longer multiplies the whole fragment here —
		 * that darkened the emissive and the specular IBL with a diffuse cavity term. The
		 * `aoFactor` computed at the top of this function now modulates each DIFFUSE
		 * ambient contribution at its addition site. */

		/* SSS ambient - scattered light fills shadow areas. */
		if ( m_useSubsurface )
		{
			const auto albedo = m_surfaceAlbedo.empty() ? "vec3(1.0)" : m_surfaceAlbedo + ".rgb";

			if ( m_useSubsurfaceThicknessMap )
			{
				const auto code = (std::stringstream{} <<
					"/* SSS ambient - scattered light fills shadow areas (with thickness map). */" "\n"
					"const vec3 sssAmbient = " << m_surfaceSubsurfaceColor << ".rgb * " << m_surfaceSubsurfaceIntensity << " * (1.0 - " << m_surfaceSubsurfaceThickness << ");" "\n" <<
					m_fragmentColor << ".rgb += sssAmbient * " << albedo << ";").str();
				Code{fragmentShader} << code;
			}
			else
			{
				const auto code = (std::stringstream{} <<
					"/* SSS ambient - scattered light fills shadow areas. */" "\n"
					"const vec3 sssAmbient = " << m_surfaceSubsurfaceColor << ".rgb * " << m_surfaceSubsurfaceIntensity << " * 0.5;" "\n" <<
					m_fragmentColor << ".rgb += sssAmbient * " << albedo << ";").str();
				Code{fragmentShader} << code;
			}
		}

		/* Sheen ambient - fabric-like materials get a subtle ambient sheen contribution. */
		if ( m_useSheen )
		{
			const auto albedo = m_surfaceAlbedo.empty() ? "vec3(1.0)" : m_surfaceAlbedo + ".rgb";

			const auto code = (std::stringstream{} <<
				"/* Sheen ambient contribution. */" "\n"
				"const vec3 sheenAmbientColor = " << m_surfaceSheenColor << ".rgb;" "\n"
				"const float sheenAmbientRoughness = " << m_surfaceSheenRoughness << ";" "\n"
				"const float sheenAmbientDFG = 0.157 * sheenAmbientRoughness + 0.04;" "\n"
				"const float sheenAmbientScaling = 1.0 - max(max(sheenAmbientColor.r, sheenAmbientColor.g), sheenAmbientColor.b) * sheenAmbientDFG;" "\n" <<
				m_fragmentColor << ".rgb = " << m_fragmentColor << ".rgb * sheenAmbientScaling + sheenAmbientColor * " << albedo << " * 0.1;").str();
			Code{fragmentShader} << code;
		}

		/* Transmission ambient - thin-surface pass-through (no refraction bending).
		 * Only runs when reflection did NOT already handle transmission in the combined branch above.
		 * Uses prefiltered cubemap with LOD for frosted glass effect.
		 * Beer's law provides wavelength-dependent absorption for colored glass.
		 * Gated by inverse Fresnel: reflected light can't also be transmitted. */
		if ( m_useTransmission && !m_useRefraction && !m_useReflection )
		{
			/* Scaled by the environment luminance: the cubemap is a normalized [0,1] source, so
			 * without this the reflections contribute a fraction of a nit to a scene lit in
			 * thousands. The surface's own IBLIntensity stays the artistic weight. */
			const auto iblIntensity = this->scaledIBLIntensity();
			const auto roughness = m_surfaceRoughness.empty() ? "0.5" : m_surfaceRoughness;

			if ( this->highQualityEnabled() )
			{
				/* High-quality: use reflectionNormal and reflectionI for proper Fresnel gating.
				 * NOTE: transmissionDir, transmissionLod, and SurfaceTransmissionColor are already
				 * declared by generateBindlessTransmissionFragmentShader() in PBRResource. */
				const auto code = (std::stringstream{} <<
					"/* Thin-surface transmission - Beer's law + Fresnel gate. */" "\n"
					"vec3 transmittedLight = " << m_surfaceTransmissionColor << ";" "\n"
					"/* Beer's law absorption. */" "\n"
					"const vec3 transAbsorption = exp(log(max(" << m_surfaceAttenuationColor << ".rgb, vec3(0.001))) / max(" << m_surfaceAttenuationDistance << ", 0.0001) * " << m_surfaceThicknessFactor << ");" "\n"
					"transmittedLight *= transAbsorption;" "\n"
					"/* Fresnel gate: reflected light can't be transmitted. */" "\n"
					"const float transNdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n"
					"const float fresnelT = 0.04 + 0.96 * pow(1.0 - transNdotV, 5.0);" "\n" <<
					m_fragmentColor << ".rgb += transmittedLight * " << m_surfaceTransmissionFactor << " * (1.0 - fresnelT) * " << iblIntensity << ";").str();
				Code{fragmentShader} << code;
			}
			else
			{
				/* Low-quality: no Fresnel gating, simpler approximation.
				 * NOTE: SurfaceTransmissionColor is already declared by PBRResource. */
				const auto code = (std::stringstream{} <<
					"/* Thin-surface transmission (LQ) - Beer's law absorption. */" "\n"
					"vec3 transmittedLight = " << m_surfaceTransmissionColor << ";" "\n"
					"const vec3 transAbsorption = exp(log(max(" << m_surfaceAttenuationColor << ".rgb, vec3(0.001))) / max(" << m_surfaceAttenuationDistance << ", 0.0001) * " << m_surfaceThicknessFactor << ");" "\n"
					"transmittedLight *= transAbsorption;" "\n" <<
					m_fragmentColor << ".rgb += transmittedLight * " << m_surfaceTransmissionFactor << " * 0.96 * " << iblIntensity << ";").str();
				Code{fragmentShader} << code;
			}
		}
	}

	bool
	LightGenerator::generateFinalFragmentOutput (FragmentShader & fragmentShader, const std::string & diffuseFactor, const std::string & specularFactor) const noexcept
	{
		if ( m_fragmentColor.empty() )
		{
			TraceError{ClassId} << "There is no name for the fragment color output !";

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

		/* NOTE: In PBR mode, use albedo instead of diffuse color. */
		const auto & surfaceColor = m_usePBRMode && !m_surfaceAlbedo.empty() ? m_surfaceAlbedo : m_surfaceDiffuseColor;

		const auto finaleDiffuseFactor = m_useOpacity ? diffuseFactor + " * " + m_surfaceOpacityAmount : diffuseFactor;

		/* PHOTOMETRIC DIRECT DIFFUSE. The light intensity is an ILLUMINANCE in lux, so the outgoing
		 * LUMINANCE of a Lambertian surface is albedo * E * cos(theta) / pi — the same 1/pi the
		 * ambient pass applies (see generateAmbientFragmentShader).
		 * ⚠️ This normalisation was missing here while the PBR generator had it
		 * (LightGenerator.PBR.cpp: "kD * albedo / 3.14159265"), so the two material models
		 * disagreed by a factor of pi on DIRECT lighting: a legacy/Standard surface rendered ~3.14x
		 * brighter than a PBR one under the same sun, which put sunlit ground well above the sky's
		 * own luminance and read as "flashy".
		 * NOTE: deliberately NOT folded into finaleDiffuseFactor — that expression is also used as
		 * a raw geometric N.L term inside the PBR low-quality specular pow() below. */
		const auto diffuseIlluminance = (std::stringstream{} << '(' << this->lightIntensity() << " * 0.3183098862)").str();

		Code{fragmentShader} << m_fragmentColor << ".rgb += " << surfaceColor << ".rgb * (" << this->lightColor() << ".rgb * projectionColor * " << diffuseIlluminance << " * " << finaleDiffuseFactor << ");";

		/* NOTE: In PBR mode, reflection/refraction (IBL) is handled ONLY in the ambient pass
		 * via generateAmbientFragmentShader(). We skip per-light reflection mixing here.
		 * In legacy (Phong) mode, reflection is mixed per-light for compatibility. */
		if ( !m_usePBRMode )
		{
			if ( m_useReflection && m_useRefraction )
			{
				/* NOTE: Fresnel effect for blending reflection and refraction.
				 * Schlick approximation: F = F0 + (1 - F0) * pow(1 - cosTheta, 5)
				 * F0 for glass is approximately 0.04, for water ~0.02, for diamond ~0.17.
				 * We compute F0 from IOR: F0 = ((n1-n2)/(n1+n2))^2 where n1=1 (air). */
				const auto code = (std::stringstream{} <<
					"const vec3 reflected = mix(" << surfaceColor << ", " << m_surfaceReflectionColor << ", " << m_surfaceReflectionAmount << ").rgb;" "\n"
					"const vec3 refracted = mix(" << surfaceColor << ", " << m_surfaceRefractionColor << ", " << m_surfaceRefractionAmount << ").rgb;" "\n\n" <<

					m_fragmentColor << ".rgb += mix(refracted, reflected, fresnelFactor) * (" << this->lightColor() << ".rgb * projectionColor * " << diffuseIlluminance << " * " << finaleDiffuseFactor << ");").str();

				Code{fragmentShader, Location::Output} << code;
			}
			else if ( m_useReflection )
			{
				Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << surfaceColor << ", " << m_surfaceReflectionColor << ", " << m_surfaceReflectionAmount << ").rgb * (" << this->lightColor() << ".rgb * projectionColor * " << diffuseIlluminance << " * " << finaleDiffuseFactor << ");";
			}
			else if ( m_useRefraction )
			{
				Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << surfaceColor << ", " << m_surfaceRefractionColor << ", " << m_surfaceRefractionAmount << ").rgb * (" << this->lightColor() << ".rgb * projectionColor * " << diffuseIlluminance << " * " << finaleDiffuseFactor << ");";
			}
		}

		/* NOTE: Specular reflection mixing is for legacy (Phong) materials only.
		 * PBR materials don't set m_surfaceSpecularColor - they use Cook-Torrance BRDF. */
		if ( !m_surfaceSpecularColor.empty() && !m_usePBRMode )
		{
			const auto finaleSpecularFactor = m_useOpacity ? specularFactor + " * " + m_surfaceOpacityAmount : specularFactor;

			if ( m_useReflection && m_useRefraction )
			{
				/* NOTE: Fresnel effect for blending reflection and refraction.
				 * Schlick approximation: F = F0 + (1 - F0) * pow(1 - cosTheta, 5)
				 * F0 for glass is approximately 0.04, for water ~0.02, for diamond ~0.17.
				 * We compute F0 from IOR: F0 = ((n1-n2)/(n1+n2))^2 where n1=1 (air). */
				const auto code = (std::stringstream{} <<
					"const vec3 reflectedSpecular = mix(" << m_surfaceSpecularColor << ", " << m_surfaceReflectionColor << ", " << m_surfaceReflectionAmount << ").rgb;" "\n"
					"const vec3 refractedSpecular = mix(" << m_surfaceSpecularColor << ", " << m_surfaceRefractionColor << ", " << m_surfaceRefractionAmount << ").rgb;" "\n\n" <<

					m_fragmentColor << ".rgb += mix(refractedSpecular, reflectedSpecular, fresnelFactor) * (" << this->lightIntensity() << " * " << finaleSpecularFactor << ");").str();

				Code{fragmentShader, Location::Output} << code;
			}
			else if ( m_useReflection )
			{
				Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << m_surfaceSpecularColor << ", " << m_surfaceReflectionColor << ", " << m_surfaceReflectionAmount << ").rgb * (" << this->lightIntensity() << " * " << finaleSpecularFactor << ");";
			}
			else if ( m_useRefraction )
			{
				Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << m_surfaceSpecularColor << ", " << m_surfaceRefractionColor << ", " << m_surfaceRefractionAmount << ").rgb * (" << this->lightIntensity() << " * " << finaleSpecularFactor << ");";
			}
			else
			{
				Code{fragmentShader} << m_fragmentColor << ".rgb += " << m_surfaceSpecularColor << ".rgb * (" << this->lightIntensity() << " * " << finaleSpecularFactor << ");";
			}
		}
		/* NOTE: PBR low-quality specular approximation using diffuseFactor.
		 * Since specularFactor isn't available in Gouraud mode for PBR, we approximate
		 * using diffuseFactor (N·L) instead of the proper (N·H) or (R·V).
		 * F0 with albedo/metalness for colored metal highlights. */
		else if ( m_usePBRMode && !m_surfaceRoughness.empty() )
		{
			const auto albedo = m_surfaceAlbedo.empty() ? "vec3(1.0)" : m_surfaceAlbedo + ".rgb";
			const auto metalness = m_surfaceMetalness.empty() ? "0.0" : m_surfaceMetalness;
			const auto code = (std::stringstream{} <<
				"/* PBR low-quality specular - F0 approximation. */" "\n"
				"const float lqShininess = pow(1.0 - " << m_surfaceRoughness << ", 2.0) * 64.0 + 1.0;" "\n"
				"const vec3 lqSpecF0 = mix(vec3(1.00), " << albedo << ", " << metalness << ");" "\n"
				"const float lqSpecPower = pow(max(" << finaleDiffuseFactor << ", 0.0), lqShininess);" "\n" <<
				m_fragmentColor << ".rgb += lqSpecF0 * " << this->lightColor() << ".rgb * projectionColor * " << this->lightIntensity() << " * lqSpecPower;").str();

			Code{fragmentShader, Location::Output} << code;
		}

		return true;
	}
}
