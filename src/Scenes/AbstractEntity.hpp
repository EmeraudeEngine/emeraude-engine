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
#include <string>
#include <string_view>
#include <any>
#include <memory>
#include <mutex>

/* Local inclusions for inheritances. */
#include "Libs/FlagArrayTrait.hpp"
#include "Libs/NameableTrait.hpp"
#include "LocatableInterface.hpp"
#include "Libs/ObserverTrait.hpp"
#include "Libs/ObservableTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/StaticVector.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Component/Abstract.hpp"
#include "Resources/Manager.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Material/BasicResource.hpp"

/* Forward declarations. */
namespace EmEn::Physics
{
	class MovableTrait;
}

namespace EmEn::Scenes
{
	/**
	 * @brief Types of visual debug overlays that can be enabled on entities.
	 *
	 * @version 0.8.35
	 */
	enum class VisualDebugType
	{
		Axis,            ///< Local coordinate system axes (RGB = XYZ).
		Velocity,        ///< Velocity vector visualization for moving entities.
		BoundingBox,     ///< Axis-aligned bounding box (AABB) wireframe.
		BoundingSphere,  ///< Bounding sphere wireframe.
		Camera,          ///< Camera frustum visualization.
	};

	/* Forward declaration for ComponentBuilder. */
	class AbstractEntity;

	/**
	 * @brief Builder for creating components with a fluent API.
	 *
	 * Provides a type-safe, chainable interface for constructing and configuring
	 * components before adding them to an entity. Supports setup callbacks and
	 * marking primary audio-visual devices.
	 *
	 * @tparam component_t The type of component to build. Must inherit from Component::Abstract.
	 *
	 * @see AbstractEntity::componentBuilder()
	 * @version 0.8.35
	 */
	template< typename component_t >
	class ComponentBuilder final
	{
		public:

			/**
			 * @brief Constructs a component builder.
			 *
			 * @param entity A reference to the entity that will own the component.
			 * @param componentName The name of the component (used for lookup and debugging).
			 *
			 * @version 0.8.35
			 */
			ComponentBuilder (AbstractEntity & entity, std::string componentName) noexcept
				: m_entity{entity},
				m_componentName{std::move(componentName)}
			{

			}

			/**
			 * @brief Sets up the component with a custom function.
			 *
			 * The setup function is called after component construction but before
			 * linking to the entity. Use this to configure component properties.
			 *
			 * @tparam function_t The type of setup function. Signature: void (component_t &)
			 * @param setupFunction The function to execute after component construction.
			 * @return ComponentBuilder & Reference to this builder for chaining.
			 *
			 * @version 0.8.35
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
			 *
			 * Primary devices receive special notification codes (PrimaryCameraCreated,
			 * PrimaryMicrophoneCreated) and may be registered as default AV devices.
			 *
			 * @return ComponentBuilder & Reference to this builder for chaining.
			 *
			 * @note Only applies to Component::Camera and Component::Microphone.
			 * @version 0.8.35
			 */
			ComponentBuilder &
			asPrimary () noexcept
			{
				m_isPrimaryDevice = true;

				return *this;
			}

			/**
			 * @brief Builds and adds the component to the entity.
			 *
			 * Creates the component with provided constructor arguments, executes the
			 * setup function if defined, and links the component to the entity. Triggers
			 * appropriate notification codes based on component type.
			 *
			 * @tparam ctor_args The types of constructor arguments.
			 * @param args Constructor arguments for the component (forwarded after componentName and entity).
			 * @return std::shared_ptr< component_t > Shared pointer to the created component, or nullptr if entity is full (MaxComponentCount reached).
			 *
			 * @note This method handles notification dispatch in the .cpp file to ensure std::any typeinfo consistency across dynamic library boundaries.
			 * @version 0.8.35
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
	 *
	 * AbstractEntity is the foundational class for all entities in Emeraude Engine's scene graph
	 * (Node, StaticEntity). It provides component management, physical properties, bounding primitives,
	 * observer pattern integration, and double-buffering for thread-safe rendering.
	 *
	 * Entities follow a composition-over-inheritance pattern: behaviors are added via Components
	 * (Visual, Camera, Light, SoundEmitter, etc.) rather than subclassing.
	 *
	 * @note [THREAD-SAFETY] Component access (m_components) is thread-safe (protected by m_componentsMutex).
	 *       Other members are read-safe, write-unsafe: multiple threads can safely read simultaneously,
	 *       but writes must be externally synchronized. The caller is responsible for ensuring thread-safety
	 *       if entities are modified concurrently.
	 *
	 * @note [DOUBLE-BUFFERING] publishStateForRendering() and getWorldCoordinatesStateForRendering()
	 *       provide thread-safe separation between logic thread (writes) and render thread (reads).
	 *       Logic thread updates entity state, then publishes to render buffer. Render thread reads
	 *       from stable buffer without blocking logic thread.
	 *
	 * @note [OBSERVER PATTERN] Entities observe their components (ObserverTrait) and notify observers
	 *       of content changes (ObservableTrait). This enables automatic registration with Scene subsystems
	 *       (Graphics, Audio, Physics) when components are added/removed.
	 *
	 * @extends EmEn::Libs::FlagArrayTrait< 8 > Provides 8 boolean flags, with 6 used by this base class.
	 * @extends EmEn::Libs::NameableTrait An entity has a unique name for identification and debugging.
	 * @extends EmEn::Scenes::LocatableInterface An entity is insertable to octree spatial partitioning systems.
	 * @extends EmEn::Libs::ObserverTrait An entity observes its components for ComponentContentModified notifications.
	 * @extends EmEn::Libs::ObservableTrait An entity notifies observers (Scene) of component creation/destruction and content changes.
	 *
	 * @see Node, StaticEntity, ComponentBuilder
	 * @see @docs/scene-graph-architecture.md
	 * @version 0.8.35
	 */
	class AbstractEntity : public Libs::FlagArrayTrait< 8 >, public Libs::NameableTrait, public LocatableInterface, public Libs::ObserverTrait, public Libs::ObservableTrait
	{
		public:

			/**
			 * @brief Observable notification codes emitted by AbstractEntity.
			 *
			 * These codes are sent via ObservableTrait::notify() when components are
			 * created or destroyed. Scene observes these notifications to register
			 * components with appropriate subsystems (Graphics, Audio, Physics).
			 *
			 * @note Component-specific codes (CameraCreated, VisualCreated, etc.) carry
			 *       std::shared_ptr< component_t > in the notification data (std::any).
			 * @note Generic codes (ComponentCreated, ComponentDestroyed) carry
			 *       std::shared_ptr< Component::Abstract >.
			 *
			 * @version 0.8.35
			 */
			enum NotificationCode
			{
				/* Main notifications */
				EntityContentModified,        ///< Entity's content has changed (bounding primitives, properties).
				ComponentCreated,             ///< Generic: any component added (carries std::shared_ptr< Component::Abstract >).
				ComponentDestroyed,           ///< Generic: any component removed.
				ModifierCreated,              ///< Generic: any AbstractModifier added.
				ModifierDestroyed,            ///< Generic: any AbstractModifier removed.
				/* Specific component notifications */
				CameraCreated,                ///< Component::Camera added (non-primary).
				PrimaryCameraCreated,         ///< Component::Camera added (marked as primary via asPrimary()).
				CameraDestroyed,              ///< Component::Camera removed.
				MicrophoneCreated,            ///< Component::Microphone added (non-primary).
				PrimaryMicrophoneCreated,     ///< Component::Microphone added (marked as primary via asPrimary()).
				MicrophoneDestroyed,          ///< Component::Microphone removed.
				DirectionalLightCreated,      ///< Component::DirectionalLight added.
				DirectionalLightDestroyed,    ///< Component::DirectionalLight removed.
				PointLightCreated,            ///< Component::PointLight added.
				PointLightDestroyed,          ///< Component::PointLight removed.
				SpotLightCreated,             ///< Component::SpotLight added.
				SpotLightDestroyed,           ///< Component::SpotLight removed.
				SoundEmitterCreated,          ///< Component::SoundEmitter added.
				SoundEmitterDestroyed,        ///< Component::SoundEmitter removed.
				VisualCreated,                ///< Component::Visual added.
				VisualComponentDestroyed,     ///< Component::Visual removed.
				MultipleVisualsCreated,       ///< Component::MultipleVisuals added.
				MultipleVisualsComponentDestroyed, ///< Component::MultipleVisuals removed.
				ParticlesEmitterCreated,      ///< Component::ParticlesEmitter added.
				ParticlesEmitterDestroyed,    ///< Component::ParticlesEmitter removed.
				DirectionalPushModifierCreated,   ///< Component::DirectionalPushModifier added.
				DirectionalPushModifierDestroyed, ///< Component::DirectionalPushModifier removed.
				SphericalPushModifierCreated,     ///< Component::SphericalPushModifier added.
				SphericalPushModifierDestroyed,   ///< Component::SphericalPushModifier removed.
				WeightCreated,                ///< Component::Weight added.
				WeightDestroyed,              ///< Component::Weight removed.
				/* Enumeration boundary. */
				MaxEnum                       ///< Total count of notification codes.
			};

			/**
			 * @brief Maximum number of components per entity.
			 *
			 * This limit ensures fixed-size storage (StaticVector) for performance and
			 * predictable memory usage. Attempting to add components beyond this limit
			 * will fail (ComponentBuilder::build() returns nullptr).
			 *
			 * @version 0.8.35
			 */
			static constexpr size_t MaxComponentCount{8};

			/**
			 * @brief Copy constructor (deleted).
			 *
			 * Entities cannot be copied due to their complex internal state (components,
			 * observer relationships, scene references). Use shared_ptr for shared ownership.
			 *
			 * @param copy A reference to the copied instance.
			 * @version 0.8.35
			 */
			AbstractEntity (const AbstractEntity & copy) noexcept = delete;

			/**
			 * @brief Move constructor (deleted).
			 *
			 * Entities cannot be moved due to observer pattern relationships and scene
			 * ownership semantics. Create new entities instead.
			 *
			 * @param copy A reference to the moved instance.
			 * @version 0.8.35
			 */
			AbstractEntity (AbstractEntity && copy) noexcept = delete;

			/**
			 * @brief Copy assignment (deleted).
			 *
			 * @param copy A reference to the copied instance.
			 * @return AbstractEntity &
			 * @version 0.8.35
			 */
			AbstractEntity & operator= (const AbstractEntity & copy) noexcept = delete;

			/**
			 * @brief Move assignment (deleted).
			 *
			 * @param copy A reference to the moved instance.
			 * @return AbstractEntity &
			 * @version 0.8.35
			 */
			AbstractEntity & operator= (AbstractEntity && copy) noexcept = delete;

			/**
			 * @brief Destructs the abstract entity.
			 *
			 * Automatically unlinks all components and detaches observer relationships.
			 * Derived classes (Node, StaticEntity) handle cleanup of their specific resources.
			 *
			 * @version 0.8.35
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
			 * @brief Returns the parent scene where the entity lives.
			 *
			 * @return const Scene & Reference to the scene that owns this entity.
			 *
			 * @note The scene reference is immutable and valid for the entity's lifetime.
			 *       Entity is destroyed when scene is destroyed.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Scene &
			parentScene () const noexcept
			{
				return m_scene;
			}

			/**
			 * @brief Returns the physical properties (read-only).
			 *
			 * Physical properties are aggregated from all components with physical
			 * properties enabled (mass, drag, bounciness, etc.). Properties are
			 * automatically updated when components are added/removed or modified.
			 *
			 * @return const Physics::BodyPhysicalProperties & Reference to aggregated properties.
			 *
			 * @note To modify properties, use the non-const overload or modify component properties.
			 * @see hasBodyPhysicalProperties()
			 * @version 0.8.35
			 */
			const Physics::BodyPhysicalProperties &
			bodyPhysicalProperties () const noexcept
			{
				return m_bodyPhysicalProperties;
			}

			/**
			 * @brief Returns the physical properties (writable).
			 *
			 * Allows direct modification of aggregated physical properties. Note that
			 * properties are recalculated when components change, so manual modifications
			 * may be overwritten.
			 *
			 * @return Physics::BodyPhysicalProperties & Writable reference to aggregated properties.
			 *
			 * @warning Manual changes to properties are overwritten when components are added/removed.
			 *          Prefer modifying individual component properties instead.
			 * @version 0.8.35
			 */
			Physics::BodyPhysicalProperties &
			bodyPhysicalProperties () noexcept
			{
				return m_bodyPhysicalProperties;
			}

			/**
			 * @brief Returns whether the entity has any components.
			 *
			 * @return bool True if at least one component is attached, false if empty.
			 *
			 * @note This method is thread-safe (protected by m_componentsMutex).
			 * @version 0.8.35
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
			 *
			 * @param name The name of the component to search for.
			 * @return bool True if a component with this name exists, false otherwise.
			 *
			 * @note This method is thread-safe (protected by m_componentsMutex).
			 * @see getComponent()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool containsComponent (std::string_view name) const noexcept;

			/**
			 * @brief Returns a component by name (untyped version).
			 *
			 * Retrieves a component by name without type casting. Prefer the templated
			 * overload for type-safe access.
			 *
			 * @param name The name of the component to retrieve.
			 * @return std::shared_ptr< Component::Abstract > Shared pointer to the component, or nullptr if not found.
			 *
			 * @note This method is thread-safe (protected by m_componentsMutex).
			 * @see getComponent< component_t >()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< Component::Abstract > getComponent (std::string_view name) noexcept;

			/**
			 * @brief Gets a component by name with automatic type casting.
			 *
			 * Type-safe retrieval with automatic downcast to the expected component type.
			 * Returns nullptr if the component doesn't exist or has the wrong type.
			 *
			 * @tparam component_t The expected type of the component (must inherit Component::Abstract).
			 * @param name The name of the component to retrieve.
			 * @return std::shared_ptr< component_t > Shared pointer to the component cast to component_t, or nullptr if not found or wrong type.
			 *
			 * @note This method is thread-safe (protected by m_componentsMutex).
			 * @see containsComponent(), getComponentsOfType()
			 * @version 0.8.35
			 */
			template< typename component_t >
			std::shared_ptr< component_t >
			getComponent (std::string_view name) noexcept requires (std::is_base_of_v< Component::Abstract, component_t >)
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
			 *
			 * Retrieves all components that can be cast to the specified type. Useful for
			 * iterating over all lights, visuals, etc. without knowing their names.
			 *
			 * @tparam component_t The type of components to retrieve (must inherit Component::Abstract).
			 * @return Libs::StaticVector< std::shared_ptr< component_t >, MaxComponentCount > Vector of all components matching the type (may be empty).
			 *
			 * @note This method is thread-safe (protected by m_componentsMutex).
			 * @note Uses dynamic_pointer_cast, so only returns components that are exactly component_t or derived from it.
			 * @version 0.8.35
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
			 * @brief Executes a function per component with thread-safe access (non-const version).
			 *
			 * Iterates over all components and applies the provided function. The mutex is held
			 * during the entire iteration, so keep the function fast to avoid blocking other threads.
			 *
			 * @tparam function_t The type of function. Signature: void (Component::Abstract &)
			 * @param processComponent The function to execute for each component.
			 *
			 * @note This method is thread-safe (protected by m_componentsMutex).
			 * @warning The function should not add/remove components (would deadlock).
			 * @version 0.8.35
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
			 * @brief Executes a function per component with thread-safe access (const version).
			 *
			 * Const overload for read-only iteration over components.
			 *
			 * @tparam function_t The type of function. Signature: void (const Component::Abstract &)
			 * @param processComponent The function to execute for each component.
			 *
			 * @note This method is thread-safe (protected by m_componentsMutex).
			 * @version 0.8.35
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
			 * @brief Removes a component by its name from this entity.
			 *
			 * Unlinks the component, triggers destruction notifications, and updates entity properties.
			 *
			 * @param name The name of the component to remove.
			 * @return bool True if the component existed and was removed, false if not found.
			 *
			 * @note This method is thread-safe (protected by m_componentsMutex).
			 * @see clearComponents()
			 * @version 0.8.35
			 */
			bool removeComponent (std::string_view name) noexcept;

			/**
			 * @brief Removes all components from this entity.
			 *
			 * Unlinks all components, triggers destruction notifications for each, and resets
			 * entity properties (bounding primitives, physical properties, flags).
			 *
			 * @note This method is thread-safe (protected by m_componentsMutex).
			 * @see removeComponent()
			 * @version 0.8.35
			 */
			void clearComponents () noexcept;

			/**
			 * @brief Suspends the entity and all its components.
			 *
			 * Called when the scene is disabled. This releases pooled resources
			 * (audio sources, etc.) while preserving state for later restoration.
			 * Calls onSuspend() for entity-specific cleanup, then suspends all components.
			 *
			 * @note This method is thread-safe (protected by m_componentsMutex).
			 * @see wakeup() To restore activity.
			 * @version 0.8.35
			 */
			void suspend () noexcept;

			/**
			 * @brief Wakes up the entity and all its components.
			 *
			 * Called when the scene is re-enabled. This reacquires pooled resources
			 * and restores state from before suspend().
			 * Calls onWakeup() for entity-specific restoration, then wakes up all components.
			 *
			 * @note This method is thread-safe (protected by m_componentsMutex).
			 * @see suspend() To release resources.
			 * @version 0.8.35
			 */
			void wakeup () noexcept;

			/**
			 * @brief Creates a component builder for fluent API construction.
			 *
			 * Entry point for the ComponentBuilder pattern. Use this to construct and configure
			 * components with a chainable interface before adding them to the entity.
			 *
			 * @tparam component_t The type of component to build (must inherit Component::Abstract).
			 * @param componentName The name of the component (must be unique within this entity).
			 * @return ComponentBuilder< component_t > Builder instance for method chaining.
			 *
			 * @see ComponentBuilder
			 * @version 0.8.35
			 */
			template< typename component_t >
			ComponentBuilder< component_t >
			componentBuilder (const std::string & componentName) noexcept
			{
				return ComponentBuilder< component_t >(*this, componentName);
			}

			/**
			 * @brief Overrides automatic bounding primitive computation.
			 *
			 * By default, bounding primitives are automatically computed by merging all
			 * component bounding shapes. This method allows manual override for custom
			 * collision shapes or performance optimization.
			 *
			 * @param box The custom axis-aligned bounding box (AABB).
			 * @param sphere The custom bounding sphere.
			 *
			 * @note Once overridden, bounding primitives will NOT update when components change.
			 *       Call this again with new values if needed.
			 * @note Sets the BoundingPrimitivesOverridden flag.
			 * @see localBoundingBox(), localBoundingSphere()
			 * @version 0.8.35
			 */
			void overrideBoundingPrimitives (const Libs::Math::Space3D::AACuboid< float > & box, const Libs::Math::Space3D::Sphere< float > & sphere) noexcept;

			/**
			 * @brief Enables a visual debug overlay for this entity.
			 *
			 * Creates visual components to display debug information (axes, velocity, bounding
			 * shapes, camera frustum). Debug visuals are rendered like regular components.
			 *
			 * @param resourceManager Reference to the resource manager (used to create debug meshes).
			 * @param type The type of visual debug to enable.
			 *
			 * @note Debug visuals are created as internal components and updated automatically.
			 * @see disableVisualDebug(), toggleVisualDebug(), VisualDebugType
			 * @version 0.8.35
			 */
			void enableVisualDebug (Resources::Manager & resourceManager, VisualDebugType type) noexcept;

			/**
			 * @brief Disables a visual debug overlay for this entity.
			 *
			 * Removes the debug visual component created by enableVisualDebug().
			 *
			 * @param type The type of visual debug to disable.
			 *
			 * @see enableVisualDebug(), toggleVisualDebug()
			 * @version 0.8.35
			 */
			void disableVisualDebug (VisualDebugType type) noexcept;

			/**
			 * @brief Toggles the visibility of a debug overlay and returns the new state.
			 *
			 * Enables the debug visual if currently disabled, disables if currently enabled.
			 *
			 * @param resourceManager Reference to the resource manager (used if enabling).
			 * @param type The type of visual debug to toggle.
			 * @return bool True if debug visual is now enabled, false if now disabled.
			 *
			 * @see enableVisualDebug(), disableVisualDebug()
			 * @version 0.8.35
			 */
			bool toggleVisualDebug (Resources::Manager & resourceManager, VisualDebugType type) noexcept;

			/**
			 * @brief Returns whether a visual debug overlay is currently displayed.
			 *
			 * @param type The type of visual debug to query.
			 * @return bool True if the debug visual is enabled, false otherwise.
			 *
			 * @see enableVisualDebug(), disableVisualDebug()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool isVisualDebugEnabled (VisualDebugType type) const noexcept;

			/**
			 * @brief Updates component logics and returns whether the entity has moved in the scene.
			 *
			 * Called once per engine cycle by the Scene. Processes all component logic updates,
			 * removes components marked for deletion (shouldBeRemoved()), and delegates to
			 * derived class via onProcessLogics().
			 *
			 * @param scene Reference to the parent scene.
			 * @param engineCycle The current engine cycle number (used to track movement).
			 * @return bool True if the entity moved during this cycle, false otherwise.
			 *
			 * @note Components marked shouldBeRemoved() are automatically removed during this call.
			 * @note Movement state is tracked via m_lastUpdatedMoveCycle for hasMoved() queries.
			 * @see hasMoved(), onProcessLogics()
			 * @version 0.8.35
			 */
			bool processLogics (const Scene & scene, size_t engineCycle) noexcept;

			/**
			 * @brief Returns whether the entity has moved since the last cycle.
			 *
			 * Used by Scene to determine if entity needs spatial partitioning (octree) updates.
			 *
			 * @param engineCycle The current engine cycle number.
			 * @return bool True if entity moved in the previous cycle (engineCycle - 1), false otherwise.
			 *
			 * @note Movement is tracked by comparing m_lastUpdatedMoveCycle (set in processLogics()).
			 * @see processLogics()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			hasMoved (size_t engineCycle) const noexcept
			{
				return m_lastUpdatedMoveCycle >= engineCycle - 1;
			}

			/**
			 * @brief Returns the scene time when the entity was created (in milliseconds).
			 *
			 * @return uint32_t Entity creation timestamp in scene time (milliseconds).
			 *
			 * @note This is scene time, not system time. Value is relative to scene start.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint32_t
			birthTime () const noexcept
			{
				return m_birthTime;
			}

			/**
			 * @brief Returns whether the entity is collidable (has bounding shapes).
			 *
			 * An entity is a deflector if it has valid bounding primitives and collision
			 * is not disabled. Deflectors participate in collision detection.
			 *
			 * @return bool True if entity has bounding shapes and collision enabled, false otherwise.
			 *
			 * @note Automatically set based on bounding box validity (updateEntityProperties()).
			 * @see disableCollision(), isCollisionDisabled()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isDeflector () const noexcept
			{
				return this->isFlagEnabled(IsDeflector);
			}

			/**
			 * @brief Returns whether the entity is renderable.
			 *
			 * An entity is renderable if it has at least one component with rendering capability
			 * (Visual, MultipleVisuals, ParticlesEmitter, debug visuals).
			 *
			 * @return bool True if entity has renderable components, false otherwise.
			 *
			 * @note Automatically set based on component properties (updateEntityProperties()).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isRenderable () const noexcept
			{
				return this->isFlagEnabled(IsRenderable);
			}

			/**
			 * @brief Returns whether the entity has valid physical properties.
			 *
			 * Physical properties are aggregated from components with non-null mass.
			 * If true, bodyPhysicalProperties() contains valid aggregated values.
			 *
			 * @return bool True if entity has at least one component with physical properties, false otherwise.
			 *
			 * @note Automatically set based on component properties (updateEntityProperties()).
			 * @todo [PHYSICS] Should query m_bodyPhysicalProperties directly instead of using flag.
			 * @see bodyPhysicalProperties()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			hasBodyPhysicalProperties () const noexcept
			{
				return this->isFlagEnabled(HasBodyPhysicalProperties);
			}

			/**
			 * @brief Pauses physics simulation on this entity.
			 *
			 * When paused, the entity will not receive automatic forces (gravity, drag).
			 * The simulation automatically resumes when custom forces are applied.
			 *
			 * @param state True to pause simulation, false to resume.
			 *
			 * @note Manual forces (applyForce) automatically resume simulation.
			 * @see isSimulationPaused()
			 * @version 0.8.35
			 */
			void
			pauseSimulation (bool state) noexcept
			{
				this->setFlag(IsSimulationPaused, state);
			}

			/**
			 * @brief Returns whether physics simulation is paused on this entity.
			 *
			 * @return bool True if simulation is paused (no gravity/drag), false if active.
			 *
			 * @see pauseSimulation()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isSimulationPaused () const noexcept
			{
				return this->isFlagEnabled(IsSimulationPaused);
			}

			/**
			 * @brief Disables collision tests for this entity.
			 *
			 * When disabled, the entity will not participate in collision detection even
			 * if it has valid bounding primitives. Use for non-solid visuals, triggers, etc.
			 *
			 * @note This also disables the IsDeflector flag.
			 * @see isCollisionDisabled()
			 * @version 0.8.35
			 */
			void
			disableCollision () noexcept
			{
				this->enableFlag(IsCollisionDisabled);
			}

			/**
			 * @brief Returns whether collision tests are disabled for this entity.
			 *
			 * @return bool True if collision is disabled, false if collision is enabled.
			 *
			 * @see disableCollision()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isCollisionDisabled () const noexcept
			{
				return this->isFlagEnabled(IsCollisionDisabled);
			}

			/**
			 * @brief Returns whether the entity has movement capability.
			 *
			 * Indicates if this entity type supports physics movement (velocity, forces, etc.).
			 * Node returns true, StaticEntity returns false.
			 *
			 * @return bool True if entity can move (has MovableTrait), false if static.
			 *
			 * @note If true, getMovableTrait() will return a valid pointer.
			 * @see getMovableTrait(), isMoving()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			virtual bool hasMovableAbility () const noexcept = 0;

			/**
			 * @brief Returns the movable trait for physics movement (non-const version).
			 *
			 * Provides access to velocity, forces, and other movement properties. Only valid
			 * if hasMovableAbility() returns true.
			 *
			 * @return Physics::MovableTrait * Pointer to movable trait (entity-owned), or nullptr if static.
			 *
			 * @note Caller must not delete this pointer - entity owns it.
			 * @see hasMovableAbility(), isMoving()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			virtual Physics::MovableTrait * getMovableTrait () noexcept = 0;

			/**
			 * @brief Returns the movable trait for physics movement (const version).
			 *
			 * @return const Physics::MovableTrait * Pointer to movable trait (entity-owned), or nullptr if static.
			 *
			 * @note Caller must not delete this pointer - entity owns it.
			 * @see hasMovableAbility(), isMoving()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			virtual const Physics::MovableTrait * getMovableTrait () const noexcept = 0;

			/**
			 * @brief Returns whether the entity is currently moving (has non-zero velocity).
			 *
			 * Static entities always return false. Nodes with zero velocity return false.
			 *
			 * @return bool True if entity has non-zero velocity, false otherwise.
			 *
			 * @see hasMovableAbility(), getMovableTrait()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			virtual bool isMoving () const noexcept = 0;

			/**
			 * @brief Publishes current entity state to render buffer (double-buffering).
			 *
			 * Called at the end of the logic frame to copy logic state (position, orientation)
			 * to the render-safe buffer. Render thread reads from this buffer without blocking
			 * logic thread.
			 *
			 * @param writeStateIndex The buffer index to write to (0 or 1, alternates each frame).
			 *
			 * @note This implements the double-buffering mechanism for thread-safe rendering.
			 * @see getWorldCoordinatesStateForRendering()
			 * @version 0.8.35
			 */
			virtual void publishStateForRendering (uint32_t writeStateIndex) noexcept = 0;

			/**
			 * @brief Returns the world coordinates of the entity for rendering (read from stable buffer).
			 *
			 * Called by render thread to retrieve stable position/orientation without blocking
			 * logic thread. Reads from the buffer NOT currently being written to.
			 *
			 * @param readStateIndex The buffer index to read from (0 or 1, opposite of write index).
			 * @return const Libs::Math::CartesianFrame< float > & Reference to stable world coordinates.
			 *
			 * @note This implements the double-buffering mechanism for thread-safe rendering.
			 * @see publishStateForRendering()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			virtual const Libs::Math::CartesianFrame< float > & getWorldCoordinatesStateForRendering (uint32_t readStateIndex) const noexcept = 0;

		protected:

			/* Friend declarations */
			template< typename component_t >
			friend class ComponentBuilder;

			/**
			 * @brief Flag indices for FlagArrayTrait< 8 >.
			 *
			 * AbstractEntity uses 6 of the 8 available flags. Derived classes can use flags
			 * starting from NextFlag (currently 6).
			 *
			 * @version 0.8.35
			 */
			static constexpr auto BoundingPrimitivesOverridden{0UL}; ///< Bounding shapes manually overridden (don't auto-update).
			static constexpr auto IsDeflector{1UL};                  ///< Entity has valid bounding shapes and collision enabled.
			static constexpr auto IsRenderable{2UL};                 ///< Entity has at least one renderable component.
			static constexpr auto HasBodyPhysicalProperties{3UL};    ///< Entity has aggregated physical properties (mass > 0).
			static constexpr auto IsSimulationPaused{4UL};           ///< Physics simulation paused (no gravity/drag).
			static constexpr auto IsCollisionDisabled{5UL};          ///< Collision detection disabled for this entity.
			static constexpr auto NextFlag{6UL};                     ///< First available flag for derived classes (Node, StaticEntity).

			/**
			 * @brief Constructs an abstract entity.
			 *
			 * Protected constructor - only derived classes (Node, StaticEntity) can instantiate.
			 *
			 * @param scene Reference to the scene that owns this entity (stored immutably).
			 * @param entityName Unique name for this entity (moved into NameableTrait).
			 * @param sceneTimepointMS Scene timestamp at creation (stored as birthTime).
			 *
			 * @note Scene reference is immutable and valid for entity's lifetime.
			 * @version 0.8.35
			 */
			AbstractEntity (const Scene & scene, std::string entityName, uint32_t sceneTimepointMS) noexcept
				: NameableTrait{std::move(entityName)},
				m_scene{scene},
				m_birthTime{sceneTimepointMS}
			{

			}

			/**
			 * @brief Called when the entity is suspended.
			 *
			 * Override in derived classes (Node, StaticEntity) to perform
			 * entity-specific cleanup when the scene is disabled.
			 *
			 * @note Default implementation does nothing.
			 * @see onWakeup()
			 * @version 0.8.35
			 */
			virtual void
			onSuspend () noexcept
			{

			}

			/**
			 * @brief Called when the entity wakes up.
			 *
			 * Override in derived classes (Node, StaticEntity) to perform
			 * entity-specific restoration when the scene is re-enabled.
			 *
			 * @note Default implementation does nothing.
			 * @see onSuspend()
			 * @version 0.8.35
			 */
			virtual void
			onWakeup () noexcept
			{

			}

			/**
			 * @brief Updates components when the entity moves.
			 *
			 * Called by derived classes (Node, StaticEntity) when world coordinates change.
			 * Dispatches move() to all components so they can update world-space data
			 * (lights, sounds, cameras, etc.).
			 *
			 * @param worldCoordinates The new world coordinates (position + orientation).
			 *
			 * @note This is thread-safe (protected by m_componentsMutex).
			 * @version 0.8.35
			 */
			void onContainerMove (const Libs::Math::CartesianFrame< float > & worldCoordinates) noexcept;

			/**
			 * @brief Sets the renderable state flag.
			 *
			 * Called by updateEntityProperties() when components with rendering capability
			 * are detected.
			 *
			 * @param state True if entity has renderable components, false otherwise.
			 *
			 * @note Prefer updateEntityProperties() over manual flag manipulation.
			 * @version 0.8.35
			 */
			void
			setRenderingAbilityState (bool state) noexcept
			{
				this->setFlag(IsRenderable, state);
			}

			/**
			 * @brief Sets the physical properties state flag.
			 *
			 * Called by updateEntityProperties() when components with physical properties
			 * (non-null mass) are detected.
			 *
			 * @param state True if entity has physical components, false otherwise.
			 *
			 * @note Prefer updateEntityProperties() over manual flag manipulation.
			 * @version 0.8.35
			 */
			void
			setBodyPhysicalPropertiesState (bool state) noexcept
			{
				this->setFlag(HasBodyPhysicalProperties, state);
			}

			/**
			 * @brief Derived class logic update hook.
			 *
			 * Called by processLogics() after component updates. Derived classes implement
			 * their specific logic (physics movement for Node, nothing for StaticEntity).
			 *
			 * @param scene Reference to the parent scene.
			 * @return bool True if entity moved during this logic update, false otherwise.
			 *
			 * @note Node implements physics integration here.
			 * @todo [PHYSICS] Should use dedicated physics update method.
			 * @version 0.8.35
			 */
			virtual bool onProcessLogics (const Scene & scene) noexcept = 0;

			/**
			 * @brief Derived class content modification hook.
			 *
			 * Called by updateEntityProperties() after bounding primitives and physical
			 * properties are recalculated. Derived classes can perform additional updates
			 * (e.g., octree re-insertion).
			 *
			 * @note This is NOT called for every component change, only when properties
			 *       (bounding shapes, mass, etc.) are recalculated.
			 * @version 0.8.35
			 */
			virtual void onContentModified () noexcept = 0;

		private:

			/**
			 * @copydoc EmEn::Libs::ObserverTrait::onNotification()
			 *
			 * Observes Component::Abstract (ComponentContentModified) and
			 * Physics::BodyPhysicalProperties (PropertiesChanged) to trigger
			 * updateEntityProperties() when components change.
			 *
			 * Delegates unhandled notifications to derived classes via onUnhandledNotification().
			 *
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept final;

			/**
			 * @brief Recalculates entity properties when components change.
			 *
			 * Aggregates physical properties (mass, drag, bounciness) from all components,
			 * merges bounding primitives, updates flags (IsRenderable, IsDeflector,
			 * HasBodyPhysicalProperties), and triggers onContentModified() hook.
			 *
			 * Called automatically when:
			 * - Components are added/removed
			 * - Component properties change (ComponentContentModified notification)
			 * - Physical properties change (PropertiesChanged notification)
			 *
			 * @note If BoundingPrimitivesOverridden flag is set, bounding shapes are NOT updated.
			 * @note This is the central orchestrator for entity state consistency.
			 * @version 0.8.35
			 */
			void updateEntityProperties () noexcept;

			/**
			 * @brief Links a component to the entity (internal).
			 *
			 * Called by ComponentBuilder::build(). Adds component to m_components, registers
			 * observers, updates properties, and triggers notification codes.
			 *
			 * @param component Shared pointer to the component to link.
			 * @param isPrimaryDevice True if this is a primary Camera/Microphone (triggers PrimaryCameraCreated/PrimaryMicrophoneCreated).
			 * @return bool True if component was linked successfully, false if m_components is full (MaxComponentCount reached).
			 *
			 * @note This method must be in .cpp to ensure std::any typeinfo consistency across dynamic library boundaries.
			 * @version 0.8.35
			 */
			bool linkComponent (const std::shared_ptr< Component::Abstract > & component, bool isPrimaryDevice = false) noexcept;

			/**
			 * @brief Unlinks a component from the entity (internal).
			 *
			 * Detaches observers, triggers destruction notifications (component-specific codes
			 * like CameraDestroyed, VisualComponentDestroyed, etc.).
			 *
			 * @param component Shared pointer to the component to unlink.
			 *
			 * @note Does NOT remove from m_components vector - caller must erase separately.
			 * @note Does NOT call updateEntityProperties() - caller must do this.
			 * @version 0.8.35
			 */
			void unlinkComponent (const std::shared_ptr< Component::Abstract > & component) noexcept;

			/**
			 * @brief Updates enabled visual debug overlays when entity properties change.
			 *
			 * Called by updateEntityProperties() to refresh debug visuals (bounding box,
			 * sphere, velocity arrow) when bounding primitives or physical properties change.
			 *
			 * @note Only updates already-enabled debug visuals, does not create new ones.
			 * @version 0.8.35
			 */
			void updateVisualDebug () noexcept;

			/**
			 * @brief Returns or creates opaque material for debug visuals (axes, velocity).
			 *
			 * @param resources Reference to resource manager (used for material creation).
			 * @return std::shared_ptr< Graphics::Material::BasicResource > Shared pointer to plain debug material.
			 *
			 * @note Cached after first creation - subsequent calls return same material.
			 * @todo This should be moved to a centralized debug utilities class.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Material::BasicResource > getPlainVisualDebugMaterial (Resources::Manager & resources) noexcept;

			/**
			 * @brief Returns or creates translucent material for debug visuals (bounding shapes).
			 *
			 * @param resources Reference to resource manager (used for material creation).
			 * @return std::shared_ptr< Graphics::Material::BasicResource > Shared pointer to translucent debug material.
			 *
			 * @note Cached after first creation - subsequent calls return same material.
			 * @todo This should be moved to a centralized debug utilities class.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Material::BasicResource > getTranslucentVisualDebugMaterial (Resources::Manager & resources) noexcept;

			/**
			 * @brief Returns or creates the axis debug mesh (RGB arrows for XYZ).
			 *
			 * @param resources Reference to resource manager (used for mesh creation).
			 * @return std::shared_ptr< Graphics::Renderable::MeshResource > Shared pointer to axis debug mesh.
			 *
			 * @note Cached after first creation - subsequent calls return same mesh.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Renderable::MeshResource > getAxisVisualDebug (Resources::Manager & resources) noexcept;

			/**
			 * @brief Returns or creates the velocity debug mesh (directional arrow).
			 *
			 * @param resources Reference to resource manager (used for mesh creation).
			 * @return std::shared_ptr< Graphics::Renderable::MeshResource > Shared pointer to velocity debug mesh.
			 *
			 * @note Cached after first creation - subsequent calls return same mesh.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Renderable::MeshResource > getVelocityVisualDebug (Resources::Manager & resources) noexcept;

			/**
			 * @brief Returns or creates the bounding box debug mesh (wireframe AABB).
			 *
			 * @param resources Reference to resource manager (used for mesh creation).
			 * @return std::shared_ptr< Graphics::Renderable::MeshResource > Shared pointer to bounding box debug mesh.
			 *
			 * @note Cached after first creation - subsequent calls return same mesh.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Renderable::MeshResource > getBoundingBoxVisualDebug (Resources::Manager & resources) noexcept;

			/**
			 * @brief Returns or creates the bounding sphere debug mesh (wireframe sphere).
			 *
			 * @param resources Reference to resource manager (used for mesh creation).
			 * @return std::shared_ptr< Graphics::Renderable::MeshResource > Shared pointer to bounding sphere debug mesh.
			 *
			 * @note Cached after first creation - subsequent calls return same mesh.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Renderable::MeshResource > getBoundingSphereVisualDebug (Resources::Manager & resources) noexcept;

			/**
			 * @brief Returns or creates the camera debug mesh (frustum wireframe).
			 *
			 * @param resources Reference to resource manager (used for mesh creation).
			 * @return std::shared_ptr< Graphics::Renderable::MeshResource > Shared pointer to camera debug mesh.
			 *
			 * @note Cached after first creation - subsequent calls return same mesh.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			static std::shared_ptr< Graphics::Renderable::MeshResource > getCameraVisualDebug (Resources::Manager & resources) noexcept;

			/**
			 * @brief Sets the deflector state flag (internal).
			 *
			 * Called by updateEntityProperties() when bounding box validity changes.
			 *
			 * @param state True if entity has valid bounding shapes, false otherwise.
			 *
			 * @note Prefer updateEntityProperties() over manual flag manipulation.
			 * @version 0.8.35
			 */
			void
			setDeflectorState (bool state) noexcept
			{
				this->setFlag(IsDeflector, state);
			}

			/**
			 * @brief Derived class notification fallback hook.
			 *
			 * Called by onNotification() for notifications not handled at AbstractEntity level.
			 * Derived classes (Node, StaticEntity) can handle additional observables here.
			 *
			 * @param observable Pointer to the observable that sent the notification.
			 * @param notificationCode The notification code sent.
			 * @param data Additional data passed with the notification (std::any).
			 * @return bool True if notification was handled, false to auto-detach observer.
			 *
			 * @note If returns false, the observer relationship is automatically broken.
			 * @todo [GENERAL] Should use dedicated method. Rethink the purpose.
			 * @version 0.8.35
			 */
			virtual bool onUnhandledNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept = 0;

			/**
			 * @brief Derived class location update hook.
			 *
			 * Called when entity's world coordinates change. Derived classes can perform
			 * updates (e.g., Node updates octree insertion, StaticEntity may do nothing).
			 *
			 * @note Called by derived classes, not by AbstractEntity directly.
			 * @todo [PHYSICS] Should use dedicated physics method.
			 * @version 0.8.35
			 */
			virtual void onLocationDataUpdate () noexcept = 0;

			/* Member variables */
			const Scene & m_scene;                          ///< Reference to parent scene (immutable, valid for lifetime).
			Libs::StaticVector< std::shared_ptr< Component::Abstract >, MaxComponentCount > m_components; ///< Fixed-size component storage.
			mutable std::mutex m_componentsMutex;           ///< Protects m_components for thread-safe access.
			Libs::Math::Space3D::AACuboid< float > m_boundingBox;      ///< Aggregated axis-aligned bounding box (local space).
			Libs::Math::Space3D::Sphere< float > m_boundingSphere;     ///< Aggregated bounding sphere (local space).
			Physics::BodyPhysicalProperties m_bodyPhysicalProperties;  ///< Aggregated physical properties (mass, drag, etc.).
			const uint32_t m_birthTime{0};                  ///< Scene timestamp at creation (milliseconds).
			size_t m_lastUpdatedMoveCycle{0};               ///< Last engine cycle when entity moved (for hasMoved()).
			CollisionDetectionModel m_collisionDetectionModel{CollisionDetectionModel::Sphere}; ///< Collision detection algorithm preference.
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
