/*
 * src/Libs/GameTools/Dice.hpp
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

/* STL inclusions. */
#include <cstddef>
#include <random>

namespace EmEn::Libs::GameTools
{
	/**
	 * @brief This class define a game dice.
	 * @note This can be used as a coin flip, a dice with 2 face.
	 */
	class Dice final
	{
		public:

			/**
			 * @brief Constructs a dice.
			 * @param faceCount The number of faces. Default 6.
			 */
			explicit Dice (size_t faceCount = 6) noexcept
				: m_faceCount(std::max(2UL, faceCount))
			{
				this->reset();
			}

			/**
			 * @brief Rolls the dice and return an number between 1 and face count.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			roll () noexcept
			{
				return std::uniform_int_distribution{1UL, m_faceCount}(m_randomEngine);
			}

			/**
			 * @brief Reset dice random generator.
			 * @return void
			 */
			void
			reset () noexcept
			{
				m_randomEngine.seed(std::random_device{}());
			}

		private:

			size_t m_faceCount;
			std::mt19937 m_randomEngine{std::random_device{}()};
	};
}
