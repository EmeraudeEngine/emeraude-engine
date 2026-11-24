/*
 * src/Scenes/WaterMedium.cpp
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

#include "WaterMedium.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <sstream>

/* Local inclusions. */
#include "Math/Vector.hpp"

namespace EmEn::Scenes
{
	using namespace Base::Math;

	namespace
	{
		/**
		 * @brief Returns the optical depth that leaves a share of the light: −ln(share).
		 * @param kept The share kept, a channel of the attenuation colour.
		 * @return float
		 */
		[[nodiscard]]
		float
		opticalDepthOf (float kept) noexcept
		{
			/* NOTE: std::clamp() hands a NaN back: a channel that is not a number keeps everything. */
			const auto share = std::isnan(kept) ? 1.0F : std::clamp(kept, WaterMedium::MinAttenuationColor, 1.0F);

			return -std::log(share);
		}
	}

	void
	WaterMedium::setAttenuation (const Base::PixelFactory::Color< float > & color, float distance) noexcept
	{
		m_attenuationColor = color;
		/* NOTE: std::max() keeps its first argument against a NaN. */
		m_attenuationDistance = std::max(MinAttenuationDistance, distance);

		this->setAbsorption({
			opticalDepthOf(color.red()) / m_attenuationDistance,
			opticalDepthOf(color.green()) / m_attenuationDistance,
			opticalDepthOf(color.blue()) / m_attenuationDistance
		});
	}

	Vector< 3, float >
	WaterMedium::diffuseAttenuation (float downwellingCosine) const noexcept
	{
		/* NOTE: std::max() keeps its first argument against a NaN; a light that does not go down has no cosine. */
		const auto cosine = std::max(1.0e-3F, std::min(downwellingCosine, 1.0F));

		return (this->absorption() + (this->scattering() * this->backscatterFraction())) * (1.0F / cosine);
	}

	Vector< 3, float >
	WaterMedium::diffuseReflectance () const noexcept
	{
		const auto & absorption = this->absorption();
		const auto backscattering = this->scattering() * this->backscatterFraction();

		Vector< 3, float > reflectance{0.0F, 0.0F, 0.0F};

		for ( size_t channel = 0; channel < 3; ++channel )
		{
			const auto loss = absorption[channel] + backscattering[channel];

			if ( loss > 0.0F )
			{
				reflectance[channel] = IrradianceReflectanceFactor * backscattering[channel] / loss;
			}
		}

		return reflectance;
	}

	WaterMedium
	WaterMedium::Lagoon () noexcept
	{
		WaterMedium water;
		water.setAttenuation({0.30F, 0.75F, 0.80F, 1.0F}, 12.0F);
		/* About 35 m of horizontal visibility (4.8 / the green extinction), the particles scattering a little more
		 * toward the blue. ⚠️ First values, to be judged on screen with the owner (engine item underwater-view, step 5). */
		water.setScattering({0.085F, 0.105F, 0.115F});
		water.setPhaseAnisotropy(0.8F);
		water.setEnabled(true);

		return water;
	}

	WaterMedium
	WaterMedium::DeepOcean () noexcept
	{
		WaterMedium water;
		water.setAttenuation({0.05F, 0.35F, 0.45F, 1.0F}, 5.0F);
		/* ⚠️ First values, never judged on screen. */
		water.setScattering({0.03F, 0.04F, 0.05F});
		water.setPhaseAnisotropy(0.8F);
		water.setEnabled(true);

		return water;
	}

	WaterMedium
	WaterMedium::PeatLake () noexcept
	{
		WaterMedium water;
		water.setAttenuation({0.12F, 0.16F, 0.06F, 1.0F}, 2.5F);
		/* ⚠️ First values, never judged on screen. */
		water.setScattering({0.45F, 0.50F, 0.40F});
		water.setPhaseAnisotropy(0.8F);
		water.setEnabled(true);

		return water;
	}

	std::ostream &
	operator<< (std::ostream & out, const WaterMedium & obj)
	{
		const auto & absorption = obj.absorption();
		const auto & scattering = obj.scattering();

		return out <<
			"Water medium :" "\n"
			"Enabled : " << ( obj.isEnabled() ? "yes" : "no" ) << "\n"
			"Attenuation colour (kept after the distance) : " << obj.m_attenuationColor << "\n"
			"Attenuation distance : " << obj.m_attenuationDistance << " m" "\n"
			"Absorption (red, green, blue) : " << absorption[X] << ", " << absorption[Y] << ", " << absorption[Z] << " 1/m" "\n"
			"Scattering (red, green, blue) : " << scattering[X] << ", " << scattering[Y] << ", " << scattering[Z] << " 1/m" "\n"
			"Phase anisotropy (Henyey-Greenstein g) : " << obj.phaseAnisotropy() << "\n";
	}

	std::string
	to_string (const WaterMedium & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
