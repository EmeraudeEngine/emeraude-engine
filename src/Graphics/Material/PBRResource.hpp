/*
 * src/Graphics/Material/PBRResource.hpp
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
#include <array>
#include <unordered_map>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Resources/Container.hpp"
#include "Physics/SurfacePhysicalProperties.hpp"
#include "Component/Interface.hpp"
#include "Graphics/TextureResource/TextureCubemap.hpp"

/* Forward declarations. */
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
	class PBRResource final : public Interface
	{
		friend class Resources::Container< PBRResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MaterialPBRResource"};

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

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Few};

			/**
			 * @brief Constructs a PBR material.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			PBRResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
				: Interface{serviceProvider, name, resourceFlags}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PBRResource (const PBRResource & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PBRResource (PBRResource && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return PBRResource &
			 */
			PBRResource & operator= (const PBRResource & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return PBRResource &
			 */
			PBRResource & operator= (PBRResource && copy) noexcept = delete;

			/**
			 * @brief Destructs the material.
			 */
			~PBRResource () override
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
				return Libs::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
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
			bool setAlbedoComponent (const Libs::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Sets the albedo (base color) component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @return bool
			 */
			bool setAlbedoComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept;

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
			 * @param value The default roughness value. Default 0.5.
			 * @param invert If true, the texture is treated as a smoothness/gloss map and inverted (1.0 - value). Default false.
			 * @return bool
			 */
			bool setRoughnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float value = DefaultRoughness, bool invert = false) noexcept;

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
			 * @param value The default metalness value. Default 0.0.
			 * @return bool
			 */
			bool setMetalnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float value = DefaultMetalness) noexcept;

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

			/**
			 * @brief Sets the auto-illumination (emissive) component as a color.
			 * @warning This function is available before creation time.
			 * @param color A reference to the emissive color.
			 * @param amount The intensity multiplier. Default 1.0.
			 * @return bool
			 */
			bool setAutoIlluminationComponent (const Libs::PixelFactory::Color< float > & color, float amount = DefaultAutoIlluminationAmount) noexcept;

			/**
			 * @brief Sets the auto-illumination (emissive) component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @param amount The intensity multiplier. Default 1.0.
			 * @return bool
			 */
			bool setAutoIlluminationComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount = DefaultAutoIlluminationAmount) noexcept;

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
			void setAlbedoColor (const Libs::PixelFactory::Color< float > & color) noexcept;

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
			void setAutoIlluminationColor (const Libs::PixelFactory::Color< float > & color) noexcept;

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
			bool setSubsurfaceComponent (float intensity = DefaultSubsurfaceIntensity, float radius = DefaultSubsurfaceRadius, const Libs::PixelFactory::Color< float > & color = DefaultSubsurfaceColor) noexcept;

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
			bool setSheenComponent (const Libs::PixelFactory::Color< float > & color = DefaultSheenColor, float roughness = DefaultSheenRoughness) noexcept;

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
			void setSubsurfaceColor (const Libs::PixelFactory::Color< float > & color) noexcept;

			/* ==================== Sheen Dynamic Property Setters (Post-creation) ==================== */

			/**
			 * @brief Changes the sheen color.
			 * @note This is a dynamic property.
			 * @param color A reference to the sheen color tint.
			 * @return void
			 */
			void setSheenColor (const Libs::PixelFactory::Color< float > & color) noexcept;

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
			bool setTransmissionComponent (float factor = DefaultTransmissionFactor, const Libs::PixelFactory::Color< float > & attenuationColor = DefaultAttenuationColor, float attenuationDistance = DefaultAttenuationDistance, float thickness = DefaultThicknessFactor) noexcept;

			/**
			 * @brief Sets the transmission component as a texture (per-pixel transmission factor).
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer for the transmission factor map.
			 * @param attenuationColor The Beer's law attenuation color. Default white (no absorption).
			 * @param attenuationDistance The distance for full attenuation. Default 1.0.
			 * @param thickness The material thickness. Default 1.0.
			 * @return bool
			 */
			bool setTransmissionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, const Libs::PixelFactory::Color< float > & attenuationColor = DefaultAttenuationColor, float attenuationDistance = DefaultAttenuationDistance, float thickness = DefaultThicknessFactor) noexcept;

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
			bool setTransmissionComponentFromGrabPass (float factor = DefaultTransmissionFactor, const Libs::PixelFactory::Color< float > & attenuationColor = DefaultAttenuationColor, float attenuationDistance = DefaultAttenuationDistance, float thickness = DefaultThicknessFactor) noexcept;

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
		bool setSpecularComponent (float factor, const Libs::PixelFactory::Color< float > & color = DefaultSpecularColor) noexcept;

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
		void setSpecularColor (const Libs::PixelFactory::Color< float > & color) noexcept;

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
			void setAttenuationColor (const Libs::PixelFactory::Color< float > & color) noexcept;

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

			/* Uniform buffer object layout (STD140 aligned, 52 floats = 208 bytes):
			 * vec4 albedoColor              (offset 0-3)
			 * float roughness               (offset 4)
			 * float metalness               (offset 5)
			 * float normalScale             (offset 6)
			 * float specularFactor          (offset 7)  - KHR_materials_specular factor (scales dielectric F0)
			 * float ior                     (offset 8)  - Index of refraction for glass/transparent
			 * float iblIntensity            (offset 9)  - IBL contribution intensity (0.0-1.0)
			 * float autoIlluminationAmount  (offset 10) - Emissive intensity multiplier
			 * float aoIntensity             (offset 11) - Ambient occlusion intensity
			 * vec4 autoIlluminationColor    (offset 12-15) - Emissive color
			 * float clearCoatFactor         (offset 16) - Clear coat intensity (0.0-1.0)
			 * float clearCoatRoughness      (offset 17) - Clear coat roughness (0.0-1.0)
			 * float subsurfaceIntensity     (offset 18) - SSS master weight + wrap amount (0.0-1.0)
			 * float subsurfaceRadius        (offset 19) - SSS scatter distance for thickness falloff
			 * vec4 subsurfaceColor          (offset 20-23) - SSS scattered light color tint
			 * vec4 sheenColor               (offset 24-27) - Sheen color tint (black = disabled)
			 * float sheenRoughness          (offset 28) - Sheen roughness (0.0 = satin, 1.0 = wool)
			 * float anisotropy             (offset 29) - Anisotropy strength (-1..1, 0 = isotropic)
			 * float anisotropyRotation     (offset 30) - Anisotropy direction rotation (0..1)
			 * float transmissionFactor     (offset 31) - Transmission weight (0.0 = opaque, 1.0 = transmissive)
			 * vec4 attenuationColor        (offset 32-35) - Beer's law absorption color
			 * float attenuationDistance    (offset 36) - Distance for full attenuation
			 * float thicknessFactor        (offset 37) - Constant material thickness for Beer's law
			 * float heightScale            (offset 38) - Parallax occlusion mapping depth
			 * float iridescenceFactor      (offset 39) - Iridescence intensity (0.0-1.0)
			 * float iridescenceIOR         (offset 40) - Thin film IOR (1.0-2.333)
			 * float iridescenceThicknessMin (offset 41) - Min thin film thickness (nm)
			 * float iridescenceThicknessMax (offset 42) - Max thin film thickness (nm)
			 * float dispersion             (offset 43) - Chromatic dispersion (KHR_materials_dispersion)
			 * vec4 specularColorFactor     (offset 44-47) - KHR_materials_specular color (tints dielectric F0)
			 * float emissiveStrength       (offset 48) - KHR_materials_emissive_strength HDR multiplier
			 * float clearCoatNormalScale   (offset 49) - Clear coat normal map scale
			 * float padding[2]             (offset 50-51) - STD140 padding
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

			/* Default values. */
			static constexpr auto DefaultAlbedoColor{Libs::PixelFactory::Grey};
			static constexpr auto DefaultRoughness{0.5F};
			static constexpr auto DefaultMetalness{0.0F};
			static constexpr auto DefaultNormalScale{1.0F};
			static constexpr auto DefaultSpecularFactor{1.0F}; /* KHR_materials_specular: scales dielectric F0 (1.0 = unchanged). */
			static constexpr auto DefaultIOR{1.5F}; /* Standard IOR for glass. */
			static constexpr auto DefaultIBLIntensity{1.0F}; /* Full IBL contribution by default. */
			static constexpr auto DefaultAutoIlluminationColor{Libs::PixelFactory::Black};
			static constexpr auto DefaultAutoIlluminationAmount{0.0F}; /* Disabled by default. */
			static constexpr auto DefaultAOIntensity{1.0F}; /* Full AO contribution by default. */
			static constexpr auto DefaultClearCoatFactor{0.0F}; /* No clear coat by default. */
			static constexpr auto DefaultClearCoatRoughness{0.0F}; /* Mirror-smooth coat by default. */
			static constexpr auto DefaultSubsurfaceIntensity{0.0F}; /* No SSS by default. */
			static constexpr auto DefaultSubsurfaceRadius{1.0F}; /* Default scatter distance. */
			static constexpr Libs::PixelFactory::Color< float > DefaultSubsurfaceColor{1.0F, 0.2F, 0.1F, 1.0F}; /* Reddish (skin-like). */
			static constexpr Libs::PixelFactory::Color< float > DefaultSheenColor{0.0F, 0.0F, 0.0F, 1.0F}; /* Black = disabled. */
			static constexpr auto DefaultSheenRoughness{0.5F}; /* Mid-roughness (fabric-like). */
			static constexpr auto DefaultAnisotropy{0.0F}; /* No anisotropy by default (isotropic). */
			static constexpr auto DefaultAnisotropyRotation{0.0F}; /* No rotation (tangent direction). */
			static constexpr auto DefaultTransmissionFactor{0.0F}; /* No transmission by default (opaque). */
			static constexpr Libs::PixelFactory::Color< float > DefaultAttenuationColor{1.0F, 1.0F, 1.0F, 1.0F}; /* White = no absorption. */
			static constexpr auto DefaultAttenuationDistance{1.0F}; /* 1 meter for full attenuation. */
			static constexpr auto DefaultThicknessFactor{1.0F}; /* Default material thickness. */
			static constexpr auto DefaultHeightScale{0.02F}; /* Parallax occlusion mapping depth. */
			static constexpr auto DefaultIridescenceFactor{0.0F}; /* No iridescence by default. */
			static constexpr auto DefaultIridescenceIOR{1.3F}; /* Thin film IOR (soap bubble ~1.3). */
			static constexpr auto DefaultIridescenceThicknessMin{100.0F}; /* Min thin film thickness in nm. */
			static constexpr auto DefaultIridescenceThicknessMax{400.0F}; /* Max thin film thickness in nm. */
			static constexpr auto DefaultDispersion{0.0F}; /* No chromatic dispersion (0.0 = off). */
			static constexpr Libs::PixelFactory::Color< float > DefaultSpecularColor{1.0F, 1.0F, 1.0F, 1.0F}; /* White = no tint (pass-through). */
			static constexpr auto DefaultEmissiveStrength{1.0F}; /* KHR_materials_emissive_strength: HDR multiplier (1.0 = pass-through). */
			static constexpr auto DefaultClearCoatNormalScale{1.0F}; /* Clear coat normal map scale (1.0 = full effect). */

			Physics::SurfacePhysicalProperties m_physicalSurfaceProperties{};
			std::unordered_map< ComponentType, std::unique_ptr< Component::Interface > > m_components{};
			BlendingMode m_blendingMode{BlendingMode::None};
			std::array< float, 52 > m_materialProperties{
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
				/* EmissiveStrength (1), ClearCoatNormalScale (1) + padding (2) for STD140 alignment */
				DefaultEmissiveStrength, DefaultClearCoatNormalScale, 0.0F, 0.0F
			};
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;
			std::shared_ptr< SharedUniformBuffer > m_sharedUniformBuffer;
			uint32_t m_sharedUBOIndex{0};
			bool m_videoMemoryUpdated{false};
			bool m_invertRoughness{false};
			float m_postProcessReflectivityAmount{-1.0F};
			bool m_isUsingEnvironmentCubemap{false};
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
	using PBRMaterials = Container< Graphics::Material::PBRResource >;
}
