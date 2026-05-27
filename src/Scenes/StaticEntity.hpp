/*
 * src/Scenes/StaticEntity.hpp
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


/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <any>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

/* Local inclusions for inheritances. */
#include "AbstractEntity.hpp"
#include "Animations/AnimatableInterface.hpp"

/* Local inclusions for usages. */
#include "Graphics/Frustum.hpp"
#include "Math/OrientedCuboid.hpp"
#include "Variant.hpp"

namespace EmEn::Scenes
{
	/**
	 * @class StaticEntity
	 * @brief Represents a static (non-movable) entity in the scene that can be transformed but has no physics simulation.
	 *
	 * A StaticEntity is designed for scene objects that need spatial positioning and orientation but do not require
	 * physics simulation or hierarchical parent-child relationships. These entities are ideal for static scenery,
	 * decorations, landmarks, and other non-interactive environmental elements.
	 *
	 * Key characteristics:
	 * - No physics simulation (hasMovableAbility() returns false)
	 * - No hierarchical children support (unlike Node class)
	 * - Can be transformed (position, rotation, scale)
	 * - Supports animation through AnimatableInterface
	 * - Maintains separate logic and render state coordinates for thread-safe rendering
	 * - Optimized for objects that never move via physics
	 *
	 * Use cases:
	 * - Static environment geometry (buildings, terrain features, rocks)
	 * - Decorative elements (statues, signs, vegetation)
	 * - Non-interactive scenery elements
	 * - Objects that can be animated but don't require physics
	 *
	 * @extends std::enable_shared_from_this A static entity needs to self-replicate its smart pointer.
	 * @extends EmEn::Scenes::AbstractEntity A static entity is an entity of the 3D world.
	 * @extends EmEn::Animations::AnimatableInterface This class can be animated by the engine logics.
	 *
	 * @see Node For entities that support hierarchical parent-child relationships
	 * @see AbstractEntity For the base entity interface
	 *
	 * @version 0.8.35
	 */
	class StaticEntity final : public std::enable_shared_from_this< StaticEntity >, public AbstractEntity, public Animations::AnimatableInterface
	{
		public:

			/** @brief Class identifier string for debugging and type identification. */
			static constexpr auto ClassId{"StaticEntity"};

			/**
			 * @enum AnimationID
			 * @brief Animation channel identifiers for the AnimatableInterface.
			 *
			 * These identifiers specify which property of the static entity should be animated.
			 * Supports both local and world space transformations for position, translation, and rotation.
			 *
			 * @see EmEn::Animations::AnimatableInterface
			 */
			enum AnimationID : uint8_t
			{
				LocalCoordinates,	 ///< Full local coordinate frame
				LocalPosition,		///< Local space position (3D vector)
				LocalXPosition,	   ///< Local space X-axis position
				LocalYPosition,	   ///< Local space Y-axis position
				LocalZPosition,	   ///< Local space Z-axis position
				LocalTranslation,	 ///< Local space translation offset (3D vector)
				LocalXTranslation,	///< Local space X-axis translation
				LocalYTranslation,	///< Local space Y-axis translation
				LocalZTranslation,	///< Local space Z-axis translation
				LocalRotation,		///< Local space rotation (quaternion or euler)
				LocalXRotation,	   ///< Local space X-axis rotation (pitch)
				LocalYRotation,	   ///< Local space Y-axis rotation (yaw)
				LocalZRotation,	   ///< Local space Z-axis rotation (roll)

				WorldPosition,		///< World space position (3D vector)
				WorldXPosition,	   ///< World space X-axis position
				WorldYPosition,	   ///< World space Y-axis position
				WorldZPosition,	   ///< World space Z-axis position
				WorldTranslation,	 ///< World space translation offset (3D vector)
				WorldXTranslation,	///< World space X-axis translation
				WorldYTranslation,	///< World space Y-axis translation
				WorldZTranslation,	///< World space Z-axis translation
				WorldRotation,		///< World space rotation (quaternion or euler)
				WorldXRotation,	   ///< World space X-axis rotation (pitch)
				WorldYRotation,	   ///< World space Y-axis rotation (yaw)
				WorldZRotation		///< World space Z-axis rotation (roll)
			};

			/**
			 * @brief Constructs a static entity.
			 *
			 * Creates a new static entity at the specified coordinates within the scene.
			 * The entity is initialized with separate logic and render state coordinate systems
			 * to support thread-safe rendering.
			 *
			 * @param scene A reference to the scene this entity belongs to.
			 * @param name A reference to a string to name the entity in the scene.
			 * @param sceneTimeMS The scene current time in milliseconds.
			 * @param coordinates The initial local coordinate frame of the static entity. Defaults to origin.
			 */
			StaticEntity (const Scene & scene, const std::string & name, uint32_t sceneTimeMS, const Base::Math::CartesianFrame< float > & coordinates = {}) noexcept
				: AbstractEntity{scene, name, sceneTimeMS},
				m_logicStateCoordinates{coordinates}
			{

			}

			/**
			 * @brief Copy constructor (deleted).
			 *
			 * StaticEntity instances cannot be copied to maintain unique ownership semantics
			 * and prevent issues with scene graph integrity.
			 *
			 * @param copy A reference to the copied instance.
			 */
			StaticEntity (const StaticEntity & copy) noexcept = delete;

			/**
			 * @brief Move constructor (deleted).
			 *
			 * StaticEntity instances cannot be moved to maintain unique ownership semantics
			 * and prevent issues with scene graph integrity.
			 *
			 * @param copy A reference to the copied instance.
			 */
			StaticEntity (StaticEntity && copy) noexcept = delete;

			/**
			 * @brief Copy assignment operator (deleted).
			 *
			 * StaticEntity instances cannot be copy-assigned to maintain unique ownership semantics
			 * and prevent issues with scene graph integrity.
			 *
			 * @param copy A reference to the copied instance.
			 * @return StaticEntity &
			 */
			StaticEntity & operator= (const StaticEntity & copy) noexcept = delete;

			/**
			 * @brief Move assignment operator (deleted).
			 *
			 * StaticEntity instances cannot be move-assigned to maintain unique ownership semantics
			 * and prevent issues with scene graph integrity.
			 *
			 * @param copy A reference to the copied instance.
			 * @return StaticEntity &
			 */
			StaticEntity & operator= (StaticEntity && copy) noexcept = delete;

			/**
			 * @brief Destructs the static entity.
			 *
			 * Cleans up all resources associated with this static entity.
			 */
			~StaticEntity () override = default;

			/** @copydoc EmEn::Scenes::LocatableInterface::setPosition(const Base::Math::Vector< 3, float > &, Base::Math::TransformSpace) */
			void
			setPosition (const Base::Math::Vector< 3, float > & position, Base::Math::TransformSpace transformSpace) noexcept override
			{
				if ( transformSpace == Base::Math::TransformSpace::Local )
				{
					m_logicStateCoordinates.setPosition(m_logicStateCoordinates.getRotationMatrix3() * position);
				}
				else
				{
					m_logicStateCoordinates.setPosition(position);
				}

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setXPosition(float, Base::Math::TransformSpace) */
			void
			setXPosition (float position, Base::Math::TransformSpace transformSpace) noexcept override
			{
				if ( transformSpace == Base::Math::TransformSpace::Local )
				{
					m_logicStateCoordinates.setPosition(m_logicStateCoordinates.rightVector() * position);
				}
				else
				{
					m_logicStateCoordinates.setXPosition(position);
				}

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setYPosition(float, Base::Math::TransformSpace) */
			void
			setYPosition (float position, Base::Math::TransformSpace transformSpace) noexcept override
			{
				if ( transformSpace == Base::Math::TransformSpace::Local )
				{
					m_logicStateCoordinates.setPosition(m_logicStateCoordinates.downwardVector() * position);
				}
				else
				{
					m_logicStateCoordinates.setYPosition(position);
				}

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setZPosition(float, Base::Math::TransformSpace) */
			void
			setZPosition (float position, Base::Math::TransformSpace transformSpace) noexcept override
			{
				if ( transformSpace == Base::Math::TransformSpace::Local )
				{
					m_logicStateCoordinates.setPosition(m_logicStateCoordinates.backwardVector() * position);
				}
				else
				{
					m_logicStateCoordinates.setZPosition(position);
				}

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::move(const Base::Math::Vector< 3, float > &, Base::Math::TransformSpace) */
			void
			move (const Base::Math::Vector< 3, float > & distance, Base::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.translate(distance, transformSpace == Base::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::moveX(float, Base::Math::TransformSpace) */
			void
			moveX (float distance, Base::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.translateX(distance, transformSpace == Base::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::moveY(float, Base::Math::TransformSpace) */
			void
			moveY (float distance, Base::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.translateY(distance, transformSpace == Base::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::moveZ(float, Base::Math::TransformSpace) */
			void
			moveZ (float distance, Base::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.translateZ(distance, transformSpace == Base::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::rotate(float, const Base::Math::Vector< 3, float > &, Base::Math::TransformSpace) */
			void
			rotate (float radian, const Base::Math::Vector< 3, float > & axis, Base::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.rotate(radian, axis, transformSpace == Base::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::pitch(float, Base::Math::TransformSpace) */
			void
			pitch (float radian, Base::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.pitch(radian, transformSpace == Base::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::yaw(float, Base::Math::TransformSpace) */
			void
			yaw (float radian, Base::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.yaw(radian, transformSpace == Base::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::roll(float, Base::Math::TransformSpace) */
			void
			roll (float radian, Base::Math::TransformSpace transformSpace) noexcept override
			{
				m_logicStateCoordinates.roll(radian, transformSpace == Base::Math::TransformSpace::Local);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setScalingFactor() */
			void setScalingFactor (const Base::Math::Vector< 3, float > & factor) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::scale(const Base::Math::Vector< 3, float > &, Base::Math::TransformSpace)
			 * @warning TransformSpace::Parent and TransformSpace::World are not yet implemented for scaling.
			 *		  Only TransformSpace::Local is currently supported. Using other transform spaces will
			 *		  have no effect on the entity.
			 * @todo Reorient the scale vector to world coordinates for Parent and World transform spaces.
			 */
			void scale (const Base::Math::Vector< 3, float > & factor, Base::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::scale(float, Base::Math::TransformSpace)
			 * @warning TransformSpace::Parent and TransformSpace::World are not yet implemented for scaling.
			 *		  Only TransformSpace::Local is currently supported. Using other transform spaces will
			 *		  have no effect on the entity.
			 * @todo Reorient the scale vector to world coordinates for Parent and World transform spaces.
			 */
			void scale (float factor, Base::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::scaleX(float, Base::Math::TransformSpace)
			 * @warning TransformSpace::Parent and TransformSpace::World are not yet implemented for scaling.
			 *		  Only TransformSpace::Local is currently supported. Using other transform spaces will
			 *		  have no effect on the entity.
			 * @todo Reorient the scale vector to world coordinates for Parent and World transform spaces.
			 */
			void scaleX (float factor, Base::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::scaleY(float, Base::Math::TransformSpace)
			 * @warning TransformSpace::Parent and TransformSpace::World are not yet implemented for scaling.
			 *		  Only TransformSpace::Local is currently supported. Using other transform spaces will
			 *		  have no effect on the entity.
			 * @todo Reorient the scale vector to world coordinates for Parent and World transform spaces.
			 */
			void scaleY (float factor, Base::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::scaleZ(float, Base::Math::TransformSpace)
			 * @warning TransformSpace::Parent and TransformSpace::World are not yet implemented for scaling.
			 *		  Only TransformSpace::Local is currently supported. Using other transform spaces will
			 *		  have no effect on the entity.
			 * @todo Reorient the scale vector to world coordinates for Parent and World transform spaces.
			 */
			void scaleZ (float factor, Base::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::lookAt(const Base::Math::Vector< 3, float > &, bool) */
			void
			lookAt (const Base::Math::Vector< 3, float > & target, bool flipZAxis) noexcept override
			{
				m_logicStateCoordinates.lookAt(target, flipZAxis);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setLocalCoordinates(const Base::Math::CartesianFrame< float > &) */
			void
			setLocalCoordinates (const Base::Math::CartesianFrame< float > & coordinates) noexcept override
			{
				m_logicStateCoordinates = coordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() const */
			[[nodiscard]]
			const Base::Math::CartesianFrame< float > &
			localCoordinates () const noexcept override
			{
				return m_logicStateCoordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() */
			[[nodiscard]]
			Base::Math::CartesianFrame< float > &
			localCoordinates () noexcept override
			{
				return m_logicStateCoordinates;
			}

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::getWorldCoordinates() const
			 * @note For StaticEntity, world coordinates are the same as local coordinates since static entities have no parent hierarchy.
			 */
			[[nodiscard]]
			Base::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return m_logicStateCoordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::isVisibleTo(const Graphics::Frustum &) const */
			[[nodiscard]]
			bool
			isVisibleTo (const Graphics::Frustum & frustum) const noexcept override
			{
				if ( !this->hasCollisionModel() )
				{
					/* No collision model: use point visibility (position only). */
					return frustum.isSeeing(m_logicStateCoordinates.position());
				}

				/* Use AABB from collision model for frustum culling. */
				const auto worldAABB = this->collisionModel()->getAABB(m_logicStateCoordinates);

				return frustum.isSeeing(worldAABB);
			}

			/**
			 * @brief Returns the unique identifier for this class.
			 *
			 * Computes a hash of the class identifier string using the FNV1a algorithm.
			 * This value is cached statically for efficient repeated access.
			 *
			 * @return size_t The unique class identifier hash.
			 *
			 * @note Thread-safe due to static local variable initialization guarantees in C++11 and later.
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::hasMovableAbility() const
			 * @return Always returns false for StaticEntity as it has no physics simulation.
			 */
			[[nodiscard]]
			bool
			hasMovableAbility () const noexcept override
			{
				return false;
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::isMoving() const
			 * @return Always returns false for StaticEntity as it has no physics simulation.
			 */
			[[nodiscard]]
			bool
			isMoving () const noexcept override
			{
				return false;
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::publishStateForRendering(uint32_t)
			 * @note Publishes the current logic state coordinates to the specified render state buffer for thread-safe access by the rendering system.
			 */
			void
			publishStateForRendering (uint32_t writeStateIndex) noexcept override
			{
				if constexpr ( IsDebug )
				{
					if ( writeStateIndex >= m_renderStateCoordinates.size() ) [[unlikely]]
					{
						Tracer::error(ClassId, "Index overflow !");

						return;
					}
				}

				m_renderStateCoordinates[writeStateIndex] = m_logicStateCoordinates;
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::getWorldCoordinatesStateForRendering(uint32_t) const
			 * @note Retrieves the coordinates from the specified render state buffer for rendering.
			 */
			[[nodiscard]]
			const Base::Math::CartesianFrame< float > &
			getWorldCoordinatesStateForRendering (uint32_t readStateIndex) const noexcept override
			{
				return m_renderStateCoordinates[readStateIndex];
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::getMovableTrait()
			 * @return Always returns nullptr for StaticEntity as it has no physics trait.
			 */
			[[nodiscard]]
			Physics::MovableTrait *
			getMovableTrait () noexcept override
			{
				return nullptr;
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::getMovableTrait() const
			 * @return Always returns nullptr for StaticEntity as it has no physics trait.
			 */
			[[nodiscard]]
			const Physics::MovableTrait *
			getMovableTrait () const noexcept override
			{
				return nullptr;
			}

			/**
			 * @brief Returns the model transformation matrix.
			 *
			 * Computes the 4x4 model matrix that transforms from local object space to world space.
			 * This matrix includes position, rotation, and scaling transformations.
			 *
			 * @return Base::Math::Matrix< 4, float > The 4x4 model transformation matrix.
			 *
			 * @see Base::Math::CartesianFrame::getModelMatrix()
			 */
			[[nodiscard]]
			Base::Math::Matrix< 4, float >
			getModelMatrix () const noexcept
			{
				return m_logicStateCoordinates.getModelMatrix();
			}

			/**
			 * @brief Returns the view transformation matrix.
			 *
			 * Computes the 4x4 view matrix that transforms from world space to camera/view space.
			 * For StaticEntity, this is derived directly from the local coordinates since there
			 * is no parent hierarchy.
			 *
			 * @return Base::Math::Matrix< 4, float > The 4x4 view transformation matrix.
			 *
			 * @see Base::Math::CartesianFrame::getViewMatrix()
			 */
			[[nodiscard]]
			Base::Math::Matrix< 4, float >
			getViewMatrix () const noexcept
			{
				return m_logicStateCoordinates.getViewMatrix();
			}

			/**
			 * @brief Returns the infinity view transformation matrix.
			 *
			 * Computes the 4x4 view matrix with translation removed, commonly used for skyboxes
			 * and other objects that should appear infinitely far away. The rotation is preserved
			 * while the position component is zeroed.
			 *
			 * @return Base::Math::Matrix< 4, float > The 4x4 infinity view transformation matrix.
			 *
			 * @see Base::Math::CartesianFrame::getInfinityViewMatrix()
			 */
			[[nodiscard]]
			Base::Math::Matrix< 4, float >
			getInfinityViewMatrix () const noexcept
			{
				return m_logicStateCoordinates.getInfinityViewMatrix();
			}

		private:

			/** @copydoc EmEn::Scenes::AbstractEntity::onUnhandledNotification() */
			bool onUnhandledNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::onLocationDataUpdate()
			 * @note Dispatches location changes to all attached components.
			 */
			void
			onLocationDataUpdate () noexcept override
			{
				/* Dispatch the movement to every component of the node. */
				this->onContainerMove(m_logicStateCoordinates);
			}

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Base::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Scenes::AbstractEntity::onProcessLogics() */
			bool onProcessLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::AbstractEntity::onContentModified() */
			void onContentModified () noexcept override;

			/**
			 * @brief Current coordinate frame used by the logic system.
			 *
			 * Stores the position, rotation, and scale of the static entity in the scene.
			 * This is the authoritative state modified by transformation methods and used
			 * during logic updates.
			 */
			Base::Math::CartesianFrame< float > m_logicStateCoordinates;

			/**
			 * @brief Double-buffered coordinate frames for thread-safe rendering.
			 *
			 * Maintains two copies of the coordinate frame to allow the logic system and
			 * rendering system to operate independently without locking. The logic system
			 * publishes its state to one buffer while the renderer reads from the other.
			 */
			std::array< Base::Math::CartesianFrame< float >, 2 > m_renderStateCoordinates{};
	};
}
