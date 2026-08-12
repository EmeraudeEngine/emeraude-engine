/*
 * src/Graphics/Material/StandardResource.hpp
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
#include <array>
#include <unordered_map>

/* Local inclusions for inheritances. */
#include "Interface.hpp"
#include "Component/Interface.hpp"

/* Local inclusions for usages. */
#include "Graphics/TextureResource/TextureCubemap.hpp"
#include "Physics/SurfacePhysicalProperties.hpp"
#include "PixelFactory/Types.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn
{
	namespace Vulkan
	{
		class TextureInterface;
	}

	namespace Graphics
	{
		namespace Material::Component
		{
			class Texture;
		}

		class SharedUniformBuffer;
	}

	namespace Resources
	{
		class Manager;
	}
}

namespace EmEn::Graphics::Material
{
	/**
	 * @brief PBR (Physically Based Rendering) material resource using the Metallic-Roughness workflow.
	 * @extends EmEn::Graphics::Material::Interface This is a material.
	 *
	 * This material implements the standard PBR metallic-roughness workflow with:
	 * - Albedo (base color)
	 * - Roughness (0.0 = mirror, 1.0 = diffuse)
	 * - Metalness (0.0 = dielectric, 1.0 = metal)
	 * - Normal mapping (optional)
	 * - Reflection/IBL via cubemap (optional)
	 */
	class EMEN_API StandardResource final : public Interface
	{
		friend class Resources::Container< StandardResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MaterialStandardResource"};

			/* Shader-specific keys. */
			static constexpr auto SurfaceAlbedoColor{"SurfaceAlbedoColor"};
			static constexpr auto SurfaceRoughness{"SurfaceRoughness"};
			static constexpr auto SurfaceMetalness{"SurfaceMetalness"};
			static constexpr auto SurfaceNormalVector{"SurfaceNormalVector"};
			static constexpr auto SurfaceReflectionColor{"SurfaceReflectionColor"};
			static constexpr auto SurfaceRefractionColor{"SurfaceRefractionColor"};
			static constexpr auto SurfaceAutoIlluminationColor{"SurfaceAutoIlluminationColor"};
			static constexpr auto SurfaceAmbientOcclusion{"SurfaceAmbientOcclusion"};
			static constexpr auto SurfaceClearCoatFactor{"SurfaceClearCoatFactor"};
			static constexpr auto SurfaceClearCoatRoughness{"SurfaceClearCoatRoughness"};
			static constexpr auto SurfaceClearCoatNormal{"SurfaceClearCoatNormal"};
			static constexpr auto SurfaceSubsurfaceIntensity{"SurfaceSubsurfaceIntensity"};
			static constexpr auto SurfaceSubsurfaceColor{"SurfaceSubsurfaceColor"};
			static constexpr auto SurfaceSubsurfaceThickness{"SurfaceSubsurfaceThickness"};
			static constexpr auto SurfaceSheenColor{"SurfaceSheenColor"};
			static constexpr auto SurfaceSheenRoughness{"SurfaceSheenRoughness"};
			static constexpr auto SurfaceAnisotropy{"SurfaceAnisotropy"};
			static constexpr auto SurfaceTransmissionFactor{"SurfaceTransmissionFactor"};
			static constexpr auto SurfaceTransmissionColor{"SurfaceTransmissionColor"};
			static constexpr auto SurfaceIridescenceFactor{"SurfaceIridescenceFactor"};
			static constexpr auto SurfaceHeightValue{"SurfaceHeight"};
			static constexpr auto SurfaceSpecularFactor{"SurfaceSpecularFactor"};
			static constexpr auto SurfaceSpecularColor{"SurfaceSpecularColor"};
			static constexpr auto SurfaceReflectivityMap{"SurfaceReflectivityMap"};
			static constexpr auto SurfaceOpacityAmount{"SurfaceOpacityAmount"};
			/** @brief The albedo AFTER the vertex-colour modulation; the single name every consumer reads. */
			static constexpr auto SurfaceAlbedoFinal{"SurfaceAlbedoFinal"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Few};

			/**
			 * @brief Constructs a PBR material.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			StandardResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags = 0) noexcept
				: Interface{serviceProvider, name, resourceFlags}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			StandardResource (const StandardResource & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			StandardResource (StandardResource && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return StandardResource &
			 */
			StandardResource & operator= (const StandardResource & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return StandardResource &
			 */
			StandardResource & operator= (StandardResource && copy) noexcept = delete;

			/**
			 * @brief Destructs the material.
			 */
			~StandardResource () override
			{
				this->destroy();
			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this);
			}

			/** @copydoc EmEn::Graphics::Material::Interface::isComplex() */
			[[nodiscard]]
			bool isComplex () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::exportRTMaterialData() */
			void exportRTMaterialData (GPURTMaterialData & outData) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::collectRTTextures() */
			void collectRTTextures (std::vector< RTTextureSlot > & outSlots) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::emissionMultiplier() */
			[[nodiscard]]
			std::string emissionMultiplier () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::requiresAlphaTestedShadows() */
			[[nodiscard]]
			bool requiresAlphaTestedShadows () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::generateShadowVertexCode() */
			[[nodiscard]]
			bool generateShadowVertexCode (const Saphir::Generator::Abstract & generator, Saphir::VertexShader & vertexShader) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::generateShadowAlphaTestCode() */
			[[nodiscard]]
			bool generateShadowAlphaTestCode (const Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::setupLightGenerator() */
			[[nodiscard]]
			bool setupLightGenerator (Saphir::LightGenerator & lightGenerator) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::generateVertexShaderCode() */
			[[nodiscard]]
			bool generateVertexShaderCode (Saphir::Generator::Abstract & generator, Saphir::VertexShader & vertexShader) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::LightGenerator & lightGenerator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::surfacePhysicalProperties() const */
			[[nodiscard]]
			const Physics::SurfacePhysicalProperties & surfacePhysicalProperties () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::surfacePhysicalProperties() */
			[[nodiscard]]
			Physics::SurfacePhysicalProperties & surfacePhysicalProperties () noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::frameCount() */
			[[nodiscard]]
			uint32_t frameCount () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::duration() */
			[[nodiscard]]
			uint32_t duration () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::frameIndexAt() */
			[[nodiscard]]
			uint32_t frameIndexAt (uint32_t sceneTime) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::enableBlending() */
			void enableBlending (BlendingMode mode) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::blendingMode() */
			[[nodiscard]]
			BlendingMode blendingMode () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::fragmentColor() */
			[[nodiscard]]
			std::string fragmentColor () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::descriptorSetLayout() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::DescriptorSetLayout > descriptorSetLayout () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::UBOIndex() */
			[[nodiscard]]
			uint32_t UBOIndex () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::UBOAlignment() */
			[[nodiscard]]
			uint32_t UBOAlignment () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::UBOOffset() */
			[[nodiscard]]
			uint32_t UBOOffset () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::descriptorSet() */
			[[nodiscard]]
			const Vulkan::DescriptorSet * descriptorSet () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::getUniformBlock() */
			[[nodiscard]]
			Saphir::Declaration::UniformBlock getUniformBlock (uint32_t set, uint32_t binding) const noexcept override;

			/* ==================== Component Setters (Pre-creation) ==================== */

			/**
			 * @brief Sets the albedo (base color) component as a color.
			 * @warning This function is available before creation time.
			 * @param color A reference to a color.
			 * @return bool
			 */
			bool setAlbedoComponent (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Sets the albedo (base color) component as a texture.
			 * @note A texture requesting 3D coordinates (a cubemap) propagates
			 * PrimaryTextureCoordinatesUses3D, so the generated shader samples it with the right
			 * coordinate type instead of compiling against 2D ones.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @param enableAlpha Sample the texture's alpha channel (blending, cutout). Default false.
			 * @return bool
			 */
			bool setAlbedoComponent (const std::shared_ptr< TextureResource::Abstract > & texture, bool enableAlpha = false) noexcept;

			/**
			 * @brief Sets the albedo (base color) component from a raw GPU texture (render target).
			 * @note For images the resource system does not own — a render target used as a
			 * surface (a screen showing a camera feed, a mirror-as-albedo). There is no resource
			 * dependency to register: the caller owns the lifetime and MUST keep the texture
			 * alive for as long as the material is used.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a GPU texture smart pointer.
			 * @param enableAlpha Sample the texture's alpha channel. Default false.
			 * @return bool
			 */
			bool setAlbedoComponentFromRenderTarget (const std::shared_ptr< Vulkan::TextureInterface > & texture, bool enableAlpha = false) noexcept;

			/**
			 * @brief Enables the per-vertex colour attribute, which MODULATES the albedo.
			 * @note glTF COLOR_0 semantics: the vertex colour MULTIPLIES the base colour. There is
			 * no second "vertex colours AS the albedo" mode — that is this same path with the
			 * neutral White albedo factor and no texture, i.e. the default state.
			 * @warning The geometry MUST carry the colour attribute (Geometry::EnableVertexColor);
			 * shader generation fails hard otherwise. This function is available before creation time.
			 * @return void
			 */
			void enableVertexColor () noexcept;

			/**
			 * @brief Declares the material UNLIT: no light pass runs over it, and its colour IS
			 * its emitted radiance (glTF KHR_materials_unlit semantics).
			 * @note For skyboxes, sprites, debug helpers and any content carrying its own BAKED
			 * lighting — re-lighting them double-counts light already present in the texel. The
			 * unlit path writes `fragmentColor().rgb * emissionMultiplier()`, so pair this with
			 * an AutoIllumination component to carry the luminance (without one the surface
			 * writes its raw [0,1] colour and reads black under photometric exposure).
			 * @warning This function is available before creation time.
			 * @return void
			 */
			void enableUnlit () noexcept;

			/**
			 * @brief Sets the roughness component as a value (0.0 = mirror, 1.0 = fully rough).
			 * @warning This function is available before creation time.
			 * @param value The roughness value between 0.0 and 1.0. Default 0.5.
			 * @return bool
			 */
			bool setRoughnessComponent (float value = DefaultRoughness) noexcept;

			/**
			 * @brief Sets the roughness component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @param value The roughness factor MULTIPLYING the sampled texel (glTF 'roughnessFactor' contract). Default 1.0 (neutral).
			 * @param invert If true, the texture is treated as a smoothness/gloss map and inverted (1.0 - texel) before the factor applies. Default false.
			 * @param sourceChannel The texel color channel holding the roughness (glTF packed metallic-roughness uses Green). Default Red.
			 * @return bool
			 */
			bool setRoughnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float value = DefaultTextureFactor, bool invert = false, Base::PixelFactory::Channel sourceChannel = Base::PixelFactory::Channel::Red) noexcept;

			/**
			 * @brief Sets the metalness component as a value (0.0 = dielectric, 1.0 = metal).
			 * @warning This function is available before creation time.
			 * @param value The metalness value between 0.0 and 1.0. Default 0.0.
			 * @return bool
			 */
			bool setMetalnessComponent (float value = DefaultMetalness) noexcept;

			/**
			 * @brief Sets the metalness component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @param value The metalness factor MULTIPLYING the sampled texel (glTF 'metallicFactor' contract). Default 1.0 (neutral).
			 * @param sourceChannel The texel color channel holding the metalness (glTF packed metallic-roughness uses Blue). Default Red.
			 * @return bool
			 */
			bool setMetalnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float value = DefaultTextureFactor, Base::PixelFactory::Channel sourceChannel = Base::PixelFactory::Channel::Red) noexcept;

			/**
			 * @brief Sets the UV transform of a texture component (KHR_texture_transform).
			 * @warning This function is available before creation time. The rotation part of the
			 * extension is NOT supported (logged and ignored by the loaders).
			 * @note Applied in the shader as 'uv * scale + offset'. Stored on the component
			 * (single source of truth, JSON "UVW"/"UVWOffset" keys land there too) and synced
			 * to the material UBO slots at creation time.
			 * @param componentType The targeted component (Albedo, Roughness, Metalness, Normal, AmbientOcclusion, AutoIllumination).
			 * @param scale The UV scale factors.
			 * @param offset The UV offsets.
			 * @return bool True when the component exists as a texture and supports a transform slot.
			 */
			bool setComponentUVWTransform (ComponentType componentType, const Base::Math::Vector< 2, float > & scale, const Base::Math::Vector< 2, float > & offset) noexcept;

			/**
			 * @brief Sets the normal component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @param scale The scale value. Default 1.0.
			 * @return bool
			 */
			bool setNormalComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float scale = DefaultNormalScale) noexcept;

			/**
			 * @brief Sets the height component for parallax occlusion mapping.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a height map texture smart pointer.
			 * @param scale The height scale (depth of parallax effect). Default 0.05.
			 * @return bool
			 */
			bool setHeightComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float scale = DefaultHeightScale) noexcept;

			/**
			 * @brief Changes the height scale for parallax occlusion mapping.
			 * @note This is a dynamic property.
			 * @param value The height scale value.
			 * @return void
			 */
			void setHeightScale (float value) noexcept;

			/**
			 * @brief Sets the reflection/IBL component as a cubemap texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a cubemap texture smart pointer.
			 * @return bool
			 */
			bool setReflectionComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept;

			/**
			 * @brief Sets the reflection/IBL component using a render target (for dynamic cubemap).
			 * @warning This function is available before creation time.
			 * @param renderTarget A reference to a texture interface smart pointer.
			 * @return bool
			 */
			bool setReflectionComponentFromRenderTarget (const std::shared_ptr< Vulkan::TextureInterface > & renderTarget) noexcept;

			/**
			 * @brief Sets the reflection/IBL component using scene environment cubemap.
			 * @note When enabled, the material will use the scene's environment cubemap for reflection
			 * instead of a material-specific texture. This is resolved at render time.
			 * @param IBLIntensity The IBL intensity. Default 1.0.
			 * @return bool
			 */
			bool setReflectionComponentFromEnvironmentCubemap (float IBLIntensity = DefaultIBLIntensity) noexcept;

			/**
			 * @brief Sets a post-process-only reflection: no cubemap is sampled, the material
			 * merely publishes a reflectivity in the material-properties G-buffer so the
			 * screen-space/ray-traced effects (SSR, RTR) can reflect it.
			 * @note This is the C++ mirror of the @code {"Reflection": {"Type": "Value", "Amount": x}} @endcode
			 * manifest filling type. It is the isolation switch of the reflection pipeline: with it,
			 * anything visible in the reflection comes from the post-process stack and nothing else.
			 * @warning This function is available before creation time.
			 * @param amount The reflectivity published to the G-buffer, in [0,1]. Default 50%.
			 * @return bool
			 */
			bool setPostProcessReflectivity (float amount = 0.5F) noexcept;

			/**
			 * @brief Sets the refraction component using the scene's environment cubemap.
			 * @note When enabled, the material will use the scene's environment cubemap for refraction
			 * instead of a material-specific texture. This is resolved at render time.
			 * @param ior The index of refraction (1.0 = air, 1.33 = water, 1.5 = glass, 2.4 = diamond). Default glass.
			 * @return bool
			 */
			bool setRefractionComponentFromEnvironmentCubemap (float ior = DefaultIOR) noexcept;

			/**
			 * @brief Sets the refraction component as a cubemap texture for glass-like materials.
			 * @warning This function is available before creation time.
			 * @note Fresnel will automatically blend between reflection and refraction.
			 * @param texture A reference to a cubemap texture smart pointer.
			 * @param ior The index of refraction (1.0 = air, 1.33 = water, 1.5 = glass, 2.4 = diamond). Default glass.
			 * @return bool
			 */
			bool setRefractionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float ior = DefaultIOR) noexcept;

			/**
			 * @brief Sets the refraction component using a render target (for dynamic cubemap).
			 * @warning This function is available before creation time.
			 * @note Fresnel will automatically blend between reflection and refraction.
			 * @param renderTarget A reference to a texture interface smart pointer.
			 * @param ior The index of refraction. Default glass.
			 * @return bool
			 */
			bool setRefractionComponentFromRenderTarget (const std::shared_ptr< Vulkan::TextureInterface > & renderTarget, float ior = DefaultIOR) noexcept;

			/** @copydoc EmEn::Graphics::Material::Interface::useEnvironmentCubemap() const noexcept */
			[[nodiscard]]
			bool
			useEnvironmentCubemap () const noexcept override
			{
				return m_isUsingEnvironmentCubemap || m_isUsingEnvironmentCubemapForRefraction || m_isUsingEnvironmentCubemapForTransmission;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::requiresGrabPass() const noexcept */
			[[nodiscard]]
			bool
			requiresGrabPass () const noexcept override
			{
				return m_isUsingGrabPassForTransmission;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::samplesTexture() const noexcept */
			[[nodiscard]]
			bool samplesTexture (const Vulkan::TextureInterface * texture) const noexcept override;

			/**
			 * @brief Sets the auto-illumination (emissive) component as a value only.
			 * @note The emissive color is set to white so the amount alone drives the emission
			 * (PBR's default emissive color is black — behavioural parity with StandardResource,
			 * whose default is white).
			 * @warning This function is available before creation time.
			 * @param amount The intensity multiplier. Default 1.0.
			 * @return bool
			 */
			bool setAutoIlluminationComponent (float amount = DefaultAutoIlluminationAmount) noexcept;

			/**
			 * @brief Sets the auto-illumination (emissive) component as a color.
			 * @warning This function is available before creation time.
			 * @param color A reference to the emissive color.
			 * @param amount The intensity multiplier. Default 1.0.
			 * @return bool
			 */
			bool setAutoIlluminationComponent (const Base::PixelFactory::Color< float > & color, float amount = DefaultAutoIlluminationAmount) noexcept;

			/**
			 * @brief Sets the auto-illumination (emissive) component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @param amount The intensity multiplier. Default 1.0.
			 * @return bool
			 */
			bool setAutoIlluminationComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount = DefaultAutoIlluminationAmount) noexcept;

			/**
			 * @brief Sets the opacity component as a global value (rule 1: uniform transparency).
			 * @note Enables blending: the whole surface becomes uniformly translucent.
			 * @warning This function is available before creation time.
			 * @param amount The global opacity [0,1] (0.0 = invisible, 1.0 = opaque).
			 * @return bool
			 */
			bool setOpacityComponent (float amount) noexcept;

			/**
			 * @brief Sets the opacity component as a texture (rules 2 and 3).
			 * @note Grayscale mode (default): the map scales the alpha per pixel and the material
			 * blends (rule 3). Cutout mode: call enableAlphaTest() afterwards — texels below the
			 * threshold are discarded and the material STAYS OPAQUE (rule 2).
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer (grayscale opacity map, red channel).
			 * @param amount The opacity multiplier applied to the sampled texel. Default 1.0.
			 * @return bool
			 */
			bool setOpacityComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount = DefaultOpacity) noexcept;

			/**
			 * @brief Updates the global opacity amount (dynamic property).
			 * @param value The opacity [0,1].
			 * @return void
			 */
			void setOpacity (float value) noexcept;

			/**
			 * @brief Returns the global opacity amount.
			 * @return float
			 */
			[[nodiscard]]
			float
			opacity () const noexcept
			{
				return m_materialProperties[OpacityOffset];
			}

			/**
			 * @brief Declares the material a binary CUTOUT (glTF alphaMode MASK): fragments whose
			 * alpha falls below the threshold are discarded and the material STAYS OPAQUE.
			 * @note Opaque render list, depth write kept, no back-to-front sorting. The alpha
			 * source is the opacity texture component when present, the albedo texture alpha
			 * channel otherwise. The threshold lives in the material UBO (never a shader literal,
			 * per the program-cache contract), so it is configurable per material and at runtime.
			 * @param threshold The alpha cutoff [0,1]. Default 0.5 (glTF alphaCutoff default).
			 * @return void
			 */
			void enableAlphaTest (float threshold = DefaultAlphaThreshold) noexcept;

			/**
			 * @brief Updates the alpha-test threshold (dynamic property).
			 * @param threshold The alpha cutoff [0,1].
			 * @return void
			 */
			void setAlphaThresholdToDiscard (float threshold) noexcept;

			/**
			 * @brief Sets the artistic reflection mix amount (dynamic property, D2 override).
			 * @note Applies to the artistic texture/probe reflection modes only; the neutral 1.0
			 * leaves the mix BRDF-controlled. The environment IBL path keeps IBLIntensity as its knob.
			 * @param value The mix amount [0,1].
			 * @return void
			 */
			void setReflectionAmount (float value) noexcept;

			/**
			 * @brief Sets the artistic refraction mix amount (dynamic property, D2 override).
			 * @note Applies to the artistic texture refraction mode only; the neutral 1.0 leaves
			 * the blend Fresnel-controlled.
			 * @param value The mix amount [0,1].
			 * @return void
			 */
			void setRefractionAmount (float value) noexcept;

			/**
			 * @brief Returns the alpha-test threshold.
			 * @return float
			 */
			[[nodiscard]]
			float
			alphaThresholdToDiscard () const noexcept
			{
				return m_materialProperties[AlphaThresholdOffset];
			}

			/**
			 * @brief Sets the ambient occlusion component as a baked texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer (grayscale AO map).
			 * @param intensity The AO intensity (0.0 = no AO, 1.0 = full AO). Default 1.0.
			 * @return bool
			 */
			bool setAmbientOcclusionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float intensity = DefaultAOIntensity) noexcept;

			/**
			 * @brief Sets the reflectivity map component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer (reflectivity map).
			 * @return bool
			 */
			bool setReflectivityMapComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept;

			/**
			 * @brief Sets the clear coat component as scalar values.
			 * @warning This function is available before creation time.
			 * @param factor The clear coat factor (0.0 = no coat, 1.0 = full coat). Default 0.0.
			 * @param roughness The clear coat roughness (0.0 = mirror, 1.0 = diffuse). Default 0.0.
			 * @return bool
			 */
			bool setClearCoatComponent (float factor = DefaultClearCoatFactor, float roughness = DefaultClearCoatRoughness) noexcept;

			/**
			 * @brief Sets the clear coat factor component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer for the clear coat factor map.
			 * @param roughness The clear coat roughness value. Default 0.0.
			 * @return bool
			 */
			bool setClearCoatComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float roughness = DefaultClearCoatRoughness) noexcept;

			/**
			 * @brief Sets the clear coat roughness component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer for the clear coat roughness map.
			 * @param factor The clear coat factor value. Default 1.0.
			 * @return bool
			 */
			bool setClearCoatRoughnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float factor = 1.0F) noexcept;

			/**
			 * @brief Sets the clear coat normal component as a texture (KHR_materials_clearcoat).
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer for the clear coat normal map.
			 * @param scale The normal map scale factor. Default 1.0.
			 * @return bool
			 */
			bool setClearCoatNormalComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float scale = DefaultClearCoatNormalScale) noexcept;

			/**
			 * @brief Changes the clear coat normal map scale factor.
			 * @note This is a dynamic property.
			 * @param value The scale value.
			 * @return void
			 */
			void setClearCoatNormalScale (float value) noexcept;

			/**
			 * @brief Returns whether a material component is present.
			 * @param componentType The type of component.
			 * @return bool
			 */
			[[nodiscard]]
			bool isComponentPresent (ComponentType componentType) const noexcept;

			/* ==================== Dynamic Property Setters (Post-creation) ==================== */

			/**
			 * @brief Changes the albedo color.
			 * @note This is a dynamic property.
			 * @param color A reference to a color.
			 * @return void
			 */
			void setAlbedoColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Changes the roughness value.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0.
			 * @return void
			 */
			void setRoughness (float value) noexcept;

			/**
			 * @brief Changes the metalness value.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0.
			 * @return void
			 */
			void setMetalness (float value) noexcept;

			/**
			 * @brief Changes the normal mapping scale factor.
			 * @note This is a dynamic property.
			 * @param value A scale value.
			 * @return void
			 */
			void setNormalScale (float value) noexcept;

			/**
			 * @brief Changes the index of refraction.
			 * @note This is a dynamic property. Only effective if refraction component is present.
			 * @param value The IOR value (1.0 to 3.0).
			 * @return void
			 */
			void setIOR (float value) noexcept;

			/**
			 * @brief Changes the IBL (Image-Based Lighting) intensity.
			 * @note This is a dynamic property. Controls the contribution of environment cubemaps.
			 * @param value The IBL intensity (0.0 = none, 1.0 = full). Default 1.0.
			 * @return void
			 */
			void setIBLIntensity (float value) noexcept;

			/**
			 * @brief Changes the auto-illumination color.
			 * @note This is a dynamic property.
			 * @param color A reference to the emissive color.
			 * @return void
			 */
			void setAutoIlluminationColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Changes the auto-illumination intensity multiplier.
			 * @note This is a dynamic property.
			 * @param value The intensity multiplier.
			 * @return void
			 */
			void setAutoIlluminationAmount (float value) noexcept;

			/**
			 * @brief Changes the ambient occlusion intensity.
			 * @note This is a dynamic property.
			 * @param value The AO intensity (0.0 = no AO, 1.0 = full AO).
			 * @return void
			 */
			void setAOIntensity (float value) noexcept;

			/**
			 * @brief Changes the clear coat factor.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0.
			 * @return void
			 */
			void setClearCoatFactor (float value) noexcept;

			/**
			 * @brief Changes the clear coat roughness.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0.
			 * @return void
			 */
			void setClearCoatRoughness (float value) noexcept;

			/* ==================== Subsurface Scattering Component Setters (Pre-creation) ==================== */

			/**
			 * @brief Sets the subsurface scattering component as scalar values.
			 * @warning This function is available before creation time.
			 * @param intensity The SSS intensity (0.0 = none, 1.0 = full). Default 0.0.
			 * @param radius The scatter radius. Default 1.0.
			 * @param color The SSS color tint. Default reddish (skin-like).
			 * @return bool
			 */
			bool setSubsurfaceComponent (float intensity = DefaultSubsurfaceIntensity, float radius = DefaultSubsurfaceRadius, const Base::PixelFactory::Color< float > & color = DefaultSubsurfaceColor) noexcept;

			/**
			 * @brief Sets the subsurface thickness component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer for the thickness map.
			 * @return bool
			 */
			bool setSubsurfaceThicknessComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept;

			/* ==================== Sheen Component Setters (Pre-creation) ==================== */

			/**
			 * @brief Sets the sheen component as scalar values for fabric-like materials.
			 * @warning This function is available before creation time.
			 * @param color The sheen color tint (black = disabled). Default black.
			 * @param roughness The sheen roughness (0.0 = satin, 1.0 = wool). Default 0.5.
			 * @return bool
			 */
			bool setSheenComponent (const Base::PixelFactory::Color< float > & color = DefaultSheenColor, float roughness = DefaultSheenRoughness) noexcept;

			/**
			 * @brief Sets the sheen component as a texture for fabric-like materials.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer for the sheen color map.
			 * @param roughness The sheen roughness value. Default 0.5.
			 * @return bool
			 */
			bool setSheenComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float roughness = DefaultSheenRoughness) noexcept;

			/* ==================== Anisotropy Component Setters (Pre-creation) ==================== */

			/**
			 * @brief Sets the anisotropy component as scalar values for brushed metal effects.
			 * @warning This function is available before creation time.
			 * @param anisotropy The anisotropy strength (-1..1, 0 = isotropic). Default 0.0.
			 * @param rotation The anisotropy direction rotation (0..1). Default 0.0.
			 * @return bool
			 */
			bool setAnisotropyComponent (float anisotropy = DefaultAnisotropy, float rotation = DefaultAnisotropyRotation) noexcept;

			/**
			 * @brief Sets the anisotropy component as a texture (direction flowmap).
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer for the anisotropy direction map.
			 * @param anisotropy The anisotropy strength. Default 0.5.
			 * @param rotation The anisotropy direction rotation. Default 0.0.
			 * @return bool
			 */
			bool setAnisotropyComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float anisotropy = 0.5F, float rotation = DefaultAnisotropyRotation) noexcept;

			/* ==================== Subsurface Scattering Dynamic Property Setters (Post-creation) ==================== */

			/**
			 * @brief Changes the subsurface scattering intensity.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0.
			 * @return void
			 */
			void setSubsurfaceIntensity (float value) noexcept;

			/**
			 * @brief Changes the subsurface scattering radius.
			 * @note This is a dynamic property.
			 * @param value The scatter radius.
			 * @return void
			 */
			void setSubsurfaceRadius (float value) noexcept;

			/**
			 * @brief Changes the subsurface scattering color.
			 * @note This is a dynamic property.
			 * @param color A reference to the SSS color tint.
			 * @return void
			 */
			void setSubsurfaceColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/* ==================== Sheen Dynamic Property Setters (Post-creation) ==================== */

			/**
			 * @brief Changes the sheen color.
			 * @note This is a dynamic property.
			 * @param color A reference to the sheen color tint.
			 * @return void
			 */
			void setSheenColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Changes the sheen roughness.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0.
			 * @return void
			 */
			void setSheenRoughness (float value) noexcept;

			/* ==================== Anisotropy Dynamic Property Setters (Post-creation) ==================== */

			/**
			 * @brief Changes the anisotropy strength.
			 * @note This is a dynamic property.
			 * @param value A value between -1.0 and 1.0.
			 * @return void
			 */
			void setAnisotropy (float value) noexcept;

			/**
			 * @brief Changes the anisotropy rotation.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0.
			 * @return void
			 */
			void setAnisotropyRotation (float value) noexcept;

			/* ==================== Transmission Component Setters (Pre-creation) ==================== */

			/**
			 * @brief Sets the transmission component as a scalar value.
			 * @warning This function is available before creation time.
			 * @param factor The transmission factor (0.0 = opaque, 1.0 = fully transmissive). Default 0.0.
			 * @param attenuationColor The Beer's law attenuation color. Default white (no absorption).
			 * @param attenuationDistance The distance for full attenuation. Default 1.0.
			 * @param thickness The material thickness. Default 1.0.
			 * @return bool
			 */
			bool setTransmissionComponent (float factor = DefaultTransmissionFactor, const Base::PixelFactory::Color< float > & attenuationColor = DefaultAttenuationColor, float attenuationDistance = DefaultAttenuationDistance, float thickness = DefaultThicknessFactor) noexcept;

			/**
			 * @brief Sets the transmission component as a texture (per-pixel transmission factor).
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer for the transmission factor map.
			 * @param attenuationColor The Beer's law attenuation color. Default white (no absorption).
			 * @param attenuationDistance The distance for full attenuation. Default 1.0.
			 * @param thickness The material thickness. Default 1.0.
			 * @return bool
			 */
			bool setTransmissionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, const Base::PixelFactory::Color< float > & attenuationColor = DefaultAttenuationColor, float attenuationDistance = DefaultAttenuationDistance, float thickness = DefaultThicknessFactor) noexcept;

			/**
			 * @brief Sets the transmission component using the GrabPass for screen-space refraction.
			 * @warning This function is available before creation time.
			 * @note When enabled, the material samples the GrabPass texture (bindless slot 4) with UV distortion
			 * based on IOR and surface normal, producing screen-space refraction instead of cubemap-based transmission.
			 * @param factor The transmission factor (0.0 = opaque, 1.0 = fully transmissive). Default 0.0.
			 * @param attenuationColor The Beer's law attenuation color. Default white (no absorption).
			 * @param attenuationDistance The distance for full attenuation. Default 1.0.
			 * @param thickness The material thickness. Default 1.0.
			 * @return bool
			 */
			bool setTransmissionComponentFromGrabPass (float factor = DefaultTransmissionFactor, const Base::PixelFactory::Color< float > & attenuationColor = DefaultAttenuationColor, float attenuationDistance = DefaultAttenuationDistance, float thickness = DefaultThicknessFactor) noexcept;

			/**
			 * @brief Enables or disables depth-based opacity for GrabPass transmission.
			 * @warning This function is available before creation time.
			 * @note When enabled, the fragment shader samples the grab pass depth buffer to compute the water
			 * column depth per-pixel and uses it as the thickness in Beer's law attenuation.
			 * Requires grab pass transmission to be active.
			 * @param state True to enable depth-based opacity, false to disable.
			 * @return void
			 */
			void enableDepthBasedOpacity (bool state) noexcept;

			/* ==================== Iridescence Component Setters (Pre-creation) ==================== */

			/**
			 * @brief Sets the iridescence component (thin-film interference) as scalar values.
			 * @warning This function is available before creation time.
			 * @param factor The iridescence factor (0.0 = none, 1.0 = full). Default 0.0.
			 * @param ior The thin film IOR (1.0-2.333). Default 1.3.
			 * @param thicknessMin The minimum thin film thickness in nm. Default 100.
			 * @param thicknessMax The maximum thin film thickness in nm. Default 400.
			 * @return bool
			 */
			bool setIridescenceComponent (float factor = DefaultIridescenceFactor, float ior = DefaultIridescenceIOR, float thicknessMin = DefaultIridescenceThicknessMin, float thicknessMax = DefaultIridescenceThicknessMax) noexcept;

			/**
			 * @brief Sets the iridescence component with a texture (per-pixel iridescence factor).
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer for the iridescence factor map.
			 * @param ior The thin film IOR (1.0-2.333). Default 1.3.
			 * @param thicknessMin The minimum thin film thickness in nm. Default 100.
			 * @param thicknessMax The maximum thin film thickness in nm. Default 400.
			 * @return bool
			 */
			bool setIridescenceComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float ior = DefaultIridescenceIOR, float thicknessMin = DefaultIridescenceThicknessMin, float thicknessMax = DefaultIridescenceThicknessMax) noexcept;

			/* ==================== Dispersion Component Setters (Pre-creation) ==================== */

		/**
		 * @brief Sets the chromatic dispersion component (KHR_materials_dispersion).
		 * @warning This function is available before creation time.
		 * @param dispersion The dispersion value (0.0 = off). Typical: diamond 0.362, emerald 0.53.
		 * @return bool
		 */
		bool setDispersionComponent (float dispersion) noexcept;

		/* ==================== Iridescence Dynamic Property Setters (Post-creation) ==================== */

			/**
			 * @brief Changes the iridescence factor.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0.
			 * @return void
			 */
			void setIridescenceFactor (float value) noexcept;

			/**
			 * @brief Changes the iridescence thin film IOR.
			 * @note This is a dynamic property.
			 * @param value A value between 1.0 and 2.333.
			 * @return void
			 */
			void setIridescenceIOR (float value) noexcept;

			/**
			 * @brief Changes the iridescence minimum film thickness.
			 * @note This is a dynamic property.
			 * @param value The minimum thickness in nanometers.
			 * @return void
			 */
			void setIridescenceThicknessMin (float value) noexcept;

			/**
			 * @brief Changes the iridescence maximum film thickness.
			 * @note This is a dynamic property.
			 * @param value The maximum thickness in nanometers.
			 * @return void
			 */
			void setIridescenceThicknessMax (float value) noexcept;

			/* ==================== Dispersion Dynamic Property Setters (Post-creation) ==================== */

		/**
		 * @brief Changes the chromatic dispersion value.
		 * @note This is a dynamic property.
		 * @param value The dispersion value (0.0 = off).
		 * @return void
		 */
		void setDispersion (float value) noexcept;

		/* ==================== Specular Component Setters (KHR_materials_specular) ==================== */

		/**
		 * @brief Sets the specular component (KHR_materials_specular).
		 * @warning This function is available before creation time.
		 * @param factor The specular factor that scales dielectric F0 (0.0=no specular, 1.0=default). Default 1.0.
		 * @param color The specular color that tints dielectric F0. Default white (no tint).
		 * @return bool
		 */
		bool setSpecularComponent (float factor, const Base::PixelFactory::Color< float > & color = DefaultSpecularColor) noexcept;

		/* ==================== Specular Dynamic Property Setters (Post-creation) ==================== */

		/**
		 * @brief Changes the specular factor (KHR_materials_specular).
		 * @note This is a dynamic property.
		 * @param value The specular factor (0.0 = no specular highlight, 1.0 = default).
		 * @return void
		 */
		void setSpecularFactor (float value) noexcept;

		/**
		 * @brief Changes the specular color (KHR_materials_specular).
		 * @note This is a dynamic property.
		 * @param color A reference to the specular color tint.
		 * @return void
		 */
		void setSpecularColor (const Base::PixelFactory::Color< float > & color) noexcept;

		/* ==================== Emissive Strength Component (KHR_materials_emissive_strength) ==================== */

		/**
		 * @brief Sets the emissive strength HDR multiplier (KHR_materials_emissive_strength).
		 * @warning This function is available before creation time.
		 * @param strength The emissive strength multiplier (>= 0.0, default 1.0). Values > 1.0 enable HDR bloom.
		 * @return bool
		 */
		bool setEmissiveStrength (float strength) noexcept;

		/**
		 * @brief Changes the emissive strength HDR multiplier (KHR_materials_emissive_strength).
		 * @note This is a dynamic property.
		 * @param value The emissive strength multiplier (>= 0.0).
		 * @return void
		 */
		void setEmissiveStrengthValue (float value) noexcept;

		/* ==================== Transmission Dynamic Property Setters (Post-creation) ==================== */

			/**
			 * @brief Changes the transmission factor.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0.
			 * @return void
			 */
			void setTransmissionFactor (float value) noexcept;

			/**
			 * @brief Changes the attenuation color for Beer's law.
			 * @note This is a dynamic property.
			 * @param color A reference to the attenuation color.
			 * @return void
			 */
			void setAttenuationColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Changes the attenuation distance for Beer's law.
			 * @note This is a dynamic property.
			 * @param value The distance in meters.
			 * @return void
			 */
			void setAttenuationDistance (float value) noexcept;

			/**
			 * @brief Changes the material thickness factor.
			 * @note This is a dynamic property.
			 * @param value The thickness value.
			 * @return void
			 */
			void setThicknessFactor (float value) noexcept;

		private:

			/** @copydoc EmEn::Graphics::Material::Interface::create() noexcept */
			[[nodiscard]]
			bool create (Renderer & renderer) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::destroy() noexcept */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::getSharedUniformBufferIdentifier() */
			[[nodiscard]]
			std::string getSharedUniformBufferIdentifier () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::createElementInSharedBuffer() */
			[[nodiscard]]
			bool createElementInSharedBuffer (Renderer & renderer, const std::string & identifier) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::createDescriptorSetLayout() */
			[[nodiscard]]
			bool createDescriptorSetLayout (Vulkan::LayoutManager & layoutManager, const std::string & identifier) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::createDescriptorSet() */
			[[nodiscard]]
			bool createDescriptorSet (Renderer & renderer, const Vulkan::UniformBufferObject & uniformBufferObject) noexcept override;

			/**
			 * @brief Parses the albedo component from JSON data.
			 * @param data A reference to the JSON data.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseAlbedoComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the roughness component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseRoughnessComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the metalness component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseMetalnessComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the normal component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseNormalComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the height component from JSON data for parallax occlusion mapping.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseHeightComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the reflection component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseReflectionComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the refraction component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseRefractionComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the auto-illumination component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseAutoIlluminationComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the opacity component from JSON data (owner's 3-rule contract).
			 * @note Type Value = global transparency, blending (rule 1). Texture WITH AlphaThreshold
			 * key = binary cutout, alpha test, stays opaque (rule 2). Texture WITHOUT AlphaThreshold
			 * = grayscale per-pixel alpha scale, blending (rule 3).
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseOpacityComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the ambient occlusion component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseAmbientOcclusionComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the reflectivity map component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseReflectivityMapComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the clear coat component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseClearCoatComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the subsurface scattering component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseSubsurfaceComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the sheen component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseSheenComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the anisotropy component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseAnisotropyComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the transmission component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseTransmissionComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the iridescence component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseIridescenceComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the specular component (KHR_materials_specular) from JSON data.
			 * @param data A reference to the JSON data.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseSpecularComponent (const Json::Value & data) noexcept;

			/**
			 * @brief Updates the UBO with material properties.
			 * @return bool
			 */
			bool updateVideoMemory () noexcept;

			/**
			 * @brief Generates the fragment shader code for a specific texture component.
			 * @param componentType The component type to find in the material.
			 * @param codeGenerator A reference to a function to generate the actual code.
			 * @param fragmentShader A reference to the fragment shader being generated.
			 * @param materialSet The current material set.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateTextureComponentFragmentShader (ComponentType componentType, const std::function< bool (Saphir::FragmentShader &, const Component::Texture *) > & codeGenerator, Saphir::FragmentShader & fragmentShader, uint32_t materialSet) const noexcept;

			/**
			 * @brief Returns the right texture coordinates for a component.
			 * @param component A pointer to the texture component.
			 * @return const char *
			 */
			[[nodiscard]]
			const char * textCoords (const Component::Texture * component) const noexcept;

			/**
			 * @brief Returns the GLSL texture coordinates expression with the component's UV
			 * transform applied (uv * scale + offset, from the material UBO — identity neutral).
			 * @param componentType The component type (selects the UBO transform slot).
			 * @param component A pointer to the texture component.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string transformedTexCoords (ComponentType componentType, const Component::Texture * component) const noexcept;

			/**
			 * @brief Returns the texture component feeding the alpha test, if any.
			 * @note The alpha source is the opacity texture component when present, the albedo
			 * texture (alpha channel) otherwise. Used by the cutout codegen and the alpha-tested
			 * shadow path so both read the same source by construction.
			 * @return const Component::Texture *
			 */
			[[nodiscard]]
			const Component::Texture * alphaSourceTextureComponent () const noexcept;

			/**
			 * @brief Returns the GLSL name/expression carrying the FINAL albedo.
			 * @note With vertex colours enabled this is the folded variable (SurfaceAlbedoFinal),
			 * declared once at the top of the fragment shader; otherwise it is the albedo texture
			 * component's variable or the material UBO colour. Every consumer (light generator,
			 * fragment colour, alpha test) must go through this so they all read the same value.
			 * @warning Decidable from the FLAG alone: setupLightGenerator() runs before the
			 * fragment stage is generated.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string albedoExpression () const noexcept;

			/**
			 * @brief Copies each texture component's UV transform into its material UBO slot.
			 * @note Called at creation time, before the first video memory update — the
			 * components are the single source of truth (loader and JSON paths both land there).
			 * @return void
			 */
			void syncComponentUVWTransforms () noexcept;

			/**
			 * @brief Generates fragment shader code for bindless reflection using the scene's environment cubemap.
			 * @note Used when automatic reflection is enabled AND bindless textures are supported.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader being generated.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateBindlessReflectionFragmentShader (const Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept;

			/**
			 * @brief Generates fragment shader code for bindless refraction using the scene's environment cubemap.
			 * @note Used when automatic refraction is enabled AND bindless textures are supported.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader being generated.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateBindlessRefractionFragmentShader (const Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept;

			/**
			 * @brief Generates fragment shader code for bindless transmission using the scene's prefiltered cubemap.
			 * @note Used when automatic transmission is enabled AND bindless textures are supported.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader being generated.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateBindlessTransmissionFragmentShader (const Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept;

			/**
			 * @brief Generates fragment shader code for screen-space transmission using the GrabPass texture.
			 * @note Samples the GrabPass (bindless 2D slot 4) with UV distortion based on IOR and surface normal.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader being generated.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateGrabPassTransmissionFragmentShader (const Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept;

			/* Uniform buffer object layout (STD140 aligned, 80 floats = 320 bytes):
			 * vec4 albedoColor			  (offset 0-3)
			 * float roughness			   (offset 4)
			 * float metalness			   (offset 5)
			 * float normalScale			 (offset 6)
			 * float specularFactor		  (offset 7)  - KHR_materials_specular factor (scales dielectric F0)
			 * float ior					 (offset 8)  - Index of refraction for glass/transparent
			 * float iblIntensity			(offset 9)  - IBL contribution intensity (0.0-1.0)
			 * float autoIlluminationAmount  (offset 10) - Emissive intensity multiplier
			 * float aoIntensity			 (offset 11) - Ambient occlusion intensity
			 * vec4 autoIlluminationColor	(offset 12-15) - Emissive color
			 * float clearCoatFactor		 (offset 16) - Clear coat intensity (0.0-1.0)
			 * float clearCoatRoughness	  (offset 17) - Clear coat roughness (0.0-1.0)
			 * float subsurfaceIntensity	 (offset 18) - SSS master weight + wrap amount (0.0-1.0)
			 * float subsurfaceRadius		(offset 19) - SSS scatter distance for thickness falloff
			 * vec4 subsurfaceColor		  (offset 20-23) - SSS scattered light color tint
			 * vec4 sheenColor			   (offset 24-27) - Sheen color tint (black = disabled)
			 * float sheenRoughness		  (offset 28) - Sheen roughness (0.0 = satin, 1.0 = wool)
			 * float anisotropy			 (offset 29) - Anisotropy strength (-1..1, 0 = isotropic)
			 * float anisotropyRotation	 (offset 30) - Anisotropy direction rotation (0..1)
			 * float transmissionFactor	 (offset 31) - Transmission weight (0.0 = opaque, 1.0 = transmissive)
			 * vec4 attenuationColor		(offset 32-35) - Beer's law absorption color
			 * float attenuationDistance	(offset 36) - Distance for full attenuation
			 * float thicknessFactor		(offset 37) - Constant material thickness for Beer's law
			 * float heightScale			(offset 38) - Parallax occlusion mapping depth
			 * float iridescenceFactor	  (offset 39) - Iridescence intensity (0.0-1.0)
			 * float iridescenceIOR		 (offset 40) - Thin film IOR (1.0-2.333)
			 * float iridescenceThicknessMin (offset 41) - Min thin film thickness (nm)
			 * float iridescenceThicknessMax (offset 42) - Max thin film thickness (nm)
			 * float dispersion			 (offset 43) - Chromatic dispersion (KHR_materials_dispersion)
			 * vec4 specularColorFactor	 (offset 44-47) - KHR_materials_specular color (tints dielectric F0)
			 * float emissiveStrength	   (offset 48) - KHR_materials_emissive_strength HDR multiplier
			 * float clearCoatNormalScale   (offset 49) - Clear coat normal map scale
			 * float opacity				 (offset 50) - Global/texture opacity amount (1.0 = opaque)
			 * float alphaThreshold		   (offset 51) - Alpha-test cutoff (cutout mode, glTF alphaCutoff)
			 * float reflectionAmount		 (offset 52) - Artistic reflection mix (texture/probe modes; 1.0 = BRDF-controlled)
			 * float refractionAmount		 (offset 53) - Artistic refraction mix (texture mode; 1.0 = Fresnel-controlled)
			 * float padding[2]			 (offset 54-55) - STD140 padding
			 * vec4 albedoUVWTransform	  (offset 56-59) - UV transform (scale.xy, offset.zw), KHR_texture_transform
			 * vec4 roughnessUVWTransform   (offset 60-63) - UV transform (scale.xy, offset.zw)
			 * vec4 metalnessUVWTransform   (offset 64-67) - UV transform (scale.xy, offset.zw)
			 * vec4 normalUVWTransform	  (offset 68-71) - UV transform (scale.xy, offset.zw)
			 * vec4 aoUVWTransform		  (offset 72-75) - UV transform (scale.xy, offset.zw)
			 * vec4 emissiveUVWTransform	(offset 76-79) - UV transform (scale.xy, offset.zw)
			 */
			static constexpr auto AlbedoColorOffset{0UL};
			static constexpr auto RoughnessOffset{4UL};
			static constexpr auto MetalnessOffset{5UL};
			static constexpr auto NormalScaleOffset{6UL};
			static constexpr auto SpecularFactorOffset{7UL};
			static constexpr auto IOROffset{8UL};
			static constexpr auto IBLIntensityOffset{9UL};
			static constexpr auto AutoIlluminationAmountOffset{10UL};
			static constexpr auto AOIntensityOffset{11UL};
			static constexpr auto AutoIlluminationColorOffset{12UL};
			static constexpr auto ClearCoatFactorOffset{16UL};
			static constexpr auto ClearCoatRoughnessOffset{17UL};
			static constexpr auto SubsurfaceIntensityOffset{18UL};
			static constexpr auto SubsurfaceRadiusOffset{19UL};
			static constexpr auto SubsurfaceColorOffset{20UL};
			static constexpr auto SheenColorOffset{24UL};
			static constexpr auto SheenRoughnessOffset{28UL};
			static constexpr auto AnisotropyOffset{29UL};
			static constexpr auto AnisotropyRotationOffset{30UL};
			static constexpr auto TransmissionFactorOffset{31UL};
			static constexpr auto AttenuationColorOffset{32UL};
			static constexpr auto AttenuationDistanceOffset{36UL};
			static constexpr auto ThicknessFactorOffset{37UL};
			static constexpr auto HeightScaleOffset{38UL};
			static constexpr auto IridescenceFactorOffset{39UL};
			static constexpr auto IridescenceIOROffset{40UL};
			static constexpr auto IridescenceThicknessMinOffset{41UL};
			static constexpr auto IridescenceThicknessMaxOffset{42UL};
			static constexpr auto DispersionOffset{43UL};
			static constexpr auto SpecularColorOffset{44UL};
			static constexpr auto EmissiveStrengthOffset{48UL};
			static constexpr auto ClearCoatNormalScaleOffset{49UL};
			static constexpr auto OpacityOffset{50UL};
			static constexpr auto AlphaThresholdOffset{51UL};
			static constexpr auto ReflectionAmountOffset{52UL};
			static constexpr auto RefractionAmountOffset{53UL};
			/* Per-component UV transforms (KHR_texture_transform): vec4 = (scale.xy, offset.zw).
			 * Neutral (1,1,0,0) — applied UNCONDITIONALLY at the sampling sites, so the neutral
			 * value MUST be the identity (same precedent as DefaultAlbedoColor/DefaultTextureFactor). */
			static constexpr auto AlbedoUVWTransformOffset{56UL};
			static constexpr auto RoughnessUVWTransformOffset{60UL};
			static constexpr auto MetalnessUVWTransformOffset{64UL};
			static constexpr auto NormalUVWTransformOffset{68UL};
			static constexpr auto AmbientOcclusionUVWTransformOffset{72UL};
			static constexpr auto AutoIlluminationUVWTransformOffset{76UL};

			/* Default values. */
			/* White, NOT grey: the albedo colour is also the TINT factor multiplying the albedo
			 * texture in the generated shader, so its neutral value has to be the multiplicative
			 * identity. A grey default would darken every textured material by half. */
			static constexpr auto DefaultAlbedoColor{Base::PixelFactory::White};
			static constexpr auto DefaultRoughness{0.5F};
			static constexpr auto DefaultMetalness{0.0F};
			/* 1.0, NOT DefaultRoughness/DefaultMetalness: when a texture drives the component,
			 * the scalar becomes the FACTOR multiplying the sampled texel (glTF contract:
			 * 'roughnessFactor * texel.g', 'metallicFactor * texel.b'), so its neutral value
			 * has to be the multiplicative identity — same precedent as DefaultAlbedoColor. */
			static constexpr auto DefaultTextureFactor{1.0F};
			static constexpr auto DefaultNormalScale{1.0F};
			static constexpr auto DefaultSpecularFactor{1.0F}; /* KHR_materials_specular: scales dielectric F0 (1.0 = unchanged). */
			static constexpr auto DefaultIOR{1.5F}; /* Standard IOR for glass. */
			static constexpr auto DefaultIBLIntensity{1.0F}; /* Full IBL contribution by default. */
			static constexpr auto DefaultAutoIlluminationColor{Base::PixelFactory::Black};
			static constexpr auto DefaultAutoIlluminationAmount{0.0F}; /* Disabled by default. */
			static constexpr auto DefaultAOIntensity{1.0F}; /* Full AO contribution by default. */
			static constexpr auto DefaultClearCoatFactor{0.0F}; /* No clear coat by default. */
			static constexpr auto DefaultClearCoatRoughness{0.0F}; /* Mirror-smooth coat by default. */
			static constexpr auto DefaultSubsurfaceIntensity{0.0F}; /* No SSS by default. */
			static constexpr auto DefaultSubsurfaceRadius{1.0F}; /* Default scatter distance. */
			static constexpr Base::PixelFactory::Color< float > DefaultSubsurfaceColor{1.0F, 0.2F, 0.1F, 1.0F}; /* Reddish (skin-like). */
			static constexpr Base::PixelFactory::Color< float > DefaultSheenColor{0.0F, 0.0F, 0.0F, 1.0F}; /* Black = disabled. */
			static constexpr auto DefaultSheenRoughness{0.5F}; /* Mid-roughness (fabric-like). */
			static constexpr auto DefaultAnisotropy{0.0F}; /* No anisotropy by default (isotropic). */
			static constexpr auto DefaultAnisotropyRotation{0.0F}; /* No rotation (tangent direction). */
			static constexpr auto DefaultTransmissionFactor{0.0F}; /* No transmission by default (opaque). */
			static constexpr Base::PixelFactory::Color< float > DefaultAttenuationColor{1.0F, 1.0F, 1.0F, 1.0F}; /* White = no absorption. */
			static constexpr auto DefaultAttenuationDistance{1.0F}; /* 1 meter for full attenuation. */
			static constexpr auto DefaultThicknessFactor{1.0F}; /* Default material thickness. */
			static constexpr auto DefaultHeightScale{0.02F}; /* Parallax occlusion mapping depth. */
			static constexpr auto DefaultIridescenceFactor{0.0F}; /* No iridescence by default. */
			static constexpr auto DefaultIridescenceIOR{1.3F}; /* Thin film IOR (soap bubble ~1.3). */
			static constexpr auto DefaultIridescenceThicknessMin{100.0F}; /* Min thin film thickness in nm. */
			static constexpr auto DefaultIridescenceThicknessMax{400.0F}; /* Max thin film thickness in nm. */
			static constexpr auto DefaultDispersion{0.0F}; /* No chromatic dispersion (0.0 = off). */
			static constexpr Base::PixelFactory::Color< float > DefaultSpecularColor{1.0F, 1.0F, 1.0F, 1.0F}; /* White = no tint (pass-through). */
			static constexpr auto DefaultEmissiveStrength{1.0F}; /* KHR_materials_emissive_strength: HDR multiplier (1.0 = pass-through). */
			static constexpr auto DefaultClearCoatNormalScale{1.0F}; /* Clear coat normal map scale (1.0 = full effect). */
			static constexpr auto DefaultOpacity{1.0F}; /* Fully opaque; also the multiplicative identity when a texture drives the component. */
			static constexpr auto DefaultAlphaThreshold{0.5F}; /* glTF alphaCutoff default (cutout mode). */
			static constexpr auto DefaultReflectionAmount{1.0F}; /* Neutral: the BRDF controls the mix (artistic override, D2). */
			static constexpr auto DefaultRefractionAmount{1.0F}; /* Neutral: Fresnel controls the mix (artistic override, D2). */

			Physics::SurfacePhysicalProperties m_physicalSurfaceProperties;
			std::unordered_map< ComponentType, std::unique_ptr< Component::Interface > > m_components;
			BlendingMode m_blendingMode{BlendingMode::None};
			std::array< float, 80 > m_materialProperties{
				/* Albedo color (4) */
				DefaultAlbedoColor.red(), DefaultAlbedoColor.green(), DefaultAlbedoColor.blue(), DefaultAlbedoColor.alpha(),
				/* Roughness (1), Metalness (1), NormalScale (1), SpecularFactor (1) */
				DefaultRoughness, DefaultMetalness, DefaultNormalScale, DefaultSpecularFactor,
				/* IOR (1), IBLIntensity (1), AutoIlluminationAmount (1), AOIntensity (1) */
				DefaultIOR, DefaultIBLIntensity, DefaultAutoIlluminationAmount, DefaultAOIntensity,
				/* AutoIlluminationColor (4) */
				DefaultAutoIlluminationColor.red(), DefaultAutoIlluminationColor.green(), DefaultAutoIlluminationColor.blue(), DefaultAutoIlluminationColor.alpha(),
				/* ClearCoatFactor (1), ClearCoatRoughness (1), SubsurfaceIntensity (1), SubsurfaceRadius (1) */
				DefaultClearCoatFactor, DefaultClearCoatRoughness, DefaultSubsurfaceIntensity, DefaultSubsurfaceRadius,
				/* SubsurfaceColor (4) */
				DefaultSubsurfaceColor.red(), DefaultSubsurfaceColor.green(), DefaultSubsurfaceColor.blue(), DefaultSubsurfaceColor.alpha(),
				/* SheenColor (4) */
				DefaultSheenColor.red(), DefaultSheenColor.green(), DefaultSheenColor.blue(), DefaultSheenColor.alpha(),
				/* SheenRoughness (1), Anisotropy (1), AnisotropyRotation (1), TransmissionFactor (1) */
				DefaultSheenRoughness, DefaultAnisotropy, DefaultAnisotropyRotation, DefaultTransmissionFactor,
				/* AttenuationColor (4) */
				DefaultAttenuationColor.red(), DefaultAttenuationColor.green(), DefaultAttenuationColor.blue(), DefaultAttenuationColor.alpha(),
				/* AttenuationDistance (1), ThicknessFactor (1), HeightScale (1), IridescenceFactor (1) */
				DefaultAttenuationDistance, DefaultThicknessFactor, DefaultHeightScale, DefaultIridescenceFactor,
				/* IridescenceIOR (1), IridescenceThicknessMin (1), IridescenceThicknessMax (1), Dispersion (1) */
				DefaultIridescenceIOR, DefaultIridescenceThicknessMin, DefaultIridescenceThicknessMax, DefaultDispersion,
				/* SpecularColorFactor (4) */
				DefaultSpecularColor.red(), DefaultSpecularColor.green(), DefaultSpecularColor.blue(), DefaultSpecularColor.alpha(),
				/* EmissiveStrength (1), ClearCoatNormalScale (1), Opacity (1), AlphaThreshold (1) */
				DefaultEmissiveStrength, DefaultClearCoatNormalScale, DefaultOpacity, DefaultAlphaThreshold,
				/* ReflectionAmount (1), RefractionAmount (1) + padding (2) for STD140 alignment */
				DefaultReflectionAmount, DefaultRefractionAmount, 0.0F, 0.0F,
				/* Per-component UV transforms (6 x vec4 = scale.xy, offset.zw), identity neutral:
				 * Albedo, Roughness, Metalness, Normal, AmbientOcclusion, AutoIllumination. */
				1.0F, 1.0F, 0.0F, 0.0F,
				1.0F, 1.0F, 0.0F, 0.0F,
				1.0F, 1.0F, 0.0F, 0.0F,
				1.0F, 1.0F, 0.0F, 0.0F,
				1.0F, 1.0F, 0.0F, 0.0F,
				1.0F, 1.0F, 0.0F, 0.0F
			};
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;
			std::shared_ptr< SharedUniformBuffer > m_sharedUniformBuffer;
			uint32_t m_sharedUBOIndex{0};
			bool m_videoMemoryUpdated{false};
			bool m_invertRoughness{false};
			float m_postProcessReflectivityAmount{-1.0F};
			bool m_isUsingEnvironmentCubemap{false};
			/** @brief Explicitly authored cubemap reflection (texture mode): never replaced by SSR/RTR. */
			bool m_reflectionIsArtistic{false};
			/** @brief Reflection source is a render target (probe/mirror): absolute luminance, no environment luminance scale. */
			bool m_reflectionSourceIsAbsolute{false};
			/** @brief Refraction source is a render target: absolute luminance, no environment luminance scale. */
			bool m_refractionSourceIsAbsolute{false};
			bool m_isUsingEnvironmentCubemapForRefraction{false};
			bool m_isUsingEnvironmentCubemapForTransmission{false};
			bool m_isUsingGrabPassForTransmission{false};
			bool m_isUsingDepthBasedOpacity{false};
			bool m_useParallaxOcclusionMapping{false};
			mutable bool m_pomGenerationActive{false};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using StandardMaterials = Container< Graphics::Material::StandardResource >;
}
