/*
 * src/Input/KeyboardListenerInterface.hpp
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
#include <cstdint>
#include <iostream>

namespace EmEn::Input
{
	/**
	 * @brief Interface giving the ability to listen to the keyboard events.
	 */
	class KeyboardListenerInterface
	{
		friend class Manager;

		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			KeyboardListenerInterface (const KeyboardListenerInterface & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			KeyboardListenerInterface (KeyboardListenerInterface && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			KeyboardListenerInterface & operator= (const KeyboardListenerInterface & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			KeyboardListenerInterface & operator= (KeyboardListenerInterface && copy) noexcept = default;

			/**
			 * @brief Destructs the keyboard input listener.
			 */
			virtual ~KeyboardListenerInterface () = default;

			/**
			 * @brief Enables or disables this listener.
			 * @param state The state.
			 * @return void
			 */
			void
			enableKeyboardListening (bool state) noexcept
			{
				m_enabled = state;
			}

			/**
			 * @brief Returns whether the listener is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isListeningKeyboard () const noexcept
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
			 * @brief Enables the text mode.
			 * @param state The state.
			 */
			void
			enableTextMode (bool state) noexcept
			{
				m_textModeEnabled = state;
			}

			/**
			 * @brief Returns whether the text mode is enabled or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTextModeEnabled () const noexcept
			{
				return m_textModeEnabled;
			}

		protected:

			/**
			 * @brief Constructs a keyboard input listener interface.
			 * @param enableProcessedEventPropagation Enable the listener to propagate events event if they are processed.
			 * @param enableTextMode Enable text mode. This will enable the character typing callback.
			 */
			KeyboardListenerInterface (bool enableProcessedEventPropagation, bool enableTextMode) noexcept
				: m_propagateProcessedEvent{enableProcessedEventPropagation},
				m_textModeEnabled{enableTextMode}
			{

			}

		private:

			/**
			 * @brief Method to override to handle key pressing.
			 * @note Returning true means the event was consumed.
			 * @param key The keyboard universal key code from GLFW.
			 * @param scancode The OS dependent scancode.
			 * @param modifiers The modifier keys mask.
			 * @param repeat Repeat state.
			 * @return bool
			 */
			virtual
			bool
			onKeyPress (int32_t /*key*/, int32_t /*scancode*/, int32_t /*modifiers*/, bool /*repeat*/) noexcept
			{
				return false;
			}

			/**
			 * @brief Method to override to handle key releasing.
			 * @note Returning true means the event was consumed.
			 * @param key The keyboard universal key code from GLFW.
			 * @param scancode The OS dependent scancode.
			 * @param modifiers The modifier keys mask.
			 * @return bool
			 */
			virtual
			bool
			onKeyRelease (int32_t /*key*/, int32_t /*scancode*/, int32_t /*modifiers*/) noexcept
			{
				return false;
			}

			/**
			 * @brief Method to override to handle text typing.
			 * @note Returning true means the event was consumed.
			 * @param unicode The character Unicode value.
			 * @return bool
			 */
			virtual
			bool
			onCharacterType (uint32_t /*unicode*/) noexcept
			{
				std::cerr << "Text mode has been enabled on a listener which not overriding the method onCharacterType() !" "\n";

				return false;
			}

			bool m_enabled{true};
			bool m_propagateProcessedEvent{false};
			bool m_textModeEnabled{false};
	};
}
