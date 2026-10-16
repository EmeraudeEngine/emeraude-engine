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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* Project configuration. */
#include "emeraude_platform.hpp"

/* STL inclusions. */
#include <cstdint>
#include <cstddef>
#include <array>
#include <set>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "ObservableTrait.hpp"
#include "Console/ControllableTrait.hpp"

/* Local inclusions for usages. */
#include "KeyboardController.hpp"
#include "KeyboardListenerInterface.hpp"
#include "PointerController.hpp"
#include "PointerListenerInterface.hpp"
#include "ScopedRegistration.hpp"

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
	 * @extends EmEn::Base::ObservableTrait This service is observable.
	 */
	class EMEN_API Manager final : public ServiceInterface, public Base::ObservableTrait, public Console::ControllableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"InputManagerService"};

			/** @brief Observable notification codes. */
			enum NotificationCode : std::uint8_t
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
				ControllableTrait{ClassId},
				m_primaryServices{primaryServices},
				m_window{window}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @note A service cannot be duplicated.
			 * @param copy A reference to the copied instance.
			 */
			Manager (const Manager & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @note A service cannot be duplicated.
			 * @param copy A reference to the copied instance.
			 */
			Manager (Manager && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @note A service cannot be duplicated.
			 * @param copy A reference to the copied instance.
			 * @return Manager &
			 */
			Manager & operator= (const Manager & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @note A service cannot be duplicated.
			 * @param copy A reference to the copied instance.
			 * @return Manager &
			 */
			Manager & operator= (Manager && copy) noexcept = delete;

			/**
			 * @brief Destructs the resource manager.
			 */
			~Manager () override
			{
#if IS_MACOS
				/* NOTE: The gesture monitor's block captures this manager (onTerminate() removes it first). */
				this->removeMacOSGestureHandlers();
#endif
			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
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
			 * @brief Controls whether the pointer events are sent to listeners.
			 * @note This is a global switch but doesn't affect the pointer controller direct state reading.
			 * @param state The state.
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
			 */
			void lockPointer () noexcept;

			/**
			 * @brief Shows the mouse cursor, and the manager will only serve listeners in absolute mode.
			 * @note The mouse move event will give the absolute pointer XY coordinates on the screen.
			 */
			void unlockPointer () noexcept;

			/**
			 * @brief Waits for a system event.
			 * @param until Sets a duration until the cancellation of the waiting in seconds.
			 */
			void waitSystemEvents (double until = 0.0) const noexcept;

			/**
			 * @brief Wakes up the thread blocked in waitSystemEvents() by posting an empty event.
			 * @note Thread-safe, this method can be called from any thread (glfwPostEmptyEvent() contract).
			 */
			static void wakeUpEventsLoop () noexcept;

			/**
			 * @brief Adds an object the keyboard can control, like a player.
			 * @param listener A pointer to a keyboard listener interfaced object.
			 * @return Base::ScopedRegistration The registration: the listener is served until this token ends (Ave Robustus
			 * II P4). Empty when the listener is null or already registered (traced).
			 * @pre The input manager outlives the token (Core declares it before every service and scene that listens).
			 * @note Main thread. Added or removed DURING a dispatch, a listener takes effect after it: a removed one is
			 * never called again, not even later in the same dispatch.
			 */
			[[nodiscard]]
			Base::ScopedRegistration addKeyboardListener (KeyboardListenerInterface * listener) noexcept;

			/**
			 * @brief Adds an object the pointer can control, like a player.
			 *
			 * @note Listeners are dispatched NEWEST FIRST, so a scene registering its controller
			 * outranks the one before it. That order is deliberate and unchanged.
			 *
			 * @warning ⚠️⚠️ A UI overlay must pass @a priority. Without it, ANY scene registered
			 * afterwards is served before the interface drawn ON TOP of it, and swallows its
			 * clicks — which is exactly what happened to the application menu: the model viewer
			 * gives its orbit controller a node to drive, `OrbitController::onButtonPress()` then
			 * answers `true` instead of declining, and the menu went dead the moment a viewer
			 * existed while still being perfectly drawn. A demo scene hid the defect, its
			 * controller having no node and therefore declining. Input has to follow the VISUAL
			 * stacking, and there is at most one priority listener: the last one to claim it wins.
			 *
			 * @param listener A pointer to a pointer listener interfaced object.
			 * @param priority Serve this listener BEFORE every other one. Default false.
			 * @return Base::ScopedRegistration The registration: the listener is served until this token ends (Ave Robustus
			 * II P4). Empty when the listener is null or already registered (traced).
			 * @pre The input manager outlives the token.
			 * @note Main thread; the same dispatch rule as addKeyboardListener().
			 */
			[[nodiscard]]
			Base::ScopedRegistration addPointerListener (PointerListenerInterface * listener, bool priority = false) noexcept;

#if IS_MACOS
			/**
			 * @brief Installs macOS-specific gesture handlers (pinch-to-zoom, etc.).
			 * @param window The GLFW window to monitor
			 */
			void installMacOSGestureHandlers (GLFWwindow * window) noexcept;

			/**
			 * @brief Removes macOS gesture handlers.
			 */
			void removeMacOSGestureHandlers () noexcept;
#endif

			/**
			 * @brief Injects a synthetic keyboard event, bypassing GLFW.
			 * @param key The key code (see Input::Key enum).
			 * @param modifiers The modifier keys mask (see Input::ModKey enum).
			 * @param action GLFW action: 1 = press, 0 = release, 2 = repeat.
			 */
			void injectKeyEvent (int32_t key, int32_t modifiers, int32_t action = 1) noexcept;

			/**
			 * @brief Injects a synthetic mouse click event at the given screen coordinates.
			 * @param positionX The X coordinate in screen pixels.
			 * @param positionY The Y coordinate in screen pixels.
			 * @param button The mouse button number (0 = left, 1 = right, 2 = middle).
			 * @param modifiers The modifier keys mask.
			 * @param action GLFW action: 1 = press, 0 = release.
			 */
			void injectMouseClickEvent (float positionX, float positionY, int32_t button = 0, int32_t modifiers = 0, int32_t action = 1) noexcept;

			/**
			 * @brief Injects a synthetic pointer move event.
			 * @param positionX The X coordinate in screen pixels (absolute mode).
			 * @param positionY The Y coordinate in screen pixels (absolute mode).
			 */
			void injectPointerMoveEvent (float positionX, float positionY) noexcept;

		private:

			/**
			 * @brief Marks a listener dispatch in progress: additions wait and removals leave a null slot until the
			 * outermost dispatch ends, which applies them (Ave Robustus II P4, owner decision 2026-10-16: a listener list is
			 * never resized while it is walked).
			 */
			class DispatchScope final
			{
				public:

					/**
					 * @brief Opens a dispatch.
					 * @param manager The input manager.
					 */
					explicit
					DispatchScope (Manager & manager) noexcept
						: m_manager{manager}
					{
						++m_manager.m_dispatchDepth;
					}

					/** @brief Closes the dispatch; the outermost one applies the changes requested meanwhile. */
					~DispatchScope ()
					{
						--m_manager.m_dispatchDepth;

						if ( m_manager.m_dispatchDepth == 0 )
						{
							m_manager.applyPendingListenerChanges();
						}
					}

					/** @brief Copy constructor (deleted). */
					DispatchScope (const DispatchScope & copy) = delete;

					/** @brief Move constructor (deleted). */
					DispatchScope (DispatchScope && copy) = delete;

					/** @brief Copy assignment (deleted). */
					DispatchScope & operator= (const DispatchScope & copy) = delete;

					/** @brief Move assignment (deleted). */
					DispatchScope & operator= (DispatchScope && copy) = delete;

				private:

					Manager & m_manager;
			};

			/**
			 * @brief Serves an event to every listener of a list, newest first, until one processes it without
			 * propagating it.
			 * @tparam listener_t The listener interface.
			 * @tparam handler_t A callable taking a listener_t & and returning whether the event was processed.
			 * @param listeners The list (m_keyboardListeners or m_pointerListeners).
			 * @param handler The event for one listener.
			 * @note The list keeps its size during the walk (DispatchScope), so an index stays valid. A listener removed by
			 * its own handler cannot be asked about propagation any more: an event it processed stops there.
			 */
			template< typename listener_t, typename handler_t >
			void
			dispatch (std::vector< listener_t * > & listeners, const handler_t & handler) noexcept
			{
				const DispatchScope scope{*this};

				for ( size_t index = 0; index < listeners.size(); ++index )
				{
					auto * listener = listeners[index];

					if ( listener == nullptr )
					{
						continue;
					}

					if ( !handler(*listener) )
					{
						continue;
					}

					if ( listeners[index] == nullptr || !listener->isPropagatingProcessedEvents() )
					{
						break;
					}
				}
			}

			/**
			 * @brief Removes a keyboard listener (the undo of addKeyboardListener()'s token).
			 * @param listener The listener.
			 */
			void removeKeyboardListener (KeyboardListenerInterface * listener) noexcept;

			/**
			 * @brief Removes a pointer listener (the undo of addPointerListener()'s token); it also loses the priority and
			 * the move-events tracking it held.
			 * @param listener The listener.
			 */
			void removePointerListener (PointerListenerInterface * listener) noexcept;

			/**
			 * @brief Inserts a pointer listener now, keeping the priority invariant (outside a dispatch only).
			 * @param listener The listener.
			 * @param priority Whether it takes the priority slot.
			 */
			void insertPointerListener (PointerListenerInterface * listener, bool priority) noexcept;

			/**
			 * @brief Applies the additions and removals requested during the dispatch that just ended.
			 */
			void applyPendingListenerChanges () noexcept;

			/**
			 * @brief Returns the input manager attached to a GLFW window, through the window's callback context.
			 * @param handle The GLFW window handle given to a callback.
			 * @return Manager * nullptr when none is attached.
			 */
			[[nodiscard]]
			static Manager * fromHandle (GLFWwindow * handle) noexcept;

			/**
			 * @brief Serves a key event to the keyboard listeners.
			 * @param key The key code.
			 * @param scancode The OS scancode (0 for an injected event).
			 * @param action GLFW_PRESS, GLFW_REPEAT or GLFW_RELEASE.
			 * @param modifiers The modifier keys mask.
			 */
			void dispatchKeyEvent (int32_t key, int32_t scancode, int32_t action, int32_t modifiers) noexcept;

			/**
			 * @brief Serves a typed character to the keyboard listeners in text mode.
			 * @param codepoint The Unicode value of the character.
			 */
			void dispatchCharacter (uint32_t codepoint) noexcept;

			/**
			 * @brief Serves a wheel event to the tracking listener, or to the pointer listeners.
			 * @param position The pointer position.
			 * @param xOffset The distance in X.
			 * @param yOffset The distance in Y.
			 * @param modifiers The modifier keys mask.
			 */
			void dispatchMouseWheel (const std::array< float, 2 > & position, float xOffset, float yOffset, int32_t modifiers) noexcept;

			/**
			 * @brief Handles a cursor move (the cursor position callback).
			 * @param xPosition The X position of the cursor.
			 * @param yPosition The Y position of the cursor.
			 */
			void processCursorPosition (double xPosition, double yPosition) noexcept;

			/**
			 * @brief Handles the cursor entering or leaving the window.
			 * @param window The GLFW window.
			 * @param entered GLFW_TRUE when entering.
			 */
			void processCursorEnter (GLFWwindow * window, int entered) noexcept;

			/**
			 * @brief Handles a pointer button event.
			 * @param window The GLFW window.
			 * @param button Which pointer button.
			 * @param action Pressed or released.
			 * @param modifiers The modifier keys mask.
			 */
			void processMouseButton (GLFWwindow * window, int button, int action, int modifiers) noexcept;

			/**
			 * @brief Handles a joystick or gamepad connection change.
			 * @param jid The joystick id.
			 * @param event GLFW_CONNECTED or GLFW_DISCONNECTED.
			 */
			void processJoystickEvent (int jid, int event) noexcept;

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Console::ControllableTrait::onRegisterToConsole() */
			void onRegisterToConsole () noexcept override;

			/**
			 * @brief Main method to attach all events callbacks from a glfw handle.
			 * @param enableKeyboard Enables listeners relative to the keyboard.
			 * @param enablePointer Enables listeners relative to the pointer.
			 */
			void linkWindowCallbacks (bool enableKeyboard, bool enablePointer) noexcept;

			/**
			 * @brief Removes all callback functions set to the window.
			 */
			void unlinkWindowCallbacks () noexcept;

			/**
			 * @brief Returns the pointer location to complete some pointer events data.
			 * @param window A pointer to the window.
			 * @return std::array< float, 2 >
			 */
			[[nodiscard]]
			std::array< float, 2 > pointerLocation (GLFWwindow * window) const noexcept;

			/**
			 * @brief The callback for the glfw API to handle key inputs.
			 * @param window The glfw handle.
			 * @param key The keyboard universal key code. I.e., QWERTY keyboard 'A' key gives the ASCII code '65' on all platforms.
			 * @param scancode The OS dependent scancode.
			 * @param action The key event.
			 * @param modifiers The modifier keys mask.
			 */
			static void keyCallback (GLFWwindow * window, int key, int scancode, int action, int modifiers) noexcept;

			/**
			 * @brief The callback for the glfw API to handle character inputs.
			 * @param window The glfw handle.
			 * @param codepoint The Unicode value of the character.
			 */
			static void charCallback (GLFWwindow * window, unsigned int codepoint) noexcept;

			/**
			 * @brief The callback for the glfw API to handle character modification inputs.
			 * @param window The glfw window handle.
			 * @param codepoint The Unicode value of the character.
			 * @param modifiers The modifier keys mask.
			 */
			static void charModsCallback (GLFWwindow * window, unsigned int codepoint, int modifiers) noexcept;

			/**
			 * @brief Separate method for a relative mode pointer called by the main Manager::cursorPositionCallback() method.
			 * @param xPosition The X position of the cursor.
			 * @param yPosition The Y position of the cursor.
			 */
			void dispatchRelativePointerPosition (double xPosition, double yPosition) noexcept;

			/**
			 * @brief Separate method for an absolute mode pointer called by the main Manager::cursorPositionCallback() method.
			 * @param xPosition The X position of the cursor.
			 * @param yPosition The Y position of the cursor.
			 */
			void dispatchAbsolutePointerPosition (double xPosition, double yPosition) noexcept;

			/**
			 * @brief The callback for the glfw API to handle cursor position changes.
			 * @param window The glfw handle.
			 * @param xPosition The X position of the cursor.
			 * @param yPosition The Y position of the cursor.
			 */
			static void cursorPositionCallback (GLFWwindow * window, double xPosition, double yPosition) noexcept;

			/**
			 * @brief The callback for the glfw API to handle cursor enter the handle.
			 * @param window The glfw handle.
			 * @param entered The state of the cursor.
			 */
			static void cursorEnterCallback (GLFWwindow * window, int entered) noexcept;

			/**
			 * @brief The callback for the glfw API to handle cursor button inputs.
			 * @param window The glfw handle.
			 * @param button Which pointer button.
			 * @param action Pressed or released.
			 * @param modifiers The modifier keys mask.
			 */
			static void mouseButtonCallback (GLFWwindow * window, int button, int action, int modifiers) noexcept;

			/**
			 * @brief The callback for the glfw API to handle scroll state changes.
			 * @param window The glfw handle.
			 * @param xOffset The distance in X.
			 * @param yOffset The distance in Y.
			 */
			static void scrollCallback (GLFWwindow * window, double xOffset, double yOffset) noexcept;

			/**
			 * @brief The callback for the glfw API to handle a file dropped in the handle.
			 * @param window The glfw handle.
			 * @param count The number of files.
			 * @param paths A C-array for the file paths.
			 */
			static void dropCallback (GLFWwindow * window, int count, const char * * paths) noexcept;

			/**
			 * @brief The callback for the glfw API to handle joystick inputs.
			 * @param jid The joystick id.
			 * @param event The type of joystick event.
			 */
			static void joystickCallback (int jid, int event) noexcept;

			static constexpr auto GameControllerDBFile{"gamecontrollerdb.txt"};

			/** @brief The manager served by the GLFW joystick callback, which carries no window: set and cleared only by
			 * m_joystickCallbackRegistration (one manager at a time), read on the main thread, null-checked. */
			inline static Manager * s_joystickCallbackReceiver{nullptr};

			PrimaryServices & m_primaryServices;
			Window & m_window;
			/* NOTE: Main thread only (registrations, GLFW callbacks, console injections); a removed listener leaves a null
			 * slot while a dispatch walks the list (DispatchScope). */
			std::vector< KeyboardListenerInterface * > m_keyboardListeners;
			std::vector< PointerListenerInterface * > m_pointerListeners;
			std::vector< KeyboardListenerInterface * > m_pendingKeyboardListeners;
			std::vector< std::pair< PointerListenerInterface *, bool > > m_pendingPointerListeners;
			/** @brief The listener served first, kept at index 0 of the vector above — the whole
			 * invariant, which is why the dispatch loops need no special case. nullptr when none. */
			PointerListenerInterface * m_priorityPointerListener{nullptr};
			PointerListenerInterface * m_moveEventsTracking{nullptr};
			KeyboardController m_keyboardController;
			PointerController m_pointerController;
			std::set< int > m_joystickIDs;
			std::set< int > m_gamepadIDs;
			std::array< double, 2 > m_pointerScalingFactors{1.0, 1.0};
			std::array< double, 2 > m_lastPointerCoordinates{0.0, 0.0};
#if IS_MACOS
			/* The pinch-gesture NSEvent monitor (an Objective-C object retained through __bridge_retained), one per manager:
			 * installMacOSGestureHandlers() / removeMacOSGestureHandlers(), the destructor. Main thread only. */
			void * m_magnificationEventMonitor{nullptr};
#endif
			/* The window callback context attachment and the joystick callback: their end detaches this manager. */
			Base::ScopedRegistration m_windowAttachment;
			Base::ScopedRegistration m_joystickCallbackRegistration;
			uint32_t m_dispatchDepth{0};
			bool m_listenerSlotsReleased{false};
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
