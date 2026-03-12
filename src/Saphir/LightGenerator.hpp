/*
 * src/Saphir/LightGenerator.hpp
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
	 * @brief PCF (Percentage-Closer Filtering) method for shadow mapping.
	 */
	enum class PCFMethod : uint32_t
	{
		/** @brief Uniform grid sampling (legacy method, can produce banding artifacts). */
		Grid = 0,
		/** @brief Vogel spiral with per-fragment rotation (recommended, best quality/performance ratio). */
		VogelDisk = 1,
		/** @brief Pre-computed Poisson disk distribution (good quality, fixed pattern). */
		PoissonDisk = 2,
		/** @brief Optimized textureGather usage (4x fewer texture fetches, good for high sample counts). */
		OptimizedGather = 3
	};

	/**
	 * @brief Converts a string to a PCFMethod enum value.
	 * @param method The string representation ("Performance", "Balanced", "Quality", "Ultra").
	 * @return PCFMethod The corresponding enum value. Defaults to VogelDisk if unknown.
	 */
	[[nodiscard]]
	inline
	PCFMethod
	stringToPCFMethod (const std::string & method) noexcept
	{
		if ( method == "Performance" )
		{
			return PCFMethod::Grid;
		}

		if ( method == "Quality" )
		{
			return PCFMethod::PoissonDisk;
		}

		if ( method == "Ultra" )
		{
			return PCFMethod::OptimizedGather;
		}

		/* Balanced or unknown -> VogelDisk (recommended) */
		return PCFMethod::VogelDisk;
	}

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
			 * @brief Low quality base reflectivity (F0) factor for dielectric materials.
			 * @note In low quality mode, Fresnel effect is not computed per-fragment.
			 * This boosted value (0.5 vs physically correct 0.04) compensates for the
			 * missing view-dependent Fresnel, providing more visible reflections.
			 */
			static constexpr auto LowQualityDielectricF0{0.5F};

			/**
			 * @brief Construct the light model generator.
			 * @param settings A reference to the settings.
			 * @param renderPassType The render pass type to know which kind of render is implied.
			 * @param highQualityEnabled Whether high quality rendering is enabled.
			 * @param fragmentColor The fragment color name produced at the end of the light application. Default "fragmentColor".
			 */
			LightGenerator (Settings & settings, Graphics::RenderPassType renderPassType, bool highQualityEnabled, const char * fragmentColor = FragmentColor) noexcept
				: m_renderPassType{renderPassType},
				m_PCFSample{settings.getOrSetDefault< uint32_t >(GraphicsShadowMappingPCFSamplesKey, DefaultGraphicsShadowMappingPCFSamples)},
				m_PCFMethod{stringToPCFMethod(settings.getOrSetDefault< std::string >(GraphicsShadowMappingPCFMethodKey, DefaultGraphicsShadowMappingPCFMethod))},
				m_fragmentColor{fragmentColor},
				m_useStaticLighting{m_renderPassType == Graphics::RenderPassType::SimplePass},
				m_highQualityEnabled{highQualityEnabled},
				m_PCFEnabled{settings.getOrSetDefault< bool >(GraphicsShadowMappingEnablePCFKey, DefaultGraphicsShadowMappingEnablePCF)}
			{

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
				m_useStaticLighting = true;
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

				m_useOpacity = true;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface auto-illumination (Phong mode).
			 * @param amountVariableName A reference to a string for GLSL variable holding the surface auto-illumination amount.
			 * @return void
			 */
			void
			declareSurfaceAutoIllumination (const std::string & amountVariableName) noexcept
			{
				m_surfaceAutoIlluminationAmount = amountVariableName;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface auto-illumination (PBR mode).
			 * @param colorVariableName A reference to a string for GLSL variable holding the surface auto-illumination color.
			 * @param amountVariableName A reference to a string for GLSL variable holding the surface auto-illumination amount.
			 * @return void
			 */
			void
			declareSurfaceAutoIllumination (const std::string & colorVariableName, const std::string & amountVariableName) noexcept
			{
				m_surfaceAutoIlluminationColor = colorVariableName;
				m_surfaceAutoIlluminationAmount = amountVariableName;
				m_useAutoIllumination = true;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the baked ambient occlusion.
			 * @param valueVariableName A reference to a string for GLSL variable holding the AO value (0.0-1.0).
			 * @param intensityVariableName A reference to a string for GLSL variable holding the AO intensity multiplier.
			 * @return void
			 */
			void
			declareSurfaceAmbientOcclusion (const std::string & valueVariableName, const std::string & intensityVariableName) noexcept
			{
				m_surfaceAmbientOcclusion = valueVariableName;
				m_surfaceAOIntensity = intensityVariableName;
				m_useAmbientOcclusion = true;
			}

			/**
			 * @brief Returns whether normal mapping is active for this light generator.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			usesNormalMapping () const noexcept
			{
				return m_useNormalMapping;
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

				m_useNormalMapping = true;
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

				m_useNormalMapping = true;
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

				m_useReflection = true;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface refraction map sampler and amount.
			 * @param colorVariableName A reference to string for the GLSL variable holding the surface refraction sample.
			 * @param amountVariableName A reference to string for the GLSL variable holding the refraction amount. Default 0.5.
			 * @param iorVariableName A reference to string for the GLSL variable holding the refraction IOR. Default 1.0.
			 * @return void
			 */
			void
			declareSurfaceRefraction (const std::string & colorVariableName, const std::string & amountVariableName = {}, const std::string & iorVariableName = {}) noexcept
			{
				m_surfaceRefractionColor = colorVariableName;

				if ( amountVariableName.empty() )
				{
					m_surfaceRefractionAmount = "(0.0)";
				}
				else
				{
					m_surfaceRefractionAmount = amountVariableName;
				}

				if ( iorVariableName.empty() )
				{
					m_surfaceRefractionIOR = "(1.0)";
				}
				else
				{
					m_surfaceRefractionIOR = iorVariableName;
				}

				m_useRefraction = true;
			}

			/* ==================== PBR Mode ==================== */

			/**
			 * @brief Enables PBR (Physically Based Rendering) mode.
			 * @note When PBR mode is enabled, the light generator uses Cook-Torrance BRDF
			 *       instead of Phong-Blinn shading.
			 * @return void
			 */
			void
			enablePBRMode () noexcept
			{
				m_usePBRMode = true;
			}

			/**
			 * @brief Returns whether PBR mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPBRMode () const noexcept
			{
				return m_usePBRMode;
			}

			/**
			 * @brief Returns whether high-quality reflection is enabled.
			 * @note When enabled, reflectionNormal and reflectionI are computed per-fragment.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			highQualityEnabled () const noexcept
			{
				return m_highQualityEnabled;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface albedo (base color).
			 * @param colorVariableName A reference to a string for GLSL variable holding the surface albedo.
			 * @return void
			 */
			void
			declareSurfaceAlbedo (const std::string & colorVariableName) noexcept
			{
				m_surfaceAlbedo = colorVariableName;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface roughness.
			 * @param valueVariableName A reference to a string for GLSL variable holding the surface roughness (0.0 = mirror, 1.0 = diffuse).
			 * @return void
			 */
			void
			declareSurfaceRoughness (const std::string & valueVariableName) noexcept
			{
				m_surfaceRoughness = valueVariableName;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the surface metalness.
			 * @param valueVariableName A reference to a string for GLSL variable holding the surface metalness (0.0 = dielectric, 1.0 = metal).
			 * @return void
			 */
			void
			declareSurfaceMetalness (const std::string & valueVariableName) noexcept
			{
				m_surfaceMetalness = valueVariableName;
			}

			/**
			 * @brief Declares the variable used by the fragment shader to get the IBL (Image-Based Lighting) intensity.
			 * @note This controls the contribution of environment cubemaps (reflection/refraction) in PBR mode.
			 * @param valueVariableName A reference to a string for GLSL variable holding the IBL intensity (0.0 = none, 1.0 = full).
			 * @return void
			 */
			void
			declareSurfaceIBLIntensity (const std::string & valueVariableName) noexcept
			{
				m_surfaceIBLIntensity = valueVariableName;
			}

			/**
			 * @brief Declares the variables used by the fragment shader for the clear coat layer.
			 * @param factorVariableName A reference to a string for GLSL variable holding the clear coat factor (0.0 = none, 1.0 = full coat).
			 * @param roughnessVariableName A reference to a string for GLSL variable holding the clear coat roughness (0.0 = mirror, 1.0 = diffuse).
			 * @return void
			 */
			void
			declareSurfaceClearCoat (const std::string & factorVariableName, const std::string & roughnessVariableName) noexcept
			{
				m_surfaceClearCoatFactor = factorVariableName;
				m_surfaceClearCoatRoughness = roughnessVariableName;
				m_useClearCoat = true;
			}

			/**
			 * @brief Declares the variable used by the fragment shader for the clear coat normal map.
			 * @param normalVariableName A reference to a string for GLSL variable holding the clear coat normal (tangent space).
			 * @return void
			 */
			void
			declareSurfaceClearCoatNormal (const std::string & normalVariableName) noexcept
			{
				m_surfaceClearCoatNormal = normalVariableName;
			}

			/**
			 * @brief Declares the variables used by the fragment shader for subsurface scattering.
			 * @param intensityVariableName A reference to a string for GLSL variable holding the SSS intensity.
			 * @param colorVariableName A reference to a string for GLSL variable holding the SSS color.
			 * @param radiusVariableName A reference to a string for GLSL variable holding the SSS scatter radius.
			 * @return void
			 */
			void
			declareSurfaceSubsurface (const std::string & intensityVariableName, const std::string & colorVariableName, const std::string & radiusVariableName) noexcept
			{
				m_surfaceSubsurfaceIntensity = intensityVariableName;
				m_surfaceSubsurfaceColor = colorVariableName;
				m_surfaceSubsurfaceRadius = radiusVariableName;
				m_useSubsurface = true;
			}

			/**
			 * @brief Declares the variable used by the fragment shader for the subsurface thickness map.
			 * @param thicknessVariableName A reference to a string for GLSL variable holding per-pixel thickness.
			 * @return void
			 */
			void
			declareSurfaceSubsurfaceThickness (const std::string & thicknessVariableName) noexcept
			{
				m_surfaceSubsurfaceThickness = thicknessVariableName;
				m_useSubsurfaceThicknessMap = true;
			}

			/**
			 * @brief Declares the variables used by the fragment shader for the sheen layer.
			 * @param colorVariableName A reference to a string for GLSL variable holding the sheen color.
			 * @param roughnessVariableName A reference to a string for GLSL variable holding the sheen roughness.
			 * @return void
			 */
			void
			declareSurfaceSheen (const std::string & colorVariableName, const std::string & roughnessVariableName) noexcept
			{
				m_surfaceSheenColor = colorVariableName;
				m_surfaceSheenRoughness = roughnessVariableName;
				m_useSheen = true;
			}

			/**
			 * @brief Declares surface anisotropy for anisotropic specular highlights.
			 * @param anisotropyVariableName A reference to a string for GLSL variable holding the anisotropy value (-1..1).
			 * @param rotationVariableName A reference to a string for GLSL variable holding the anisotropy rotation (0..1).
			 * @return void
			 */
			void
			declareSurfaceAnisotropy (const std::string & anisotropyVariableName, const std::string & rotationVariableName) noexcept
			{
				m_surfaceAnisotropy = anisotropyVariableName;
				m_surfaceAnisotropyRotation = rotationVariableName;
				m_useAnisotropy = true;
			}

			/**
			 * @brief Declares per-pixel anisotropy direction from a texture (KHR_materials_anisotropy).
			 * @param directionVariableName A reference to a string for GLSL vec2 variable holding the tangent-space direction.
			 * @return void
			 */
			void
			declareSurfaceAnisotropyDirection (const std::string & directionVariableName) noexcept
			{
				m_surfaceAnisotropyDirection = directionVariableName;
			}

			/**
			 * @brief Declares the variables used by the fragment shader for PBR transmission.
			 * @param factorVariableName A reference to a string for GLSL variable holding the transmission factor (0.0 = opaque, 1.0 = fully transmissive).
			 * @param attenuationColorVariableName A reference to a string for GLSL variable holding the Beer's law attenuation color.
			 * @param transmissionColorVariableName A reference to a string for GLSL variable holding the sampled transmission color (from cubemap).
			 * @param attenuationDistanceVariableName A reference to a string for GLSL variable holding the Beer's law attenuation distance.
			 * @param thicknessVariableName A reference to a string for GLSL variable holding the material thickness.
			 * @return void
			 */
			void
			declareSurfaceTransmission (const std::string & factorVariableName, const std::string & transmissionColorVariableName, const std::string & attenuationColorVariableName, const std::string & attenuationDistanceVariableName, const std::string & thicknessVariableName) noexcept
			{
				m_surfaceTransmissionFactor = factorVariableName;
				m_surfaceTransmissionColor = transmissionColorVariableName;
				m_surfaceAttenuationColor = attenuationColorVariableName;
				m_surfaceAttenuationDistance = attenuationDistanceVariableName;
				m_surfaceThicknessFactor = thicknessVariableName;
				m_useTransmission = true;
			}

			/**
			 * @brief Declares the variables used by the fragment shader for PBR iridescence (thin-film interference).
			 * @param factorVariableName A reference to a string for GLSL variable holding the iridescence factor (0.0-1.0).
			 * @param iorVariableName A reference to a string for GLSL variable holding the thin film IOR.
			 * @param thicknessMinVariableName A reference to a string for GLSL variable holding the minimum film thickness (nm).
			 * @param thicknessMaxVariableName A reference to a string for GLSL variable holding the maximum film thickness (nm).
			 * @return void
			 */
			void
			declareSurfaceIridescence (const std::string & factorVariableName, const std::string & iorVariableName, const std::string & thicknessMinVariableName, const std::string & thicknessMaxVariableName) noexcept
			{
				m_surfaceIridescenceFactor = factorVariableName;
				m_surfaceIridescenceIOR = iorVariableName;
				m_surfaceIridescenceThicknessMin = thicknessMinVariableName;
				m_surfaceIridescenceThicknessMax = thicknessMaxVariableName;
				m_useIridescence = true;
			}

			/**
			 * @brief Declares the material IOR for physically-correct dielectric F0 computation.
			 * @note When set, F0 = ((ior-1)/(ior+1))^2 replaces the default 0.04 for dielectrics (KHR_materials_ior).
			 * @param iorVariableName The GLSL expression for the material IOR.
			 */
			void
			declareSurfaceMaterialIOR (const std::string & iorVariableName) noexcept
			{
				m_surfaceMaterialIOR = iorVariableName;
				m_useMaterialIOR = true;
			}

			/**
			 * @brief Declares the variables used for KHR_materials_specular (factor + color tint for dielectric F0).
			 * @param factorVariableName The GLSL expression for the specular factor.
			 * @param colorVariableName The GLSL expression for the specular color factor (vec4).
			 */
			void
			declareSurfaceKHRSpecular (const std::string & factorVariableName, const std::string & colorVariableName) noexcept
			{
				m_surfaceKHRSpecularFactor = factorVariableName;
				m_surfaceKHRSpecularColor = colorVariableName;
				m_useKHRSpecular = true;
			}

			/**
			 * @brief Declares the emissive strength HDR multiplier (KHR_materials_emissive_strength).
			 * @param variableName The GLSL expression for the emissive strength value.
			 */
			void
			declareSurfaceEmissiveStrength (const std::string & variableName) noexcept
			{
				m_surfaceEmissiveStrength = variableName;
			}

			/**
			 * @brief Returns a GLSL expression for the surface roughness.
			 * @note For PBR materials, returns the roughness variable directly.
			 * For Phong materials, converts shininess to roughness using the Beckmann approximation.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string roughnessShaderExpression () const noexcept;

			/**
			 * @brief Returns a GLSL expression for the surface metalness.
			 * @note For PBR materials, returns the metalness variable directly.
			 * For non-PBR materials, returns "0.0" (dielectric).
			 * @return std::string
			 */
			[[nodiscard]]
			std::string metalnessShaderExpression () const noexcept;

			/**
			 * @brief Returns a GLSL expression for the final view-space normal.
			 * @note When normal mapping is active, returns "N" (the perturbed normal
			 * computed by the PBR lighting code). Otherwise, returns the interpolated
			 * geometric normal (svNormalViewSpace).
			 * @return std::string
			 */
			[[nodiscard]]
			std::string finalNormalViewSpaceExpression () const noexcept;

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
			 * @param useShadowMap States the use of a shadow map.
			 * @param useColorProjection States the use of a color projection texture.
			 * @return Declaration::UniformBlock
			 */
			[[nodiscard]]
			static Declaration::UniformBlock getUniformBlock (uint32_t set, uint32_t binding, Graphics::LightType lightType, bool useShadowMap, bool useColorProjection) noexcept;

			/**
			 * @brief Returns a uniform block for a directional light with Cascaded Shadow Maps.
			 * @param set The set index.
			 * @param binding The binding point in the set.
			 * @param cascadeCount The number of cascades (1-4).
			 * @return Declaration::UniformBlock
			 */
			[[nodiscard]]
			static Declaration::UniformBlock getUniformBlockCSM (uint32_t set, uint32_t binding, uint32_t cascadeCount = 4) noexcept;

		private:

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
			 * @param shadowCubemap State the shadow map is a cubemap.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateVertexShaderShadowMapCode (Generator::Abstract & generator, VertexShader & vertexShader, bool shadowCubemap) const noexcept;

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
			bool generateGouraudVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, Graphics::LightType lightType, bool enableShadowMap, bool enableColorProjection) const noexcept;

			/**
			 * @brief Generates the fragment shader for a light using the Gouraud shading technic.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @param enableColorProjection Enables the color projection code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateGouraudFragmentShader (Generator::Abstract & generator, FragmentShader & fragmentShader, Graphics::LightType lightType, bool enableShadowMap, bool enableColorProjection) const noexcept;

			/**
			 * @brief Generates the vertex shader for a light using the Phong-Blinn shading technic.
			 * @param generator A reference to the shader generator.
			 * @param vertexShader A reference to the vertex shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @param enableColorProjection Enables the color projection code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generatePhongBlinnVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, Graphics::LightType lightType, bool enableShadowMap, bool enableColorProjection) const noexcept;

			/**
			 * @brief Generates the fragment shader for a light using the Phong-Blinn shading technic.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @param enableColorProjection Enables the color projection code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generatePhongBlinnFragmentShader (Generator::Abstract & generator, FragmentShader & fragmentShader, Graphics::LightType lightType, bool enableShadowMap, bool enableColorProjection) const noexcept;

			/**
			 * @brief Generates the vertex shader for a directional light using the Phong-Blinn shading technic and normal mapping.
			 * @param generator A reference to the shader generator.
			 * @param vertexShader A reference to the vertex shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @param enableColorProjection Enables the color projection code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generatePhongBlinnWithNormalMapVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, Graphics::LightType lightType, bool enableShadowMap, bool enableColorProjection) const noexcept;

			/**
			 * @brief Generates the fragment shader for a directional light using the Phong-Blinn shading technic and normal mapping.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @param enableColorProjection Enables the color projection code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generatePhongBlinnWithNormalMapFragmentShader (Generator::Abstract & generator, FragmentShader & fragmentShader, Graphics::LightType lightType, bool enableShadowMap, bool enableColorProjection) const noexcept;

			/**
			 * @brief Generates the vertex shader for a light using PBR Cook-Torrance BRDF.
			 * @param generator A reference to the shader generator.
			 * @param vertexShader A reference to the vertex shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @param enableColorProjection Enables the color projection code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generatePBRVertexShader (Generator::Abstract & generator, VertexShader & vertexShader, Graphics::LightType lightType, bool enableShadowMap, bool enableColorProjection) const noexcept;

			/**
			 * @brief Generates the fragment shader for a light using PBR Cook-Torrance BRDF.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader.
			 * @param lightType The light type.
			 * @param enableShadowMap Enables the shadow mapping code generation.
			 * @param enableColorProjection Enables the color projection code generation.
			 * @return bool
			 */
			[[nodiscard]]
			bool generatePBRFragmentShader (const Generator::Abstract & generator, FragmentShader & fragmentShader, Graphics::LightType lightType, bool enableShadowMap, bool enableColorProjection) const noexcept;

			/**
			 * @brief Generates the PBR BRDF helper functions (Fresnel, NDF, Geometry).
			 * @param fragmentShader A reference to the fragment shader.
			 * @return void
			 */
			void generatePBRBRDFFunctions (FragmentShader & fragmentShader) const noexcept;

			/**
			 * @brief
			 * @param shadowMap
			 * @param fragmentPosition
			 * @return std::string
			 */
			[[nodiscard]]
			std::string generate2DShadowMapCode (const std::string & shadowMap, const std::string & fragmentPosition) const noexcept;

			/**
			 * @brief
			 * @param shadowMap
			 * @param fragmentPosition
			 * @return std::string
			 */
			[[nodiscard]]
			std::string generate2DShadowMapPCFCode (const std::string & shadowMap, const std::string & fragmentPosition) const noexcept;

			/**
			 * @brief
			 * @param shadowMap
			 * @param directionWorldSpace
			 * @param nearFar
			 * @return std::string
			 */
			[[nodiscard]]
			std::string generate3DShadowMapCode (const std::string & shadowMap, const std::string & directionWorldSpace, const std::string & nearFar) const noexcept;

			/**
			 * @brief
			 * @param shadowMap
			 * @param directionWorldSpace
			 * @param nearFar
			 * @return std::string
			 */
			[[nodiscard]]
			std::string generate3DShadowMapPCFCode (const std::string & shadowMap, const std::string & directionWorldSpace, const std::string & nearFar) const noexcept;

			/**
			 * @brief Generates the Cascaded Shadow Map sampling code for directional lights.
			 * @param shadowMapArray The name of the sampler2DArrayShadow uniform.
			 * @param fragmentPositionWorldSpace The fragment position in world space.
			 * @param viewMatrix The view matrix uniform to compute view-space depth.
			 * @param cascadeMatrices The array of cascade view-projection matrices.
			 * @param splitDistances The cascade split distances (view-space depths).
			 * @param cascadeCount The number of cascades.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string generateCSMShadowMapCode (const std::string & shadowMapArray, const std::string & fragmentPositionWorldSpace, const std::string & viewMatrix, const std::string & cascadeMatrices, const std::string & splitDistances, const std::string & cascadeCount) const noexcept;

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
			uint32_t m_PCFSample{0};
			PCFMethod m_PCFMethod{PCFMethod::Grid};
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
			std::string m_surfaceRefractionColor;
			std::string m_surfaceRefractionAmount;
			std::string m_surfaceRefractionIOR;
			/* PBR-specific variables. */
			std::string m_surfaceAlbedo;
			std::string m_surfaceRoughness;
			std::string m_surfaceMetalness;
			std::string m_surfaceIBLIntensity;
			std::string m_surfaceAutoIlluminationColor;
			std::string m_surfaceAmbientOcclusion;
			std::string m_surfaceAOIntensity;
			std::string m_surfaceClearCoatFactor;
			std::string m_surfaceClearCoatRoughness;
			std::string m_surfaceClearCoatNormal;
			/* SSS-specific variables. */
			std::string m_surfaceSubsurfaceIntensity;
			std::string m_surfaceSubsurfaceColor;
			std::string m_surfaceSubsurfaceRadius;
			std::string m_surfaceSubsurfaceThickness;
			std::string m_surfaceSheenColor;
			std::string m_surfaceSheenRoughness;
			std::string m_surfaceAnisotropy;
			std::string m_surfaceAnisotropyRotation;
			std::string m_surfaceAnisotropyDirection;
			/* Transmission-specific variables. */
			std::string m_surfaceTransmissionFactor;
			std::string m_surfaceTransmissionColor;
			std::string m_surfaceAttenuationColor;
			std::string m_surfaceAttenuationDistance;
			std::string m_surfaceThicknessFactor;
			/* Material IOR variable (KHR_materials_ior). */
			std::string m_surfaceMaterialIOR;
			/* Iridescence-specific variables. */
			std::string m_surfaceIridescenceFactor;
			std::string m_surfaceIridescenceIOR;
			std::string m_surfaceIridescenceThicknessMin;
			std::string m_surfaceIridescenceThicknessMax;
			/* KHR_materials_specular variables. */
			std::string m_surfaceKHRSpecularFactor;
			std::string m_surfaceKHRSpecularColor;
			/* KHR_materials_emissive_strength variable. */
			std::string m_surfaceEmissiveStrength;
			const StaticLighting * m_staticLighting{nullptr};
			bool m_discardUnlitFragment{true};
			bool m_useStaticLighting{false};
			bool m_useNormalMapping{false};
			bool m_useOpacity{false};
			bool m_useReflection{false};
			bool m_useRefraction{false};
			bool m_enableAmbientNoise{false};
			bool m_usePBRMode{false};
			bool m_useAutoIllumination{false};
			bool m_useAmbientOcclusion{false};
			bool m_useClearCoat{false};
			bool m_useSubsurface{false};
			bool m_useSubsurfaceThicknessMap{false};
			bool m_useSheen{false};
			bool m_useAnisotropy{false};
			bool m_useTransmission{false};
			bool m_useIridescence{false};
			bool m_useKHRSpecular{false};
			bool m_useMaterialIOR{false};
			bool m_highQualityEnabled{false};
			bool m_PCFEnabled{false};
	};
}
