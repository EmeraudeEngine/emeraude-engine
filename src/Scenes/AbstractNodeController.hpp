/*
 * src/Scenes/AbstractNodeController.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <memory>

/* Local inclusions for usages. */
#include "Mutex.hpp"
#include "ThreadSafety.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class Node;
}

namespace EmEn::Scenes
{
	/**
	 * @brief The base of the controllers that drive a scene node from an input device.
	 * @details THE contract of the family (owner decisions, 2026-11-17): **the input records, the logic applies.**
	 * - An input callback (a key, the pointer, the wheel) runs on the MAIN thread, inside the input manager's event
	 *   pump. It only RECORDS what the user asked, in the finality's own state, under the finality's own mutex. It
	 *   never touches the node.
	 * - update() runs on the LOGIC thread, once per cycle (Scene::processLogics()). It takes what was recorded and
	 *   writes the node: the logic thread is the ONLY writer of a controlled node, like every other node of a running
	 *   scene. What the user did reaches the node one logic cycle later at most.
	 * A node has no lock: a controller that moved it from its callback tore the camera's view under the logic thread's
	 * publication (31 ThreadSanitizer reports on one drag in the model viewer).
	 * @note A finality derives this class and the listener interface of its device (keyboard, pointer), guards
	 * everything its callbacks record with a mutex of its own, and implements onUpdate(). The base guards the node's
	 * pointer with its own mutex; the two are never held together.
	 */
	class EMEN_API AbstractNodeController
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractNodeController (const AbstractNodeController & copy) = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			AbstractNodeController (AbstractNodeController && copy) = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractNodeController &
			 */
			AbstractNodeController & operator= (const AbstractNodeController & copy) = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return AbstractNodeController &
			 */
			AbstractNodeController & operator= (AbstractNodeController && copy) = delete;

			/**
			 * @brief Destructs the node controller.
			 */
			virtual ~AbstractNodeController () = default;

			/**
			 * @brief Sets the node to control and places it at once.
			 * @note The caller is the thread that BUILDS the scene (not enabled yet), or the logic thread: this call
			 * applies the controller's state to the node immediately, as update() does.
			 * @param node The scene node. A null pointer releases the current one.
			 */
			void controlNode (const std::shared_ptr< Node > & node) noexcept EMEN_EXCLUDES(m_nodeAccess);

			/**
			 * @brief Releases the controlled node.
			 * @note Any thread.
			 */
			void releaseNode () noexcept EMEN_EXCLUDES(m_nodeAccess);

			/**
			 * @brief Releases the controlled node if it is this one (a node that leaves its scene).
			 * @note Any thread.
			 * @param node The node.
			 * @return bool True when the controller held this node.
			 */
			bool releaseNode (const std::shared_ptr< const Node > & node) noexcept EMEN_EXCLUDES(m_nodeAccess);

			/**
			 * @brief Returns whether a node is controlled.
			 * @note Any thread.
			 * @return bool
			 */
			[[nodiscard]]
			bool hasNode () const noexcept EMEN_EXCLUDES(m_nodeAccess);

			/**
			 * @brief Returns the controlled node.
			 * @note Any thread.
			 * @return std::shared_ptr< Node >
			 */
			[[nodiscard]]
			std::shared_ptr< Node > node () const noexcept EMEN_EXCLUDES(m_nodeAccess);

			/**
			 * @brief Applies to the node what the input recorded since the last call.
			 * @note LOGIC thread, once per cycle (Scene::processLogics()): the only writer of the controlled node.
			 */
			void update () noexcept EMEN_EXCLUDES(m_nodeAccess);

		protected:

			/**
			 * @brief Constructs a node controller.
			 */
			AbstractNodeController () noexcept = default;

			/**
			 * @brief Called when a node has just been given to the controller, before it is placed.
			 * @note The caller's thread of controlNode(); the base's mutex is NOT held.
			 */
			virtual
			void
			onNodeControlled () noexcept
			{

			}

			/**
			 * @brief Called when the controlled node has just been released.
			 * @note Any thread; the base's mutex is NOT held.
			 */
			virtual
			void
			onNodeReleased () noexcept
			{

			}

			/**
			 * @brief Applies the recorded input to the node.
			 * @note LOGIC thread (or the scene's builder, from controlNode()); no mutex is held: take what the
			 * callbacks recorded under the finality's own, release it, THEN write the node — a node notifies its components.
			 * @param node The controlled node.
			 */
			virtual void onUpdate (Node & node) noexcept = 0;

		private:

			/** @brief Guards the controlled node's pointer: given by the scene's builder, released by any thread, read
			 * by the input callbacks (main thread) and by update() (logic thread). */
			mutable Base::Mutex m_nodeAccess;
			std::shared_ptr< Node > m_controlledNode EMEN_GUARDED_BY(m_nodeAccess);
	};
}
