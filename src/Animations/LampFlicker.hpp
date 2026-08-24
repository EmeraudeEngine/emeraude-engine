/*
 * src/Animations/LampFlicker.hpp
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
#include "PixelFactory/Color.hpp"
#include "Randomizer.hpp"
#include "Variant.hpp"

namespace EmEn::Animations
{
	/**
	 * @brief Animation source reproducing an ailing lamp: a filament that sags and breathes,
	 * punctuated by the brutal dropouts of a failing contact.
	 * @note Feeds a LUMINOUS INTENSITY in candela, so it plugs straight into the `Intensity`
	 * animation id of any light emitter:
	 * @code
	 * light->addAnimation(Component::SpotLight::Intensity, std::make_shared< LampFlicker >(nominalCandela, 0.4F));
	 * @endcode
	 * @warning This drives the INTENSITY only. The colour drift of a dying lamp tracks the
	 * average state of its power source — minutes — not the individual flickers, so it is a
	 * function of the health and is applied ONCE with `colorForHealth()` rather than animated.
	 * Animating both would also need two `AnimationInterface` registrations, which
	 * `AnimatableInterface::updateAnimations()` advances independently: they would desynchronise.
	 * @extends EmEn::Animations::AnimationInterface This is an animation.
	 */
	class EMEN_API LampFlicker final : public AnimationInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LampFlicker"};

			/**
			 * @brief Constructs a lamp flicker.
			 * @param nominalCandela The luminous intensity of the lamp in perfect health.
			 * @param health The condition of the lamp, 1 = new, 0 = dead. Default 1.
			 */
			explicit LampFlicker (float nominalCandela, float health = 1.0F) noexcept;

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
				/* NOTE: A lamp never stops being a lamp. */
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
			 * @brief Sets the condition of the lamp.
			 * @note Continuous on purpose: a lamp can degrade during play by walking this down.
			 * @param health 1 = new, 0 = dead.
			 * @return void
			 */
			void setHealth (float health) noexcept;

			/**
			 * @brief Returns the condition of the lamp.
			 * @return float
			 */
			[[nodiscard]]
			float
			health () const noexcept
			{
				return m_health;
			}

			/**
			 * @brief Returns the colour a lamp emits at a given health.
			 * @note A weakening supply runs the filament cooler, so the light reddens — a real
			 * effect on an incandescent source, and the cue that reads as "dying" before the
			 * flicker is even noticed. Apply it with `Component::PointLight::setColor()` when
			 * the health changes; do NOT animate it (see the class warning).
			 * @param healthyColor The colour of the lamp in perfect health.
			 * @param health 1 = new, 0 = dead.
			 * @return Base::PixelFactory::Color< float >
			 */
			[[nodiscard]]
			static Base::PixelFactory::Color< float > colorForHealth (const Base::PixelFactory::Color< float > & healthyColor, float health) noexcept;

		private:

			/**
			 * @brief Rolls the state machine forward by one logic cycle.
			 * @return float The output level, 0 to 1.
			 */
			[[nodiscard]]
			float nextLevel () noexcept;

			/* The two breathing oscillators are deliberately INCOMMENSURATE: their ratio is
			 * irrational, so their sum never repeats and the sag cannot be heard as a loop. */
			static constexpr auto SlowBreathHz{3.1F};
			static constexpr auto FastBreathHz{7.7F};

			/* Logic cycles. A dropout is 2 to 12 cycles, i.e. roughly 30 to 200 ms — long enough
			 * to be seen as a cut, short enough to read as a fault rather than a switch. */
			static constexpr auto MinDropoutCycles{2};
			static constexpr auto MaxDropoutCycles{12};

			/* After a dropout the contact stays unreliable for a moment, which is what produces
			 * the characteristic BURSTS of two or three cuts instead of evenly spread ones. */
			static constexpr auto BurstWindowCycles{45};
			static constexpr auto BurstChanceMultiplier{12.0F};

			Base::Randomizer< float > m_randomizer{};
			float m_nominalCandela;
			float m_health{1.0F};
			float m_phase{0.0F};
			uint32_t m_dropoutRemaining{0};
			uint32_t m_burstRemaining{0};
			bool m_paused{false};
	};
}
