/*
 * src/Scenes/WaterMedium.hpp
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

/* Engine configuration file. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <ostream>
#include <string>

/* Local inclusions for inheritances. */
#include "ParticipatingMedium.hpp"

/* Local inclusions for usages. */
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The WATER of a sea — what the sea level declares once, and what both of its readers integrate: the
	 * surface material for the column seen from above, the underwater view for what lies between the eye and what it
	 * sees (owner decision 2026-11-23, engine item underwater-view: above and below agree by construction).
	 * @note Homogeneous: the same water at every depth. Its absorption is stated the way the engine's materials state
	 * a volume (KHR_materials_volume): the colour the light keeps after a distance — the surface material takes those
	 * two values as they were stated, and the coefficient per metre is derived from them.
	 * @note A default-constructed water is NO water: a sea that declares it is drawn and behaves as before.
	 * @extends EmEn::Scenes::ParticipatingMedium The coefficients every consumer integrates, the phase, the state.
	 */
	class EMEN_API WaterMedium final : public ParticipatingMedium
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"WaterMedium"};

			/** @brief The index of refraction of water. */
			static constexpr auto RefractiveIndex{1.33F};

			/** @brief The reflectance of the surface of water at normal incidence, from the air or from the water: ((n − 1) / (n + 1))². */
			static constexpr auto NormalReflectance{((RefractiveIndex - 1.0F) / (RefractiveIndex + 1.0F)) * ((RefractiveIndex - 1.0F) / (RefractiveIndex + 1.0F))};

			/** @brief The factor f of the irradiance reflectance R = f · bb / (a + bb) (Morel & Prieur 1977: 0.33). */
			static constexpr auto IrradianceReflectanceFactor{0.33F};

			/** @brief The cosine of the downwelling light's angle from the vertical when no sun gives it (a diffuse sky, a refracted high sun). */
			static constexpr auto MeanDownwellingCosine{0.8F};

			/** @brief The darkest a channel of the attenuation colour is taken: a black channel would be an infinite absorption. */
			static constexpr auto MinAttenuationColor{1.0e-6F};

			/** @brief The shortest attenuation distance, in m (the surface material's own floor). */
			static constexpr auto MinAttenuationDistance{1.0e-4F};

			/**
			 * @brief Constructs no water: disabled, without extinction.
			 */
			WaterMedium () noexcept = default;

			/**
			 * @brief States the absorption as the colour the light keeps after a distance in the water.
			 * @note σa = −ln(colour) / distance per channel, the colour taken in [MinAttenuationColor, 1] and the distance
			 * no shorter than MinAttenuationDistance. White keeps everything: no absorption.
			 * @param color The colour kept (linear, per channel in [0, 1]).
			 * @param distance The metres of water after which the light keeps that colour.
			 */
			void setAttenuation (const Base::PixelFactory::Color< float > & color, float distance) noexcept;

			/**
			 * @brief Returns the colour the light keeps after attenuationDistance(), as it was stated.
			 * @return const Base::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Base::PixelFactory::Color< float > &
			attenuationColor () const noexcept
			{
				return m_attenuationColor;
			}

			/**
			 * @brief Returns the metres of water after which the light keeps attenuationColor(), as they were stated.
			 * @return float
			 */
			[[nodiscard]]
			float
			attenuationDistance () const noexcept
			{
				return m_attenuationDistance;
			}

			/**
			 * @brief States the scattering coefficient σs per channel, in 1/m: the particles and the water itself
			 * redirecting the light — what turns a distance into a veil of the water's own colour.
			 * @note The common level's setter, public for a water: σs per channel (red, green, blue); a negative,
			 * infinite or not-a-number channel takes 0.
			 */
			using ParticipatingMedium::setScattering;

			/**
			 * @brief Returns how fast the downwelling light falls off with the depth, per channel, in 1/m:
			 * Kd = (σa + B σs) / μ, B the backward share of the scattering, μ the cosine of the downwelling light.
			 * @param downwellingCosine The cosine of the downwelling light's angle from the vertical, in ]0, 1].
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > diffuseAttenuation (float downwellingCosine = MeanDownwellingCosine) const noexcept;

			/**
			 * @brief Returns the diffuse reflectance of a deep water, per channel: the share of the light that entered
			 * it which comes back up — R = f · bb / (a + bb), bb = B σs the backscattering, f = IrradianceReflectanceFactor.
			 * @note The irradiance reflectance of ocean optics (H. R. Gordon, O. B. Brown, M. M. Jacobs, "Computed
			 * relationships between the inherent and apparent optical properties of a flat homogeneous ocean", Applied
			 * Optics 14(2), 1975; A. Morel, L. Prieur, "Analysis of variations in ocean color", Limnology and
			 * Oceanography 22(4), 1977). A few percent for a clear water: what looks down into the water sees far
			 * less than what looks up, the scattering being forward.
			 * @note What the surface's mirror shows around Snell's window
			 * (written by Graphics::Material::StandardResource::setWaterVolume()).
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > diffuseReflectance () const noexcept;

			/**
			 * @brief Returns clear lagoon water: the sand shows through a few metres of it, turquoise past a dozen.
			 * @note The absorption the owner approved from above on 2026-09-28 ("une belle plage claire"): at 3 m the light
			 * keeps 74 / 93 / 95 %, at 20 m 13 / 62 / 69 %.
			 * @return WaterMedium
			 */
			[[nodiscard]]
			static WaterMedium Lagoon () noexcept;

			/**
			 * @brief Returns deep open-ocean water: dark blue-green within a few metres.
			 * @return WaterMedium
			 */
			[[nodiscard]]
			static WaterMedium DeepOcean () noexcept;

			/**
			 * @brief Returns dark green-brown fresh water: a peaty lake, its bottom gone within a couple of metres.
			 * @return WaterMedium
			 */
			[[nodiscard]]
			static WaterMedium PeatLake () noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend EMEN_API std::ostream & operator<< (std::ostream & out, const WaterMedium & obj);

		private:

			Base::PixelFactory::Color< float > m_attenuationColor{1.0F, 1.0F, 1.0F, 1.0F};
			float m_attenuationDistance{1.0F};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	[[nodiscard]]
	EMEN_API std::string to_string (const WaterMedium & obj) noexcept;
}
