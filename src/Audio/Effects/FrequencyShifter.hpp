/*
 * src/Audio/Effects/FrequencyShifter.hpp
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
#include "Abstract.hpp"

namespace EmEn::Audio::Effects
{
	/**
	 * @brief Frequency shifter effect.
	 * @warning Support broken on OpenAL Software 1.19.1.
	 * @extends EmEn::Audio::Effects::Abstract This is an effect.
	 */
	class FrequencyShifter final : public Abstract
	{
		public:

			/** @brief The direction enumerations. */
			enum class Direction
			{
				Down,
				Up,
				Off
			};

			/** @brief Class identifier. */
			static constexpr auto ClassId{"FrequencyShifter"};

			/**
			 * @brief Constructs a frequency shifter effect.
			 */
			FrequencyShifter () noexcept;

			/** @copydoc EmEn::Audio::Effects::Abstract::resetProperties() */
			void resetProperties () noexcept override;

			/**
			 * @brief Sets the frequency.
			 * @param value A value between 0 and 24000.
			 */
			void setFrequency (float value) noexcept;

			/**
			 * @brief Sets the left direction.
			 * @param value A direction type.
			 */
			void setLeftDirection (Direction value) noexcept;

			/**
			 * @brief Sets the right direction.
			 * @param value A direction type.
			 */
			void setRightDirection (Direction value) noexcept;

			/**
			 * @brief Returns the frequency value.
			 * @return float
			 */
			[[nodiscard]]
			float frequency () const noexcept;

			/**
			 * @brief Returns the left direction type.
			 * @return Direction
			 */
			[[nodiscard]]
			Direction leftDirection () const noexcept;

			/**
			 * @brief Returns the right direction type.
			 * @return Direction
			 */
			[[nodiscard]]
			Direction rightDirection () const noexcept;
	};
}
