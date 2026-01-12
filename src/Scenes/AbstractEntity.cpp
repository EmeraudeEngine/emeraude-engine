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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "AbstractEntity.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "Physics/CollisionModelInterface.hpp"
#include "Physics/AABBCollisionModel.hpp"
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
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Graphics;
	using namespace Physics;

	constexpr auto TracerTag{"AbstractEntity"};

	bool
	AbstractEntity::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept
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
		auto inertiaTensor = Matrix< 3, float >::identity();

		this->setRenderingAbilityState(false);

		/* NOTE: If bounding primitives are overridden, we don't recompute them. */
		if ( m_collisionModel != nullptr && !m_collisionModel->areShapeParametersOverridden() )
		{
			m_collisionModel->resetShapeParameters();
		}

		/* NOTE: Reset flags. */
		this->setRenderingAbilityState(false);
		this->setCollidable(false);

		{
			const std::lock_guard< std::mutex > lock{m_componentsMutex};

			for ( const auto & component : m_components )
			{
				/* Checks render ability. */
				if ( component->isRenderable() )
				{
					this->setRenderingAbilityState(true);
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
					/* FIXME: How to combine this ! */
					inertiaTensor = physicalProperties.inertiaTensor();

					physicalEntityCount++;
				}

				/* NOTE: If no collision model we create a default AABB. */
				if ( m_collisionModel == nullptr )
				{
					m_collisionModel = std::make_unique< AABBCollisionModel >(component->localBoundingBox());
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

						case CollisionModelType::AABB :
						case CollisionModelType::Capsule :
							m_collisionModel->mergeShapeParameters(component->localBoundingBox());
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
				inertiaTensor // FIXME: Incorrect !
			);

			this->setCollidable(true);
		}
		else
		{
			m_bodyPhysicalProperties.reset();
		}

		/* NOTE: Update bounding primitive visual representations. */
		this->updateVisualDebug();

		this->onContentModified();
	}

	void
	AbstractEntity::setCollisionModel (std::unique_ptr< CollisionModelInterface > model) noexcept
	{
		m_collisionModel = std::move(model);
	}

	void
	AbstractEntity::onContainerMove (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		/* NOTE: Dispatch the move to every component. */
		std::lock_guard< std::mutex > lock(m_componentsMutex);

		for ( const auto & component : m_components )
		{
			component->move(worldCoordinates);
		}
	}

	bool
	AbstractEntity::linkComponent (const std::shared_ptr< Component::Abstract > & component, bool isPrimaryDevice) noexcept
	{
		{
			std::lock_guard< std::mutex > lock(m_componentsMutex);

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
		 * to ensure std::any typeinfo consistency when Emeraude is used as a dynamic library. */
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

		this->notify(ComponentDestroyed);
	}

	bool
	AbstractEntity::containsComponent (std::string_view name) const noexcept
	{
		std::lock_guard< std::mutex > lock(m_componentsMutex);

		return std::ranges::any_of(m_components, [&name] (const auto & component) {
			return component->name() == name;
		});
	}

	std::shared_ptr< Component::Abstract >
	AbstractEntity::getComponent (std::string_view name) noexcept
	{
		std::lock_guard< std::mutex > lock(m_componentsMutex);

		auto it = std::ranges::find_if(m_components, [&name] (const auto & component) {
			return component->name() == name;
		});

		return (it != m_components.end()) ? *it : nullptr;
	}

	bool
	AbstractEntity::removeComponent (std::string_view name) noexcept
	{
		std::shared_ptr< Component::Abstract > componentToUnlink;

		{
			std::lock_guard< std::mutex > lock(m_componentsMutex);

			auto it = std::ranges::find_if(m_components, [&name] (const auto & component) {
				return component->name() == name;
			});

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

	void
	AbstractEntity::clearComponents () noexcept
	{
		{
			std::lock_guard< std::mutex > lock(m_componentsMutex);
			for ( const auto & component : m_components )
			{
				this->unlinkComponent(component);
			}

			m_components.clear();
		}

		this->updateEntityProperties();
	}

	void
	AbstractEntity::suspend () noexcept
	{
		/* Call entity-specific suspend logic. */
		this->onSuspend();

		/* Suspend all components. */
		std::lock_guard< std::mutex > lock(m_componentsMutex);

		for ( const auto & component : m_components )
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
		std::lock_guard< std::mutex > lock(m_componentsMutex);

		for ( const auto & component : m_components )
		{
			component->onWakeup();
		}
	}

	bool
	AbstractEntity::processLogics (const Scene & scene, size_t engineCycle) noexcept
	{
		/* Updates every entity at this Node. */
		{
			std::lock_guard< std::mutex > lock(m_componentsMutex);
			auto componentIt = m_components.begin();

			while ( componentIt != m_components.end() )
			{
				if ( (*componentIt)->shouldBeRemoved() ) [[unlikely]]
				{
					TraceWarning{TracerTag} << "Removing automatically a component from entity '" << this->name() << "' ...";

					this->unlinkComponent(*componentIt);

					componentIt = m_components.erase(componentIt);
				}
				else
				{
					(*componentIt)->processLogics(scene);

					++componentIt;
				}
			}
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
