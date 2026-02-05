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
#include "Declaration/Function.hpp"
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

	std::string
	LightGenerator::lightPositionWorldSpace () const noexcept
	{
		if ( m_useStaticLighting )
		{
			return m_staticLighting->positionVec4();
		}

		return LightUB(UniformBlock::Component::PositionWorldSpace);
	}

	std::string
	LightGenerator::lightDirectionWorldSpace () const noexcept
	{
		if ( m_useStaticLighting )
		{
			return m_staticLighting->directionVec4();
		}

		return LightUB(UniformBlock::Component::DirectionWorldSpace);
	}

	std::string
	LightGenerator::ambientLightColor () const noexcept
	{
		if ( m_useStaticLighting )
		{
			return m_staticLighting->ambientColorVec4();
		}

		return ViewUB(UniformBlock::Component::AmbientLightColor, false);
	}

	std::string
	LightGenerator::ambientLightIntensity () const noexcept
	{
		if ( m_useStaticLighting )
		{
			return std::to_string(m_staticLighting->ambientIntensity());
		}

		return ViewUB(UniformBlock::Component::AmbientLightIntensity, false);
	}

	std::string
	LightGenerator::lightIntensity () const noexcept
	{
		if ( m_useStaticLighting )
		{
			return std::to_string(m_staticLighting->intensity());
		}

		return LightUB(UniformBlock::Component::Intensity);
	}

	std::string
	LightGenerator::lightRadius () const noexcept
	{
		if ( m_useStaticLighting )
		{
			return std::to_string(m_staticLighting->radius());
		}

		return LightUB(UniformBlock::Component::Radius);
	}

	std::string
	LightGenerator::lightInnerCosAngle () const noexcept
	{
		if ( m_useStaticLighting )
		{
			return std::to_string(m_staticLighting->innerCosAngle());
		}

		return LightUB(UniformBlock::Component::InnerCosAngle);
	}

	std::string
	LightGenerator::lightOuterCosAngle () const noexcept
	{
		if ( m_useStaticLighting )
		{
			return std::to_string(m_staticLighting->outerCosAngle());
		}

		return LightUB(UniformBlock::Component::OuterCosAngle);
	}

	std::string
	LightGenerator::lightColor () const noexcept
	{
		if ( m_useStaticLighting )
		{
			return m_staticLighting->colorVec4();
		}

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
	LightGenerator::getUniformBlock (uint32_t set, uint32_t binding, LightType lightType, bool useShadowMap) noexcept
	{
		switch ( lightType )
		{
			case LightType::Directional :
			{
				Declaration::UniformBlock block{set, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::DirectionalLight, UniformBlock::Light};
				block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::Color);
				block.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::DirectionWorldSpace);
				block.addMember(Declaration::VariableType::Float, UniformBlock::Component::Intensity);

				if ( useShadowMap )
				{
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::PCFRadius);
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ShadowBias);
					block.addMember(Declaration::VariableType::Matrix4, UniformBlock::Component::ViewProjectionMatrix);
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

				if ( useShadowMap )
				{
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::PCFRadius);
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ShadowBias);
					block.addMember(Declaration::VariableType::Matrix4, UniformBlock::Component::ViewProjectionMatrix);
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

				if ( useShadowMap )
				{
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::PCFRadius);
					block.addMember(Declaration::VariableType::Float, UniformBlock::Component::ShadowBias);
					block.addMember(Declaration::VariableType::Matrix4, UniformBlock::Component::ViewProjectionMatrix);
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

	RenderPassType
	LightGenerator::checkRenderPassType () const noexcept
	{
		if ( m_renderPassType != RenderPassType::SimplePass )
		{
			return m_renderPassType;
		}

		switch ( m_staticLighting->type() )
		{
			case LightType::Directional :
				return RenderPassType::DirectionalLightPassNoShadow;

			case LightType::Point :
				return RenderPassType::PointLightPassNoShadow;

			case LightType::Spot :
				return RenderPassType::SpotLightPassNoShadow;

			default:
				return RenderPassType::None;
		}
	}

	bool
	LightGenerator::generateVertexShaderCode (Generator::Abstract & generator, VertexShader & vertexShader) const noexcept
	{
		const auto lightSetIndex = generator.shaderProgram()->setIndex(SetType::PerLight);

		auto lightType = LightType::Directional;
		bool enableShadowMap = false;

		switch ( this->checkRenderPassType() )
		{
			case RenderPassType::AmbientPass :
				/* NOTE: Nothing to do for ambient pass inside the vertex shader. */
				return true;

			case RenderPassType::DirectionalLightPassCSM :
				/* CSM uses a different uniform block. For now, fall through to standard. */
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::DirectionalLightPass :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::DirectionalLightPassNoShadow :
				lightType = LightType::Directional;
				break;

			case RenderPassType::PointLightPass :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::PointLightPassNoShadow :
				lightType = LightType::Point;
				break;

			case RenderPassType::SpotLightPass :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::SpotLightPassNoShadow :
				lightType = LightType::Spot;
				break;

			case RenderPassType::None :
			case RenderPassType::SimplePass :
			default:

				Tracer::error(ClassId, "Calling the light code generation render pass set to 'None' !");
				return false;
		}

		/* CSM uses a specialized uniform block. */
		const bool useCSM = (this->checkRenderPassType() == RenderPassType::DirectionalLightPassCSM);

		if ( !m_useStaticLighting )
		{
			if ( useCSM )
			{
				if ( !vertexShader.declare(LightGenerator::getUniformBlockCSM(lightSetIndex, 0)) )
				{
					return false;
				}
			}
			else if ( !vertexShader.declare(LightGenerator::getUniformBlock(lightSetIndex, 0, lightType, enableShadowMap)) )
			{
				return false;
			}
		}


		if ( generator.highQualityEnabled() )
		{
			/* PBR mode uses Cook-Torrance BRDF. */
			if ( m_usePBRMode )
			{
				return this->generatePBRVertexShader(generator, vertexShader, lightType, enableShadowMap);
			}

			if ( m_useNormalMapping )
			{
				return this->generatePhongBlinnWithNormalMapVertexShader(generator, vertexShader, lightType, enableShadowMap);
			}

			return this->generatePhongBlinnVertexShader(generator, vertexShader, lightType, enableShadowMap);
		}

		return this->generateGouraudVertexShader(generator, vertexShader, lightType, enableShadowMap);
	}

	bool
	LightGenerator::generateFragmentShaderCode (Generator::Abstract & generator, FragmentShader & fragmentShader) const noexcept
	{
		const auto lightSetIndex = generator.shaderProgram()->setIndex(SetType::PerLight);

		auto lightType = LightType::Directional;
		bool enableShadowMap = false;

		switch ( this->checkRenderPassType() )
		{
			case RenderPassType::AmbientPass :
			{
				if ( m_fragmentColor.empty() )
				{
					TraceError{ClassId} << "There is no name for the fragment color output !";

					return false;
				}

				Code{fragmentShader, Location::Top} << "vec4 " << m_fragmentColor << " = vec4(0.0, 0.0, 0.0, 1.0);";

				this->generateAmbientFragmentShader(fragmentShader);

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

			case RenderPassType::DirectionalLightPassCSM :
				/* CSM uses a different uniform block. For now, fall through to standard. */
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::DirectionalLightPass :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::DirectionalLightPassNoShadow :
				lightType = LightType::Directional;
				break;

			case RenderPassType::PointLightPass :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::PointLightPassNoShadow :
				lightType = LightType::Point;
				break;

			case RenderPassType::SpotLightPass :
				enableShadowMap = true;
				[[fallthrough]];
			case RenderPassType::SpotLightPassNoShadow :
				lightType = LightType::Spot;
				break;

			default :
				Tracer::error(ClassId, "Invalid render pass for lighting !");

				return false;
		}

		/* CSM uses a specialized uniform block. */
		const bool useCSM = (this->checkRenderPassType() == RenderPassType::DirectionalLightPassCSM);

		if ( !m_useStaticLighting )
		{
			if ( useCSM )
			{
				if ( !fragmentShader.declare(LightGenerator::getUniformBlockCSM(lightSetIndex, 0)) )
				{
					return false;
				}
			}
			else if ( !fragmentShader.declare(LightGenerator::getUniformBlock(lightSetIndex, 0, lightType, enableShadowMap)) )
			{
				return false;
			}
		}


		if ( generator.highQualityEnabled() )
		{
			/* PBR mode uses Cook-Torrance BRDF. */
			if ( m_usePBRMode )
			{
				return this->generatePBRFragmentShader(generator, fragmentShader, lightType, enableShadowMap);
			}

			if ( m_useNormalMapping )
			{
				return this->generatePhongBlinnWithNormalMapFragmentShader(generator, fragmentShader, lightType, enableShadowMap);
			}

			return this->generatePhongBlinnFragmentShader(generator, fragmentShader, lightType, enableShadowMap);
		}

		return this->generateGouraudFragmentShader(generator, fragmentShader, lightType, enableShadowMap);
	}

	void
	LightGenerator::generateAmbientFragmentShader (FragmentShader & fragmentShader) const noexcept
	{
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

		if ( m_surfaceAmbientColor.empty() )
		{
			/* NOTE: Get 5% of the surface diffuse/albedo color to create the surface ambient color.
			 * In PBR mode, use albedo instead of diffuse. */
			const auto & baseColor = m_usePBRMode && !m_surfaceAlbedo.empty() ? m_surfaceAlbedo : m_surfaceDiffuseColor;
			surfaceColor = (std::stringstream{} << "(" << baseColor << " * 0.05)").str();
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

		if ( m_usePBRMode && m_useReflection && m_useRefraction && this->highQualityEnabled() )
		{
			/* NOTE: PBR Glass/transparent materials with both reflection and refraction.
			 * The Fresnel effect determines the blend between reflection and refraction.
			 * IBL is the main contribution for glass - it shows the environment, not ambient light.
			 * IBLIntensity allows dynamic control over the cubemap contribution.
			 * Requires high-quality mode for reflectionNormal and reflectionI variables. */
			const auto iblIntensity = m_surfaceIBLIntensity.empty() ? "1.0" : m_surfaceIBLIntensity;
			const auto code = (std::stringstream{} <<
				"/* PBR Glass IBL - Fresnel-Schlick approximation. */" "\n"
				"const float NdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n"
				"const float fresnelFactor = 0.04 + (1.0 - 0.04) * pow(1.0 - NdotV, 5.0);" "\n"
				"const vec3 reflectedColor = " << m_surfaceReflectionColor << ".rgb * " << m_surfaceReflectionAmount << ";" "\n"
				"vec3 refractedColor = " << m_surfaceRefractionColor << ".rgb * " << m_surfaceRefractionAmount << ";" "\n").str();

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
					m_fragmentColor << ".rgb += mix(refractedColor, reflectedColor, fresnelFactor) * " << iblIntensity << ";").str();
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
					m_fragmentColor << ".rgb += reflectedColor * ccFactor * ccFresnel * " << iblIntensity << ";").str();
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
			const auto iblIntensity = m_surfaceIBLIntensity.empty() ? "1.0" : m_surfaceIBLIntensity;
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
				"const vec3 reflectedColor = " << m_surfaceReflectionColor << ".rgb * " << m_surfaceReflectionAmount << ";" "\n"
				"/* Beer's law absorption for transmission. */" "\n"
				"const vec3 transAbsorption = exp(log(max(" << m_surfaceAttenuationColor << ".rgb, vec3(0.001))) / max(" << m_surfaceAttenuationDistance << ", 0.0001) * " << m_surfaceThicknessFactor << ");" "\n"
				"const vec3 transmittedLight = " << m_surfaceTransmissionColor << " * transAbsorption;" "\n"
				"/* F = reflection, (1-F)*transmissionFactor = transmission. */" "\n" <<
				m_fragmentColor << ".rgb += reflectedColor * fresnelDielectric * " << iblIntensity << ";" "\n" <<
				m_fragmentColor << ".rgb += transmittedLight * " << m_surfaceTransmissionFactor << " * (1.0 - fresnelDielectric) * " << iblIntensity << ";").str();

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
					m_fragmentColor << ".rgb += reflectedColor * ccFactor * ccFresnel * " << iblIntensity << ";").str();
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
			const auto iblIntensity = m_surfaceIBLIntensity.empty() ? "1.0" : m_surfaceIBLIntensity;
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
					"const vec3 reflectedColor = " << m_surfaceReflectionColor << ".rgb * " << m_surfaceReflectionAmount << ";" "\n"
					"/* IBL contribution modulated by Fresnel and IBL intensity. */" "\n" <<
					m_fragmentColor << ".rgb += reflectedColor * fresnelIBL * " << iblIntensity << ";").str();

				Code{fragmentShader, Location::Output} << code;
			}
			else
			{
				const auto code = (std::stringstream{} <<
					"/* PBR IBL - Fresnel-Schlick with proper F0 for metals. */" "\n" <<
					iblF0Computation << "\n"
					"const float NdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n"
					"const vec3 fresnelIBL = iblF0 + (1.0 - iblF0) * pow(1.0 - NdotV, 5.0);" "\n"
					"const vec3 reflectedColor = " << m_surfaceReflectionColor << ".rgb * " << m_surfaceReflectionAmount << ";" "\n"
					"/* IBL contribution modulated by Fresnel and IBL intensity. */" "\n" <<
					m_fragmentColor << ".rgb += reflectedColor * fresnelIBL * " << iblIntensity << ";").str();

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
					m_fragmentColor << ".rgb += reflectedColor * ccFactor * ccFresnel * " << iblIntensity << ";").str();
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
			const auto iblIntensity = m_surfaceIBLIntensity.empty() ? "1.0" : m_surfaceIBLIntensity;
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
				m_fragmentColor << ".rgb += " << m_surfaceReflectionColor << ".rgb * lqF0 * " << m_surfaceReflectionAmount << " * " << iblIntensity << ";").str();
			Code{fragmentShader, Location::Output} << code;

			/* Clear coat IBL - simplified constant attenuation (LQ, no reflectionNormal available). */
			if ( m_useClearCoat )
			{
				const auto ccCode = (std::stringstream{} <<
					"/* Clear coat IBL - simplified constant attenuation (LQ). */" "\n"
					"const float ccFactor = " << m_surfaceClearCoatFactor << ";" "\n" <<
					m_fragmentColor << ".rgb *= (1.0 - ccFactor * 0.04);" "\n" <<
					m_fragmentColor << ".rgb += " << m_surfaceReflectionColor << ".rgb * ccFactor * 0.04 * " << m_surfaceReflectionAmount << " * " << iblIntensity << ";").str();
				Code{fragmentShader, Location::Output} << ccCode;
			}
		}
		else if ( m_usePBRMode && m_useRefraction )
		{
			/* NOTE: PBR low-quality fallback for refraction-only materials.
			 * Refraction is less affected by F0 - use a subtle blend. */
			const auto iblIntensity = m_surfaceIBLIntensity.empty() ? "1.0" : m_surfaceIBLIntensity;

			if ( m_useTransmission )
			{
				const auto code = (std::stringstream{} <<
					"/* PBR refraction with Beer's law absorption. */" "\n"
					"const vec3 beerAbsorption = exp(log(max(" << m_surfaceAttenuationColor << ".rgb, vec3(0.001))) / max(" << m_surfaceAttenuationDistance << ", 0.0001) * " << m_surfaceThicknessFactor << ");" "\n"
					"vec3 refrRefractedColor = " << m_surfaceRefractionColor << ".rgb * " << m_surfaceRefractionAmount << " * 0.96;" "\n"
					"refrRefractedColor *= beerAbsorption * " << m_surfaceTransmissionFactor << " + (1.0 - " << m_surfaceTransmissionFactor << ");" "\n" <<
					m_fragmentColor << ".rgb += refrRefractedColor * " << iblIntensity << ";").str();
				Code{fragmentShader} << code;
			}
			else
			{
				Code{fragmentShader} << m_fragmentColor << ".rgb += " << m_surfaceRefractionColor << ".rgb * " << m_surfaceRefractionAmount << " * 0.96 * " << iblIntensity << ";";
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
			Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << surfaceColor << ", " << m_surfaceReflectionColor << ", " << m_surfaceReflectionAmount << ").rgb * (" << this->ambientLightColor() << ".rgb * " << intensity << ");";
		}
		else if ( m_useRefraction )
		{
			Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << surfaceColor << ", " << m_surfaceRefractionColor << ", " << m_surfaceRefractionAmount << ").rgb * (" << this->ambientLightColor() << ".rgb * " << intensity << ");";
		}
		else
		{
			Code{fragmentShader} << m_fragmentColor << ".rgb += " << surfaceColor << ".rgb * (" << this->ambientLightColor() << ".rgb * " << intensity << ");";
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

		/* Ambient Occlusion (baked texture) support - modulates the ambient contribution. */
		if ( m_useAmbientOcclusion && !m_surfaceAmbientOcclusion.empty() )
		{
			const auto aoIntensity = m_surfaceAOIntensity.empty() ? "1.0" : m_surfaceAOIntensity;
			/* NOTE: AO darkens ambient lighting. mix(1.0, ao, intensity) allows intensity control.
			 * When intensity = 0, no AO effect. When intensity = 1, full AO effect. */
			Code{fragmentShader} << m_fragmentColor << ".rgb *= mix(1.0, " << m_surfaceAmbientOcclusion << ", " << aoIntensity << ");";
		}

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
			const auto iblIntensity = m_surfaceIBLIntensity.empty() ? "1.0" : m_surfaceIBLIntensity;
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

		if ( m_useStaticLighting )
		{
			this->generateAmbientFragmentShader(fragmentShader);
		}

		/* NOTE: In PBR mode, use albedo instead of diffuse color. */
		const auto & surfaceColor = m_usePBRMode && !m_surfaceAlbedo.empty() ? m_surfaceAlbedo : m_surfaceDiffuseColor;

		const auto finaleDiffuseFactor = m_useOpacity ? diffuseFactor + " * " + m_surfaceOpacityAmount : diffuseFactor;

		Code{fragmentShader} << m_fragmentColor << ".rgb += " << surfaceColor << ".rgb * (" << this->lightColor() << ".rgb * " << this->lightIntensity() << " * " << finaleDiffuseFactor << ");";

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

					m_fragmentColor << ".rgb += mix(refracted, reflected, fresnelFactor) * (" << this->lightColor() << ".rgb * " << this->lightIntensity() << " * " << finaleDiffuseFactor << ");").str();

				Code{fragmentShader, Location::Output} << code;
			}
			else if ( m_useReflection )
			{
				Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << surfaceColor << ", " << m_surfaceReflectionColor << ", " << m_surfaceReflectionAmount << ").rgb * (" << this->lightColor() << ".rgb * " << this->lightIntensity() << " * " << finaleDiffuseFactor << ");";
			}
			else if ( m_useRefraction )
			{
				Code{fragmentShader} << m_fragmentColor << ".rgb += mix(" << surfaceColor << ", " << m_surfaceRefractionColor << ", " << m_surfaceRefractionAmount << ").rgb * (" << this->lightColor() << ".rgb * " << this->lightIntensity() << " * " << finaleDiffuseFactor << ");";
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
				m_fragmentColor << ".rgb += lqSpecF0 * " << this->lightColor() << ".rgb * " << this->lightIntensity() << " * lqSpecPower;").str();

			Code{fragmentShader, Location::Output} << code;
		}

		return true;
	}
}
