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
#include <cstddef>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The common level of the participating media: what a ray loses and gains per metre of a medium, whatever
	 * the medium is.
	 * @note A family with levels (owner decision 2026-11-23, engine item underwater-view). This level holds what every
	 * volumetric consumer integrates — the absorption and the scattering per channel, the phase anisotropy, and whether
	 * the medium takes part in rendering. Below it, each member states the same physics in its own terms and keeps
	 * what is its own: AtmosphericMedium (the air of a scene: a density thinning with the altitude, an albedo, a
	 * luminance) and WaterMedium (the water of a sea: homogeneous, stated as the colour the light keeps after a
	 * distance).
	 * @note A VALUE type, never used polymorphically: no virtual function, a protected destructor, each member is final.
	 * @note The channels are the engine's linear RGB, in the order (red, green, blue) of the vector's (x, y, z).
	 */
	class EMEN_API ParticipatingMedium
	{
		public:

			/** @brief The lowest and the highest Henyey-Greenstein anisotropy a medium keeps (the phase function is singular at ±1). */
			static constexpr auto MaxPhaseAnisotropy{0.99F};

			/**
			 * @brief Returns the absorption coefficient σa per channel, in 1/m: the light the medium removes.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			absorption () const noexcept
			{
				return m_absorption;
			}

			/**
			 * @brief Returns the scattering coefficient σs per channel, in 1/m: the light the medium redirects.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			scattering () const noexcept
			{
				return m_scattering;
			}

			/**
			 * @brief Returns the extinction coefficient σt = σa + σs per channel, in 1/m: what a ray loses per metre.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			extinction () const noexcept
			{
				return m_absorption + m_scattering;
			}

			/**
			 * @brief Returns what a ray keeps after a distance in the medium, per channel: exp(−σt · distance).
			 * @note A channel without extinction keeps everything, at any distance, the infinite one included.
			 * @param distance The length of the path in the medium, in m. Negative or not a number: no path, 1.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > transmittance (float distance) const noexcept;

			/**
			 * @brief Changes the Henyey-Greenstein anisotropy.
			 * @param anisotropy The g parameter, in ]-1,1[. 0 is isotropic, positive is forward. Clamped to
			 * ±MaxPhaseAnisotropy; not a number: 0.
			 */
			void
			setPhaseAnisotropy (float anisotropy) noexcept
			{
				/* NOTE: std::clamp() hands a NaN back, and the phase function then poisons every pixel it touches. */
				m_phaseAnisotropy = std::isnan(anisotropy) ? 0.0F : std::clamp(anisotropy, -MaxPhaseAnisotropy, MaxPhaseAnisotropy);
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
			 * @brief Returns the share of a scattering that goes into the BACKWARD hemisphere, for the medium's
			 * Henyey-Greenstein phase function: 1/2 when isotropic, about 5 % at g = 0.8.
			 * @return float
			 */
			[[nodiscard]]
			float backscatterFraction () const noexcept;

			/**
			 * @brief Changes whether the medium takes part in rendering.
			 * @param state The state.
			 */
			void
			setEnabled (bool state) noexcept
			{
				m_enabled = state;
			}

			/**
			 * @brief Returns whether the medium takes part in rendering.
			 * @note A medium without extinction is a vacuum whatever the flag says.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return m_enabled && this->hasExtinction();
			}

			/**
			 * @brief Returns whether a channel at least loses something per metre.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasExtinction () const noexcept
			{
				for ( size_t channel = 0; channel < 3; ++channel )
				{
					if ( m_absorption[channel] + m_scattering[channel] > 0.0F )
					{
						return true;
					}
				}

				return false;
			}

		protected:

			/**
			 * @brief Constructs a vacuum, disabled.
			 */
			ParticipatingMedium () noexcept = default;

			/**
			 * @brief Destructs the medium (protected and not virtual: a medium is never destroyed through this level).
			 */
			~ParticipatingMedium () = default;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ParticipatingMedium (const ParticipatingMedium & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ParticipatingMedium (ParticipatingMedium && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return ParticipatingMedium &
			 */
			ParticipatingMedium & operator= (const ParticipatingMedium & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return ParticipatingMedium &
			 */
			ParticipatingMedium & operator= (ParticipatingMedium && copy) noexcept = default;

			/**
			 * @brief States the absorption coefficient, a member's own terms restated per channel.
			 * @note A coefficient that is negative, infinite or not a number is no coefficient: that channel takes 0.
			 * @param perMetre σa per channel (red, green, blue), in 1/m.
			 */
			void setAbsorption (const Base::Math::Vector< 3, float > & perMetre) noexcept;

			/**
			 * @brief States the scattering coefficient, a member's own terms restated per channel.
			 * @note A coefficient that is negative, infinite or not a number is no coefficient: that channel takes 0.
			 * @param perMetre σs per channel (red, green, blue), in 1/m.
			 */
			void setScattering (const Base::Math::Vector< 3, float > & perMetre) noexcept;

		private:

			Base::Math::Vector< 3, float > m_absorption{0.0F, 0.0F, 0.0F};
			Base::Math::Vector< 3, float > m_scattering{0.0F, 0.0F, 0.0F};
			float m_phaseAnisotropy{0.0F};
			bool m_enabled{false};
	};
}
