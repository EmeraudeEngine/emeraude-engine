/*
 * src/Scenes/AbstractNodeController.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 */

#include "AbstractNodeController.hpp"

/* STL inclusions. */
#include <utility>

namespace EmEn::Scenes
{
	void
	AbstractNodeController::controlNode (const std::shared_ptr< Node > & node) noexcept
	{
		if ( node == nullptr )
		{
			this->releaseNode();

			return;
		}

		{
			const Base::MutexLock lock{m_nodeAccess};

			m_controlledNode = node;
		}

		this->onNodeControlled();

		/* NOTE: The node is placed at once (see the method's contract). */
		this->update();
	}

	void
	AbstractNodeController::releaseNode () noexcept
	{
		/* NOTE: The node is let go outside the lock: it may be the last owner. */
		std::shared_ptr< Node > released;

		{
			const Base::MutexLock lock{m_nodeAccess};

			released = std::exchange(m_controlledNode, nullptr);
		}

		if ( released != nullptr )
		{
			this->onNodeReleased();
		}
	}

	bool
	AbstractNodeController::releaseNode (const std::shared_ptr< const Node > & node) noexcept
	{
		std::shared_ptr< Node > released;

		{
			const Base::MutexLock lock{m_nodeAccess};

			if ( m_controlledNode == nullptr || m_controlledNode != node )
			{
				return false;
			}

			released = std::exchange(m_controlledNode, nullptr);
		}

		this->onNodeReleased();

		return true;
	}

	bool
	AbstractNodeController::hasNode () const noexcept
	{
		const Base::MutexLock lock{m_nodeAccess};

		return m_controlledNode != nullptr;
	}

	std::shared_ptr< Node >
	AbstractNodeController::node () const noexcept
	{
		const Base::MutexLock lock{m_nodeAccess};

		return m_controlledNode;
	}

	void
	AbstractNodeController::update () noexcept
	{
		/* NOTE: A copy of the pointer: the node is written with no lock held. */
		const auto node = this->node();

		if ( node == nullptr )
		{
			return;
		}

		this->onUpdate(*node);
	}
}
