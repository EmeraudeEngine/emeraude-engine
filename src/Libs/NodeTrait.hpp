/*
 * src/Libs/NodeTrait.hpp
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

/* STL inclusions. */
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <stack>
#include <iostream>

/* Local inclusions for usages. */
#include "Math/CartesianFrame.hpp"

namespace EmEn::Libs
{
	/**
	 * @brief Node class to define a skin.
	 * @tparam precision_t The location data precision type. Default float.
	 * @extends std::enable_shared_from_this< Node >
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	class NodeTrait : public std::enable_shared_from_this< NodeTrait< precision_t > >
	{
		public:

			static constexpr auto Root{"Root"};

			/**
			 * @brief Constructs the root node.
			 */
			NodeTrait () noexcept = default;

			/**
			 * @brief Constructs a child node.
			 * @param parent a reference to the smart pointer of the parent.
			 * @param coordinates A reference to a coordinates. Default Origin.
			 */
			explicit
			NodeTrait (const std::shared_ptr< NodeTrait > & parent, const Math::CartesianFrame< precision_t > & coordinates = {}) noexcept
				: m_parent(parent),
				m_cartesianFrame(coordinates)
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			NodeTrait (const NodeTrait & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			NodeTrait (NodeTrait && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return node &
			 */
			NodeTrait & operator= (const NodeTrait & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return node &
			 */
			NodeTrait & operator= (NodeTrait && copy) noexcept = delete;

			/**
			 * @brief Destructs the node.
			 */
			virtual ~NodeTrait () = default;

			/**
			 * @brief Returns whether the node is the top tree one. Parent pointer is nullptr.
			 * @return bool.
			 */
			[[nodiscard]]
			bool
			isRoot () const noexcept
			{
				return m_parent.expired();
			}

			/**
			 * @brief Returns true if the node have no child.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLeaf () const noexcept
			{
				return m_children.empty();
			}

			/**
			 * @brief Returns the level below the root of this node.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			getDepth () const noexcept
			{
				if ( this->isRoot() )
				{
					return 0;
				}

				size_t depth = 0;

				auto parent = m_parent.lock();

				while ( !parent->isRoot() )
				{
					depth++;

					parent = parent->m_parent.lock();
				}

				return depth;
			}

			/**
			 * @brief Returns the parent node smart pointer.
			 * @warning If the node is root, it will be nullptr.
			 * @return std::shared_ptr< NodeTrait >
			 */
			[[nodiscard]]
			std::shared_ptr< NodeTrait >
			parent () noexcept
			{
				return m_parent.lock();
			}

			/**
			 * @brief Returns the parent node smart pointer.
			 * @warning If the node is root, it will be nullptr.
			 * @return std::shared_ptr< const NodeTrait >
			 */
			[[nodiscard]]
			std::shared_ptr< const NodeTrait >
			parent () const noexcept
			{
				return m_parent.lock();
			}

			/**
			 * @brief Returns the children nodes.
			 * @return const std::map< std::string, std::shared_ptr< NodeTrait > > &
			 */
			[[nodiscard]]
			const std::map< std::string, std::shared_ptr< NodeTrait > > &
			children () const noexcept
			{
				return m_children;
			}

			/**
			 * @brief Returns the children nodes.
			 * @return std::map< std::string, std::shared_ptr< NodeTrait > > &
			 */
			[[nodiscard]]
			std::map< std::string, std::shared_ptr< NodeTrait > > &
			children () noexcept
			{
				return m_children;
			}

			/**
			 * @brief Returns the root node.
			 * @return std::shared_ptr< NodeTrait >
			 */
			[[nodiscard]]
			std::shared_ptr< NodeTrait >
			getRoot () noexcept
			{
				auto currentNode = this->shared_from_this();

				while ( !currentNode->isRoot() )
				{
					currentNode = currentNode->m_parent.lock();
				}

				return currentNode;
			}

			/**
			 * @brief Returns the root node.
			 * @return std::shared_ptr< const NodeTrait >
			 */
			[[nodiscard]]
			std::shared_ptr< const NodeTrait >
			getRoot () const noexcept
			{
				auto currentNode = this->shared_from_this();

				while ( !currentNode->isRoot() )
				{
					currentNode = currentNode->m_parent.lock();
				}

				return currentNode;
			}

			/**
			 * @brief Creates a sub node at a given coordinates.
			 * @warning If the node already exists, the method will return a null pointer.
			 * @param name A reference to a string.
			 * @param coordinates Set the coordinates of the new node. Default Origin.
			 * @return std::shared_ptr< NodeTrait >
			 */
			[[nodiscard]]
			std::shared_ptr< NodeTrait >
			createChild (const std::string & name, const Math::CartesianFrame< precision_t > & coordinates = {}) noexcept
			{
				if ( name == Root )
				{
					std::cerr << "The node name '" << Root << "' is reserved !" "\n";

					return nullptr;
				}

				if ( m_children.contains(name) )
				{
					std::cerr << "The node name '" << name << "' is already used at this level !" "\n";

					return nullptr;
				}

				return m_children.emplace(name, std::make_shared< NodeTrait >(this->shared_from_this(), coordinates)).first->second;
			}

			/**
			 * @brief Creates a sub node at a given position.
			 * @warning If the node already exists, the method will return a null pointer.
			 * @param name A reference to a string.
			 * @param position Set the position of the new node.
			 * @return std::shared_ptr< NodeTrait >
			 */
			[[nodiscard]]
			std::shared_ptr< NodeTrait >
			createChild (const std::string & name, const Math::Vector< 3, precision_t > & position) noexcept
			{
				return this->createChild(name, Math::CartesianFrame< precision_t >{position});
			}

			/**
			 * @brief Returns a child by its name.
			 * @warning Can be nullptr !
			 * @param name A reference to a string.
			 * @return std::shared_ptr< NodeTrait >
			 */
			[[nodiscard]]
			std::shared_ptr< NodeTrait >
			findChild (const std::string & name) const noexcept
			{
				const auto nodeIt = m_children.find(name);

				return nodeIt != m_children.cend() ? nodeIt->second : nullptr;
			}

			/**
			 * @brief Destroys a child by its name and returns true if the child existed.
			 * @param name A reference to a string.
			 * @return bool
			 */
			bool
			destroyChild (const std::string & name) noexcept
			{
				const auto nodeIt = m_children.find(name);

				if ( nodeIt == m_children.end() )
				{
					return false;
				}

				m_children.erase(nodeIt);

				return true;
			}

			/**
			 * @brief Destroys children.
			 * @return void
			 */
			void
			destroyChildren () noexcept
			{
				m_children.clear();
			}

			/**
			* @brief Returns the cartesian frame in local space.
			* @return const Math::CartesianFrame< precision_t > &
			*/
			[[nodiscard]]
			const Math::CartesianFrame< precision_t > &
			cartesianFrame () const noexcept
			{
				return m_cartesianFrame;
			}

			/**
			 * @brief Returns the cartesian frame in local space.
			 * @return Math::CartesianFrame< precision_t > &
			 */
			[[nodiscard]]
			Math::CartesianFrame< precision_t > &
			cartesianFrame () noexcept
			{
				return m_cartesianFrame;
			}

			/**
			 * @brief Returns the cartesian frame in world space.
			 * @return Math::CartesianFrame< precision_t >
			 */
			[[nodiscard]]
			Math::CartesianFrame< precision_t >
			getWorldCartesianFrame () const noexcept
			{
				/* NOTE: As root, return the origin!
				 * If the parent is the root node, return the frame. */
				if ( this->isRoot() || this->parent()->isRoot() )
				{
					return m_cartesianFrame;
				}

				/* Stack up a reversed tree for each cartesian frame. */
				std::stack< Math::CartesianFrame< precision_t > > reversedTree;

				{
					const auto * node = this;

					/* While the parent is not root. */
					while ( node != nullptr )
					{
						reversedTree.emplace(node->m_cartesianFrame);

						node = node->m_parent.lock().get();
					}
				}

				Math::Matrix< 4, precision_t > matrix;
				Math::Vector< 3, precision_t > scalingVector{1, 1, 1};

				while ( !reversedTree.empty() )
				{
					matrix *= reversedTree.top().getModelMatrix();
					scalingVector *= reversedTree.top().scalingFactor();

					reversedTree.pop();
				}

				return Math::CartesianFrame< precision_t >{matrix, scalingVector};
			}

		private:

			/* NOTE : If nullptr, this node is the root. */
			std::weak_ptr< NodeTrait > m_parent;
			std::map< std::string, std::shared_ptr< NodeTrait > > m_children;
			Math::CartesianFrame< precision_t > m_cartesianFrame;
	};
}
