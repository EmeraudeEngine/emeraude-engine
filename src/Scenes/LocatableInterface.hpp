/*
 * src/Scenes/LocatableInterface.hpp
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

/* Local inclusions for usages. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The LocatableInterface class is used to locate something in the 3D world and set its coordinates.
	 */
	class LocatableInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			LocatableInterface (const LocatableInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			LocatableInterface (LocatableInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return LocatableInterface &
			 */
			LocatableInterface & operator= (const LocatableInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return LocatableInterface &
			 */
			LocatableInterface & operator= (LocatableInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the locatable interface.
			 */
			virtual ~LocatableInterface () = default;

			/**
			 * @brief Sets an absolute position in meters.
			 * @param position A reference to a vector.
			 * @param transformSpace The space where occurs the move.
			 * @return void
			 */
			virtual void setPosition (const Libs::Math::Vector< 3, float > & position, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Sets an absolute X position in meters.
			 * @param position The position on X axis.
			 * @param transformSpace The space where occurs the move.
			 * @return void
			 */
			virtual void setXPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Sets an absolute Y position in meters.
			 * @param position The position on Y axis.
			 * @param transformSpace The space where occurs the move.
			 * @return void
			 */
			virtual void setYPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Sets an absolute Z position in meters.
			 * @param position The position on Z axis.
			 * @param transformSpace The space where occurs the move.
			 * @return void
			 */
			virtual void setZPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Shifts the position in meters.
			 * @param distance A reference to a vector.
			 * @param transformSpace The space where occurs the move.
			 * @return void
			 */
			virtual void move (const Libs::Math::Vector< 3, float > & distance, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Shifts the X position in meters.
			 * @param distance The distance on X axis.
			 * @param transformSpace The space where occurs the move.
			 * @return void
			 */
			virtual void moveX (float distance, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Shifts the Y position in meters.
			 * @param distance The distance on Y axis.
			 * @param transformSpace The space where occurs the move.
			 * @return void
			 */
			virtual void moveY (float distance, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Shifts the Z position in meters.
			 * @param distance The distance on Z axis.
			 * @param transformSpace The space where occurs the move.
			 * @return void
			 */
			virtual void moveZ (float distance, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Performs a rotation around an arbitrary axis.
			 * @param radian The angle expressed in radians. You can use the function Math::Radians(degree).
			 * @param axis A vector to define the axis.
			 * @param transformSpace The space where occurs the rotation.
			 * @return void
			 */
			virtual void rotate (float radian, const Libs::Math::Vector< 3, float > & axis, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Performs a rotation around the X axis.
			 * @param radian The angle expressed in radians. You can use the function Math::Radians(degree).
			 * @param transformSpace The space where occurs the rotation.
			 * @return void
			 */
			virtual void pitch (float radian, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Performs a rotation around the Y axis.
			 * @param radian The angle expressed in radians. You can use the function Math::Radians(degree).
			 * @param transformSpace The space where occurs the rotation.
			 * @return void
			 */
			virtual void yaw (float radian, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Performs a rotation around the Z axis.
			 * @param radian The angle expressed in radians. You can use the function Math::Radians(degree).
			 * @param transformSpace The space where occurs the rotation.
			 * @return void
			 */
			virtual void roll (float radian, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Applies a scale factor on the coordinates.
			 * @param factor A reference to a scale vector.
			 * @param transformSpace The space where occurs the scale.
			 * @return void
			 */
			virtual void scale (const Libs::Math::Vector< 3, float > & factor, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Applies a scale factor on the coordinates.
			 * @param factor an unidirectional factor value.
			 * @param transformSpace The space where occurs the scale.
			 * @return void
			 */
			virtual void scale (float factor, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Applies a scale factor on the coordinates.
			 * @param factor an unidirectional factor value.
			 * @param transformSpace The space where occurs the scale.
			 * @return void
			 */
			virtual void scaleX (float factor, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Applies a scale factor on the coordinates.
			 * @param factor an unidirectional factor value.
			 * @param transformSpace The space where occurs the scale.
			 * @return void
			 */
			virtual void scaleY (float factor, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Applies a scale factor on the coordinates.
			 * @param factor an unidirectional factor value.
			 * @param transformSpace The space where occurs the scale.
			 * @return void
			 */
			virtual void scaleZ (float factor, Libs::Math::TransformSpace transformSpace) noexcept = 0;

			/**
			 * @brief Points the backward (Z- axis) of the coordinates system toward the target.
			 * @note Use this for pointing a camera.
			 * @param target A position to look at.
			 * @param flipZAxis Flip the final Z axis.
			 * @return void
			 */
			virtual void lookAt (const Libs::Math::Vector< 3, float > & target, bool flipZAxis) noexcept = 0;

			/**
			 * @brief Sets the local coordinates of this entity.
			 * @return Libs::Math::Coordinates< float > &
			 */
			virtual void setLocalCoordinates (const Libs::Math::CartesianFrame< float > & coordinates) noexcept = 0;

			/**
			 * @brief Returns the local coordinates of a 3D world entity.
			 * @return const Libs::Math::Coordinates< float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::CartesianFrame< float > & localCoordinates () const noexcept = 0;

			/**
			 * @brief Returns the internal local coordinates of a 3D world entity.
			 * @return const Libs::Math::Coordinates< float > &
			 */
			[[nodiscard]]
			virtual Libs::Math::CartesianFrame< float > & localCoordinates () noexcept = 0;

			/**
			 * @brief Returns the world coordinates of a 3D world entity.
			 * @return Libs::Math::Coordinates< float >
			 */
			[[nodiscard]]
			virtual Libs::Math::CartesianFrame< float > getWorldCoordinates () const noexcept = 0;

			/**
			 * @brief Returns the world bounding box of a 3D world entity.
			 * @return const Libs::Math::Space3D::AACuboid< float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Space3D::AACuboid< float > & localBoundingBox () const noexcept = 0;

			/**
			 * @brief Returns the world bounding box of a 3D world entity.
			 * @return Libs::Math::Space3D::AACuboid< float >
			 */
			[[nodiscard]]
			virtual Libs::Math::Space3D::AACuboid< float > getWorldBoundingBox () const noexcept = 0;

			/**
			 * @brief Returns the world bounding sphere of a 3D world entity.
			 * @return const Libs::Math::Space3D::Sphere< float > &
			 */
			[[nodiscard]]
			virtual const Libs::Math::Space3D::Sphere< float > & localBoundingSphere () const noexcept = 0;

			/**
			 * @brief Returns the world bounding sphere of a 3D world entity.
			 * @return Libs::Math::Space3D::Sphere< float >
			 */
			[[nodiscard]]
			virtual Libs::Math::Space3D::Sphere< float > getWorldBoundingSphere () const noexcept = 0;

			/**
			 * @brief Enables the sphere collision simplification.
			 * @return state The state.
			 */
			virtual void enableSphereCollision (bool state) noexcept = 0;

			/**
			 * @brief Returns whether the sphere collision is enabled or not.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool sphereCollisionIsEnabled () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a locatable interface.
			 */
			LocatableInterface () noexcept = default;
	};
}
