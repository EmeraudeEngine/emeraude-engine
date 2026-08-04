/*
 * src/Animations/LampFlicker.cpp
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

#include "LampFlicker.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <numbers>

/* Local inclusions. */
#include "Constants.hpp"

namespace EmEn::Animations
{
	using namespace Base;
	using namespace Base::PixelFactory;

	LampFlicker::LampFlicker (float nominalCandela, float health) noexcept
		: m_nominalCandela{std::max(0.0F, nominalCandela)},
		m_health{std::clamp(health, 0.0F, 1.0F)}
	{

	}

	void
	LampFlicker::setHealth (float health) noexcept
	{
		m_health = std::clamp(health, 0.0F, 1.0F);
	}

	Color< float >
	LampFlicker::colorForHealth (const Color< float > & healthyColor, float health) noexcept
	{
		const auto clampedHealth = std::clamp(health, 0.0F, 1.0F);

		if ( clampedHealth >= 1.0F )
		{
			return healthyColor;
		}

		/* A cooling filament loses its blue end first, then its green: at health 0 the lamp keeps
		 * all of its red, 55% of its green and 20% of its blue. Applied as a MULTIPLIER on the
		 * authored colour so a lamp that was never white does not turn white on the way down. */
		const auto sag = 1.0F - clampedHealth;

		return {
			healthyColor.red(),
			healthyColor.green() * (1.0F - (0.45F * sag)),
			healthyColor.blue() * (1.0F - (0.80F * sag)),
			healthyColor.alpha()
		};
	}

	float
	LampFlicker::nextLevel () noexcept
	{
		/* A lamp in perfect health does not flicker at all — skip the machinery entirely so
		 * attaching this to a healthy lamp costs nothing but a multiply. */
		if ( m_health >= 1.0F )
		{
			return 1.0F;
		}

		const auto sag = 1.0F - m_health;

		/* Still inside a dropout: the contact is open, the lamp is all but out. Not exactly zero
		 * — a filament retains a dull glow through a brief cut, and a hard zero reads as the
		 * light being deleted rather than failing. */
		if ( m_dropoutRemaining > 0 )
		{
			--m_dropoutRemaining;

			return 0.04F;
		}

		/* Roll for a new dropout. Quadratic in the sag so a lamp that is merely tired stays
		 * watchable, while a dying one cuts out constantly. The burst window multiplies the
		 * odds, which is what groups the cuts instead of spreading them evenly. */
		const auto burstFactor = m_burstRemaining > 0 ? BurstChanceMultiplier : 1.0F;

		if ( m_randomizer.value(0.0F, 1.0F) < (sag * sag * 0.0025F * burstFactor) )
		{
			m_dropoutRemaining = static_cast< uint32_t >(m_randomizer.value(static_cast< float >(MinDropoutCycles), static_cast< float >(MaxDropoutCycles)));
			m_burstRemaining = BurstWindowCycles;

			return 0.04F;
		}

		if ( m_burstRemaining > 0 )
		{
			--m_burstRemaining;
		}

		/* Between the cuts the lamp BREATHES: two incommensurate oscillators plus a little grain,
		 * around a mean that drops as the lamp weakens. */
		m_phase += 1.0F / WorldPhysicsUpdateFrequency< float >;

		constexpr auto Tau = 2.0F * std::numbers::pi_v< float >;

		const auto breath =
			(std::sin(m_phase * SlowBreathHz * Tau) * 0.6F) +
			(std::sin(m_phase * FastBreathHz * Tau) * 0.4F);

		const auto mean = 1.0F - (0.55F * sag);
		const auto amplitude = 0.35F * sag;
		const auto grain = m_randomizer.value(-0.04F, 0.04F) * sag;

		return std::clamp(mean + (breath * amplitude) + grain, 0.0F, 1.0F);
	}

	Variant
	LampFlicker::getNextValue () noexcept
	{
		return Variant{m_nominalCandela * this->nextLevel()};
	}
}
