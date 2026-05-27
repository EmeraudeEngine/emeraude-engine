/*
 * src/Physics/Particle.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <memory>

/* Local inclusions for inheritances. */
#include "Scenes/LocatableInterface.hpp"

/* Local inclusions for usages. */
#include "Graphics/Frustum.hpp"
#include "Scenes/Component/Abstract.hpp"
#include "Scenes/Component/AbstractModifier.hpp"

namespace EmEn::Physics
{
	/* Forward declarations. */
	class CollisionModelInterface;
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
			void initialize (uint32_t initialLife, float initialSize, float spreadingRadius = 0.0F, const Base::Math::CartesianFrame< float > & initialLocation = {}) noexcept;

			/** @copydoc EmEn::Scenes::LocatableInterface::setPosition(const Base::Math::Vector< 3, float > &, Base::Math::TransformSpace) */
			void setPosition (const Base::Math::Vector< 3, float > & position, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::setXPosition(float, Base::Math::TransformSpace) */
			void setXPosition (float position, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::setYPosition(float, Base::Math::TransformSpace) */
			void setYPosition (float position, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::setZPosition(float, Base::Math::TransformSpace) */
			void setZPosition (float position, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::move(const Base::Math::Vector< 3, float > &, Base::Math::TransformSpace) */
			void move (const Base::Math::Vector< 3, float > & distance, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::moveX(float, Base::Math::TransformSpace) */
			void moveX (float distance, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::moveY(float, Base::Math::TransformSpace) */
			void moveY (float distance, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::moveZ(float, Base::Math::TransformSpace) */
			void moveZ (float distance, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::rotate(float, const Base::Math::Vector< 3, float > &, Base::Math::TransformSpace) */
			void rotate (float radian, const Base::Math::Vector< 3, float > & axis, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::pitch(float, Base::Math::TransformSpace) */
			void pitch (float radian, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::yaw(float, Base::Math::TransformSpace) */
			void yaw (float radian, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::roll(float, Base::Math::TransformSpace) */
			void roll (float radian, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::setScalingFactor() */
			void setScalingFactor (const Base::Math::Vector< 3, float > & factor) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scale(const Base::Math::Vector< 3, float > &, Base::Math::TransformSpace) */
			void scale (const Base::Math::Vector< 3, float > & factor, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scale(float, Base::Math::TransformSpace) */
			void scale (float factor, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scaleX(float, Base::Math::TransformSpace) */
			void scaleX (float factor, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scaleY(float, Base::Math::TransformSpace) */
			void scaleY (float factor, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scaleZ(float, Base::Math::TransformSpace) */
			void scaleZ (float factor, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::lookAt(const Base::Math::Vector< 3, float > &, bool) */
			void
			lookAt (const Base::Math::Vector< 3, float > & target, bool flipZAxis) noexcept override
			{
				m_cartesianFrame.lookAt(target, flipZAxis);
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setLocalCoordinates(const Base::Math::CartesianFrame< float > &) */
			void
			setLocalCoordinates (const Base::Math::CartesianFrame< float > & coordinates) noexcept override
			{
				m_cartesianFrame = coordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() const */
			[[nodiscard]]
			const Base::Math::CartesianFrame< float > &
			localCoordinates () const noexcept override
			{
				return m_cartesianFrame;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() */
			[[nodiscard]]
			Base::Math::CartesianFrame< float > &
			localCoordinates () noexcept override
			{
				return m_cartesianFrame;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::getWorldCoordinates() const */
			[[nodiscard]]
			Base::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return m_cartesianFrame;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setCollisionModel(std::unique_ptr< CollisionModelInterface >) */
			void
			setCollisionModel (std::unique_ptr< CollisionModelInterface > /*model*/) noexcept override
			{
				/* Particles are points without collision model. */
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::hasCollisionModel() const */
			[[nodiscard]]
			bool
			hasCollisionModel () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::collisionModel() const */
			[[nodiscard]]
			const CollisionModelInterface *
			collisionModel () const noexcept override
			{
				return nullptr;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::collisionModel() */
			[[nodiscard]]
			CollisionModelInterface *
			collisionModel () noexcept override
			{
				return nullptr;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::isVisibleTo(const Graphics::Frustum &) const */
			[[nodiscard]]
			bool
			isVisibleTo (const Graphics::Frustum & frustum) const noexcept override
			{
				return frustum.isSeeing(m_cartesianFrame.position());
			}

			/**
			 * @brief Returns the current particle velocity.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			linearVelocity () const noexcept
			{
				return m_linearVelocity;
			}

			/**
			 * @brief Gives access to the velocity vector.
			 * @return Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > &
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
			 * @param scene A reference to the scene.
			 * @param particleProperties A reference to the physical object properties coming from the particle emitter.
			 * @param worldCoordinates A reference to a cartesian frame according to the particle emitter location.
			 * @return bool
			 */
			bool updateSimulation (const Scenes::Scene & scene, const BodyPhysicalProperties & particleProperties, const Base::Math::CartesianFrame< float > & worldCoordinates) noexcept;

			/**
			 * @brief Updates the particles position and properties.
			 * @param velocity A reference to a velocity vector.
			 * @param sizeDelta A value to add to the current size of the particle.
			 * @param chaos A chaos value to apply.
			 * @return void
			 */
			void update (const Base::Math::Vector< 3, float > & velocity, float sizeDelta, float chaos) noexcept;

		private:

			/**
			 * @brief Shifts the particle location randomly according to a magnitude.
			 * @param magnitude The magnitude value.
			 */
			void shiftLocation (float magnitude) noexcept;

			Base::Math::CartesianFrame< float > m_cartesianFrame;
			Base::Math::Vector< 3, float > m_linearVelocity;
			uint32_t m_lifetime{0};
			float m_size{1.0F};
	};
}
