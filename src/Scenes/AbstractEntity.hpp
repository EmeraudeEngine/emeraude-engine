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
#include "Physics/BodyPhysicalProperties.hpp"
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

	/* Forward declaration for ComponentBuilder. */
	class AbstractEntity;

	/**
	 * @brief Builder for creating components with a fluent API.
	 * @tparam component_t The type of component to build.
	 */
	template< typename component_t >
	class ComponentBuilder final
	{
		public:

			/**
			 * @brief Constructs a component builder.
			 * @param entity A reference to the entity that will own the component.
			 * @param componentName The name of the component.
			 */
			ComponentBuilder (AbstractEntity & entity, std::string componentName) noexcept
				: m_entity{entity},
				m_componentName{std::move(componentName)}
			{

			}

			/**
			 * @brief Sets up the component with a custom function.
			 * @tparam function_t The type of setup function. Signature: void (component_t &)
			 * @param setupFunction The function to execute after component construction.
			 * @return ComponentBuilder &
			 */
			template< typename function_t >
			ComponentBuilder &
			setup (function_t && setupFunction) noexcept requires (std::is_invocable_v< function_t, component_t & >)
			{
				m_setupFunction = std::forward< function_t >(setupFunction);

				return *this;
			}

			/**
			 * @brief Marks the component as a primary device (for cameras and microphones).
			 * @return ComponentBuilder &
			 */
			ComponentBuilder &
			asPrimary () noexcept
			{
				m_isPrimaryDevice = true;

				return *this;
			}

			/**
			 * @brief Builds and adds the component to the entity.
			 * @tparam ctor_args The types of constructor arguments.
			 * @param args Constructor arguments for the component (forwarded after componentName and entity).
			 * @return std::shared_ptr< component_t >
			 */
			template< typename... ctor_args >
			std::shared_ptr< component_t >
			build (ctor_args &&... args) noexcept requires (std::is_base_of_v< Component::Abstract, component_t >);

		private:

			AbstractEntity & m_entity;
			std::string m_componentName;
			std::function< void(component_t &) > m_setupFunction{nullptr};
			bool m_isPrimaryDevice{false};
	};

	/**
	 * @brief Defines the base of an entity in the 3D world composed with components.
	 * @note [THREAD-SAFETY] Access to m_components is thread-safe (protected by m_componentsMutex).
	 *       Other members are NOT thread-safe. Each entity instance should be accessed by only
	 *       one thread at a time for operations not involving m_components. The caller is responsible
	 *       for ensuring thread-safety if entities are shared across threads.
	 * @note [OBS][SHARED-OBSERVER][SHARED-OBSERVABLE]
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
				VisualCreated,
				VisualComponentDestroyed,
				MultipleVisualsCreated,
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

			/** @brief Maximum number of components per entity. */
			static constexpr size_t MaxComponentCount{8};

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

			/** @copydoc EmEn::Scenes::LocatableInterface::setCollisionDetectionModel(Scenes::CollisionDetectionModel) */
			void
			setCollisionDetectionModel (CollisionDetectionModel model) noexcept override
			{
				m_collisionDetectionModel = model;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::collisionDetectionModel() const */
			[[nodiscard]]
			CollisionDetectionModel
			collisionDetectionModel () const noexcept override
			{
				return m_collisionDetectionModel;
			}

			/**
			 * @brief Returns the parent scene where the entity live.
			 * @return const Scene &
			 */
			[[nodiscard]]
			const Scene &
			parentScene () const noexcept
			{
				return m_scene;
			}

			/**
			 * @brief Returns the physical properties.
			 * @return const Physics::BodyPhysicalProperties &
			 */
			const Physics::BodyPhysicalProperties &
			bodyPhysicalProperties () const noexcept
			{
				return m_bodyPhysicalProperties;
			}

			/**
			 * @brief Returns the writable physical properties.
			 * @return Physics::BodyPhysicalProperties &
			 */
			Physics::BodyPhysicalProperties &
			bodyPhysicalProperties () noexcept
			{
				return m_bodyPhysicalProperties;
			}

			/**
			 * @brief Returns whether the entity have component.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasComponent () const noexcept
			{
				std::lock_guard< std::mutex > lock(m_componentsMutex);
				return !m_components.empty();
			}

			/**
			 * @brief Returns whether a named component exists in the entity.
			 * @param name The name of the component.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			containsComponent (const std::string & name) const noexcept
			{
				std::lock_guard< std::mutex > lock(m_componentsMutex);
				return std::ranges::any_of(m_components, [&name] (const auto & component) {
					return component->name() == name;
				});
			}

			/**
			 * @brief Returns the smart-pointer on an abstract component smart-pointer.
			 * @param name The name of the component.
			 * @return bool
			 */
			[[nodiscard]]
			std::shared_ptr< Component::Abstract > getComponent (const std::string & name) noexcept;

			/**
			 * @brief Gets a component by name with automatic type casting.
			 * @tparam component_t The expected type of the component.
			 * @param name The name of the component.
			 * @return std::shared_ptr< component_t > The component cast to the specified type, or nullptr if not found or wrong type.
			 */
			template< typename component_t >
			std::shared_ptr< component_t >
			getComponent (const std::string & name) noexcept requires (std::is_base_of_v< Component::Abstract, component_t >)
			{
				std::lock_guard< std::mutex > lock(m_componentsMutex);
				for ( const auto & component : m_components )
				{
					if ( component->name() == name )
					{
						return std::dynamic_pointer_cast< component_t >(component);
					}
				}

				return nullptr;
			}

			/**
			 * @brief Gets all components of a specific type.
			 * @tparam component_t The type of components to retrieve.
			 * @return Libs::StaticVector< std::shared_ptr< component_t >, MaxComponentCount > Vector of all components matching the type.
			 */
			template< typename component_t >
			Libs::StaticVector< std::shared_ptr< component_t >, MaxComponentCount >
			getComponentsOfType () noexcept requires (std::is_base_of_v< Component::Abstract, component_t >)
			{
				std::lock_guard< std::mutex > lock(m_componentsMutex);
				Libs::StaticVector< std::shared_ptr< component_t >, MaxComponentCount > result;

				for ( const auto & component : m_components )
				{
					if ( auto casted = std::dynamic_pointer_cast< component_t >(component) )
					{
						result.push_back(casted);
					}
				}

				return result;
			}

			/**
			 * @brief Executes a function per component with thread-safe access.
			 * @tparam function_t The type of function. Signature: void (Component::Abstract &)
			 * @param processComponent The function to execute.
			 * @return void
			 */
			template< typename function_t >
			void
			forEachComponent (function_t && processComponent) noexcept requires (std::is_invocable_v< function_t, Component::Abstract & >)
			{
				std::lock_guard< std::mutex > lock(m_componentsMutex);
				for ( const auto & component : m_components )
				{
					processComponent(*component.get());
				}
			}

			/**
			 * @brief Executes a function per component with thread-safe access.
			 * @tparam function_t The type of function. Signature: void (const Component::Abstract &)
			 * @param processComponent The function to execute.
			 * @return void
			 */
			template< typename function_t >
			void
			forEachComponent (function_t && processComponent) const noexcept requires (std::is_invocable_v< function_t, const Component::Abstract & >)
			{
				std::lock_guard< std::mutex > lock(m_componentsMutex);
				for ( const auto & component : m_components )
				{
					processComponent(*component.get());
				}
			}

			/**
			 * @brief Removes a component by his name from this entity. Returns 'true' whether the component existed.
			 * @param name The name of the component.
			 * @return bool
			 */
			bool removeComponent (const std::string & name) noexcept;

			/**
			 * @brief Removes all components.
			 * @return void
			 */
			void clearComponents () noexcept;

			/**
			 * @brief Creates a component builder for fluent API construction.
			 * @tparam component_t The type of component to build.
			 * @param componentName The name of the component.
			 * @return ComponentBuilder< component_t >
			 */
			template< typename component_t >
			ComponentBuilder< component_t >
			componentBuilder (const std::string & componentName) noexcept
			{
				return ComponentBuilder< component_t >(*this, componentName);
			}

			/**
			 * @brief This will override the computation of bounding primitives.
			 * @param box A reference to a box.
			 * @param sphere A reference to a sphere.
			 * @return void
			 */
			void overrideBoundingPrimitives (const Libs::Math::Space3D::AACuboid< float > & box, const Libs::Math::Space3D::Sphere< float > & sphere) noexcept;

			/**
			 * @brief Enables a visual debug for this entity.
			 * @param resourceManager A reference to the resource manager.
			 * @param type The type of visual debug.
			 * @return void
			 */
			void enableVisualDebug (Resources::Manager & resourceManager, VisualDebugType type) noexcept;

			/**
			 * @brief Disable a visual debug for this entity.
			 * @param type The type of visual debug.
			 * @return void
			 */
			void disableVisualDebug (VisualDebugType type) noexcept;

			/**
			 * @brief Toggle the visibility of a debug and returns the current state.
			 * @param resourceManager A reference to the resource manager.
			 * @param type The type of visual debug.
			 * @return bool
			 */
			bool toggleVisualDebug (Resources::Manager & resourceManager, VisualDebugType type) noexcept;

			/**
			 * @brief Returns whether a visual debug is displayed.
			 * @param type The type of visual debug.
			 * @return bool
			 */
			[[nodiscard]]
			bool isVisualDebugEnabled (VisualDebugType type) const noexcept;

			/**
			 * @brief Updates components logics from the engine cycle and returns if the entity has moved in the scene.
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
			hasBodyPhysicalProperties () const noexcept
			{
				return this->isFlagEnabled(HasBodyPhysicalProperties);
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

			/**
			 * @brief Copies local data for a stable render.
			 * @note This must be done at the end of the logic loop.
			 * @param writeStateIndex The render state-free index to write data.
			 * @return void
			 */
			virtual void publishStateForRendering (uint32_t writeStateIndex) noexcept = 0;

			/**
			 * @brief Returns the world coordinates of the entity for rendering.
			 * @param readStateIndex The render state-valid index to read data.
			 * @return void const Libs::Math::CartesianFrame< float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::CartesianFrame< float > & getWorldCoordinatesStateForRendering (uint32_t readStateIndex) const noexcept = 0;

		protected:

			/* Friend declarations */
			template< typename component_t >
			friend class ComponentBuilder;

			/* Flag names */
			static constexpr auto BoundingPrimitivesOverridden{0UL};
			static constexpr auto IsDeflector{1UL};
			static constexpr auto IsRenderable{2UL};
			static constexpr auto HasBodyPhysicalProperties{3UL};
			static constexpr auto IsSimulationPaused{4UL};
			static constexpr auto IsCollisionDisabled{5UL};
			static constexpr auto NextFlag{6UL};

			/**
			 * @brief Constructs an abstract entity.
			 * @param scene A reference to the scene this entity belongs to.
			 * @param entityName A string [std::move].
			 * @param sceneTimepointMS The scene current timepoint in milliseconds.
			 */
			AbstractEntity (const Scene & scene, std::string entityName, uint32_t sceneTimepointMS) noexcept
				: NameableTrait{std::move(entityName)},
				m_scene{scene},
				m_birthTime{sceneTimepointMS}
			{

			}

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
			setBodyPhysicalPropertiesState (bool state) noexcept
			{
				this->setFlag(HasBodyPhysicalProperties, state);
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
			 * @brief Links a component to the entity.
			 * @param component A reference to the component smart pointer.
			 * @param isPrimaryDevice Set to true if this is a primary AV device (Camera/Microphone). Default false.
			 * @return bool
			 */
			bool linkComponent (const std::shared_ptr< Component::Abstract > & component, bool isPrimaryDevice = false) noexcept;

			/**
			 * @brief Prepares to unlink a component from the entity.
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
			 * @todo [GENERAL] Should use its own method. Rethink the purpose.
			 * @note If this function return false, the observer will be automatically detached.
			 * @return bool
			 */
			virtual bool onUnhandledNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept = 0;

			/**
			 * @brief Called when the entity has moved.
			 * @todo [PHYSICS] Should use its own method.
			 * @return void
			 */
			virtual void onLocationDataUpdate () noexcept = 0;

			const Scene & m_scene;
			Libs::StaticVector< std::shared_ptr< Component::Abstract >, MaxComponentCount > m_components;
			mutable std::mutex m_componentsMutex;
			Libs::Math::Space3D::AACuboid< float > m_boundingBox;
			Libs::Math::Space3D::Sphere< float > m_boundingSphere;
			Physics::BodyPhysicalProperties m_bodyPhysicalProperties;
			const uint32_t m_birthTime{0};
			size_t m_lastUpdatedMoveCycle{0};
			CollisionDetectionModel m_collisionDetectionModel{CollisionDetectionModel::Sphere};
	};

	template< typename component_t >
	template< typename... ctor_args >
	std::shared_ptr< component_t >
	ComponentBuilder< component_t >::build (ctor_args &&... args) noexcept requires (std::is_base_of_v< Component::Abstract, component_t >)
	{
		/* Create the component. */
		auto component = std::make_shared< component_t >(m_componentName, m_entity, std::forward< ctor_args >(args)...);

		/* Execute setup function if provided. */
		if ( m_setupFunction )
		{
			m_setupFunction(*component);
		}

		/* Link component to entity. The linkComponent() method handles all notifications
		 * in the .cpp file to ensure std::any typeinfo consistency across dynamic library boundaries. */
		if ( !m_entity.linkComponent(component, m_isPrimaryDevice) )
		{
			return nullptr;
		}

		return component;
	}
}
