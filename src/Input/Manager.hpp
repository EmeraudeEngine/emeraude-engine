/*
 * src/Input/Manager.hpp
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
#include <array>
#include <vector>
#include <set>

/* Third-party libraries */
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

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
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class Manager final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"InputManagerService"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

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
			Manager (PrimaryServices & primaryServices, Window & window) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Manager (const Manager & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Manager (Manager && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			Manager & operator= (const Manager & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Manager & operator= (Manager && copy) noexcept = delete;

			/**
			 * @brief Destructs the resource manager.
			 */
			~Manager () override;

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_flags[ServiceInitialized];
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
			 * @note This is a global switch, but doesn't affect the keyboard controller direct state reading.
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
				return m_flags[IsListeningKeyboard];
			}

			/**
			 * @brief @brief Controls whether the pointer events are sent to listeners.
			 * @note This is a global switch, but doesn't affect the pointer controller direct state reading.
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
				return m_flags[IsListeningPointer];
			}

			/**
			 * @brief Returns whether the pointer is locked on screen (FPS mode).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPointerLocked () const noexcept
			{
				return m_flags[PointerLocked];
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
				m_flags[PointerCoordinatesScalingEnabled] = true;
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
				m_flags[PointerCoordinatesScalingEnabled] = false;
			}

			/**
			 * @brief Returns whether the pointer scaling is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPointerScalingEnabled () const noexcept
			{
				return m_flags[PointerCoordinatesScalingEnabled];
			}

			/**
			 * @brief Enables the copy of keyboard state.
			 * @note Direct query of the keyboard state must be done on the main-thread.
			 * @param state The state.
			 * @return void
			 */
			void
			enableCopyKeyboardState (bool state) noexcept
			{
				m_flags[CopyKeyboardStateEnabled] = state;
			}

			/**
			 * @brief Returns whether the copy of keyboard state is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCopyKeyboardStateEnabled () const noexcept
			{
				return m_flags[CopyKeyboardStateEnabled];
			}

			/**
			 * @brief Enables the copy of pointer state.
			 * @note Direct query of the pointer state must be done on the main-thread.
			 * @param state The state.
			 * @return void
			 */
			void
			enableCopyPointerState (bool state) noexcept
			{
				m_flags[CopyPointerStateEnabled] = state;
			}

			/**
			 * @brief Returns whether the copy of pointer state is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCopyPointerStateEnabled () const noexcept
			{
				return m_flags[CopyPointerStateEnabled];
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
				m_flags[CopyJoysticksStateEnabled] = state;
			}

			/**
			 * @brief Returns whether the copy of joystick state is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCopyJoysticksStateEnabled () const noexcept
			{
				return m_flags[CopyJoysticksStateEnabled];
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
				m_flags[CopyGamepadsStateEnabled] = state;
			}

			/**
			 * @brief Returns whether the copy of gamepad state is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCopyGamepadsStateEnabled () const noexcept
			{
				return m_flags[CopyGamepadsStateEnabled];
			}

			/**
			 * @brief Hides the mouse cursor and the manager will only serve listeners in relative mode.
			 * @note The mouse move event will give the difference on the X and Y axis.
			 * @return void
			 */
			void lockPointer () noexcept;

			/**
			 * @brief Shows the mouse cursor and the manager will only serve listeners in absolute mode.
			 * @note The mouse move event will give the absolute pointer XY coordinates on the screen.
			 * @return void
			 */
			void unlockPointer () noexcept;

			/**
			 * @brief Gets events from the system.
			 * @return void
			 */
			void pollSystemEvents () const noexcept;

			/**
			 * @brief Waits for a system event.
			 * @param until Sets a duration until the cancellation of the waiting.
			 * @return void
			 */
			void waitSystemEvents (double until = 0.0) noexcept;

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

			/**
			 * @brief Returns the instance of the input manager.
			 * @return Manager *
			 */
			[[nodiscard]]
			static
			Manager *
			instance () noexcept
			{
				return s_instance;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Main method to attach all events callback from a glfw handle.
			 * @param enableKeyboard Enables listeners relative to keyboard.
			 * @param enablePointer Enables listeners relative to pointer.
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
			 * @param key The keyboard universal key code. I.e, QWERTY keyboard 'A' key gives the ASCII code '65' on all platform.
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
			 * @brief Separate method for relative mode pointer called by the main Manager::cursorPositionCallback() method.
			 * @param xPosition The X position of the cursor.
			 * @param yPosition The Y position of the cursor.
			 * @return void
			 */
			static void dispatchRelativePointerPosition (double xPosition, double yPosition) noexcept;

			/**
			 * @brief Separate method for absolute mode pointer called by the main Manager::cursorPositionCallback() method.
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
			 * @brief The callback for the glfw API to handle file dropped in the handle.
			 * @param window The glfw handle.
			 * @param count The number of files.
			 * @param paths An C-array for the file paths.
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

			/* Flag names. */
			static constexpr auto ServiceInitialized{0UL};
			static constexpr auto ShowInformation{1UL};
			static constexpr auto WindowLinked{2UL};
			static constexpr auto IsListeningKeyboard{3UL};
			static constexpr auto IsListeningPointer{4UL};
			static constexpr auto PointerLocked{5UL};
			static constexpr auto PointerCoordinatesScalingEnabled{6UL};
			static constexpr auto CopyKeyboardStateEnabled{7UL};
			static constexpr auto CopyPointerStateEnabled{8UL};
			static constexpr auto CopyJoysticksStateEnabled{9UL};
			static constexpr auto CopyGamepadsStateEnabled{10UL};

			static constexpr auto GameControllerDBFile{"gamecontrollerdb.txt"};

			static Manager * s_instance;

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
			std::array< bool, 16 > m_flags{
				false/*ServiceInitialized*/,
				false/*ShowInformation*/,
				false/*WindowLinked*/,
				false/*IsListeningKeyboard*/,
				false/*IsListeningPointer*/,
				false/*PointerLocked*/,
				false/*PointerCoordinatesScalingEnabled*/,
				false/*CopyKeyboardStateEnabled*/,
				false/*CopyPointerStateEnabled*/,
				false/*CopyJoysticksStateEnabled*/,
				false/*CopyGamepadsStateEnabled*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
