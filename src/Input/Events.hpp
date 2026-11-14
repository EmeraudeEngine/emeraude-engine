/*
 * src/Input/Events.hpp
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
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <type_traits>
#include <variant>

namespace EmEn::Input
{
	/* NOTE: What a GLFW input callback hands to the input manager: plain values, read in the callback, so the event can
	 * wait in the window's queue until the dispatch released the native-surface lock (Window::recordInputEvent(),
	 * Manager::deliverEvent()). A position is GLFW's cursor position when the callback ran, in screen coordinates and
	 * before the pointer scaling: the manager scales it at the delivery. */

	/** @brief A key pressed, repeated or released (the GLFW key callback). */
	struct KeyEvent final
	{
		/** @brief The GLFW key code. */
		int32_t key{0};
		/** @brief The system scancode. */
		int32_t scancode{0};
		/** @brief GLFW_PRESS, GLFW_REPEAT or GLFW_RELEASE. */
		int32_t action{0};
		/** @brief The modifier keys mask. */
		int32_t modifiers{0};
	};

	/** @brief A character typed (the GLFW character callback). */
	struct CharacterEvent final
	{
		/** @brief The Unicode code point. */
		uint32_t codepoint{0};
	};

	/** @brief The pointer moved (the GLFW cursor position callback). */
	struct CursorPositionEvent final
	{
		double xPosition{0.0};
		double yPosition{0.0};
	};

	/** @brief The pointer entered or left the window (the GLFW cursor enter callback). */
	struct CursorEnterEvent final
	{
		double xPosition{0.0};
		double yPosition{0.0};
		/** @brief True when entering, false when leaving. */
		bool entered{false};
	};

	/** @brief A pointer button pressed or released (the GLFW mouse button callback). */
	struct MouseButtonEvent final
	{
		double xPosition{0.0};
		double yPosition{0.0};
		/** @brief Which pointer button. */
		int32_t button{0};
		/** @brief GLFW_PRESS or GLFW_RELEASE. */
		int32_t action{0};
		/** @brief The modifier keys mask. */
		int32_t modifiers{0};
	};

	/** @brief The wheel turned (the GLFW scroll callback). */
	struct MouseWheelEvent final
	{
		double xPosition{0.0};
		double yPosition{0.0};
		float xOffset{0.0F};
		float yOffset{0.0F};
		/** @brief The modifier keys mask, read from the keyboard state when the callback ran. */
		int32_t modifiers{0};
	};

	/** @brief A joystick or a gamepad was connected or disconnected (the GLFW joystick callback). */
	struct JoystickEvent final
	{
		/** @brief The GLFW joystick id. */
		int32_t deviceID{0};
		/** @brief GLFW_CONNECTED or GLFW_DISCONNECTED. */
		int32_t event{0};
	};

	/** @brief Files were dropped on the window (the GLFW drop callback). The paths wait in the manager, one set per
	 * event, in the order of the events: they are no plain values. */
	struct DroppedFilesEvent final
	{

	};

	/** @brief An input event recorded by a GLFW callback, delivered by Manager::deliverEvent(). */
	using Event = std::variant< KeyEvent, CharacterEvent, CursorPositionEvent, CursorEnterEvent, MouseButtonEvent, MouseWheelEvent, JoystickEvent, DroppedFilesEvent >;

	/* NOTE: Queued by value at every pointer motion: a copy must stay a plain memory copy, never an allocation. */
	static_assert(std::is_trivially_copyable_v< Event >, "An input event is queued by value: it must stay trivially copyable !");
}
