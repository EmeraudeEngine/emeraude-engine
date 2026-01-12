/*
 * src/Scenes/NodeCrawler.hpp
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
#include <stack>
#include <type_traits>
#include <memory>

/* Local inclusions for usages. */
#include "Node.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief Utility class to crawl down the node tree from a node.
	 * @tparam node_t The type of node
	 */
	template< typename node_t >
	requires (std::is_class_v< Node >)
	class NodeCrawler final
	{
		public:

			/**
			 * @brief Constructs a crawler from a node.
			 * @param baseNode From which node to execute the crawling.
			 */
			explicit
			NodeCrawler (const std::shared_ptr< node_t > & baseNode) noexcept
			{
				this->populateStack(baseNode);
			}

			/**
			 * @brief Returns whether a next node exists.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasNextNode () const noexcept
			{
				return !m_nodes.empty();
			}

			/**
			 * @brief Returns the next node.
			 * @warning This will never return a discarded node waiting to be removed from the tree.
			 * @return std::shared_ptr< type_t >
			 */
			[[nodiscard]]
			std::shared_ptr< node_t >
			nextNode () noexcept
			{
				if ( m_nodes.empty() )
				{
					return nullptr;
				}

				/* Gets the next node */
				auto nextNode = m_nodes.top();

				/* Removes it from the stack. */
				m_nodes.pop();

				/* Prepares next nodes from current node children. */
				this->populateStack(nextNode);

				return nextNode;
			}

		private:

			/**
			 * @brief Populates the stacks with children of a node.
			 * @note Discardable nodes are not taken in account.
			 * @param currentNode A reference to the smart pointer of the current node.
			 * @return void
			 */
			void
			populateStack (const std::shared_ptr< node_t > & currentNode) noexcept
			{
				for ( const auto & nodePair : currentNode->children() )
				{
					if ( nodePair.second->isDiscardable() )
					{
						continue;
					}

					m_nodes.emplace(nodePair.second);
				}
			}

			std::stack< std::shared_ptr< node_t > > m_nodes;
	};
}
