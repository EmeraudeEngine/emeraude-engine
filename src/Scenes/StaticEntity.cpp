/*
 * src/Scenes/StaticEntity.cpp
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

#include "StaticEntity.hpp"

/* Local inclusions. */
#include "Scenes/Scene.hpp"

namespace EmEn::Scenes
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Physics;

	const size_t StaticEntity::ClassUID{getClassUID(ClassId)};

	StaticEntity::StaticEntity (const std::string & name, uint32_t sceneTimeMS, const CartesianFrame< float > & coordinates) noexcept
		: AbstractEntity(name, sceneTimeMS), m_cartesianFrame(coordinates)
	{

	}

	void
	StaticEntity::scale (const Vector< 3, float > & factor, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingFactor(factor);
				break;

			case TransformSpace::Parent :
			case TransformSpace::World :
				/* TODO: Reorient the scale vector to world. */
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	StaticEntity::scale (float factor, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingFactor(factor);
				break;

			case TransformSpace::Parent :
			case TransformSpace::World :
				/* TODO: Reorient the scale vector to world. */
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	StaticEntity::scaleX (float factor, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingXFactor(factor);
				break;

			case TransformSpace::Parent :
			case TransformSpace::World :
				/* TODO: Reorient the scale vector to world. */
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	StaticEntity::scaleY (float factor, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingYFactor(factor);
				break;

			case TransformSpace::Parent :
			case TransformSpace::World :
				/* TODO: Reorient the scale vector to world. */
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	StaticEntity::scaleZ (float factor, TransformSpace transformSpace) noexcept
	{
		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingZFactor(factor);
				break;

			case TransformSpace::Parent :
			case TransformSpace::World :
				/* TODO: Reorient the scale vector to world. */
				break;
		}

		this->onLocationDataUpdate();
	}

	bool
	StaticEntity::onProcessLogics (const Scene & scene) noexcept
	{
		this->updateAnimations(scene.cycle());

		return false;
	}

	void
	StaticEntity::onContentModified () noexcept
	{
		this->notify(EntityContentModified, this->shared_from_this());
	}

	bool
	StaticEntity::onUnhandledNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable->is(PhysicalObjectProperties::ClassUID) )
		{
			return true;
		}

		/* NOTE: Don't know what is it, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable '" << whoIs(observable->classUID()) << "' (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
	}

	bool
	StaticEntity::playAnimation (uint8_t animationID, const Variant & value, size_t /*cycle*/) noexcept
	{
		switch ( animationID )
		{
			case LocalCoordinates :
				this->setLocalCoordinates(value.asCartesianFrameFloat());
				return true;

			case LocalPosition :
				this->setPosition(value.asVector3Float(), TransformSpace::Local);
				return true;

			case LocalXPosition :
				this->setXPosition(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalYPosition :
				this->setYPosition(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalZPosition :
				this->setZPosition(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalTranslation :
				this->move(value.asVector3Float(), TransformSpace::Local);
				return true;

			case LocalXTranslation :
				this->moveX(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalYTranslation :
				this->moveY(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalZTranslation :
				this->moveZ(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalRotation :
				// ...
				return true;

			case LocalXRotation :
				this->pitch(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalYRotation :
				this->yaw(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalZRotation :
				this->roll(value.asFloat(), TransformSpace::Local);
				return true;

			case WorldPosition :
				this->setPosition(value.asVector3Float(), TransformSpace::World);
				return true;

			case WorldXPosition :
				this->setXPosition(value.asFloat(), TransformSpace::World);
				return true;

			case WorldYPosition :
				this->setYPosition(value.asFloat(), TransformSpace::World);
				return true;

			case WorldZPosition :
				this->setZPosition(value.asFloat(), TransformSpace::World);
				return true;

			case WorldTranslation :
				this->move(value.asVector3Float(), TransformSpace::World);
				return true;

			case WorldXTranslation :
				this->moveX(value.asFloat(), TransformSpace::World);
				return true;

			case WorldYTranslation :
				this->moveY(value.asFloat(), TransformSpace::World);
				return true;

			case WorldZTranslation :
				this->moveZ(value.asFloat(), TransformSpace::World);
				return true;

			case WorldRotation :
				// ...
				return true;

			case WorldXRotation :
				this->pitch(value.asFloat(), TransformSpace::World);
				return true;

			case WorldYRotation :
				this->yaw(value.asFloat(), TransformSpace::World);
				return true;

			case WorldZRotation :
				this->roll(value.asFloat(), TransformSpace::World);
				return true;

			default:
				break;
		}

		return false;
	}
}
