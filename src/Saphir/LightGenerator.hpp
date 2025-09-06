/*
 * src/Saphir/LightGenerator.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <array>
#include <string>

/* Local inclusions for usages. */
#include "Declaration/UniformBlock.hpp"
#include "StaticLighting.hpp"
#include "Settings.hpp"
#include "SettingKeys.hpp"

/* Forward declarations. */
namespace EmEn::Saphir
{
	namespace Generator
	{
		class Abstract;
	}

	class VertexShader;
	class FragmentShader;
}

namespace EmEn::Saphir
{
	/**
	 * @brief The light model generator is responsible for generating GLSL lighting code independently of a light processor.
	 */
	class LightGenerator final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LightGenerator"};

			static constexpr auto FragmentColor{"fragmentColor"};

			/**
			 * @brief Construct the light model generator.
			 * @param settings A reference to the settings.
			 * @param renderPassType The render pass type to know which kind of render is implied.
			 * @param fragmentColor The fragment color name produced at the end of the light application. Default "fragmentColor".
			 */
			LightGenerator (Settings & settings, Graphics::RenderPassType renderPassType, const char * fragmentColor = FragmentColor) noexcept
				: m_renderPassType{renderPassType},
				m_PCFSample{settings.getOrSetDefault< uint32_t >(GraphicsShadowMappingPCFSampleKey, DefaultGraphicsShadowMappingPCFSample)},
				m_PCFRadius{settings.getOrSetDefault< float >(GraphicsShadowMappingPCFRadiusKey, DefaultGraphicsShadowMappingPCFRadius)},
				m_fragmentColor{fragmentColor}
			{
				if ( m_renderPassType == Graphics::RenderPassType::SimplePass )
				{
					m_flags[UseStaticLighting] = true;
				}
			}

			/**
			 * @brief Returns whether this is generating the ambient pass.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAmbientPass () const noexcept
			{
				return m_renderPassType == Graphics::RenderPassType::AmbientPass;
			}

			/**
			 * @brief Sets a static lighting to use.
			 * @param staticLighting A pointer to a static lighting.
			 * @return void
			 */
			void
			setStaticLighting (const StaticLighting * staticLighting) noexcept
			{
				m_staticLighting = staticLighting;
				m_flags[UseStaticLighting] = true;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface ambient color.
			 * @param colorVariableName A reference to a string for GLSL variable holding the surface ambient color.
			 * @return void
			 */
			void
			declareSurfaceAmbient (const std::string & colorVariableName) noexcept
			{
				m_surfaceAmbientColor = colorVariableName;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface diffuse color.
			 * @param colorVariableName A reference to a string for GLSL variable holding the surface diffuse color.
			 * @return void
			 */
			void
			declareSurfaceDiffuse (const std::string & colorVariableName) noexcept
			{
				m_surfaceDiffuseColor = colorVariableName;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface specular color and option.
			 * @param colorVariableName A reference to a string for GLSL variable holding the surface specular color.
			 * @param shininessAmountVariableName A reference to a string for GLSL variable holding the surface shininess factor. Default, 200.0.
			 * @return void
			 */
			void
			declareSurfaceSpecular (const std::string & colorVariableName, const std::string & shininessAmountVariableName = {}) noexcept
			{
				m_surfaceSpecularColor = colorVariableName;

				if ( shininessAmountVariableName.empty() )
				{
					m_surfaceShininessAmount = "(200.0)";
				}
				else
				{
					m_surfaceShininessAmount = shininessAmountVariableName;
				}
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface opacity.
			 * @param amountVariableName A reference to a string for the GLSL variable holding the surface opacity amount.
			 * @return void
			 */
			void
			declareSurfaceOpacity (const std::string & amountVariableName) noexcept
			{
				m_surfaceOpacityAmount = amountVariableName;

				m_flags[UseOpacity] = true;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface auto-illumination.
			 * @param amountVariableName A reference to a string for GLSL variable holding the surface auto-illumination.
			 * @return void
			 */
			void
			declareSurfaceAutoIllumination (const std::string & amountVariableName) noexcept
			{
				m_surfaceAutoIlluminationAmount = amountVariableName;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the current sample from the normal map.
			 * @param vectorVariableName A reference to string for GLSL variable holding the surface normal.
			 * @return void
			 */
			void
			declareSurfaceNormal (const std::string & vectorVariableName) noexcept
			{
				m_surfaceNormalVector = vectorVariableName;

				m_flags[UseNormalMapping] = true;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface normal map sampler.
			 * @param normalMap A reference to string for GLSL variable holding the surface normal map.
			 * @param textureCoordinates A reference to the used texture coordinates. Default, the first one.
			 * @param scale A reference to string for the GLSL variable holding the normal map scale. Default 1.0.
			 * @return void
			 */
			void
			declareSurfaceNormalMapSampler (const std::string & normalMap, const std::string & textureCoordinates = {}, const std::string & scale = {}) noexcept
			{
				m_normalMap = normalMap;
				m_normalMapTextureCoordinates = textureCoordinates;

				if ( scale.empty() )
				{
					m_normalMapScale = "1.0";
				}
				else
				{
					m_normalMapScale = scale;
				}

				m_flags[UseNormalMapping] = true;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface reflection map sampler and amount.
			 * @param colorVariableName A reference to string for the GLSL variable holding the surface reflection sample.
			 * @param amountVariableName A reference to string for the GLSL variable holding the reflection amount. Default 0.5.
			 * @return void
			 */
			void
			declareSurfaceReflection (const std::string & colorVariableName, const std::string & amountVariableName = {}) noexcept
			{
				m_surfaceReflectionColor = colorVariableName;

				if ( amountVariableName.empty() )
				{
					m_surfaceReflectionAmount = "(0.5)";
				}
				else
				{
					m_surfaceReflectionAmount = amountVariableName;
				}

				m_flags[UseReflection] = true;
			}

			/**
			 * @brief Returns the variable name of the produced fragment color.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			fragmentColor () const noexcept
			{
				return m_fragmentColor;
			}

			/**
			 * @brief Generates the vertex shader light code.
			 * @param generator A reference to the shader generator.
			 * @param vertexShader A reference to the vertex shader.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateVertexShaderCode (Generator::Abstract & generator, VertexShader & vertexShader) const noexcept;

			/**
			 * @brief Generates the fragment shader light code.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateFragmentShaderCode (Generator::Abstract & generator, FragmentShader & fragmentShader) const noexcept;

			/**
			 * @brief Returns a uniform block for a light type.
			 * @param set The set index.
			 * @param binding The binding point in the set.
			 * @param lightType The type of light.
			 * @param useShadowMap States the use of a shadow.
			 * @return Declaration::UniformBlock
			 */
			[[nodiscard]]
			static Declaration::UniformBlock getUniformBlock (uint32_t set, uint32_t binding, Graphics::LightType lightType, bool useShadowMap) noexcept;

		private:

			enum class DepthTextureFunction: int
			{
				Texture = 0,
				TextureProj = 1,
				TextureGather = 2
			};

			/**
			 * @brief Returns the real light pass type.
			 * @return Graphics::RenderPassType
			 */
			[[nodiscard]]
			Graphics::RenderPassType checkRenderPassType () const noexcept;

			/**
			 * @brief Generate the vertex shader code to fetch data from a shadow map.
			 * @param generator A reference to the shader generator.
			 * @param vertexShader A reference to the vertex shader.
			 * @param isCubemap Define the shadow map as a cubemap.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateVertexShaderShadowMapCode (Generator::Abstract & generator, VertexShader & vertexShader, bool isCubemap) const noexcept;

			/**
			 * @brief Generates the ambient component light which is the same for every light.
			 * @param fragmentShader A reference to the fragment shader.
			 * @return void
			 */
			void generateAmbientFragmentShader (FragmentShader & fragmentShader) const noexcept;

			/**
			 * @brief Common code to assemble light component results into the final fragment color.
			 * @param fragmentShader A reference to the fragment shader.
			 * @param diffuseFactor A reference to a string.
			 * @param specularFactor A reference to a string.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateFinalFragmentOutput (FragmentShader & fragmentShader, const std::string & diffuseFactor, const std::string & specularFactor) const noexcept;

			/**
			 * @brief Generates the vertex shader for a light using the Gouraud shading technic.
			 * @param generator A reference to the shader generator.
			 * @param vertexShader A reference to the vertex shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateGouraudVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, Graphics::LightType lightType, bool enableShadowMap) const noexcept;

			/**
			 * @brief Generates the fragment shader for a light using the Gouraud shading technic.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateGouraudFragmentShader (Generator::Abstract & generator, FragmentShader & fragmentShader, Graphics::LightType lightType, bool enableShadowMap) const noexcept;

			/**
			 * @brief Generates the vertex shader for a light using the Phong-Blinn shading technic.
			 * @param generator A reference to the shader generator.
			 * @param vertexShader A reference to the vertex shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generatePhongBlinnVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, Graphics::LightType lightType, bool enableShadowMap) const noexcept;

			/**
			 * @brief Generates the fragment shader for a light using the Phong-Blinn shading technic.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generatePhongBlinnFragmentShader (Generator::Abstract & generator, FragmentShader & fragmentShader, Graphics::LightType lightType, bool enableShadowMap) const noexcept;

			/**
			 * @brief Generates the vertex shader for a directional light using the Phong-Blinn shading technic and normal mapping.
			 * @param generator A reference to the shader generator.
			 * @param vertexShader A reference to the vertex shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generatePhongBlinnWithNormalMapVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, Graphics::LightType lightType, bool enableShadowMap) const noexcept;

			/**
			 * @brief Generates the fragment shader for a directional light using the Phong-Blinn shading technic and normal mapping.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generatePhongBlinnWithNormalMapFragmentShader (Generator::Abstract & generator, FragmentShader & fragmentShader, Graphics::LightType lightType, bool enableShadowMap) const noexcept;

			/**
			 * @brief generate2DShadowMapCode
			 * @param shadowMap
			 * @param fragmentPosition
			 * @param function
			 * @return std::string
			 */
			[[nodiscard]]
			std::string generate2DShadowMapCode (const std::string & shadowMap, const std::string & fragmentPosition, DepthTextureFunction function) const noexcept;

			/**
			 * @brief generate3DShadowMapCode
			 * @param shadowMap
			 * @param directionWorldSpace
			 * @param nearFar
			 * @return std::string
			 */
			[[nodiscard]]
			std::string generate3DShadowMapCode (const std::string & shadowMap, const std::string & directionWorldSpace, const std::string & nearFar) const noexcept;

			/**
			 * @brief Returns the variable responsible for the light position in world space.
			 * @note Useful with point and spotlights.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string lightPositionWorldSpace () const noexcept;

			/**
			 * @brief Returns the variable responsible for the light direction in world space.
			 * @note Useful with directional and spotlights.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string lightDirectionWorldSpace () const noexcept;

			/**
			 * @brief Returns the variable responsible for the ambient light color.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string ambientLightColor () const noexcept;

			/**
			 * @brief Returns the variable responsible for the light ambient level.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string ambientLightIntensity () const noexcept;

			/**
			 * @brief Returns the variable responsible for the light level.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string lightIntensity () const noexcept;

			/**
			 * @brief Returns the variable responsible for the light radius.
			 * @note Useful with point and spotlights.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string lightRadius () const noexcept;

			/**
			 * @brief Returns the variable responsible for the cosine of the spot cone inner angle.
			 * @note Useful with spotlights.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string lightInnerCosAngle () const noexcept;

			/**
			 * @brief Returns the variable responsible for the cosine of the spot cone outer angle.
			 * @note Useful with spotlights.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string lightOuterCosAngle () const noexcept;

			/**
			 * @brief Returns the variable responsible for the light color.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string lightColor () const noexcept;

			/**
			 * @brief Gets the right component for a light interstage variable.
			 * @param componentName The component name of the light.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string variable (const char * componentName) noexcept;

			/* Flag names. */
			static constexpr auto DiscardUnlitFragment{0UL};
			static constexpr auto UseStaticLighting{1UL};
			static constexpr auto UseNormalMapping{2UL};
			static constexpr auto UseOpacity{3UL};
			static constexpr auto UseReflection{4UL};
			static constexpr auto EnableAmbientNoise{5UL};

			/* Light shader block-specific keys. */
			static constexpr auto LightBlock{"LightBlock"};
			static constexpr auto LightFactor{"lightFactor"};
			static constexpr auto DiffuseFactor{"diffuseFactor"};
			static constexpr auto SpecularFactor{"specularFactor"};
			static constexpr auto LightPositionViewSpace{"lightPositionViewSpace"};
			static constexpr auto SpotLightDirectionViewSpace{"spotLightDirectionViewSpace"};
			static constexpr auto RayDirectionViewSpace{"rayDirectionViewSpace"};
			static constexpr auto RayDirectionTextureSpace{"rayDirectionTextureSpace"};
			static constexpr auto Distance{"distance"};

			Graphics::RenderPassType m_renderPassType;
			unsigned int m_PCFSample{0};
			float m_PCFRadius{1.0F};
			std::string m_fragmentColor;
			std::string m_surfaceAmbientColor;
			std::string m_surfaceDiffuseColor;
			std::string m_surfaceSpecularColor;
			std::string m_surfaceShininessAmount;
			std::string m_surfaceOpacityAmount;
			std::string m_surfaceAutoIlluminationAmount;
			std::string m_normalMap;
			std::string m_normalMapScale;
			std::string m_normalMapTextureCoordinates;
			std::string m_surfaceNormalVector;
			std::string m_surfaceReflectionColor;
			std::string m_surfaceReflectionAmount;
			const StaticLighting * m_staticLighting{nullptr};
			/* FIXME: Remove unused flags. */
			std::array< bool, 8 > m_flags{
				true/*DiscardUnlitFragment*/,
				false/*UseStaticLighting*/,
				false/*UseNormalMapping*/,
				false/*UseOpacity*/,
				false/*UseReflection*/,
				false/*EnableAmbientNoise*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
