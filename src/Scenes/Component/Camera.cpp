/*
 * src/Scenes/Component/Camera.cpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Camera.hpp"

/* STL inclusions. */
#include <cmath>

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Animations;
	using namespace Saphir;
	using namespace Graphics;

	void
	Camera::updateDeviceFromCoordinates (const CartesianFrame< float > & worldCoordinates, const Vector< 3, float > & worldVelocity) noexcept
	{
		if ( !this->hasOutputConnected() )
		{
			return;
		}

		/* NOTE: We send the new camera coordinates to update the matrices of render targets. */
		this->forEachOutputs([&worldCoordinates, &worldVelocity] (const auto & output) {
			output->updateDeviceFromCoordinates(worldCoordinates, worldVelocity);
		});
	}

	void
	Camera::onOutputDeviceConnected (EngineContext & /*engineContext*/, AbstractVirtualDevice & targetDevice) noexcept
	{
		/* When a new render target is connected, we initialize it with coordinates and camera properties. */
		if ( this->isPerspectiveProjection() )
		{
			targetDevice.updateVideoDeviceProperties(m_fov, m_distance, false);
		}
		else
		{
			targetDevice.updateVideoDeviceProperties(m_near, m_far, true);
		}

		targetDevice.updateDeviceFromCoordinates(this->getWorldCoordinates(), this->getWorldVelocity());
	}

	void
	Camera::updateAllVideoDeviceProperties () const noexcept
	{
		if ( this->isPerspectiveProjection() )
		{
			this->forEachOutputs([&] (const auto & output) {
				output->updateVideoDeviceProperties(m_fov, m_distance, false);
			});
		}
		else
		{
			this->forEachOutputs([&] (const auto & output) {
				output->updateVideoDeviceProperties(m_near, m_far, true);
			});
		}
	}

	void
	Camera::setPerspectiveProjection (float fov, float distance) noexcept
	{
		this->enableFlag(PerspectiveProjection);

		m_fov = std::min(std::abs(fov), FullRevolution< float >);

		if ( distance >= 0.0F )
		{
			m_distance = distance;
		}

		/* Update existing connected render targets. */
		if ( this->hasOutputConnected() )
		{
			this->updateAllVideoDeviceProperties();
		}
	}

	void
	Camera::setFieldOfView (float degrees) noexcept
	{
		m_fov = std::min(std::abs(degrees), FullRevolution< float >);

		/* Update existing connected render targets only if perspective projection is enabled. */
		if ( this->hasOutputConnected() && this->isPerspectiveProjection() )
		{
			this->updateAllVideoDeviceProperties();
		}
	}

	void
	Camera::setDistance (float distance) noexcept
	{
		if ( distance >= 0.0F )
		{
			m_distance = distance;
		}

		/* Update existing connected render targets (only if perspective projection is enabled). */
		if ( this->hasOutputConnected() && this->isPerspectiveProjection() )
		{
			this->updateAllVideoDeviceProperties();
		}
	}

	void
	Camera::setOrthographicProjection (float near, float far) noexcept
	{
		this->disableFlag(PerspectiveProjection);

		m_near = std::min(0.0F, near);
		m_far = std::max(0.0F, far);

		/* Update existing connected render targets. */
		if ( this->hasOutputConnected() )
		{
			this->updateAllVideoDeviceProperties();
		}
	}

	void
	Camera::setNear (float distance) noexcept
	{
		m_near = std::min(0.0F, distance);

		/* Update existing connected render targets (only for orthographic projection is enabled). */
		if ( this->hasOutputConnected() && this->isOrthographicProjection() )
		{
			this->updateAllVideoDeviceProperties();
		}
	}

	void
	Camera::setFar (float distance) noexcept
	{
		m_far = std::max(0.0F, distance);

		/* Update existing connected render targets (only for orthographic projection is enabled). */
		if ( this->hasOutputConnected() && this->isOrthographicProjection() )
		{
			this->updateAllVideoDeviceProperties();
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

		if ( lensIt == m_lensEffects.cend() )
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
}
