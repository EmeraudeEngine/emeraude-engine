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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <algorithm>
#include <string>

/* Local inclusions for usages. */
#include "Declaration/UniformBlock.hpp"
#include "Graphics/Types.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"

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
	enum class PCFMethod : std::uint8_t
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
	 * @note Usage contract: the render pass type (ambient, or one directional/point/spot variant,
	 * possibly with shadow map / color projection / CSM) is fixed at construction and drives every
	 * subsequent call. Between construction and generateVertexShaderCode()/generateFragmentShaderCode(),
	 * the material must call the relevant declareSurfaceXxx() setters to advertise which GLSL
	 * variables carry each surface property; a property never declared falls back to a neutral
	 * default baked into the corresponding *ShaderExpression()/materialPropertiesExpression() helper.
	 * There is a single lighting model (Cook-Torrance, evaluated per fragment); the former
	 * Blinn-Phong/Gouraud machinery and the shader-quality setting are gone — the only remaining
	 * quality switch is Generator::Abstract::highQualityEnabled(), a per-program rendering-distance
	 * decision, not a user setting.
	 */
	class LightGenerator final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LightGenerator"};

			/** @brief Default GLSL variable name for the fragment color produced by this generator. */
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
			 * @param fragmentColor The fragment color name produced at the end of the light application. Default "fragmentColor".
			 */
			LightGenerator (Settings & settings, Graphics::RenderPassType renderPassType, const char * fragmentColor = FragmentColor) noexcept
				: m_renderPassType{renderPassType},
				m_PCFSample{settings.getOrSetDefault< uint32_t >(GraphicsShadowMappingPCFSamplesKey, DefaultGraphicsShadowMappingPCFSamples)},
				/* NOTE: the initialiser order must follow the declaration order (-Wreorder, an
				 * error in the Debug configuration): m_cascadeBlendRatio and m_normalOffsetScale
				 * are declared before m_PCFMethod and m_fragmentColor. */
				m_cascadeBlendRatio{std::clamp(settings.getOrSetDefault< float >(GraphicsShadowMappingCascadeBlendRatioKey, DefaultGraphicsShadowMappingCascadeBlendRatio), 0.0F, 0.5F)},
				m_normalOffsetScale{std::max(0.0F, settings.getOrSetDefault< float >(GraphicsShadowMappingNormalOffsetScaleKey, DefaultGraphicsShadowMappingNormalOffsetScale))},
				m_PCFMethod{stringToPCFMethod(settings.getOrSetDefault< std::string >(GraphicsShadowMappingPCFMethodKey, DefaultGraphicsShadowMappingPCFMethod))},
				m_fragmentColor{fragmentColor},
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
			 * @warning colorVariableName is stored but currently read by no generator: the Blinn-Phong
			 * specular codegen that used to consume it was deleted with the rest of that machinery. Only
			 * shininessAmountVariableName still has an effect, through roughnessShaderExpression()'s
			 * Beckmann-style shininess-to-roughness fallback for a material that never declares roughness.
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
			 * @brief Declares the surface auto-illumination as a single scalar amount, tinted by the
			 * surface diffuse/albedo color rather than by its own color.
			 * @note Unlike the two-argument overload below, this does not set m_useAutoIllumination, so
			 * materialPropertiesExpression() will not publish an emissive mask for it. No current caller
			 * uses this overload (StandardResource always goes through the two-argument, PBR-tinted one).
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
			 * @brief Declares the variable holding the surface's atmospheric-fog response.
			 * @note Packed into the material-properties G-buffer A channel, high nibble, and read
			 * by AtmosphericFog. Undeclared means 1.0 — fully fogged.
			 * @param valueVariableName A reference to a string for the GLSL variable.
			 * @return void
			 */
			void
			declareSurfaceFogResponse (const std::string & valueVariableName) noexcept
			{
				m_surfaceFogResponse = valueVariableName;
			}

			/**
			 * @brief Declares the variable holding the surface's depth-of-field response.
			 * @note Packed into the material-properties G-buffer A channel, low nibble, and read
			 * by DepthOfField. Undeclared means 1.0 — fully defocused.
			 * @param valueVariableName A reference to a string for the GLSL variable.
			 * @return void
			 */
			void
			declareSurfaceDoFMask (const std::string & valueVariableName) noexcept
			{
				m_surfaceDoFMask = valueVariableName;
			}

			/**
			 * @brief Declares a per-pixel reflectivity map for the G-buffer material properties output.
			 * @param valueVariableName The GLSL variable name of the sampled reflectivity map (luminance).
			 * @return void
			 */
			void
			declareSurfaceReflectivityMap (const std::string & valueVariableName) noexcept
			{
				m_surfaceReflectivityMap = valueVariableName;
				m_useReflectivityMap = true;
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
			 * @brief Declares the surface reflection as ARTISTIC: an explicitly authored
			 * cubemap texture the post-processing must never replace.
			 * @note The material then publishes a ZERO reflectivity to the G-buffer nibble,
			 * keeping SSR/RTR off this surface. The scene-coherent modes (environment "auto",
			 * render-target probe) stay overridable — see docs/reflection-pipeline.md
			 * ("reflection cost ladder"). An explicit ReflectivityMap keeps priority over
			 * this flag: an artist asking for per-pixel post-process control is obeyed.
			 * @return void
			 */
			void
			declareReflectionArtistic () noexcept
			{
				m_reflectionArtistic = true;
			}

			/**
			 * @brief Declares the surface reflection source as an ABSOLUTE luminance — a
			 * render target (probe/mirror), i.e. the RENDERED SCENE re-read.
			 * @note The environment luminance scale then NEVER applies to the reflected
			 * color: only what comes out of the normalized SKY cubemap gets it. Measured
			 * before this: a probe reflection burned to 8000 nits under a clear-sky manifest.
			 * @return void
			 */
			void
			declareReflectionSourceAbsolute () noexcept
			{
				m_reflectionSourceAbsolute = true;
			}

			/**
			 * @brief Declares the surface refraction source as an ABSOLUTE luminance (render target).
			 * @copydetails declareReflectionSourceAbsolute()
			 * @return void
			 */
			void
			declareRefractionSourceAbsolute () noexcept
			{
				m_refractionSourceAbsolute = true;
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
			 * @param transmissionIsSceneRadiance Set when the transmission color was sampled from the
			 * GRAB PASS rather than from the environment cubemap. It changes the UNIT of the source and
			 * therefore whether it must be scaled by the environment luminance — see
			 * transmissionIsSceneRadiance(). Default false (cubemap).
			 * @return void
			 */
			void
			declareSurfaceTransmission (const std::string & factorVariableName, const std::string & transmissionColorVariableName, const std::string & attenuationColorVariableName, const std::string & attenuationDistanceVariableName, const std::string & thicknessVariableName, bool transmissionIsSceneRadiance = false) noexcept
			{
				m_surfaceTransmissionFactor = factorVariableName;
				m_surfaceTransmissionColor = transmissionColorVariableName;
				m_surfaceAttenuationColor = attenuationColorVariableName;
				m_surfaceAttenuationDistance = attenuationDistanceVariableName;
				m_surfaceThicknessFactor = thicknessVariableName;
				m_useTransmission = true;
				m_transmissionIsSceneRadiance = transmissionIsSceneRadiance;
			}

			/**
			 * @brief Returns whether the transmitted color is already an absolute scene luminance.
			 *
			 * The environment cubemap is a NORMALIZED [0,1] source, so a reflection or a transmission
			 * sampled from it must be multiplied by the sky luminance to become a luminance. The GRAB
			 * PASS is not: it is a copy of the rendered scene, in nits, so scaling it again multiplies
			 * the whole scene by the sky luminance a second time.
			 *
			 * @return bool
			 */
			[[nodiscard]]
			bool
			transmissionIsSceneRadiance () const noexcept
			{
				return m_transmissionIsSceneRadiance;
			}

			/**
			 * @brief Declares the variables used by the fragment shader for PBR iridescence (thin-film interference).
			 * @param factorVariableName A reference to a string for GLSL variable holding the iridescence factor (0.0-1.0).
			 * @param iorVariableName A reference to a string for GLSL variable holding the thin film IOR.
			 * @param thicknessMinVariableName A reference to a string for GLSL variable holding the minimum film thickness (nm).
			 * @param thicknessMaxVariableName A reference to a string for GLSL variable holding the maximum film thickness (nm).
			 * @param thicknessMapVariableName A reference to a string for the GLSL variable holding the
			 * thickness map's G channel, already resolved to [0,1]. Leave EMPTY when the material
			 * declares no thickness map: the spec's fallback is then the MAXIMUM thickness, which is
			 * what iridescenceThicknessExpression() returns.
			 * @return void
			 */
			void
			declareSurfaceIridescence (const std::string & factorVariableName, const std::string & iorVariableName, const std::string & thicknessMinVariableName, const std::string & thicknessMaxVariableName, const std::string & thicknessMapVariableName = {}) noexcept
			{
				m_surfaceIridescenceFactor = factorVariableName;
				m_surfaceIridescenceIOR = iorVariableName;
				m_surfaceIridescenceThicknessMin = thicknessMinVariableName;
				m_surfaceIridescenceThicknessMax = thicknessMaxVariableName;
				m_surfaceIridescenceThicknessMap = thicknessMapVariableName;
				m_useIridescence = true;
			}

			/**
			 * @brief Returns the GLSL expression giving the thin film thickness in nanometres.
			 * @note ⚠️ Both the ambient pass and the light passes MUST use this — they used to
			 * disagree, `mix(min, max, 0.5)` in the ambient against `mix(min, max, 1.0)` in the
			 * light passes, which gave the same surface two different films depending on the pass.
			 * @return std::string
			 */
			/**
			 * @brief Returns the GLSL expression for the DIELECTRIC F0, as a scalar.
			 * @note Derived from KHR_materials_ior when the material carries one, and further
			 * weighted by KHR_materials_specular's factor when present. Falls back to the 0.04 of
			 * a common dielectric.
			 * @warning ⚠️ Three ambient branches used to hardcode 0.04 and ignore the material IOR
			 * outright — a glass authored at ior 1.7 reflected like ior 1.5. Use this, never a
			 * literal.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			dielectricF0Expression () const noexcept
			{
				if ( !m_useMaterialIOR )
				{
					return "0.04";
				}

				const auto base = "pow((" + m_surfaceMaterialIOR + " - 1.0) / (" + m_surfaceMaterialIOR + " + 1.0), 2.0)";

				if ( m_useKHRSpecular )
				{
					return "min(" + base + " * " + m_surfaceKHRSpecularFactor + ", 1.0)";
				}

				return base;
			}

			/**
			 * @brief Emits the GLSL declaring an ambient-pass Fresnel term as a vec3.
			 * @note ⚠️⚠️ THE single place where iridescence enters an ambient Fresnel. Four ambient
			 * branches compute a Fresnel split — refraction, reflection+transmission, reflection
			 * only, thin-surface transmission — and only ONE of them used to know about
			 * iridescence, so a surface that was both iridescent and transmissive rendered
			 * iridescent in the light passes and plainly dielectric in the ambient one. Route every
			 * split through here; never write a Schlick term inline again.
			 * @note KHR_materials_iridescence REPLACES the Fresnel term, it is not layered on top:
			 * the thin film IS the reflectance of the interface.
			 * @param variableName The name of the vec3 to declare.
			 * @param F0Expression The GLSL expression giving F0, as a vec3.
			 * @param NdotVExpression The GLSL expression giving the cosine of the view angle.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			ambientFresnelDeclaration (const std::string & variableName, const std::string & F0Expression, const std::string & NdotVExpression) const noexcept
			{
				const auto F0 = variableName + "F0";

				std::string code{"const vec3 " + F0 + " = " + F0Expression + ";\n"};

				const auto schlick = F0 + " + (vec3(1.0) - " + F0 + ") * pow(1.0 - " + NdotVExpression + ", 5.0)";

				if ( !m_useIridescence )
				{
					return code + "const vec3 " + variableName + " = " + schlick + ";\n";
				}

				return code +
					"const vec3 " + variableName + " = mix(" + schlick + ", evalIridescence(1.0, " +
					m_surfaceIridescenceIOR + ", " + NdotVExpression + ", " + this->iridescenceThicknessExpression() + ", " + F0 + "), " +
					m_surfaceIridescenceFactor + ");\n";
			}

			[[nodiscard]]
			std::string
			iridescenceThicknessExpression () const noexcept
			{
				/* ⚠️ SPEC, not a placeholder: without a thickness map KHR_materials_iridescence
				 * says the film thickness is the MAXIMUM, never the midpoint. */
				const auto weight = m_surfaceIridescenceThicknessMap.empty() ? std::string{"1.0"} : m_surfaceIridescenceThicknessMap;

				return "mix(" + m_surfaceIridescenceThicknessMin + ", " + m_surfaceIridescenceThicknessMax + ", " + weight + ")";
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
			 * @brief Returns a GLSL vec4 expression for the surface albedo (base color).
			 * @note For PBR materials, returns the albedo variable (texture sample or uniform).
			 * For Standard/Basic materials, returns the diffuse color variable.
			 * Falls back to white (neutral for indirect-light modulation).
			 * @return std::string
			 */
			[[nodiscard]]
			std::string albedoShaderExpression () const noexcept;

			/**
			 * @brief Returns a GLSL vec3 expression for the surface DIFFUSE albedo.
			 * @note ⚠️ This is what the albedo G-buffer attachment carries, and it is NOT the
			 * base color: it is the base color times the energy the diffuse lobe actually gets,
			 * `baseColor * (1 - metalness) * (1 - transmission)`. The attachment exists for ONE
			 * job — re-modulating a demodulated indirect-diffuse signal (SSGI, RTGI) at full
			 * resolution — and that signal is irradiance: multiplying it by the base color lights
			 * a metal, which has no diffuse lobe, and a transmissive surface, whose penetrating
			 * light leaves through the other side. Same convention as NVIDIA NRD (its
			 * demodulation albedo is `baseColor * saturate(1 - metalness)`) and as the glTF
			 * dielectric BRDF, which mixes the diffuse lobe INTO the transmission by the
			 * transmission factor rather than adding to it.
			 * @note Measured: a `KHR_materials_transmission` glass, whose default base color is
			 * WHITE and which overwrites the G-buffer of everything behind it, turned into an
			 * opaque milky plate under RTGI — it was receiving the full sky irradiance as if it
			 * were a Lambertian sheet of paper.
			 * @note A material declaring neither metalness nor transmission generates the very
			 * same expression as before, hence a bit-identical shader.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string diffuseAlbedoShaderExpression () const noexcept;

			/**
			 * @brief Returns a GLSL vec4 expression for the material properties G-buffer output.
			 * @note Encodes reflectivity, AO response, emissive mask and other properties
			 * as nibble-packed values based on the declared surface properties.
			 * Falls back to neutral defaults for undeclared properties.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string materialPropertiesExpression () const noexcept;

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
			 * @todo The switch on lightType has no case for a light type outside
			 * {Directional, Point, Spot}; the default branch silently returns an invalid,
			 * zero-initialized block (set 0, binding 0, no type/usage) instead of asserting
			 * or reporting an error ("TODO: Fix this!" in the implementation).
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
			 * @note When the program carries the bindless set, this is where the IBL lands:
			 * diffuse irradiance (reserved cube slot 1) and split-sum specular (BRDF LUT,
			 * reserved 2D slot 3) with the Fdez-Agüera multi-scatter compensation.
			 * @param generator A reference to the shader generator (bindless set access).
			 * @param fragmentShader A reference to the fragment shader.
			 * @return void
			 */
			void generateAmbientFragmentShader (Generator::Abstract & generator, FragmentShader & fragmentShader) const noexcept;

			/**
			 * @brief Common code to assemble light component results into the final fragment color.
			 * @param fragmentShader A reference to the fragment shader.
			 * @param diffuseFactor A reference to a string.
			 * @param specularFactor A reference to a string.
			 * @return bool
			 * @warning Vestigial declaration: this method has no implementation left in any
			 * LightGenerator*.cpp file and no caller anywhere in the codebase. It carried the
			 * Blinn-Phong (!m_usePBRMode) diffuse/specular assembly path, deleted along with the
			 * Gouraud/Phong generators (see src/Saphir/AGENTS.md). Kept here only because
			 * documentation may not remove declarations; do not implement it expecting the old
			 * Blinn-Phong contract to still apply — that decision belongs to the project owner.
			 */
			[[nodiscard]]
			bool generateFinalFragmentOutput (FragmentShader & fragmentShader, const std::string & diffuseFactor, const std::string & specularFactor) const noexcept;







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
			 * @brief Generates the single-sample (no PCF) shadow test for a 2D shadow map (directional or spot light).
			 * @note Skips the test (shadowFactor = 1.0, fully lit) when the projected fragment position
			 * falls outside the shadow map's clip-space depth range [0, w]. When m_discardUnlitFragment
			 * is set, a fully-shadowed fragment (shadowFactor <= 0.0) is discarded rather than shaded black.
			 * @param shadowMap The GLSL expression naming the sampler2DShadow uniform.
			 * @param fragmentPosition The GLSL expression for the fragment position in light clip space (vec4), sampled with textureProj().
			 * @return std::string The generated GLSL code declaring and computing `shadowFactor`.
			 */
			[[nodiscard]]
			std::string generate2DShadowMapCode (const std::string & shadowMap, const std::string & fragmentPosition) const noexcept;

			/**
			 * @brief Generates the Percentage-Closer Filtered shadow test for a 2D shadow map (directional or spot light).
			 * @note Same clip-space depth-range skip and m_discardUnlitFragment behavior as
			 * generate2DShadowMapCode(). The sampling pattern depends on m_PCFMethod (Grid/VogelDisk/
			 * PoissonDisk/OptimizedGather) and the sample count/radius on m_PCFSample and the light's
			 * PCFRadius uniform.
			 * @param shadowMap The GLSL expression naming the sampler2DShadow uniform.
			 * @param fragmentPosition The GLSL expression for the fragment position in light clip space (vec4).
			 * @return std::string The generated GLSL code declaring and computing `shadowFactor`.
			 */
			[[nodiscard]]
			std::string generate2DShadowMapPCFCode (const std::string & shadowMap, const std::string & fragmentPosition, const std::string & fragmentPositionWorldSpace) const noexcept;

			/**
			 * @brief Generates the single-sample (no PCF) shadow test for a 3D (cubemap) shadow map, used by point lights.
			 * @note Skips filtering: compares the cubemap-sampled linear depth (scaled by nearFar.y, the
			 * light's far plane) against the fragment's distance from the light, with a minimum bias of
			 * 0.005. When m_discardUnlitFragment is set, a fully-shadowed fragment is discarded.
			 * @param shadowMap The GLSL expression naming the samplerCubeShadow (depth) cubemap uniform.
			 * @param directionWorldSpace The GLSL expression for the world-space vector from the fragment to the light.
			 * @param nearFar The GLSL expression for a vec2(near, far) of the shadow-casting light's projection.
			 * @return std::string The generated GLSL code declaring and computing `shadowFactor`.
			 */
			[[nodiscard]]
			std::string generate3DShadowMapCode (const std::string & shadowMap, const std::string & directionWorldSpace, const std::string & nearFar) const noexcept;

			/**
			 * @brief Generates the Percentage-Closer Filtered shadow test for a 3D (cubemap) shadow map, used by point lights.
			 * @note Same shadowFactor/m_discardUnlitFragment behavior as generate3DShadowMapCode(). The
			 * sampling pattern depends on m_PCFMethod: Grid and VogelDisk build a genuine 3D sample set
			 * around the lookup direction; PoissonDisk uses a fixed 20-point sphere; OptimizedGather has
			 * no cubemap equivalent of textureGather and falls back to the PoissonDisk sphere. The filter
			 * radius is the light-to-fragment distance scaled by the light's PCFRadius uniform.
			 * @param shadowMap The GLSL expression naming the samplerCubeShadow (depth) cubemap uniform.
			 * @param directionWorldSpace The GLSL expression for the world-space vector from the fragment to the light.
			 * @param nearFar The GLSL expression for a vec2(near, far) of the shadow-casting light's projection.
			 * @return std::string The generated GLSL code declaring and computing `shadowFactor`.
			 */
			[[nodiscard]]
			std::string generate3DShadowMapPCFCode (const std::string & shadowMap, const std::string & directionWorldSpace, const std::string & nearFar, const std::string & fragmentPositionWorldSpace) const noexcept;

			/**
			 * @brief Generates the Cascaded Shadow Map sampling code for directional lights.
			 * @note Cascade selection walks the cascades in order and picks the first whose split
			 * distance exceeds the fragment's view-space depth, falling through to the last cascade
			 * otherwise. PCF (m_PCFSample × m_PCFSample grid) is used only when m_PCFEnabled is set;
			 * otherwise a single sampler2DArrayShadow tap is taken. As with the other shadow-map
			 * generators, a fully-shadowed fragment is discarded when m_discardUnlitFragment is set.
			 * @param shadowMapArray The GLSL expression naming the sampler2DArrayShadow uniform.
			 * @param fragmentPositionWorldSpace The GLSL expression for the fragment position in world space (vec3).
			 * @param fragmentPositionViewSpace The GLSL expression for the fragment position in view space (vec3 or vec4), used to derive the depth for cascade selection. This is the interpolated fragment position, not a matrix — re-deriving it from world space would require the view matrix, unavailable to the main render target's fragment shader.
			 * @param cascadeMatrices The GLSL expression naming the array of cascade view-projection matrices.
			 * @param splitDistances The GLSL expression naming the array of cascade split distances (view-space depths).
			 * @param cascadeCount The GLSL expression for the number of active cascades.
			 * @return std::string The generated GLSL code declaring and computing `shadowFactor`.
			 */
			[[nodiscard]]
			std::string generateCSMShadowMapCode (const std::string & shadowMapArray, const std::string & fragmentPositionWorldSpace, const std::string & fragmentPositionViewSpace, const std::string & cascadeMatrices, const std::string & splitDistances, const std::string & cascadeCount, const std::string & shadowBias, const std::string & normalWorldSpace, const std::string & lightDirectionWorldSpace, FragmentShader & fragmentShader) const noexcept;

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
			/** @brief Name of the per-light vertex-to-fragment interstage output block (see variable()). */
			static constexpr auto LightBlock{"LightBlock"};
			/** @brief GLSL variable name for the combined light visibility factor (radius falloff × spot cone × shadow), applied before the BRDF. */
			static constexpr auto LightFactor{"lightFactor"};
			/**
			 * @warning Unused: no LightGenerator*.cpp file references this constant. It named the
			 * Lambertian diffuse weight in the removed Blinn-Phong path (see AGENTS.md,
			 * generateFinalFragmentOutput()) and has no role in the current Cook-Torrance model.
			 */
			static constexpr auto DiffuseFactor{"diffuseFactor"};
			/**
			 * @warning Unused: no LightGenerator*.cpp file references this constant. It named the
			 * specular weight in the removed Blinn-Phong path (see AGENTS.md,
			 * generateFinalFragmentOutput()) and has no role in the current Cook-Torrance model.
			 */
			static constexpr auto SpecularFactor{"specularFactor"};
			/** @brief GLSL variable name for the light position transformed to view space (point/spot lights). */
			static constexpr auto LightPositionViewSpace{"lightPositionViewSpace"};
			/** @brief Interstage member name for the spotlight cone axis direction, in view space. */
			static constexpr auto SpotLightDirectionViewSpace{"spotLightDirectionViewSpace"};
			/** @brief GLSL variable/interstage member name for the normalized view-space direction from the fragment to the light. */
			static constexpr auto RayDirectionViewSpace{"rayDirectionViewSpace"};
			/**
			 * @warning Unused: no LightGenerator*.cpp file references this constant. Dead alongside
			 * DiffuseFactor/SpecularFactor.
			 */
			static constexpr auto RayDirectionTextureSpace{"rayDirectionTextureSpace"};
			/** @brief Interstage member name for the un-normalized view-space vector from the fragment to the light (its length is the light distance). */
			static constexpr auto Distance{"distance"};

			Graphics::RenderPassType m_renderPassType;
			uint32_t m_PCFSample{0};
			/** @brief Cascade cross-fade band, as a fraction of the cascade depth range. 0 = no blend. */
			float m_cascadeBlendRatio{0.0F};
			/** @brief Normal-offset distance in shadow TEXELS. 0 = disabled, and the varying is not even requested. */
			float m_normalOffsetScale{0.0F};
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
			std::string m_surfaceFogResponse;
			std::string m_surfaceDoFMask;
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
			std::string m_surfaceIridescenceThicknessMap; /* ⚠️ EMPTY = no map = the film thickness is the MAXIMUM (spec fallback), not a neutral value. */
			/* KHR_materials_specular variables. */
			std::string m_surfaceKHRSpecularFactor;
			std::string m_surfaceKHRSpecularColor;
			/* KHR_materials_emissive_strength variable. */
			std::string m_surfaceEmissiveStrength;
			/* Reflectivity map variable (per-pixel reflectivity for G-buffer). */
			std::string m_surfaceReflectivityMap;
			/**
			 * @note No declareXxx() setter currently exposes this flag: it is always its default
			 * (true) for every LightGenerator instance. Kept as a member (rather than a hard-coded
			 * constant) because all five shadow-map code generators branch on it to decide whether a
			 * fully-shadowed fragment is discarded, but nothing in the public API can turn it off yet.
			 */
			bool m_discardUnlitFragment{true};
			bool m_useNormalMapping{false};
			bool m_useOpacity{false};
			bool m_useReflection{false};
			/** @brief Explicitly authored cubemap reflection: never replaced by SSR/RTR (zero nibble). */
			bool m_reflectionArtistic{false};
			/** @brief Reflection source is a render target: absolute luminance, no environment luminance scale. */
			bool m_reflectionSourceAbsolute{false};
			/** @brief Refraction source is a render target: absolute luminance, no environment luminance scale. */
			bool m_refractionSourceAbsolute{false};
			bool m_useRefraction{false};
			/**
			 * @note No declareXxx() setter currently exposes this flag: it is always its default
			 * (false) for every LightGenerator instance. When true, it would multiply the ambient
			 * light intensity by a screen-space hash (per-pixel dithering) in generateAmbientFragmentShader().
			 */
			bool m_enableAmbientNoise{false};
			bool m_useAutoIllumination{false};
			bool m_useAmbientOcclusion{false};
			bool m_useClearCoat{false};
			bool m_useSubsurface{false};
			bool m_useSubsurfaceThicknessMap{false};
			bool m_useSheen{false};
			bool m_useAnisotropy{false};
			bool m_useTransmission{false};
			bool m_transmissionIsSceneRadiance{false};
			bool m_useIridescence{false};
			bool m_useKHRSpecular{false};
			bool m_useMaterialIOR{false};
			bool m_useReflectivityMap{false};
			bool m_PCFEnabled{false};
			/**
			 * @brief Returns the IBL weight expression, scaled by the environment luminance.
			 * @note The environment cubemap is a normalized [0,1] source (the image pipeline has no
			 * HDR format), so everything reflecting it would contribute a fraction of a nit in a
			 * scene lit in thousands — i.e. no visible reflections at all. The luminance, in nits,
			 * is read from the view uniform block (fed by the scene background), so it can change
			 * at runtime without regenerating any program.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			scaledIBLIntensity () const noexcept
			{
				const auto weight = m_surfaceIBLIntensity.empty() ? std::string{"1.0"} : m_surfaceIBLIntensity;

				return '(' + weight + " * " + ViewUB(Keys::UniformBlock::Component::EnvironmentLuminance, false) + ')';
			}

			/**
			 * @brief Returns the GLSL intensity factor for the REFLECTED color leg.
			 * @note A render-target source (probe/mirror) is the rendered scene — already an
			 * absolute luminance: only the artistic weight applies. A cubemap source is a
			 * normalized [0,1] texture: the environment luminance turns it into nits.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			reflectionIntensity () const noexcept
			{
				if ( m_reflectionSourceAbsolute )
				{
					const auto weight = m_surfaceIBLIntensity.empty() ? std::string{"1.0"} : m_surfaceIBLIntensity;

					return '(' + weight + ')';
				}

				return this->scaledIBLIntensity();
			}

			/**
			 * @brief Returns the GLSL intensity factor for the REFRACTED color leg.
			 * @copydetails reflectionIntensity()
			 * @return std::string
			 */
			[[nodiscard]]
			std::string
			refractionIntensity () const noexcept
			{
				if ( m_refractionSourceAbsolute )
				{
					const auto weight = m_surfaceIBLIntensity.empty() ? std::string{"1.0"} : m_surfaceIBLIntensity;

					return '(' + weight + ')';
				}

				return this->scaledIBLIntensity();
			}
	};
}
