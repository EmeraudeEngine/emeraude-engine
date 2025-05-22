/*
 * src/Animations/RandomValue.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include "AnimationInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/Variant.hpp"

namespace EmEn::Animations
{
	/**
	 * @brief Animation using a simple random value.
	 * @note ie, This can be used to randomize something each logic cycle.
	 * @extends EmEn::Animations::AnimationInterface This is an animation.
	 */
	class RandomValue final : public AnimationInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RandomValue"};

			/**
			 * @brief Constructs a random value.
			 */
			RandomValue () noexcept = default;

			/**
			 * @brief Constructs a random value.
			 * @param minimum A reference to a minimum value.
			 * @param maximum A reference to a maximum value.
			 */
			RandomValue (const Libs::Variant & minimum, const Libs::Variant & maximum) noexcept
			{
				this->setValue(minimum, maximum);
			}

			/** @copydoc EmEn::Animations::AnimationInterface::getNextValue() */
			Libs::Variant getNextValue () noexcept override;

			/** @copydoc EmEn::Animations::AnimationInterface::isPlaying() */
			[[nodiscard]]
			bool
			isPlaying () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::isPaused() */
			[[nodiscard]]
			bool
			isPaused () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::isFinished() */
			[[nodiscard]]
			bool
			isFinished () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::play() */
			bool
			play () noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Animations::AnimationInterface::pause() */
			bool
			pause () noexcept override
			{
				return false;
			}

			/**
			 * @brief Sets the new random value.
			 * @param minimum A reference to a minimum value.
			 * @param maximum A reference to a maximum value.
			 */
			void setValue (const Libs::Variant & minimum, const Libs::Variant & maximum) noexcept;

		private:

			Libs::Variant m_minimumValue{};
			Libs::Variant m_maximumValue{};
	};
}
