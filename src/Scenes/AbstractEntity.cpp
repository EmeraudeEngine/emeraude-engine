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

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Graphics;
	using namespace Physics;

	constexpr auto TracerTag{"AbstractEntity"};

	AbstractEntity::AbstractEntity (const std::string & name, uint32_t sceneTimeMS) noexcept
		: NameableTrait(name), m_birthTime(sceneTimeMS)
	{

	}

	bool
	AbstractEntity::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept
	{
		bool identifiedObservable = false;

		if ( observable->is(Component::Abstract::ClassUID) )
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

		if ( observable->is(PhysicalObjectProperties::ClassUID) )
		{
			identifiedObservable = true;

			if ( notificationCode == PhysicalObjectProperties::PropertiesChanged )
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
		auto bounciness = 0.0F;
		auto stickiness = 0.0F;

		this->setRenderingAbilityState(false);

		if ( !this->isFlagEnabled(BoundingPrimitivesOverridden) )
		{
			/* Reset the bounding box to make a valid
			 * bounding boxes merging from entities. */
			m_boundingBox.reset();
			m_boundingSphere.reset();
		}

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			/* Checks render ability. */
			if ( component->isRenderable() )
			{
				this->setRenderingAbilityState(true);
			}

			if ( component->isPhysicalPropertiesEnabled() )
			{
				const auto & physicalObjectProperties = component->physicalObjectProperties();

				/* Gets physical properties of a component only if it's a physical object. */
				if ( !physicalObjectProperties.isMassNull() )
				{
					surface += physicalObjectProperties.surface();
					mass += physicalObjectProperties.mass();

					dragCoefficient += physicalObjectProperties.dragCoefficient();
					bounciness += physicalObjectProperties.bounciness();
					stickiness += physicalObjectProperties.stickiness();

					physicalEntityCount++;
				}

				if ( !this->isFlagEnabled(BoundingPrimitivesOverridden) )
				{
					/* Merge the component local bounding shapes to the scene node local bounding shapes. */
					m_boundingBox.merge(component->boundingBox());
					m_boundingSphere.merge(component->boundingSphere());
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

			m_physicalObjectProperties.setProperties(
				mass,
				surface,
				dragCoefficient / div,
				clampToUnit(bounciness / div),
				clampToUnit(stickiness / div)
			);

			this->setPhysicalObjectPropertiesState(true);
		}
		else
		{
			m_physicalObjectProperties.reset();

			this->setPhysicalObjectPropertiesState(false);
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
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		/* NOTE: Dispatch the move to every component. */
		for ( const auto & component: m_components | std::views::values )
		{
			component->move(worldCoordinates);
		}
	}

	bool
	AbstractEntity::containsComponent (const std::string & name) const noexcept
	{
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		return m_components.contains(name);
	}

	bool
	AbstractEntity::containsComponent (const std::shared_ptr< Component::Abstract > & component) const noexcept
	{
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		return std::ranges::any_of(m_components, [component] (const auto & pair) {
			return pair.second == component;
		});
	}

	bool
	AbstractEntity::checkComponentNameAvailability (const std::string & name) const noexcept
	{
		if ( m_components.contains(name) )
		{
			TraceWarning{TracerTag} <<
				"A component using the '" << name << "' is already present in this entity ! "
				"Creation cancelled.";

			return false;
		}

		return true;
	}

	bool
	AbstractEntity::addComponent (const std::string & name, const std::shared_ptr< Component::Abstract > & component) noexcept
	{
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		if ( !m_components.emplace(name, component).second )
		{
			TraceError{TracerTag} << "Unable to create the component '" << name << "' !";

			return false;
		}

		/* NOTE: First update properties before sending any signals. */
		this->updateEntityProperties();

		this->observe(component.get());
		this->observe(&component->physicalObjectProperties()); // NOTE: Don't know if observing non-physical object is useful.

		this->notify(ComponentCreated, component);

		return true;
	}

	void
	AbstractEntity::unlinkComponent (const std::shared_ptr< Component::Abstract > & component) noexcept
	{
		auto * pointer = component.get();

		this->forget(pointer);
		this->forget(&pointer->physicalObjectProperties());

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
	AbstractEntity::removeComponent (const std::string & name) noexcept
	{
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		const auto componentIt = m_components.find(name);

		if ( componentIt == m_components.end() )
		{
			TraceWarning{TracerTag} <<
				"No named component '" << name << "' exists. "
				"Deletion skipped.";

			return false;
		}

		this->unlinkComponent(componentIt->second);

		m_components.erase(componentIt);

		this->updateEntityProperties();

		return true;
	}

	bool
	AbstractEntity::removeComponent (const std::shared_ptr< Component::Abstract > & component) noexcept
	{
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		for ( auto componentIt = m_components.begin(); componentIt != m_components.end(); ++componentIt )
		{
			if ( componentIt->second != component )
			{
				continue;
			}

			this->unlinkComponent(componentIt->second);

			m_components.erase(componentIt);

			this->updateEntityProperties();

			return true;
		}

		TraceWarning{TracerTag} <<
			"Component " << component.get() << " doesn't exist. "
			"Deletion skipped.";

		return false;
	}

	std::shared_ptr< Component::Abstract >
	AbstractEntity::getComponent (const std::string & name) const noexcept
	{
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		const auto componentIt = m_components.find(name);

		return componentIt != m_components.cend() ? componentIt->second : nullptr;
	}

	void
	AbstractEntity::clearComponents () noexcept
	{
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		auto componentIt = std::begin(m_components);

		while ( componentIt != std::end(m_components) )
		{
			this->unlinkComponent(componentIt->second);

			componentIt = m_components.erase(componentIt);
		}

		this->updateEntityProperties();
	}

	void
	AbstractEntity::forEachComponent (const std::function< bool (const Component::Abstract & component) > & lambda) const noexcept
	{
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( lambda(*component))
			{
				break;
			}
		}
	}

	void
	AbstractEntity::forEachComponent (const std::function< bool (const Component::Abstract & component, size_t index) > & lambda) const noexcept
	{
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		for ( auto componentIt = m_components.cbegin(); componentIt != m_components.cend(); ++componentIt )
		{
			const auto index = std::distance(m_components.cbegin(), componentIt);

			if ( !lambda(*componentIt->second, index) )
			{
				break;
			}
		}
	}

	void
	AbstractEntity::forEachComponent (const std::function< bool (Component::Abstract & component) > & lambda) noexcept
	{
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		for ( const auto & component : std::ranges::views::values(m_components) )
		{
			if ( lambda(*component))
			{
				break;
			}
		}
	}

	void
	AbstractEntity::forEachComponent (const std::function< bool (Component::Abstract & component, size_t index) > & lambda) noexcept
	{
		const std::lock_guard< std::mutex > lockGuard{m_componentsAccess};

		for ( auto componentIt = m_components.begin(); componentIt != m_components.end(); ++componentIt )
		{
			const auto index = std::distance(m_components.begin(), componentIt);

			if ( !lambda(*componentIt->second, index) )
			{
				break;
			}
		}
	}

	std::shared_ptr< Component::Camera >
	AbstractEntity::newCamera (bool perspective, bool primaryDevice, const std::string & componentName) noexcept
	{
		if ( !this->checkComponentNameAvailability(componentName) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::Camera >(componentName, *this, perspective);

		/* Registering the smart pointer. */
		if ( !this->addComponent(componentName, component) )
		{
			return nullptr;
		}

		/* Notify the camera creation. */
		this->notify(primaryDevice ? PrimaryCameraCreated : CameraCreated, component);

		return component;
	}

	std::shared_ptr< Component::Microphone >
	AbstractEntity::newMicrophone (bool primaryDevice, const std::string & componentName) noexcept
	{
		if ( !this->checkComponentNameAvailability(componentName) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::Microphone >(componentName, *this);

		/* Registering the smart pointer. */
		if ( !this->addComponent(componentName, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(primaryDevice ? PrimaryMicrophoneCreated : MicrophoneCreated, component);

		return component;
	}

	std::shared_ptr< Component::DirectionalLight >
	AbstractEntity::newDirectionalLight (uint32_t shadowMapResolution, const std::string & componentName) noexcept
	{
		if ( !this->checkComponentNameAvailability(componentName) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::DirectionalLight >(componentName, *this, shadowMapResolution);

		/* Registering the smart pointer. */
		if ( !this->addComponent(componentName, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(DirectionalLightCreated, component);

		return component;
	}

	std::shared_ptr< Component::PointLight >
	AbstractEntity::newPointLight (uint32_t shadowMapResolution, const std::string & componentName) noexcept
	{
		if ( !this->checkComponentNameAvailability(componentName) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::PointLight >(componentName, *this, shadowMapResolution);

		/* Registering the smart pointer. */
		if ( !this->addComponent(componentName, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(PointLightCreated, component);

		return component;
	}

	std::shared_ptr< Component::SpotLight >
	AbstractEntity::newSpotLight (uint32_t shadowMapResolution, const std::string & componentName) noexcept
	{
		if ( !this->checkComponentNameAvailability(componentName) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::SpotLight >(componentName, *this, shadowMapResolution);

		/* Registering the smart pointer. */
		if ( !this->addComponent(componentName, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(SpotLightCreated, component);

		return component;
	}

	std::shared_ptr< Component::SoundEmitter >
	AbstractEntity::newSoundEmitter (const std::string & componentName) noexcept
	{
		if ( !this->checkComponentNameAvailability(componentName) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::SoundEmitter >(componentName, *this);

		/* Registering the smart pointer. */
		if ( !this->addComponent(componentName, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(SoundEmitterCreated, component);

		return component;
	}

	std::shared_ptr< Component::SoundEmitter >
	AbstractEntity::newSoundEmitter (const std::shared_ptr< Audio::SoundResource > & resource, float gain, bool loop, const std::string & componentName) noexcept
	{
		auto component = this->newSoundEmitter(componentName);

		component->play(resource, gain, loop);

		return component;
	}

	std::shared_ptr< Component::Visual >
	AbstractEntity::newVisual (const std::shared_ptr< Renderable::Interface > & resource, bool enablePhysicalProperties, bool enableLighting, const std::string & componentName) noexcept
	{
		/* If no name were passed, we use the resource name. */
		const auto name = componentName.empty() ? resource->name() : componentName;

		if ( !this->checkComponentNameAvailability(name) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::Visual >(name, *this, resource);
		component->enablePhysicalProperties(enablePhysicalProperties);

		if ( enableLighting )
		{
			component->getRenderableInstance()->enableLighting();
		}

		/* Registering the smart pointer. */
		if ( !this->addComponent(name, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(VisualComponentCreated, component);

		return component;
	}

	std::shared_ptr< Component::MultipleVisuals >
	AbstractEntity::newVisual (const std::shared_ptr< Renderable::Interface > & resource, const std::vector< CartesianFrame< float > > & coordinates, bool enablePhysicalProperties, bool enableLighting, const std::string & componentName) noexcept
	{
		/* If no name were passed, we use the mesh name. */
		const auto name = componentName.empty() ? resource->name() : componentName;

		if ( !this->checkComponentNameAvailability(name) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::MultipleVisuals >(name, *this, resource, coordinates);
		component->enablePhysicalProperties(enablePhysicalProperties);

		if ( enableLighting )
		{
			component->getRenderableInstance()->enableLighting();
		}

		/* Registering the smart pointer. */
		if ( !this->addComponent(name, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(MultipleVisualsComponentCreated, component);

		return component;
	}

	std::shared_ptr< Component::ParticlesEmitter >
	AbstractEntity::newParticlesEmitter (const std::shared_ptr< Renderable::SpriteResource > & resource, uint32_t maxParticleCount, const std::string & componentName) noexcept
	{
		/* If no name were passed, we use the resource name. */
		const auto name = componentName.empty() ? resource->name() : componentName;

		if ( !this->checkComponentNameAvailability(name) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::ParticlesEmitter >(name, *this, resource, maxParticleCount);

		/* Registering the smart pointer. */
		if ( !this->addComponent(name, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(ParticlesEmitterCreated, component);

		return component;
	}

	std::shared_ptr< Component::ParticlesEmitter >
	AbstractEntity::newParticlesEmitter (const std::shared_ptr< Renderable::MeshResource > & resource, uint32_t maxParticleCount, const std::string & componentName) noexcept
	{
		/* If no name were passed, we use the resource name. */
		const auto name = componentName.empty() ? resource->name() : componentName;

		if ( !this->checkComponentNameAvailability(name) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::ParticlesEmitter >(name, *this, resource, maxParticleCount);

		/* Registering the smart pointer. */
		if ( !this->addComponent(name, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(ParticlesEmitterCreated, component);

		return component;
	}

	std::shared_ptr< Component::DirectionalPushModifier >
	AbstractEntity::newDirectionalPushModifier (const std::string & componentName) noexcept
	{
		if ( !this->checkComponentNameAvailability(componentName) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::DirectionalPushModifier >(componentName, *this);

		/* Registering the smart pointer. */
		if ( !this->addComponent(componentName, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(ModifierCreated, std::static_pointer_cast< Component::AbstractModifier >(component));
		this->notify(DirectionalPushModifierCreated, component);

		return component;
	}

	std::shared_ptr< Component::SphericalPushModifier >
	AbstractEntity::newSphericalPushModifier (const std::string & componentName) noexcept
	{
		if ( !this->checkComponentNameAvailability(componentName) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::SphericalPushModifier >(componentName, *this);

		/* Registering the smart pointer. */
		if ( !this->addComponent(componentName, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(ModifierCreated, std::static_pointer_cast< Component::AbstractModifier >(component));
		this->notify(SphericalPushModifierCreated, component);

		return component;
	}

	std::shared_ptr< Component::Weight >
	AbstractEntity::newWeight (const PhysicalObjectProperties & initialProperties, const std::string & componentName) noexcept
	{
		if ( !this->checkComponentNameAvailability(componentName) )
		{
			return nullptr;
		}

		auto component = std::make_shared< Component::Weight >(componentName, *this, initialProperties);
		component->enablePhysicalProperties(true);

		/* Registering the smart pointer. */
		if ( !this->addComponent(componentName, component) )
		{
			return nullptr;
		}

		/* Notify the creation. */
		this->notify(WeightCreated, component);

		return component;
	}

	bool
	AbstractEntity::processLogics (const Scene & scene, size_t engineCycle) noexcept
	{
		/* Updates every entity at this Node. */
		auto componentIt = m_components.begin();

		while ( componentIt != m_components.end() )
		{
			if ( componentIt->second->shouldBeRemoved() )
			{
				TraceWarning{TracerTag} << "Removing automatically component '" << componentIt->second->name() << "' ...";

				this->unlinkComponent(componentIt->second);

				componentIt = m_components.erase(componentIt);
			}
			else
			{
				componentIt->second->processLogics(scene);

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
