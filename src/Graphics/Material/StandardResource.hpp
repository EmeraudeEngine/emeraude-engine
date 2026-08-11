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
	 * @brief The standard material resource of the engine.
	 * @extends EmEn::Graphics::Material::Interface This is a material.
	 */
	class EMEN_API StandardResource final : public Interface
	{
		friend class Resources::Container< StandardResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MaterialStandardResource"};

			/* Shader-specific keys. */
			static constexpr auto SurfaceAmbientColor{"SurfaceAmbientColor"};
			static constexpr auto SurfaceDiffuseColor{"SurfaceDiffuseColor"};
			static constexpr auto SurfaceSpecularColor{"SurfaceSpecularColor"};
			static constexpr auto SurfaceAutoIlluminationColor{"SurfaceAutoIlluminationColor"};
			static constexpr auto SurfaceOpacityAmount{"SurfaceOpacityAmount"};
			static constexpr auto SurfaceNormalVector{"SurfaceNormalVector"};
			static constexpr auto SurfaceReflectionColor{"SurfaceReflectionColor"};
			static constexpr auto SurfaceRefractionColor{"SurfaceRefractionColor"};
			static constexpr auto SurfaceHeightValue{"SurfaceHeight"};
			static constexpr auto SurfaceReflectivityMap{"SurfaceReflectivityMap"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Few};

			/**
			 * @brief Constructs a standard material.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			StandardResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
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

			/** @copydoc EmEn::Graphics::Material::Interface::isCreated() */
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

			/**
			 * @brief Sets the ambient component as a color.
			 * @warning This function is available before creation time.
			 * @param color A reference to a color.
			 * @return bool
			 */
			bool setAmbientComponent (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Sets the ambient component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @return bool
			 */
			bool setAmbientComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept;

			/**
			 * @brief Alias of setDiffuseComponent for cross-material compatibility (PBR convention).
			 * @warning This function is available before creation time.
			 * @note Tracks the albedo color for the PBR-to-Phong specular conversion driven by
			 * setRoughnessComponent / setMetalnessComponent. Calling these setters in any order
			 * yields a consistent specular component (color, shininess).
			 * @param color A reference to a color.
			 * @return bool
			 */
			bool setAlbedoComponent (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Alias of setDiffuseComponent for cross-material compatibility (PBR convention).
			 * @warning This function is available before creation time.
			 * @note Texture is forwarded to the diffuse component. The albedo color tracked for
			 * specular conversion keeps its previously set value (default grey otherwise).
			 * @param texture A reference to a texture smart pointer.
			 * @return bool
			 */
			bool setAlbedoComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept;

			/**
			 * @brief Sets the roughness factor (PBR convention) — converted to shininess.
			 * @warning This function is available before creation time.
			 * @note Conversion: shininess = (1 - roughness)² × MaxPBRShininess. Final specular
			 * (color, shininess) is recomputed from the tracked albedo / metalness / roughness.
			 * @param value Roughness in [0, 1]. Default DefaultPBRRoughness.
			 * @return bool
			 */
			bool setRoughnessComponent (float value = DefaultPBRRoughness) noexcept;

			/**
			 * @brief Sets the roughness factor (PBR convention) — texture is ignored.
			 * @warning This function is available before creation time.
			 * @note Standard material has no roughness map; only @a value is honored.
			 * @param texture Ignored (kept for API parity with PBRResource).
			 * @param value Roughness in [0, 1]. Default DefaultPBRRoughness.
			 * @param invert If true, the value is inverted before use (glossiness → roughness).
			 * @param sourceChannel Ignored (kept for API parity with PBRResource).
			 * @return bool
			 */
			bool setRoughnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float value = DefaultPBRRoughness, bool invert = false, Base::PixelFactory::Channel sourceChannel = Base::PixelFactory::Channel::Red) noexcept;

			/**
			 * @brief Sets the metalness factor (PBR convention) — converted to specular tint.
			 * @warning This function is available before creation time.
			 * @note Conversion: specularColor = mix(vec3(0.04), albedo, metalness). Final specular
			 * (color, shininess) is recomputed from the tracked albedo / metalness / roughness.
			 * @param value Metalness in [0, 1]. Default DefaultPBRMetalness.
			 * @return bool
			 */
			bool setMetalnessComponent (float value = DefaultPBRMetalness) noexcept;

			/**
			 * @brief Sets the metalness factor (PBR convention) — texture is ignored.
			 * @warning This function is available before creation time.
			 * @note Standard material has no metalness map; only @a value is honored.
			 * @param texture Ignored (kept for API parity with PBRResource).
			 * @param value Metalness in [0, 1]. Default DefaultPBRMetalness.
			 * @param sourceChannel Ignored (kept for API parity with PBRResource).
			 * @return bool
			 */
			bool setMetalnessComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float value = DefaultPBRMetalness, Base::PixelFactory::Channel sourceChannel = Base::PixelFactory::Channel::Red) noexcept;

			/**
			 * @brief No-op alias for API parity with PBRResource — Standard has no AO channel.
			 * @warning This function is available before creation time.
			 * @param texture Ignored.
			 * @param intensity Ignored.
			 * @return bool Always true.
			 */
			bool
			setAmbientOcclusionComponent (const std::shared_ptr< TextureResource::Abstract > & /*texture*/, float /*intensity*/ = 1.0F) noexcept
			{
				return true;
			}

			/**
			 * @brief No-op alias for API parity with PBRResource — Standard has no clearcoat layer.
			 * @return bool Always true.
			 */
			bool
			setClearCoatComponent (float /*factor*/, float /*roughness*/) noexcept
			{
				return true;
			}

			/**
			 * @brief No-op alias for API parity with PBRResource — Standard has no sheen layer.
			 * @return bool Always true.
			 */
			bool
			setSheenComponent (const Base::PixelFactory::Color< float > & /*color*/, float /*roughness*/) noexcept
			{
				return true;
			}

			/**
			 * @brief No-op alias for API parity with PBRResource — Standard has no transmission.
			 * @return bool Always true.
			 */
			bool
			setTransmissionComponent (float /*factor*/) noexcept
			{
				return true;
			}

			/**
			 * @brief No-op alias for API parity with PBRResource — Standard has no transmission.
			 * @return bool Always true.
			 */
			bool
			setTransmissionComponentFromGrabPass (float /*factor*/) noexcept
			{
				return true;
			}

			/**
			 * @brief Sets the UV transform of a texture component (KHR_texture_transform).
			 * @warning This function is available before creation time.
			 * @note Cross-material alias: ComponentType::Albedo maps to the Diffuse component.
			 * Components without a transform slot (Ambient, Specular, ReflectivityMap) return false.
			 * @param componentType The targeted component.
			 * @param scale The UV scale factors.
			 * @param offset The UV offsets.
			 * @return bool True when the component exists as a texture and supports a transform slot.
			 */
			bool setComponentUVWTransform (ComponentType componentType, const Base::Math::Vector< 2, float > & scale, const Base::Math::Vector< 2, float > & offset) noexcept;

			/**
			 * @brief No-op alias for API parity with PBRResource — Standard has no iridescence.
			 * @return bool Always true.
			 */
			bool
			setIridescenceComponent (float /*factor*/) noexcept
			{
				return true;
			}

			/**
			 * @brief Sets the diffuse component as a color.
			 * @warning This function is available before creation time.
			 * @param color A reference to a color.
			 * @return bool
			 */
			bool setDiffuseComponent (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Sets the diffuse component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @return bool
			 */
			bool setDiffuseComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept;

			/**
			 * @brief Sets the specular component as a color.
			 * @warning This function is available before creation time.
			 * @param color A reference to a color.
			 * @param shininess A positive value.
			 * @return bool
			 */
			bool setSpecularComponent (const Base::PixelFactory::Color< float > & color, float shininess = DefaultShininess) noexcept;

			/**
			 * @brief Sets the specular component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @param shininess A positive value. Default 32.0.
			 * @return bool
			 */
			bool setSpecularComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float shininess = DefaultShininess) noexcept;

			/**
			 * @brief Sets the opacity component as a value.
			 * @warning This function is available before creation time.
			 * @param amount The control amount. Default 100%.
			 * @return bool
			 */
			bool setOpacityComponent (float amount = DefaultOpacity) noexcept;

			/**
			 * @brief Sets the opacity component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @param amount The control amount. Default 100%.
			 * @return bool
			 */
			bool setOpacityComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount = DefaultOpacity) noexcept;

			/**
			 * @brief Sets the auto-illumination component as a value.
			 * @warning This function is available before creation time.
			 * @note The auto-illumination will light globally up the diffuse color.
			 * @param amount The control amount. Default 100%.
			 * @return bool
			 */
			bool setAutoIlluminationComponent (float amount = DefaultAutoIlluminationAmount) noexcept;

			/**
			 * @brief Sets the auto-illumination component as a color.
			 * @warning This function is available before creation time.
			 * @note The auto-illumination will light globally up a custom color over the final result.
			 * @param color A reference to a color.
			 * @param amount The control amount. Default 100%.
			 * @return bool
			 */
			bool setAutoIlluminationComponent (const Base::PixelFactory::Color< float > & color, float amount = DefaultAutoIlluminationAmount) noexcept;

			/**
			 * @brief Sets the auto-illumination component as a texture.
			 * @warning This function is available before creation time.
			 * @note The auto-illumination will light up the final result using a texture.
			 * @param texture A reference to a texture smart pointer.
			 * @param amount The control amount. Default 100%.
			 * @return bool
			 */
			bool setAutoIlluminationComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount = DefaultAutoIlluminationAmount) noexcept;

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
			 * @brief Sets the reflection component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @param amount The control amount. Default 50%.
			 * @return bool
			 */
			bool setReflectionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount = DefaultReflectionAmount) noexcept;

			/**
			 * @brief Sets the reflection component using a render target (for dynamic cubemap reflections).
			 * @warning This function is available before creation time.
			 * @note Useful for dynamic reflections using RenderTarget::Texture cubemaps.
			 * @param renderTarget A reference to a texture interface smart pointer (e.g., RenderTarget::Texture).
			 * @param amount The control amount. Default 50%.
			 * @return bool
			 */
			bool setReflectionComponentFromRenderTarget (const std::shared_ptr< Vulkan::TextureInterface > & renderTarget, float amount = DefaultReflectionAmount) noexcept;

			/**
			 * @brief Sets the reflection component using an environment cubemap from the scene.
			 * @note When enabled, the material will use the scene's environment cubemap for reflection
			 * instead of a material-specific texture. This is resolved at render time.
			 * @param amount The reflection amount. Default 50%.
			 * @return bool
			 */
			bool setReflectionComponentFromEnvironmentCubemap (float amount = DefaultReflectionAmount) noexcept;

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
			 * @param ior The index of refraction. Default 1.5 (glass).
			 * @param amount The refraction amount. Default 95%.
			 * @return bool
			 */
			bool setRefractionComponentFromEnvironmentCubemap (float ior = DefaultRefractionIOR, float amount = DefaultRefractionAmount) noexcept;

			/**
			 * @brief Sets the refraction component as a texture (for glass/water effects).
			 * @warning This function is available before creation time.
			 * @param texture A reference to a cubemap texture smart pointer.
			 * @param ior The index of refraction. Default 1.5 (glass).
			 * @param amount The control amount. Default 95%.
			 * @return bool
			 */
			bool setRefractionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float ior = DefaultRefractionIOR, float amount = DefaultRefractionAmount) noexcept;

			/**
			 * @brief Sets the refraction component using a render target (for dynamic cubemap refractions).
			 * @warning This function is available before creation time.
			 * @note Useful for dynamic refractions using RenderTarget::Texture cubemaps.
			 * @param renderTarget A reference to a texture interface smart pointer (e.g., RenderTarget::Texture).
			 * @param ior The index of refraction. Default 1.5 (glass).
			 * @param amount The control amount. Default 95%.
			 * @return bool
			 */
			bool setRefractionComponentFromRenderTarget (const std::shared_ptr< Vulkan::TextureInterface > & renderTarget, float ior = DefaultRefractionIOR, float amount = DefaultRefractionAmount) noexcept;

			/**
			 * @brief Sets the reflectivity map component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer (reflectivity map).
			 * @return bool
			 */
			bool setReflectivityMapComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept;

			/** @copydoc EmEn::Graphics::Material::Interface::useEnvironmentCubemap() const noexcept */
			[[nodiscard]]
			bool
			useEnvironmentCubemap () const noexcept override
			{
				return m_isUsingEnvironmentCubemap || m_isUsingEnvironmentCubemapForRefraction;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::samplesTexture() const noexcept */
			[[nodiscard]]
			bool samplesTexture (const Vulkan::TextureInterface * texture) const noexcept override;

			/**
			 * @brief Returns whether a material component is present.
			 * @param componentType The type of component.
			 * @return bool
			 */
			[[nodiscard]]
			bool isComponentPresent (ComponentType componentType) const noexcept;

			/**
			 * @brief Changes the ambient color.
			 * @note This is a dynamic property.
			 * @param color A reference to a color.
			 * @return void
			 */
			void setAmbientColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Changes the diffuse color.
			 * @note This is a dynamic property.
			 * @param color A reference to a color.
			 * @return void
			 */
			void setDiffuseColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Changes the albedo color. Cross-material alias of setDiffuseColor().
			 * @note Exists so a caller driving both PBRResource and StandardResource through one
			 * generic lambda can set the albedo tint without knowing the concrete container.
			 * @param color A reference to a color.
			 * @return void
			 */
			void
			setAlbedoColor (const Base::PixelFactory::Color< float > & color) noexcept
			{
				this->setDiffuseColor(color);
			}

			/**
			 * @brief Changes the specular color.
			 * @note This is a dynamic property.
			 * @param color A reference to a color.
			 * @return void
			 */
			void setSpecularColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Changes the auto-illumination color.
			 * @note This is a dynamic property.
			 * @param color A reference to a color.
			 * @return void
			 */
			void setAutoIlluminationColor (const Base::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Converts an authored GLOSSINESS into a Blinn-Phong specular EXPONENT.
			 * @note This is the semantic boundary of the "Shininess" MANIFEST KEY: material JSON
			 * files author a perceptual glossiness in [0, 1], while the whole C++ API and the
			 * shader uniform carry a real Blinn-Phong exponent. The mapping is exponential
			 * (UE3 convention) so the perceived change stays regular across the range:
			 * 0.0 -> 2, 0.1 -> 4, 0.2 -> 8, 0.4 -> 32, 0.5 -> 45, 0.9 -> 1024, 1.0 -> 2048.
			 * @warning Do NOT apply this to values coming from setShininess(), setSpecularComponent()
			 * or the PBR-to-Phong conversion (setRoughness): those are ALREADY exponents. Remapping
			 * an exponent of 32 as a glossiness would yield exp2(321).
			 * @param glossiness The authored glossiness, clamped to [0, 1].
			 * @return float The Blinn-Phong exponent.
			 */
			[[nodiscard]]
			static float specularExponentFromGlossiness (float glossiness) noexcept;

			/**
			 * @brief Changes the specular shininess amount.
			 * @note This is a dynamic property.
			 * @param value A positive value.
			 * @return void
			 */
			void setShininess (float value) noexcept;

			/**
			 * @brief Changes the opacity.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0
			 * @return void
			 */
			void setOpacity (float value) noexcept;

			/**
			 * @brief Sets an alpha value below the pixel will be discarded.
			 * @param value A value between 0.0 and 1.0
			 * @return void
			 */
			void setAlphaThresholdToDiscard (float value) noexcept;

			/**
			 * @brief Changes the auto-illumination amount of light.
			 * @note This is a dynamic property.
			 * @param value A positive value.
			 * @return void
			 */
			void setAutoIlluminationAmount (float value) noexcept;

			/**
			 * @brief Changes the normal mapping scale factor.
			 * @note This is a dynamic property.
			 * @param value A value.
			 * @return void
			 */
			void setNormalScale (float value) noexcept;

			/**
			 * @brief Changes the reflection amount.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0
			 * @return void
			 */
			void setReflectionAmount (float value) noexcept;

			/**
			 * @brief Changes the refraction amount.
			 * @note This is a dynamic property.
			 * @param value A value between 0.0 and 1.0
			 * @return void
			 */
			void setRefractionAmount (float value) noexcept;

			/**
			 * @brief Changes the refraction index of refraction.
			 * @note This is a dynamic property.
			 * @param value The IOR value (typically 1.0-2.5, glass=1.5, water=1.33, diamond=2.42)
			 * @return void
			 */
			void setRefractionIOR (float value) noexcept;

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
			 * @brief Parses the ambient component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseAmbientComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the diffuse component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseDiffuseComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the specular component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseSpecularComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the opacity component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseOpacityComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Parses the auto-illumination component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseAutoIlluminationComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

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
			 * @brief Parses the reflectivity map component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseReflectivityMapComponent (const Json::Value & data, Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Updates the UBO with material properties.
			 * @return void
			 */
			bool updateVideoMemory () noexcept;

			/**
			 * @brief Generates the fragment shader code for a specific component.
			 * @param componentType The component type to find in the material.
			 * @param codeGenerator A reference to a function to generate the actual code.
			 * @param fragmentShader A reference to the fragment shader being generated.
			 * @param materialSet The current material set.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateTextureComponentFragmentShader (ComponentType componentType, const std::function< bool (Saphir::FragmentShader &, const Component::Texture *) > & codeGenerator, Saphir::FragmentShader & fragmentShader, uint32_t materialSet) const noexcept;

			/**
			 * @brief Generates the fragment shader code for bindless reflection.
			 * @note This method generates shader code that uses the global bindless texture array
			 * instead of per-material samplers for reflection cubemaps.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader being generated.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateBindlessReflectionFragmentShader (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept;

			/**
			 * @brief Generates the fragment shader code for bindless refraction.
			 * @note This method generates shader code that uses the global bindless texture array
			 * instead of per-material samplers for refraction cubemaps.
			 * @param generator A reference to the shader generator.
			 * @param fragmentShader A reference to the fragment shader being generated.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateBindlessRefractionFragmentShader (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept;

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
			 * @note Cross-material alias mapping: ComponentType::Albedo targets the Diffuse slot.
			 * @param componentType The component type (selects the UBO transform slot).
			 * @param component A pointer to the texture component.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string transformedTexCoords (ComponentType componentType, const Component::Texture * component) const noexcept;

			/**
			 * @brief Copies each texture component's UV transform into its material UBO slot.
			 * @note Called at creation time, before the first video memory update.
			 * @return void
			 */
			void syncComponentUVWTransforms () noexcept;

			/* Uniform buffer object offset to write data. */
			static constexpr auto AmbientColorOffset{0UL};
			static constexpr auto DiffuseColorOffset{4UL};
			static constexpr auto SpecularColorOffset{8UL};
			static constexpr auto AutoIlluminationColorOffset{12UL};
			static constexpr auto ShininessOffset{16UL};
			static constexpr auto OpacityOffset{17UL};
			static constexpr auto AutoIlluminationAmountOffset{18UL};
			static constexpr auto NormalScaleOffset{19UL};
			static constexpr auto ReflectionAmountOffset{20UL};
			static constexpr auto RefractionAmountOffset{21UL};
			static constexpr auto RefractionIOROffset{22UL};
			static constexpr auto HeightScaleOffset{23UL};
			static constexpr auto EmissiveStrengthOffset{24UL};
			/* Per-component UV transforms (KHR_texture_transform): vec4 = (scale.xy, offset.zw).
			 * Neutral (1,1,0,0) — applied UNCONDITIONALLY at the sampling sites. */
			static constexpr auto DiffuseUVWTransformOffset{28UL};
			static constexpr auto NormalUVWTransformOffset{32UL};
			static constexpr auto OpacityUVWTransformOffset{36UL};
			static constexpr auto AutoIlluminationUVWTransformOffset{40UL};

			/* Default values. */
			static constexpr auto DefaultAmbientColor{Base::PixelFactory::DarkGrey};
			/* White, NOT grey: the diffuse colour is also the TINT factor multiplying the diffuse
			 * texture in the generated shader, so its neutral value has to be the multiplicative
			 * identity. A grey default would darken every textured material by half. */
			static constexpr auto DefaultDiffuseColor{Base::PixelFactory::White};
			static constexpr auto DefaultSpecularColor{Base::PixelFactory::White};
			static constexpr auto DefaultAutoIlluminationColor{Base::PixelFactory::White};
			static constexpr auto DefaultShininess{32.0F}; /* Blinn-Phong EXPONENT (C++ API unit). */
			/* Authored GLOSSINESS in [0,1] used when a manifest declares no "Shininess" key.
			 * 0.4 maps to DefaultShininess through specularExponentFromGlossiness(), so an
			 * absent key keeps the historical exponent of 32. */
			static constexpr auto DefaultGlossiness{0.4F};
			static constexpr auto DefaultOpacity{1.0F};
			static constexpr auto DefaultAutoIlluminationAmount{1.0F};
			static constexpr auto DefaultNormalScale{1.0F};
			static constexpr auto DefaultReflectionAmount{0.5F};
			static constexpr auto DefaultRefractionAmount{0.95F};
			static constexpr auto DefaultRefractionIOR{1.5F}; /* Glass */
			static constexpr auto DefaultHeightScale{0.02F}; /* Parallax occlusion mapping depth. */
			static constexpr auto DefaultEmissiveStrength{1.0F}; /* KHR_materials_emissive_strength: HDR multiplier (1.0 = pass-through). */

			/* PBR-to-Phong cross-material conversion (used by setAlbedo/setRoughness/setMetalness aliases). */
			static constexpr auto DefaultPBRRoughness{0.5F};
			static constexpr auto DefaultPBRMetalness{0.0F};
			static constexpr auto MaxPBRShininess{128.0F};
			static constexpr auto DielectricF0{0.04F};

			Physics::SurfacePhysicalProperties m_physicalSurfaceProperties;
			std::unordered_map< ComponentType, std::unique_ptr< Component::Interface > > m_components;
			BlendingMode m_blendingMode{BlendingMode::None};
			std::array< float, 44 > m_materialProperties{
				/* Ambient color (4), */
				DefaultAmbientColor.red(), DefaultAmbientColor.green(), DefaultAmbientColor.blue(), DefaultDiffuseColor.alpha(),
				/* Diffuse color (4), */
				DefaultDiffuseColor.red(), DefaultDiffuseColor.green(), DefaultDiffuseColor.blue(), DefaultDiffuseColor.alpha(),
				/* Specular color (4), */
				DefaultSpecularColor.red(), DefaultSpecularColor.green(), DefaultSpecularColor.blue(), DefaultSpecularColor.alpha(),
				/* Auto-illumination color (4), */
				DefaultAutoIlluminationColor.red(), DefaultAutoIlluminationColor.green(), DefaultAutoIlluminationColor.blue(), DefaultAutoIlluminationColor.alpha(),
				/* Shininess (1), Opacity (1), AutoIlluminationColor (1), NormalScale (1). */
				DefaultShininess, DefaultOpacity, DefaultAutoIlluminationAmount, DefaultNormalScale,
				/* ReflectionAmount (1), RefractionAmount (1), RefractionIOR (1), HeightScale (1). */
				DefaultReflectionAmount, DefaultRefractionAmount, DefaultRefractionIOR, DefaultHeightScale,
				/* EmissiveStrength (1) + padding (3) for STD140 alignment */
				DefaultEmissiveStrength, 0.0F, 0.0F, 0.0F,
				/* Per-component UV transforms (4 x vec4 = scale.xy, offset.zw), identity neutral:
				 * Diffuse, Normal, Opacity, AutoIllumination. */
				1.0F, 1.0F, 0.0F, 0.0F,
				1.0F, 1.0F, 0.0F, 0.0F,
				1.0F, 1.0F, 0.0F, 0.0F,
				1.0F, 1.0F, 0.0F, 0.0F
			};
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;
			std::shared_ptr< SharedUniformBuffer > m_sharedUniformBuffer;
			float m_alphaThresholdToDiscard{0.1F};
			uint32_t m_sharedUBOIndex{0};
			/* TODO: Unify video memory update mechanism between all materials. */
			/* PBR-to-Phong tracking (used by setAlbedo/setRoughness/setMetalness aliases). */
			Base::PixelFactory::Color< float > m_pbrAlbedoColor{DefaultDiffuseColor};
			float m_pbrRoughness{DefaultPBRRoughness};
			float m_pbrMetalness{DefaultPBRMetalness};
			bool m_videoMemoryUpdated{false};
			bool m_isUsingEnvironmentCubemap{false};
			/** @brief Explicitly authored cubemap reflection (texture mode): never replaced by SSR/RTR. */
			bool m_reflectionIsArtistic{false};
			/** @brief Reflection source is a render target (probe/mirror): absolute luminance, no environment luminance scale. */
			bool m_reflectionSourceIsAbsolute{false};
			/** @brief Refraction source is a render target: absolute luminance, no environment luminance scale. */
			bool m_refractionSourceIsAbsolute{false};
			bool m_isUsingEnvironmentCubemapForRefraction{false};
			bool m_useParallaxOcclusionMapping{false};
			mutable bool m_pomGenerationActive{false};
			float m_postProcessReflectivityAmount{-1.0F};

			/**
			 * @brief Recomputes the specular component (color, shininess) from the tracked
			 * PBR triple (albedo, roughness, metalness).
			 * @return void
			 */
			void recomputeSpecularFromPBR () noexcept;
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using StandardMaterials = Container< Graphics::Material::StandardResource >;
}
