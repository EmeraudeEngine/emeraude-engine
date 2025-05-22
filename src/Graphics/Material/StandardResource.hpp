/*
 * src/Graphics/Material/StandardResource.hpp
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
#include <array>
#include <unordered_map>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Resources/Container.hpp"
#include "Physics/PhysicalSurfaceProperties.hpp"
#include "Component/Interface.hpp"

/* Forward declarations. */
namespace EmEn
{
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
	class StandardResource final : public Interface
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

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::Few};

			/**
			 * @brief Constructs a material.
			 * @param name A reference to a string for the resource name.
			 * @param materialFlags The resource flag bits. Default none.
			 */
			explicit
			StandardResource (const std::string & name, uint32_t materialFlags = 0) noexcept
				: Interface{name, materialFlags}
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
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
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

			/** @copydoc EmEn::Graphics::Material::Interface::createOnHardware() */
			bool createOnHardware (Renderer & renderer) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::destroyFromHardware() */
			void destroyFromHardware () noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::isCreated() */
			[[nodiscard]]
			bool isCreated () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::isCreated() */
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

			/** @copydoc EmEn::Graphics::Material::Interface::physicalSurfaceProperties() const */
			[[nodiscard]]
			const Physics::PhysicalSurfaceProperties & physicalSurfaceProperties () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::physicalSurfaceProperties() */
			[[nodiscard]]
			Physics::PhysicalSurfaceProperties & physicalSurfaceProperties () noexcept override;

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
			bool setAmbientComponent (const Libs::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Sets the ambient component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @return bool
			 */
			bool setAmbientComponent (const std::shared_ptr< TextureResource::Abstract > & texture) noexcept;

			/**
			 * @brief Sets the diffuse component as a color.
			 * @warning This function is available before creation time.
			 * @param color A reference to a color.
			 * @return bool
			 */
			bool setDiffuseComponent (const Libs::PixelFactory::Color< float > & color) noexcept;

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
			bool setSpecularComponent (const Libs::PixelFactory::Color< float > & color, float shininess = DefaultShininess) noexcept;

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
			bool setAutoIlluminationComponent (const Libs::PixelFactory::Color< float > & color, float amount = DefaultAutoIlluminationAmount) noexcept;

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
			 * @brief Sets the reflection component as a texture.
			 * @warning This function is available before creation time.
			 * @param texture A reference to a texture smart pointer.
			 * @param amount The control amount. Default 50%.
			 * @return bool
			 */
			bool setReflectionComponent (const std::shared_ptr< TextureResource::Abstract > & texture, float amount = DefaultReflectionAmount) noexcept;

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
			void setAmbientColor (const Libs::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Changes the diffuse color.
			 * @note This is a dynamic property.
			 * @param color A reference to a color.
			 * @return void
			 */
			void setDiffuseColor (const Libs::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Changes the specular color.
			 * @note This is a dynamic property.
			 * @param color A reference to a color.
			 * @return void
			 */
			void setSpecularColor (const Libs::PixelFactory::Color< float > & color) noexcept;

			/**
			 * @brief Changes the auto-illumination color.
			 * @note This is a dynamic property.
			 * @param color A reference to a color.
			 * @return void
			 */
			void setAutoIlluminationColor (const Libs::PixelFactory::Color< float > & color) noexcept;

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

		private:

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

			/** @copydoc EmEn::Graphics::Material::Interface::onMaterialLoaded() */
			void onMaterialLoaded () noexcept override;

			/**
			 * @brief Parses the ambient component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param resources A reference to the resource manager.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseAmbientComponent (const Json::Value & data, Resources::Manager & resources) noexcept;

			/**
			 * @brief Parses the diffuse component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param resources A reference to the resource manager.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseDiffuseComponent (const Json::Value & data, Resources::Manager & resources) noexcept;

			/**
			 * @brief Parses the specular component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param resources A reference to the resource manager.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseSpecularComponent (const Json::Value & data, Resources::Manager & resources) noexcept;

			/**
			 * @brief Parses the opacity component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param resources A reference to the resource manager.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseOpacityComponent (const Json::Value & data, Resources::Manager & resources) noexcept;

			/**
			 * @brief Parses the auto-illumination component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param resources A reference to the resource manager.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseAutoIlluminationComponent (const Json::Value & data, Resources::Manager & resources) noexcept;

			/**
			 * @brief Parses the normal component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param resources A reference to the resource manager.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseNormalComponent (const Json::Value & data, Resources::Manager & resources) noexcept;

			/**
			 * @brief Parses the reflection component from JSON data.
			 * @param data A reference to the JSON data.
			 * @param resources A reference to the resource manager.
			 * @return bool
			 */
			[[nodiscard]]
			bool parseReflectionComponent (const Json::Value & data, Resources::Manager & resources) noexcept;

			/**
			 * @brief Creates the necessary data onto the GPU for this material.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createVideoMemory (Renderer & renderer) noexcept;

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
			 * @brief Returns the right texture coordinates for a component.
			 * @param component A pointer to the texture component.
			 * @return const char *
			 */
			[[nodiscard]]
			static const char * textCoords (const Component::Texture * component) noexcept;

			/* Flag names. */
			static constexpr auto VideoMemoryUpdateRequested{0UL};

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

			/* Default values. */
			static constexpr auto DefaultAmbientColor{Libs::PixelFactory::DarkGrey};
			static constexpr auto DefaultDiffuseColor{Libs::PixelFactory::Grey};
			static constexpr auto DefaultSpecularColor{Libs::PixelFactory::White};
			static constexpr auto DefaultAutoIlluminationColor{Libs::PixelFactory::White};
			static constexpr auto DefaultShininess{32.0F};
			static constexpr auto DefaultOpacity{1.0F};
			static constexpr auto DefaultAutoIlluminationAmount{1.0F};
			static constexpr auto DefaultNormalScale{1.0F};
			static constexpr auto DefaultReflectionAmount{0.5F};

			Physics::PhysicalSurfaceProperties m_physicalSurfaceProperties{};
			std::unordered_map< ComponentType, std::unique_ptr< Component::Interface > > m_components{};
			BlendingMode m_blendingMode{BlendingMode::None};
			std::array< float, 24 > m_materialProperties{
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
				/* ReflectionAmount (1), Unused (1), Unused (1), Unused (1). */
				DefaultReflectionAmount, 0.0F, 0.0F, 0.0F
			};
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;
			std::shared_ptr< SharedUniformBuffer > m_sharedUniformBuffer;
			float m_alphaThresholdToDiscard{0.1F};
			uint32_t m_sharedUBOIndex{0};
			std::array< bool, 8 > m_flags{
				true/*VideoMemoryUpdateRequested*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using StandardMaterials = Container< Graphics::Material::StandardResource >;
}
