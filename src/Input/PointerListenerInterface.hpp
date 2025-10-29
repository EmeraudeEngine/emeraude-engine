/*
 * src/Input/PointerListenerInterface.hpp
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
#include <cstdint>

namespace EmEn::Input
{
	/**
	 * @brief Interface giving the ability to listen to the pointer (like the mouse) events.
	 * @note By default, a pointer listener will use the absolute mode to be used like a mouse on screen.
	 */
	class PointerListenerInterface
	{
		friend class Manager;

		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PointerListenerInterface (const PointerListenerInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PointerListenerInterface (PointerListenerInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			PointerListenerInterface & operator= (const PointerListenerInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			PointerListenerInterface & operator= (PointerListenerInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the pointer input listener.
			 */
			virtual ~PointerListenerInterface () = default;

			/**
			 * @brief Enables or disables this listener.
			 * @param state The state.
			 * @return void
			 */
			void
			enablePointerListening (bool state) noexcept
			{
				m_enabled = state;
			}

			/**
			 * @brief Returns whether the listener is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isListeningPointer () const noexcept
			{
				return m_enabled;
			}

			/**
			 * @brief Sets whether the listener is propagating the processed events.
			 * @param state The state.
			 * @return void
			 */
			void
			propagateProcessedEvent (bool state) noexcept
			{
				m_propagateProcessedEvent = state;
			}

			/**
			 * @brief Returns whether the listener is propagating the processed events.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPropagatingProcessedEvents () const noexcept
			{
				return m_propagateProcessedEvent;
			}

			/**
			 * @brief Sets the pointer to relative mode.
			 * @return void
			 */
			void
			enableRelativeMode () noexcept
			{
				m_isRelativeMode = true;
			}

			/**
			 * @brief Sets the pointer to relative mode.
			 * @return void
			 */
			void
			enableAbsoluteMode () noexcept
			{
				m_isRelativeMode = false;
			}

			/**
			 * @brief Returns whether the pointer uses the relative mode.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRelativeModeEnabled () const noexcept
			{
				return m_isRelativeMode;
			}

			/**
			 * @brief Returns whether the pointer uses the absolute mode.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAbsoluteModeEnabled () const noexcept
			{
				return !m_isRelativeMode;
			}

			/**
			 * @brief Lock this listener when holding a mouse button to send all move events to it.
			 * @param state The state.
			 * @return void
			 */
			void
			lockListenerOnMoveEvents (bool state) noexcept
			{
				m_listenerLockedOnMoveEvents = state;
			}

			/**
			 * @brief Returns whether the move events are tracked when a button is held.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isListenerLockedOnMoveEvents () const noexcept
			{
				return m_listenerLockedOnMoveEvents;
			}

		protected:

			/**
			 * @brief Constructs a pointer input listener interface.
			 * @param enableProcessedEventPropagation Enable the listener to propagate events event if they are processed.
			 * @param enableRelativeMode Enable the relative coordinate mode.
			 * @param lockListenerOnMoveEvents Enable the move event tracking.
			 */
			PointerListenerInterface (bool enableProcessedEventPropagation, bool enableRelativeMode, bool lockListenerOnMoveEvents) noexcept
				: m_propagateProcessedEvent{enableProcessedEventPropagation},
				m_isRelativeMode{enableRelativeMode},
				m_listenerLockedOnMoveEvents(lockListenerOnMoveEvents)
			{

			}

		private:

			/**
			 * @brief Method to override to handle pointer entering this listener surface.
			 * @note Returning true means the event was consumed.
			 * @param positionX The X position of the pointer according to absolute or relative mode.
			 * @param positionY The Y position of the pointer according to absolute or relative mode.
			 * @return bool
			 */
			virtual
			bool
			onPointerEnter (float /*positionX*/, float /*positionY*/) noexcept
			{
				return false;
			}

			/**
			 * @brief Method to override to handle pointer leaving this listener surface.
			 * @note Returning true means the event was consumed.
			 * @param positionX The X position of the pointer according to absolute or relative mode.
			 * @param positionY The Y position of the pointer according to absolute or relative mode.
			 * @return bool
			 */
			virtual
			bool
			onPointerLeave (float /*positionX*/, float /*positionY*/) noexcept
			{
				return false;
			}

			/**
			 * @brief Method to override to handle pointer move.
			 * @note Returning true means the event was consumed.
			 * @param positionX The X position of the pointer according to absolute or relative mode.
			 * @param positionY The Y position of the pointer according to absolute or relative mode.
			 * @return bool
			 */
			virtual
			bool
			onPointerMove (float /*positionX*/, float /*positionY*/) noexcept
			{
				return false;
			}

			/**
			 * @brief Method to override to handle a pointer button press.
			 * @note Returning true means the event was consumed.
			 * @param positionX The X position of the pointer according to absolute or relative mode.
			 * @param positionY The Y position of the pointer according to absolute or relative mode.
			 * @param buttonNumber The button number of the pointer.
			 * @param modifiers The modification keys state. (From keyboard)
			 * @return bool
			 */
			virtual
			bool
			onButtonPress (float /*positionX*/, float /*positionY*/, int32_t /*buttonNumber*/, int32_t /*modifiers*/) noexcept
			{
				return false;
			}

			/**
			 * @brief Method to override to handle a pointer button release.
			 * @note Returning true means the event was consumed.
			 * @param positionX The X position of the pointer according to absolute or relative mode.
			 * @param positionY The Y position of the pointer according to absolute or relative mode.
			 * @param buttonNumber The button number of the pointer.
			 * @param modifiers The modification keys state.  (From keyboard)
			 * @return bool
			 */
			virtual
			bool
			onButtonRelease (float /*positionX*/, float /*positionY*/, int32_t /*buttonNumber*/, int32_t /*modifiers*/) noexcept
			{
				return false;
			}

			/**
			 * @brief Method to override to handle a pointer wheel change.
			 * @note Returning true means the event was consumed.
			 * @param positionX The X position of the pointer according to absolute or relative mode.
			 * @param positionY The Y position of the pointer according to absolute or relative mode.
			 * @param xOffset The X offset of the wheel.
			 * @param yOffset The Y offset of the wheel.
			 * @param modifiers The keyboard modifiers pressed during the scroll (Ctrl, Shift, Alt, etc.).
			 * @return bool
			 */
			virtual
			bool
			onMouseWheel (float /*positionX*/, float /*positionY*/, float /*xOffset*/, float /*yOffset*/, int32_t /*modifiers*/ = 0) noexcept
			{
				return false;
			}

			bool m_enabled{true};
			bool m_propagateProcessedEvent{false};
			bool m_isRelativeMode{false};
			bool m_listenerLockedOnMoveEvents{false};
	};
}
