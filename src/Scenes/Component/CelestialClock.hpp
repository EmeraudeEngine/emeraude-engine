/*
 * src/Scenes/Component/CelestialClock.hpp
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
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Math/Astronomy.hpp"
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class Node;

	namespace Component
	{
		class DirectionalLight;
	}
}

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Drives the sky's bodies from a calendar: a planet, a latitude, a date and a time of day place the star (the
	 * sun) and the moons, and their directional lights follow — direction, illuminance, colour, on and off.
	 * @details The astronomy is `Base::Math::Astronomy` (a generic Keplerian two-body model, owner decision 2026-10-24):
	 * the star moves along the ecliptic with the planet's orbit, the planet turns at its sidereal rate, the axial tilt
	 * makes the seasons, the latitude the day length. Earth is a preset, an alien planet is data. Each logic cycle:
	 * - the SUN's pivot is placed on the unit vector toward the star, and its light gets the illuminance and the
	 *   colour temperature of the air mass on the line of sight (Kasten & Young 1989, Beer-Lambert from
	 *   `SunPhotometry::zenithIlluminance`), dark while the star is below the horizon;
	 * - each MOON's pivot is placed toward it, and its light gets the full-moon illuminance × a Lambert-sphere phase
	 *   function × the same extinction; a moon lights the scene while it is up AND the sun is down.
	 * @note It REPLACES the former SunCourse (owner decision 2026-10-24, "a special case"): a planet without tilt nor
	 * orbit, observed at the latitude 90° − the noon elevation, is exactly that course (`Options::simpleCourse()`,
	 * proven by emeraude-base's `MathAstronomy.AStillPlanetReproducesTheSimpleSunCourse`).
	 * @note TIME: seconds since the epoch (the vernal equinox, at the observer's noon), in DOUBLE, recomputed every
	 * cycle from a stored reference and an integer cycle count — never accumulated. `timeScale` simulated seconds
	 * pass per real second.
	 * @note FRAME: the observer's East-North-Up is mapped onto the world with `Options::northDirection` (horizontal)
	 * and up = +Y; east = north × up. The default north is −Z, so east is +X and the noon sun of the northern
	 * hemisphere stands toward +Z — the former SunCourse layout.
	 * @note ⚠️ THE SKY ITSELF is not driven here (owner decision 2026-09-13): the background, its ambient and the IBL
	 * belong to the scene's background; `SkyFollowsSun` can make a picture sky follow this clock.
	 * @note Like `NodeAnimation`, the clock holds WEAK references to its nodes and lights: once its sun is bound, it
	 * asks for its own removal when the sun's node or light dies.
	 * @see EmEn::Scenes::Toolkit::generateCelestialClock() Builds the pivots, the lights and this component in one call.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 */
	class EMEN_API CelestialClock final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CelestialClock"};

			/** @brief Direct sunlight at the zenith through a clear atmosphere, in lux. */
			static constexpr auto DefaultZenithIlluminance{100000.0F};

			/** @brief Broadband Beer-Lambert optical depth of a clear sky at air mass 1. */
			static constexpr auto DefaultExtinction{0.22F};

			/** @brief Colour temperature of the high sun, in kelvins. */
			static constexpr auto DefaultZenithTemperature{5800.0F};

			/** @brief Colour temperature of the sun on the horizon, in kelvins. */
			static constexpr auto DefaultHorizonTemperature{2000.0F};

			/** @brief Illuminance of a full moon at the zenith above the atmosphere, in lux (the Moon: ~0.3 lx). */
			static constexpr auto DefaultFullMoonIlluminance{0.3F};

			/** @brief Colour temperature of moonlight, in kelvins. */
			static constexpr auto DefaultMoonTemperature{4100.0F};

			/**
			 * @brief The sun's photometry, through the planet's atmosphere.
			 */
			struct SunPhotometry
			{
				/** @brief Direct normal illuminance at air mass 1 (the star at the zenith), in lux. */
				float zenithIlluminance{DefaultZenithIlluminance};
				/** @brief Broadband Beer-Lambert optical depth at air mass 1 (0.22 clear, 0.5 hazy). Also dims the moons. */
				float extinction{DefaultExtinction};
				/** @brief Colour temperature of the star at the zenith, in kelvins. */
				float zenithTemperature{DefaultZenithTemperature};
				/** @brief Colour temperature of the star on the horizon, in kelvins. */
				float horizonTemperature{DefaultHorizonTemperature};
			};

			/**
			 * @brief A moon: its orbit and its light.
			 */
			struct Moon
			{
				/** @brief Its orbit around the planet. */
				Base::Math::Astronomy::MoonOrbit orbit{Base::Math::Astronomy::moon()};
				/** @brief Illuminance when full and at the zenith, above the atmosphere, in lux. */
				float fullIlluminance{DefaultFullMoonIlluminance};
				/** @brief Colour temperature of its light, in kelvins. */
				float temperature{DefaultMoonTemperature};
			};

			/**
			 * @brief Parameters of the clock.
			 * @note Designated initialisers are the intended use.
			 */
			struct Options
			{
				/** @brief The planet: rotation, orbit, tilt. Default Earth. */
				Base::Math::Astronomy::Planet planet{Base::Math::Astronomy::earth()};
				/** @brief The observer's latitude, in degrees, in [-90, 90]. Default 45° north. */
				float latitude{45.0F};
				/** @brief Horizontal world direction toward the planet's north. The vertical component is discarded. Default −Z. */
				Base::Math::Vector< 3, float > northDirection{0.0F, 0.0F, -1.0F};
				/** @brief Days (solar days of the planet) since the vernal equinox. Default 0. */
				double dayOfYear{0.0};
				/** @brief Time of day, as a fraction of the solar day: 0 midnight, 0.25 morning, 0.5 noon. Default 0.5. */
				double timeOfDay{0.5};
				/** @brief Simulated seconds per real second. Default 1 (real time). */
				float timeScale{1.0F};
				/** @brief The sun's photometry. */
				SunPhotometry sun{};
				/** @brief The moons. Default none; `{Moon{}}` adds Earth's Moon, full at the equinox. */
				std::vector< Moon > moons;
				/** @brief Whether the clock runs as soon as it is built. Default true. */
				bool autoStart{true};
			};

			/**
			 * @brief The parameters of the former SunCourse: one sun on a fixed great circle, day and night of equal
			 * length.
			 */
			struct SimpleCourse
			{
				/** @brief Horizontal direction toward the point where the sun RISES. Default +X. */
				Base::Math::Vector< 3, float > sunriseDirection{1.0F, 0.0F, 0.0F};
				/** @brief Horizontal direction toward the noon sun; ZERO derives `cross(sunrise, up)` (the northern layout). */
				Base::Math::Vector< 3, float > noonDirection{0.0F, 0.0F, 0.0F};
				/** @brief Seconds from sunrise to sunset; the night lasts as long. Default 120 s. */
				float dayDuration{120.0F};
				/** @brief Elevation of the sun at noon, in degrees, in (0, 90]. Default 60°. */
				float noonElevation{60.0F};
				/** @brief Phase of the course at start, in [0, 1): 0 sunrise, 0.25 noon, 0.5 sunset, 0.75 midnight. */
				float startPhase{0.0F};
				/** @brief The sun's photometry. */
				SunPhotometry sun{};
				/** @brief Whether the course runs as soon as it is built. Default true. */
				bool autoStart{true};
			};

			/**
			 * @brief Returns the options that reproduce a former SunCourse exactly: a planet without tilt nor orbit whose
			 * sidereal day is two day durations, observed at the latitude 90° − the noon elevation.
			 * @note A noon direction on the LEFT of the sunrise is the southern layout: the latitude goes south.
			 * @param course A reference to the course.
			 * @return Options
			 */
			[[nodiscard]]
			static Options simpleCourse (const SimpleCourse & course) noexcept;

			/**
			 * @brief Constructs a celestial clock component.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 */
			CelestialClock (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
				: Abstract{componentName, parentEntity}
			{

			}

			/* ---- Setup ---- */

			/**
			 * @brief Sets the parameters and applies the resulting state at once.
			 * @note Degenerate values are corrected and traced. The moons' bindings are kept for the moons that remain.
			 * @param options A reference to the options.
			 */
			void configure (const Options & options) noexcept;

			/**
			 * @brief Binds the node and the directional light of the sun.
			 * @note The light is switched to its position-to-origin direction mode: the pivot's position is the unit vector
			 * toward the star. ⚠️ The pivot must be a direct child of the root (`Toolkit::generateNode()`).
			 * @param pivot A reference to the node whose position the clock writes.
			 * @param light A reference to the directional light whose photometry the clock writes.
			 */
			void bindSun (const std::shared_ptr< Node > & pivot, const std::shared_ptr< DirectionalLight > & light) noexcept;

			/**
			 * @brief Binds the node and the directional light of a moon declared in the options.
			 * @param index The moon's index in `Options::moons`.
			 * @param pivot A reference to the node whose position the clock writes.
			 * @param light A reference to the directional light whose photometry the clock writes.
			 */
			void bindMoon (size_t index, const std::shared_ptr< Node > & pivot, const std::shared_ptr< DirectionalLight > & light) noexcept;

			/* ---- Playback ---- */

			/**
			 * @brief Starts (or resumes) the clock.
			 */
			void
			start () noexcept
			{
				m_running = true;
			}

			/**
			 * @brief Pauses the clock: every body stays where it is, lit as it is.
			 */
			void
			stop () noexcept
			{
				m_running = false;
			}

			/**
			 * @brief Returns whether the clock is advancing.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRunning () const noexcept
			{
				return m_running;
			}

			/**
			 * @brief Jumps to a date and a time of day and applies it.
			 * @param dayOfYear Days since the vernal equinox (any real value).
			 * @param timeOfDay Fraction of the solar day, wrapped into [0, 1): 0.5 noon.
			 */
			void setDate (double dayOfYear, double timeOfDay) noexcept;

			/**
			 * @brief Jumps to a time of day, keeping the day, and applies it.
			 * @param timeOfDay Fraction of the solar day, wrapped into [0, 1): 0 midnight, 0.5 noon.
			 */
			void
			setTimeOfDay (double timeOfDay) noexcept
			{
				this->setDate(static_cast< double >(this->day()), timeOfDay);
			}

			/**
			 * @brief Changes how many simulated seconds pass per real second, from now on.
			 * @param timeScale The scale, ≥ 0 (0 freezes the sky without stopping the clock).
			 */
			void setTimeScale (float timeScale) noexcept;

			/* ---- State ---- */

			/**
			 * @brief Returns the simulated time, in seconds since the epoch (the vernal equinox at noon).
			 * @return double
			 */
			[[nodiscard]]
			double currentTime () const noexcept;

			/**
			 * @brief Returns the current day since the vernal equinox (the integer part of the date).
			 * @return int64_t
			 */
			[[nodiscard]]
			int64_t day () const noexcept;

			/**
			 * @brief Returns the current time of day, in [0, 1): 0 midnight, 0.5 noon.
			 * @return double
			 */
			[[nodiscard]]
			double timeOfDay () const noexcept;

			/**
			 * @brief Returns the length of the planet's solar day, in simulated seconds.
			 * @return double
			 */
			[[nodiscard]]
			double
			solarDay () const noexcept
			{
				return m_solarDay;
			}

			/**
			 * @brief Returns the sun's elevation above the horizon, in degrees (negative at night).
			 * @return float
			 */
			[[nodiscard]]
			float
			sunElevation () const noexcept
			{
				return m_sunElevation;
			}

			/**
			 * @brief Returns whether the sun is above the horizon.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDaytime () const noexcept
			{
				return m_sunElevation > 0.0F;
			}

			/**
			 * @brief Returns the unit world vector toward the sun.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			sunDirection () const noexcept
			{
				return m_sunDirection;
			}

			/**
			 * @brief Returns the illuminance written to the sun's light, in lux (0 at night).
			 * @return float
			 */
			[[nodiscard]]
			float
			illuminance () const noexcept
			{
				return m_illuminance;
			}

			/**
			 * @brief Returns the colour temperature written to the sun's light, in kelvins.
			 * @return float
			 */
			[[nodiscard]]
			float
			temperature () const noexcept
			{
				return m_temperature;
			}

			/**
			 * @brief Returns the number of moons.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			moonCount () const noexcept
			{
				return m_moons.size();
			}

			/**
			 * @brief Returns a moon's elevation, in degrees.
			 * @param index The moon's index.
			 * @return float 0 for an index out of range.
			 */
			[[nodiscard]]
			float moonElevation (size_t index) const noexcept;

			/**
			 * @brief Returns the unit world vector toward a moon.
			 * @param index The moon's index.
			 * @return Base::Math::Vector< 3, float > Zero for an index out of range.
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > moonDirection (size_t index) const noexcept;

			/**
			 * @brief Returns the illuminated fraction of a moon's disc, in [0, 1]: 0 new, 1 full.
			 * @param index The moon's index.
			 * @return float 0 for an index out of range.
			 */
			[[nodiscard]]
			float moonIlluminatedFraction (size_t index) const noexcept;

			/**
			 * @brief Returns the illuminance written to a moon's light, in lux (0 while it does not light the scene).
			 * @param index The moon's index.
			 * @return float 0 for an index out of range.
			 */
			[[nodiscard]]
			float moonIlluminance (size_t index) const noexcept;

			/**
			 * @brief Returns the unit world vector toward the northern celestial pole: the axis the stars turn around.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			celestialPole () const noexcept
			{
				return m_celestialPole;
			}

			/**
			 * @brief Returns the local sidereal angle — the right ascension on the meridian — in radians, in [0, 2π).
			 * @note The rotation of the celestial sphere around celestialPole(): what a star field turns by.
			 * @return float
			 */
			[[nodiscard]]
			float
			siderealAngle () const noexcept
			{
				return m_siderealAngle;
			}

			/**
			 * @brief Returns the current options (the date and time are those given at the last configure()).
			 * @return const Options &
			 */
			[[nodiscard]]
			const Options &
			options () const noexcept
			{
				return m_options;
			}

			/**
			 * @brief Returns the sun's directional light.
			 * @return std::shared_ptr< DirectionalLight > Null when unbound or dead.
			 */
			[[nodiscard]]
			std::shared_ptr< DirectionalLight > sunLight () const noexcept;

			/**
			 * @brief Returns a moon's directional light.
			 * @param index The moon's index.
			 * @return std::shared_ptr< DirectionalLight > Null when unbound, dead, or out of range.
			 */
			[[nodiscard]]
			std::shared_ptr< DirectionalLight > moonLight (size_t index) const noexcept;

			/* ---- Component contract ---- */

			/** @copydoc EmEn::Scenes::Component::Abstract::getComponentType() */
			[[nodiscard]]
			const char *
			getComponentType () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::isComponent() */
			[[nodiscard]]
			bool
			isComponent (const char * classID) const noexcept override
			{
				return std::strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void
			move (const Base::Math::CartesianFrame< float > & /*worldCoordinates*/) noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				/* Once bound, the clock dies with the node or the light of its sun. */
				return m_sunBound && (m_sun.pivot.expired() || m_sun.light.expired());
			}

			/**
			 * @brief Returns the relative optical air mass on a line of sight.
			 * @note Kasten, F. & Young, A. T., "Revised optical air mass tables and approximation formula", Applied
			 * Optics 28 (22), 1989: `1 / (sin h + 0.50572 (h + 6.07995)^-1.6364)`, h in degrees. 1 at the zenith, about 38
			 * on the horizon.
			 * @param elevationDegrees The elevation, in degrees, clamped to [0, 90].
			 * @return float
			 */
			[[nodiscard]]
			static float airMass (float elevationDegrees) noexcept;

			/**
			 * @brief Returns the Lambert-sphere phase function: the brightness of a diffuse sphere relative to full.
			 * @note Φ(α) = (sin α + (π − α) cos α) / π, α the phase angle: 1 full, 1/π at quarter, 0 new. The Moon is
			 * darker than this at quarter (~0.1, its opposition surge); a planet-generic moon takes the Lambert sphere.
			 * @param illuminatedFraction The illuminated fraction k, in [0, 1] (cos α = 2k − 1).
			 * @return float
			 */
			[[nodiscard]]
			static float lambertPhase (float illuminatedFraction) noexcept;

		private:

			/**
			 * @brief A body's node and light, and what was last written to them.
			 */
			struct BoundBody
			{
				std::weak_ptr< Node > pivot;
				std::weak_ptr< DirectionalLight > light;
				Base::Math::Vector< 3, float > direction{0.0F, 1.0F, 0.0F};
				float elevation{0.0F};
				float illuminatedFraction{0.0F};
				float illuminance{0.0F};
				bool lit{false};
			};

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void onSuspend () noexcept override { }

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void onWakeup () noexcept override { }

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool
			playAnimation (uint8_t /*animationID*/, const Base::Variant & /*value*/, size_t /*cycle*/) noexcept override
			{
				return false;
			}

			/**
			 * @brief Converts a direction of the observer's East-North-Up frame to a world vector.
			 * @param horizontal The direction.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > toWorld (const Base::Math::Astronomy::Horizontal & horizontal) const noexcept;

			/**
			 * @brief Re-anchors the time reference on the current cycle (before a scale change, or a jump).
			 * @param time The simulated time to anchor, in seconds since the epoch.
			 */
			void anchor (double time) noexcept;

			/**
			 * @brief Writes the state of the current time to the pivots and the lights.
			 */
			void apply () noexcept;

			/**
			 * @brief Writes a moon's state to its pivot and light.
			 * @param index The moon's index.
			 * @param towardStar The star's direction in the observer's frame.
			 * @param time The simulated time.
			 */
			void applyMoon (size_t index, const Base::Math::Astronomy::Horizontal & towardStar, double time) noexcept;

			/** @brief Rate of the colour-temperature slide with the air mass excess (m - 1): 5100 K at 30° of elevation, 3700 K at 11°, 2700 K at 5.5°, the horizon value at 0°. */
			static constexpr auto ReddeningRate{0.2F};

			Options m_options;
			BoundBody m_sun;
			std::vector< BoundBody > m_moons;
			Base::Math::Vector< 3, float > m_east{1.0F, 0.0F, 0.0F};
			Base::Math::Vector< 3, float > m_north{0.0F, 0.0F, -1.0F};
			Base::Math::Vector< 3, float > m_sunDirection{0.0F, 1.0F, 0.0F};
			Base::Math::Vector< 3, float > m_celestialPole{0.0F, 1.0F, 0.0F};
			double m_solarDay{Base::Math::Astronomy::SecondsPerDay};
			double m_referenceTime{0.0}; ///< Simulated time at m_referenceCycle, seconds since the epoch.
			uint64_t m_referenceCycle{0};
			uint64_t m_cycle{0}; ///< Logic cycles elapsed since the clock was built.
			float m_sunElevation{0.0F};
			float m_illuminance{0.0F};
			float m_temperature{DefaultHorizonTemperature};
			float m_siderealAngle{0.0F};
			bool m_sunBound{false};
			bool m_running{false};
	};
}
