/*
 * src/Scenes/Component/SkyFollowsSun.cpp
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

#include "SkyFollowsSun.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>

/* Local inclusions. */
#include "Graphics/Renderable/AbstractBackground.hpp"
#include "Scenes/Component/SunCourse.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes::Component
{
	SkyFollowsSun::~SkyFollowsSun ()
	{
		/* Give the sky its day back: a follower removed at dusk must not leave a dimmed sky behind. */
		if ( const auto background = m_background.lock(); background != nullptr && m_dayLuminance > 0.0F )
		{
			background->setLuminance(m_dayLuminance);
		}
	}

	void
	SkyFollowsSun::bind (const std::shared_ptr< SunCourse > & course) noexcept
	{
		if ( course == nullptr )
		{
			TraceError{ClassId} << "The sky follower '" << this->name() << "' needs a sun course !";

			return;
		}

		m_course = course;
		m_bound = true;
		m_lastFactor = -1.0F;
	}

	void
	SkyFollowsSun::configure (const Options & options) noexcept
	{
		m_options = options;

		if ( m_options.dayElevation <= m_options.duskElevation )
		{
			TraceWarning{ClassId} << "The sky follower '" << this->name() << "' declares a day elevation (" << m_options.dayElevation << "°) not above its dusk elevation (" << m_options.duskElevation << "°): -6° / 10° used.";

			m_options.duskElevation = -6.0F;
			m_options.dayElevation = 10.0F;
		}

		m_options.nightFactor = std::clamp(m_options.nightFactor, 0.0F, 1.0F);

		/* Force the next cycle to push the curve's new value. */
		m_lastFactor = -1.0F;
	}

	float
	SkyFollowsSun::twilightFactor (float elevationDegrees) const noexcept
	{
		const auto t = std::clamp((elevationDegrees - m_options.duskElevation) / (m_options.dayElevation - m_options.duskElevation), 0.0F, 1.0F);
		const auto smooth = t * t * (3.0F - 2.0F * t);

		return m_options.nightFactor + (1.0F - m_options.nightFactor) * smooth;
	}

	void
	SkyFollowsSun::processLogics (const Scene & scene) noexcept
	{
		const auto course = m_course.lock();

		if ( course == nullptr )
		{
			return;
		}

		const auto background = scene.background();

		if ( background == nullptr || !background->isLoaded() )
		{
			return;
		}

		/* Anchor on the background the first time it is seen loaded — its manifest luminance is
		 * the DAY value — and again whenever the scene swaps its sky. */
		if ( m_background.lock() != background )
		{
			m_background = background;
			m_dayLuminance = background->luminance();
			m_lastFactor = -1.0F;

			TraceInfo{ClassId} << "The sky follower '" << this->name() << "' anchors on background '" << background->name() << "' (" << m_dayLuminance << " nits by day).";
		}

		/* Quantised: a constant day or night pushes nothing. */
		const auto factor = std::round(this->twilightFactor(course->elevation()) / FactorStep) * FactorStep;

		if ( std::abs(factor - m_lastFactor) < FactorStep * 0.5F )
		{
			return;
		}

		m_lastFactor = factor;

		/* ONE knob, every consumer: the IBL scale and the sky term through the view buffers, the
		 * drawn skybox through the background's own hook. */
		background->setLuminance(m_dayLuminance * factor);

		scene.refreshAmbientLightProperties();
	}
}
