/*
 * src/Libs/GameTools/CardDeck.cpp
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

#include "CardDeck.hpp"

/* Local inclusions. */
#include "Libs/Utility.hpp"

namespace EmEn::Libs::GameTools
{
	CardDeck::CardDeck (size_t handCount, size_t cardCount) noexcept
		: m_maxCardCount(cardCount)
	{
		/* Initialize each hand. */
		m_hands.resize(handCount);

		std::generate(m_hands.begin(), m_hands.end(), [cardCount] () {
			return std::make_shared< CardHand >(cardCount);
		});

		/* Reserve space in the discarded card pile. */
		m_discardedCards.reserve(m_maxCardCount);

		/* Set the initial state. */
		this->reset();
	}

	void
	CardDeck::reset () noexcept
	{
		/* Reset all hands. */
		for ( auto & hand : m_hands )
		{
			hand->m_cards.clear();
		}

		/* Replenish the deck. */
		m_cards.resize(m_maxCardCount);

		std::generate(m_cards.begin(), m_cards.end(), [number = 0] () mutable {
			return number++;
		});

		/* Reset discard pile. */
		m_discardedCards.clear();

		/* Set a new seed for the random engine. */
		{
			const auto seed = std::random_device{}();

			m_randomEngine.seed(seed);
		}
	}

	bool
	CardDeck::pick (std::vector< size_t > & pile, const std::shared_ptr< CardHand > & hand, Where where) noexcept
	{
		if ( hand == nullptr )
		{
			return false;
		}

		if ( std::find(m_hands.cbegin(), m_hands.cend(), hand) == m_hands.end() )
		{
			return false;
		}

		/* NOTE: No more card. */
		if ( pile.empty() )
		{
			return false;
		}

		std::vector< size_t >::iterator pickedIterator;

		switch ( where )
		{
			case Where::Top :
				pickedIterator = pile.begin();
				break;

			case Where::Bottom :
				pickedIterator = std::prev(pile.end());

				break;

			case Where::Randomly :
				pickedIterator = pile.begin() + std::uniform_int_distribution{0, static_cast< int >(pile.size() - 1)}(m_randomEngine);
				break;
		}

		const auto pickedCard = *pickedIterator;

		/* NOTE: Remove it from the targeted pile. */
		pile.erase(pickedIterator);

		/* NOTE: Push it to the hand. */
		hand->m_cards.push_back(pickedCard);

		return true;
	}

	bool
	CardDeck::insert (std::vector< size_t > & pile, const std::shared_ptr< CardHand > & hand, size_t card, Where where) noexcept
	{
		if ( hand == nullptr )
		{
			return false;
		}

		if ( std::find(m_hands.cbegin(), m_hands.cend(), hand) == m_hands.end() )
		{
			return false;
		}

		auto & handCards = hand->m_cards;

		/* NOTE: No card in hand. */
		if ( handCards.empty() )
		{
			return false;
		}

		/* NOTE: The card is not in hand. */
		auto pickedIterator = std::find(handCards.begin(), handCards.end(), card);

		if ( pickedIterator == handCards.end() )
		{
			return false;
		}

		/* NOTE: Remove it from the hand. */
		handCards.erase(pickedIterator);

		/* NOTE: Push it to the targeted pile. */
		switch ( where )
		{
			case Where::Top :
				pile.insert(pile.begin(), card);
				break;

			case Where::Bottom :
				pile.push_back(card);
				break;

			case Where::Randomly :
				const auto randomPosition = pile.begin() + std::uniform_int_distribution{0, static_cast< int >(pile.size() - 1)}(m_randomEngine);

				pile.insert(randomPosition, card);
				break;
		}

		return true;
	}
}
