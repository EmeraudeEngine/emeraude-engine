/*
 * src/Libs/GameTools/CardHand.hpp
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
#include <vector>
#include <algorithm>
#include <random>

namespace EmEn::Libs::GameTools
{
	class CardDeck;

	/**
	 * @brief Define a card hand.
	 */
	class CardHand final
	{
		friend CardDeck;

		public:

			/**
			 * @brief Constructs a default card hand.
			 */
			CardHand () noexcept = default;

			/**
			 * @brief Constructs a card hand with a reserved space.
			 * @param reserve Reserve space for hand.
			 */
			explicit CardHand (size_t reserve) noexcept
			{
				m_cards.reserve(reserve);
			}

			/**
			 * @brief Shuffles the hand.
			 */
			void
			shuffle () noexcept
			{
				const auto seed = std::random_device{}();

				std::shuffle(m_cards.begin(), m_cards.end(), std::mt19937{seed});
			}

			/**
			 * @brief Returns the card container.
			 * @return const std::vector< size_t > &
			 */
			[[nodiscard]]
			const std::vector< size_t > &
			cards () const noexcept
			{
				return m_cards;
			}

			/**
			 * @brief Returns the number of cards in hand.
			 * @note Shortcut to CardHand::cards().size().
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			cardCount () const noexcept
			{
				return m_cards.size();
			}

		private:

			std::vector< size_t > m_cards;
	};
}
