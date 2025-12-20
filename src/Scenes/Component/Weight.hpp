/*
 * src/Scenes/Component/Weight.hpp
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

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Physics/BodyPhysicalProperties.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Dummy entity to add artificial physics properties to a Node.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 */
	class Weight final : public Abstract
	{
		public:

			/** @brief Class identifier */
			static constexpr auto ClassId{"Weight"};

			/** @brief Animatable Interface key. */
			enum AnimationID : uint8_t
			{
				Mass,
				Surface,
				DragCoefficient,
				AngularDragCoefficient,
				Bounciness,
				Stickiness
			};

			/**
			 * @brief Constructs a weight component to artificially physical properties to a scene node.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 */
			Weight (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
				: Abstract{componentName, parentEntity}
			{

			}

			/**
			 * @brief Constructs a weight component to artificially physical properties to a scene node.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param initialProperties Set initial physical properties.
			 */
			Weight (const std::string & componentName, const AbstractEntity & parentEntity, const Physics::BodyPhysicalProperties & initialProperties) noexcept
				: Abstract{componentName, parentEntity}
			{
				this->bodyPhysicalProperties().setProperties(initialProperties);
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::getComponentType() */
			[[nodiscard]]
			const char *
			getComponentType () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::isComponent() */
			[[nodiscard]]
			bool
			isComponent (const char * classID) const noexcept override
			{
				return strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::localBoundingBox() const */
			[[nodiscard]]
			const Libs::Math::Space3D::AACuboid< float > &
			localBoundingBox () const noexcept override
			{
				return m_boundingBox;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::localBoundingSphere() const */
			[[nodiscard]]
			const Libs::Math::Space3D::Sphere< float > &
			localBoundingSphere () const noexcept override
			{
				return m_boundingSphere;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void
			move (const Libs::Math::CartesianFrame< float > & /*worldCoordinates*/) noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

			/**
			 * @brief Set the bounding sphere radius.
			 * @param radius The radius.
			 * @return void
			 */
			void
			setRadius (float radius) noexcept
			{
				m_boundingSphere.setRadius(radius);

				this->notify(ComponentContentModified);
			}

			/**
			 * @brief Set the bounding box size.
			 * @param size The unilateral size.
			 * @return void
			 */
			void
			setBoxSize (float size) noexcept
			{
				m_boundingBox.set(size * 0.5F);

				this->notify(ComponentContentModified);
			}

			/**
			 * @brief Set the bounding box size.
			 * @param xSize The X-axis size.
			 * @param ySize The Y-axis size.
			 * @param zSize The Z-axis size.
			 * @return void
			 */
			void
			setBoxSize (float xSize, float ySize, float zSize) noexcept
			{
				m_boundingBox.set({xSize * 0.5F, ySize * 0.5F, zSize * 0.5F}, {-xSize * 0.5F, -ySize * 0.5F, -zSize * 0.5F});

				this->notify(ComponentContentModified);
			}

		private:

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void onSuspend () noexcept override { }

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void onWakeup () noexcept override { }

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			Libs::Math::Space3D::AACuboid< float > m_boundingBox;
			Libs::Math::Space3D::Sphere< float > m_boundingSphere;
	};
}
