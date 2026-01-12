/*
 * src/Input/Manager.hpp
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

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstddef>
#include <array>
#include <vector>
#include <set>

/* Third-party libraries */
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Libs/ObservableTrait.hpp"

/* Local inclusions for usage */
#include "KeyboardListenerInterface.hpp"
#include "PointerListenerInterface.hpp"
#include "KeyboardController.hpp"
#include "PointerController.hpp"

/* Forward declarations. */
namespace EmEn
{
	class PrimaryServices;
	class Window;
}

namespace EmEn::Input
{
	/**
	 * @brief The input manager service class.
	 * @note [OBS][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Libs::ObservableTrait This service is observable.
	 */
	class Manager final : public ServiceInterface, public Libs::ObservableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"InputManagerService"};

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				DroppedFiles,
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs the input manager.
			 * @param primaryServices A reference to primary services.
			 * @param window A reference to the handle.
			 */
			Manager (PrimaryServices & primaryServices, Window & window) noexcept
				: ServiceInterface{ClassId},
				m_primaryServices{primaryServices},
				m_window{window}
			{
				if ( s_instance != nullptr )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

					std::terminate();
				}

				s_instance = this;
			}

			/**
			 * @brief Destructs the resource manager.
			 */
			~Manager () override
			{
				s_instance = nullptr;
			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Libs::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/**
			 * @brief Returns a keyboard controller.
			 * @return const KeyboardController &
			 */
			[[nodiscard]]
			const KeyboardController &
			keyboardController () const noexcept
			{
				return m_keyboardController;
			}

			/**
			 * @brief Returns a keyboard controller.
			 * @return KeyboardController &
			 */
			[[nodiscard]]
			KeyboardController &
			keyboardController () noexcept
			{
				return m_keyboardController;
			}

			/**
			 * @brief Returns a pointer controller.
			 * @return const PointerController &
			 */
			[[nodiscard]]
			const PointerController &
			pointerController () const noexcept
			{
				return m_pointerController;
			}

			/**
			 * @brief Returns a pointer controller.
			 * @return PointerController &
			 */
			[[nodiscard]]
			PointerController &
			pointerController () noexcept
			{
				return m_pointerController;
			}

			/**
			 * @brief Controls whether the keyboard events are sent to listeners.
			 * @note This is a global switch but doesn't affect the keyboard controller direct state reading.
			 * @param state The state.
			 * @return void
			 */
			void enableKeyboardListening (bool state) noexcept;

			/**
			 * @brief Returns whether the manager is listening to keyboard input.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isListeningKeyboard () const noexcept
			{
				return m_isListeningKeyboard;
			}

			/**
			 * @brief @brief Controls whether the pointer events are sent to listeners.
			 * @note This is a global switch but doesn't affect the pointer controller direct state reading.
			 * @param state The state.
			 * @return void
			 */
			void enablePointerListening (bool state) noexcept;

			/**
			 * @brief Returns whether the manager is listening to pointer input.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isListeningPointer () const noexcept
			{
				return m_isListeningPointer;
			}

			/**
			 * @brief Returns whether the pointer is locked on screen (FPS mode).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPointerLocked () const noexcept
			{
				return m_pointerLocked;
			}

			/**
			 * @brief Enables the pointer scaling to meet HDPI screen coordinates.
			 * @param xScale The X scale factor.
			 * @param yScale The Y scale factor.
			 * @return void
			 */
			void
			enablePointerScaling (double xScale, double yScale) noexcept
			{
				m_pointerScalingFactors[0] = xScale;
				m_pointerScalingFactors[1] = yScale;

				m_pointerCoordinatesScalingEnabled = true;
			}

			/**
			 * @brief Disables the pointer scaling.
			 * @return void
			 */
			void
			disablePointerScaling () noexcept
			{
				m_pointerScalingFactors[0] = 1.0;
				m_pointerScalingFactors[1] = 1.0;

				m_pointerCoordinatesScalingEnabled = false;
			}

			/**
			 * @brief Returns whether the pointer scaling is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPointerScalingEnabled () const noexcept
			{
				return m_pointerCoordinatesScalingEnabled;
			}

			/**
			 * @brief Enables the copy of the keyboard state.
			 * @note Direct query of the keyboard state must be done on the main-thread.
			 * @param state The state.
			 * @return void
			 */
			void
			enableCopyKeyboardState (bool state) noexcept
			{
				m_copyKeyboardStateEnabled = state;
			}

			/**
			 * @brief Returns whether the copy of the keyboard state is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCopyKeyboardStateEnabled () const noexcept
			{
				return m_copyKeyboardStateEnabled;
			}

			/**
			 * @brief Enables the copy of the pointer state.
			 * @note Direct query of the pointer state must be done on the main-thread.
			 * @param state The state.
			 * @return void
			 */
			void
			enableCopyPointerState (bool state) noexcept
			{
				m_copyPointerStateEnabled = state;
			}

			/**
			 * @brief Returns whether the copy of the pointer state is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCopyPointerStateEnabled () const noexcept
			{
				return m_copyPointerStateEnabled;
			}

			/**
			 * @brief Enables the copy of joystick state.
			 * @note Direct query of the joystick state must be done on the main-thread.
			 * @param state The state.
			 * @return void
			 */
			void
			enableCopyJoysticksState (bool state) noexcept
			{
				m_copyJoysticksStateEnabled = state;
			}

			/**
			 * @brief Returns whether the copy of joystick state is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCopyJoysticksStateEnabled () const noexcept
			{
				return m_copyJoysticksStateEnabled;
			}

			/**
			 * @brief Enables the copy of gamepad state.
			 * @note Direct query of the gamepad state must be done on the main-thread.
			 * @param state The state.
			 * @return void
			 */
			void
			enableCopyGamepadsState (bool state) noexcept
			{
				m_copyGamepadsStateEnabled = state;
			}

			/**
			 * @brief Returns whether the copy of the gamepad state is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCopyGamepadsStateEnabled () const noexcept
			{
				return m_copyGamepadsStateEnabled;
			}

			/**
			 * @brief Hides the mouse cursor, and the manager will only serve listeners in relative mode.
			 * @note The mouse move event will give the difference on the X and Y axis.
			 * @return void
			 */
			void lockPointer () noexcept;

			/**
			 * @brief Shows the mouse cursor, and the manager will only serve listeners in absolute mode.
			 * @note The mouse move event will give the absolute pointer XY coordinates on the screen.
			 * @return void
			 */
			void unlockPointer () noexcept;

			/**
			 * @brief Waits for a system event.
			 * @param until Sets a duration until the cancellation of the waiting in seconds.
			 * @return void
			 */
			void waitSystemEvents (double until = 0.0) const noexcept;

			/**
			 * @brief Adds an object the keyboard can control, like a player.
			 * @param listener A pointer to a keyboard listener interfaced object.
			 * @return void
			 */
			void addKeyboardListener (KeyboardListenerInterface * listener) noexcept;

			/**
			 * @brief Removes an object of keyboard listeners.
			 * @param listener A pointer to a keyboard listener interfaced object.
			 * @return void
			 */
			void removeKeyboardListener (KeyboardListenerInterface * listener) noexcept;

			/**
			 * @brief Clears all keyboard listeners.
			 * @return void
			 */
			void removeAllKeyboardListeners () noexcept;

			/**
			 * @brief Adds an object the pointer can control, like a player.
			 * @param listener A pointer to a pointer listener interfaced object.
			 * @return void
			 */
			void addPointerListener (PointerListenerInterface * listener) noexcept;

			/**
			 * @brief Removes an object of pointer listeners.
			 * @param listener A pointer to a pointer listener interfaced object.
			 * @return void
			 */
			void removePointerListener (PointerListenerInterface * listener) noexcept;

			/**
			 * @brief Clears all pointer listeners.
			 * @return void
			 */
			void removeAllPointerListeners () noexcept;

#if IS_MACOS
			/**
			 * @brief Installs macOS-specific gesture handlers (pinch-to-zoom, etc.).
			 * @param window The GLFW window to monitor
			 * @return void
			 */
			void installMacOSGestureHandlers (GLFWwindow * window) noexcept;

			/**
			 * @brief Removes macOS gesture handlers.
			 * @return void
			 */
			void removeMacOSGestureHandlers () noexcept;
#endif

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Main method to attach all events callbacks from a glfw handle.
			 * @param enableKeyboard Enables listeners relative to the keyboard.
			 * @param enablePointer Enables listeners relative to the pointer.
			 * @return void
			 */
			void linkWindowCallbacks (bool enableKeyboard, bool enablePointer) noexcept;

			/**
			 * @brief Removes all callback functions set to the window.
			 * @return void
			 */
			void unlinkWindowCallbacks () noexcept;

			/**
			 * @brief Returns the pointer location to complete some pointer events data.
			 * @param window A pointer to the window.
			 * @return std::array< float, 2 >
			 */
			[[nodiscard]]
			static std::array< float, 2 > getPointerLocation (GLFWwindow * window) noexcept;

			/**
			 * @brief The callback for the glfw API to handle key inputs.
			 * @param window The glfw handle.
			 * @param key The keyboard universal key code. I.e., QWERTY keyboard 'A' key gives the ASCII code '65' on all platforms.
			 * @param scancode The OS dependent scancode.
			 * @param action The key event.
			 * @param modifiers The modifier keys mask.
			 * @return void
			 */
			static void keyCallback (GLFWwindow * window, int key, int scancode, int action, int modifiers) noexcept;

			/**
			 * @brief The callback for the glfw API to handle character inputs.
			 * @param window The glfw handle.
			 * @param codepoint The Unicode value of the character.
			 * @return void
			 */
			static void charCallback (GLFWwindow * window, unsigned int codepoint) noexcept;

			/**
			 * @brief The callback for the glfw API to handle character modification inputs.
			 * @param window The glfw window handle.
			 * @param codepoint The Unicode value of the character.
			 * @param modifiers The modifier keys mask.
			 * @return void
			 */
			static void charModsCallback (GLFWwindow * window, unsigned int codepoint, int modifiers) noexcept;

			/**
			 * @brief Separate method for a relative mode pointer called by the main Manager::cursorPositionCallback() method.
			 * @param xPosition The X position of the cursor.
			 * @param yPosition The Y position of the cursor.
			 * @return void
			 */
			static void dispatchRelativePointerPosition (double xPosition, double yPosition) noexcept;

			/**
			 * @brief Separate method for an absolute mode pointer called by the main Manager::cursorPositionCallback() method.
			 * @param xPosition The X position of the cursor.
			 * @param yPosition The Y position of the cursor.
			 * @return void
			 */
			static void dispatchAbsolutePointerPosition (double xPosition, double yPosition) noexcept;

			/**
			 * @brief The callback for the glfw API to handle cursor position changes.
			 * @param window The glfw handle.
			 * @param xPosition The X position of the cursor.
			 * @param yPosition The Y position of the cursor.
			 * @return void
			 */
			static void cursorPositionCallback (GLFWwindow * window, double xPosition, double yPosition) noexcept;

			/**
			 * @brief The callback for the glfw API to handle cursor enter the handle.
			 * @param window The glfw handle.
			 * @param entered The state of the cursor.
			 * @return void
			 */
			static void cursorEnterCallback (GLFWwindow * window, int entered) noexcept;

			/**
			 * @brief The callback for the glfw API to handle cursor button inputs.
			 * @param window The glfw handle.
			 * @param button Which pointer button.
			 * @param action Pressed or released.
			 * @param modifiers The modifier keys mask.
			 * @return void
			 */
			static void mouseButtonCallback (GLFWwindow * window, int button, int action, int modifiers) noexcept;

			/**
			 * @brief The callback for the glfw API to handle scroll state changes.
			 * @param window The glfw handle.
			 * @param xOffset The distance in X.
			 * @param yOffset The distance in Y.
			 * @return void
			 */
			static void scrollCallback (GLFWwindow * window, double xOffset, double yOffset) noexcept;

			/**
			 * @brief The callback for the glfw API to handle a file dropped in the handle.
			 * @param window The glfw handle.
			 * @param count The number of files.
			 * @param paths A C-array for the file paths.
			 * @return void
			 */
			static void dropCallback (GLFWwindow * window, int count, const char * * paths) noexcept;

			/**
			 * @brief The callback for the glfw API to handle joystick inputs.
			 * @param jid The joystick id.
			 * @param event The type of joystick event.
			 * @return void
			 */
			static void joystickCallback (int jid, int event) noexcept;

			static constexpr auto GameControllerDBFile{"gamecontrollerdb.txt"};

			inline static Manager * s_instance{nullptr};

			PrimaryServices & m_primaryServices;
			Window & m_window;
			std::vector< KeyboardListenerInterface * > m_keyboardListeners;
			std::vector< PointerListenerInterface * > m_pointerListeners;
			PointerListenerInterface * m_moveEventsTracking{nullptr};
			KeyboardController m_keyboardController;
			PointerController m_pointerController;
			std::set< int > m_joystickIDs;
			std::set< int > m_gamepadIDs;
			std::array< double, 2 > m_pointerScalingFactors{1.0, 1.0};
			std::array< double, 2 > m_lastPointerCoordinates{0.0, 0.0};
			bool m_showInformation{false};
			bool m_windowLess{false};
			bool m_windowLinked{false};
			bool m_isListeningKeyboard{false};
			bool m_isListeningPointer{false};
			bool m_pointerLocked{false};
			bool m_pointerCoordinatesScalingEnabled{false};
			bool m_copyKeyboardStateEnabled{false};
			bool m_copyPointerStateEnabled{false};
			bool m_copyJoysticksStateEnabled{false};
			bool m_copyGamepadsStateEnabled{false};
	};
}
