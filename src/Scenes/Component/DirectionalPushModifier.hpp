/*
 * src/Scenes/Component/DirectionalPushModifier.hpp
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

/* Local inclusions for inheritances. */
#include "AbstractModifier.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Defines a directional push modifier to make move physical object.
	 * @extends EmEn::Scenes::Component::AbstractModifier The base class for modifier.
	 */
	class DirectionalPushModifier final : public AbstractModifier
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DirectionalPushModifier"};

			/**
			 * @brief Constructs a directional push modifier.
			 * @param name The name of the modifier.
			 * @param parentEntity A reference to the parent entity.
			 */
			DirectionalPushModifier (const std::string & name, const AbstractEntity & parentEntity) noexcept;

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
			 * @brief Sets the push magnitude.
			 * @param magnitude A arbitrary value.
			 * @return void
			 */
			void
			setMagnitude (float magnitude) noexcept
			{
				m_magnitude = magnitude;
			}

			/**
			 * @brief Returns the push magnitude.
			 * @return float
			 */
			[[nodiscard]]
			float
			magnitude () const noexcept
			{
				return m_magnitude;
			}

			/**
			 * @brief Sets a custom push direction.
			 * @param direction A reference to a vector.
			 * @return void
			 */
			void setCustomDirection (const Libs::Math::Vector< 3, float > & direction) noexcept;

			/**
			 * @brief Disables the custom push direction.
			 * @return void
			 */
			void disableCustomDirection () noexcept;

			/**
			 * @brief Returns the push direction.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			direction () const noexcept
			{
				return m_direction;
			}

		private:

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/* Flag names */
			static constexpr auto UseCoordinatesDirection{UnusedFlag + 0UL};

			float m_magnitude{1.0F};
			Libs::Math::Vector< 3, float > m_direction;
	};
}
