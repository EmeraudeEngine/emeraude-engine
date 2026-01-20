/*
 * src/Scenes/LightSet.hpp
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
#include <cstddef>
#include <set>
#include <map>
#include <memory>
#include <mutex>

/* Local inclusions for inheritances. */
#include "Libs/ObservableTrait.hpp"

/* Local inclusions for usage. */
#include "Component/DirectionalLight.hpp"
#include "Component/PointLight.hpp"
#include "Component/SpotLight.hpp"
#include "Graphics/TextureResource/TextureCubemap.hpp"
#include "Saphir/StaticLighting.hpp"
#include "Vulkan/DescriptorSet.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class LayoutManager;
		class DescriptorSetLayout;
	}

	namespace Scenes
	{
		class Scene;
	}
}

namespace EmEn::Scenes
{
	/**
	 * @brief This class hold lights from a whole scene.
	 * @note [OBS][STATIC-OBSERVABLE]
	 * @extends EmEn::Libs::ObservableTrait The light set can be observed for light addition or removal.
	 */
	class LightSet final : public Libs::ObservableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LightSet"};

			/* Default variables. */
			static constexpr auto DefaultStaticLightingName{"Default"};
			static constexpr auto DefaultLightPercentToAmbient{0.1F};
			static constexpr auto DefaultAmbientLightIntensity{0.1F};

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				DirectionalLightAdded,
				PointLightAdded,
				SpotLightAdded,
				DirectionalLightRemoved,
				PointLightRemoved,
				SpotLightRemoved,
				AmbientLightChanged,
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs a light set.
			 */
			LightSet () noexcept = default;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			LightSet (const LightSet & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			LightSet (LightSet && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return LightSet &
			 */
			LightSet & operator= (const LightSet & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return LightSet &
			 */
			LightSet & operator= (LightSet && copy) noexcept = delete;

			/**
			 * @brief Destructs the light set.
			 */
			~LightSet () override = default;

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

			/**
			 * @brief Returns the light set mutex.
			 * @return std::mutex &
			 */
			[[nodiscard]]
			std::mutex &
			mutex () const noexcept
			{
				return m_lightsAccess;
			}

			/**
			 * @brief Enables the lighting for the scene.
			 * @return void
			 */
			void
			enable () noexcept
			{
				m_enabled = true;
			}

			/**
			 * @brief Returns whether the lighting has been enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return m_enabled;
			}

			/**
			 * @brief Enables the lighting for the scene with static light.
			 * @return Saphir::StaticLighting &
			 */
			Saphir::StaticLighting &
			enableAsStaticLighting () noexcept
			{
				this->enable();
				this->setStaticLightingState(true);

				return this->getOrCreateStaticLighting(DefaultStaticLightingName);
			}

			/**
			 * @brief Initializes the light set GPU resources.
			 * @param scene A reference to a scene.
			 * @return bool
			 */
			[[nodiscard]]
			bool initialize (Scene & scene) noexcept;

			/**
			 * @brief Releases the GPU resources.
			 * @param scene A reference to a scene.
			 * @return bool
			 */
			bool terminate (Scene & scene) noexcept;

			/**
			 * @brief Enables the use of static lighting incorporated to the shaders.
			 * @param state The state.
			 * @return void
			 */
			void
			setStaticLightingState (bool state) noexcept
			{
				m_useStaticLighting = state;

				if ( state )
				{
					this->getOrCreateDefaultStaticLighting();
				}
			}

			/**
			 * @brief Returns whether static lighting is used.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUsingStaticLighting () const noexcept
			{
				return m_useStaticLighting;
			}

			/**
			 * @brief Returns the shared buffer uniform for directional light buffer.
			 * @return std::shared_ptr< Graphics::SharedUniformBuffer >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::SharedUniformBuffer >
			directionalLightBuffer () const noexcept
			{
				return m_directionalLightBuffer;
			}

			/**
			 * @brief Returns the shared buffer uniform for point light buffer.
			 * @return std::shared_ptr< Graphics::SharedUniformBuffer >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::SharedUniformBuffer >
			pointLightBuffer () const noexcept
			{
				return m_pointLightBuffer;
			}

			/**
			 * @brief Returns the shared buffer uniform for spotlight buffer.
			 * @return std::shared_ptr< Graphics::SharedUniformBuffer >
			 */
			[[nodiscard]]
			std::shared_ptr< Graphics::SharedUniformBuffer >
			spotLightBuffer () const noexcept
			{
				return m_spotLightBuffer;
			}

			/**
			 * @brief Sets a base ambient light for the scene.
			 * @note This will have an effect even if no light is in the scene.
			 * @param color The color of the global basis ambient light.
			 * @return void
			 */
			void
			setAmbientLightColor (const Libs::PixelFactory::Color< float > & color) noexcept
			{
				m_ambientLightColor = color;

				this->notify(AmbientLightChanged);
			}

			/**
			 * @brief Sets a base ambient light for the scene using a cubemap for averaging the color.
			 * @note This will have an effect even if there is no are in the scene.
			 * @param cubemap Use the average color of the cubemap to set the global base ambient light.
			 * @param percent Set the amount of the retrieved color (intensity) from the cubemap. Default 20%.
			 * @return void
			 */
			void
			setAmbientLightColor (const std::shared_ptr< Graphics::TextureResource::TextureCubemap > & cubemap, float percent = 0.2F) noexcept
			{
				m_ambientLightColor = cubemap->averageColor() * Libs::Math::clampToUnit(percent);

				this->notify(AmbientLightChanged);
			}

			/**
			 * @brief Sets the ambient light intensity.
			 * @param intensity The light intensity value.
			 * @return void
			 */
			void
			setAmbientLightIntensity (float intensity) noexcept
			{
				m_ambientLightIntensity = intensity;

				this->notify(AmbientLightChanged);
			}

			/**
			 * @brief Returns the ambient light color.
			 * @return const Libraries::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Libs::PixelFactory::Color< float > &
			ambientLightColor () const noexcept
			{
				return m_ambientLightColor;
			}

			/**
			 * @brief Returns the ambient light intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			ambientLightIntensity () const noexcept
			{
				return m_ambientLightIntensity;
			}

			/**
			 * @brief When computing the final ambient lighting, use a fraction of
			 * light color instead of the ambient component of each light source.
			 * FIXME: Re-enable this features !
			 * @param state Enable or disable the computation.
			 * @param factor The percentage of each color light. Default 10%.
			 * @return void
			 */
			void
			enableAmbientGenerationFromLights (bool state, float factor = DefaultLightPercentToAmbient) noexcept
			{
				m_createAmbientFromLights = state;
				m_lightPercentToAmbient = Libs::Math::clampToUnit(factor);
			}

			/**
			 * @brief Enables the light distance for ambient generation.
			 * @param state The state.
			 * @return void
			 */
			void
			enableLightDistanceForAmbientGeneration (bool state) noexcept
			{
				m_useLightDistance = state;
			}

			/**
			 * @brief Adds a directional light to the light set for the scene.
			 * @param scene A reference to a scene.
			 * @param light A smart pointer to the scene directional light.
			 * @return void
			 */
			void add (Scene & scene, const std::shared_ptr< Component::DirectionalLight > & light) noexcept;

			/**
			 * @brief Adds a point light to the light set for the scene.
			 * @param scene A reference to a scene.
			 * @param light A smart pointer to the scene point light.
			 * @return void
			 */
			void add (Scene & scene, const std::shared_ptr< Component::PointLight > & light) noexcept;

			/**
			 * @brief Adds a spotlight to the light set for the scene.
			 * @param scene A reference to a scene.
			 * @param light A smart pointer to the scene spotlight.
			 * @return void
			 */
			void add (Scene & scene, const std::shared_ptr< Component::SpotLight > & light) noexcept;

			/**
			 * @brief Removes a directional light from the light set of the scene.
			 * @param scene A reference to a scene.
			 * @param light A smart pointer to the scene directional light.
			 * @return void
			 */
			void remove (Scene & scene, const std::shared_ptr< Component::DirectionalLight > & light) noexcept;

			/**
			 * @brief Removes a point light from the light set of the scene.
			 * @param scene A reference to a scene.
			 * @param light A smart pointer to the scene point light.
			 * @return void
			 */
			void remove (Scene & scene, const std::shared_ptr< Component::PointLight > & light) noexcept;

			/**
			 * @brief Removes a spotlight from the light set of the scene.
			 * @param scene A reference to a scene.
			 * @param light A smart pointer to the scene spotlight.
			 * @return void
			 */
			void remove (Scene & scene, const std::shared_ptr< Component::SpotLight > & light) noexcept;

			/**
			 * @brief Returns the light emitter list.
			 * @return const std::set< std::shared_ptr< Component::DirectionalLight > > &
			 */
			[[nodiscard]]
			const std::set< std::shared_ptr< Component::AbstractLightEmitter > > &
			lights () const noexcept
			{
				return m_lights;
			}

			/**
			 * @brief Returns the directional light list.
			 * @return const std::set< std::shared_ptr< Component::DirectionalLight > > &
			 */
			[[nodiscard]]
			const std::set< std::shared_ptr< Component::DirectionalLight > > &
			directionalLights () const noexcept
			{
				return m_directionalLights;
			}

			/**
			 * @brief Returns the spotlight list.
			 * @return const std::set< std::shared_ptr< Component::PointLight > > &
			 */
			[[nodiscard]]
			const std::set< std::shared_ptr< Component::PointLight > > &
			pointLights () const noexcept
			{
				return m_pointLights;
			}

			/**
			 * @brief Returns the point light list.
			 * @return const std::set< std::shared_ptr< Component::SpotLight > > &
			 */
			[[nodiscard]]
			const std::set< std::shared_ptr< Component::SpotLight > > &
			spotLights () const noexcept
			{
				return m_spotLights;
			}

			/**
			 * @brief Returns the percentage of each light color to use for ambient light calculation.
			 * @return float
			 */
			[[nodiscard]]
			float
			lightPercentToAmbient () const noexcept
			{
				return m_lightPercentToAmbient;
			}

			/**
			 * @brief Returns whether we check the distance between the light radius and the camera to add or not the ambient component.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			useLightDistance () const noexcept
			{
				return m_useLightDistance;
			}

			/**
			 * @brief Computes the ambient light fraction to add to global ambient light.
			 * @tparam vectorData_t The data type to use for Vector. Default float.
			 * @param light The targeted light.
			 * @return Libs::Math::Vector< 4, vectorData_t >
			 */
			template< typename vectorData_t = float >
			Libs::Math::Vector< 4, vectorData_t >
			getLightColorFraction (const std::shared_ptr< Component::AbstractLightEmitter > & light) const noexcept
			{
				return light->color().toVector4<vectorData_t>() * this->lightPercentToAmbient();
			}

			/**
			 * @brief Removes all lights.
			 * @return void
			 */
			void removeAllLights () noexcept;

			/**
			 * @return Returns or creates the default static lighting.
			 * @return Saphir::StaticLighting &
			 */
			Saphir::StaticLighting & getOrCreateDefaultStaticLighting () noexcept;

			/**
			 * @brief Returns or creates a static lighting.
			 * @param name A reference to a string.
			 * @return Saphir::StaticLighting &
			 */
			Saphir::StaticLighting & getOrCreateStaticLighting (const std::string & name) noexcept;

			/**
			 * @brief Returns the pointer to a static lighting.
			 * @warning Can be nullptr!
			 * @param name A reference to a string. Default, none.
			 * @return const Saphir::StaticLighting *
			 */
			[[nodiscard]]
			const Saphir::StaticLighting * getStaticLightingPointer (const std::string & name = DefaultStaticLightingName) const noexcept;

			/**
			 * @brief Updates all light UBO.
			 * @return bool
			 */
			[[nodiscard]]
			bool updateVideoMemory () const noexcept;

			/**
			 * @brief Returns the descriptor set layout for a light (UBO only, no shadow map).
			 * @param layoutManager A reference to the layout manager.
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			static std::shared_ptr< Vulkan::DescriptorSetLayout > getDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept;

			/**
			 * @brief Returns the descriptor set layout for a light with shadow map (UBO + shadow sampler).
			 * @param layoutManager A reference to the layout manager.
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			static std::shared_ptr< Vulkan::DescriptorSetLayout > getDescriptorSetLayoutWithShadow (Vulkan::LayoutManager & layoutManager) noexcept;

		private:

			/**
			 * @brief Creates the descriptor set for a light within the shared uniform buffer object.
			 * @param renderer A reference to the graphics renderer.
			 * @param uniformBufferObject A reference to the uniform buffer object.
			 * @return std::unique_ptr< Vulkan::DescriptorSet >
			 */
			[[nodiscard]]
			static std::unique_ptr< Vulkan::DescriptorSet > createDescriptorSet (Graphics::Renderer & renderer, const Vulkan::UniformBufferObject & uniformBufferObject) noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const LightSet & obj);

			std::shared_ptr< Graphics::SharedUniformBuffer > m_directionalLightBuffer;
			std::shared_ptr< Graphics::SharedUniformBuffer > m_pointLightBuffer;
			std::shared_ptr< Graphics::SharedUniformBuffer > m_spotLightBuffer;
			std::set< std::shared_ptr< Component::AbstractLightEmitter > > m_lights;
			std::set< std::shared_ptr< Component::DirectionalLight > > m_directionalLights;
			std::set< std::shared_ptr< Component::PointLight > > m_pointLights;
			std::set< std::shared_ptr< Component::SpotLight > > m_spotLights;
			std::map< std::string, Saphir::StaticLighting > m_staticLighting;
			Libs::PixelFactory::Color< float > m_ambientLightColor{Libs::PixelFactory::Black};
			float m_ambientLightIntensity{DefaultAmbientLightIntensity};
			float m_lightPercentToAmbient{DefaultLightPercentToAmbient};
			mutable std::mutex m_lightsAccess;
			bool m_initialized{false};
			bool m_enabled{false};
			bool m_useStaticLighting{false};
			bool m_createAmbientFromLights{false};
			bool m_useLightDistance{false};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const LightSet & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
