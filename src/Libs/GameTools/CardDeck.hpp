/*
 * src/Libs/GameTools/CardDeck.hpp
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
#include <algorithm>
#include <cstddef>
#include <memory>
#include <random>
#include <vector>

/* Local inclusions for usages. */
#include "CardHand.hpp"

namespace EmEn::Libs::GameTools
{
	/**
	 * @brief The class define a card deck and a discarded card pile.
	 */
	class CardDeck final
	{
		public:

			/** Type of card picking. */
			enum class Where
			{
				Top,
				Bottom,
				Randomly
			};

			/**
			 * @brief Construct a card deck.
			 * @param handCount The number of hands.
			 * @param cardCount The number of cards. Default 52.
			 */
			explicit CardDeck (size_t handCount, size_t cardCount = 52) noexcept;

			/**
			 * @brief Shuffles the deck.
			 * @return void
			 */
			void
			shuffleCardDeck () noexcept
			{
				std::shuffle(m_cards.begin(), m_cards.end(), m_randomEngine);
			}

			/**
			 * @brief Shuffles the discarded cards.
			 * @return void
			 */
			void
			shuffleDiscardedCards () noexcept
			{
				std::shuffle(m_discardedCards.begin(), m_discardedCards.end(), m_randomEngine);
			}

			/**
			 * @brief Reconstructs the deck entirely.
			 * @return void
			 */
			void reset () noexcept;

			/**
			 * @brief Picks a card from the deck.
			 * @param hand A reference to a hand smart pointer.
			 * @param where From where to pick the card from the deck. Default from top.
			 * @return bool
			 */
			bool
			pickFromCardDeck (const std::shared_ptr< CardHand > & hand, Where where = Where::Top) noexcept
			{
				return this->pick(m_cards, hand, where);
			}

			/**
			 * @brief Picks a card from the discardPile.
			 * @param hand A reference to a hand smart pointer.
			 * @param pick From where to pick the card from the deck. Default from top.
			 * @return bool
			 */
			bool
			pickFromDiscardedPile (const std::shared_ptr< CardHand > & hand, Where where = Where::Top) noexcept
			{
				return this->pick(m_discardedCards, hand, where);
			}

			/**
			 * @brief Releases a card to the deck.
			 * @param hand A reference to a hand smart pointer.
			 * @param card The card from the hand.
			 * @param where Where to put the card. Default top.
			 * @return bool
			 */
			bool
			release (const std::shared_ptr< CardHand > & hand, size_t card, Where where = Where::Top) noexcept
			{
				return this->insert(m_cards, hand, card, where);
			}

			/**
			 * @brief Discards a card.
			 * @param hand A reference to a hand smart pointer.
			 * @param card The card from the hand.
			 * @param where Where to put the card. Default top.
			 * @return bool
			 */
			bool
			discard (const std::shared_ptr< CardHand > & hand, size_t card, Where where = Where::Top) noexcept
			{
				return this->insert(m_discardedCards, hand, card, where);
			}

			/**
			 * @brief Returns the number of card in the game.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			maxCardCount () const noexcept
			{
				return m_maxCardCount;
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
			 * @brief Returns the number of card in the deck.
			 * @note Shortcut to CardDeck::cards().size().
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			cardCount () const noexcept
			{
				return m_cards.size();
			}

			/**
			 * @brief Returns the discarded card container.
			 * @return const std::vector< size_t > &
			 */
			[[nodiscard]]
			const std::vector< size_t > &
			discardedCards () const noexcept
			{
				return m_discardedCards;
			}

			/**
			 * @brief Returns the number of card in the discarded pile.
			 * @note Shortcut to CardDeck::discardedCards().size().
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			discardedCardCount () const noexcept
			{
				return m_discardedCards.size();
			}

			/**
			 * @brief Returns the hand list.
			 * @return const std::vector< std::shared_ptr< CardHand > > &
			 */
			[[nodiscard]]
			const std::vector< std::shared_ptr< CardHand > > &
			hands () const noexcept
			{
				return m_hands;
			}

			/**
			 * @brief Returns the number of hands in the game.
			 * @note Shortcut to CardDeck::hands().size().
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			handCount () const noexcept
			{
				return m_hands.size();
			}

		private:

			/**
			 * @brief Picks a card from a pile.
			 * @param pile A reference to the target pile.
			 * @param hand A reference to a hand smart pointer.
			 * @param where Where to put the card.
			 * @return bool
			 */
			bool pick (std::vector< size_t > & pile, const std::shared_ptr< CardHand > & hand, Where where) noexcept;

			/**
			 * @brief Inserts a card to a pile.
			 * @param pile A reference to the target pile.
			 * @param hand A reference to a hand smart pointer.
			 * @param card The card from the hand.
			 * @param where Where to put the card.
			 * @return bool
			 */
			bool insert (std::vector< size_t > & pile, const std::shared_ptr< CardHand > & hand, size_t card, Where where) noexcept;

			size_t m_maxCardCount;
			std::vector< size_t > m_cards;
			std::vector< size_t > m_discardedCards;
			std::vector< std::shared_ptr< CardHand > > m_hands;
			std::mt19937 m_randomEngine{std::random_device{}()};
	};
}
