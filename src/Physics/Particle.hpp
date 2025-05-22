/*
 * src/Physics/Particle.hpp
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

/* STL inclusions */
#include <cstdint>
#include <set>
#include <memory>

/* Local inclusions for inheritances. */
#include "Scenes/LocatableInterface.hpp"

/* Local inclusions. */
#include "Scenes/Component/Abstract.hpp"
#include "Scenes/Component/AbstractModifier.hpp"
#include "PhysicalEnvironmentProperties.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief The particle class.
	 * @extends EmEn::Scenes::LocatableInterface A particle is locatable in the scene.
	 */
	class Particle final : public Scenes::LocatableInterface
	{
		public:

			/**
			 * @brief Constructs a particle.
			 * @note Particle is dead by default.
			 */
			Particle () = default;

			/**
			 * @brief Initializes the particle.
			 * @param initialLife The initial life of the particle.
			 * @param initialSize The initial size of the particle.
			 * @param spreadingRadius The spreading parameters. Default 0.0.
			 * @param initialLocation A reference to a cartesian frame. Default origin.
			 * return void
			 */
			void initialize (uint32_t initialLife, float initialSize, float spreadingRadius = 0.0F, const Libs::Math::CartesianFrame< float > & initialLocation = {}) noexcept;

			/** @copydoc EmEn::Scenes::LocatableInterface::setPosition(const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace) */
			void setPosition (const Libs::Math::Vector< 3, float > & position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::setXPosition(float, Libs::Math::TransformSpace) */
			void setXPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::setYPosition(float, Libs::Math::TransformSpace) */
			void setYPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::setZPosition(float, Libs::Math::TransformSpace) */
			void setZPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::move(const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace) */
			void move (const Libs::Math::Vector< 3, float > & distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::moveX(float, Libs::Math::TransformSpace) */
			void moveX (float distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::moveY(float, Libs::Math::TransformSpace) */
			void moveY (float distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::moveZ(float, Libs::Math::TransformSpace) */
			void moveZ (float distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::rotate(float, const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace) */
			void rotate (float radian, const Libs::Math::Vector< 3, float > & axis, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::pitch(float, Libs::Math::TransformSpace) */
			void pitch (float radian, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::yaw(float, Libs::Math::TransformSpace) */
			void yaw (float radian, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::roll(float, Libs::Math::TransformSpace) */
			void roll (float radian, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scale(const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace) */
			void scale (const Libs::Math::Vector< 3, float > & factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scale(float, Libs::Math::TransformSpace) */
			void scale (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scaleX(float, Libs::Math::TransformSpace) */
			void scaleX (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scaleY(float, Libs::Math::TransformSpace) */
			void scaleY (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scaleZ(float, Libs::Math::TransformSpace) */
			void scaleZ (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::lookAt(const Libs::Math::Vector< 3, float > &, bool) */
			void
			lookAt (const Libs::Math::Vector< 3, float > & target, bool flipZAxis) noexcept override
			{
				m_cartesianFrame.lookAt(target, flipZAxis);
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setLocalCoordinates(const Libs::Math::CartesianFrame< float > &) */
			void
			setLocalCoordinates (const Libs::Math::CartesianFrame< float > & coordinates) noexcept override
			{
				m_cartesianFrame = coordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() const */
			[[nodiscard]]
			const Libs::Math::CartesianFrame< float > &
			localCoordinates () const noexcept override
			{
				return m_cartesianFrame;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float > &
			localCoordinates () noexcept override
			{
				return m_cartesianFrame;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::getWorldCoordinates() const */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return m_cartesianFrame;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localBoundingBox() const */
			[[nodiscard]]
			const Libs::Math::Space3D::AACuboid< float > &
			localBoundingBox () const noexcept override
			{
				return Scenes::Component::Abstract::NullBoundingBox;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::getWorldBoundingBox() const */
			[[nodiscard]]
			Libs::Math::Space3D::AACuboid< float >
			getWorldBoundingBox () const noexcept override
			{
				return Scenes::Component::Abstract::NullBoundingBox;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localBoundingSphere() const */
			[[nodiscard]]
			const Libs::Math::Space3D::Sphere< float > &
			localBoundingSphere () const noexcept override
			{
				return Scenes::Component::Abstract::NullBoundingSphere;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::getWorldBoundingSphere() const */
			[[nodiscard]]
			Libs::Math::Space3D::Sphere< float >
			getWorldBoundingSphere () const noexcept override
			{
				return Scenes::Component::Abstract::NullBoundingSphere;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::enableSphereCollision(bool) */
			void
			enableSphereCollision (bool /*state*/) noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::LocatableInterface::sphereCollisionIsEnabled() const */
			[[nodiscard]]
			bool
			sphereCollisionIsEnabled () const noexcept override
			{
				return true;
			}

			/**
			 * @brief Returns the current particle velocity.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			linearVelocity () const noexcept
			{
				return m_linearVelocity;
			}

			/**
			 * @brief Gives access to the velocity vector.
			 * @return Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > &
			linearVelocity () noexcept
			{
				return m_linearVelocity;
			}

			/**
			 * @brief Returns the linear speed in meters per second.
			 * @note Computed on the fly.
			 * @return float
			 */
			[[nodiscard]]
			float
			linearSpeed () const noexcept
			{
				return m_linearVelocity.length();
			}

			/**
			 * @brief Sets the lifetime of the particle in milliseconds.
			 * @note If you set zero, the particle will be considered as dead.
			 * @param life The new life of the particle.
			 * @return void
			 */
			void
			setLifetime (uint32_t life) noexcept
			{
				m_lifetime = life;
			}

			/**
			 * @brief Returns the remaining lifetime of the particle in milliseconds.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			remainingLifetime () const noexcept
			{
				return m_lifetime;
			}

			/**
			 * @brief Sets the size of the particle.
			 * @param size The new size of the particle.
			 * @return void
			 */
			void
			setSize (float size) noexcept
			{
				m_size = std::abs(size);
			}

			/**
			 * @brief Returns the current size of the particle.
			 * @return size_t
			 */
			[[nodiscard]]
			float
			size () const noexcept
			{
				return m_size;
			}

			/**
			 * @brief Returns whether the particle is dead or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDead () const noexcept
			{
				return m_lifetime == 0;
			}

			/**
			 * @brief Updates the particle physics simulation.
			 * @note This is the default engine behavior.
			 * @param envProperties A reference to the physical environment properties coming from the scene.
			 * @param particleProperties A reference to the physical object properties coming from the particle emitter.
			 * @param worldCoordinates A reference to a cartesian frame according to the particle emitter location.
			 * @param modifiers A reference to a vector of scene modifiers.
			 * @return bool
			 */
			bool updateSimulation (const PhysicalEnvironmentProperties & envProperties, const PhysicalObjectProperties & particleProperties, const Libs::Math::CartesianFrame< float > & worldCoordinates, const std::set< std::shared_ptr< Scenes::Component::AbstractModifier > > & modifiers) noexcept;

			/**
			 * @brief Updates the particles position and properties.
			 * @param velocity A reference to a velocity vector.
			 * @param sizeDelta A value to add to the current size of the particle.
			 * @param chaos A chaos value to apply.
			 * @return void
			 */
			void update (const Libs::Math::Vector< 3, float > & velocity, float sizeDelta, float chaos) noexcept;

		private:

			/**
			 * @brief Shifts the particle location randomly according to a magnitude.
			 * @param magnitude The magnitude value.
			 */
			void shiftLocation (float magnitude) noexcept;

			Libs::Math::CartesianFrame< float > m_cartesianFrame;
			Libs::Math::Vector< 3, float > m_linearVelocity;
			uint32_t m_lifetime{0};
			float m_size{1.0F};
	};
}
