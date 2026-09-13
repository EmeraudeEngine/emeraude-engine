/*
 * src/Scenes/Component/SunCourse.hpp
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

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
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
	 * @brief Drives a directional light along the daily course of the sun: sunrise, culmination,
	 * sunset, night — with the illuminance and the colour temperature the elevation dictates.
	 * @details The sun follows a GREAT CIRCLE through the sunrise point and the noon point, at a
	 * CONSTANT angular speed: the day (sunrise to sunset) and the night last the same
	 * `Options::dayDuration`, one revolution being twice that. Each logic cycle the component:
	 * - places its PIVOT node on the unit vector toward the sun — a `DirectionalLight` in its
	 *   default mode (`useDirectionVector(false)`) shines from its position toward the origin, so
	 *   the position IS the light direction;
	 * - sets the light's illuminance from the air mass on the line of sight (Kasten & Young 1989)
	 *   through a Beer-Lambert extinction referenced at air mass 1 (`Options::zenithIlluminance` at
	 *   the zenith, a few tens of lux on the horizon);
	 * - sets the light's colour from a colour temperature sliding from `Options::zenithTemperature`
	 *   to `Options::horizonTemperature` with the same air mass (an exponential fit, anchored on
	 *   those two ends — an empirical shape, not a radiative-transfer result);
	 * - DISABLES the light while the sun is below the horizon: at night it lights nothing.
	 * @note ⚠️ THE SUN ONLY (owner decision, 2026-09-13). The sky, its ambient and the IBL belong to
	 * the scene's background (`Scene::applyBackgroundLighting()`): this component never touches
	 * them. A scene that wants its ambient to follow the sun drives the background itself.
	 * @note The phase is an INTEGER cycle counter over an integer revolution length — never a float
	 * accumulated cycle after cycle — so the course is exactly periodic and `setPhase()` lands on
	 * the same direction every time (owner rule: recompute from a stored reference, never
	 * accumulate on the live value).
	 * @note Like `NodeAnimation`, the component holds WEAK references to the node and the light it
	 * drives: once bound, it asks for its own removal when either dies.
	 * @see EmEn::Scenes::Toolkit::generateSunCourse() Builds the pivot, the light and this component in one call.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 */
	class EMEN_API SunCourse final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SunCourse"};

			/** @brief Direct sunlight at the zenith through a clear atmosphere, in lux — the default `Options::zenithIlluminance`. */
			static constexpr auto DefaultZenithIlluminance{100000.0F};

			/** @brief Broadband Beer-Lambert optical depth of a clear sky at air mass 1 — the default `Options::extinction`. */
			static constexpr auto DefaultExtinction{0.22F};

			/** @brief Colour temperature of the high sun, in kelvins — the default `Options::zenithTemperature`. */
			static constexpr auto DefaultZenithTemperature{5800.0F};

			/** @brief Colour temperature of the sun on the horizon, in kelvins — the default `Options::horizonTemperature`. */
			static constexpr auto DefaultHorizonTemperature{2000.0F};

			/**
			 * @brief Parameters of a sun course.
			 * @note Designated initialisers are the intended use: `{.dayDuration = 60.0F, .noonElevation = 45.0F}`.
			 */
			struct Options
			{
				/**
				 * @brief Horizontal direction toward the point where the sun RISES.
				 * @note The vertical component is discarded and the vector normalised. Default +X.
				 */
				Base::Math::Vector< 3, float > sunriseDirection{1.0F, 0.0F, 0.0F};

				/**
				 * @brief Horizontal direction toward the sun at NOON.
				 * @note ZERO (the default) derives it as `cross(sunrise, up)`: with the sun rising at +X it
				 * culminates toward +Z — the northern-hemisphere layout, where an observer facing the noon
				 * sun sees it rise on the left and set on the right. A non-zero vector is projected on the
				 * horizontal plane and made orthogonal to the sunrise direction.
				 */
				Base::Math::Vector< 3, float > noonDirection{0.0F, 0.0F, 0.0F};

				/** @brief Seconds from sunrise to sunset. The night lasts as long; a revolution is twice this. Default 120 s. */
				float dayDuration{120.0F};

				/** @brief Elevation of the sun at noon, in degrees, in (0, 90]. Default 60°. */
				float noonElevation{60.0F};

				/** @brief Direct normal illuminance at air mass 1 (the sun at the zenith), in lux. Default 100 000 lx. */
				float zenithIlluminance{DefaultZenithIlluminance};

				/** @brief Broadband Beer-Lambert optical depth at air mass 1. 0.22 is a clear sky; 0.5 a hazy one. */
				float extinction{DefaultExtinction};

				/** @brief Colour temperature of the sun at the zenith, in kelvins. Default 5800 K. */
				float zenithTemperature{DefaultZenithTemperature};

				/** @brief Colour temperature of the sun on the horizon, in kelvins. Default 2000 K. */
				float horizonTemperature{DefaultHorizonTemperature};

				/** @brief Phase of the revolution at start, in [0, 1): 0 sunrise, 0.25 noon, 0.5 sunset, 0.75 midnight. Default 0. */
				float startPhase{0.0F};

				/** @brief Whether the course runs as soon as it is built. Default true. */
				bool autoStart{true};
			};

			/**
			 * @brief Constructs a sun course component.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 */
			SunCourse (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
				: Abstract{componentName, parentEntity}
			{

			}

			/* ---- Setup ---- */

			/**
			 * @brief Binds the node and the directional light the course drives.
			 * @note The light is switched to its position-to-origin direction mode
			 * (`useDirectionVector(false)`): the pivot's position is the unit vector toward the sun.
			 * ⚠️ The pivot must be a direct child of the root — `Node::setPosition()` in world space
			 * is only complete at that depth — which is what `Toolkit::generateNode()` produces.
			 * @param pivot A reference to the node whose position the course writes.
			 * @param light A reference to the directional light whose photometry the course writes.
			 * @return void
			 */
			void bind (const std::shared_ptr< Node > & pivot, const std::shared_ptr< DirectionalLight > & light) noexcept;

			/**
			 * @brief Sets the course parameters and applies the resulting state at once.
			 * @note Degenerate values are corrected and traced (a vertical sunrise direction, a
			 * non-positive duration, an elevation outside (0, 90]). Reconfiguring a running course
			 * keeps its current phase.
			 * @param options A reference to the options.
			 * @return void
			 */
			void configure (const Options & options) noexcept;

			/* ---- Playback ---- */

			/**
			 * @brief Starts (or resumes) the course from its current phase.
			 * @return void
			 */
			void
			start () noexcept
			{
				m_running = true;
			}

			/**
			 * @brief Pauses the course. The sun stays where it is, lit as it is.
			 * @return void
			 */
			void
			stop () noexcept
			{
				m_running = false;
			}

			/**
			 * @brief Returns whether the course is advancing.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRunning () const noexcept
			{
				return m_running;
			}

			/**
			 * @brief Jumps to a phase of the revolution and applies it.
			 * @param phase The phase, wrapped into [0, 1): 0 sunrise, 0.25 noon, 0.5 sunset, 0.75 midnight.
			 * @return void
			 */
			void setPhase (float phase) noexcept;

			/**
			 * @brief Returns the current phase of the revolution, in [0, 1).
			 * @return float
			 */
			[[nodiscard]]
			float
			phase () const noexcept
			{
				return static_cast< float >(m_cycle) / static_cast< float >(m_revolutionCycles);
			}

			/**
			 * @brief Returns the current elevation of the sun above the horizon, in degrees (negative at night).
			 * @return float
			 */
			[[nodiscard]]
			float
			elevation () const noexcept
			{
				return m_elevation;
			}

			/**
			 * @brief Returns whether the sun is above the horizon.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDaytime () const noexcept
			{
				return m_elevation > 0.0F;
			}

			/**
			 * @brief Returns the illuminance currently written to the light, in lux (0 at night).
			 * @return float
			 */
			[[nodiscard]]
			float
			illuminance () const noexcept
			{
				return m_illuminance;
			}

			/**
			 * @brief Returns the colour temperature currently written to the light, in kelvins (the horizon value at night).
			 * @return float
			 */
			[[nodiscard]]
			float
			temperature () const noexcept
			{
				return m_temperature;
			}

			/**
			 * @brief Returns the current options.
			 * @return const Options &
			 */
			[[nodiscard]]
			const Options &
			options () const noexcept
			{
				return m_options;
			}

			/**
			 * @brief Returns the driven directional light.
			 * @return std::shared_ptr< DirectionalLight > Null when unbound or dead.
			 */
			[[nodiscard]]
			std::shared_ptr< DirectionalLight >
			light () const noexcept
			{
				return m_light.lock();
			}

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
				/* Once bound, the course dies with the node or the light it drives. */
				return m_bound && (m_pivot.expired() || m_light.expired());
			}

			/**
			 * @brief Returns the relative optical air mass on the line of sight to the sun.
			 * @note Kasten, F. & Young, A. T., "Revised optical air mass tables and approximation
			 * formula", Applied Optics 28 (22), 1989: `1 / (sin h + 0.50572 (h + 6.07995)^-1.6364)`,
			 * h in degrees. 1 at the zenith, about 38 on the horizon.
			 * @param elevationDegrees The elevation of the sun, in degrees, clamped to [0, 90].
			 * @return float
			 */
			[[nodiscard]]
			static float airMass (float elevationDegrees) noexcept;

		private:

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
			 * @brief Writes the state of the current phase to the pivot and the light.
			 * @return void
			 */
			void apply () noexcept;

			/** @brief Rate of the colour-temperature slide with the air mass excess (m - 1): 5100 K at 30° of elevation, 3700 K at 11°, 2700 K at 5.5°, the horizon value at 0°. */
			static constexpr auto ReddeningRate{0.2F};

			Options m_options;
			std::weak_ptr< Node > m_pivot;
			std::weak_ptr< DirectionalLight > m_light;
			Base::Math::Vector< 3, float > m_sunrise{1.0F, 0.0F, 0.0F}; ///< Unit, horizontal: the sun at phase 0.
			Base::Math::Vector< 3, float > m_noon{0.0F, 1.0F, 0.0F}; ///< Unit, orthogonal to m_sunrise: the sun at phase 0.25.
			uint32_t m_cycle{0}; ///< Logic cycles elapsed in the current revolution.
			uint32_t m_revolutionCycles{1}; ///< Logic cycles per revolution (two day durations).
			float m_elevation{0.0F};
			float m_illuminance{0.0F};
			float m_temperature{DefaultHorizonTemperature};
			bool m_bound{false};
			bool m_running{false};
			bool m_lit{false}; ///< The enable state last written to the light.
	};
}
