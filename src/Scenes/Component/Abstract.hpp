/*
 * src/Scenes/Component/Abstract.hpp
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
#include <string>
#include <memory>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"
#include "Libs/FlagArrayTrait.hpp"
#include "Libs/ObservableTrait.hpp"
#include "Animations/AnimatableInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Physics/MovableTrait.hpp"
#include "CoreTypes.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics
	{
		namespace Renderable
		{
			class Interface;
		}

		namespace RenderableInstance
		{
			class Abstract;
		}
	}

	namespace Scenes
	{
		class Scene;
		class AbstractEntity;
	}
}

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Class that hold the base of every component that can be attached to an entity.
	 * @note [OBS][SHARED-OBSERVABLE]
	 * @extends EmEn::Libs::NameableTrait Each component is named.
	 * @extends EmEn::Libs::FlagArrayTrait Each component has 8 flags, 2 are used by this base class.
	 * @extends EmEn::Libs::ObservableTrait To transfer physical properties changes. FIXME: Observable is kept for future features.
	 * @extends Animations::AnimatableInterface Component are animatable.
	 */
	class Abstract : public Libs::NameableTrait, public Libs::FlagArrayTrait< 8 >, public Libs::ObservableTrait, public Animations::AnimatableInterface
	{
		public:

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				ComponentContentModified,
				MaxEnum
			};

			static constexpr Libs::Math::Space3D::AACuboid< float > NullBoundingBox{};
			static constexpr Libs::Math::Space3D::Sphere< float > NullBoundingSphere{};

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (Abstract && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (Abstract && copy) noexcept = delete;

			/**
			 * @brief Destructs the abstract entity component.
			 */
			~Abstract () override = default;

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				static const size_t classUID = EmEn::Libs::Hash::FNV1a("Component");

				return classUID;
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
			 * @brief Sets the physical properties application state.
			 * @note This affects the bounding primitives.
			 * @param state The state.
			 * @return void
			 */
			void
			enablePhysicalProperties (bool state) noexcept
			{
				this->setFlag(EnablePhysicalProperties, state);
			}

			/**
			 * @brief Returns whether the physical properties are disabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPhysicalPropertiesEnabled () const noexcept
			{
				return this->isFlagEnabled(EnablePhysicalProperties);
			}

			/**
			 * @brief Returns the entity where this entity is attached.
			 * @return const AbstractEntity &
			 */
			[[nodiscard]]
			const AbstractEntity &
			parentEntity () const noexcept
			{
				return m_parentEntity;
			}

			/**
			 * @brief Returns the engine context from the parent entity's scene.
			 * @return const EngineContext &
			 */
			[[nodiscard]]
			const EngineContext & engineContext () const noexcept;

			/**
			 * @brief Returns whether the parent entity has the movable trait.
			 * @return bool
			 */
			[[nodiscard]]
			bool isParentEntityMovable () const noexcept;

			/**
			 * @brief Returns true if the component is renderable.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRenderable () const noexcept
			{
				return this->getRenderableInstance() != nullptr;
			}

			/**
			 * @brief Returns the renderable if the component is visual.
			 * @warning Can be a null pointer!
			 * @return const Graphics::Renderable::Interface *
			 */
			[[nodiscard]]
			const Graphics::Renderable::Interface * getRenderable () const noexcept;

			/**
			 * @brief Returns physical properties of the component.
			 * @return const Physics::PhysicalObjectProperties &
			 */
			[[nodiscard]]
			const Physics::BodyPhysicalProperties &
			bodyPhysicalProperties () const noexcept
			{
				return m_bodyPhysicalProperties;
			}

			/**
			 * @brief Returns physical properties of the component.
			 * @return Physics::PhysicalObjectProperties &
			 */
			[[nodiscard]]
			Physics::BodyPhysicalProperties &
			bodyPhysicalProperties () noexcept
			{
				return m_bodyPhysicalProperties;
			}

			/**
			 * @brief Returns the absolute coordinates of this component using the parent node.
			 * @return Libs::Math::Coordinates< float >
			 */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float > getWorldCoordinates () const noexcept;

			/**
			 * @brief Returns the absolute velocity of this component using the parent node.
			 * @return Libs::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > getWorldVelocity () const noexcept;

			/**
			 * @brief Initializes that entity from JSON rules.
			 * @param jsonData A native json value from project JsonCpp.
			 * @return bool
			 */
			virtual bool initialize (const Json::Value & jsonData) noexcept;

			/**
			 * @brief Returns the renderable if the component is visual.
			 * @warning Can be a null pointer!
			 * @return std::shared_ptr< Graphics::RenderableInstance::Abstract >
			 */
			[[nodiscard]]
			virtual
			std::shared_ptr< Graphics::RenderableInstance::Abstract >
			getRenderableInstance () const noexcept
			{
				return nullptr;
			}

			/**
			 * @brief Returns the local bounding box of this component.
			 * @note Can be invalid. On a non-overridden method, this will return a null bounding box.
			 * @return const Libs::Math::Space3D::AACuboid< float > &
			 */
			[[nodiscard]]
			virtual
			const Libs::Math::Space3D::AACuboid< float > &
			localBoundingBox () const noexcept
			{
				return NullBoundingBox;
			}

			/**
			 * @brief Returns the local bounding sphere of this component.
			 * @note Can be invalid. On non-overridden method, this will return a null bounding sphere.
			 * @return const Libs::Math::Space3D::Sphere< float > &
			 */
			[[nodiscard]]
			virtual
			const Libs::Math::Space3D::Sphere< float > &
			localBoundingSphere () const noexcept
			{
				return NullBoundingSphere;
			}

			/**
			 * @brief Returns the type of component.
			 * @return const char *
			 */
			[[nodiscard]]
			virtual const char * getComponentType () const noexcept = 0;

			/**
			 * @brief Checks the type of the current component.
			 * @param classID A C-string.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isComponent (const char * classID) const noexcept = 0;

			/**
			 * @brief This method is called when the entity is updated by the core logic every cycle.
			 * @param scene A reference to the scene.
			 * @return void
			 */
			virtual void processLogics (const Scene & scene) noexcept = 0;

			/**
			 * @brief This method is called from the entity holding this component.
			 * @note This can be called by the Abstract::processLogics() method.
			 * @param worldCoordinates A reference to the world coordinates.
			 * @return void
			 */
			virtual void move (const Libs::Math::CartesianFrame< float > & worldCoordinates) noexcept = 0;

			/**
			 * @brief Tells the entity to remove this component.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool shouldBeRemoved () const noexcept = 0;

		protected:

			/**
			 * @brief Called when the scene is suspended (disabled).
			 *
			 * Components that manage pooled resources (audio sources, etc.)
			 * should release them here and remember their state for onWakeup().
			 *
			 * @note Called by AbstractEntity::suspend().
			 * @see onWakeup() To restore activity.
			 * @version 0.8.35
			 */
			virtual void onSuspend () noexcept = 0;

			/**
			 * @brief Called when the scene wakes up (re-enabled).
			 *
			 * Components should reacquire pooled resources and restore
			 * their state from before onSuspend().
			 *
			 * @note Called by AbstractEntity::wakeup().
			 * @see onSuspend() To release resources.
			 * @version 0.8.35
			 */
			virtual void onWakeup () noexcept = 0;

			/* Flag names */
			static constexpr auto EnablePhysicalProperties{0UL};
			static constexpr auto UnusedFlag{1UL};

			/**
			 * @brief Constructs an abstract entity component.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 */
			Abstract (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
				: NameableTrait{componentName},
				m_parentEntity{parentEntity}
			{

			}

		private:

			/* Allow AbstractEntity to call onSuspend()/onWakeup() on components. */
			friend class Scenes::AbstractEntity;

			const AbstractEntity & m_parentEntity;
			Physics::BodyPhysicalProperties m_bodyPhysicalProperties;
	};
}
