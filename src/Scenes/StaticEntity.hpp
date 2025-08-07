/*
 * src/Scenes/StaticEntity.hpp
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
#include <cstdint>
#include <any>
#include <string>
#include <array>

/* Local inclusions for inheritances. */
#include "AbstractEntity.hpp"
#include "Animations/AnimatableInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/OrientedCuboid.hpp"
#include "Libs/Variant.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief Defines a static entity, which means that it does not physically act on its own.
	 * @extends std::enable_shared_from_this A static entity needs to self-replicate its smart pointer.
	 * @extends EmEn::Scenes::AbstractEntity A static entity is an entity of the 3D world.
	 * @extends EmEn::Animations::AnimatableInterface This class can be animated by the engine logics.
	 */
	class StaticEntity final : public std::enable_shared_from_this< StaticEntity >, public AbstractEntity, public Animations::AnimatableInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"StaticEntity"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/** @brief Animatable Interface key. */
			enum AnimationID : uint8_t
			{
				LocalCoordinates,
				LocalPosition,
				LocalXPosition,
				LocalYPosition,
				LocalZPosition,
				LocalTranslation,
				LocalXTranslation,
				LocalYTranslation,
				LocalZTranslation,
				LocalRotation,
				LocalXRotation,
				LocalYRotation,
				LocalZRotation,

				WorldPosition,
				WorldXPosition,
				WorldYPosition,
				WorldZPosition,
				WorldTranslation,
				WorldXTranslation,
				WorldYTranslation,
				WorldZTranslation,
				WorldRotation,
				WorldXRotation,
				WorldYRotation,
				WorldZRotation
			};

			/**
			 * @brief Constructs a static entity.
			 * @param name A reference to a string to name the entity in the scene.
			 * @param sceneTimeMS The scene current time in milliseconds.
			 * @param coordinates The location of the static entity. Default origin.
			 */
			StaticEntity (const std::string & name, uint32_t sceneTimeMS, const Libs::Math::CartesianFrame< float > & coordinates = {}) noexcept
				: AbstractEntity{name, sceneTimeMS},
				m_logicStateCoordinates{coordinates}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			StaticEntity (const StaticEntity & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			StaticEntity (StaticEntity && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return StaticEntity &
			 */
			StaticEntity & operator= (const StaticEntity & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return StaticEntity &
			 */
			StaticEntity & operator= (StaticEntity && copy) noexcept = delete;

			/**
			 * @brief Destructs the static entity.
			 */
			~StaticEntity () override = default;

			/** @copydoc EmEn::Scenes::LocatableInterface::setPosition(const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace) */
			void
			setPosition (const Libs::Math::Vector< 3, float > & position, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				if ( transformSpace == Libs::Math::TransformSpace::Local )
				{
					m_logicStateCoordinates.setPosition(m_logicStateCoordinates.getRotationMatrix3() * position);
				}
				else
				{
					m_logicStateCoordinates.setPosition(position);
				}

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setXPosition(float, Libs::Math::TransformSpace) */
			void
			setXPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				if ( transformSpace == Libs::Math::TransformSpace::Local )
				{
					m_logicStateCoordinates.setPosition(m_logicStateCoordinates.rightVector() * position);
				}
				else
				{
					m_logicStateCoordinates.setXPosition(position);
				}

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setYPosition(float, Libs::Math::TransformSpace) */
			void
			setYPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				if ( transformSpace == Libs::Math::TransformSpace::Local )
				{
					m_logicStateCoordinates.setPosition(m_logicStateCoordinates.downwardVector() * position);
				}
				else
				{
					m_logicStateCoordinates.setYPosition(position);
				}

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setZPosition(float, Libs::Math::TransformSpace) */
			void
			setZPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				if ( transformSpace == Libs::Math::TransformSpace::Local )
				{
					m_logicStateCoordinates.setPosition(m_logicStateCoordinates.backwardVector() * position);
				}
				else
				{
					m_logicStateCoordinates.setZPosition(position);
				}

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::move(const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace) */
			void
			move (const Libs::Math::Vector< 3, float > & distance, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.translate(distance, transformSpace == Libs::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::moveX(float, Libs::Math::TransformSpace) */
			void
			moveX (float distance, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.translateX(distance, transformSpace == Libs::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::moveY(float, Libs::Math::TransformSpace) */
			void
			moveY (float distance, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.translateY(distance, transformSpace == Libs::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::moveZ(float, Libs::Math::TransformSpace) */
			void
			moveZ (float distance, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.translateZ(distance, transformSpace == Libs::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::rotate(float, const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace) */
			void
			rotate (float radian, const Libs::Math::Vector< 3, float > & axis, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.rotate(radian, axis, transformSpace == Libs::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::pitch(float, Libs::Math::TransformSpace) */
			void
			pitch (float radian, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.pitch(radian, transformSpace == Libs::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::yaw(float, Libs::Math::TransformSpace) */
			void
			yaw (float radian, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.yaw(radian, transformSpace == Libs::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::roll(float, Libs::Math::TransformSpace) */
			void
			roll (float radian, Libs::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.roll(radian, transformSpace == Libs::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

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
				m_logicStateCoordinates.lookAt(target, flipZAxis);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setLocalCoordinates(const Libs::Math::CartesianFrame< float > &) */
			void
			setLocalCoordinates (const Libs::Math::CartesianFrame< float > & coordinates) noexcept override
			{
				m_logicStateCoordinates = coordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() const */
			[[nodiscard]]
			const Libs::Math::CartesianFrame< float > &
			localCoordinates () const noexcept override
			{
				return m_logicStateCoordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float > &
			localCoordinates () noexcept override
			{
				return m_logicStateCoordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::getWorldCoordinates() const */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return m_logicStateCoordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::getWorldBoundingBox() const */
			[[nodiscard]]
			Libs::Math::Space3D::AACuboid< float >
			getWorldBoundingBox () const noexcept override
			{
				return Libs::Math::OrientedCuboid< float >{this->localBoundingBox(), m_logicStateCoordinates}.getAxisAlignedBox();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::getWorldBoundingSphere() const */
			[[nodiscard]]
			Libs::Math::Space3D::Sphere< float >
			getWorldBoundingSphere () const noexcept override
			{
				return {
					this->localBoundingSphere().radius(),
					m_logicStateCoordinates.position() + this->localBoundingSphere().position()
				};
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::enableSphereCollision(bool) */
			void
			enableSphereCollision (bool state) noexcept override
			{
				this->setFlag(SphereCollisionEnabled, state);
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::sphereCollisionIsEnabled() const */
			[[nodiscard]]
			bool
			sphereCollisionIsEnabled () const noexcept override
			{
				return this->isFlagEnabled(SphereCollisionEnabled);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

			/** @copydoc EmEn::Scenes::AbstractEntity::hasMovableAbility() const */
			[[nodiscard]]
			bool
			hasMovableAbility () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::AbstractEntity::isMoving() const */
			[[nodiscard]]
			bool
			isMoving () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Scenes::AbstractEntity::publishStateForRendering(uint32_t) */
			void
			publishStateForRendering (uint32_t writeStateIndex) noexcept override
			{
				if constexpr ( IsDebug )
				{
					if ( writeStateIndex >= m_renderStateCoordinates.size() )
					{
						Tracer::error(ClassId, "Index overflow !");

						return;
					}
				}

				m_renderStateCoordinates[writeStateIndex] = m_logicStateCoordinates;
			}

			/** @copydoc EmEn::Scenes::AbstractEntity::getWorldCoordinatesStateForRendering(uint32_t) const */
			[[nodiscard]]
			const Libs::Math::CartesianFrame< float > &
			getWorldCoordinatesStateForRendering (uint32_t readStateIndex) const noexcept override
			{
				return m_renderStateCoordinates[readStateIndex];
			}

			/** @copydoc EmEn::Scenes::AbstractEntity::getMovableTrait() */
			[[nodiscard]]
			Physics::MovableTrait *
			getMovableTrait () noexcept override
			{
				return nullptr;
			}

			/** @copydoc EmEn::Scenes::AbstractEntity::getMovableTrait() const */
			[[nodiscard]]
			const Physics::MovableTrait *
			getMovableTrait () const noexcept override
			{
				return nullptr;
			}

			/**
			 * @brief Returns the coordinates of the static renderable.
			 * @return Libs::Math::Matrix< 4, float >
			 */
			[[nodiscard]]
			Libs::Math::Matrix< 4, float >
			getModelMatrix () const noexcept
			{
				return m_logicStateCoordinates.getModelMatrix();
			}

			/**
			 * @brief Shorthand for Coordinates class, but use the tree to get the right view matrix.
			 * @return Matrix< 4, float >
			 */
			[[nodiscard]]
			Libs::Math::Matrix< 4, float >
			getViewMatrix () const noexcept
			{
				return m_logicStateCoordinates.getViewMatrix();
			}

			/**
			 * @brief Shorthand for Coordinates class, but use the tree to get the right infinity view matrix.
			 * @return Matrix< 4, float >
			 */
			[[nodiscard]]
			Libs::Math::Matrix< 4, float >
			getInfinityViewMatrix () const noexcept
			{
				return m_logicStateCoordinates.getInfinityViewMatrix();
			}

		private:

			/** @copydoc EmEn::Scenes::AbstractEntity::onUnhandledNotification() */
			bool onUnhandledNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/** @copydoc EmEn::Scenes::AbstractEntity::onLocationDataUpdate() */
			void
			onLocationDataUpdate () noexcept override
			{
				/* Dispatch the movement to every component of the node. */
				this->onContainerMove(m_logicStateCoordinates);
			}

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Scenes::AbstractEntity::onProcessLogics() */
			bool onProcessLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::AbstractEntity::onContentModified() */
			void onContentModified () noexcept override;

			Libs::Math::CartesianFrame< float > m_logicStateCoordinates;
			std::array< Libs::Math::CartesianFrame< float >, 2 > m_renderStateCoordinates{};
	};
}
