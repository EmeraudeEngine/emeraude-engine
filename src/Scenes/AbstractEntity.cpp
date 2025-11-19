/*
 * src/Scenes/AbstractEntity.cpp
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

#include "AbstractEntity.hpp"

/* Local inclusions. */
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

		if ( !this->isFlagEnabled(BoundingPrimitivesOverridden) )
		{
			/* Reset the bounding box to make a valid
			 * bounding box merging from entities. */
			m_boundingBox.reset();
			m_boundingSphere.reset();
		}

		for ( const auto & component : m_components )
		{
			/* Checks render ability. */
			if ( component->isRenderable() )
			{
				this->setRenderingAbilityState(true);
			}

			if ( component->isPhysicalPropertiesEnabled() )
			{
				const auto & bodyPhysicalProperties = component->bodyPhysicalProperties();

				/* Gets physical properties of a component only if it's a physical object. */
				if ( !bodyPhysicalProperties.isMassNull() )
				{
					surface += bodyPhysicalProperties.surface();
					mass += bodyPhysicalProperties.mass();

					dragCoefficient += bodyPhysicalProperties.dragCoefficient();
					angularDragCoefficient += bodyPhysicalProperties.angularDragCoefficient();
					bounciness += bodyPhysicalProperties.bounciness();
					stickiness += bodyPhysicalProperties.stickiness();
					/* FIXME: How to combine this ! */
					inertiaTensor = bodyPhysicalProperties.inertiaTensor();

					physicalEntityCount++;
				}

				if ( !this->isFlagEnabled(BoundingPrimitivesOverridden) )
				{
					/* Merge the component local bounding shapes to the scene node local bounding shapes. */
					m_boundingBox.merge(component->localBoundingBox());
					m_boundingSphere.merge(component->localBoundingSphere());
				}
			}
		}

		if ( !this->isCollisionDisabled() )
		{
			this->setDeflectorState(m_boundingBox.isValid());
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
				/* FIXME: Incorrect ! */
				inertiaTensor
			);

			this->setBodyPhysicalPropertiesState(true);
		}
		else
		{
			m_bodyPhysicalProperties.reset();

			this->setBodyPhysicalPropertiesState(false);
		}

		/* NOTE: Update bounding primitive visual representations. */
		this->updateVisualDebug();

		this->onContentModified();
	}

	void
	AbstractEntity::overrideBoundingPrimitives (const Space3D::AACuboid< float > & box, const Space3D::Sphere< float > & sphere) noexcept
	{
		m_boundingBox = box;
		m_boundingSphere = sphere;

		this->enableFlag(BoundingPrimitivesOverridden);

		/* NOTE: Update bounding primitives visual representation. */
		this->updateVisualDebug();
	}

	void
	AbstractEntity::onContainerMove (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		/* NOTE: Dispatch the move to every component. */
		for ( const auto & component : m_components )
		{
			component->move(worldCoordinates);
		}
	}

	bool
	AbstractEntity::addComponent (const std::shared_ptr< Component::Abstract > & component) noexcept
	{
		if ( m_components.full() )
		{
			TraceError{TracerTag} << "Unable to add a new component !";

			return false;
		}

		m_components.push_back(component);

		/* NOTE: First update properties before sending any signals. */
		this->updateEntityProperties();

		this->observe(component.get());
		this->observe(&component->bodyPhysicalProperties()); // NOTE: Don't know if observing non-physical object is useful.

		this->notify(ComponentCreated, component);

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

	std::shared_ptr< Component::Abstract >
	AbstractEntity::getComponent (const std::string & name) noexcept
	{
		for ( auto componentIt = m_components.begin(); componentIt != m_components.end(); ++componentIt )
		{
			if ( (*componentIt)->name() == name )
			{
				return *componentIt;
			}
		}

		return nullptr;
	}

	bool
	AbstractEntity::removeComponent (const std::string & name) noexcept
	{
		for ( auto componentIt = m_components.begin(); componentIt != m_components.end(); ++componentIt )
		{
			if ( (*componentIt)->name() == name )
			{
				m_components.erase(componentIt);

				return true;
			}
		}

		return false;
	}

	void
	AbstractEntity::clearComponents () noexcept
	{
		for ( const auto & component : m_components )
		{
			this->unlinkComponent(component);
		}

		m_components.clear();

		this->updateEntityProperties();
	}

	bool
	AbstractEntity::processLogics (const Scene & scene, size_t engineCycle) noexcept
	{
		/* Updates every entity at this Node. */
		auto componentIt = m_components.begin();

		while ( componentIt != m_components.end() )
		{
			if ( (*componentIt)->shouldBeRemoved() )
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

		/* NOTE: If the entity has move, we save the cycle number. */
		if ( this->onProcessLogics(scene) )
		{
			m_lastUpdatedMoveCycle = engineCycle;

			return true;
		}

		return false;
	}
}
