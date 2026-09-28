/*
 * src/Animations/FlameFlicker.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>

/* Local inclusions for inheritances. */
#include "AnimationInterface.hpp"

/* Local inclusions for usages. */
#include "Randomizer.hpp"

namespace EmEn::Animations
{
	/**
	 * @brief Animation source reproducing the light of an open flame — a torch, a brazier, a campfire.
	 * @note Three components, summed around the nominal level (the MEAN output is the nominal intensity):
	 * - the PUFFING of a buoyant diffusion flame: it sheds vortices at a frequency set by its base
	 *   diameter alone, f ≈ 1.5 / √D Hz (B. M. Cetegen & T. A. Ahmed, "Experiments on the periodic
	 *   instability of buoyant plumes and pool fires", Combustion and Flame 93, 1993) — ~5 Hz for an 8 cm
	 *   torch head, ~2.4 Hz for a 40 cm brazier. Its phase DRIFTS (a random walk), because a real flame
	 *   puffs irregularly, not like a metronome;
	 * - a broadband turbulent flicker, an Ornstein-Uhlenbeck process (mean-reverting noise, correlation
	 *   time 0.15 s);
	 * - occasional GUSTS that lay the flame down: a smooth dip of 25-50 % over 0.4-1.2 s.
	 * Every instance takes its own seed: two fires never beat together.
	 * @note Feeds a LUMINOUS INTENSITY in candela, so it plugs straight into the `Intensity` animation id of
	 * any light emitter (like LampFlicker, which models a failing bulb, not a flame):
	 * @code
	 * light->addAnimation(Component::PointLight::Intensity, std::make_shared< FlameFlicker >(nominalCandela, 0.08F, seed));
	 * @endcode
	 * @extends EmEn::Animations::AnimationInterface This is an animation.
	 */
	class EMEN_API FlameFlicker final : public AnimationInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"FlameFlicker"};

			/**
			 * @brief Constructs a flame flicker.
			 * @param nominalCandela The MEAN luminous intensity of the flame, in candela.
			 * @param baseDiameter The diameter of the flame's base, in metres (a torch head 0.08, a brazier 0.4).
			 * @param seed The seed of this flame's randomness: give every fire its own.
			 */
			FlameFlicker (float nominalCandela, float baseDiameter, uint32_t seed) noexcept;

			/** @copydoc EmEn::Animations::AnimationInterface::getNextValue() */
			Base::Variant getNextValue () noexcept override;

			/** @copydoc EmEn::Animations::AnimationInterface::isPlaying() */
			[[nodiscard]]
			bool
			isPlaying () const noexcept override
			{
				return !m_paused;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::isPaused() */
			[[nodiscard]]
			bool
			isPaused () const noexcept override
			{
				return m_paused;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::isFinished() */
			[[nodiscard]]
			bool
			isFinished () const noexcept override
			{
				/* NOTE: A fire burns until someone puts it out. */
				return false;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::play() */
			bool
			play () noexcept override
			{
				m_paused = false;

				return true;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::pause() */
			bool
			pause () noexcept override
			{
				m_paused = true;

				return true;
			}

			/**
			 * @brief Returns the puffing frequency of a buoyant diffusion flame (Cetegen & Ahmed 1993).
			 * @param baseDiameter The diameter of the flame's base, in metres (clamped to 1 cm at least).
			 * @return float The frequency, in hertz.
			 */
			[[nodiscard]]
			static float puffingFrequency (float baseDiameter) noexcept;

			/**
			 * @brief Returns the puffing frequency of this flame.
			 * @return float The frequency, in hertz.
			 */
			[[nodiscard]]
			float
			frequency () const noexcept
			{
				return m_frequency;
			}

		private:

			/**
			 * @brief Rolls the flame forward by one logic cycle.
			 * @return float The output level around 1 (the mean), clamped to [MinimumLevel, MaximumLevel].
			 */
			[[nodiscard]]
			float nextLevel () noexcept;

			/**
			 * @brief Draws a standard normal deviate (Box-Muller, from two uniform draws).
			 * @return float
			 */
			[[nodiscard]]
			float gaussian () noexcept;

			/* Relative amplitude of the puffing sine, and the phase's random walk in radians per √s. */
			static constexpr auto PuffAmplitude{0.10F};
			static constexpr auto PhaseDiffusion{2.0F};
			/* The turbulent flicker: its standard deviation and its correlation time, in seconds. */
			static constexpr auto TurbulenceDeviation{0.08F};
			static constexpr auto TurbulenceTime{0.15F};
			/* Gusts: how many per second on average, their duration range (s) and depth range (fraction). */
			static constexpr auto GustRate{0.08F};
			static constexpr auto GustMinimumDuration{0.4F};
			static constexpr auto GustMaximumDuration{1.2F};
			static constexpr auto GustMinimumDepth{0.25F};
			static constexpr auto GustMaximumDepth{0.5F};
			/* A flame is never out, nor more than half again as bright as its mean. */
			static constexpr auto MinimumLevel{0.2F};
			static constexpr auto MaximumLevel{1.5F};

			Base::Randomizer< float > m_randomizer;
			float m_nominalCandela;
			float m_frequency;
			float m_phase{0.0F};
			float m_turbulence{0.0F};
			float m_gustElapsed{0.0F};
			float m_gustDuration{0.0F};
			float m_gustDepth{0.0F};
			bool m_paused{false};
	};
}