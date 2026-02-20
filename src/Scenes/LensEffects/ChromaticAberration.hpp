/*
 * src/Scenes/LensEffects/ChromaticAberration.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Local inclusions for inheritances. */
#include "Saphir/FramebufferEffectInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"

namespace EmEn::Scenes::LensEffects
{
	/**
	 * @brief The chromatic aberration lens effect class.
	 * @extends EmEn::Saphir::FramebufferEffectInterface This is a framebuffer effect.
	 */
	class ChromaticAberration final : public Saphir::FramebufferEffectInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ChromaticAberration"};

			/**
			 * @brief Constructs a chromatic aberration lens effect.
			 * @param intensity The maximum offset magnitude for each RGB channel. Default 0.005.
			 */
			explicit ChromaticAberration (float intensity = 0.005F) noexcept;

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::generateFragmentShaderCode() */
			[[nodiscard]]
			bool generateFragmentShaderCode (Saphir::Generator::Abstract & generator, Saphir::FragmentShader & fragmentShader) const noexcept override;

			/**
			 * @brief Sets the aberration intensity (maximum UV offset magnitude).
			 * @param intensity Value > 0.
			 * @return void
			 */
			void setIntensity (float intensity) noexcept;

			/**
			 * @brief Returns the aberration intensity.
			 * @return float
			 */
			[[nodiscard]]
			float
			intensity () const noexcept
			{
				return m_intensity;
			}

			/**
			 * @brief Enables or disables radial mode.
			 * @note In radial mode, aberration is zero at the image center and increases
			 * toward the edges, simulating real lens dispersion. Each RGB channel is
			 * displaced along the radial direction from center with a different scale.
			 * @param enabled True for radial, false for linear (default).
			 * @return void
			 */
			void
			enableRadial (bool enabled) noexcept
			{
				m_radial = enabled;
			}

			/**
			 * @brief Returns whether radial mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			radialEnabled () const noexcept
			{
				return m_radial;
			}

			/**
			 * @brief Enables or disables barrel distortion.
			 * @note Barrel distortion simulates CRT glass curvature by warping UV coordinates
			 * before RGB sampling. Combinable with both linear and radial modes.
			 * @param enabled True to enable barrel distortion.
			 * @return void
			 */
			void
			enableBarrelDistortion (bool enabled) noexcept
			{
				m_barrelEnabled = enabled;
			}

			/**
			 * @brief Returns whether barrel distortion is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			barrelDistortionEnabled () const noexcept
			{
				return m_barrelEnabled;
			}

			/**
			 * @brief Sets the barrel distortion strength.
			 * @param strength Curvature magnitude (0 = flat, higher = more curved). Default 0.1.
			 * @return void
			 */
			void setBarrelStrength (float strength) noexcept;

			/**
			 * @brief Returns the barrel distortion strength.
			 * @return float
			 */
			[[nodiscard]]
			float
			barrelStrength () const noexcept
			{
				return m_barrelStrength;
			}

		private:

			/** @copydoc EmEn::Saphir::FramebufferEffectInterface::overrideFragmentFetching() */
			[[nodiscard]]
			bool
			overrideFragmentFetching () const noexcept override
			{
				return true;
			}

			float m_intensity{0.005F};
			bool m_radial{false};
			bool m_barrelEnabled{false};
			float m_barrelStrength{0.1F};
			Libs::Math::Vector< 2, float > m_redOffset{};
			Libs::Math::Vector< 2, float > m_greenOffset{};
			Libs::Math::Vector< 2, float > m_blueOffset{};
	};
}
