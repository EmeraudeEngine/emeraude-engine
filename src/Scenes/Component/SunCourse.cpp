/*
 * src/Scenes/Component/SunCourse.cpp
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

#include "SunCourse.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <numbers>

/* Local inclusions. */
#include "Constants.hpp"
#include "Graphics/Photometry.hpp"
#include "Math/Base.hpp"
#include "Math/CartesianFrame.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/Node.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;

	void
	SunCourse::bind (const std::shared_ptr< Node > & pivot, const std::shared_ptr< DirectionalLight > & light) noexcept
	{
		if ( pivot == nullptr || light == nullptr )
		{
			TraceError{ClassId} << "The sun course '" << this->name() << "' needs a pivot node AND a directional light !";

			return;
		}

		/* The pivot's position is the unit vector toward the sun, so the light must read its
		 * direction from the position (toward the origin), not from the node's forward axis. */
		light->useDirectionVector(false);

		m_pivot = pivot;
		m_light = light;
		m_bound = true;
		m_lit = light->isEnabled();

		this->apply();
	}

	void
	SunCourse::configure (const Options & options) noexcept
	{
		const auto previousPhase = this->phase();

		m_options = options;

		/* ---- Duration: an integer number of logic cycles per revolution. ---- */

		if ( m_options.dayDuration <= 0.0F )
		{
			TraceWarning{ClassId} << "The sun course '" << this->name() << "' declares a non-positive day duration (" << m_options.dayDuration << " s): 1 s used.";

			m_options.dayDuration = 1.0F;
		}

		const auto revolutionSeconds = 2.0F * m_options.dayDuration;

		m_revolutionCycles = std::max(1U, static_cast< uint32_t >(std::lround(revolutionSeconds * WorldPhysicsUpdateFrequency< float >)));

		/* ---- Elevation at noon. ---- */

		if ( m_options.noonElevation <= 0.0F || m_options.noonElevation > 90.0F )
		{
			TraceWarning{ClassId} << "The sun course '" << this->name() << "' declares a noon elevation of " << m_options.noonElevation << "°, outside (0, 90]: 60° used.";

			m_options.noonElevation = 60.0F;
		}

		/* ---- Photometry. ---- */

		m_options.zenithIlluminance = std::max(0.0F, m_options.zenithIlluminance);
		m_options.extinction = std::max(0.0F, m_options.extinction);

		/* ---- The course frame: sunrise (horizontal) and noon (tilted) unit vectors. ---- */

		const Vector< 3, float > up{0.0F, 1.0F, 0.0F};

		Vector< 3, float > sunrise{m_options.sunriseDirection.x(), 0.0F, m_options.sunriseDirection.z()};

		if ( sunrise.lengthSquared() < 1e-8F )
		{
			TraceWarning{ClassId} << "The sun course '" << this->name() << "' declares a vertical (or null) sunrise direction: +X used.";

			sunrise = {1.0F, 0.0F, 0.0F};
		}

		sunrise.normalize();

		/* The horizontal direction toward the noon sun: given, or derived on the right of the
		 * sunrise (the northern-hemisphere layout). Gram-Schmidt keeps a given one orthogonal. */
		Vector< 3, float > noonHorizontal{m_options.noonDirection.x(), 0.0F, m_options.noonDirection.z()};

		noonHorizontal -= sunrise * Vector< 3, float >::dotProduct(noonHorizontal, sunrise);

		if ( noonHorizontal.lengthSquared() < 1e-8F )
		{
			if ( m_options.noonDirection.lengthSquared() > 0.0F )
			{
				TraceWarning{ClassId} << "The sun course '" << this->name() << "' declares a noon direction collinear with the sunrise (or vertical): the derived one is used.";
			}

			noonHorizontal = Vector< 3, float >::crossProduct(sunrise, up);
		}

		noonHorizontal.normalize();

		const auto noonElevation = Radian(m_options.noonElevation);

		m_sunrise = sunrise;
		m_noon = (noonHorizontal * std::cos(noonElevation)) + (up * std::sin(noonElevation));

		/* ---- Phase: keep a running course where it was, else take the start phase. ---- */

		this->setPhase(m_running ? previousPhase : m_options.startPhase);
	}

	void
	SunCourse::setPhase (float phase) noexcept
	{
		const auto wrapped = phase - std::floor(phase);

		m_cycle = static_cast< uint32_t >(std::lround(wrapped * static_cast< float >(m_revolutionCycles))) % m_revolutionCycles;

		this->apply();
	}

	float
	SunCourse::airMass (float elevationDegrees) noexcept
	{
		const auto elevation = std::clamp(elevationDegrees, 0.0F, 90.0F);

		return 1.0F / (std::sin(Radian(elevation)) + 0.50572F * std::pow(elevation + 6.07995F, -1.6364F));
	}

	void
	SunCourse::processLogics (const Scene & /*scene*/) noexcept
	{
		if ( !m_running )
		{
			return;
		}

		/* An integer counter wrapping on the revolution: exactly periodic, no floating drift. */
		m_cycle = (m_cycle + 1) % m_revolutionCycles;

		this->apply();
	}

	void
	SunCourse::apply () noexcept
	{
		const auto pivot = m_pivot.lock();
		const auto light = m_light.lock();

		if ( pivot == nullptr || light == nullptr )
		{
			return;
		}

		const auto angle = this->phase() * 2.0F * std::numbers::pi_v< float >;

		/* The unit vector toward the sun on the great circle through the sunrise and noon points. */
		const auto towardSun = (m_sunrise * std::cos(angle)) + (m_noon * std::sin(angle));

		m_elevation = Degree(std::asin(std::clamp(towardSun.y(), -1.0F, 1.0F)));

		const bool daytime = m_elevation > 0.0F;

		/* Switch the light BEFORE moving it: Node::setPosition() dispatches move() to the
		 * components synchronously, and the light must be in its final state when it reads its
		 * frame (the engine no longer skips a disabled light's move(), but the order stays the
		 * honest one: decide, then aim). */
		if ( daytime != m_lit )
		{
			light->enable(daytime);

			m_lit = daytime;

			/* Twice a revolution: the events a reader of the log can time the course with. */
			TraceInfo{ClassId} << "Sun course '" << this->name() << "': " << (daytime ? "sunrise" : "sunset") << " (phase " << this->phase() << "), sun toward " << towardSun << ".";
		}

		/* DirectionalLight (position mode) shines from its position toward the origin. */
		pivot->setPosition(towardSun, TransformSpace::World);

		if ( !daytime )
		{
			/* Night: the sun lights nothing. */
			m_illuminance = 0.0F;
			m_temperature = m_options.horizonTemperature;

			return;
		}

		/* Day: Beer-Lambert extinction over the air-mass excess, referenced at the zenith. */
		const auto airMassExcess = SunCourse::airMass(m_elevation) - 1.0F;

		m_illuminance = m_options.zenithIlluminance * std::exp(-m_options.extinction * airMassExcess);
		m_temperature = m_options.horizonTemperature + (m_options.zenithTemperature - m_options.horizonTemperature) * std::exp(-ReddeningRate * airMassExcess);

		light->setIlluminance(m_illuminance);
		light->setColor(Graphics::Photometry::colorFromTemperature(m_temperature));

		/* Solar noon, once a revolution: the staged state a reader can check the photometry with. */
		if ( m_cycle == m_revolutionCycles / 4 )
		{
			TraceInfo{ClassId} << "Sun course '" << this->name() << "': noon, " << m_elevation << "° of elevation, " << m_illuminance << " lx, " << m_temperature << " K, light travelling " << light->direction() << ".";
		}
	}
}
