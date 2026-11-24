/*
 * src/Scenes/ParticipatingMedium.cpp
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

#include "ParticipatingMedium.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace EmEn::Scenes
{
	using namespace Base::Math;

	namespace
	{
		/**
		 * @brief Returns a coefficient a medium can hold: finite and positive, else none.
		 * @param value The stated coefficient, in 1/m.
		 * @return float
		 */
		[[nodiscard]]
		float
		heldCoefficient (float value) noexcept
		{
			return std::isfinite(value) && value > 0.0F ? value : 0.0F;
		}
	}

	Vector< 3, float >
	ParticipatingMedium::transmittance (float distance) const noexcept
	{
		/* NOTE: std::max() keeps its first argument against a NaN: no path. */
		const auto length = std::max(0.0F, distance);

		Vector< 3, float > kept{1.0F, 1.0F, 1.0F};

		for ( size_t channel = 0; channel < 3; ++channel )
		{
			const auto extinction = m_absorption[channel] + m_scattering[channel];

			/* NOTE: A channel without extinction is skipped, never multiplied: 0 × ∞ is not a number. */
			if ( extinction > 0.0F )
			{
				kept[channel] = std::exp(-extinction * length);
			}
		}

		return kept;
	}

	float
	ParticipatingMedium::backscatterFraction () const noexcept
	{
		/* The backward hemisphere of the Henyey-Greenstein phase function, integrated:
		 * B(g) = (1 − g) / (2 g) · ((1 + g) / sqrt(1 + g²) − 1), 1/2 at g = 0. */
		const auto anisotropy = m_phaseAnisotropy;

		if ( std::abs(anisotropy) < 1.0e-3F )
		{
			return 0.5F;
		}

		return ((1.0F - anisotropy) / (2.0F * anisotropy)) * (((1.0F + anisotropy) / std::sqrt(1.0F + (anisotropy * anisotropy))) - 1.0F);
	}

	void
	ParticipatingMedium::setAbsorption (const Vector< 3, float > & perMetre) noexcept
	{
		for ( size_t channel = 0; channel < 3; ++channel )
		{
			m_absorption[channel] = heldCoefficient(perMetre[channel]);
		}
	}

	void
	ParticipatingMedium::setScattering (const Vector< 3, float > & perMetre) noexcept
	{
		for ( size_t channel = 0; channel < 3; ++channel )
		{
			m_scattering[channel] = heldCoefficient(perMetre[channel]);
		}
	}
}
