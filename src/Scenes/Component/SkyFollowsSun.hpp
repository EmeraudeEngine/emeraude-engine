/*
 * src/Scenes/Component/SkyFollowsSun.hpp
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

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics::Renderable
	{
		class AbstractBackground;
	}

	namespace Scenes::Component
	{
		class SunCourse;
	}
}

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Makes the scene's BACKGROUND follow a SunCourse: the sky's luminance — hence the
	 * IBL it feeds (diffuse and specular), the picture drawn in the skybox and the sky term the
	 * post-processing reads — is scaled by a twilight factor of the sun's elevation, down to
	 * `Options::nightFactor` at night.
	 * @details Every logic cycle the component reads the course's elevation, maps it through a
	 * smoothstep from `Options::duskElevation` (factor 0) to `Options::dayElevation` (factor 1) and
	 * writes `dayLuminance * factor` to the background (`AbstractBackground::setLuminance()`), then
	 * asks the scene to push the new environment luminance to the view buffers
	 * (`Scene::refreshAmbientLightProperties()`). The DAY luminance is the manifest's, captured the
	 * first time the background is seen loaded — and captured again if the scene swaps its sky.
	 * @note ⚠️ This is the COMPLEMENT of the Toolkit's sun, kept apart on purpose (owner decision,
	 * 2026-09-13): `Toolkit::generateSunCourse()` builds the sun only; a scene that wants its sky
	 * to follow adds this component next to it. The two are separable: a course without it keeps
	 * a constant sky, a night courtyard then lit by a daylight ambient.
	 * @note The curve is PERCEPTUAL, not radiative: a clear sky's luminance drops by orders of
	 * magnitude through twilight and a linear factor cannot follow that; the smoothstep gives a
	 * legible dusk on a demo clock (a two-minute day). The two elevations are options for that
	 * reason. A moonless night is `nightFactor = 0`: the background lights nothing and the scene's
	 * own lamps carry it.
	 * @note Writes are QUANTISED (1/1024 of the factor): during the day and the night the factor
	 * is constant and nothing is pushed; only the twilight costs a view-buffer refresh per change.
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 */
	class EMEN_API SkyFollowsSun final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SkyFollowsSun"};

			/**
			 * @brief Parameters of the twilight curve.
			 */
			struct Options
			{
				/** @brief Elevation of the sun, in degrees, below which the sky is at its night factor. Default -6° (end of civil twilight). */
				float duskElevation{-6.0F};

				/** @brief Elevation of the sun, in degrees, from which the sky is at its full day luminance. Default 10°. */
				float dayElevation{10.0F};

				/** @brief Fraction of the day luminance kept at night, in [0, 1]. Default 0 (moonless). */
				float nightFactor{0.0F};
			};

			/**
			 * @brief Constructs a sky-follows-sun component.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 */
			SkyFollowsSun (const std::string & componentName, const AbstractEntity & parentEntity) noexcept
				: Abstract{componentName, parentEntity}
			{

			}

			/**
			 * @brief Destructs the component, giving the background its day luminance back.
			 * @note The view buffers are not refreshed here (no scene at hand): the next
			 * Scene::refreshAmbientLightProperties() picks the value up.
			 */
			~SkyFollowsSun () override;

			/**
			 * @brief Binds the sun course whose elevation drives the sky.
			 * @param course A reference to the sun course.
			 * @return void
			 */
			void bind (const std::shared_ptr< SunCourse > & course) noexcept;

			/**
			 * @brief Sets the twilight curve.
			 * @note An inverted or degenerate pair of elevations is corrected and traced; the night
			 * factor is clamped to [0, 1].
			 * @param options A reference to the options.
			 * @return void
			 */
			void configure (const Options & options) noexcept;

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
			 * @brief Returns the factor last written to the background, in [0, 1] (-1 before the first write).
			 * @return float
			 */
			[[nodiscard]]
			float
			factor () const noexcept
			{
				return m_lastFactor;
			}

			/**
			 * @brief Returns the DAY luminance of the background, in nits, as captured from its manifest (0 before the background is seen).
			 * @return float
			 */
			[[nodiscard]]
			float
			dayLuminance () const noexcept
			{
				return m_dayLuminance;
			}

			/**
			 * @brief Returns the twilight factor for an elevation of the sun.
			 * @param elevationDegrees The elevation, in degrees.
			 * @return float In [nightFactor, 1].
			 */
			[[nodiscard]]
			float twilightFactor (float elevationDegrees) const noexcept;

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
				/* Once bound, the follower dies with its course. */
				return m_bound && m_course.expired();
			}

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

			/** @brief Factor resolution: a change smaller than this is not pushed to the view buffers. */
			static constexpr auto FactorStep{1.0F / 1024.0F};

			Options m_options;
			std::weak_ptr< SunCourse > m_course;
			std::weak_ptr< Graphics::Renderable::AbstractBackground > m_background; ///< The background whose day luminance is captured.
			float m_dayLuminance{0.0F};
			float m_lastFactor{-1.0F};
			bool m_bound{false};
	};
}
