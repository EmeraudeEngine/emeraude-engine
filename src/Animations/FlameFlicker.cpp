/*
 * src/Animations/FlameFlicker.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "FlameFlicker.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <numbers>

/* Local inclusions. */
#include "Constants.hpp"

namespace EmEn::Animations
{
	using namespace Base;

	FlameFlicker::FlameFlicker (float nominalCandela, float baseDiameter, uint32_t seed) noexcept
		: m_randomizer{seed},
		m_nominalCandela{std::max(0.0F, nominalCandela)},
		m_frequency{FlameFlicker::puffingFrequency(baseDiameter)}
	{
		/* Start anywhere in the cycle: two flames built on the same tick must not start in phase. */
		m_phase = m_randomizer.value(0.0F, 2.0F * std::numbers::pi_v< float >);
	}

	float
	FlameFlicker::puffingFrequency (float baseDiameter) noexcept
	{
		return 1.5F / std::sqrt(std::max(0.01F, baseDiameter));
	}

	float
	FlameFlicker::gaussian () noexcept
	{
		/* Box-Muller: u1 kept away from 0, whose logarithm is -inf. */
		const auto u1 = m_randomizer.value(1.0e-6F, 1.0F);
		const auto u2 = m_randomizer.value(0.0F, 1.0F);

		return std::sqrt(-2.0F * std::log(u1)) * std::cos(2.0F * std::numbers::pi_v< float > * u2);
	}

	float
	FlameFlicker::nextLevel () noexcept
	{
		constexpr auto DeltaTime = 1.0F / WorldPhysicsUpdateFrequency< float >;
		constexpr auto Tau = 2.0F * std::numbers::pi_v< float >;

		/* The puffing: the phase advances at the flame's own frequency and drifts by a random walk. */
		m_phase += (Tau * m_frequency * DeltaTime) + (PhaseDiffusion * std::sqrt(DeltaTime) * this->gaussian());

		if ( m_phase > Tau )
		{
			m_phase -= Tau;
		}
		else if ( m_phase < 0.0F )
		{
			m_phase += Tau;
		}

		const auto puff = PuffAmplitude * std::sin(m_phase);

		/* The turbulent flicker: an Ornstein-Uhlenbeck step (exact discretisation), which keeps its
		 * stationary deviation at TurbulenceDeviation whatever the logic frequency. */
		const auto decay = std::exp(-DeltaTime / TurbulenceTime);

		m_turbulence = (m_turbulence * decay) + (TurbulenceDeviation * std::sqrt(1.0F - (decay * decay)) * this->gaussian());

		/* A gust: a smooth dip (half a sine) that lays the flame down, then lets it up. */
		auto gust = 0.0F;

		if ( m_gustElapsed < m_gustDuration )
		{
			m_gustElapsed += DeltaTime;

			gust = m_gustDepth * std::sin(std::numbers::pi_v< float > * std::min(1.0F, m_gustElapsed / m_gustDuration));
		}
		else if ( m_randomizer.value(0.0F, 1.0F) < GustRate * DeltaTime )
		{
			m_gustElapsed = 0.0F;
			m_gustDuration = m_randomizer.value(GustMinimumDuration, GustMaximumDuration);
			m_gustDepth = m_randomizer.value(GustMinimumDepth, GustMaximumDepth);
		}

		return std::clamp(1.0F + puff + m_turbulence - gust, MinimumLevel, MaximumLevel);
	}

	Variant
	FlameFlicker::getNextValue () noexcept
	{
		return Variant{m_nominalCandela * this->nextLevel()};
	}
}
