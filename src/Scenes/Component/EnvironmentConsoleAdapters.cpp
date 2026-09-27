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
#include <cmath>
#include <limits>
#include <optional>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions. */
#include "CloudVolume.hpp"
#include "DirectionalLight.hpp"
#include "FastJSON.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "SkyFollowsSun.hpp"
#include "SunCourse.hpp"

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
		 * @brief Returns the full state of a sun course as JSON.
		 * @param course The sun course.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		stateOf (const SunCourse & course) noexcept
		{
			const auto & options = course.options();

			Json::Value state{Json::objectValue};
			state["name"] = course.name();
			state["entity"] = course.parentEntity().name();
			state["type"] = SunCourse::ClassId;
			state["running"] = course.isRunning();
			state["phase"] = static_cast< double >(course.phase());
			state["elevationDegrees"] = static_cast< double >(course.elevation());
			state["daytime"] = course.isDaytime();
			state["illuminanceLux"] = static_cast< double >(course.illuminance());
			state["temperatureKelvin"] = static_cast< double >(course.temperature());

			if ( const auto light = course.light(); light != nullptr )
			{
				state["light"] = light->name();
			}

			Json::Value parameters{Json::objectValue};
			parameters["dayDurationSeconds"] = static_cast< double >(options.dayDuration);
			parameters["noonElevationDegrees"] = static_cast< double >(options.noonElevation);
			parameters["zenithIlluminanceLux"] = static_cast< double >(options.zenithIlluminance);
			parameters["extinction"] = static_cast< double >(options.extinction);
			parameters["zenithTemperatureKelvin"] = static_cast< double >(options.zenithTemperature);
			parameters["horizonTemperatureKelvin"] = static_cast< double >(options.horizonTemperature);
			state["course"] = std::move(parameters);

			return FastJSON::stringify(state);
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

		/** @brief `Core.SceneManagerService.SunCourse.*`. */
		class SunCourseConsoleAdapter final : public ConsoleAdapter< SunCourse >
		{
			public:

				explicit
				SunCourseConsoleAdapter (const Manager & sceneManager) noexcept
					: ConsoleAdapter{sceneManager}
				{

				}

			private:

				void
				onRegisterToConsole () noexcept override
				{
					const Console::Parameter entity{entityParameter("the sun course")};
					const Console::Parameter component{componentParameter(SunCourse::ClassId)};

					this->bindCommand("getState", "Returns the state of a sun course as JSON: running, phase (0 sunrise, 0.25 noon, 0.5 sunset, 0.75 midnight), sun elevation (degrees), illuminance (lux) and colour temperature (kelvins) written to its light, and the course parameters.",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (SunCourse & course) {
								return Console::CommandResult::json(stateOf(course));
							});
						}, Console::CommandHint::ReadOnly);

					this->bindCommand("start", "Starts (or resumes) a sun course from its current phase.",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (SunCourse & course) {
								course.start();

								return changed(course, "Sun course '" + course.name() + "' running.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("stop", "Pauses a sun course: the sun stays where it is, lit as it is — the way to freeze the light for an A/B measurement.",
						{entity, component},
						[this] (const std::string & entityName, const std::string & componentName) {
							return this->act(entityName, componentName, [] (SunCourse & course) {
								course.stop();

								return changed(course, "Sun course '" + course.name() + "' paused.");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setPhase", "Moves a sun course to a phase of its revolution and applies it at once (running or not).",
						{
							entity,
							component,
							{"phase", "The phase, 0 to 1: 0 sunrise, 0.25 noon, 0.5 sunset, 0.75 midnight."}
						},
						[this] (const std::string & entityName, const std::string & componentName, float phase) {
							if ( !std::isfinite(phase) || phase < 0.0F || phase > 1.0F )
							{
								return Console::CommandResult::error("The phase must lie between 0 and 1.");
							}

							return this->act(entityName, componentName, [phase] (SunCourse & course) {
								course.setPhase(phase);

								return changed(course, "Sun course '" + course.name() + "' moved to phase " + std::to_string(course.phase()) + " (" + std::to_string(course.elevation()) + "° of elevation).");
							});
						}, Console::CommandHint::Idempotent);

					this->bindCommand("setCourse", "Changes the parameters of a sun course; an omitted one is kept. The phase is kept too, so the sun does not jump.",
						{
							entity,
							component,
							{"dayDuration", "Seconds from sunrise to sunset, more than 0 (the night lasts as long)."},
							{"noonElevation", "Elevation of the sun at noon, in degrees, more than 0 and at most 90."},
							{"zenithIlluminance", "Direct normal illuminance with the sun at the zenith, in lux, 0 or more (100 000 = a clear sky)."},
							{"extinction", "Broadband optical depth of the atmosphere at air mass 1, 0 or more (0.22 = clear, 0.5 = hazy)."},
							{"zenithTemperature", "Colour temperature of the high sun, in kelvins, 1667 to 25000 (default 5800)."},
							{"horizonTemperature", "Colour temperature of the sun on the horizon, in kelvins, 1667 to 25000 (default 2000)."}
						},
						[this] (const std::string & entityName, const std::string & componentName, std::optional< float > dayDuration, std::optional< float > noonElevation, std::optional< float > zenithIlluminance, std::optional< float > extinction, std::optional< float > zenithTemperature, std::optional< float > horizonTemperature) {
							if ( !dayDuration && !noonElevation && !zenithIlluminance && !extinction && !zenithTemperature && !horizonTemperature )
							{
								return Console::CommandResult::error("Give at least one parameter to change.");
							}

							if ( ( dayDuration.has_value() && ( !std::isfinite(*dayDuration) || *dayDuration <= 0.0F ) ) || ( noonElevation.has_value() && ( !std::isfinite(*noonElevation) || *noonElevation <= 0.0F || *noonElevation > 90.0F ) ) )
							{
								return Console::CommandResult::error("The day duration must be more than 0 s and the noon elevation within (0, 90] degrees.");
							}

							if ( !absentOrWithin(zenithIlluminance, 0.0F, std::numeric_limits< float >::max()) || !absentOrWithin(extinction, 0.0F, std::numeric_limits< float >::max()) )
							{
								return Console::CommandResult::error("The zenith illuminance and the extinction must be finite numbers, 0 or more.");
							}

							if ( !absentOrWithin(zenithTemperature, MinimumTemperature, MaximumTemperature) || !absentOrWithin(horizonTemperature, MinimumTemperature, MaximumTemperature) )
							{
								return Console::CommandResult::error("The colour temperatures must lie between 1667 K and 25000 K (the range the engine converts).");
							}

							return this->act(entityName, componentName, [&] (SunCourse & course) {
								auto options = course.options();
								options.dayDuration = dayDuration.value_or(options.dayDuration);
								options.noonElevation = noonElevation.value_or(options.noonElevation);
								options.zenithIlluminance = zenithIlluminance.value_or(options.zenithIlluminance);
								options.extinction = extinction.value_or(options.extinction);
								options.zenithTemperature = zenithTemperature.value_or(options.zenithTemperature);
								options.horizonTemperature = horizonTemperature.value_or(options.horizonTemperature);

								/* ⚠️ SunCourse::configure() keeps the phase of a RUNNING course only: a paused one
								 * restarts from Options::startPhase. Restoring it keeps the sun where it is. */
								const auto phase = course.phase();

								course.configure(options);
								course.setPhase(phase);

								return changed(course, "Sun course '" + course.name() + "' reconfigured.");
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
		adapters.emplace_back(std::make_unique< SunCourseConsoleAdapter >(sceneManager));
		adapters.emplace_back(std::make_unique< SkyFollowsSunConsoleAdapter >(sceneManager));
		adapters.emplace_back(std::make_unique< CloudVolumeConsoleAdapter >(sceneManager));
	}
}
