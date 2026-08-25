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
#include <sstream>

namespace EmEn::Scenes
{
	ParticipatingMedium
	ParticipatingMedium::ClearAir () noexcept
	{
		/* The barely visible aerial perspective of a clear day: a kilometre of it removes about
		 * three quarters of the contrast, which is what makes a distant ridge read as distant. */
		ParticipatingMedium medium;
		medium.setDensity(0.0015F);
		medium.setHeightFalloff(0.2F);
		medium.setScatteringAlbedo({0.55F, 0.65F, 0.8F, 1.0F});
		medium.setPhaseAnisotropy(0.3F);
		medium.setEnabled(true);

		return medium;
	}

	ParticipatingMedium
	ParticipatingMedium::Fog () noexcept
	{
		/* A ground-hugging bank: dense, strongly forward-scattering, and thinning fast with
		 * altitude so a hill emerges from it. */
		ParticipatingMedium medium;
		medium.setDensity(0.05F);
		medium.setHeightFalloff(0.35F);
		medium.setScatteringAlbedo({0.65F, 0.68F, 0.72F, 1.0F});
		medium.setPhaseAnisotropy(0.6F);
		medium.setEnabled(true);

		return medium;
	}

	std::ostream &
	operator<< (std::ostream & out, const ParticipatingMedium & obj)
	{
		return out <<
			"Participating medium :" "\n"
			"Enabled : " << ( obj.isEnabled() ? "yes" : "no" ) << "\n"
			"Density (extinction at base height) : " << obj.m_density << " 1/m" "\n"
			"Height falloff : " << obj.m_heightFalloff << " 1/m (POSITIVE, the shader negates it)" "\n"
			"Base height : " << obj.m_baseHeight << " m" "\n"
			"Max distance : " << obj.m_maxDistance << " m" "\n"
			"Scattering albedo (chromaticity) : " << obj.m_scatteringAlbedo << "\n"
			"Phase anisotropy (Henyey-Greenstein g) : " << obj.m_phaseAnisotropy << "\n"
			"Luminance : " << ( obj.m_luminance < 0.0F ? std::string{"derived from the main directional light"} : std::to_string(obj.m_luminance) + " nits" ) << "\n";
	}

	std::string
	to_string (const ParticipatingMedium & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
