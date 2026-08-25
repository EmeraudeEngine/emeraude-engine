/*
 * src/Scenes/ParticipatingMedium.hpp
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
#include <algorithm>
#include <cmath>
#include <numbers>
#include <ostream>
#include <string>

/* Local inclusions for usages. */
#include "PixelFactory/Color.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The participating medium of a scene — the ONE atmosphere every volumetric consumer
	 * integrates.
	 * @note ⚠️ This exists because the medium had NO owner. Its parameters lived inside
	 * @code AtmosphericFog::Parameters @endcode, private to one effect instance, and effects in a
	 * post-process stack cannot see each other. AtmosphericFog is used by ONE demo while
	 * VolumetricLight is used by EIGHT, so "share the fog's medium" had no instance to share with in
	 * seven cases out of eight. A medium is a property of the SCENE, like its gravity.
	 * @note ⚠️ Do NOT confuse this with @code VolumetricLight::Parameters::density @endcode, which is
	 * a screen-space step multiplier for a radial blur and carries no physical unit whatsoever.
	 */
	class EMEN_API ParticipatingMedium final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ParticipatingMedium"};

			/**
			 * @brief Constructs a vacuum.
			 */
			ParticipatingMedium () noexcept = default;

			/**
			 * @brief Changes the extinction coefficient at the base height.
			 * @param density The coefficient in 1/m. 0 means vacuum.
			 * @return void
			 */
			void
			setDensity (float density) noexcept
			{
				m_density = std::max(0.0F, density);
			}

			/**
			 * @brief Returns the extinction coefficient σt at the base height, in 1/m.
			 * @return float
			 */
			[[nodiscard]]
			float
			density () const noexcept
			{
				return m_density;
			}

			/**
			 * @brief Changes how fast the density falls off with altitude.
			 * @warning ⚠️ POSITIVE decay rate. The shader NEGATES it; passing a negative value makes
			 * the medium grow denser with altitude, which is exactly the defect the Y-up migration
			 * left in AtmosphericFog for months. Same contract as the effect it replaces.
			 * @param falloff The decay rate in 1/m.
			 * @return void
			 */
			void
			setHeightFalloff (float falloff) noexcept
			{
				m_heightFalloff = std::max(0.0F, falloff);
			}

			/**
			 * @brief Returns the POSITIVE decay rate, in 1/m.
			 * @return float
			 */
			[[nodiscard]]
			float
			heightFalloff () const noexcept
			{
				return m_heightFalloff;
			}

			/**
			 * @brief Changes the altitude the density is expressed at.
			 * @param height The world-space Y, in m.
			 * @return void
			 */
			void
			setBaseHeight (float height) noexcept
			{
				m_baseHeight = height;
			}

			/**
			 * @brief Returns the reference altitude, in m.
			 * @return float
			 */
			[[nodiscard]]
			float
			baseHeight () const noexcept
			{
				return m_baseHeight;
			}

			/**
			 * @brief Changes the integration clamp.
			 * @param distance The distance in m.
			 * @return void
			 */
			void
			setMaxDistance (float distance) noexcept
			{
				m_maxDistance = std::max(0.0F, distance);
			}

			/**
			 * @brief Returns the integration clamp, in m.
			 * @return float
			 */
			[[nodiscard]]
			float
			maxDistance () const noexcept
			{
				return m_maxDistance;
			}

			/**
			 * @brief Changes the single-scattering albedo.
			 * @warning ⚠️ A CHROMATICITY in [0,1], never a luminance. The luminance is carried
			 * separately by luminance()/resolveLuminance(), because the scene colour buffer holds
			 * ABSOLUTE luminance in nits: a [0,1] constant composited there reads black at any real
			 * exposure. That mistake shipped for months, and it is invisible in review precisely
			 * because the numbers look like a colour.
			 * @param albedo A reference to a color.
			 * @return void
			 */
			void
			setScatteringAlbedo (const Base::PixelFactory::Color< float > & albedo) noexcept
			{
				m_scatteringAlbedo = albedo;
			}

			/**
			 * @brief Returns the single-scattering albedo, a chromaticity in [0,1].
			 * @return const Base::PixelFactory::Color< float > &
			 */
			[[nodiscard]]
			const Base::PixelFactory::Color< float > &
			scatteringAlbedo () const noexcept
			{
				return m_scatteringAlbedo;
			}

			/**
			 * @brief Changes the Henyey-Greenstein anisotropy.
			 * @param anisotropy The g parameter, in ]-1,1[. 0 is isotropic, positive is forward.
			 * @return void
			 */
			void
			setPhaseAnisotropy (float anisotropy) noexcept
			{
				m_phaseAnisotropy = std::clamp(anisotropy, -0.99F, 0.99F);
			}

			/**
			 * @brief Returns the Henyey-Greenstein anisotropy g.
			 * @return float
			 */
			[[nodiscard]]
			float
			phaseAnisotropy () const noexcept
			{
				return m_phaseAnisotropy;
			}

			/**
			 * @brief Changes the absolute luminance of the medium.
			 * @param luminance The luminance in NITS, or a negative value to derive it from the
			 * scene's main directional light.
			 * @return void
			 */
			void
			setLuminance (float luminance) noexcept
			{
				m_luminance = luminance;
			}

			/**
			 * @brief Returns the absolute luminance in nits, or a negative value when it is derived.
			 * @return float
			 */
			[[nodiscard]]
			float
			luminance () const noexcept
			{
				return m_luminance;
			}

			/**
			 * @brief Returns the absolute luminance to composite with, in nits.
			 * @note ONE definition for every consumer. When the scene states no luminance, it is
			 * derived as L = E · ρ / π — the Lambertian relation used everywhere else in the engine,
			 * with E the illuminance in lux and ρ the scattering albedo carried separately.
			 * @param directionalIlluminance The main directional light illuminance, in lux.
			 * @return float
			 */
			[[nodiscard]]
			float
			resolveLuminance (float directionalIlluminance) const noexcept
			{
				if ( m_luminance >= 0.0F )
				{
					return m_luminance;
				}

				return directionalIlluminance / std::numbers::pi_v< float >;
			}

			/**
			 * @brief Returns the extinction coefficient at an arbitrary altitude, in 1/m.
			 * @note The exponential height profile a ray-march or a froxel injection samples.
			 * @param altitude The world-space Y, in m.
			 * @return float
			 */
			[[nodiscard]]
			float
			densityAt (float altitude) const noexcept
			{
				return m_density * std::exp(-m_heightFalloff * (altitude - m_baseHeight));
			}

			/**
			 * @brief Changes whether the medium takes part in rendering.
			 * @param state The state.
			 * @return void
			 */
			void
			setEnabled (bool state) noexcept
			{
				m_enabled = state;
			}

			/**
			 * @brief Returns whether the medium takes part in rendering.
			 * @note A zero density is vacuum whatever the flag says.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return m_enabled && m_density > 0.0F;
			}

			/**
			 * @brief Returns a vacuum — the DEFAULT for every scene.
			 * @note ⚠️ The default must stay vacuum: a non-zero default would put fog in every scene
			 * of the engine at once.
			 * @return ParticipatingMedium
			 */
			[[nodiscard]]
			static
			ParticipatingMedium
			Vacuum () noexcept
			{
				return {};
			}

			/**
			 * @brief Returns a clear-air medium, the barely visible aerial perspective of a sunny day.
			 * @return ParticipatingMedium
			 */
			[[nodiscard]]
			static ParticipatingMedium ClearAir () noexcept;

			/**
			 * @brief Returns a fog bank.
			 * @return ParticipatingMedium
			 */
			[[nodiscard]]
			static ParticipatingMedium Fog () noexcept;

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend EMEN_API std::ostream & operator<< (std::ostream & out, const ParticipatingMedium & obj);

		private:

			Base::PixelFactory::Color< float > m_scatteringAlbedo{0.5F, 0.6F, 0.7F, 1.0F};
			float m_density{0.0F};
			float m_heightFalloff{0.2F};
			float m_baseHeight{0.0F};
			float m_maxDistance{10000.0F};
			float m_phaseAnisotropy{0.0F};
			float m_luminance{-1.0F};
			bool m_enabled{false};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	[[nodiscard]]
	EMEN_API std::string to_string (const ParticipatingMedium & obj) noexcept;
}
