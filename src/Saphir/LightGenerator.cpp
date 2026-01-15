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
					block.addMember(Declaration::VariableType::Matrix4, UniformBlock::Component::ViewProjectionMatrix);
				}

				return block;
			}

			default:
				/* TODO: Fix this! */
				return {0, 0, Declaration::MemoryLayout::Std140, nullptr, nullptr};
		}
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

		if ( !m_useStaticLighting && !vertexShader.declare(LightGenerator::getUniformBlock(lightSetIndex, 0, lightType, enableShadowMap)) )
		{
			return false;
		}

		if ( generator.highQualityLightEnabled() )
		{
			/* PBR mode uses Cook-Torrance BRDF. */
			if ( m_usePBRMode )
			{
				return this->generatePBRVertexShader(generator, vertexShader, lightType, enableShadowMap);
			}

			if ( generator.normalMappingEnabled() && m_useNormalMapping )
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

		if ( !m_useStaticLighting && !fragmentShader.declare(LightGenerator::getUniformBlock(lightSetIndex, 0, lightType, enableShadowMap)) )
		{
			return false;
		}

		if ( generator.highQualityLightEnabled() )
		{
			/* PBR mode uses Cook-Torrance BRDF. */
			if ( m_usePBRMode )
			{
				return this->generatePBRFragmentShader(generator, fragmentShader, lightType, enableShadowMap);
			}

			if ( generator.normalMappingEnabled() && m_useNormalMapping )
			{
				return this->generatePhongBlinnWithNormalMapFragmentShader(generator, fragmentShader, lightType, enableShadowMap);
			}

			return this->generatePhongBlinnFragmentShader(generator, fragmentShader, lightType, enableShadowMap);
		}

		return this->generateGouraudFragmentShader(generator, fragmentShader, lightType, enableShadowMap);
	}

	std::string
	LightGenerator::generate2DShadowMapCode (const std::string & shadowMap, const std::string & fragmentPosition, DepthTextureFunction function) const noexcept
	{
		std::stringstream code{};

		code << "/* Shadow map 2D resolution. */" "\n\n";

		code << "float shadowFactor = 1.0;" "\n\n";

		if ( m_PCFSample > 0 )
		{
			code << GLSL::ConstInteger << " offset = " << m_PCFSample << ";" "\n\n";

			switch ( function )
			{
				/* NOTE: OpenGL 3.2 (GLSL 150) - float textureOffset(sampler2DShadow sampler, vec4 P, ivec2 offset, [float bias]); */
				case DepthTextureFunction::Texture :
					code <<
						"for ( " << GLSL::Integer << " idy = -offset; idy <= offset; idy++ )" "\n"
						"	for ( " << GLSL::Integer << " idx = -offset; idx <= offset; idx++ )" "\n"
						"		shadowFactor += textureOffset(" << shadowMap << ", " << fragmentPosition << ".xyz, ivec2(idx, idy));" "\n\n";
					break;

				/* NOTE: OpenGL 4.0 (GLSL 400) - float textureProjOffset(sampler2DShadow sampler, vec4 P, ivec2 offset, [float bias]); */
				case DepthTextureFunction::TextureProj :
					code <<
						"for ( " << GLSL::Integer << " idy = -offset; idy <= offset; idy++ )" "\n"
						"	for ( " << GLSL::Integer << " idx = -offset; idx <= offset; idx++ )" "\n"
						"		shadowFactor += textureProjOffset(" << shadowMap << ", " << fragmentPosition << ", ivec2(idx, idy));" "\n\n";
					break;

				/* NOTE: OpenGL 4.6 (GLSL 460) - vec4 textureGatherOffset(gsampler2DShadow sampler, vec2 P, float refZ, ivec2 offset); */
				case DepthTextureFunction::TextureGather :
					code <<
						"for ( " << GLSL::Integer << " idy = -offset; idy <= offset; idy++ )" "\n"
						"{" "\n"
						"	for ( " << GLSL::Integer << " idx = -offset; idx <= offset; idx++ )" "\n"
						"	{" "\n"
						"		const vec4 gather = textureGatherOffset(" << shadowMap << ", " << fragmentPosition << ".xy, " << fragmentPosition << ".z, ivec2(idx, idy));" "\n"
						"		const vec2 shadowSize = textureSize(" << shadowMap << ", 0);" "\n"
						"		const vec2 texelCoord = " << fragmentPosition << ".xy * shadowSize;" "\n"
						"		const vec2 sampleCoord = fract(texelCoord + 0.5);" "\n\n"

						"		const float texelY0 = mix(gather.w, gather.x, sampleCoord.y);" "\n"
						"		const float texelY1 = mix(gather.z, gather.y, sampleCoord.y);" "\n"
						"		shadowFactor += mix(texelY0, texelY1, sampleCoord.x);" "\n"
						"	}" "\n"
						"}" "\n\n";
					break;
			}

			code << "shadowFactor /= pow(float(offset) * 2.0 + 1.0, 2);" "\n";
		}
		else
		{
			switch ( function )
			{
				/* NOTE: OpenGL 3.2 (GLSL 150) - float texture(sampler2DShadow sampler, vec3 P, [float bias]); */
				case DepthTextureFunction::Texture :
					code << "shadowFactor = texture(" << shadowMap << ", " << fragmentPosition << ".xyz);" "\n\n";
					break;

				/* NOTE: OpenGL 4.0 (GLSL 400) - float textureProj(sampler2DShadow sampler, vec4 P, [float bias]); */
				case DepthTextureFunction::TextureProj :
					code << "shadowFactor = textureProj(" << shadowMap << ", " << fragmentPosition << ");" "\n\n";
					break;

				/* NOTE: OpenGL 4.6 (GLSL 460) - vec4 textureGather(gsampler2DShadow sampler, vec2 P, float refZ); */
				case DepthTextureFunction::TextureGather :
					code <<
						"const vec4 gather = textureGather(" << shadowMap << ", " << fragmentPosition << ".xy, " << fragmentPosition << ".z);" "\n"
						"const vec2 shadowSize = textureSize(" << shadowMap << ", 0);" "\n"
						"const vec2 texelCoord = " << fragmentPosition << ".xy * shadowSize;" "\n"
						"const vec2 sampleCoord = fract(texelCoord + 0.5);" "\n\n"

						"const float texelY0 = mix(gather.w, gather.x, sampleCoord.y);" "\n"
						"const float texelY1 = mix(gather.z, gather.y, sampleCoord.y);" "\n" <<
						"shadowFactor = mix(texelY0, texelY1, sampleCoord.x);" "\n\n";
					break;
			}
		}

		if ( m_discardUnlitFragment )
		{
			code << "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code.str();
	}

	std::string
	LightGenerator::generate3DShadowMapCode (const std::string & shadowMap, const std::string & directionWorldSpace, const std::string & nearFar) const noexcept
	{
		std::stringstream code{};

		code << "/* Shadow map 3D resolution. */" "\n\n";

		code << "float shadowFactor = 1.0;" "\n\n";

		if ( m_PCFSample > 0 )
		{
			code <<
				"const vec3 lookupVector = vec3(-" << directionWorldSpace << ".x, " << directionWorldSpace << ".y, " << directionWorldSpace << ".z);" "\n"
				"const float depth = length(lookupVector);" "\n\n"

				"const vec3 sampleOffsetDirections[20] = vec3[] (" "\n"
				"	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1)," "\n"
				"	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1)," "\n"
				"	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0)," "\n"
				"	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1)," "\n"
				"	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)" "\n"
				");" "\n"

				"const float diskRadius = " << m_PCFRadius << ";" "\n\n"

				"for ( " << GLSL::Integer << " index = 0; index < 20; ++index )" "\n"
				"{" "\n"
				"	const float smallestDepth = texture(" << shadowMap << ", lookupVector + sampleOffsetDirections[index] * diskRadius).r * " << nearFar << ".y;" "\n\n"

				"	if ( smallestDepth - 0.01 > depth )" "\n"
				"	{" "\n"
				"		shadowFactor += 1.0;" "\n"
				"	}" "\n"
				"}" "\n\n"

				"shadowFactor /= 20.0;" "\n\n";
		}
		else
		{
			code <<
				"const vec3 lookupVector = vec3(-" << directionWorldSpace << ".x, " << directionWorldSpace << ".y, " << directionWorldSpace << ".z);" "\n"
				"const float smallestDepth = texture(" << shadowMap << ", lookupVector).r * " << nearFar << ".y;" "\n"
				"const float depth = length(lookupVector);" "\n\n"

				"if ( smallestDepth + 0.01 < depth )" "\n"
				"{" "\n"
				"	shadowFactor = 0.0;" "\n"
				"}" "\n\n";
		}

		if ( m_discardUnlitFragment )
		{
			code << "if ( shadowFactor <= 0.0 ) { discard; }" "\n\n";
		}

		return code.str();
	}

	bool
	LightGenerator::generateVertexShaderShadowMapCode (Generator::Abstract & generator, VertexShader & vertexShader, bool shadowCubemap) const noexcept
	{
		/* NOTE: For point light. */
		if ( shadowCubemap )
		{
			if ( !vertexShader.declare(Declaration::StageOutput{generator.getNextShaderVariableLocation(), GLSL::FloatVector4, "DirectionWorldSpace", GLSL::Smooth}) )
			{
				return false;
			}

			if ( vertexShader.isInstancingEnabled() )
			{
				/* Get the model matrix from VBO. */
				Code{vertexShader, Location::Output} << "DirectionWorldSpace = " << this->lightPositionWorldSpace() << " - " << Attribute::ModelMatrix << " * vec4(" << Attribute::Position << ", 1.0);";
			}
			else
			{
				/* Get the model matrix from UBO. */
				Code{vertexShader, Location::Output} << "DirectionWorldSpace = " << this->lightPositionWorldSpace() << " - " << MatrixPC(PushConstant::Component::ModelMatrix) << " * vec4(" << Attribute::Position << ", 1.0);";
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
				Code{vertexShader, Location::Output} << "PositionLightSpace = " << LightUB(UniformBlock::Component::ViewProjectionMatrix) << " * " << Attribute::ModelMatrix << " * vec4(" << Attribute::Position << ", 1.0);";
			}
			else
			{
				Code{vertexShader, Location::Output} << "PositionLightSpace = " << LightUB(UniformBlock::Component::ViewProjectionMatrix) << " * " << MatrixPC(PushConstant::Component::ModelMatrix) << " * vec4(" << Attribute::Position << ", 1.0);";
			}
		}

		return true;
	}

	void
	LightGenerator::generateAmbientFragmentShader (FragmentShader & fragmentShader) const noexcept
	{
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

		if ( m_usePBRMode && m_useReflection && m_useRefraction && this->highQualityReflectionEnabled() )
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
				"const vec3 refractedColor = " << m_surfaceRefractionColor << ".rgb * " << m_surfaceRefractionAmount << ";" "\n"
				"/* Blend reflection and refraction based on Fresnel, modulated by IBL intensity. */" "\n" <<
				m_fragmentColor << ".rgb += mix(refractedColor, reflectedColor, fresnelFactor) * " << iblIntensity << ";").str();

			Code{fragmentShader, Location::Output} << code;
		}
		else if ( m_usePBRMode && m_useReflection && this->highQualityReflectionEnabled() )
		{
			/* NOTE: PBR Metal/reflective materials.
			 * IBL is modulated by Fresnel (with proper F0 based on metalness) and IBLIntensity.
			 * For metals (metalness=1), F0 = albedo color, giving strong colored reflections.
			 * For dielectrics (metalness=0), F0 = 0.04, giving weak white reflections.
			 * Requires high-quality mode for reflectionNormal and reflectionI variables. */
			const auto iblIntensity = m_surfaceIBLIntensity.empty() ? "1.0" : m_surfaceIBLIntensity;
			const auto albedo = m_surfaceAlbedo.empty() ? "vec3(1.0)" : m_surfaceAlbedo + ".rgb";
			const auto metalness = m_surfaceMetalness.empty() ? "0.0" : m_surfaceMetalness;
			const auto code = (std::stringstream{} <<
				"/* PBR IBL - Fresnel-Schlick with proper F0 for metals. */" "\n"
				"const vec3 iblF0 = mix(vec3(0.04), " << albedo << ", " << metalness << ");" "\n"
				"const float NdotV = max(dot(reflectionNormal, -reflectionI), 0.0);" "\n"
				"const vec3 fresnelIBL = iblF0 + (1.0 - iblF0) * pow(1.0 - NdotV, 5.0);" "\n"
				"const vec3 reflectedColor = " << m_surfaceReflectionColor << ".rgb * " << m_surfaceReflectionAmount << ";" "\n"
				"/* IBL contribution modulated by Fresnel and IBL intensity. */" "\n" <<
				m_fragmentColor << ".rgb += reflectedColor * fresnelIBL * " << iblIntensity << ";").str();

			Code{fragmentShader, Location::Output} << code;
		}
		else if ( m_usePBRMode && m_useReflection )
		{
			/* NOTE: PBR low-quality fallback - simplified IBL without per-fragment Fresnel.
			 * When high-quality reflection is disabled, reflectionNormal and reflectionI
			 * are not available. We approximate F0 using metalness:
			 * - Dielectrics (metalness=0): F0 ≈ 0.04 (only 4% reflection)
			 * - Metals (metalness=1): F0 = albedo (colored reflections)
			 * This prevents the overly bright "flashy" look. */
			const auto iblIntensity = m_surfaceIBLIntensity.empty() ? "1.0" : m_surfaceIBLIntensity;
			const auto albedo = m_surfaceAlbedo.empty() ? "vec3(1.0)" : m_surfaceAlbedo + ".rgb";
			const auto metalness = m_surfaceMetalness.empty() ? "0.0" : m_surfaceMetalness;
			const auto code = (std::stringstream{} <<
				"/* Low-quality PBR IBL - F0 approximation without Fresnel. */" "\n"
				"const vec3 lqF0 = mix(vec3(0.04), " << albedo << ", " << metalness << ");" "\n" <<
				m_fragmentColor << ".rgb += " << m_surfaceReflectionColor << ".rgb * lqF0 * " << m_surfaceReflectionAmount << " * " << iblIntensity << ";").str();
			Code{fragmentShader, Location::Output} << code;
		}
		else if ( m_usePBRMode && m_useRefraction )
		{
			/* NOTE: PBR low-quality fallback for refraction-only materials.
			 * Refraction is less affected by F0 - use a subtle blend. */
			const auto iblIntensity = m_surfaceIBLIntensity.empty() ? "1.0" : m_surfaceIBLIntensity;
			Code{fragmentShader} << m_fragmentColor << ".rgb += " << m_surfaceRefractionColor << ".rgb * " << m_surfaceRefractionAmount << " * 0.96 * " << iblIntensity << ";";
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
				Code{fragmentShader} << m_fragmentColor << ".rgb += " << m_surfaceAutoIlluminationColor << ".rgb * " << m_surfaceAutoIlluminationAmount << ";";
			}
			else
			{
				/* Legacy/Phong mode: use diffuse color as emissive base. */
				Code{fragmentShader} << m_fragmentColor << ".rgb += " << m_surfaceDiffuseColor << ".rgb * " << m_surfaceAutoIlluminationAmount << ";";
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
