/*
 * src/Input/Manager.cpp
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

#include "Manager.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <ranges>
#include <utility>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"

/* Local inclusions. */
#include "Console/Command.hpp"
#include "GamepadController.hpp"
#include "JoystickController.hpp"
#include "IO/IO.hpp"
#include "Arguments.hpp"
#include "BaseUtility.hpp"
#include "FileSystem.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "Window.hpp"

namespace EmEn::Input
{
	using namespace Base;
	using namespace Vulkan;

	void
	Manager::linkWindowCallbacks (bool enableKeyboard, bool enablePointer) noexcept
	{
		auto * window = m_window.handle();

		/* Keyboard listeners. */
		if ( enableKeyboard )
		{
			glfwSetKeyCallback(window, keyCallback);
			glfwSetCharCallback(window, charCallback);

			/* TODO: Make it optional */
			glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
			glfwSetInputMode(window, GLFW_LOCK_KEY_MODS, GLFW_TRUE);

			m_isListeningKeyboard = true;
		}

		/* Pointer listeners. */
		if ( enablePointer )
		{
			glfwSetMouseButtonCallback(window, mouseButtonCallback);
			glfwSetCursorPosCallback(window, cursorPositionCallback);
			glfwSetCursorEnterCallback(window, cursorEnterCallback);
			glfwSetScrollCallback(window, scrollCallback);

#if IS_MACOS
			/* Install macOS-specific gesture handlers for pinch-to-zoom */
			this->installMacOSGestureHandlers(window);
#endif

			/* TODO: Make it optional */
			glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

			m_isListeningPointer = true;
		}

		/* Misc. listeners. */
		/* TODO: Make it optional */
		glfwSetDropCallback(window, dropCallback);
		glfwSetJoystickCallback(joystickCallback);

		m_windowLinked = true;
	}

	void
	Manager::unlinkWindowCallbacks () noexcept
	{
		auto * window = m_window.handle();

		/* Keyboard listeners. */
		glfwSetKeyCallback(window, nullptr);
		glfwSetCharCallback(window, nullptr);

		/* Pointer listeners. */
		glfwSetMouseButtonCallback(window, nullptr);
		glfwSetCursorPosCallback(window, nullptr);
		glfwSetCursorEnterCallback(window, nullptr);
		glfwSetScrollCallback(window, nullptr);

#if IS_MACOS
		/* Remove macOS gesture handlers */
		this->removeMacOSGestureHandlers();
#endif

		/* Misc. listeners. */
		glfwSetDropCallback(window, nullptr);
		glfwSetJoystickCallback(nullptr);

		m_windowLinked = false;
	}

	void
	Manager::enableKeyboardListening (bool state) noexcept
	{
		if ( m_windowLess )
		{
			/* No window available. */
			return;
		}

		if ( m_isListeningKeyboard == state )
		{
			return;
		}

		if ( !m_window.usable() )
		{
			Tracer::error(ClassId, "The window is not usable ! Unable to link callbacks to it.");

			return;
		}

		auto * window = m_window.handle();

		if ( state )
		{
			glfwSetKeyCallback(window, keyCallback);
			glfwSetCharCallback(window, charCallback);
		}
		else
		{
			glfwSetKeyCallback(window, nullptr);
			glfwSetCharCallback(window, nullptr);
		}

		m_isListeningKeyboard = state;
	}

	void
	Manager::enablePointerListening (bool state) noexcept
	{
		if ( m_windowLess )
		{
			/* No window available. */
			return;
		}

		if ( m_isListeningPointer == state )
		{
			return;
		}

		if ( !m_window.usable() )
		{
			Tracer::error(ClassId, "The window is not usable ! Unable to link callbacks to it.");

			return;
		}

		auto * window = m_window.handle();

		if ( state )
		{
			glfwSetMouseButtonCallback(window, mouseButtonCallback);
			glfwSetCursorPosCallback(window, cursorPositionCallback);
			glfwSetCursorEnterCallback(window, cursorEnterCallback);
			glfwSetScrollCallback(window, scrollCallback);
		}
		else
		{
			glfwSetMouseButtonCallback(window, nullptr);
			glfwSetCursorPosCallback(window, nullptr);
			glfwSetCursorEnterCallback(window, nullptr);
			glfwSetScrollCallback(window, nullptr);
		}

		m_isListeningPointer = state;
	}

	std::array< float, 2 >
	Manager::getPointerLocation (GLFWwindow * window) noexcept
	{
		/* NOTE: If the pointer is locked (mouse view), we don't care about computing the position. */
		if ( s_instance->isPointerLocked() )
		{
			return {0.5F, 0.5F};
		}

		double xPosition = 0.0;
		double yPosition = 0.0;

		glfwGetCursorPos(window, &xPosition, &yPosition);

		if ( s_instance->m_pointerCoordinatesScalingEnabled )
		{
			xPosition *= s_instance->m_pointerScalingFactors[0];
			yPosition *= s_instance->m_pointerScalingFactors[1];
		}

		return {static_cast< float >(xPosition), static_cast< float >(yPosition)};
	}

	void
	Manager::keyCallback (GLFWwindow * /*handle*/, int key, int scancode, int action, int modifiers) noexcept
	{
		if constexpr ( KeyboardInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Keyboard input detected!" "\n"
				"Key: " << key << "\n"
				"ScanCode: " << scancode << "\n"
				"Action: " << ( action == GLFW_RELEASE ? "Release" : "Press") << "\n"
				"Repeat: " << ( action == GLFW_REPEAT ? "On" : "Off") << "\n"
				"Keyboard modifiers: " << getModifierListString(modifiers) << '\n';
		}

		switch ( key )
		{
			case GLFW_KEY_LEFT_SHIFT :
			case GLFW_KEY_LEFT_CONTROL :
			case GLFW_KEY_LEFT_ALT :
			case GLFW_KEY_LEFT_SUPER :
			case GLFW_KEY_RIGHT_SHIFT :
			case GLFW_KEY_RIGHT_CONTROL :
			case GLFW_KEY_RIGHT_ALT :
			case GLFW_KEY_RIGHT_SUPER :
			case GLFW_KEY_MENU :
				KeyboardController::changeKeyState(key, action == GLFW_PRESS);
				break;

			default:
				break;
		}

		for ( const auto & listener : s_instance->m_keyboardListeners )
		{
			if ( !listener->isListeningKeyboard() )
			{
				continue;
			}

			auto eventProcessed = false;

			switch ( action )
			{
				case GLFW_PRESS :
					eventProcessed = listener->onKeyPress(key, scancode, modifiers, false);
					break;

				case GLFW_REPEAT :
					eventProcessed = listener->onKeyPress(key, scancode, modifiers, true);
					break;

				case GLFW_RELEASE :
					eventProcessed = listener->onKeyRelease(key, scancode, modifiers);
					break;

				default:
					break;
			}
			
			if ( eventProcessed && !listener->isPropagatingProcessedEvents() )
			{
				break;
			}
		}
	}

	void
	Manager::charCallback (GLFWwindow * /*handle*/, unsigned int codepoint) noexcept
	{
		if constexpr ( KeyboardInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Unicode input detected (no modifier) !" "\n"
				"Unicode: " << codepoint << '\n';
		}

		for ( const auto & listener : s_instance->m_keyboardListeners )
		{
			if ( !listener->isListeningKeyboard() || !listener->isTextModeEnabled() )
			{
				continue;
			}

			const auto eventProcessed = listener->onCharacterType(codepoint);

			if ( eventProcessed && !listener->isPropagatingProcessedEvents() )
			{
				break;
			}
		}
	}

	void
	Manager::charModsCallback (GLFWwindow * /*handle*/, unsigned int codepoint, int modifiers) noexcept
	{
		if constexpr ( KeyboardInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Unicode input detected!" "\n"
				"Unicode: " << codepoint << "\n"
				"Keyboard modifiers: " << getModifierListString(modifiers) << "\n";
		}

		for ( const auto & listener : s_instance->m_keyboardListeners )
		{
			if ( !listener->isListeningKeyboard() || !listener->isTextModeEnabled() )
			{
				continue;
			}

			const auto eventProcessed = listener->onCharacterType(codepoint);

			if ( eventProcessed && !listener->isPropagatingProcessedEvents() )
			{
				break;
			}
		}
	}

	void
	Manager::dispatchRelativePointerPosition (double xPosition, double yPosition) noexcept
	{
		/* NOTE: Compute the relative position from the last one. */
		const auto deltaX = static_cast< float >(xPosition - s_instance->m_lastPointerCoordinates[0]);
		const auto deltaY = static_cast< float >(yPosition - s_instance->m_lastPointerCoordinates[1]);

		if constexpr ( PointerHeavyInputDebugEnabled )
		{
			TraceDebug{ClassId} << "[RelativeMode] X:" << deltaX << ", Y:" << deltaY << '\n';
		}

		if ( s_instance->m_moveEventsTracking != nullptr )
		{
			s_instance->m_moveEventsTracking->onPointerMove(deltaX, deltaY);
		}
		else
		{
			for ( const auto & listener : s_instance->m_pointerListeners )
			{
				/* NOTE: If the listener is disabled or in absolute mode, we jump to the next listener. */
				if ( !listener->isListeningPointer() || listener->isAbsoluteModeEnabled() )
				{
					continue;
				}

				/* NOTE: If the event is processed and the listener blocks events propagation, we stop the loop. */
				if ( listener->onPointerMove(deltaX, deltaY) && !listener->isPropagatingProcessedEvents() )
				{
					break;
				}
			}
		}

		/* Save the last position. */
		s_instance->m_lastPointerCoordinates[0] = xPosition;
		s_instance->m_lastPointerCoordinates[1] = yPosition;
	}

	void
	Manager::dispatchAbsolutePointerPosition (double xPosition, double yPosition) noexcept
	{
		const auto pointerX = static_cast< float >(xPosition);
		const auto pointerY = static_cast< float >(yPosition);

		if ( s_instance->m_moveEventsTracking != nullptr )
		{
			s_instance->m_moveEventsTracking->onPointerMove(pointerX, pointerY);
		}
		else
		{
			for ( const auto & listener : s_instance->m_pointerListeners )
			{
				/* NOTE: If the listener is disabled or in relative mode, we jump to the next listener. */
				if ( !listener->isListeningPointer() || listener->isRelativeModeEnabled() )
				{
					continue;
				}

				/* NOTE: If the event is processed and the listener blocks events propagation, we stop the loop. */
				if ( listener->onPointerMove(pointerX, pointerY) && !listener->isPropagatingProcessedEvents() )
				{
					break;
				}
			}
		}
	}

	void
	Manager::cursorPositionCallback (GLFWwindow * /*window*/, double xPosition, double yPosition) noexcept
	{
		if constexpr ( PointerHeavyInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Pointer move detected!" "\n"
				"[AbsoluteMode] X:" << xPosition << ", Y:" << yPosition << '\n';
		}

		if ( s_instance->m_pointerCoordinatesScalingEnabled )
		{
			xPosition *= s_instance->m_pointerScalingFactors[0];
			yPosition *= s_instance->m_pointerScalingFactors[1];
		}

		/* If the pointer is locked, we serve the listener in relative mode ('mouse look' like an FPS). */
		if ( s_instance->isPointerLocked() )
		{
			Manager::dispatchRelativePointerPosition(xPosition, yPosition);
		}
		else
		{
			Manager::dispatchAbsolutePointerPosition(xPosition, yPosition);
		}
	}

	void
	Manager::cursorEnterCallback (GLFWwindow * window, int entered) noexcept
	{
		if constexpr ( PointerInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Pointer window interaction detected!" "\n"
				"Action: " << ( entered == GLFW_TRUE ? "entering" : "leaving" ) <<  "\n";
		}

		/* NOTE: Retrieve the pointer position to set the entering/leaving coordinates. */
		const auto position = Manager::getPointerLocation(window);

		/* NOTE: This must always be processed to avoid bug. */
		if ( s_instance->m_moveEventsTracking != nullptr )
		{
			if ( entered == GLFW_TRUE )
			{
				s_instance->m_moveEventsTracking->onPointerEnter(position[0], position[1]);

				if ( !s_instance->m_pointerController.isAnyButtonPressed() )
				{
					s_instance->m_moveEventsTracking = nullptr;
				}
			}
			else
			{
				s_instance->m_moveEventsTracking->onPointerLeave(position[0], position[1]);
			}

			return;
		}

		for ( const auto & listener : s_instance->m_pointerListeners )
		{
			if ( !listener->isListeningPointer() )
			{
				continue;
			}

			const auto eventProcessed = entered == GLFW_TRUE ?
				listener->onPointerEnter(position[0], position[1]) :
				listener->onPointerLeave(position[0], position[1]);

			if ( eventProcessed && !listener->isPropagatingProcessedEvents() )
			{
				break;
			}
		}
	}

	void
	Manager::mouseButtonCallback (GLFWwindow * window, int button, int action, int modifiers) noexcept
	{
		/* NOTE: Retrieve the pointer position to set the click coordinates. */
		const auto position = Manager::getPointerLocation(window);

		if constexpr ( PointerInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Pointer click detected!" "\n"
				"Button number:" << button << "\n"
				"Position: " << position[0] << ", " << position[1] << "\n"
				"Action:" << ( action == GLFW_PRESS ? "Press" : "Release" ) << "\n"
				"Keyboard modifiers: " << getModifierListString(modifiers) << "\n";
		}

		/* NOTE: On release, automatically stop the tracking. */
		if ( s_instance->m_moveEventsTracking != nullptr && action == GLFW_RELEASE )
		{
			s_instance->m_moveEventsTracking->onButtonRelease(position[0], position[1], button, modifiers);
			s_instance->m_moveEventsTracking = nullptr;

			return;
		}

		for ( const auto & listener : s_instance->m_pointerListeners )
		{
			if ( !listener->isListeningPointer() )
			{
				continue;
			}

			auto eventProcessed = false;

			if ( action == GLFW_PRESS )
			{
				/* NOTE: Check if we need to lock this listener to track move events. */
				if ( !listener->isAbsoluteModeEnabled() && listener->isListenerLockedOnMoveEvents() )
				{
					s_instance->m_moveEventsTracking = listener;
				}

				eventProcessed = listener->onButtonPress(position[0], position[1], button, modifiers);
			}
			else
			{
				eventProcessed = listener->onButtonRelease(position[0], position[1], button, modifiers);
			}

			if ( eventProcessed && !listener->isPropagatingProcessedEvents() )
			{
				break;
			}
		}
	}

	void
	Manager::scrollCallback (GLFWwindow * window, double xOffset, double yOffset) noexcept
	{
		if constexpr ( PointerHeavyInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Scrolling detected!" "\n"
				"Offset X:" << xOffset << ", Y:" << yOffset << '\n';
		}

		/* NOTE: Retrieve the pointer position to set the scroll coordinates. */
		const auto position = Manager::getPointerLocation(window);

		const auto xOffsetF = static_cast< float >(xOffset);
		const auto yOffsetF = static_cast< float >(yOffset);

		/* NOTE: Retrieve keyboard modifiers (Ctrl, Shift, Alt, Super) during scroll */
		int32_t modifiers = 0;
		const auto leftCtrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL);
		const auto rightCtrl = glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL);
		const auto leftShift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
		const auto rightShift = glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT);

		if ( leftCtrl == GLFW_PRESS || rightCtrl == GLFW_PRESS )
		{
			modifiers |= GLFW_MOD_CONTROL;
		}
		if ( leftShift == GLFW_PRESS || rightShift == GLFW_PRESS )
		{
			modifiers |= GLFW_MOD_SHIFT;
		}
		if ( glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS )
		{
			modifiers |= GLFW_MOD_ALT;
		}
		if ( glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS )
		{
			modifiers |= GLFW_MOD_SUPER;
		}

		if ( s_instance->m_moveEventsTracking != nullptr )
		{
			/* NOTE: If the move is locked on one listener, checks for listening or relative mode is already done. */
			s_instance->m_moveEventsTracking->onMouseWheel(position[0], position[1], xOffsetF, yOffsetF, modifiers);
		}
		else
		{
			for ( const auto & listener : s_instance->m_pointerListeners )
			{
				if ( !listener->isListeningPointer() )
				{
					continue;
				}

				const auto eventProcessed = listener->onMouseWheel(position[0], position[1], xOffsetF, yOffsetF, modifiers);

				if ( eventProcessed && !listener->isPropagatingProcessedEvents() )
				{
					break;
				}
			}
		}
	}

	void
	Manager::dropCallback (GLFWwindow * /*handle*/, int count, const char * * paths) noexcept
	{
		if constexpr ( WindowEventsDebugEnabled )
		{
			TraceDebug{ClassId} << count << " files has been dropped into the window." "\n";
		}

		std::vector< std::filesystem::path > fsPaths;
		fsPaths.reserve(count);

		for ( auto index = 0; index < count; index++ )
		{
			if ( auto fsPath = IO::u8path(paths[index]); IO::fileExists(fsPath) )
			{
				TraceDebug{ClassId} << "Path dropped : " << IO::toU8String(fsPath);

				fsPaths.emplace_back(std::move(fsPath));
			}
			else
			{
				TraceWarning{ClassId} << "The file '" << IO::toU8String(fsPath) << "' doesn't exists! Skipping ...";
			}
		}

		/* NOTE: Only send existing files. */
		if ( !fsPaths.empty() )
		{
			s_instance->notify(DroppedFiles, fsPaths);
		}
	}

	void
	Manager::joystickCallback (int jid, int event) noexcept
	{
		if constexpr ( PointerInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Joystick/gamepad configuration changed!" "\n"
				"Device ID #" << jid << " is " << ( event == GLFW_CONNECTED ? "connected" : "disconnected" ) << "." "\n";
		}

		switch ( event )
		{
			case GLFW_CONNECTED :
				if ( glfwJoystickIsGamepad(jid) == GLFW_TRUE )
				{
					TraceInfo{ClassId} << "Gamepad '" << glfwGetGamepadName(jid) << "' (GUID:" <<  glfwGetJoystickGUID(jid) << ") connected at slot #" << jid << " !";

					s_instance->m_gamepadIDs.emplace(jid);
				}
				else
				{
					TraceInfo{ClassId} << "Joystick '" << glfwGetJoystickName(jid) << "' (GUID:" <<  glfwGetJoystickGUID(jid) << ") connected at slot #" << jid << " !";

					s_instance->m_joystickIDs.emplace(jid);
				}
				break;

			case GLFW_DISCONNECTED :
			{
				TraceInfo{ClassId} << "Game device #" << jid << " disconnected!";

				/* NOTE: Here we don't know if it's a joystick or a gamepad.
				 * But we don't care a bit because IDs are shared between gamepads and joysticks. */

				/* First, we reset the device last state. */
				JoystickController::clearDeviceState(jid);
				GamepadController::clearDeviceState(jid);

				/* We remove the ID from both ID sets. */
				s_instance->m_joystickIDs.erase(jid);
				s_instance->m_gamepadIDs.erase(jid);
			}
				break;

			default :
				break;
		}
	}

	bool
	Manager::onInitialize () noexcept
	{
		const auto & arguments = m_primaryServices.arguments();
		auto & settings = m_primaryServices.settings();

		m_showInformation =
			settings.getOrSetDefault< bool >(InputShowInformationKey, DefaultInputShowInformation) ||
			arguments.isSwitchPresent("--show-all-infos") ||
			arguments.isSwitchPresent("--show-input-infos");

		if ( arguments.isSwitchPresent("-W", "--window-less") )
		{
			m_windowLess = true;

			return true;
		}

		if ( !m_window.usable() )
		{
			Tracer::error(ClassId, "No handle available, cannot link input listeners!");

			return false;
		}

		this->linkWindowCallbacks(true, true);

		/* Update gamepad database. */
		std::string devicesDatabase;

		for ( auto filepath : m_primaryServices.fileSystem().dataDirectories() )
		{
			filepath.append(GameControllerDBFile);

			if ( !IO::fileExists(filepath) )
			{
				if ( m_showInformation )
				{
					TraceInfo{ClassId} << "The file " << filepath << " is not present there!";
				}

				continue;
			}

			if ( !IO::fileGetContents(filepath, devicesDatabase) )
			{
				TraceError{ClassId} << "Unable to read " << filepath << " !";

				continue;
			}

			if ( glfwUpdateGamepadMappings(devicesDatabase.c_str()) == GLFW_FALSE )
			{
				TraceError{ClassId} << "Update input devices from " << filepath << " failed!";

				continue;
			}

			if ( m_showInformation )
			{
				TraceSuccess{ClassId} << "Update input devices from " << filepath << " succeed!";
			}
		}

		if ( devicesDatabase.empty() )
		{
			TraceWarning{ClassId} << "There was no " << GameControllerDBFile << " file available!";
		}

		/* Checks every device connected. */
		for ( int jid = 0; jid <= GLFW_JOYSTICK_LAST; jid++ )
		{
			/* Checks if a device is present. */
			if ( glfwJoystickPresent(jid) == GLFW_FALSE )
			{
				continue;
			}

			/* If present, determine if it's a gamepad or a joystick. */
			if ( glfwJoystickIsGamepad(jid) == GLFW_TRUE )
			{
				m_gamepadIDs.emplace(jid);

				if ( m_showInformation )
				{
					TraceSuccess{ClassId} << "Gamepad '" << glfwGetGamepadName(jid) << "' (GUID:" <<  glfwGetJoystickGUID(jid) << ") available at slot #" << jid;
				}
			}
			else
			{
				m_joystickIDs.emplace(jid);

				if ( m_showInformation )
				{
					TraceSuccess{ClassId} << "Joystick '" << glfwGetJoystickName(jid) << "' (GUID:" <<  glfwGetJoystickGUID(jid) << ") available at slot #" << jid;
				}
			}
		}

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
		if ( !m_window.usable() )
		{
			Tracer::warning(ClassId, "No handle was available!");

			return false;
		}

		/* Disables every callback. */
		if ( !m_windowLess )
		{
			this->unlinkWindowCallbacks();
		}

		return true;
	}

	void
	Manager::waitSystemEvents (double until) const noexcept
	{
		if ( !m_windowLess )
		{
			if ( this->isCopyKeyboardStateEnabled() )
			{
				KeyboardController::readDeviceState(m_window);
			}

			if ( this->isCopyPointerStateEnabled() )
			{
				PointerController::readDeviceState(m_window);
			}

			if ( this->isCopyJoysticksStateEnabled() )
			{
				for ( const auto & joystickID : m_joystickIDs )
				{
					JoystickController::readDeviceState(joystickID);
				}
			}

			if ( this->isCopyGamepadsStateEnabled() )
			{
				for ( const auto & gamepadID : m_gamepadIDs )
				{
					GamepadController::readDeviceState(gamepadID);
				}
			}
		}

		/* This function is blocking the process by waiting for an event from a system. */
		if ( until > 0.0 )
		{
			glfwWaitEventsTimeout(until);
		}
		else
		{
			glfwWaitEvents();
		}
	}

	void
	Manager::wakeUpEventsLoop () noexcept
	{
		/* NOTE: Documented callable from any thread. Before GLFW initialization (or after
		 * termination), GLFW reports GLFW_NOT_INITIALIZED through the error callback and
		 * the call is a harmless no-op. */
		glfwPostEmptyEvent();
	}

	void
	Manager::addKeyboardListener (KeyboardListenerInterface * listener) noexcept
	{
		if ( std::ranges::binary_search(std::as_const(m_keyboardListeners), listener) )
		{
			TraceWarning{ClassId} << "Listener @" << listener << " already added!";

			return;
		}

		m_keyboardListeners.emplace(m_keyboardListeners.begin(), listener);
	}

	void
	Manager::removeKeyboardListener (KeyboardListenerInterface * listener) noexcept
	{
		m_keyboardListeners.erase(std::ranges::remove(m_keyboardListeners, listener).begin(), m_keyboardListeners.end());
	}

	void
	Manager::removeAllKeyboardListeners () noexcept
	{
		m_keyboardListeners.clear();
	}

	void
	Manager::addPointerListener (PointerListenerInterface * listener, bool priority) noexcept
	{
		/* ⚠️ `find`, NOT `binary_search`: this vector is insertion-ordered and never sorted, so a
		 * binary search over it is meaningless — the duplicate guard it was written as could miss a
		 * real duplicate or invent one, at the compiler's discretion. */
		if ( std::ranges::find(m_pointerListeners, listener) != m_pointerListeners.cend() )
		{
			TraceWarning{ClassId} << "Listener @" << listener << " already added!";

			return;
		}

		/* ⚠️⚠️ THE INVARIANT: the priority listener, when there is one, sits at index 0 and every
		 * other listener is inserted just after it. That is why the five dispatch loops below need
		 * no special case — they simply walk the vector, and none of them can be forgotten.
		 * Everything else keeps the newest-first order it always had. */
		if ( priority )
		{
			if ( m_priorityPointerListener != nullptr && m_priorityPointerListener != listener )
			{
				TraceWarning{ClassId} << "Pointer listener @" << m_priorityPointerListener << " loses its priority to @" << listener << ".";
			}

			m_priorityPointerListener = listener;

			m_pointerListeners.emplace(m_pointerListeners.begin(), listener);

			return;
		}

		m_pointerListeners.emplace(m_pointerListeners.begin() + (m_priorityPointerListener != nullptr ? 1 : 0), listener);
	}

	void
	Manager::removePointerListener (PointerListenerInterface * listener) noexcept
	{
		/* The slot has to be released with the listener, or the next insertion would keep skipping
		 * index 0 to protect a priority listener that is no longer there. */
		if ( m_priorityPointerListener == listener )
		{
			m_priorityPointerListener = nullptr;
		}

		m_pointerListeners.erase(std::ranges::remove(m_pointerListeners, listener).begin(), m_pointerListeners.end());
	}

	void
	Manager::removeAllPointerListeners () noexcept
	{
		/* Released with the listeners: a dangling slot would make every later insertion skip index 0
		 * to protect a priority listener that no longer exists. */
		m_priorityPointerListener = nullptr;

		m_pointerListeners.clear();
	}

	void
	Manager::lockPointer () noexcept
	{
		auto * window = m_window.handle();

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		if ( glfwRawMouseMotionSupported() == GLFW_TRUE )
		{
			TraceSuccess{ClassId} << "Raw mouse motion enabled!";

			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}

		m_pointerLocked = true;
	}

	void
	Manager::unlockPointer () noexcept
	{
		auto * window = m_window.handle();

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		if ( glfwRawMouseMotionSupported() == GLFW_TRUE )
		{
			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);
		}

		m_pointerLocked = false;
	}

	void
	Manager::injectKeyEvent (int32_t key, int32_t modifiers, int32_t action) noexcept
	{
		if ( s_instance == nullptr )
		{
			return;
		}

		for ( const auto & listener : s_instance->m_keyboardListeners )
		{
			if ( !listener->isListeningKeyboard() )
			{
				continue;
			}

			auto eventProcessed = false;

			switch ( action )
			{
				case GLFW_PRESS :
					eventProcessed = listener->onKeyPress(key, 0, modifiers, false);
					break;

				case GLFW_REPEAT :
					eventProcessed = listener->onKeyPress(key, 0, modifiers, true);
					break;

				case GLFW_RELEASE :
					eventProcessed = listener->onKeyRelease(key, 0, modifiers);
					break;

				default:
					break;
			}

			if ( eventProcessed && !listener->isPropagatingProcessedEvents() )
			{
				break;
			}
		}
	}

	void
	Manager::injectMouseClickEvent (float positionX, float positionY, int32_t button, int32_t modifiers, int32_t action) noexcept
	{
		if ( s_instance == nullptr )
		{
			return;
		}

		/* NOTE: Copy the listener list before iterating (same reason as keyboard). */
		const auto pointerListeners = s_instance->m_pointerListeners;

		for ( const auto & listener : pointerListeners )
		{
			if ( !listener->isListeningPointer() )
			{
				continue;
			}

			auto eventProcessed = false;

			if ( action == GLFW_PRESS )
			{
				eventProcessed = listener->onButtonPress(positionX, positionY, button, modifiers);
			}
			else
			{
				eventProcessed = listener->onButtonRelease(positionX, positionY, button, modifiers);
			}

			if ( eventProcessed && !listener->isPropagatingProcessedEvents() )
			{
				break;
			}
		}
	}

	void
	Manager::injectPointerMoveEvent (float positionX, float positionY) noexcept
	{
		if ( s_instance == nullptr )
		{
			return;
		}

		const auto pointerListeners = s_instance->m_pointerListeners;

		for ( const auto & listener : pointerListeners )
		{
			if ( !listener->isListeningPointer() || listener->isRelativeModeEnabled() )
			{
				continue;
			}

			if ( listener->onPointerMove(positionX, positionY) && !listener->isPropagatingProcessedEvents() )
			{
				break;
			}
		}
	}

	void
	Manager::onRegisterToConsole () noexcept
	{
		/* Injected values reach listeners that may index per-key / per-button state: an out-of-range
		 * code is refused here rather than handed to them. */
		const auto checkKey = [] (int32_t key, int32_t modifiers, std::string & error) {
			if ( key < GLFW_KEY_SPACE || key > GLFW_KEY_LAST )
			{
				error = "Key code " + std::to_string(key) + " is not a GLFW key (" + std::to_string(GLFW_KEY_SPACE) + " to " + std::to_string(GLFW_KEY_LAST) + ").";

				return false;
			}

			if ( modifiers < 0 || modifiers > ( GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER | GLFW_MOD_CAPS_LOCK | GLFW_MOD_NUM_LOCK ) )
			{
				error = "Modifiers " + std::to_string(modifiers) + " is not a GLFW modifier bit mask (0 to 63).";

				return false;
			}

			return true;
		};

		const auto checkButton = [checkKey] (int32_t button, int32_t modifiers, std::string & error) {
			if ( button < GLFW_MOUSE_BUTTON_1 || button > GLFW_MOUSE_BUTTON_LAST )
			{
				error = "Mouse button " + std::to_string(button) + " is not a GLFW button (0 to " + std::to_string(GLFW_MOUSE_BUTTON_LAST) + ").";

				return false;
			}

			/* NOTE: same modifier rule as a key; GLFW_KEY_SPACE only satisfies the key half of the check. */
			return checkKey(GLFW_KEY_SPACE, modifiers, error);
		};

		this->bindCommand("keyPress", "Injects a key press then release. It reaches Core-level bindings, never a focused CEF field (the letters run as application shortcuts).",
			{
				{"key", "The GLFW key code (e.g. 298 for F9, 256 for Escape)."},
				{"modifiers", "The GLFW modifier bit mask (1 Shift, 2 Control, 4 Alt, 8 Super).", 0}
			},
			[checkKey] (int32_t key, int32_t modifiers) {
				std::string error;

				if ( !checkKey(key, modifiers, error) )
				{
					return Console::CommandResult::error(error);
				}

				Manager::injectKeyEvent(key, modifiers, GLFW_PRESS);
				Manager::injectKeyEvent(key, modifiers, GLFW_RELEASE);

				return Console::CommandResult::success("Key event injected.");
			});

		/* NOTE: Read-only probe. The pointer lock is expressed twice — as m_pointerLocked (which
		 * selects relative "FPS" dispatch over absolute dispatch, see cursorPositionCallback) and
		 * as the GLFW cursor mode (which decides whether the OS cursor is drawn and grabbed).
		 * lockPointer()/unlockPointer() are the only writers and always set both, so the two MUST
		 * agree; a disagreement means someone else moved the cursor mode, and the visible symptom
		 * is "the mouse behaves like an FPS but the cursor is still displayed". This command is the
		 * only way to observe that state — a screenshot captures the swap chain and never shows the
		 * OS cursor. */
		this->bindCommand("pointerState", "Reports the pointer lock state and the GLFW cursor mode, and flags a desync between them.", [this] () {
			if ( m_windowLess || !m_window.usable() )
			{
				return Console::CommandResult::error("No usable window — pointer state is meaningless.");
			}

			const auto cursorMode = glfwGetInputMode(m_window.handle(), GLFW_CURSOR);

			const auto * cursorModeLabel = [cursorMode] () -> const char * {
				switch ( cursorMode )
				{
					case GLFW_CURSOR_NORMAL :
						return "NORMAL (cursor visible, not grabbed)";

					case GLFW_CURSOR_HIDDEN :
						return "HIDDEN (cursor invisible, not grabbed)";

					case GLFW_CURSOR_DISABLED :
						return "DISABLED (cursor invisible, grabbed)";

					case GLFW_CURSOR_CAPTURED :
						return "CAPTURED (cursor visible, confined)";

					default :
						return "unknown";
				}
			}();

			Console::Outputs outputs;

			outputs.emplace_back(Severity::Info, std::stringstream{} <<
				"isPointerLocked(): " << (m_pointerLocked ? "true" : "false") <<
				" | GLFW cursor mode: " << cursorModeLabel <<
				" | dispatch: " << (m_pointerLocked ? "relative (FPS)" : "absolute")
			);

			if ( m_pointerLocked != (cursorMode == GLFW_CURSOR_DISABLED) )
			{
				outputs.emplace_back(Severity::Error, "DESYNC — the lock flag and the GLFW cursor mode disagree. This is the defect, not a display quirk.");
			}

			return Console::CommandResult::fromOutputs(std::move(outputs), true);
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("mouseClick", "Injects a mouse click (press then release, so it cannot hold a drag).",
			{
				{"x", "Horizontal position, in PHYSICAL pixels from the left of the framebuffer."},
				{"y", "Vertical position, in PHYSICAL pixels from the top of the framebuffer."},
				{"button", "The GLFW mouse button (0 left, 1 right, 2 middle).", 0},
				{"modifiers", "The GLFW modifier bit mask (1 Shift, 2 Control, 4 Alt, 8 Super).", 0}
			},
			[checkButton] (float x, float y, int32_t button, int32_t modifiers) {
				std::string error;

				if ( !checkButton(button, modifiers, error) )
				{
					return Console::CommandResult::error(error);
				}

				Manager::injectMouseClickEvent(x, y, button, modifiers, GLFW_PRESS);
				Manager::injectMouseClickEvent(x, y, button, modifiers, GLFW_RELEASE);

				return Console::CommandResult::success("Mouse click injected.");
			});

		/* ⚠️ mousePress/mouseRelease exist because mouseClick CANNOT express a drag: it releases
		 * immediately, so any pointer move sent afterwards arrives with the button already up and a
		 * drag-driven control sees nothing. That is not hypothetical — it made the scene's orbit
		 * camera untestable from the console on 2026-09-18, and a fix to the pointer dispatch had to
		 * ship verified by code reading instead of by measurement. A drag is now:
		 *   mousePress(x, y) ; mouseMove(...) ... ; mouseRelease(x, y)
		 * ⚠️ The press and the release are NOT paired by the engine: a caller that forgets the
		 * release leaves the control believing the button is still down. */
		this->bindCommand("mousePress", "Injects a mouse button PRESS and leaves it down, so a drag can be performed with mouseMove.",
			{
				{"x", "Horizontal position, in PHYSICAL pixels from the left of the framebuffer."},
				{"y", "Vertical position, in PHYSICAL pixels from the top of the framebuffer."},
				{"button", "The GLFW mouse button (0 left, 1 right, 2 middle).", 0},
				{"modifiers", "The GLFW modifier bit mask (1 Shift, 2 Control, 4 Alt, 8 Super).", 0}
			},
			[checkButton] (float x, float y, int32_t button, int32_t modifiers) {
				std::string error;

				if ( !checkButton(button, modifiers, error) )
				{
					return Console::CommandResult::error(error);
				}

				Manager::injectMouseClickEvent(x, y, button, modifiers, GLFW_PRESS);

				return Console::CommandResult::success("Mouse button press injected (it stays DOWN until mouseRelease).");
			});

		this->bindCommand("mouseRelease", "Injects a mouse button RELEASE, ending a drag started with mousePress.",
			{
				{"x", "Horizontal position, in PHYSICAL pixels from the left of the framebuffer."},
				{"y", "Vertical position, in PHYSICAL pixels from the top of the framebuffer."},
				{"button", "The GLFW mouse button (0 left, 1 right, 2 middle).", 0},
				{"modifiers", "The GLFW modifier bit mask (1 Shift, 2 Control, 4 Alt, 8 Super).", 0}
			},
			[checkButton] (float x, float y, int32_t button, int32_t modifiers) {
				std::string error;

				if ( !checkButton(button, modifiers, error) )
				{
					return Console::CommandResult::error(error);
				}

				Manager::injectMouseClickEvent(x, y, button, modifiers, GLFW_RELEASE);

				return Console::CommandResult::success("Mouse button release injected.");
			});

		this->bindCommand("mouseMove", "Injects a pointer move (absolute dispatch; ignored by a listener in relative mode).",
			{
				{"x", "Horizontal position, in PHYSICAL pixels from the left of the framebuffer."},
				{"y", "Vertical position, in PHYSICAL pixels from the top of the framebuffer."}
			},
			[] (float x, float y) {
				Manager::injectPointerMoveEvent(x, y);

				return Console::CommandResult::success("Mouse move injected.");
			});
	}
}
