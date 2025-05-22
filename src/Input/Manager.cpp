/*
 * src/Input/Manager.cpp
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

#include "Manager.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <iostream>
#include <utility>

/* Third-party libraries */
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

/* Local inclusions. */
#include "Libs/Utility.hpp"
#include "Libs/IO/IO.hpp"
#include "GamepadController.hpp"
#include "JoystickController.hpp"
#include "PrimaryServices.hpp"
#include "Window.hpp"
#include "SettingKeys.hpp"

namespace EmEn::Input
{
	using namespace EmEn::Libs;
	using namespace Vulkan;

	const size_t Manager::ClassUID{getClassUID(ClassId)};
	Manager * Manager::s_instance{nullptr};

	Manager::Manager (PrimaryServices & primaryServices, Window & window) noexcept
		: ServiceInterface(ClassId),
		m_primaryServices(primaryServices),
		m_window(window)
	{
		if ( s_instance != nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

			std::terminate();
		}

		s_instance = this;
	}

	Manager::~Manager ()
	{
		s_instance = nullptr;
	}

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

			m_flags[IsListeningKeyboard] = true;
		}

		/* Pointer listeners. */
		if ( enablePointer )
		{
			glfwSetMouseButtonCallback(window, mouseButtonCallback);
			glfwSetCursorPosCallback(window, cursorPositionCallback);
			glfwSetCursorEnterCallback(window, cursorEnterCallback);
			glfwSetScrollCallback(window, scrollCallback);

			/* TODO: Make it optional */
			glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

			m_flags[IsListeningPointer] = true;
		}

		/* Misc. listeners. */
		/* TODO: Make it optional */
		glfwSetDropCallback(window, dropCallback);
		glfwSetJoystickCallback(joystickCallback);

		m_flags[WindowLinked] = true;
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

		/* Misc. listeners. */
		glfwSetDropCallback(window, nullptr);
		glfwSetJoystickCallback(nullptr);

		m_flags[WindowLinked] = false;
	}

	void
	Manager::enableKeyboardListening (bool state) noexcept
	{
		if ( m_flags[IsListeningKeyboard] == state )
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

		m_flags[IsListeningKeyboard] = state;
	}

	void
	Manager::enablePointerListening (bool state) noexcept
	{
		if ( m_flags[IsListeningPointer] == state )
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

		m_flags[IsListeningPointer] = state;
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

		if ( s_instance->m_flags[PointerCoordinatesScalingEnabled] )
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
				"Keyboard input detected !" "\n"
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
				"Unicode input detected !" "\n"
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

		if constexpr ( PointerInputDebugEnabled )
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
		if constexpr ( PointerInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Pointer move detected !" "\n"
				"[AbsoluteMode] X:" << xPosition << ", Y:" << yPosition << '\n';
		}

		if ( s_instance->m_flags[PointerCoordinatesScalingEnabled] )
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
				"Pointer window interaction detected !" "\n"
				"Action: " << ( entered == GLFW_TRUE ? "entering" : "leaving" ) <<  "\n";
		}

		/* NOTE: Retrieve the pointer position to set the entering/leaving coordinates. */
		const auto position = getPointerLocation(window);

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
		if constexpr ( PointerInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Pointer click detected !" "\n"
				"Button number:" << button << "\n"
				"Action:" << ( action == GLFW_PRESS ? "Press" : "Release" ) << "\n"
				"Keyboard modifiers: " << getModifierListString(modifiers) << "\n";
		}

		/* NOTE: Retrieve the pointer position to set the click coordinates. */
		const auto position = Manager::getPointerLocation(window);

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
		if constexpr ( PointerInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Scrolling detected !" "\n"
				"Offset X:" << xOffset << ", Y:" << yOffset << '\n';
		}

		/* NOTE: Retrieve the pointer position to set the scroll coordinates. */
		const auto position = getPointerLocation(window);

		const auto xOffsetF = static_cast< float >(xOffset);
		const auto yOffsetF = static_cast< float >(yOffset);

		if ( s_instance->m_moveEventsTracking != nullptr )
		{
			/* NOTE: If the move is locked on one listener, checks for listening or relative mode is already done. */
			s_instance->m_moveEventsTracking->onMouseWheel(position[0], position[1], xOffsetF, yOffsetF);
		}
		else
		{
			for ( const auto & listener : s_instance->m_pointerListeners )
			{
				if ( !listener->isListeningPointer() )
				{
					continue;
				}

				const auto eventProcessed = listener->onMouseWheel(position[0], position[1], xOffsetF, yOffsetF);

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

		std::vector< std::filesystem::path > filePaths;
		filePaths.reserve(count);

		for ( auto i = 0; i < count; i++ )
		{
			const std::filesystem::path filepath{paths[i]};

			if ( !IO::fileExists(filepath) )
			{
				TraceError{ClassId} << "File '" << filepath << "' doesn't exists !";

				continue;
			}

			filePaths.emplace_back(filepath);
		}

		s_instance->notify(DroppedFiles, filePaths);
	}

	void
	Manager::joystickCallback (int jid, int event) noexcept
	{
		if constexpr ( PointerInputDebugEnabled )
		{
			TraceDebug{ClassId} <<
				"Joystick/gamepad configuration changed !" "\n"
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
				TraceInfo{ClassId} << "Game device #" << jid << " disconnected !";

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
		m_flags[ShowInformation] = m_primaryServices.settings().get< bool >(InputShowInformationKey, DefaultInputShowInformation);

		if ( !m_window.usable() )
		{
			Tracer::error(ClassId, "No handle available, cannot link input listeners !");

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
				if ( m_flags[ShowInformation] )
				{
					TraceInfo{ClassId} << "The file " << filepath << " is not present there !";
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
				TraceError{ClassId} << "Update input devices from " << filepath << " failed !";

				continue;
			}

			if ( m_flags[ShowInformation] )
			{
				TraceSuccess{ClassId} << "Update input devices from " << filepath << " succeed !";
			}
		}

		if ( devicesDatabase.empty() )
		{
			TraceWarning{ClassId} << "There was no " << GameControllerDBFile << " file available !";
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

				if ( m_flags[ShowInformation] )
				{
					TraceSuccess{ClassId} << "Gamepad '" << glfwGetGamepadName(jid) << "' (GUID:" <<  glfwGetJoystickGUID(jid) << ") available at slot #" << jid;
				}
			}
			else
			{
				m_joystickIDs.emplace(jid);

				if ( m_flags[ShowInformation] )
				{
					TraceSuccess{ClassId} << "Joystick '" << glfwGetJoystickName(jid) << "' (GUID:" <<  glfwGetJoystickGUID(jid) << ") available at slot #" << jid;
				}
			}
		}

		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
		m_flags[ServiceInitialized] = false;

		if ( !m_window.usable() )
		{
			Tracer::warning(ClassId, "No handle was available !");

			return false;
		}

		/* Disables every callback. */
		this->unlinkWindowCallbacks();

		return true;
	}

	void
	Manager::pollSystemEvents () const noexcept
	{
		if ( m_flags[CopyKeyboardStateEnabled] )
		{
			KeyboardController::readDeviceState(m_window);
		}

		if ( m_flags[CopyPointerStateEnabled] )
		{
			PointerController::readDeviceState(m_window);
		}

		if ( m_flags[CopyJoysticksStateEnabled] )
		{
			for ( const auto & joystickID : m_joystickIDs )
			{
				JoystickController::readDeviceState(joystickID);
			}
		}

		if ( m_flags[CopyGamepadsStateEnabled] )
		{
			for ( const auto & gamepadID : m_gamepadIDs )
			{
				GamepadController::readDeviceState(gamepadID);
			}
		}

		glfwPollEvents();
	}

	void
	Manager::waitSystemEvents (double until) noexcept
	{
		/* This function is blocking the
		 * process by waiting an event from a system. */
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
	Manager::addKeyboardListener (KeyboardListenerInterface * listener) noexcept
	{
		if ( std::ranges::binary_search(std::as_const(m_keyboardListeners), listener) )
		{
			TraceWarning{ClassId} << "Listener @" << listener << " already added !";

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
	Manager::addPointerListener (PointerListenerInterface * listener) noexcept
	{
		if ( std::ranges::binary_search(std::as_const(m_pointerListeners), listener) )
		{
			TraceWarning{ClassId} << "Listener @" << listener << " already added !";

			return;
		}

		m_pointerListeners.emplace(m_pointerListeners.begin(), listener);
	}

	void
	Manager::removePointerListener (PointerListenerInterface * listener) noexcept
	{
		m_pointerListeners.erase(std::ranges::remove(m_pointerListeners, listener).begin(), m_pointerListeners.end());
	}

	void
	Manager::removeAllPointerListeners () noexcept
	{
		m_pointerListeners.clear();
	}

	void
	Manager::lockPointer () noexcept
	{
		auto * window = m_window.handle();

		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		if ( glfwRawMouseMotionSupported() == GLFW_TRUE )
		{
			TraceSuccess{ClassId} << "Raw mouse motion enabled !";

			glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
		}

		m_flags[PointerLocked] = true;
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

		m_flags[PointerLocked] = false;
	}
}
