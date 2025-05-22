/*
 * src/Scenes/AbstractEntity.hpp
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
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>
#include <string>
#include <any>
#include <memory>
#include <mutex>

/* Local inclusions for inheritances. */
#include "LocatableInterface.hpp"
#include "Libs/ObserverTrait.hpp"
#include "Libs/ObservableTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Audio/SoundResource.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/SpriteResource.hpp"
#include "Graphics/Material/BasicResource.hpp"
#include "Physics/PhysicalObjectProperties.hpp"
#include "Component/Camera.hpp"
#include "Component/DirectionalLight.hpp"
#include "Component/DirectionalPushModifier.hpp"
#include "Component/Microphone.hpp"
#include "Component/MultipleVisuals.hpp"
#include "Component/ParticlesEmitter.hpp"
#include "Component/PointLight.hpp"
#include "Component/SoundEmitter.hpp"
#include "Component/SphericalPushModifier.hpp"
#include "Component/SpotLight.hpp"
#include "Component/Visual.hpp"
#include "Component/Weight.hpp"

/* Forward declarations. */
namespace EmEn::Physics
{
	class MovableTrait;
}

namespace EmEn::Scenes
{
	enum class VisualDebugType
	{
		Axis,
		Velocity,
		BoundingBox,
		BoundingSphere,
		Camera,
	};

	/**
	 * @brief Defines the base of an entity in the 3D world composed with components.
	 * @extends EmEn::Libs::FlagArrayTrait Each component has 8 flags, 2 are used by this base class.
	 * @extends EmEn::Libs::NameableTrait An entity is nameable.
	 * @extends EmEn::Scenes::LocatableInterface An entity is insertable to an octree system.
	 * @extends EmEn::Libs::ObserverTrait An entity listens to its components.
	 * @extends EmEn::Libs::ObservableTrait An entity is observable to notify its content modification easily.
	 */
	class AbstractEntity : public Libs::FlagArrayTrait< 8 >, public Libs::NameableTrait, public LocatableInterface, public Libs::ObserverTrait, public Libs::ObservableTrait
	{
		public:

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				/* Main notifications */
				EntityContentModified,
				ComponentCreated,
				ComponentDestroyed,
				ModifierCreated,
				ModifierDestroyed,
				/* Specific component notifications */
				CameraCreated,
				PrimaryCameraCreated,
				CameraDestroyed,
				MicrophoneCreated,
				PrimaryMicrophoneCreated,
				MicrophoneDestroyed,
				DirectionalLightCreated,
				DirectionalLightDestroyed,
				PointLightCreated,
				PointLightDestroyed,
				SpotLightCreated,
				SpotLightDestroyed,
				SoundEmitterCreated,
				SoundEmitterDestroyed,
				VisualComponentCreated,
				VisualComponentDestroyed,
				MultipleVisualsComponentCreated,
				MultipleVisualsComponentDestroyed,
				ParticlesEmitterCreated,
				ParticlesEmitterDestroyed,
				DirectionalPushModifierCreated,
				DirectionalPushModifierDestroyed,
				SphericalPushModifierCreated,
				SphericalPushModifierDestroyed,
				WeightCreated,
				WeightDestroyed,
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractEntity (const AbstractEntity & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractEntity (AbstractEntity && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractEntity &
			 */
			AbstractEntity & operator= (const AbstractEntity & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractEntity &
			 */
			AbstractEntity & operator= (AbstractEntity && copy) noexcept = delete;

			/**
			 * @brief Destructs the abstract entity.
			 */
			~AbstractEntity () override = default;

			/** @copydoc EmEn::Scenes::LocatableInterface::localBoundingBox() const */
			[[nodiscard]]
			const Libs::Math::Space3D::AACuboid< float > &
			localBoundingBox () const noexcept final
			{
				return m_boundingBox;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localBoundingSphere() const */
			[[nodiscard]]
			const Libs::Math::Space3D::Sphere< float > &
			localBoundingSphere () const noexcept final
			{
				return m_boundingSphere;
			}

			/**
			 * @brief Returns the physical properties.
			 * @return const PhysicalObjectProperties &
			 */
			const Physics::PhysicalObjectProperties &
			physicalObjectProperties () const noexcept
			{
				return m_physicalObjectProperties;
			}

			/**
			 * @brief Returns the writable physical properties.
			 * @return PhysicalObjectProperties &
			 */
			Physics::PhysicalObjectProperties &
			physicalObjectProperties () noexcept
			{
				return m_physicalObjectProperties;
			}

			/**
			 * @brief Returns whether a named component exists in the entity.
			 * @param name The name of the component.
			 * @return bool
			 */
			[[nodiscard]]
			bool containsComponent (const std::string & name) const noexcept;

			/**
			 * @brief Returns whether a component is part of this entity.
			 * @param component A reference to the component smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool containsComponent (const std::shared_ptr< Component::Abstract > & component) const noexcept;

			/**
			 * @brief Returns the component list of this entity.
			 * @return const std::map< std::string, std::shared_ptr< Component::Abstract > > &
			 */
			[[nodiscard]]
			const std::map< std::string, std::shared_ptr< Component::Abstract > > &
			components () const noexcept
			{
				return m_components;
			}

			/**
			 * @brief Returns the component list of this entity.
			 * @return std::map< std::string, std::shared_ptr< Component::Abstract > > &
			 */
			[[nodiscard]]
			std::map< std::string, std::shared_ptr< Component::Abstract > > &
			components () noexcept
			{
				return m_components;
			}

			/**
			 * @brief Returns a possible named component from the entity.
			 * @param name The name of the component.
			 * @return std::shared_ptr< Component::Abstract >
			 */
			[[nodiscard]]
			std::shared_ptr< Component::Abstract > getComponent (const std::string & name) const noexcept;

			/**
			 * @brief Removes a component by his name from this entity. Returns 'true' whether the component existed.
			 * @param name The name of the component.
			 * @return bool
			 */
			bool removeComponent (const std::string & name) noexcept;

			/**
			 * @brief Removes a component by its pointer from this entity. Returns 'true' whether the component existed.
			 * @param component The name of the component.
			 * @return bool
			 */
			bool removeComponent (const std::shared_ptr< Component::Abstract > & component) noexcept;

			/**
			 * @brief Removes all components.
			 * @return void
			 */
			void clearComponents () noexcept;

			/**
			 * @brief Reads something on each component of the entity.
			 * @param lambda A reference to a function using the signature "bool method (const Component::Abstract & component)".
			 * @return void
			 */
			void forEachComponent (const std::function< bool (const Component::Abstract & component) > & lambda) const noexcept;

			/**
			 * @brief Reads something on each component of the entity.
			 * @param lambda A reference to a function using the signature "bool method (const Component::Abstract & component, int index)".
			 * @return void
			 */
			void forEachComponent (const std::function< bool (const Component::Abstract & component, size_t index) > & lambda) const noexcept;

			/**
			 * @brief Modifies something on each component of the entity.
			 * @param lambda A reference to a function using the signature "bool method (const Component::Abstract & component)".
			 * @return void
			 */
			void forEachComponent (const std::function< bool (Component::Abstract & component) > & lambda) noexcept;

			/**
			 * @brief Modifies something on each component of the entity.
			 * @param lambda A reference to a function using the signature "bool method (const Component::Abstract & component, int index)".
			 * @return void
			 */
			void forEachComponent (const std::function< bool (Component::Abstract & component, size_t index) > & lambda) noexcept;

			/**
			 * @brief Creates a camera.
			 * @param perspective Set up a perspective projection to the camera. Default true.
			 * @param primaryDevice Declare this device as a primary one. Default false.
			 * @param componentName The name of the component. Default "Camera".
			 * @return std::shared_ptr< Component::Camera >
			 */
			std::shared_ptr< Component::Camera > newCamera (bool perspective = true, bool primaryDevice = false, const std::string & componentName = "Camera") noexcept;

			/**
			 * @brief Creates a camera as a primary device, using a perspective projection [SHORTCUT].
			 * @param componentName The name of the component. Default "PrimaryPerspectiveCamera".
			 * @return std::shared_ptr< Component::Camera >
			 */
			std::shared_ptr< Component::Camera >
			newPrimaryPerspectiveCamera (const std::string & componentName = "PrimaryPerspectiveCamera") noexcept
			{
				return this->newCamera(true, true, componentName);
			}

			/**
			 * @brief Creates a camera as a primary device, using an orthographic projection [SHORTCUT].
			 * @param componentName The name of the component. Default "PrimaryOrthographicCamera".
			 * @return std::shared_ptr< Component::Camera >
			 */
			std::shared_ptr< Component::Camera >
			newPrimaryOrthographicCamera (const std::string & componentName = "PrimaryOrthographicCamera") noexcept
			{
				return this->newCamera(false, true, componentName);
			}

			/**
			 * @brief Creates a camera using a perspective projection [SHORTCUT].
			 * @param componentName The name of the component. Default "PerspectiveCamera".
			 * @return std::shared_ptr< Component::Camera >
			 */
			std::shared_ptr< Component::Camera >
			newPerspectiveCamera (const std::string & componentName = "PerspectiveCamera") noexcept
			{
				return this->newCamera(true, false, componentName);
			}

			/**
			 * @brief Creates a camera using an orthographic projection [SHORTCUT].
			 * @param componentName The name of the component. Default "OrthographicCamera".
			 * @return std::shared_ptr< Component::Camera >
			 */
			std::shared_ptr< Component::Camera >
			newOrthographicCamera (const std::string & componentName = "OrthographicCamera") noexcept
			{
				return this->newCamera(false, false, componentName);
			}

			/**
			 * @brief Creates a microphone.
			 * @param componentName The name of the component. Default "Microphone".
			 * @param primaryDevice Set this device as primary one. Default false.
			 * @return std::shared_ptr< Component::Microphone >
			 */
			std::shared_ptr< Component::Microphone > newMicrophone (bool primaryDevice = false, const std::string & componentName = "Microphone") noexcept;

			/**
			 * @brief Creates a microphone as a primary device [SHORTCUT].
			 * @param componentName The name of the component. Default "PrimaryMicrophone".
			 * @return std::shared_ptr< Component::Microphone >
			 */
			std::shared_ptr< Component::Camera >
			newPrimaryMicrophone (const std::string & componentName = "PrimaryMicrophone") noexcept
			{
				return this->newCamera(true, true, componentName);
			}

			/**
			 * @brief Creates a directional light emitter.
			 * @param shadowMapResolution The shadow map resolution. Default, no shadow map.
			 * @param componentName The name of the component. Default "DirectionalLight".
			 * @return std::shared_ptr< Component::DirectionalLight >
			 */
			std::shared_ptr< Component::DirectionalLight > newDirectionalLight (uint32_t shadowMapResolution = 0, const std::string & componentName = "DirectionalLight") noexcept;

			/**
			 * @brief Creates a point light emitter.
			 * @param shadowMapResolution The shadow map resolution. Default, no shadow map.
			 * @param componentName The name of the component. Default "PointLight".
			 * @return std::shared_ptr< Component::PointLight >
			 */
			std::shared_ptr< Component::PointLight > newPointLight (uint32_t shadowMapResolution = 0, const std::string & componentName = "PointLight") noexcept;

			/**
			 * @brief Creates a spotlight emitter.
			 * @param shadowMapResolution The shadow map resolution. Default, no shadow map.
			 * @param componentName The name of the component. Default "SpotLight".
			 * @return std::shared_ptr< Component::SpotLight >
			 */
			std::shared_ptr< Component::SpotLight > newSpotLight (uint32_t shadowMapResolution = 0, const std::string & componentName = "SpotLight") noexcept;

			/**
			 * @brief Creates a sound emitter.
			 * @param componentName The name of the component. Default "SoundEmitter".
			 * @return std::shared_ptr< Component::SoundEmitter >
			 */
			std::shared_ptr< Component::SoundEmitter > newSoundEmitter (const std::string & componentName = "SoundEmitter") noexcept;

			/**
			 * @brief Creates a sound emitter and provide a sound.
			 * @param resource A reference to a sound resource smart pointer to play.
			 * @param gain Set the playback volume.
			 * @param loop Set the loop state.
			 * @param componentName The name of the component. Default "SoundEmitter".
			 * @return std::shared_ptr< Component::SoundEmitter >
			 */
			std::shared_ptr< Component::SoundEmitter > newSoundEmitter (const std::shared_ptr< Audio::SoundResource > & resource, float gain, bool loop, const std::string & componentName = "SoundEmitter") noexcept;

			/**
			 * @brief Creates a visual instance.
			 * @param resource A reference to a renderable interface smart pointer.
			 * @param enablePhysicalProperties Enable physicals properties on the new component. Default true.
			 * @param enableLighting Enable the lighting on the visual. Default true.
			 * @param componentName A reference to a string for the name of the component. Default "Visual".
			 * @return std::shared_ptr< Component::Visual >
			 */
			std::shared_ptr< Component::Visual > newVisual (const std::shared_ptr< Graphics::Renderable::Interface > & resource, bool enablePhysicalProperties = true, bool enableLighting = true, const std::string & componentName = "Visual") noexcept;

			/**
			 * @brief Creates a visual instances.
			 * @note Version for multiple underlying renderable object instances.
			 * @param resource A reference to a renderable interface smart pointer.
			 * @param coordinates An array of coordinates to place instances.
			 * @param enablePhysicalProperties Enable physicals properties on the new component. Default true.
			 * @param enableLighting Enable the lighting on the visual. Default true.
			 * @param componentName The name of the component. Default "MultipleVisuals".
			 * @return std::shared_ptr< Component::MultipleVisuals >
			 */
			std::shared_ptr< Component::MultipleVisuals > newVisual (const std::shared_ptr< Graphics::Renderable::Interface > & resource, const std::vector< Libs::Math::CartesianFrame< float > > & coordinates, bool enablePhysicalProperties = true, bool enableLighting = true, const std::string & componentName = "MultipleVisuals") noexcept;

			/**
			 * @brief Creates a particles emitter using a sprite resource.
			 * @param resource a reference to a sprite resource smart pointer.
			 * @param maxParticleCount The limit of particles allowed to be generated at once.
			 * @param componentName The name of the component. Default "ParticlesEmitter".
			 * @return std::shared_ptr< Component::ParticlesEmitter >
			 */
			std::shared_ptr< Component::ParticlesEmitter > newParticlesEmitter (const std::shared_ptr< Graphics::Renderable::SpriteResource > & resource, uint32_t maxParticleCount, const std::string & componentName = "ParticlesEmitter") noexcept;

			/**
			 * @brief Creates a particles emitter using a mesh resource.
			 * @param resource a reference to a mesh resource smart pointer.
			 * @param maxParticleCount The limit of particles allowed to be generated at once.
			 * @param componentName The name of the component. Default "ParticlesEmitter".
			 * @return std::shared_ptr< Component::ParticlesEmitter >
			 */
			std::shared_ptr< Component::ParticlesEmitter > newParticlesEmitter (const std::shared_ptr< Graphics::Renderable::MeshResource > & resource, uint32_t maxParticleCount, const std::string & componentName = "ParticlesEmitter") noexcept;

			/**
			 * @brief Creates a directional push modifier.
			 * @param componentName The name of the component. Default "DirectionalPushModifier".
			 * @return std::shared_ptr< Component::DirectionalPushModifier >
			 */
			std::shared_ptr< Component::DirectionalPushModifier > newDirectionalPushModifier (const std::string & componentName = "DirectionalPushModifier") noexcept;

			/**
			 * @brief Creates a spherical push modifier.
			 * @param componentName The name of the component. Default "SphericalPushModifier".
			 * @return std::shared_ptr< Component::SphericalPushModifier >
			 */
			std::shared_ptr< Component::SphericalPushModifier > newSphericalPushModifier (const std::string & componentName = "SphericalPushModifier") noexcept;

			/**
			 * @brief Creates a weight with customized properties.
			 * @note This component will automatically enable the physical properties.
			 * @param initialProperties A reference to a physical object properties.
			 * @param componentName The name of the component. Default "Weight".
			 * @return std::shared_ptr< Component::Weight >
			 */
			std::shared_ptr< Component::Weight > newWeight (const Physics::PhysicalObjectProperties & initialProperties, const std::string & componentName = "Weight") noexcept;

			/**
			 * @brief This will override the computation of bounding primitives.
			 * @param box A reference to a box.
			 * @param sphere A reference to a sphere.
			 * @return void
			 */
			void overrideBoundingPrimitives (const Libs::Math::Space3D::AACuboid< float > & box, const Libs::Math::Space3D::Sphere< float > & sphere) noexcept;

			/**
			 * @brief Enables a visual debug for this entity.
			 * @param type The type of visual debug.
			 * @return void
			 */
			void enableVisualDebug (VisualDebugType type) noexcept;

			/**
			 * @brief Disable a visual debug for this entity.
			 * @param type The type of visual debug.
			 * @return void
			 */
			void disableVisualDebug (VisualDebugType type) noexcept;

			/**
			 * @brief Toggle the visibility of a debug and returns the current state.
			 * @param type The type of visual debug.
			 * @return bool
			 */
			bool toggleVisualDebug (VisualDebugType type) noexcept;

			/**
			 * @brief Returns whether a visual debug is displayed.
			 * @param type The type of visual debug.
			 * @return bool
			 */
			[[nodiscard]]
			bool isVisualDebugEnabled (VisualDebugType type) const noexcept;

			/**
			 * @brief Updates components logics from engine cycle and returns if the entity has moved in the scene.
			 * @param scene A reference to the scene.
			 * @param engineCycle The current engine cycle number.
			 * @return bool
			 */
			bool processLogics (const Scene & scene, size_t engineCycle) noexcept;

			/**
			 * @brief Returns if the entity has moved since the last cycle.
			 * @param engineCycle The current engine cycle number.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasMoved (size_t engineCycle) const noexcept
			{
				return m_lastUpdatedMoveCycle >= engineCycle - 1;
			}

			/**
			 * @brief Returns when the entity was born in the time scene (ms).
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			birthTime () const noexcept
			{
				return m_birthTime;
			}

			/**
			* @brief Returns whether the entity is collide-able.
			* @note This means the entity has bounding shapes.
			* @return bool
			*/
			[[nodiscard]]
			bool
			isDeflector () const noexcept
			{
				return this->isFlagEnabled(IsDeflector);
			}

			/**
			 * @brief Returns whether the entity is renderable.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRenderable () const noexcept
			{
				return this->isFlagEnabled(IsRenderable);
			}

			/**
			 * @brief Returns whether the entity has physical properties.
			 * @todo [PHYSICS] Should question present var to determine if there are valid properties. No more flag.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasPhysicalObjectProperties () const noexcept
			{
				return this->isFlagEnabled(HasPhysicalObjectProperties);
			}

			/**
			 * @brief Pauses the simulation on this entity. The object will not receive gravity and drag force.
			 * @note The simulation will restart automatically after adding a custom force.
			 * @param state The state.
			 */
			void
			pauseSimulation (bool state) noexcept
			{
				this->setFlag(IsSimulationPaused, state);
			}

			/**
			 * @brief Returns whether the physics simulation is paused on this entity.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSimulationPaused () const noexcept
			{
				return this->isFlagEnabled(IsSimulationPaused);
			}

			/**
			 * @brief Disables the collision tests.
			 * @return void
			 */
			void
			disableCollision () noexcept
			{
				this->enableFlag(IsCollisionDisabled);
			}

			/**
			 * @brief Returns whether the collision test is disabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCollisionDisabled () const noexcept
			{
				return this->isFlagEnabled(IsCollisionDisabled);
			}

			/**
			 * @brief Returns whether the entity is able to move.
			 * @note If true, this should give access to a MovableTrait with AbstractEntity::getMovableTrait().
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool hasMovableAbility () const noexcept = 0;

			/**
			 * @brief Returns whether the entity is moving.
			 * @return Physics::MovableTrait *
			 */
			[[nodiscard]]
			virtual Physics::MovableTrait * getMovableTrait () noexcept = 0;

			/**
			 * @brief Returns whether the entity is moving.
			 * @return Physics::MovableTrait *
			 */
			[[nodiscard]]
			virtual const Physics::MovableTrait * getMovableTrait () const noexcept = 0;

			/**
			 * @brief Returns whether the entity is currently moving. This denotes the entity has a non-null velocity.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isMoving () const noexcept = 0;

		protected:

			/* Flag names */
			static constexpr auto BoundingPrimitivesOverridden{0UL};
			static constexpr auto IsDeflector{1UL};
			static constexpr auto IsRenderable{2UL};
			static constexpr auto HasPhysicalObjectProperties{3UL};
			static constexpr auto SphereCollisionEnabled{4UL};
			static constexpr auto IsSimulationPaused{5UL};
			static constexpr auto IsCollisionDisabled{6UL};
			static constexpr auto NextFlag{7UL};

			/**
			 * @brief Constructs an abstract entity.
			 * @param name The name of the entity.
			 * @param sceneTimeMS The scene current time in milliseconds.
			 */
			AbstractEntity (const std::string & name, uint32_t sceneTimeMS) noexcept;

			/**
			 * @brief Updates components when the holder is moving.
			 * @param worldCoordinates A reference to the container world coordinates.
			 * @return void
			 */
			void onContainerMove (const Libs::Math::CartesianFrame< float > & worldCoordinates) noexcept;

			/**
			 * @brief Sets whether the entity is renderable.
			 * @param state The state of rendering ability.
			 * @return void
			 */
			void
			setRenderingAbilityState (bool state) noexcept
			{
				this->setFlag(IsRenderable, state);
			}

			/**
			 * @brief Sets whether the entity has physical properties.
			 * @param state The state of physical ability.
			 * @return void
			 */
			void
			setPhysicalObjectPropertiesState (bool state) noexcept
			{
				this->setFlag(HasPhysicalObjectProperties, state);
			}

			/**
			 * @brief Updates components logics from engine cycle for child class and returns if the entity has moved in the scene.
			 * @todo [PHYSICS] Should use is own method.
			 * @param scene A reference to the scene.
			 * @return bool
			 */
			virtual bool onProcessLogics (const Scene & scene) noexcept = 0;

			/**
			 * @brief Events when the entity content is modifier.
			 * @return void
			 */
			virtual void onContentModified () noexcept = 0;

		private:

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept final;

			/**
			 * @brief Updates the entity properties when content is modified.
			 * @note This includes the physical properties, bounding primitives and states.
			 * @return void
			 */
			void updateEntityProperties () noexcept;

			/**
			 * @brief Before creating a new component, this function is called to know if the component name is available.
			 * @param name The name of the component.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkComponentNameAvailability (const std::string & name) const noexcept;

			/**
			 * @brief Adds a component in the entity.
			 * @param name The name of the new component.
			 * @param component A reference to the component smart pointer.
			 * @return bool
			 */
			bool addComponent (const std::string & name, const std::shared_ptr< Component::Abstract > & component) noexcept;

			/**
			 * @brief Prepares to remove a component from the entity.
			 * @param component A reference to a component smart pointer.
			 * @return void
			 */
			void unlinkComponent (const std::shared_ptr< Component::Abstract > & component) noexcept;

			/**
			 * @brief Updates enabled visual debug on physical properties changes on the entity.
			 * @return void
			 */
			void updateVisualDebug () noexcept;

			/**
			 * @brief Returns a material for plain visual debug objects.
			 * @todo This should be moved to a general place for all debug objects.
			 * @return std::shared_ptr< Graphics::Material::BasicResource >
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Material::BasicResource > getPlainVisualDebugMaterial (Resources::Manager & resources) noexcept;

			/**
			 * @brief Returns a material for translucent visual debug objects.
			 * @todo This should be moved to a general place for all debug objects.
			 * @return std::shared_ptr< Graphics::Material::BasicResource >
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Material::BasicResource > getTranslucentVisualDebugMaterial (Resources::Manager & resources) noexcept;

			/**
			 * @brief Gets or creates the axis visual debug.
			 * @param resources A reference to the resource manager.
			 * @return std::shared_ptr< Graphics::Renderable::MeshResource
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Renderable::MeshResource > getAxisVisualDebug (Resources::Manager & resources) noexcept;

			/**
			 * @brief Gets or creates the velocity visual debug.
			 * @param resources A reference to the resource manager.
			 * @return std::shared_ptr< Graphics::Renderable::MeshResource
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Renderable::MeshResource > getVelocityVisualDebug (Resources::Manager & resources) noexcept;

			/**
			 * @brief Gets or creates the bounding box visual debug.
			 * @param resources A reference to the resource manager.
			 * @return std::shared_ptr< Graphics::Renderable::MeshResource
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Renderable::MeshResource > getBoundingBoxVisualDebug (Resources::Manager & resources) noexcept;

			/**
			 * @brief Gets or creates the bounding sphere visual debug.
			 * @param resources A reference to the resource manager.
			 * @return std::shared_ptr< Graphics::Renderable::MeshResource
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Renderable::MeshResource > getBoundingSphereVisualDebug (Resources::Manager & resources) noexcept;

			/**
			 * @brief Gets or creates the camera visual debug.
			 * @param resources A reference to the resource manager.
			 * @return std::shared_ptr< Graphics::Renderable::MeshResource
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Renderable::MeshResource > getCameraVisualDebug (Resources::Manager & resources) noexcept;

			/**
			 * @brief Enables the deflector state.
			 * @param state The state of the effect.
			 * @return void
			 */
			void
			setDeflectorState (bool state) noexcept
			{
				this->setFlag(IsDeflector, state);
			}

			/**
			 * @brief Called when the entity does not handle a notification abstract level.
			 * @todo [GENERAL] Should use is own method. Rethink the purpose.
			 * @note If this function return false, the observer will be automatically detached.
			 * @return bool
			 */
			virtual bool onUnhandledNotification (const Libs::ObservableTrait * observable, int notificationCode, const std::any & data) noexcept = 0;

			/**
			 * @brief Called when the entity has moved.
			 * @todo [PHYSICS] Should use its own method.
			 * @return void
			 */
			virtual void onLocationDataUpdate () noexcept = 0;

			std::map< std::string, std::shared_ptr< Component::Abstract > > m_components;
			Libs::Math::Space3D::AACuboid< float > m_boundingBox;
			Libs::Math::Space3D::Sphere< float > m_boundingSphere;
			Physics::PhysicalObjectProperties m_physicalObjectProperties;
			uint32_t m_birthTime{0};
			size_t m_lastUpdatedMoveCycle{0};
			mutable std::mutex m_componentsAccess;
	};
}
