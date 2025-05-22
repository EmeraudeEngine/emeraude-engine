/*
 * src/Scenes/Component/Camera.cpp
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

#include "Camera.hpp"

/* STL inclusions. */
#include <cmath>

namespace EmEn::Scenes::Component
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Animations;
	using namespace Saphir;
	using namespace Graphics;

	Camera::Camera (const std::string & name, const AbstractEntity & parentEntity, bool perspective) noexcept
		: Abstract(name, parentEntity),
		AbstractVirtualDevice(name, AVConsole::DeviceType::Video, AVConsole::ConnexionType::Output)
	{
		this->setFlag(PerspectiveProjection, perspective);
	}

	void
	Camera::setPerspectiveProjection (float fov, float maxViewableDistance) noexcept
	{
		this->enableFlag(PerspectiveProjection);

		m_fov = std::abs(fov);

		if ( maxViewableDistance > 0.0F )
		{
			m_distance = maxViewableDistance;
		}

		this->updateProperties(true, m_distance, m_fov);
	}

	void
	Camera::setOrthographicProjection (float size) noexcept
	{
		this->disableFlag(PerspectiveProjection);

		m_distance = std::abs(size);

		this->updateProperties(false, m_distance, 0.0F);
	}

	void
	Camera::setFieldOfView (float degrees) noexcept
	{
		m_fov = std::min(std::abs(degrees), FullRevolution< float >);

		if ( this->isPerspectiveProjection() )
		{
			this->updateProperties(true, m_distance, m_fov);
		}
	}

	void
	Camera::setDistance (float distance) noexcept
	{
		m_distance = std::abs(distance);

		if ( this->isPerspectiveProjection() )
		{
			this->updateProperties(true, m_distance, m_fov);
		}
		else
		{
			this->updateProperties(false, m_distance, 0.0F);
		}
	}

	void
	Camera::addLensEffect (const std::shared_ptr< FramebufferEffectInterface > & effect) noexcept
	{
		/* We don't want to notify an effect twice. */
		if ( this->isLensEffectPresent(effect) )
		{
			return;
		}

		m_lensEffects.emplace(effect);

		this->notify(LensEffectsChanged);
	}

	void
	Camera::removeLensEffect (const std::shared_ptr< FramebufferEffectInterface > & effect) noexcept
	{
		const auto lensIt = m_lensEffects.find(effect);

		if (lensIt == m_lensEffects.cend() )
		{
			return;
		}

		m_lensEffects.erase(lensIt);

		this->notify(LensEffectsChanged);
	}

	void
	Camera::clearLensEffects () noexcept
	{
		if ( m_lensEffects.empty() )
		{
			return;
		}

		m_lensEffects.clear();

		this->notify(LensEffectsChanged);
	}

	void
	Camera::move (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		this->updateDeviceFromCoordinates(worldCoordinates, this->getWorldVelocity());
	}

	void
	Camera::processLogics (const Scene & /*scene*/) noexcept
	{
		this->updateDeviceFromCoordinates(this->getWorldCoordinates(), this->getWorldVelocity());
	}

	void
	Camera::updateDeviceFromCoordinates (const CartesianFrame< float > & worldCoordinates, const Vector< 3, float > & worldVelocity) noexcept
	{
		if ( !this->hasOutputConnected() )
		{
			return;
		}

		for ( const auto & output : this->outputs() )
		{
			output->updateDeviceFromCoordinates(worldCoordinates, worldVelocity);
		}
	}

	void
	Camera::updateProperties (bool isPerspectiveProjection, float distance, float fovOrNear) noexcept
	{
		if ( !this->hasOutputConnected() )
		{
			return;
		}

		for ( const auto & output : this->outputs() )
		{
			output->updateProperties(isPerspectiveProjection, distance, fovOrNear);
		}
	}

	void
	Camera::onTargetConnected (AbstractVirtualDevice * targetDevice) noexcept
	{
		/* Initialize the target device with coordinates and camera properties. */
		targetDevice->updateDeviceFromCoordinates(this->getWorldCoordinates(), this->getWorldVelocity());

		if ( this->isPerspectiveProjection() )
		{
			this->updateProperties(true, m_distance, m_fov);
		}
		else
		{
			this->updateProperties(false, m_distance, 0.0F);
		}
	}

	bool
	Camera::playAnimation (uint8_t animationID, const Variant & value, size_t /*cycle*/) noexcept
	{
		switch ( animationID )
		{
			case FieldOfView :
				this->setFieldOfView(value.asFloat());
				return true;

			case Distance  :
				this->setDistance(value.asFloat());
				return true;

			default:
				return false;
		}
	}

	std::ostream &
	operator<< (std::ostream & out, const Camera & obj)
	{
		const auto coordinates = obj.getWorldCoordinates();
		const auto velocity = obj.getWorldVelocity();

		return out <<
			"Video Listener information" "\n"
			"Position: " << coordinates.position() << "\n"
			"Forward: " << coordinates.forwardVector() << "\n"
			"Velocity: " << velocity << "\n"
			"Field of view: " << obj.fieldOfView() << "\n"
			"Size of view: " << obj.distance() << "\n";
	}

	std::string
	to_string (const Camera & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
