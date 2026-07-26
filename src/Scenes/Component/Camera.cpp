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
 * https://github.com/EmeraudeEngine/emeraude-engine
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
	using namespace Base;
	using namespace Base::Math;
	using namespace Animations;
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
			targetDevice.updateVideoDeviceProperties(this->fieldOfView(), m_distance, false);
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
				output->updateVideoDeviceProperties(this->fieldOfView(), m_distance, false);
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
	Camera::setPerspectiveProjection (float distance) noexcept
	{
		this->enableFlag(PerspectiveProjection);

		/* NOTE: No field of view here. The framing belongs to the optics and the lens already
		 * mounted keeps it, which is exactly what one wants when coming back from an orthographic
		 * projection: the focus and framing are found unchanged. */
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
	Camera::setFocalLength (float millimeters) noexcept
	{
		/* THE single framing writer. The field of view is derived from this and the sensor
		 * (`fieldOfView()`), so there is nothing to keep in sync — the previous design stored both
		 * and had FOUR writers, one of which (setPerspectiveProjection) updated only the angle and
		 * left a stale focal length behind: the panel then reported a lens that did not match the
		 * image. A one-millimeter floor keeps the derived angle below 171 degrees, which also
		 * replaces the old clamp that allowed a geometrically meaningless 360. */
		m_focalLength = std::max(millimeters, 1.0F);

		if ( this->hasOutputConnected() && this->isPerspectiveProjection() )
		{
			this->updateAllVideoDeviceProperties();
		}
	}

	void
	Camera::setSensorWidth (float millimeters) noexcept
	{
		if ( this->isTechnicalCamera() )
		{
			TraceWarning{ClassId} << "Camera '" << this->name() << "' is a technical camera: its "
				"field of view is a geometric constraint, so the sensor format is locked. Ignoring "
				"the " << millimeters << " mm request.";

			return;
		}

		m_sensorWidth = std::max(1.0F, millimeters);

		/* CONSTANT LENS: the focal length is untouched, so the derived field of view changes —
		 * the physical crop factor. Only the render targets need to hear about it. */
		if ( this->hasOutputConnected() && this->isPerspectiveProjection() )
		{
			this->updateAllVideoDeviceProperties();
		}
	}

	void
	Camera::setTechnicalFieldOfView (float degrees) noexcept
	{
		this->enableFlag(TechnicalProjection);

		const auto clamped = std::clamp(std::abs(degrees), 1.0F, 179.0F);

		/* Expressed as the focal length that yields the angle on the current sensor, so the single
		 * source of truth still holds; the format is locked from here on so nothing can reframe it
		 * behind the caller's back. 90 degrees on a 24 mm-high sensor is exactly 12 mm. */
		m_focalLength = this->sensorHeight() / (2.0F * std::tan(Radian(clamped) * 0.5F));

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
	Camera::enableDepthOfField (bool state) noexcept
	{
		if ( this->isFlagEnabled(DepthOfFieldEnabled) == state )
		{
			return;
		}

		this->setFlag(DepthOfFieldEnabled, state);

		/* The scene observes its active camera and (de)materializes the effect. */
		this->notify(PhysicalEffectsToggled);
	}

	void
	Camera::enableHDR (bool state) noexcept
	{
		if ( this->isFlagEnabled(HDREnabled) == state )
		{
			return;
		}

		this->setFlag(HDREnabled, state);

		/* The scene observes its active camera and (de)materializes the effect. */
		this->notify(PhysicalEffectsToggled);
	}

	void
	Camera::enableBloom (bool state) noexcept
	{
		if ( this->isFlagEnabled(BloomEnabled) == state )
		{
			return;
		}

		this->setFlag(BloomEnabled, state);

		/* The scene observes its active camera and (de)materializes the effect. */
		this->notify(PhysicalEffectsToggled);
	}

	void
	Camera::addLensEffect (const std::shared_ptr< DirectPostProcessEffect > & effect) noexcept
	{
		/* We don't want to notify an effect twice. */
		if ( this->isLensEffectPresent(effect) )
		{
			return;
		}

		m_lensEffects.emplace_back(effect);

		this->notify(LensEffectsChanged);
	}

	void
	Camera::removeLensEffect (const std::shared_ptr< DirectPostProcessEffect > & effect) noexcept
	{
		const auto lensIt = std::find(m_lensEffects.cbegin(), m_lensEffects.cend(), effect);

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
			case Distance  :
				this->setDistance(value.asFloat());
				return true;

			case Aperture :
				this->setAperture(value.asFloat());
				return true;

			case FocalLength :
				this->setFocalLength(value.asFloat());
				return true;

			/* NOTE: Animating the focus = a focus pull; it implies manual focus. */
			case FocusDistance :
				this->setFocusDistance(value.asFloat());
				return true;

			case ExposureCompensation :
				this->setExposureCompensation(value.asFloat());
				return true;

			default:
				return false;
		}
	}
}
