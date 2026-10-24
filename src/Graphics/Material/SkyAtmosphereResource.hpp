/*
 * src/Graphics/Material/SkyAtmosphereResource.hpp
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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "ScopedRegistration.hpp"
#include "Physics/SurfacePhysicalProperties.hpp"
#include "PixelFactory/Color.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn::Graphics
{
	class SharedUniformBuffer;
	class SkyAtmosphere;
}

namespace EmEn::Graphics::Material
{
	/**
	 * @brief The material that draws a PHYSICAL SKY on a background (Renderable::DynamicSkyResource): the planet's
	 * atmosphere seen from the camera, read from the tables of a Graphics::SkyAtmosphere, and the disc of its star.
	 * @note Unlit, opaque: what it writes IS the sky's radiance, in nits, absolute (the scene epilogue pre-exposes it).
	 * The direction of a fragment is the skybox vertex position (the cube is centred on the camera, never rotated).
	 * @note Descriptor set (per model layer): 0 the material UBO (Keys SkyGrading, SkyStar), 1 the atmosphere's
	 * parameters, 2 the sky-view table, 3 the transmittance table, 4 the frame texture (the star and the camera of the
	 * frame, written with the sky-view table by SkyAtmosphere::recordFrame()).
	 * @note The artistic controls (tint, intensity, saturation) leave the physics untouched at their defaults.
	 * @extends EmEn::Graphics::Material::Interface This is a material.
	 */
	class EMEN_LEAN_API SkyAtmosphereResource final : public Interface
	{
		friend class Resources::Container< SkyAtmosphereResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MaterialSkyAtmosphereResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/* JSON keys. */
			static constexpr auto JKTint{"Tint"};
			static constexpr auto JKIntensity{"Intensity"};
			static constexpr auto JKSaturation{"Saturation"};
			static constexpr auto JKStarDisc{"StarDisc"};
			static constexpr auto JKLimbDarkening{"LimbDarkening"};

			/** @brief The linear limb-darkening coefficient of the star's disc: the Sun's, in the visible. */
			static constexpr auto DefaultLimbDarkening{0.6F};

			/**
			 * @brief Constructs a sky atmosphere material.
			 * @param serviceProvider A reference to the resource manager through a service provider.
			 * @param name The name of the resource.
			 * @param resourceFlags The resource flag bits. Default none.
			 */
			SkyAtmosphereResource (Resources::AbstractServiceProvider & serviceProvider, const std::string & name, uint32_t resourceFlags = 0) noexcept;

			/** @brief Deleted copy constructor. */
			SkyAtmosphereResource (const SkyAtmosphereResource & copy) noexcept = delete;

			/** @brief Deleted move constructor. */
			SkyAtmosphereResource (SkyAtmosphereResource && copy) noexcept = delete;

			/** @brief Deleted copy assignment. */
			SkyAtmosphereResource & operator= (const SkyAtmosphereResource & copy) noexcept = delete;

			/** @brief Deleted move assignment. */
			SkyAtmosphereResource & operator= (SkyAtmosphereResource && copy) noexcept = delete;

			/**
			 * @brief Destructs the sky atmosphere material.
			 */
			~SkyAtmosphereResource () override
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

			/** @copydoc EmEn::Resources::ResourceTrait::classUID() const noexcept */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::is() const noexcept */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const noexcept */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Resources::ResourceTrait::load() noexcept */
			bool load () noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) noexcept */
			bool load (const Json::Value & data) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				return sizeof(*this);
			}

			/** @copydoc EmEn::Graphics::Material::Interface::updateVideoMemory() noexcept */
			bool updateVideoMemory () noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::isComplex() const noexcept */
			[[nodiscard]]
			bool
			isComplex () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::samplesTexture() const noexcept */
			[[nodiscard]]
			bool samplesTexture (const Vulkan::TextureInterface * texture) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::setupLightGenerator() const noexcept */
			[[nodiscard]]
			bool
			setupLightGenerator (Saphir::LightGenerator & /*lightGenerator*/) const noexcept override
			{
				/* Unlit: the sky IS light. */
				return true;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::generateVertexShaderCode() const noexcept */
			[[nodiscard]]
			bool generateVertexShaderCode (Saphir::Generator::Abstract & generator, Saphir::AbstractVertexStage & vertexShader) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::generateFragmentShaderCode() const noexcept */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::LightGenerator & lightGenerator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::surfacePhysicalProperties() const noexcept */
			[[nodiscard]]
			const Physics::SurfacePhysicalProperties &
			surfacePhysicalProperties () const noexcept override
			{
				return m_physicalSurfaceProperties;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::surfacePhysicalProperties() noexcept */
			[[nodiscard]]
			Physics::SurfacePhysicalProperties &
			surfacePhysicalProperties () noexcept override
			{
				return m_physicalSurfaceProperties;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::frameCount() const noexcept */
			[[nodiscard]]
			uint32_t
			frameCount () const noexcept override
			{
				return 1;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::duration() const noexcept */
			[[nodiscard]]
			uint32_t
			duration () const noexcept override
			{
				return 0;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::frameIndexAt() const noexcept */
			[[nodiscard]]
			uint32_t
			frameIndexAt (uint32_t /*sceneTime*/) const noexcept override
			{
				return 0;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::enableBlending() noexcept */
			void enableBlending (BlendingMode mode) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::blendingMode() const noexcept */
			[[nodiscard]]
			BlendingMode
			blendingMode () const noexcept override
			{
				return BlendingMode::None;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::fragmentColor() const noexcept */
			[[nodiscard]]
			std::string fragmentColor () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::descriptorSetLayout() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::DescriptorSetLayout >
			descriptorSetLayout () const noexcept override
			{
				return m_descriptorSetLayout;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::UBOIndex() const noexcept */
			[[nodiscard]]
			uint32_t
			UBOIndex () const noexcept override
			{
				return m_sharedUBOIndex;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::UBOAlignment() const noexcept */
			[[nodiscard]]
			uint32_t UBOAlignment () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::UBOOffset() const noexcept */
			[[nodiscard]]
			uint32_t UBOOffset () const noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::descriptorSet() const noexcept */
			[[nodiscard]]
			const Vulkan::DescriptorSet *
			descriptorSet () const noexcept override
			{
				return m_descriptorSet.get();
			}

			/** @copydoc EmEn::Graphics::Material::Interface::getUniformBlock() const noexcept */
			[[nodiscard]]
			Saphir::Declaration::UniformBlock getUniformBlock (uint32_t set, uint32_t binding) const noexcept override;

			/**
			 * @brief Sets the atmosphere this material draws. Before the material is created only: its tables are bound
			 * in the descriptor set.
			 * @note Without one, create() builds an Earth atmosphere of its own.
			 * @param atmosphere A reference to the atmosphere smart pointer.
			 * @return bool False when the material is already created.
			 */
			bool setAtmosphere (const std::shared_ptr< SkyAtmosphere > & atmosphere) noexcept;

			/**
			 * @brief Returns the atmosphere this material draws.
			 * @return std::shared_ptr< SkyAtmosphere >
			 */
			[[nodiscard]]
			std::shared_ptr< SkyAtmosphere >
			atmosphere () const noexcept
			{
				return m_atmosphere;
			}

			/**
			 * @brief Sets the tint and the intensity of the whole sky (artistic). White and 1 leave it physical.
			 * @param tint A linear colour multiplied with the sky.
			 * @param intensity A scale of the sky's radiance.
			 */
			void setGrading (const Base::PixelFactory::Color< float > & tint, float intensity) noexcept;

			/**
			 * @brief Sets the saturation of the sky (artistic): 0 grey, 1 physical, above 1 more saturated.
			 * @param saturation The saturation factor.
			 */
			void setSaturation (float saturation) noexcept;

			/**
			 * @brief Enables or disables the star's disc.
			 * @param state The state.
			 */
			void enableStarDisc (bool state) noexcept;

			/**
			 * @brief Sets the linear limb-darkening coefficient of the star's disc: I(mu) = 1 - u (1 - mu), the disc's
			 * mean radiance unchanged (its illuminance is the star's).
			 * @param coefficient In [0, 1]. 0 is a flat disc.
			 */
			void setLimbDarkening (float coefficient) noexcept;

			/**
			 * @brief Returns the tint.
			 * @return Base::PixelFactory::Color< float >
			 */
			[[nodiscard]]
			Base::PixelFactory::Color< float >
			tint () const noexcept
			{
				return {m_properties[GradingOffset], m_properties[GradingOffset + 1], m_properties[GradingOffset + 2], 1.0F};
			}

			/**
			 * @brief Returns the intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_properties[GradingOffset + 3];
			}

			/**
			 * @brief Returns the saturation.
			 * @return float
			 */
			[[nodiscard]]
			float
			saturation () const noexcept
			{
				return m_properties[StarOffset];
			}

			/**
			 * @brief Returns whether the star's disc is drawn.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isStarDiscEnabled () const noexcept
			{
				return m_properties[StarOffset + 1] > 0.5F;
			}

			/**
			 * @brief Returns the limb-darkening coefficient.
			 * @return float
			 */
			[[nodiscard]]
			float
			limbDarkening () const noexcept
			{
				return m_properties[StarOffset + 2];
			}

		private:

			/** @copydoc EmEn::Graphics::Material::Interface::create() noexcept */
			[[nodiscard]]
			bool create (Renderer & renderer) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::destroy() noexcept */
			void destroy () noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::getSharedUniformBufferIdentifier() const noexcept */
			[[nodiscard]]
			std::string
			getSharedUniformBufferIdentifier () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Graphics::Material::Interface::createElementInSharedBuffer() noexcept */
			[[nodiscard]]
			bool createElementInSharedBuffer (Renderer & renderer, const std::string & identifier) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::createDescriptorSetLayout() noexcept */
			[[nodiscard]]
			bool createDescriptorSetLayout (Vulkan::LayoutManager & layoutManager, const std::string & identifier) noexcept override;

			/** @copydoc EmEn::Graphics::Material::Interface::createDescriptorSet() noexcept */
			[[nodiscard]]
			bool createDescriptorSet (Renderer & renderer, const Vulkan::UniformBufferObject & uniformBufferObject) noexcept override;

			/**
			 * @brief Registers the material for a video memory update after a property change (any thread).
			 */
			void markVideoMemoryDirty () noexcept;

			/* UBO layout (std140, two vec4 — Keys SkyGrading, SkyStar). */
			static constexpr auto GradingOffset{0UL};
			static constexpr auto StarOffset{4UL};
			static constexpr auto PropertiesSize{8UL};

			Physics::SurfacePhysicalProperties m_physicalSurfaceProperties;
			std::array< float, PropertiesSize > m_properties{
				/* Tint (rgb), intensity. */
				1.0F, 1.0F, 1.0F, 1.0F,
				/* Saturation, star disc on, limb darkening, unused. */
				1.0F, 1.0F, DefaultLimbDarkening, 0.0F
			};
			std::shared_ptr< SkyAtmosphere > m_atmosphere;
			std::shared_ptr< SharedUniformBuffer > m_sharedUniformBuffer;
			/* The seat in m_sharedUniformBuffer (Ave Robustus II P4): its end frees it. Declared after the buffer. */
			Base::ScopedRegistration m_sharedUBOSeat;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::unique_ptr< Vulkan::DescriptorSet > m_descriptorSet;
			Renderer * m_renderer{nullptr}; ///< Set by create(): the flusher of dynamic properties (owned by the engine).
			uint32_t m_sharedUBOIndex{0};
			std::atomic< bool > m_videoMemoryUpdated{false}; ///< Raised by markVideoMemoryDirty() (any thread), lowered by updateVideoMemory() (render thread) BEFORE it reads the properties.
	};
}

/* Expose the resource manager as a convenient type. */
namespace EmEn::Resources
{
	using SkyAtmosphereMaterials = Container< Graphics::Material::SkyAtmosphereResource >;
}
