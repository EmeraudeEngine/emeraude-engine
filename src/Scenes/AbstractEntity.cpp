/*
 * src/Scenes/AbstractEntity.cpp
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

#include "AbstractEntity.hpp"

/* STL inclusions. */
#include <algorithm>
#include <optional>
#include <utility>

/* Local inclusions. */
#include "Component/Camera.hpp"
#include "Component/CharacterController.hpp"
#include "Component/Vehicle.hpp"
#include "Scene.hpp"
#include "Component/CloudVolume.hpp"
#include "Component/DirectionalLight.hpp"
#include "Component/DirectionalPushModifier.hpp"
#include "Component/Microphone.hpp"
#include "Component/MultipleVisuals.hpp"
#include "Component/ParticlesEmitter.hpp"
#include "Component/PointLight.hpp"
#include "Component/SoundEmitter.hpp"
#include "Component/SphericalPushModifier.hpp"
#include "Component/SpotLight.hpp"
#include "Component/LineLight.hpp"
#include "Component/Visual.hpp"
#include "Component/Weight.hpp"
#include "Physics/BoxCollisionModel.hpp"
#include "Physics/CapsuleCollisionModel.hpp"
#include "Physics/CollisionModelInterface.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Graphics;
	using namespace Physics;

	constexpr auto TracerTag{"AbstractEntity"};

	bool
	AbstractEntity::onNotification (const ObservableTrait * observable, int notificationCode, const Base::Any & data) noexcept
	{
		bool identifiedObservable = false;

		if ( observable->is(Component::Abstract::getClassUID()) )
		{
			identifiedObservable = true;

			switch ( notificationCode )
			{
				/* NOTE: This signal is used for late object creation. */
				case Component::Abstract::ComponentContentModified :
					this->updateEntityProperties();
					break;

				/* NOTE: Per-tick signal from animated skinned visuals — shape only, no
				 * physical properties re-derivation.
				 * DEFERRED: this notification fires from component->processLogics() (the logic
				 * thread, during the component dispatch); the flag is consumed at the end of
				 * AbstractEntity::processLogics(), which deduplicates multiple visuals notifying in
				 * the same tick. */
				case Component::Abstract::ComponentBoundariesModified :
					m_collisionBoundariesDirty = true;
					break;

				default:
					break;
			}
		}

		if ( observable->is(BodyPhysicalProperties::getClassUID()) )
		{
			identifiedObservable = true;

			if ( notificationCode == BodyPhysicalProperties::PropertiesChanged )
			{
				this->updateEntityProperties();
			}
		}

		/* Let child class look after the notification. */
		if ( this->onUnhandledNotification(observable, notificationCode, data) )
		{
			return true;
		}

		return identifiedObservable;
	}

	uint64_t
	AbstractEntity::allocateCreationNumber (const Scene & scene) noexcept
	{
		return scene.allocateEntityNumber();
	}

	void
	AbstractEntity::updateEntityProperties () noexcept
	{
		auto physicalEntityCount = 0;

		auto surface = 0.0F;
		auto mass = 0.0F;
		auto dragCoefficient = 0.0F;
		auto angularDragCoefficient = 0.0F;
		auto bounciness = 0.0F;
		auto stickiness = 0.0F;
		std::optional< Matrix< 3, float > > inertiaTensor;

		auto masslessShapeCount = 0;
		auto masslessBounciness = 0.0F;
		auto masslessStickiness = 0.0F;

		this->setRenderingAbilityState(false);

		/* NOTE: If bounding primitives are overridden, we don't recompute them. */
		if ( m_collisionModel != nullptr && !m_collisionModel->areShapeParametersOverridden() )
		{
			m_collisionModel->resetShapeParameters();
		}

		m_renderBoundingBox.reset();

		/* NOTE: Reset flags. The collision state is derived at the end, unless the author decided it. */
		this->setRenderingAbilityState(false);

		std::shared_ptr< Component::CharacterController > characterController;
		std::shared_ptr< Component::Vehicle > vehicle;

		{
			const Base::MutexLock lock{m_componentsMutex};

			for ( const auto & component : m_components )
			{
				/* A kinematic character controller (P4): found here, applied after the loop. */
				if ( characterController == nullptr && component->isComponent(Component::CharacterController::ClassId) )
				{
					characterController = std::dynamic_pointer_cast< Component::CharacterController >(component);
				}

				/* A wheeled vehicle: this entity is its chassis. */
				if ( vehicle == nullptr && component->isComponent(Component::Vehicle::ClassId) )
				{
					vehicle = std::dynamic_pointer_cast< Component::Vehicle >(component);
				}

				/* Checks render ability, and accumulates the VISUAL extent that the rendering
				 * octree places this entity with. Kept separate from the collision model on
				 * purpose: culling asks "is any of this visible", collision asks "against what".
				 * ⚠️ A debug overlay still SETS the rendering ability — it does draw — but it must
				 * not enlarge the extent. @see Component::Abstract::setContributesToEntityExtents() */
				if ( component->isRenderable() )
				{
					this->setRenderingAbilityState(true);

					if ( component->contributesToEntityExtents() )
					{
						m_renderBoundingBox.merge(component->renderBoundingBox());
					}
				}

				/* Gets physical properties of a component. */
				if ( const auto & physicalProperties = component->bodyPhysicalProperties(); !physicalProperties.isMassNull() )
				{
					surface += physicalProperties.surface();
					mass += physicalProperties.mass();

					dragCoefficient += physicalProperties.dragCoefficient();
					angularDragCoefficient += physicalProperties.angularDragCoefficient();
					bounciness += physicalProperties.bounciness();
					stickiness += physicalProperties.stickiness();
					/* An explicit tensor is kept for a single physical component; several components leave it to the
					 * physics, derived from the entity's collision shape and its total mass (P3, decision 8c). */
					inertiaTensor = physicalEntityCount == 0 ? physicalProperties.inertiaTensor() : std::nullopt;

					physicalEntityCount++;
				}

				/* ⚠️ Same gate on the COLLISION side. Without it the axis gizmo (generated at
				 * extent 1.0) merged into the collider of whatever it was attached to, so a 1 m cube
				 * got its collider nearly doubled by the mere act of looking at it — and since the
				 * gizmo is itself SCALED BY that collider's radius, the arrows then overshot the
				 * object, which is how the defect was spotted. */
				if ( !component->contributesToEntityExtents() )
				{
					continue;
				}

				/* The contact material of a MASSLESS shape (BodyPhysicalProperties::contactMaterial()):
				 * a static solid has no mass to declare, but its contacts still have a feel. Only the
				 * components that shape the collider speak for its surface. */
				if ( const auto & physicalProperties = component->bodyPhysicalProperties(); physicalProperties.isMassNull() )
				{
					masslessBounciness += physicalProperties.bounciness();
					masslessStickiness += physicalProperties.stickiness();

					masslessShapeCount++;
				}

				/* NOTE: If no collision model we create a default AABB — unless the model was withdrawn
				 * (setCollisionModel(nullptr)): a component update must not stand a corpse up again. */
				if ( m_collisionModel == nullptr )
				{
					if ( !this->isFlagEnabled(IsCollisionModelWithdrawn) )
					{
						m_collisionModel = std::make_unique< BoxCollisionModel >(component->localBoundingBox());
					}
				}
				else if ( !m_collisionModel->areShapeParametersOverridden() )
				{
					switch ( m_collisionModel->modelType() )
					{
						case CollisionModelType::Point :
							/* Nothing to do ... */
							break;

						case CollisionModelType::Sphere :
							m_collisionModel->mergeShapeParameters(component->localBoundingSphere());
							break;

						case CollisionModelType::Box :
						case CollisionModelType::Capsule :
							m_collisionModel->mergeShapeParameters(component->localBoundingBox());
							break;

						case CollisionModelType::TriangleMesh :
							/* Its triangles are its shape (never fitted to the visuals). */
							break;
					}
				}
			}
		}

		if ( physicalEntityCount > 0 )
		{
			const auto div = static_cast< float >(physicalEntityCount);

			m_bodyPhysicalProperties.setProperties(
				mass,
				surface,
				dragCoefficient / div,
				angularDragCoefficient / div,
				clampToUnit(bounciness / div),
				clampToUnit(stickiness / div),
				inertiaTensor
			);
		}
		else
		{
			m_bodyPhysicalProperties.reset();

			/* No mass anywhere: the contact material comes from the massless shapes. It used to stay
			 * at the defaults (bounciness and stickiness 0.5), whatever the author declared — a stone
			 * wall could only feel like stone by carrying a fictitious mass. */
			if ( masslessShapeCount > 0 )
			{
				const auto div = static_cast< float >(masslessShapeCount);

				m_bodyPhysicalProperties.setBounciness(clampToUnit(masslessBounciness / div), false);
				m_bodyPhysicalProperties.setStickiness(clampToUnit(masslessStickiness / div), false);
			}
		}

		/* A kinematic character: a capsule standing on the origin, and a solid whatever its mass (unless the author
		 * decided otherwise). */
		{
			const Base::MutexLock lock{m_componentsMutex};

			m_characterController = characterController;
			m_vehicle = vehicle;
		}

		if ( characterController != nullptr && !this->isFlagEnabled(IsCollisionModelWithdrawn) )
		{
			m_collisionModel = std::make_unique< CapsuleCollisionModel >(characterController->controller().localCapsule(), true);
		}

		/* The DERIVED collision state: a component with a mass makes the entity collidable. It never
		 * overrides the author's setCollidable() — that used to be reset here on every component
		 * update, both ways: a cloud made non-solid became solid again the moment a component with a
		 * mass joined it, and a static wall could only be solid by declaring a fictitious mass. */
		if ( !this->isFlagEnabled(IsCollisionAuthored) )
		{
			this->setFlag(IsCollisionDisabled, physicalEntityCount == 0 && characterController == nullptr);
		}

		/* NOTE: Update bounding primitive visual representations. */
		this->updateVisualDebug();

		this->onContentModified();
	}

	void
	AbstractEntity::refreshCollisionBoundaries () noexcept
	{
		if ( m_collisionModel == nullptr || m_collisionModel->areShapeParametersOverridden() )
		{
			return;
		}

		m_collisionModel->resetShapeParameters();

		const Base::MutexLock lock{m_componentsMutex};

		for ( const auto & component : m_components )
		{
			switch ( m_collisionModel->modelType() )
			{
				case CollisionModelType::Point :
					/* Nothing to do ... */
					break;

				case CollisionModelType::Sphere :
					m_collisionModel->mergeShapeParameters(component->localBoundingSphere());
					break;

				case CollisionModelType::Box :
				case CollisionModelType::Capsule :
					m_collisionModel->mergeShapeParameters(component->localBoundingBox());
					break;

				case CollisionModelType::TriangleMesh :
					/* Its triangles are its shape (never fitted to the visuals). */
					break;
			}
		}
	}

	void
	AbstractEntity::setCollisionModel (std::unique_ptr< CollisionModelInterface > model) noexcept
	{
		/* Removing a model WITHDRAWS it (no body until a model is set again, isCollisionModelWithdrawn()); the scene
		 * takes the entity out of its physics octree on the notification below. */
		if ( model != nullptr )
		{
			this->setFlag(IsCollisionModelWithdrawn, false);
		}
		else if ( m_collisionModel != nullptr )
		{
			this->setFlag(IsCollisionModelWithdrawn, true);
		}

		m_collisionModel = std::move(model);

		/* The scene files the entity in its physics octree NOW: until a content notification it is not in the physics
		 * step, and a body alone with its material was notified only when its visual finished loading (asynchronous:
		 * BenchSpinner started turning 20-30 cycles late on Windows, 2026-10-02). */
		this->onContentModified();
	}

	void
	AbstractEntity::onContainerMove (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		/* NOTE: During a component dispatch (from a component's processLogics(), or from another thread meanwhile),
		 * moving now would move the siblings in the MIDDLE of their own cycle: the coordinates are kept and
		 * processLogics() dispatches them right after the components ran. The last request of a cycle wins, which is the
		 * final frame. */
		Base::StaticVector< std::shared_ptr< Component::Abstract >, MaxComponentCount > components;

		{
			const Base::MutexLock lock{m_componentsMutex};

			if ( m_dispatchingComponentLogics )
			{
				m_deferredMoveCoordinates = worldCoordinates;
				m_containerMoveDeferred = true;

				return;
			}

			components = m_components;
		}

		/* NOTE: Dispatch the move to every component, without the lock (a component may call back its entity). */
		for ( const auto & component : components )
		{
			component->move(worldCoordinates);
		}
	}

	bool
	AbstractEntity::linkComponent (const std::shared_ptr< Component::Abstract > & component, bool isPrimaryDevice) noexcept
	{
		/* ⚠️ The component dispatch of processLogics() runs on a copy of m_components: a component linking a sibling
		 * from its processLogics() is refused with a diagnostic (the list must not change during the dispatch). */
		{
			Base::MutexLock lock{m_componentsMutex};

			/* NOTE: From the dispatching thread itself (a component's processLogics()): refused; from ANOTHER thread (a
			 * scene timer spawning an actor): waits for the dispatch to end (triad 2026-09-30: a refusal there lost a
			 * lighten-marbles marble its light). */
			if ( this->awaitComponentDispatch(lock) ) [[unlikely]]
			{
				TraceError{TracerTag} << "Refusing to link the component '" << component->name() << "' to entity '" << this->name() << "' from a component's processLogics() !";

				return false;
			}

			if ( m_components.full() ) [[unlikely]]
			{
				TraceError{TracerTag} << "Unable to add a new component !";

				return false;
			}

			m_components.push_back(component);
		}

		/* NOTE: First update properties before sending any signals. */
		this->updateEntityProperties();

		this->observe(component.get());
		this->observe(&component->bodyPhysicalProperties()); // NOTE: Don't know if observing non-physical object is useful.

		this->notify(ComponentCreated, component);

		/* NOTE: Send specific component type notifications.
		 * This must be done here (in the .cpp) rather than in the template ComponentBuilder::build()
		 * for a single notification site (it was a requirement of std::any's typeinfo identity in a shared library; Base::Any compares a type-name hash, which does not need it). */
		auto * pointer = component.get();

		if ( typeid(*pointer) == typeid(Component::Camera) )
		{
			this->notify(isPrimaryDevice ? PrimaryCameraCreated : CameraCreated, std::static_pointer_cast< Component::Camera >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::Microphone) )
		{
			this->notify(isPrimaryDevice ? PrimaryMicrophoneCreated : MicrophoneCreated, std::static_pointer_cast< Component::Microphone >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::SphericalPushModifier) )
		{
			this->notify(ModifierCreated, std::static_pointer_cast< Component::AbstractModifier >(component));
			this->notify(SphericalPushModifierCreated, std::static_pointer_cast< Component::SphericalPushModifier >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::DirectionalPushModifier) )
		{
			this->notify(ModifierCreated, std::static_pointer_cast< Component::AbstractModifier >(component));
			this->notify(DirectionalPushModifierCreated, std::static_pointer_cast< Component::DirectionalPushModifier >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::DirectionalLight) )
		{
			this->notify(DirectionalLightCreated, std::static_pointer_cast< Component::DirectionalLight >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::PointLight) )
		{
			this->notify(PointLightCreated, std::static_pointer_cast< Component::PointLight >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::SpotLight) )
		{
			this->notify(SpotLightCreated, std::static_pointer_cast< Component::SpotLight >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::LineLight) )
		{
			this->notify(LineLightCreated, std::static_pointer_cast< Component::LineLight >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::SoundEmitter) )
		{
			this->notify(SoundEmitterCreated, std::static_pointer_cast< Component::SoundEmitter >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::Visual) )
		{
			this->notify(VisualCreated, std::static_pointer_cast< Component::Visual >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::MultipleVisuals) )
		{
			this->notify(MultipleVisualsCreated, std::static_pointer_cast< Component::MultipleVisuals >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::ParticlesEmitter) )
		{
			this->notify(ParticlesEmitterCreated, std::static_pointer_cast< Component::ParticlesEmitter >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::Weight) )
		{
			this->notify(WeightCreated, std::static_pointer_cast< Component::Weight >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::CloudVolume) )
		{
			this->notify(CloudVolumeCreated, std::static_pointer_cast< Component::CloudVolume >(component));
		}

		return true;
	}

	void
	AbstractEntity::unlinkComponent (const std::shared_ptr< Component::Abstract > & component) noexcept
	{
		auto * pointer = component.get();

		this->forget(pointer);
		this->forget(&pointer->bodyPhysicalProperties());

		if ( typeid(*pointer) == typeid(Component::Camera) )
		{
			this->notify(CameraDestroyed, std::static_pointer_cast< Component::Camera >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::Microphone) )
		{
			this->notify(MicrophoneDestroyed, std::static_pointer_cast< Component::Microphone >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::SphericalPushModifier) || typeid(*pointer) == typeid(Component::DirectionalPushModifier) )
		{
			this->notify(ModifierDestroyed, std::static_pointer_cast< Component::AbstractModifier >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::DirectionalLight) )
		{
			this->notify(DirectionalLightDestroyed, std::static_pointer_cast< Component::DirectionalLight >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::PointLight) )
		{
			this->notify(PointLightDestroyed, std::static_pointer_cast< Component::PointLight >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::SpotLight) )
		{
			this->notify(SpotLightDestroyed, std::static_pointer_cast< Component::SpotLight >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::LineLight) )
		{
			this->notify(LineLightDestroyed, std::static_pointer_cast< Component::LineLight >(component));
		}
		else if ( typeid(*pointer) == typeid(Component::CloudVolume) )
		{
			this->notify(CloudVolumeDestroyed, std::static_pointer_cast< Component::CloudVolume >(component));
		}

		this->notify(ComponentDestroyed);
	}

	bool
	AbstractEntity::containsComponent (std::string_view name) const noexcept
	{
		const Base::MutexLock lock{m_componentsMutex};

		return std::ranges::any_of(m_components, [&name] (const auto & component) {
			return component->name() == name;
		});
	}

	std::shared_ptr< Component::Abstract >
	AbstractEntity::getComponent (std::string_view name) noexcept
	{
		const Base::MutexLock lock{m_componentsMutex};

		auto * const it = std::ranges::find_if(m_components, [&name] (const auto & component)
											   { return component->name() == name; });

		return (it != m_components.end()) ? *it : nullptr;
	}

	bool
	AbstractEntity::removeComponent (std::string_view name) noexcept
	{
		/* ⚠️ See linkComponent(): the component loop is iterating m_components. A component asking
		 * for its own removal from processLogics() returns true from shouldBeRemoved() instead. */
		std::shared_ptr< Component::Abstract > componentToUnlink;

		{
			Base::MutexLock lock{m_componentsMutex};

			/* NOTE: The dispatching thread is refused, another one waits for the dispatch to end — see linkComponent(). */
			if ( this->awaitComponentDispatch(lock) ) [[unlikely]]
			{
				TraceError{TracerTag} << "Refusing to remove a component of entity '" << this->name() << "' from a component's processLogics() !";

				return false;
			}

			auto * const it = std::ranges::find_if(m_components, [&name] (const auto & component)
												   { return component->name() == name; });

			if ( it == m_components.end() ) [[unlikely]]
			{
				return false;
			}

			componentToUnlink = *it;
			m_components.erase(it);
		}

		/* Unlink outside mutex to avoid deadlock if unlinkComponent triggers notifications. */
		this->unlinkComponent(componentToUnlink);
		this->updateEntityProperties();

		return true;
	}

	bool
	AbstractEntity::awaitComponentDispatch (Base::MutexLock & lock) noexcept EMEN_REQUIRES(m_componentsMutex)
	{
		if ( !m_dispatchingComponentLogics )
		{
			return false;
		}

		if ( m_componentDispatchThread == std::this_thread::get_id() )
		{
			return true;
		}

		++m_componentDispatchWaiters;

		m_componentDispatchEnded.wait(lock, [this] () EMEN_REQUIRES(m_componentsMutex) {
			return !m_dispatchingComponentLogics;
		});

		--m_componentDispatchWaiters;

		return false;
	}

	void
	AbstractEntity::clearComponents () noexcept
	{
		/* ⚠️ See linkComponent(): the component loop is iterating m_components. A component asking
		 * for its own removal from processLogics() returns true from shouldBeRemoved() instead. */
		Base::StaticVector< std::shared_ptr< Component::Abstract >, MaxComponentCount > components;

		{
			Base::MutexLock lock{m_componentsMutex};

			/* NOTE: The dispatching thread is refused, another one waits for the dispatch to end — see linkComponent(). */
			if ( this->awaitComponentDispatch(lock) ) [[unlikely]]
			{
				TraceError{TracerTag} << "Refusing to remove a component of entity '" << this->name() << "' from a component's processLogics() !";

				return;
			}

			components = m_components;
			m_components.clear();
		}

		/* NOTE: Unlinked outside the lock: unlinkComponent() notifies, and an observer may call back this entity. */
		for ( const auto & component : components )
		{
			this->unlinkComponent(component);
		}

		this->updateEntityProperties();
	}

	void
	AbstractEntity::suspend () noexcept
	{
		/* Call entity-specific suspend logic. */
		this->onSuspend();

		/* Suspend all components. */
		Base::StaticVector< std::shared_ptr< Component::Abstract >, MaxComponentCount > components;

		{
			const Base::MutexLock lock{m_componentsMutex};

			components = m_components;
		}

		/* NOTE: Without the lock: a component may call back its entity. */
		for ( const auto & component : components )
		{
			component->onSuspend();
		}
	}

	void
	AbstractEntity::wakeup () noexcept
	{
		/* Call entity-specific wakeup logic. */
		this->onWakeup();

		/* Wakeup all components. */
		Base::StaticVector< std::shared_ptr< Component::Abstract >, MaxComponentCount > components;

		{
			const Base::MutexLock lock{m_componentsMutex};

			components = m_components;
		}

		/* NOTE: Without the lock: a component may call back its entity. */
		for ( const auto & component : components )
		{
			component->onWakeup();
		}
	}

	bool
	AbstractEntity::processLogics (const Scene & scene, size_t engineCycle) noexcept
	{
		/* NOTE: The components run on a copy of the list, WITHOUT the lock (CP.22: a component may call back its entity,
		 * which takes the lock afresh; the mutex was recursive for that until 2026-10-17). The list does not change
		 * meanwhile: the dispatching thread's own link / removal is refused, another thread waits for the end
		 * (awaitComponentDispatch()). A StaticVector on the stack: no allocation. */
		Base::StaticVector< std::shared_ptr< Component::Abstract >, MaxComponentCount > components;
		Base::StaticVector< std::shared_ptr< Component::Abstract >, MaxComponentCount > removedComponents;

		{
			const Base::MutexLock lock{m_componentsMutex};

			m_dispatchingComponentLogics = true;
			m_componentDispatchThread = std::this_thread::get_id();

			components = m_components;
		}

		for ( const auto & component : components )
		{
			if ( component->shouldBeRemoved() ) [[unlikely]]
			{
				removedComponents.push_back(component);
			}
			else
			{
				component->processLogics(scene);
			}
		}

		bool moveDeferred = false;
		bool waitersToWake = false;
		Base::Math::CartesianFrame< float > deferredMoveCoordinates;

		{
			const Base::MutexLock lock{m_componentsMutex};

			for ( const auto & component : removedComponents )
			{
				if ( auto * const it = std::ranges::find(m_components, component); it != m_components.end() )
				{
					m_components.erase(it);
				}
			}

			m_dispatchingComponentLogics = false;
			m_componentDispatchThread = {};
			waitersToWake = m_componentDispatchWaiters > 0;

			moveDeferred = std::exchange(m_containerMoveDeferred, false);
			deferredMoveCoordinates = m_deferredMoveCoordinates;
		}

		/* NOTE: Notified after the guard's scope (CP.31/32), and only when a thread waits (every entity ends a dispatch
		 * every cycle): the threads waiting to link or remove a component go on. */
		if ( waitersToWake )
		{
			m_componentDispatchEnded.notifyAll();
		}

		/* NOTE: Unlinked outside the lock: unlinkComponent() notifies, and an observer may call back this entity. */
		for ( const auto & component : removedComponents )
		{
			TraceWarning{TracerTag} << "Removing automatically a component from entity '" << this->name() << "' ...";

			this->unlinkComponent(component);
		}

		/* Deferred move (a component moved its entity during the dispatch): dispatched here, so the siblings see the new
		 * frame in the same cycle. */
		if ( moveDeferred )
		{
			this->onContainerMove(deferredMoveCoordinates);
		}

		/* Deferred collision shape refresh (ComponentBoundariesModified, raised during the dispatch): consumed here, once
		 * per cycle. */
		if ( m_collisionBoundariesDirty )
		{
			m_collisionBoundariesDirty = false;

			this->refreshCollisionBoundaries();
		}

		/* NOTE: If the entity has move, we save the cycle number. */
		if ( this->onProcessLogics(scene) )
		{
			m_lastUpdatedMoveCycle = engineCycle;

			return true;
		}

		return false;
	}
}
