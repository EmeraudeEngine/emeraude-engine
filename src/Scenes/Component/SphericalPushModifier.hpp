/*
 * src/Scenes/Component/SphericalPushModifier.hpp
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
#include <cstdint>
#include <string>

/* Local inclusions for inheritances. */
#include "AbstractModifier.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Defines a spherical push modifier to make move physical object.
	 * @extends EmEn::Scenes::Component::AbstractModifier The base class for modifier.
	 */
	class SphericalPushModifier final : public AbstractModifier
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SphericalPushModifier"};

			/**
			 * @brief Constructs a spherical push modifier.
			 * @param name The name of the modifier.
			 * @param parentEntity A reference to the parent entity.
			 */
			SphericalPushModifier (const std::string & name, const AbstractEntity & parentEntity) noexcept;

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

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void move (const Libs::Math::CartesianFrame< float > & worldCoordinates) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::Component::AbstractModifier::getForceAppliedToEntity(const Libs::Math::CartesianFrame< float > &, const Libs::Math::Space3D::Sphere< float > &) */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > getForceAppliedToEntity (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Space3D::Sphere< float > & worldBoundingSphere) const noexcept override;

			/** @copydoc EmEn::Scenes::Component::AbstractModifier::getForceAppliedToEntity(const Libs::Math::CartesianFrame< float > &, const Libs::Math::Space3D::AACuboid< float > &) */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > getForceAppliedToEntity (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Space3D::AACuboid< float > & worldBoundingBox) const noexcept override;

			/**
			 * @brief Sets the constant push magnitude.
			 * @param magnitude The value
			 * @return void
			 */
			void
			setMagnitude (float magnitude) noexcept
			{
				m_magnitude = std::abs(magnitude);
			}

			/**
			 * @brief Returns the constant push magnitude.
			 * @return float
			 */
			[[nodiscard]]
			float
			magnitude () const noexcept
			{
				return m_magnitude;
			}

		private:

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			float m_magnitude{1.0F};
	};
}
