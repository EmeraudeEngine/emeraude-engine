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

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Few};

			/**
			 * @brief Constructs a PBR material.
			 * @param name A reference to a string for the resource name.
			 * @param materialFlags The resource flag bits. Default none.
			 */
			explicit
			PBRResource (const std::string & name, uint32_t materialFlags = 0) noexcept
				: Interface{name, materialFlags}
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

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::ServiceProvider &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(Resources::Manager &, const Json::Value &) */
			bool load (Resources::AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept override;

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

			/**
			 * @brief Enables automatic reflection from scene environment cubemap.
			 * @note When enabled, the material will use the scene's environment cubemap for reflection
			 * instead of a material-specific texture. This is resolved at render time.
			 * @param iblIntensity The IBL intensity. Default 1.0.
			 */
			void enableAutomaticReflection (float iblIntensity = DefaultIBLIntensity) noexcept;

			/** @copydoc EmEn::Graphics::Material::Interface::useAutomaticReflection() const noexcept */
			[[nodiscard]]
			bool
			useAutomaticReflection () const noexcept override
			{
				return m_useAutomaticReflection;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::updateAutomaticReflectionCubemap() noexcept */
			[[nodiscard]]
			bool updateAutomaticReflectionCubemap (const Vulkan::TextureInterface & cubemap) noexcept override;

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
			 * @param serviceProvider A reference to the resource manager through a service provider.
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
			static const char * textCoords (const Component::Texture * component) noexcept;

			/* Uniform buffer object layout (STD140 aligned, 16 floats = 64 bytes):
			 * vec4 albedoColor              (offset 0-3)
			 * float roughness               (offset 4)
			 * float metalness               (offset 5)
			 * float normalScale             (offset 6)
			 * float f0                      (offset 7)  - Base reflectivity for dielectrics
			 * float ior                     (offset 8)  - Index of refraction for glass/transparent
			 * float iblIntensity            (offset 9)  - IBL contribution intensity (0.0-1.0)
			 * float autoIlluminationAmount  (offset 10) - Emissive intensity multiplier
			 * float aoIntensity             (offset 11) - Ambient occlusion intensity
			 * vec4 autoIlluminationColor    (offset 12-15) - Emissive color
			 */
			static constexpr auto AlbedoColorOffset{0UL};
			static constexpr auto RoughnessOffset{4UL};
			static constexpr auto MetalnessOffset{5UL};
			static constexpr auto NormalScaleOffset{6UL};
			static constexpr auto F0Offset{7UL};
			static constexpr auto IOROffset{8UL};
			static constexpr auto IBLIntensityOffset{9UL};
			static constexpr auto AutoIlluminationAmountOffset{10UL};
			static constexpr auto AOIntensityOffset{11UL};
			static constexpr auto AutoIlluminationColorOffset{12UL};

			/* Default values. */
			static constexpr auto DefaultAlbedoColor{Libs::PixelFactory::Grey};
			static constexpr auto DefaultRoughness{0.5F};
			static constexpr auto DefaultMetalness{0.0F};
			static constexpr auto DefaultNormalScale{1.0F};
			static constexpr auto DefaultF0{0.04F}; /* Standard F0 for dielectrics. */
			static constexpr auto DefaultIOR{1.5F}; /* Standard IOR for glass. */
			static constexpr auto DefaultIBLIntensity{1.0F}; /* Full IBL contribution by default. */
			static constexpr auto DefaultAutoIlluminationColor{Libs::PixelFactory::Black};
			static constexpr auto DefaultAutoIlluminationAmount{0.0F}; /* Disabled by default. */
			static constexpr auto DefaultAOIntensity{1.0F}; /* Full AO contribution by default. */

			Physics::SurfacePhysicalProperties m_physicalSurfaceProperties{};
			std::unordered_map< ComponentType, std::unique_ptr< Component::Interface > > m_components{};
			BlendingMode m_blendingMode{BlendingMode::None};
			std::array< float, 16 > m_materialProperties{
				/* Albedo color (4) */
				DefaultAlbedoColor.red(), DefaultAlbedoColor.green(), DefaultAlbedoColor.blue(), DefaultAlbedoColor.alpha(),
				/* Roughness (1), Metalness (1), NormalScale (1), F0 (1) */
				DefaultRoughness, DefaultMetalness, DefaultNormalScale, DefaultF0,
				/* IOR (1), IBLIntensity (1), AutoIlluminationAmount (1), AOIntensity (1) */
				DefaultIOR, DefaultIBLIntensity, DefaultAutoIlluminationAmount, DefaultAOIntensity,
				/* AutoIlluminationColor (4) */
				DefaultAutoIlluminationColor.red(), DefaultAutoIlluminationColor.green(), DefaultAutoIlluminationColor.blue(), DefaultAutoIlluminationColor.alpha()
			};
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;
			std::shared_ptr< SharedUniformBuffer > m_sharedUniformBuffer;
			uint32_t m_sharedUBOIndex{0};
			bool m_videoMemoryUpdated{false};
			bool m_invertRoughness{false};
			bool m_useAutomaticReflection{false};
			/** @brief Binding point for automatic reflection cubemap (only valid when m_useAutomaticReflection is true). */
			uint32_t m_automaticReflectionBindingPoint{0};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using PBRMaterials = Container< Graphics::Material::PBRResource >;
}
