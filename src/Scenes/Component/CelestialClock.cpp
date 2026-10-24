/*
 * src/Scenes/Component/CelestialClock.cpp
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

#include "CelestialClock.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <numbers>
#include <span>

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

	namespace Sky = Base::Math::Astronomy;

	CelestialClock::Options
	CelestialClock::simpleCourse (const SimpleCourse & course) noexcept
	{
		const Vector< 3, float > up{0.0F, 1.0F, 0.0F};

		Vector< 3, float > sunrise{course.sunriseDirection.x(), 0.0F, course.sunriseDirection.z()};

		if ( sunrise.lengthSquared() < 1e-8F )
		{
			sunrise = {1.0F, 0.0F, 0.0F};
		}

		sunrise.normalize();

		/* The horizontal direction toward the noon sun, Gram-Schmidt against the sunrise (the former SunCourse). */
		Vector< 3, float > noonHorizontal{course.noonDirection.x(), 0.0F, course.noonDirection.z()};
		noonHorizontal -= sunrise * Vector< 3, float >::dotProduct(noonHorizontal, sunrise);

		if ( noonHorizontal.lengthSquared() < 1e-8F )
		{
			noonHorizontal = Vector< 3, float >::crossProduct(sunrise, up);
		}

		noonHorizontal.normalize();

		const auto noonElevation = std::clamp(course.noonElevation, 1e-3F, 90.0F);
		const auto dayDuration = std::max(course.dayDuration, 1e-3F);

		/* The noon sun stands toward the equator. On the RIGHT of the sunrise (the default `sunrise × up`) the observer
		 * is north of it and the north is opposite the noon sun; on the left, south of it. East = north × up must be the
		 * sunrise: it is, in both cases. */
		const bool northern = Vector< 3, float >::dotProduct(noonHorizontal, Vector< 3, float >::crossProduct(sunrise, up)) > 0.0F;

		Options options;
		options.planet = {
			.siderealDay = 2.0 * static_cast< double >(dayDuration),
			.orbitalPeriod = 0.0,
			.axialTilt = 0.0,
			.eccentricity = 0.0,
			.longitudeOfPerihelion = 0.0
		};
		options.latitude = northern ? 90.0F - noonElevation : noonElevation - 90.0F;
		options.northDirection = northern ? -noonHorizontal : noonHorizontal;
		options.dayOfYear = 0.0;
		/* Phase 0 is the sunrise, a quarter of a day before noon. */
		options.timeOfDay = static_cast< double >(course.startPhase) + 0.25;
		options.timeScale = 1.0F;
		options.sun = course.sun;
		options.autoStart = course.autoStart;

		return options;
	}

	void
	CelestialClock::configure (const Options & options) noexcept
	{
		m_options = options;

		/* ---- The planet. ---- */
		if ( !(m_options.planet.siderealDay > 0.0) || !std::isfinite(m_options.planet.siderealDay) )
		{
			TraceWarning{ClassId} << "The celestial clock '" << this->name() << "' declares a non-positive sidereal day: Earth's is used.";

			m_options.planet.siderealDay = Sky::earth().siderealDay;
		}

		if ( !(m_options.planet.orbitalPeriod >= 0.0) || ( m_options.planet.orbitalPeriod > 0.0 && m_options.planet.orbitalPeriod <= m_options.planet.siderealDay ) )
		{
			TraceWarning{ClassId} << "The celestial clock '" << this->name() << "' declares an orbital period not longer than its sidereal day: the star is held still.";

			m_options.planet.orbitalPeriod = 0.0;
		}

		m_options.planet.eccentricity = std::clamp(m_options.planet.eccentricity, 0.0, 0.99);
		m_options.latitude = std::clamp(m_options.latitude, -90.0F, 90.0F);
		m_options.timeScale = std::max(0.0F, m_options.timeScale);

		/* ---- Photometry. ---- */
		m_options.sun.zenithIlluminance = std::max(0.0F, m_options.sun.zenithIlluminance);
		m_options.sun.extinction = std::max(0.0F, m_options.sun.extinction);

		for ( auto & moon : m_options.moons )
		{
			moon.fullIlluminance = std::max(0.0F, moon.fullIlluminance);
		}

		/* ---- The observer's frame: north (horizontal), up = +Y, east = north × up. ---- */
		const Vector< 3, float > up{0.0F, 1.0F, 0.0F};
		Vector< 3, float > north{m_options.northDirection.x(), 0.0F, m_options.northDirection.z()};

		if ( north.lengthSquared() < 1e-8F )
		{
			TraceWarning{ClassId} << "The celestial clock '" << this->name() << "' declares a vertical (or null) north: −Z used.";

			north = {0.0F, 0.0F, -1.0F};
		}

		north.normalize();

		m_north = north;
		m_east = Vector< 3, float >::crossProduct(north, up);
		m_solarDay = Sky::solarDay(m_options.planet);

		/* ---- The moons: the bindings of the moons that remain are kept. ---- */
		m_moons.resize(m_options.moons.size());

		/* ---- The date. ---- */
		this->setDate(m_options.dayOfYear, m_options.timeOfDay);
	}

	void
	CelestialClock::bindSun (const std::shared_ptr< Node > & pivot, const std::shared_ptr< DirectionalLight > & light) noexcept
	{
		if ( pivot == nullptr || light == nullptr )
		{
			TraceError{ClassId} << "The celestial clock '" << this->name() << "' needs a pivot node AND a directional light for its sun !";

			return;
		}

		/* The pivot's position is the unit vector toward the star: the light reads its direction from the position. */
		light->useDirectionVector(false);

		m_sun.pivot = pivot;
		m_sun.light = light;
		m_sun.lit = light->isEnabled();
		m_sunBound = true;

		this->apply();
	}

	void
	CelestialClock::bindMoon (size_t index, const std::shared_ptr< Node > & pivot, const std::shared_ptr< DirectionalLight > & light) noexcept
	{
		if ( index >= m_moons.size() )
		{
			TraceError{ClassId} << "The celestial clock '" << this->name() << "' has no moon #" << index << " (" << m_moons.size() << " declared) !";

			return;
		}

		if ( pivot == nullptr || light == nullptr )
		{
			TraceError{ClassId} << "The celestial clock '" << this->name() << "' needs a pivot node AND a directional light for its moon #" << index << " !";

			return;
		}

		light->useDirectionVector(false);

		m_moons[index].pivot = pivot;
		m_moons[index].light = light;
		m_moons[index].lit = light->isEnabled();

		this->apply();
	}

	void
	CelestialClock::setDate (double dayOfYear, double timeOfDay) noexcept
	{
		if ( !std::isfinite(dayOfYear) || !std::isfinite(timeOfDay) )
		{
			TraceWarning{ClassId} << "The celestial clock '" << this->name() << "' refused a non-finite date.";

			return;
		}

		const auto wrappedTimeOfDay = timeOfDay - std::floor(timeOfDay);

		/* The epoch is noon of day 0: noon is the fraction 0.5. */
		this->anchor((dayOfYear + wrappedTimeOfDay - 0.5) * m_solarDay);
		this->apply();
	}

	void
	CelestialClock::setTimeScale (float timeScale) noexcept
	{
		if ( !std::isfinite(timeScale) || timeScale < 0.0F )
		{
			TraceWarning{ClassId} << "The celestial clock '" << this->name() << "' refused the time scale " << timeScale << ".";

			return;
		}

		/* Re-anchor first: the time already elapsed keeps the scale it elapsed at. */
		this->anchor(this->currentTime());

		m_options.timeScale = timeScale;
	}

	void
	CelestialClock::anchor (double time) noexcept
	{
		m_referenceTime = time;
		m_referenceCycle = m_cycle;
	}

	double
	CelestialClock::currentTime () const noexcept
	{
		/* Recomputed from a stored reference and an integer count: never accumulated. */
		const auto elapsedCycles = static_cast< double >(m_cycle - m_referenceCycle);

		return m_referenceTime + (elapsedCycles * static_cast< double >(m_options.timeScale) / WorldPhysicsUpdateFrequency< double >);
	}

	int64_t
	CelestialClock::day () const noexcept
	{
		return static_cast< int64_t >(std::floor((this->currentTime() / m_solarDay) + 0.5));
	}

	double
	CelestialClock::timeOfDay () const noexcept
	{
		const auto date = (this->currentTime() / m_solarDay) + 0.5;

		return date - std::floor(date);
	}

	float
	CelestialClock::moonElevation (size_t index) const noexcept
	{
		return index < m_moons.size() ? m_moons[index].elevation : 0.0F;
	}

	Vector< 3, float >
	CelestialClock::moonDirection (size_t index) const noexcept
	{
		return index < m_moons.size() ? m_moons[index].direction : Vector< 3, float >{};
	}

	float
	CelestialClock::moonIlluminatedFraction (size_t index) const noexcept
	{
		return index < m_moons.size() ? m_moons[index].illuminatedFraction : 0.0F;
	}

	float
	CelestialClock::moonIlluminance (size_t index) const noexcept
	{
		return index < m_moons.size() ? m_moons[index].illuminance : 0.0F;
	}

	std::shared_ptr< DirectionalLight >
	CelestialClock::sunLight () const noexcept
	{
		return m_sun.light.lock();
	}

	std::shared_ptr< DirectionalLight >
	CelestialClock::moonLight (size_t index) const noexcept
	{
		return index < m_moons.size() ? m_moons[index].light.lock() : nullptr;
	}

	float
	CelestialClock::airMass (float elevationDegrees) noexcept
	{
		const auto elevation = std::clamp(elevationDegrees, 0.0F, 90.0F);

		return 1.0F / (std::sin(Radian(elevation)) + 0.50572F * std::pow(elevation + 6.07995F, -1.6364F));
	}

	float
	CelestialClock::lambertPhase (float illuminatedFraction) noexcept
	{
		const auto phaseAngle = std::acos(std::clamp((2.0F * illuminatedFraction) - 1.0F, -1.0F, 1.0F));

		return std::max(0.0F, (std::sin(phaseAngle) + ((std::numbers::pi_v< float > - phaseAngle) * std::cos(phaseAngle))) / std::numbers::pi_v< float >);
	}

	void
	CelestialClock::publishStateForRendering (uint32_t writeStateIndex) noexcept
	{
		auto & state = *std::next(m_renderStates.begin(), static_cast< std::ptrdiff_t >(writeStateIndex));

		/* Above the atmosphere: the zenith illuminance is referenced at air mass 1, through one extinction. */
		const auto starColor = Graphics::Photometry::colorFromTemperature(m_options.sun.zenithTemperature).unitLuminanceChromaticity();

		state.sun = {
			.direction = m_sunDirection,
			.illuminance = m_options.sun.zenithIlluminance * std::exp(m_options.sun.extinction),
			.color = starColor,
			.angularDiameter = m_options.sun.angularDiameter,
			.illuminatedFraction = 1.0F
		};

		/* m_moons mirrors m_options.moons one for one (setOptions()); at most MaxMoons are published. */
		const auto moonCount = std::min({m_moons.size(), m_options.moons.size(), static_cast< size_t >(SkyState::MaxMoons)});

		state.moonCount = static_cast< uint32_t >(moonCount);

		std::ranges::transform(std::span{m_moons}.first(moonCount), std::span{m_options.moons}.first(moonCount), state.moons.begin(), [] (const BoundBody & moon, const Moon & declared) {
			return SkyBody{
				.direction = moon.direction,
				.illuminance = declared.fullIlluminance * CelestialClock::lambertPhase(moon.illuminatedFraction),
				.color = Graphics::Photometry::colorFromTemperature(declared.temperature).unitLuminanceChromaticity(),
				.angularDiameter = declared.angularDiameter,
				.illuminatedFraction = moon.illuminatedFraction
			};
		});

		state.celestialPole = m_celestialPole;
		state.siderealAngle = m_siderealAngle;
		state.valid = true;
	}

	void
	CelestialClock::processLogics (const Scene & /*scene*/) noexcept
	{
		if ( !m_running )
		{
			return;
		}

		++m_cycle;

		this->apply();
	}

	Vector< 3, float >
	CelestialClock::toWorld (const Sky::Horizontal & horizontal) const noexcept
	{
		const Vector< 3, float > up{0.0F, 1.0F, 0.0F};

		return (m_east * static_cast< float >(horizontal.east)) + (m_north * static_cast< float >(horizontal.north)) + (up * static_cast< float >(horizontal.up));
	}

	void
	CelestialClock::apply () noexcept
	{
		const auto time = this->currentTime();
		const Sky::Observer observer{.latitude = Sky::radians(static_cast< double >(m_options.latitude))};
		const auto towardStar = Sky::starDirection(m_options.planet, observer, time);

		m_sunDirection = this->toWorld(towardStar);
		m_sunElevation = static_cast< float >(Sky::degrees(towardStar.elevation()));
		m_celestialPole = this->toWorld(Sky::celestialPole(observer));
		m_siderealAngle = static_cast< float >(Sky::siderealAngle(m_options.planet, time));

		const auto daytime = m_sunElevation > 0.0F;

		if ( daytime )
		{
			/* Day: Beer-Lambert extinction over the air-mass excess, referenced at the zenith. */
			const auto airMassExcess = CelestialClock::airMass(m_sunElevation) - 1.0F;

			m_illuminance = m_options.sun.zenithIlluminance * std::exp(-m_options.sun.extinction * airMassExcess);
			m_temperature = m_options.sun.horizonTemperature + ((m_options.sun.zenithTemperature - m_options.sun.horizonTemperature) * std::exp(-ReddeningRate * airMassExcess));
		}
		else
		{
			/* Night: the sun lights nothing. */
			m_illuminance = 0.0F;
			m_temperature = m_options.sun.horizonTemperature;
		}

		const auto pivot = m_sun.pivot.lock();
		const auto light = m_sun.light.lock();

		if ( pivot != nullptr && light != nullptr )
		{
			/* Switch the light BEFORE moving it: Node::setPosition() dispatches move() to the components synchronously,
			 * and the light must be in its final state when it reads its frame. */
			if ( daytime != m_sun.lit )
			{
				light->enable(daytime);
				m_sun.lit = daytime;

				/* Twice a day: the events a reader of the log can time the clock with. */
				TraceInfo{ClassId} << "Celestial clock '" << this->name() << "': " << (daytime ? "sunrise" : "sunset") << " (day " << this->day() << ", time of day " << this->timeOfDay() << "), sun toward " << m_sunDirection << ".";
			}

			/* DirectionalLight (position mode) shines from its position toward the origin. */
			pivot->setPosition(m_sunDirection, TransformSpace::World);

			if ( daytime )
			{
				light->setIlluminance(m_illuminance);
				light->setColor(Graphics::Photometry::colorFromTemperature(m_temperature));
			}
		}

		for ( size_t index = 0; index < m_moons.size(); ++index )
		{
			this->applyMoon(index, towardStar, time);
		}
	}

	void
	CelestialClock::applyMoon (size_t index, const Sky::Horizontal & towardStar, double time) noexcept
	{
		const auto & declared = m_options.moons[index];
		auto & moon = m_moons[index];
		const Sky::Observer observer{.latitude = Sky::radians(static_cast< double >(m_options.latitude))};
		const auto towardMoon = Sky::moonDirection(m_options.planet, declared.orbit, observer, time);

		moon.direction = this->toWorld(towardMoon);
		moon.elevation = static_cast< float >(Sky::degrees(towardMoon.elevation()));
		moon.illuminatedFraction = static_cast< float >(Sky::illuminatedFraction(towardStar, towardMoon));

		/* A moon lights the scene while it is up and the sun is down: in daylight its fraction of a lux is nothing, and
		 * one directional light less is one light pass less. */
		const auto lighting = moon.elevation > 0.0F && m_sunElevation <= 0.0F;

		if ( lighting )
		{
			const auto airMassExcess = CelestialClock::airMass(moon.elevation) - 1.0F;

			moon.illuminance = declared.fullIlluminance * CelestialClock::lambertPhase(moon.illuminatedFraction) * std::exp(-m_options.sun.extinction * (airMassExcess + 1.0F));
		}
		else
		{
			moon.illuminance = 0.0F;
		}

		const auto pivot = moon.pivot.lock();
		const auto light = moon.light.lock();

		if ( pivot == nullptr || light == nullptr )
		{
			return;
		}

		const auto lit = lighting && moon.illuminance > 0.0F;

		if ( lit != moon.lit )
		{
			light->enable(lit);
			moon.lit = lit;
		}

		pivot->setPosition(moon.direction, TransformSpace::World);

		if ( lit )
		{
			light->setIlluminance(moon.illuminance);
			light->setColor(Graphics::Photometry::colorFromTemperature(declared.temperature));
		}
	}
}
