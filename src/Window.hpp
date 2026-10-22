/*
 * src/Window.hpp
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
#include "emeraude_platform.hpp"
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Console/ControllableTrait.hpp"
#include "ObservableTrait.hpp"

/* Local inclusions for usages. */
#include "Identification.hpp"
#include "ScopedRegistration.hpp"
#include "StaticVector.hpp"
#include "Vulkan/Surface.hpp"
#if IS_MACOS || IS_WINDOWS
	#include "PlatformSpecific/Helpers.hpp"
#endif

namespace EmEn
{
	namespace Vulkan
	{
		class Instance;
	}

	namespace Input
	{
		class Manager;
	}

	class PrimaryServices;
}

#if IS_MACOS

using id = struct objc_object *;

namespace EmEn
{
	/* The handler of the macOS "quit" Apple Event (defined in Window.mac.mm, Objective-C under ARC). */
	class QuitAppleEventHandler;

	/** @brief Destroys a QuitAppleEventHandler where its type is complete (Window.mac.mm). */
	struct QuitAppleEventHandlerDeleter
	{
		void operator() (QuitAppleEventHandler * handler) const noexcept;
	};
}

#elif IS_WINDOWS
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif

	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <Windows.h>
#endif

namespace EmEn
{
	/**
	 * @brief The handle service class is responsible for the physical screen.
	 * @note [OBS][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Base::ObservableTrait This service is observable.
	 */
	class EMEN_LEAN_API Window final : public ServiceInterface, public Console::ControllableTrait, public Base::ObservableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"WindowService"};

			/**
			 * @brief Describe a connected monitor/display device.
			 */
			struct MonitorDevice
			{
				/** The monitor name as reported by the system. */
				std::string name{"Unknown"};
				/** Whether this is the primary monitor. */
				bool primary{false};
				/** Physical size of the display in millimeters. */
				int32_t physicalWidthMM{0};
				int32_t physicalHeightMM{0};
				/** Current resolution in pixels. */
				int32_t currentResolutionX{0};
				int32_t currentResolutionY{0};
				/** Position on the virtual desktop in screen coordinates. */
				int32_t positionX{0};
				int32_t positionY{0};
				/** Current refresh rate in Hz. */
				int32_t refreshRate{0};
				/** Color depth (sum of red, green and blue bits). */
				int32_t colorDepth{0};
				/** Content scale factors (for HiDPI/Retina displays). */
				float contentScaleX{1.0F};
				float contentScaleY{1.0F};
			};

			/**
			 * @brief Structure to keep the state of the window geometry.
			 */
			struct State
			{
				/* The position (top left) of the window in the desktop screen. */
				int32_t windowXPosition{0};
				int32_t windowYPosition{0};
				/* The window dimension expressed in the OS/desktop screen. */
				uint32_t windowWidth{0};
				uint32_t windowHeight{0};
				/* The window borders sizes (Depend on the OS/desktop). */
				int32_t borderLeftSize{0};
				int32_t borderTopSize{0};
				int32_t borderRightSize{0};
				int32_t borderBottomSize{0};
				/* The content scale factor from the desktop (HDPI screen). */
				float contentXScale{1.0F};
				float contentYScale{1.0F};
				/* The framebuffer of the window expressed in pixels. */
				uint32_t framebufferWidth{0};
				uint32_t framebufferHeight{0};
			};

			/** @brief Observable notification codes. */
			enum NotificationCode : std::uint8_t
			{
				Created,
				Destroyed,
				Centered,
				TitleChanged,
				OSNotifiesWindowGetFocus,
				OSNotifiesWindowLostFocus,
				OSNotifiesWindowMovedTo,
				OSNotifiesWindowVisible,
				OSNotifiesWindowHidden,
				OSNotifiesWindowSizeMaximized,
				OSNotifiesWindowSizeMinimized,
				OSNotifiesWindowResized,
				OSNotifiesFramebufferResized, /* This tells the graphics framebuffer should be resized. */
				OSRequestsToRescaleContentBy, /* This tells the graphics framebuffer should be resized according to the new scale. */
				OSRequestsToRefreshContent,
				OSRequestsToTerminate,
				OSMonitorConfigurationChanged,
				OSRequestsToEndSession, /* The system asks the application to quit: Windows WM_QUERYENDSESSION (shutdown, logoff), the macOS "quit" Apple Event of the session end (log out, restart, shut down; a Dock "Quit" or an AppleScript quit is OSRequestsToTerminate). */
				OSEndsSession, /* Windows WM_ENDSESSION: the session ends; the process may be killed once the observers return (synchronous). */
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constant for automatic size.
			 */
			static constexpr auto Automatic{0};

			/**
			 * @brief Constructs a Window.
			 * @param primaryServices A reference to primary services.
			 * @param instance A reference to the Vulkan instance.
			 * @param identification A reference to an application identification.
			 */
			Window (PrimaryServices & primaryServices, const Vulkan::Instance & instance, const Identification & identification) noexcept
				: ServiceInterface{ClassId},
				ControllableTrait{ClassId},
				m_primaryServices{primaryServices},
				m_instance{instance},
				m_title{identification.applicationId()}
			{

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
			 * @brief Returns the primary services bundle.
			 * @return PrimaryServices &
			 */
			[[nodiscard]]
			PrimaryServices &
			primaryServices () const noexcept
			{
				return m_primaryServices;
			}

			/**
			 * @brief Returns whether the application started in window less mode.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isWindowLessMode () const noexcept
			{
				return m_windowLess;
			}

			/**
			 * @brief Returns whether the window is currently being resized by the user.
			 * @note On Windows, this is true during a drag resize operation (between WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE).
			 * On other platforms, this always returns false.
			 * @todo Generalize this method. For now disabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUserResizing () const noexcept
			{
				return m_isUserResizing;
			}

			/**
			 * @brief Returns the handle of the GLFW window object.
			 * @return GLFWwindow *
			 */
			[[nodiscard]]
			GLFWwindow *
			handle () const noexcept
			{
				return m_handle.get();
			}

			/**
			 * @brief Returns the handle of the GLFW window object.
			 * @return GLFWwindow *
			 */
			[[nodiscard]]
			GLFWwindow *
			handle () noexcept
			{
				return m_handle.get();
			}

			/**
			 * @brief What a GLFW callback of this window reaches through the window user pointer: GLFW gives each window
			 * ONE user-pointer slot, owned by the Window (Ave Robustus II P4, owner decision 2026-10-16: no static instance
			 * for the window callbacks).
			 */
			struct CallbackContext final
			{
				/** @brief The window (set at construction, never null). */
				Window * window{nullptr};
				/** @brief The input manager while it is attached (attachInputManager()), else nullptr. */
				Input::Manager * inputManager{nullptr};
			};

			/**
			 * @brief Returns the callback context of a GLFW window created by a Window.
			 * @param handle A GLFW window handle (may be nullptr).
			 * @return CallbackContext * nullptr when the handle is null or carries no context.
			 * @note Main thread (the GLFW callbacks run on the thread that pumps the events).
			 */
			[[nodiscard]]
			static
			CallbackContext *
			callbackContext (GLFWwindow * handle) noexcept
			{
				if ( handle == nullptr )
				{
					return nullptr;
				}

				return static_cast< CallbackContext * >(glfwGetWindowUserPointer(handle));
			}

			/**
			 * @brief Attaches the input manager to the window's callback context: its GLFW input callbacks reach it there.
			 * @param inputManager A reference to the input manager.
			 * @return Base::ScopedRegistration The attachment: its end detaches the manager. Empty when another manager is
			 * already attached (traced).
			 * @pre The window outlives the returned token (Core declares the window before the input manager).
			 * @note Main thread.
			 */
			[[nodiscard]]
			Base::ScopedRegistration attachInputManager (Input::Manager & inputManager) noexcept;

#if IS_MACOS
			/**
			 * @brief Returns the application window as a Cocoa window id.
			 * @return id
			 */
			[[nodiscard]]
			id getCocoaWindow () const noexcept;

			/**
			 * @brief Returns the application window as a Cocoa view id.
			 * @return id
			 */
			[[nodiscard]]
			id getCocoaView () const noexcept;
#elif IS_WINDOWS
			/**
			 * @brief Returns the application window as a win32 window pointer.
			 * @return HWND
			 */
			[[nodiscard]]
			HWND getWin32Window () const noexcept;
#endif

			/**
			 * @brief Returns the vulkan handle of the surface.
			 * @return const Vulkan::Surface *
			 */
			[[nodiscard]]
			const Vulkan::Surface *
			surface () const noexcept
			{
				return m_surface.get();
			}

			/**
			 * @brief Returns the vulkan handle of the surface.
			 * @return Vulkan::Surface *
			 */
			[[nodiscard]]
			Vulkan::Surface *
			surface () noexcept
			{
				return m_surface.get();
			}

			/**
			 * @brief Changes the window title.
			 * @param title A reference to a string.
			 */
			void setTitle (const std::string & title) noexcept;

			/**
			 * @brief Resizes programmatically the window size.
			 * @note This function takes care of the fullscreen/window mode.
			 * @param width The new width.
			 * @param height The new height.
			 * @return bool
			 */
			bool resize (int32_t width, int32_t height) noexcept;

			/**
			 * @brief Sets the window position on the desktop.
			 * @param xPosition The X position in screen coordinates.
			 * @param yPosition The Y position in screen coordinates.
			 * @return bool
			 */
			bool setPosition (int32_t xPosition, int32_t yPosition) noexcept;

			/**
			 * @brief Center the windows on a specific monitor.
			 * @param desiredMonitor The number of the monitor for multiple monitors configuration. Default initial monitor.
			 */
			void centerPosition (int32_t desiredMonitor = -1) noexcept;

			/**
			 * @brief Sets a gamma value to a specific monitor.
			 * @param value A gamma value.
			 * @param desiredMonitor The number of the monitor for multiple monitors configuration. Default initial monitor.
			 */
			void setGamma (float value, int32_t desiredMonitor = -1) const noexcept;

			/**
			 * @brief Sets a gamma ramp to a specific monitor.
			 * @param generateRampFn A reference to a function to generate de gamma ramp.
			 * @param desiredMonitor The number of the monitor for multiple monitors configuration. Default initial monitor.
			 * @return bool
			 */
			template< size_t color_count_t >
			bool
			setCustomGamma (const std::function< void (std::array< uint16_t, color_count_t > &, std::array< uint16_t, color_count_t > &, std::array< uint16_t, color_count_t > &) > & generateRampFn, int32_t desiredMonitor = -1) const noexcept
			{
				auto * monitor = Window::getMonitor(desiredMonitor);

				if ( monitor == nullptr )
				{
					return false;
				}

				std::array< uint16_t, color_count_t > red{};
				std::array< uint16_t, color_count_t > green{};
				std::array< uint16_t, color_count_t > blue{};

				generateRampFn(red, green, blue);

				/* Creates a gamma ramp to pass to monitor. */
				GLFWgammaramp ramp;
				ramp.red = red.data();
				ramp.green = green.data();
				ramp.blue = blue.data();
				ramp.size = color_count_t;

				glfwSetGammaRamp(monitor, &ramp);

				return true;
			}

			/**
			 * @brief Switch the application to fullscreen mode.
			 * @param useNativeResolution Overrides the settings and fetch the monitor resolution. Default false.
			 * @param desiredMonitor The number of the monitor for multiple monitors configuration. Default initial monitor.
			 */
			void switchToFullscreenMode (bool useNativeResolution = false, int32_t desiredMonitor = -1) const noexcept;

			/**
			 * @brief Switch the application to windowed mode.
			 */
			void switchToWindowedMode () const noexcept;

			/**
			 * @brief Returns the centered position for the window.
			 * @param desiredMonitor The number of the monitor for multiple monitors configuration. Default initial monitor.
			 * @return std::array< int32_t, 2 >
			 */
			[[nodiscard]]
			std::array< int32_t, 2 >
			getCenteredPosition (int32_t desiredMonitor = -1) const noexcept
			{
				return this->getCenteredPosition(this->getSize(), desiredMonitor);
			}

			/**
			 * @brief Returns whether the window is visible on the user view.
			 * @return bool
			 */
			[[nodiscard]]
			bool isVisibleOnDesktop () const noexcept;

			/**
			 * @brief Returns whether the window is hidden from the user view.
			 * @return bool
			 */
			[[nodiscard]]
			bool isIconified () const noexcept;

			/**
			 * @brief Reduces the window to the OS toolbar or similar.
			 */
			void iconify () const noexcept;

			/**
			 * @brief Restores the window on the user desktop view, from the taskbar (the reverse of iconify()).
			 * @note Does nothing when the window is not minimised (tested with GLFW_ICONIFIED: on Windows a minimised
			 * window is still GLFW_VISIBLE).
			 */
			void getBackOnDesktop () const noexcept;

			/**
			 * @brief Returns whether the window covers the whole desktop.
			 * @return bool
			 */
			[[nodiscard]]
			bool isSizeMaximized () const noexcept;

			/**
			 * @brief Returns whether the window is a floating window.
			 * @return bool
			 */
			[[nodiscard]]
			bool isSizeReduced () const noexcept;

			/**
			 * @brief Maximizes the window size on the user desktop.
			 */
			void maximizeSize () const noexcept;

			/**
			 * @brief Restores the window to previous size.
			 */
			void restoreSize () const noexcept;

			/**
			 * @brief Returns whether the window is focused by the system host.
			 * @return bool
			 */
			[[nodiscard]]
			bool isFocused () const noexcept;

			/**
			 * @brief Returns whether the window is blurred by the system host.
			 * @return bool
			 */
			[[nodiscard]]
			bool isBlurred () const noexcept;

			/**
			 * @brief Asks the host system to focus on the window.
			 */
			void focus () const noexcept;

			/**
			 * @brief Returns whether the window is visible.
			 * @return bool
			 */
			[[nodiscard]]
			bool isVisible () const noexcept;

			/**
			 * @brief Returns whether the window is hidde.
			 * @return bool
			 */
			[[nodiscard]]
			bool isHidden () const noexcept;

			/**
			 * @brief hides the window.
			 */
			void hide () const noexcept;

			/**
			 * @brief Shows the window.
			 */
			void show () const noexcept;

			/**
			 * @brief Reads what the display server sent, when the thread that normally does is busy [RENDERING THREAD].
			 * @note Wayland only (a no-op elsewhere, and under X11): never blocks, never dispatches anything — the
			 * events join their queues in memory, GLFW's on the main thread's next poll, the driver's own at its next
			 * present. ⚠️⚠️ Without it, a main thread busy for seconds (a scene loaded before the main loop, 9 s for
			 * `terrain`) while the swap-chain presents in MAILBOX (13 000 frames per second on a loading screen) let
			 * the compositor's send buffer fill with one `wl_buffer.release` per present: GNOME dropped the client at
			 * 1 MiB (`WL: Data too big for buffer`, `error in client communication`), GLFW turned the dead connection
			 * into a silent close request and the swap-chain never got an image back (2026-09-25).
			 */
			void drainDisplayConnection () const noexcept;

			/**
			 * @brief Returns whether the application is currently in fullscreen mode.
			 * @return bool
			 */
			[[nodiscard]]
			bool isFullscreenMode () const noexcept;

			/**
			 * @brief Returns whether the application is currently in windowed mode.
			 * @return bool
			 */
			[[nodiscard]]
			bool isWindowedMode () const noexcept
			{
				return !this->isFullscreenMode();
			}

			/**
			 * @brief Returns the position, in screen coordinates of the window content area upper-left corner.
			 * @warning Do not use this function to respond to window events. Data are updated after the event in GLFW.  Use Window::state() instead.
			 * @return std::array< int32_t, 2 >
			 */
			[[nodiscard]]
			std::array< int32_t, 2 > getPosition () const noexcept;

			/**
			 * @brief Returns the size, in screen coordinates of the window content area.
			 * @warning Do not use this function to respond to window events. Data are updated after the event in GLFW.  Use Window::state() instead.
			 * @return std::array< uint32_t, 2 >
			 */
			[[nodiscard]]
			std::array< uint32_t, 2 > getSize () const noexcept;

			/**
			 * @brief Returns the size, in screen coordinates, of each window frame edge.
			 * @warning Do not use this function to respond to window events. Data are updated after the event in GLFW. Use Window::state() instead.
			 * @note The array will hold left, top, right and bottom.
			 * @return std::array< uint32_t, 4 >
			 */
			[[nodiscard]]
			std::array< uint32_t, 4 > getBorderSize () const noexcept;

			/**
			 * @brief Returns the size, in pixels, of the window framebuffer.
			 * @warning Do not use this function to respond to window events. Data are updated after the event in GLFW. Use Window::state() instead.
			 * @param applyScale Divide the real framebuffer size by scaling factors. Default false.
			 * @return std::array< uint32_t, 2 >
			 */
			[[nodiscard]]
			std::array< uint32_t, 2 > getFramebufferSize (bool applyScale = false) const noexcept;

			/**
			 * @brief Returns the content scale for the window.
			 * @warning Do not use this function to respond to window events. Data are updated after the event in GLFW.  Use Window::state() instead.
			 * @return std::array< float, 2 >
			 */
			[[nodiscard]]
			std::array< float, 2 > getContentScale () const noexcept;

			/**
			 * @brief Returns the actual state of the window.
			 * @return const State &
			 */
			[[nodiscard]]
			const State &
			state () const noexcept
			{
				return m_state;
			}

			/**
			 * @brief Returns the aspect ratio of the window.
			 * @return float
			 */
			[[nodiscard]]
			float aspectRatio () const noexcept;

			/**
			 * @brief Returns the framebuffer size last published by the main thread, in pixels.
			 * @note The render thread's way to read the size (owner ruling 2026-10-01): GLFW queries are main-thread
			 * only (an AppKit query on Cocoa), and a windowless resize wrote m_state while the render thread read it (a
			 * torn pair). The pair is one atomic 64-bit value.
			 * @return std::array< uint32_t, 2 >
			 */
			[[nodiscard]]
			std::array< uint32_t, 2 >
			publishedFramebufferSize () const noexcept
			{
				const auto packed = m_publishedFramebufferSize.load(std::memory_order_acquire);

				return {static_cast< uint32_t >(packed >> 32), static_cast< uint32_t >(packed & 0xFFFFFFFFU)};
			}

			/**
			 * @brief Drains the pending OS window events once, delivering callbacks.
			 *
			 * @note This exists for the ONE case the main loop cannot cover: a long operation that
			 * legitimately owns the main thread — loading a large asset, above all. While it runs,
			 * nothing drains the queue, and a Wayland compositor CLOSES an application that stops
			 * answering its pings. The window dies mid-load and the stage is torn down the instant
			 * the operation returns; the swap-chain acquisition timeout that follows is the last
			 * symptom, not the fault.
			 *
			 * ⚠️ Callbacks ARE delivered — that is the point, the compositor must see a live client.
			 * Anything called here therefore runs re-entrantly with respect to the operation being
			 * waited on: keep that operation on a worker thread and this call on the main one.
			 *
			 * @see Scenes::Loaders::Interface::loadAsync()
			 * @see PlatformSpecific::executeCommandPumpingEvents(), which applies this to the Linux
			 * native dialogs: without it a zenity/kdialog box gets the application force-quit prompt.
			 * @note A no-op until the window exists: a native dialog may run before GLFW is initialized (or after it
			 * failed to), where glfwPollEvents() would raise GLFW_NOT_INITIALIZED at every pump.
			 */
			void
			pumpEvents () const noexcept
			{
				if ( m_handle == nullptr )
				{
					return;
				}

				glfwPollEvents();
			}

			/**
			 * @brief Executes a function with all GLFW callbacks temporarily disabled.
			 * @details This method is useful on Windows where vkCreateSwapchainKHR() can deadlock
			 * if GLFW callbacks are triggered during swap-chain recreation. The method:
			 * 1. Saves and disables all GLFW callbacks (window, input, error, monitor, joystick)
			 * 2. Polls pending system events (pumps the Windows message queue)
			 * 3. Executes the provided function
			 * 4. Restores all callbacks to their previous values
			 * @tparam function_t The callable type.
			 * @param function The function to execute while callbacks are disabled.
			 */
			template< typename function_t >
			void
			executeWithoutCallbacks (function_t && function) const noexcept
			{
				if ( m_handle == nullptr )
				{
					std::forward< function_t >(function)();

					return;
				}

				auto * window = m_handle.get();

				/* Save and disable all window callbacks. */
				const auto prevWindowPos = glfwSetWindowPosCallback(window, nullptr);
				const auto prevWindowSize = glfwSetWindowSizeCallback(window, nullptr);
				const auto prevWindowClose = glfwSetWindowCloseCallback(window, nullptr);
				const auto prevWindowRefresh = glfwSetWindowRefreshCallback(window, nullptr);
				const auto prevWindowFocus = glfwSetWindowFocusCallback(window, nullptr);
				const auto prevWindowIconify = glfwSetWindowIconifyCallback(window, nullptr);
				const auto prevWindowMaximize = glfwSetWindowMaximizeCallback(window, nullptr);
				const auto prevFramebufferSize = glfwSetFramebufferSizeCallback(window, nullptr);
				const auto prevWindowContentScale = glfwSetWindowContentScaleCallback(window, nullptr);

				/* Save and disable all input callbacks. */
				const auto prevKey = glfwSetKeyCallback(window, nullptr);
				const auto prevChar = glfwSetCharCallback(window, nullptr);
				const auto prevMouseButton = glfwSetMouseButtonCallback(window, nullptr);
				const auto prevCursorPos = glfwSetCursorPosCallback(window, nullptr);
				const auto prevCursorEnter = glfwSetCursorEnterCallback(window, nullptr);
				const auto prevScroll = glfwSetScrollCallback(window, nullptr);
				const auto prevDrop = glfwSetDropCallback(window, nullptr);

				/* Save and disable global callbacks. */
				const auto prevError = glfwSetErrorCallback(nullptr);
				const auto prevMonitor = glfwSetMonitorCallback(nullptr);
				const auto prevJoystick = glfwSetJoystickCallback(nullptr);

				/* Pump the message queue without triggering any callbacks. */
				glfwPollEvents();

				/* Execute the user function. */
				std::forward< function_t >(function)();

				/* Restore all window callbacks. */
				glfwSetWindowPosCallback(window, prevWindowPos);
				glfwSetWindowSizeCallback(window, prevWindowSize);
				glfwSetWindowCloseCallback(window, prevWindowClose);
				glfwSetWindowRefreshCallback(window, prevWindowRefresh);
				glfwSetWindowFocusCallback(window, prevWindowFocus);
				glfwSetWindowIconifyCallback(window, prevWindowIconify);
				glfwSetWindowMaximizeCallback(window, prevWindowMaximize);
				glfwSetFramebufferSizeCallback(window, prevFramebufferSize);
				glfwSetWindowContentScaleCallback(window, prevWindowContentScale);

				/* Restore all input callbacks. */
				glfwSetKeyCallback(window, prevKey);
				glfwSetCharCallback(window, prevChar);
				glfwSetMouseButtonCallback(window, prevMouseButton);
				glfwSetCursorPosCallback(window, prevCursorPos);
				glfwSetCursorEnterCallback(window, prevCursorEnter);
				glfwSetScrollCallback(window, prevScroll);
				glfwSetDropCallback(window, prevDrop);

				/* Restore global callbacks. */
				glfwSetErrorCallback(prevError);
				glfwSetMonitorCallback(prevMonitor);
				glfwSetJoystickCallback(prevJoystick);
			}

			/**
			 * @brief Gets the window state into a string.
			 * @param directData Call glfw lib to get the data instead of printing the last known state.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getWindowStateString (bool directData) const noexcept;

			/**
			 * @brief Gets the window attrib into a string.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string getWindowAttribString () const noexcept;

			/**
			 * @brief Removes the window top bar containing the title and OS-specific buttons.
			 */
			void disableTitleBar () noexcept;

			/**
			 * @brief Applies the light/dark appearance of the OS title bar.
			 * @note Driven by the "Core/Video/Window/TitleBarTheme" setting: "System" (default) follows
			 * the OS light/dark preference, "Dark" and "Light" force it. Only the appearance is set,
			 * the colors themselves stay the ones the OS theme dictates.
			 * @note Implemented on Windows (DWM). No-op on macOS, where AppKit already follows the
			 * system appearance, and on Linux, where the compositor owns the decorations.
			 */
			void applyTitleBarTheme () noexcept;

			/**
			 * @brief Answers a pending quit request of the system as refused: the application decided to run on.
			 * @note macOS: the suspended "quit" Apple Event (Dock, AppleScript, the end of the session) is answered
			 * "user canceled" (-128) — a session end then shows that the application cancelled it. No-op elsewhere (a
			 * termination signal or the end of a Windows session has no answer). Main thread.
			 */
			void cancelQuitRequest () noexcept;

			/**
			 * @brief Returns the position, in screen coordinates of the window content area upper-left corner.
			 * @param monitor A pointer to the monitor. Default primary.
			 * @return std::array< int32_t, 2 >
			 */
			[[nodiscard]]
			std::array< int32_t, 2 > getDesktopPosition (GLFWmonitor * monitor = nullptr) const noexcept;

			/**
			 * @brief Returns the desktop resolution.
			 * @param monitor A pointer to the monitor. Default primary.
			 * @return std::array< uint32_t, 2 >
			 */
			[[nodiscard]]
			std::array< uint32_t, 2 > getDesktopSize (GLFWmonitor * monitor = nullptr) const noexcept;

			/**
			 * @brief Returns the largest window size a resize may ask, in screen coordinates.
			 * @note The device's maxImageDimension2D divided by the content scale (the framebuffer is size × scale), and,
			 * for a window, the largest monitor work area (owner ruling 2026-10-01: a 17000 pt request became a
			 * 34000 px framebuffer on macOS, exhausted the GPU memory and lost the device).
			 * @return std::array< uint32_t, 2 >
			 */
			[[nodiscard]]
			std::array< uint32_t, 2 > maximumWindowSize () const noexcept;

			/**
			 * @brief Returns the cached list of connected monitors.
			 * @note The list is populated on initialization and updated on hot-plug events.
			 * @return const Base::StaticVector< MonitorDevice, 16 > &
			 */
			[[nodiscard]]
			const Base::StaticVector< MonitorDevice, 16 > &
			monitorDevices () const noexcept
			{
				return m_monitorDevices;
			}

			/**
			 * @brief Recreates the Vulkan surface (destroys then creates).
			 * @note This is useful on Windows where vkCreateSwapchainKHR can deadlock.
			 * By destroying the surface completely and recreating it, we avoid the
			 * problematic swap-chain transition.
			 * @return bool
			 */
			[[nodiscard]]
			bool recreateSurface () noexcept;

		private:

			/**
			 * @brief Sets the framebuffer size of the state and publishes it for the render thread.
			 * @note Main thread only (the GLFW callbacks, create(), resize(), initializeState()).
			 * @param width The width in pixels.
			 * @param height The height in pixels.
			 */
			void
			setFramebufferState (uint32_t width, uint32_t height) noexcept
			{
				m_state.framebufferWidth = width;
				m_state.framebufferHeight = height;

				m_publishedFramebufferSize.store((static_cast< uint64_t >(width) << 32) | height, std::memory_order_release);
			}

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Console::ControllableTrait::onRegisterToConsole() */
			void onRegisterToConsole () noexcept override;

			/**
			 * @brief Tells whether the window should show information.
			 * @return bool
			 */
			[[nodiscard]]
			bool showInformation () const noexcept;

			/**
			 * @brief Initializes native window info.
			 * @return bool
			 */
			bool initializeNativeWindow () noexcept;

			/**
			 * @brief Releases the native window info.
			 */
			void releaseNativeWindow () noexcept;

			/**
			 * @brief Saves the least minimum info on the window state at creation.
			 * @param fakeWindow Declare a fake window
			 */
			void initializeState (bool fakeWindow) noexcept;

			/**
			 * @brief Checks monitors availability, populates the monitor cache and registers the hot-plug callback.
			 * @param showInformation Log monitor details to the trace output.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkMonitors (bool showInformation) noexcept;

			/**
			 * @brief Refreshes the cached monitor list from GLFW.
			 */
			void refreshMonitorDevices () noexcept;

			/**
			 * @brief Creates the window-related objects.
			 * @param width The width of the window.
			 * @param height The height of the window.
			 * @param monitorNumber The number of the monitor for multiple monitors configuration.
			 * @param fullscreenMode Request the fullscreen mode.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (int width, int height, int monitorNumber, bool fullscreenMode) noexcept;

			/**
			 * @brief Creates the vulkan surface, through glfwCreateWindowSurface() on every platform.
			 * @note The native-code paths and their setting (Core/Video/Window/GLFW/EnableNativeCodeForVkSurface) were
			 * removed (owner ruling 2026-10-01): off by default and broken (an XCB surface with a null connection,
			 * nothing for Wayland, an empty macOS branch).
			 * @return bool
			 */
			[[nodiscard]]
			bool createSurface () noexcept;

			/**
			 * @brief Destroys the current Vulkan surface.
			 */
			void destroySurface () noexcept;

			/**
			 * @brief Returns the monitor handles at monitor desired index.
			 * @note If the desired monitor is -1, the settings will be used then, if the desired monitor index is invalid, the primary one will be used.
			 * @param monitorIndex The desired monitor index. Default -1.
			 * @return GLFWmonitor *
			 */
			[[nodiscard]]
			GLFWmonitor * getMonitor (int32_t monitorIndex = -1) const noexcept;

			/**
			 * @brief Returns a list of available monitors on the system.
			 * @return std::vector< GLFWmonitor * >
			 */
			[[nodiscard]]
			static std::vector< GLFWmonitor * > getMonitors () noexcept;

			/**
			 * @brief Returns a list of available mode for a specified monitor.
			 * @return std::vector< GLFWvidmode >
			 */
			[[nodiscard]]
			static std::vector< GLFWvidmode > getMonitorModes (GLFWmonitor * monitor) noexcept;

			/**
			 * @brief This callback for GLFW is called when the allocated framebuffer of the window is resized.
			 * @note The order of call is: framebufferSizeCallback(), windowRefreshCallback(), windowSizeCallback() and finally windowPositionCallback().
			 * @param window A pointer to the system window handle.
			 * @param width The width of the framebuffer in pixel.
			 * @param height The height of the framebuffer in pixel.
			 */
			static void framebufferSizeCallback (GLFWwindow * window, int width, int height) noexcept;

			/**
			 * @brief This callback for GLFW is called when the window is modified by the system. Example: a resize.
			 * @param window A pointer to the system window handle.
			 */
			static void windowRefreshCallback (GLFWwindow * window) noexcept;

			/**
			 * @brief This callback for GLFW is called when the allocated framebuffer of the window is resized.
			 * @note Do not pass the window size to glViewport or other pixel-based OpenGL calls.
			 * The window size is in screen coordinates, not pixels.
			 * Use the framebuffer size, which is in pixels, for pixel-based calls.
			 * @param window A pointer to the system window handle.
			 * @param width The new width of the window in screen coordinates.
			 * @param height The new height of the window in screen coordinates.
			 */
			static void windowSizeCallback (GLFWwindow * window, int width, int height) noexcept;

			/**
			 * @brief Returns the Window a GLFW window callback belongs to, through its callback context.
			 * @param handle The GLFW window handle given to the callback.
			 * @return Window * nullptr when the handle carries no context.
			 */
			[[nodiscard]]
			static
			Window *
			fromHandle (GLFWwindow * handle) noexcept
			{
				const auto * context = callbackContext(handle);

				return context != nullptr ? context->window : nullptr;
			}

			/**
			 * @brief Notifies a window position change on the user desktop.
			 * @param window A pointer to the system window handle.
			 * @param xPosition The new x position of the window.
			 * @param yPosition The new y position of the window.
			 */
			static void windowPositionCallback (GLFWwindow * window, int xPosition, int yPosition) noexcept;

			/**
			 * @brief Notifies a monitor configuration change.
			 * @param monitor A pointer to the system monitor handle.
			 * @param event The type of event.
			 */
			static void monitorConfigurationChanged (GLFWmonitor * monitor, int event) noexcept;

			/**
			 * @brief Notifies a use try to close the window from the host system.
			 * @param window A pointer to the system window handle.
			 */
			static void windowCloseCallback (GLFWwindow * window) noexcept;

			/**
			 * @brief Notifies the window is focused by the host system.
			 * @param window A pointer to the system window handle.
			 * @param focused The state from the OS.
			 */
			static void windowFocusCallback (GLFWwindow * window, int focused) noexcept;

			/**
			 * @brief Notifies the window is hidden from the desktop in the host system.
			 * @param window A pointer to the system window handle.
			 * @param hidden The state from the OS.
			 */
			static void windowIconifyCallback (GLFWwindow * window, int hidden) noexcept;

			/**
			 * @brief Notifies whether the window is enlarged to cover the whole desktop in the host system.
			 * @param window A pointer to the system window handle.
			 * @param maximized The state from the OS.
			 */
			static void windowMaximizeCallback (GLFWwindow * window, int maximized) noexcept;

			/**
			 * @brief Notifies the window content request a scale by the host system.
			 * @param window A pointer to the system window handle.
			 * @param xScale The scale in X of the window content.
			 * @param yScale The scale in Y of the window content.
			 */
			static void windowContentScaleCallback (GLFWwindow * window, float xScale, float yScale) noexcept;

			/**
			 * @brief Returns the centered position for the window.
			 * @note The window dimensions are forced due to a GLFW bug at startup.
			 * @param windowSize A reference to an array of 2 unsigned integers.
			 * @param desiredMonitor The number of the monitor for multiple monitors configuration. Default preferred monitor.
			 * @return std::array< int32_t, 2 >
			 */
			[[nodiscard]]
			std::array< int32_t, 2 > getCenteredPosition (const std::array< uint32_t , 2 > & windowSize, int32_t desiredMonitor = -1) const noexcept;

#if IS_WINDOWS
			/**
			 * @brief Subclasses the GLFW window procedure: the end of the user session (WM_QUERYENDSESSION, WM_ENDSESSION,
			 * always) and the resize pause (WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE, only when enabled — off for now).
			 * @param enableResizePause Whether the resize pause is handled.
			 */
			void setupWindowsResizeHandling (bool enableResizePause) noexcept;

			/**
			 * @brief Gives the original GLFW window procedure back. Called before the window is destroyed.
			 */
			void restoreWindowProcedure () noexcept;

			/**
			 * @brief Custom Windows procedure to intercept WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE.
			 * @param hWnd The window handle.
			 * @param uMsg The message identifier.
			 * @param wParam Additional message information.
			 * @param lParam Additional message information.
			 * @return LRESULT
			 */
			static LRESULT CALLBACK windowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

			PrimaryServices & m_primaryServices;
			const Vulkan::Instance & m_instance;
			std::string m_title;
			State m_state{};
			std::unique_ptr< GLFWwindow, std::function< void (GLFWwindow *) > > m_handle;
			/* The window user pointer of m_handle: its address is stable for the Window's whole life. */
			CallbackContext m_callbackContext{.window = this, .inputManager = nullptr};
			std::unique_ptr< Vulkan::Surface > m_surface;
#if IS_WINDOWS
			WNDPROC m_originalWndProc{nullptr};
#endif
#if IS_MACOS || IS_WINDOWS
			/* Held while the window exists when Core/Video/Window/PreventDisplaySleep is true (owner decision 2026-10-11). */
			PlatformSpecific::DisplaySleepInhibitor m_displaySleepInhibitor;
#endif
#if IS_MACOS
			/* The "quit" Apple Event (AppleScript, the end of the user session) as a system stop, answered when the window is
			 * released (initializeNativeWindow() / releaseNativeWindow()). */
			std::unique_ptr< QuitAppleEventHandler, QuitAppleEventHandlerDeleter > m_quitAppleEventHandler;
#endif
			/* The framebuffer size for the render thread (publishedFramebufferSize()): width in the high 32 bits. */
			std::atomic< uint64_t > m_publishedFramebufferSize{0};
			/* The monitor hot-plug callback and its receiver (checkMonitors()): its end uninstalls both. */
			Base::ScopedRegistration m_monitorCallbackRegistration;
			bool m_windowLess{false};
			bool m_saveWindowPropertiesAtExit{false};
			bool m_isUserResizing{false};
#if IS_WINDOWS
			/* Whether initializeNativeWindow() initialised COM, so that releaseNativeWindow() uninitialises it only then. */
			bool m_COMInitialized{false};
			/* Whether windowProc() pauses the rendering while the user drags or resizes the window (off for now). */
			bool m_resizePauseEnabled{false};
#endif
			Base::StaticVector< MonitorDevice, 16 > m_monitorDevices;

			/** @brief The window served by the GLFW monitor callback, which carries no user pointer: set and cleared only by
			 * m_monitorCallbackRegistration (one window at a time), read on the main thread, null-checked. */
			inline static Window * s_monitorCallbackReceiver{nullptr};
	};
}
