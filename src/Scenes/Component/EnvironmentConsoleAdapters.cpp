/*
 * src/Scenes/Component/EnvironmentConsoleAdapters.cpp
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

#include "ConsoleAdapter.hpp"

/* STL inclusions. */
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <memory>
#include <numbers>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "CelestialClock.hpp"
#include "CloudVolume.hpp"
#include "Console/CommandResult.hpp"
#include "Console/ControllableTrait.hpp"
#include "Console/Parameter.hpp"
#include "Console/Types.hpp"
#include "DirectionalLight.hpp"
#include "FastJSON.hpp"
#include "Math/Astronomy.hpp"
#include "Math/SolarSystem.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "SkyFollowsSun.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class Manager;
}

namespace EmEn::Scenes::Component
{
	using namespace Base;

	namespace
	{
		/** @brief The range of colour temperatures the engine converts (Graphics::Photometry, Kang et al. 2002). */
		constexpr auto MinimumTemperature{1667.0F};
		constexpr auto MaximumTemperature{25000.0F};

		/**
		 * @brief Returns whether an optional value is absent, or finite and inside a closed range.
		 * @param value The optional value.
		 * @param minimum The lowest accepted value.
		 * @param maximum The highest accepted value.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		absentOrWithin (const std::optional< float > & value, float minimum, float maximum) noexcept
		{
			return !value.has_value() || ( std::isfinite(*value) && *value >= minimum && *value <= maximum );
		}

		/**
		 * @brief Returns a world vector as a JSON array.
		 * @param vector The vector.
		 * @return Json::Value
		 */
		[[nodiscard]]
		Json::Value
		toJson (const Base::Math::Vector< 3, float > & vector) noexcept
		{
			Json::Value array{Json::arrayValue};
			array.append(static_cast< double >(vector.x()));
			array.append(static_cast< double >(vector.y()));
			array.append(static_cast< double >(vector.z()));

			return array;
		}

		/**
		 * @brief Returns a Julian day (UT) as an ISO 8601 UTC text, e.g. "2026-10-26T22:00:00.000Z".
		 * @param julianDay The Julian day.
		 * @return std::string "invalid" outside the Gregorian range.
		 */
		[[nodiscard]]
		std::string
		utcText (double julianDay) noexcept
		{
			/* Rounded to the millisecond BEFORE the split: 18:59:59.9999 prints 19:00:00.000, never 18:59:60.000. */
			const auto date = Base::Math::SolarSystem::calendarDate(julianDay + (0.0005 / Base::Math::Astronomy::SecondsPerDay));

			if ( !date.has_value() )
			{
				return "invalid";
			}

			std::ostringstream text;
			text << std::setfill('0') << std::setw(4) << date->year << '-' << std::setw(2) << date->month << '-' << std::setw(2) << date->day
				<< 'T' << std::setw(2) << date->hour << ':' << std::setw(2) << date->minute << ':'
				<< std::fixed << std::setprecision(3) << std::setw(6) << (std::floor(date->second * 1000.0) / 1000.0) << 'Z';

			return text.str();
		}

		/**
		 * @brief Moves a clock to a real instant, switching it to the calendar when it is on the planet model.
		 * @param clock A reference to the clock.
		 * @param julianDayUT The instant, a Julian day in UT (validated by the caller).
		 * @return bool False when the clock cannot be on the calendar (its planet is not Earth).
		 */
		[[nodiscard]]
		bool
		moveToInstant (CelestialClock & clock, double julianDayUT) noexcept
		{
			if ( clock.isCalendar() )
			{
				return clock.setInstant(julianDayUT);
			}

			auto options = clock.options();
			options.timeModel = CelestialClock::TimeModel::Calendar;
			options.julianDay = julianDayUT;

			clock.configure(options);

			return clock.isCalendar();
		}

		/**
		 * @brief Returns the full state of a celestial clock as JSON.
		 * @param clock The celestial clock.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const CelestialClock & clock) noexcept
		{
			namespace Sky = Base::Math::Astronomy;

			const auto & options = clock.options();

			Json::Value state{Json::objectValue};
			state["name"] = clock.name();
			state["entity"] = clock.parentEntity().name();
			state["type"] = CelestialClock::ClassId;
			state["running"] = clock.isRunning();
			state["day"] = static_cast< Json::Int64 >(clock.day());
			state["timeOfDay"] = clock.timeOfDay();
			state["timeScale"] = static_cast< double >(options.timeScale);
			state["solarDaySeconds"] = clock.solarDay();
			state["latitudeDegrees"] = static_cast< double >(options.latitude);
			state["longitudeDegrees"] = static_cast< double >(options.longitude);
			state["timeModel"] = clock.isCalendar() ? "Calendar" : "Planet";

			/* The real instant: the Julian day (UT) and its calendar date, ISO 8601. */
			if ( clock.isCalendar() )
			{
				/* A string: the JSON writer keeps ~5 significant digits, a Julian day needs 13. */
				std::ostringstream julian;
				julian << std::fixed << std::setprecision(6) << clock.julianDay();

				state["julianDayUT"] = julian.str();

				state["utc"] = utcText(clock.julianDay());
			}
			state["siderealAngleDegrees"] = Sky::degrees(static_cast< double >(clock.siderealAngle()));
			state["celestialPole"] = toJson(clock.celestialPole());

			/* The stars' frame: the world vectors of the equatorial axes (right ascension 0 and 90°, the pole). */
			Json::Value equatorialFrame{Json::arrayValue};

			for ( const auto & axis : clock.equatorialFrame() )
			{
				equatorialFrame.append(toJson(axis));
			}

			state["equatorialFrame"] = equatorialFrame;

			Json::Value sun{Json::objectValue};
			sun["elevationDegrees"] = static_cast< double >(clock.sunElevation());
			sun["daytime"] = clock.isDaytime();
			sun["direction"] = toJson(clock.sunDirection());
			sun["illuminanceLux"] = static_cast< double >(clock.illuminance());
			sun["temperatureKelvin"] = static_cast< double >(clock.temperature());
			sun["zenithIlluminanceLux"] = static_cast< double >(options.sun.zenithIlluminance);
			sun["extinction"] = static_cast< double >(options.sun.extinction);
			sun["zenithTemperatureKelvin"] = static_cast< double >(options.sun.zenithTemperature);
			sun["horizonTemperatureKelvin"] = static_cast< double >(options.sun.horizonTemperature);
			sun["model"] = clock.followsAtmosphere() ? "atmosphere" : "empirical";
			sun["topOfAtmosphereLux"] = static_cast< double >(clock.topOfAtmosphereIlluminance());
			sun["unitDistanceIlluminanceLux"] = static_cast< double >(options.sun.unitDistanceIlluminance);
			sun["starTemperatureKelvin"] = static_cast< double >(options.sun.starTemperature);

			if ( clock.followsAtmosphere() )
			{
				sun["transmittance"] = toJson(clock.sunTransmittance());
				sun["color"] = toJson(clock.sunColor());
			}

			if ( const auto light = clock.sunLight(); light != nullptr )
			{
				sun["light"] = light->name();
			}

			state["sun"] = std::move(sun);

			Json::Value planet{Json::objectValue};
			planet["siderealDaySeconds"] = options.planet.siderealDay;
			planet["orbitalPeriodSeconds"] = options.planet.orbitalPeriod;
			planet["axialTiltDegrees"] = Sky::degrees(options.planet.axialTilt);
			planet["eccentricity"] = options.planet.eccentricity;
			planet["name"] = options.planet.name;
			planet["semiMajorAxisAU"] = options.planet.semiMajorAxis;
			planet["starDistanceAU"] = clock.starDistance();
			state["planet"] = std::move(planet);

			Json::Value moons{Json::arrayValue};

			for ( size_t index = 0; index < clock.moonCount(); ++index )
			{
				Json::Value moon{Json::objectValue};
				moon["elevationDegrees"] = static_cast< double >(clock.moonElevation(index));
				moon["direction"] = toJson(clock.moonDirection(index));
				moon["illuminatedFraction"] = static_cast< double >(clock.moonIlluminatedFraction(index));
				moon["phaseAngleDegrees"] = static_cast< double >(clock.moonPhaseAngle(index)) * (180.0 / std::numbers::pi);
				moon["angularDiameterDegrees"] = static_cast< double >(clock.moonAngularDiameter(index));
				moon["surfaceBrightness"] = static_cast< double >(clock.moonSurfaceBrightness(index));
				moon["illuminanceLux"] = static_cast< double >(clock.moonIlluminance(index));

				if ( const auto light = clock.moonLight(index); light != nullptr )
				{
					moon["light"] = light->name();
				}

				moons.append(std::move(moon));
			}

			state["moons"] = std::move(moons);

			return FastJSON::stringify(state);
		}

		/**
		 * @brief Reconfigures a clock, keeping its current date and time of day.
		 * @param clock The celestial clock.
		 * @param options The new options (their date is replaced by the clock's current one).
		 */
		void
		reconfigureKeepingTheDate (CelestialClock & clock, CelestialClock::Options options) noexcept
		{
			/* The calendar keeps its instant; the planet model its day and time of day. */
			options.julianDay = clock.julianDay();
			options.dayOfYear = static_cast< double >(clock.day());
			options.timeOfDay = clock.timeOfDay();

			clock.configure(options);
		}

		/**
		 * @brief Returns the full state of a sky follower as JSON.
		 * @param follower The sky follower.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const SkyFollowsSun & follower) noexcept
		{
			const auto & options = follower.options();

			Json::Value state{Json::objectValue};
			state["name"] = follower.name();
			state["entity"] = follower.parentEntity().name();
			state["type"] = SkyFollowsSun::ClassId;
			state["factor"] = static_cast< double >(follower.factor());
			state["dayLuminanceNits"] = static_cast< double >(follower.dayLuminance());
			state["duskElevationDegrees"] = static_cast< double >(options.duskElevation);
			state["dayElevationDegrees"] = static_cast< double >(options.dayElevation);
			state["nightFactor"] = static_cast< double >(options.nightFactor);

			return FastJSON::stringify(state);
		}

		/**
		 * @brief Returns the full state of a volumetric cloud as JSON.
		 * @param cloud The cloud.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const CloudVolume & cloud) noexcept
		{
			const auto & look = cloud.look();
			const auto & halfExtents = cloud.halfExtents();

			Json::Value state{Json::objectValue};
			state["name"] = cloud.name();
			state["entity"] = cloud.parentEntity().name();
			state["type"] = CloudVolume::ClassId;

			Json::Value extents{Json::arrayValue};
			extents.append(static_cast< double >(halfExtents.x()));
			extents.append(static_cast< double >(halfExtents.y()));
			extents.append(static_cast< double >(halfExtents.z()));
			state["halfExtents"] = std::move(extents);

			Json::Value lookState{Json::objectValue};
			lookState["opticalThickness"] = static_cast< double >(look.opticalThickness);
			lookState["erosion"] = static_cast< double >(look.erosion);
			lookState["detailFrequency"] = static_cast< double >(look.detailFrequency);
			lookState["boilingSpeed"] = static_cast< double >(look.boilingSpeed);
			lookState["skyTint"] = static_cast< double >(look.skyTint);

			Json::Value albedo{Json::arrayValue};
			albedo.append(static_cast< double >(look.scatteringAlbedo.red()));
			albedo.append(static_cast< double >(look.scatteringAlbedo.green()));
			albedo.append(static_cast< double >(look.scatteringAlbedo.blue()));
			lookState["scatteringAlbedo"] = std::move(albedo);
			state["look"] = std::move(lookState);

			return FastJSON::stringify(state);
		}

		/**
		 * @brief The answer of a setter: the confirmation, then the component's NEW state as JSON.
		 * @tparam component_t The component type.
		 * @param component The component, after the change.
		 * @param message The confirmation.
		 * @return Console::CommandResult
		 */
		template< typename component_t >
		[[nodiscard]]
		Console::CommandResult
		changed (const component_t & component, std::string message) noexcept
		{
			return changedState(std::move(message), stateOf(component));
		}

		/** @brief `Core.SceneManagerService.CelestialClock.*`. */
		class CelestialClockConsoleAdapter final : public ConsoleAdapter< CelestialClock >
		{
			public:

				explicit
				CelestialClockConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

			private:

				void
				onRegisterToConsole () noexcept override
				{
					const Console::Parameter entity{entityParameter("the celestial clock")};
					const Console::Parameter component{componentParameter(CelestialClock::ClassId)};

					this->bindCommand("getState", "Returns the state of a celestial clock as JSON: running, the date (day since the vernal equinox, time of day 0 midnight / 0.5 noon), the time scale, the latitude, the sun (elevation, direction, lux and kelvins written to its light, photometry; with a dynamic sky the model atmosphere, the disc's transmittance and the linear colour written), the planet (its name and its distance to the star in AU), every moon (elevation, direction, illuminated fraction, lux), the celestial pole and the sidereal angle.",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (CelestialClock & clock) {
								return Console::CommandResult::json(stateOf(clock));
							});
						}, Console::CommandHint::ReadOnly);

					this->bindCommand("start", "Starts (or resumes) a celestial clock.",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (CelestialClock & clock) {
								clock.start();

								return changed(clock, "Celestial clock '" + clock.name() + "' running.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("stop", "Pauses a celestial clock: every body stays where it is, lit as it is — the way to freeze the light for an A/B measurement.",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (CelestialClock & clock) {
								clock.stop();

								return changed(clock, "Celestial clock '" + clock.name() + "' paused.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setTimeOfDay", "Moves a celestial clock to a time of day, keeping the day, and applies it at once (running or not).",
						{
							entity,
							component,
							{"timeOfDay", "The time of day, 0 to 1: 0 midnight, 0.25 morning, 0.5 noon, 0.75 evening."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float timeOfDay) {
							if ( !std::isfinite(timeOfDay) || timeOfDay < 0.0F || timeOfDay > 1.0F )
							{
								return Console::CommandResult::error("The time of day must lie between 0 and 1.");
							}

							return this->act(entityName, componentName, [timeOfDay] (CelestialClock & clock) {
								clock.setTimeOfDay(static_cast< double >(timeOfDay));

								return changed(clock, "Celestial clock '" + clock.name() + "' moved to time of day " + std::to_string(clock.timeOfDay()) + " (sun at " + std::to_string(clock.sunElevation()) + "°).");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setDate", "Moves a celestial clock to a date and a time of day and applies it at once.",
						{
							entity,
							component,
							{"day", "Days since the vernal equinox (Earth: 93 ≈ the June solstice, 186 the September equinox, 276 the December solstice)."},
							{"timeOfDay", "The time of day, 0 to 1: 0 midnight, 0.5 noon."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float day, float timeOfDay) {
							if ( !std::isfinite(day) || !std::isfinite(timeOfDay) || timeOfDay < 0.0F || timeOfDay > 1.0F )
							{
								return Console::CommandResult::error("The day must be finite and the time of day between 0 and 1.");
							}

							return this->act(entityName, componentName, [day, timeOfDay] (CelestialClock & clock) {
								/* A day since the equinox is the planet model's: a calendar clock goes back to it. */
								if ( clock.isCalendar() )
								{
									auto options = clock.options();
									options.timeModel = CelestialClock::TimeModel::Planet;
									options.dayOfYear = static_cast< double >(day);
									options.timeOfDay = static_cast< double >(timeOfDay);

									clock.configure(options);
								}
								else
								{
									clock.setDate(static_cast< double >(day), static_cast< double >(timeOfDay));
								}

								return changed(clock, "Celestial clock '" + clock.name() + "' moved to day " + std::to_string(clock.day()) + ", time of day " + std::to_string(clock.timeOfDay()) + " (sun at " + std::to_string(clock.sunElevation()) + "°).");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setCalendarDate", "Moves a celestial clock to a real UTC instant — our sky then: the Sun, the Moon and the precessed stars from the ephemerides, at the clock's latitude and longitude. A planet-model clock switches to the calendar (Earth only); setDate goes back.",
						{
							entity,
							component,
							{"year", "The year, 1583 to 9999 (Gregorian)."},
							{"month", "The month, 1 to 12."},
							{"day", "The day of the month."},
							{"hour", "The hour, 0 to 23, UTC."},
							{"minute", "The minute, 0 to 59."},
							{"second", "The second, 0 to 60."}
						},
						[this] (const std::string & entityName, const std::string & componentName, int32_t year, int32_t month, int32_t day, int32_t hour, int32_t minute, float second) {
							const auto instant = Base::Math::SolarSystem::julianDay({.year = year, .month = month, .day = day, .hour = hour, .minute = minute, .second = static_cast< double >(second)});

							if ( !instant.has_value() )
							{
								return Console::CommandResult::error("Not a Gregorian UTC instant (1583 to 9999, a real day of the month, 0-23 h, 0-59 min, 0-60 s).");
							}

							return this->act(entityName, componentName, [julianDayUT = instant.value()] (CelestialClock & clock) {
								if ( !moveToInstant(clock, julianDayUT) )
								{
									return Console::CommandResult::error("Celestial clock '" + clock.name() + "' cannot be on the calendar: its planet is '" + clock.options().planet.name + "', the calendar is Earth's.");
								}

								return changed(clock, "Celestial clock '" + clock.name() + "' at " + utcText(clock.julianDay()) + " (sun at " + std::to_string(clock.sunElevation()) + "°).");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setNow", "Moves a celestial clock to the system's current UTC time (switching it to the calendar, Earth only): the real sky of this instant at the clock's place.",
						{
							entity,
							component
						},
						[this] (const std::string & entityName, const std::string & componentName) {
							const auto unixSeconds = std::chrono::duration< double >(std::chrono::system_clock::now().time_since_epoch()).count();

							return this->act(entityName, componentName, [julianDayUT = Base::Math::SolarSystem::julianDayFromUnixTime(unixSeconds)] (CelestialClock & clock) {
								if ( !moveToInstant(clock, julianDayUT) )
								{
									return Console::CommandResult::error("Celestial clock '" + clock.name() + "' cannot be on the calendar: its planet is '" + clock.options().planet.name + "', the calendar is Earth's.");
								}

								return changed(clock, "Celestial clock '" + clock.name() + "' at " + utcText(clock.julianDay()) + " (sun at " + std::to_string(clock.sunElevation()) + "°).");
							});
						});

					this->bindCommand("setLongitude", "Moves the observer of a celestial clock to a longitude, keeping the instant (the calendar's local sidereal time; the planet model ignores it).",
						{
							entity,
							component,
							{"longitude", "The longitude, in degrees, EAST positive, −180 to 180 (Liège: 5.568)."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float longitude) {
							if ( !std::isfinite(longitude) || longitude < -180.0F || longitude > 180.0F )
							{
								return Console::CommandResult::error("The longitude must lie between −180 and 180 degrees.");
							}

							return this->act(entityName, componentName, [longitude] (CelestialClock & clock) {
								auto options = clock.options();
								options.longitude = longitude;

								reconfigureKeepingTheDate(clock, options);

								return changed(clock, "Celestial clock '" + clock.name() + "' observed from " + std::to_string(longitude) + "° of longitude (sun at " + std::to_string(clock.sunElevation()) + "°).");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setTimeScale", "Changes how many simulated seconds pass per real second, from now on (1 = real time, 720 = a day in two minutes, 0 freezes the sky).",
						{
							entity,
							component,
							{"timeScale", "Simulated seconds per real second, 0 or more."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float timeScale) {
							if ( !std::isfinite(timeScale) || timeScale < 0.0F )
							{
								return Console::CommandResult::error("The time scale must be a finite number, 0 or more.");
							}

							return this->act(entityName, componentName, [timeScale] (CelestialClock & clock) {
								clock.setTimeScale(timeScale);

								return changed(clock, "Celestial clock '" + clock.name() + "' at " + std::to_string(timeScale) + "× real time.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setLatitude", "Moves the observer of a celestial clock to a latitude, keeping the date.",
						{
							entity,
							component,
							{"latitude", "The latitude, in degrees, −90 (south pole) to 90 (north pole)."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float latitude) {
							if ( !std::isfinite(latitude) || latitude < -90.0F || latitude > 90.0F )
							{
								return Console::CommandResult::error("The latitude must lie between −90 and 90 degrees.");
							}

							return this->act(entityName, componentName, [latitude] (CelestialClock & clock) {
								auto options = clock.options();
								options.latitude = latitude;

								reconfigureKeepingTheDate(clock, options);

								return changed(clock, "Celestial clock '" + clock.name() + "' observed from " + std::to_string(latitude) + "° of latitude (sun at " + std::to_string(clock.sunElevation()) + "°).");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setSunPhotometry", "Changes the sun's photometry of a celestial clock; an omitted value is kept, and so is the date.",
						{
							entity,
							component,
							{"zenithIlluminance", "Direct normal illuminance with the sun at the zenith, in lux, 0 or more (100 000 = a clear sky)."},
							{"extinction", "Broadband optical depth of the atmosphere at air mass 1, 0 or more (0.22 = clear, 0.5 = hazy)."},
							{"zenithTemperature", "Colour temperature of the high sun, in kelvins, 1667 to 25000 (default 5800)."},
							{"horizonTemperature", "Colour temperature of the sun on the horizon, in kelvins, 1667 to 25000 (default 2000)."}
						},
						[this] (const std::string & entityName, const std::string & componentName, std::optional< float > zenithIlluminance, std::optional< float > extinction, std::optional< float > zenithTemperature, std::optional< float > horizonTemperature) {
							if ( !zenithIlluminance && !extinction && !zenithTemperature && !horizonTemperature )
							{
								return Console::CommandResult::error("Give at least one value to change.");
							}

							if ( !absentOrWithin(zenithIlluminance, 0.0F, std::numeric_limits< float >::max()) || !absentOrWithin(extinction, 0.0F, std::numeric_limits< float >::max()) )
							{
								return Console::CommandResult::error("The zenith illuminance and the extinction must be finite numbers, 0 or more.");
							}

							if ( !absentOrWithin(zenithTemperature, MinimumTemperature, MaximumTemperature) || !absentOrWithin(horizonTemperature, MinimumTemperature, MaximumTemperature) )
							{
								return Console::CommandResult::error("The colour temperatures must lie between 1667 K and 25000 K (the range the engine converts).");
							}

							return this->act(entityName, componentName, [&] (CelestialClock & clock) {
								auto options = clock.options();
								options.sun.zenithIlluminance = zenithIlluminance.value_or(options.sun.zenithIlluminance);
								options.sun.extinction = extinction.value_or(options.sun.extinction);
								options.sun.zenithTemperature = zenithTemperature.value_or(options.sun.zenithTemperature);
								options.sun.horizonTemperature = horizonTemperature.value_or(options.sun.horizonTemperature);

								reconfigureKeepingTheDate(clock, options);

								return changed(clock, "Celestial clock '" + clock.name() + "' sun photometry changed.");
							});
						}, Console::CommandHint::Idempotent);
				}
		};

		/** @brief `Core.SceneManagerService.SkyFollowsSun.*` (read-only: the twilight curve is authored with the scene). */
		class SkyFollowsSunConsoleAdapter final : public ConsoleAdapter< SkyFollowsSun >
		{
			public:

				explicit
				SkyFollowsSunConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

			private:

				void
				onRegisterToConsole () noexcept override
				{
					this->bindCommand("getState", "Returns the state of a sky follower as JSON: the factor last written to the sky background (0 night to 1 day), the day luminance of that background (nits), and its twilight curve (sun elevations in degrees, night factor).",
						{entityParameter("the sky follower"), componentParameter(SkyFollowsSun::ClassId)},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (SkyFollowsSun & follower) {
								return Console::CommandResult::json(stateOf(follower));
							});
						}, Console::CommandHint::ReadOnly);
				}
		};

		/** @brief `Core.SceneManagerService.CloudVolume.*`. */
		class CloudVolumeConsoleAdapter final : public ConsoleAdapter< CloudVolume >
		{
			public:

				explicit
				CloudVolumeConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

			private:

				void
				onRegisterToConsole () noexcept override
				{
					const Console::Parameter entity{entityParameter("the cloud")};
					const Console::Parameter component{componentParameter(CloudVolume::ClassId)};

					this->bindCommand("getState", "Returns the state of a volumetric cloud as JSON: its box half extents (metres) and its look (optical thickness, erosion, detail frequency, boiling speed, sky tint, scattering albedo).",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (CloudVolume & cloud) {
								return Console::CommandResult::json(stateOf(cloud));
							});
						}, Console::CommandHint::ReadOnly);

					this->bindCommand("setLook", "Changes the look of a volumetric cloud; an omitted parameter is kept. The scattering albedo is given as its three components together.",
						{
							entity,
							component,
							{"opticalThickness", "Vertical optical depth across the cloud's height at full density, 0 or more (~10 is a thin cumulus, default 30)."},
							{"erosion", "How deeply the detail noise erodes the shell, 0 to 1 (default 0.55)."},
							{"detailFrequency", "Detail noise cells across the cloud's width, more than 0 (default 4)."},
							{"boilingSpeed", "Drift of the detail noise, in cells per second, 0 or more (default 0.04)."},
							{"skyTint", "How much the cloud takes the hue of the sky's irradiance, 0 (physical, white) to 1 (default 0)."},
							{"albedoRed", "Red single-scattering albedo, 0 to 1 (water droplets: 1)."},
							{"albedoGreen", "Green single-scattering albedo, 0 to 1."},
							{"albedoBlue", "Blue single-scattering albedo, 0 to 1."}
						},
						[this] (const std::string & entityName, const std::string & componentName, std::optional< float > opticalThickness, std::optional< float > erosion, std::optional< float > detailFrequency, std::optional< float > boilingSpeed, std::optional< float > skyTint, std::optional< float > albedoRed, std::optional< float > albedoGreen, std::optional< float > albedoBlue) {
							const auto albedoCount = static_cast< int >(albedoRed.has_value()) + static_cast< int >(albedoGreen.has_value()) + static_cast< int >(albedoBlue.has_value());

							if ( !opticalThickness && !erosion && !detailFrequency && !boilingSpeed && !skyTint && albedoCount == 0 )
							{
								return Console::CommandResult::error("Give at least one parameter to change.");
							}

							if ( albedoCount != 0 && albedoCount != 3 )
							{
								return Console::CommandResult::error("The scattering albedo is a colour: give albedoRed, albedoGreen and albedoBlue together.");
							}

							constexpr auto Unbounded = std::numeric_limits< float >::max();

							if ( !absentOrWithin(opticalThickness, 0.0F, Unbounded) || !absentOrWithin(erosion, 0.0F, 1.0F) || !absentOrWithin(boilingSpeed, 0.0F, Unbounded) || !absentOrWithin(skyTint, 0.0F, 1.0F) )
							{
								return Console::CommandResult::error("Out of range: opticalThickness and boilingSpeed must be 0 or more, erosion and skyTint within [0, 1].");
							}

							if ( detailFrequency.has_value() && ( !std::isfinite(*detailFrequency) || *detailFrequency <= 0.0F ) )
							{
								return Console::CommandResult::error("The detail frequency must be a finite number, more than 0.");
							}

							if ( !absentOrWithin(albedoRed, 0.0F, 1.0F) || !absentOrWithin(albedoGreen, 0.0F, 1.0F) || !absentOrWithin(albedoBlue, 0.0F, 1.0F) )
							{
								return Console::CommandResult::error("Each scattering albedo component must lie within [0, 1].");
							}

							return this->act(entityName, componentName, [&] (CloudVolume & cloud) {
								auto look = cloud.look();
								look.opticalThickness = opticalThickness.value_or(look.opticalThickness);
								look.erosion = erosion.value_or(look.erosion);
								look.detailFrequency = detailFrequency.value_or(look.detailFrequency);
								look.boilingSpeed = boilingSpeed.value_or(look.boilingSpeed);
								look.skyTint = skyTint.value_or(look.skyTint);

								if ( albedoCount == 3 )
								{
									look.scatteringAlbedo = PixelFactory::Color< float >{*albedoRed, *albedoGreen, *albedoBlue};
								}

								cloud.setLook(look);

								return changed(cloud, "Cloud '" + cloud.name() + "' look changed.");
							});
						}, Console::CommandHint::Idempotent);
				}
		};
	}

	void
	appendEnvironmentConsoleAdapters (const Manager & sceneManager, std::vector< std::unique_ptr< Console::ControllableTrait > > & adapters) noexcept
	{
		adapters.emplace_back(std::make_unique< CelestialClockConsoleAdapter >(sceneManager));
		adapters.emplace_back(std::make_unique< SkyFollowsSunConsoleAdapter >(sceneManager));
		adapters.emplace_back(std::make_unique< CloudVolumeConsoleAdapter >(sceneManager));
	}
}
