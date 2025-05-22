/*
 * src/Window.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Vulkan/Surface.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class Instance;
	}

	class Identification;
	class PrimaryServices;
}

#if IS_MACOS

using id = struct objc_object *;

#elif IS_WINDOWS
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif

	#include <Windows.h>
#endif

namespace EmEn
{
	/**
	 * @brief The handle service class is responsible for the physical screen.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class Window final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"WindowService"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/**
			 * @brief Structure to keep a state of the window geometry.
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
			enum NotificationCode: uint8_t
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
				OSNotifiesFramebufferResized,
				OSRequestsToRescaleContentBy,
				OSRequestsToRefreshContent,
				OSRequestsToTerminate,
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
			Window (PrimaryServices & primaryServices, const Vulkan::Instance & instance, const Identification & identification) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Window (const Window & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Window (Window && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Window &
			 */
			Window & operator= (const Window & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Window &
			 */
			Window & operator= (Window && copy) noexcept = delete;

			/**
			 * @brief Destructs the window.
			 */
			~Window () override;

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
				return m_handle != nullptr;
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
			 * @return void
			 */
			void setTitle (const std::string & title) noexcept;

			/**
			 * @brief Resizes programmatically the window size. This function takes care of the fullscreen/window mode.
			 * @param width The new width.
			 * @param height The new height.
			 * @return bool
			 */
			bool resize (int32_t width, int32_t height) const noexcept;

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
			 * @return void
			 */
			void centerPosition (int32_t desiredMonitor = -1) noexcept;

			/**
			 * @brief Sets a gamma value to a specific monitor.
			 * @param value A gamma value.
			 * @param desiredMonitor The number of the monitor for multiple monitors configuration. Default initial monitor.
			 * @return void
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
			 * @return void
			 */
			void switchToFullscreenMode (bool useNativeResolution = false, int32_t desiredMonitor = -1) const noexcept;

			/**
			 * @brief Switch the application to windowed mode.
			 * @return void
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
			 * @return void
			 */
			void iconify () const noexcept;

			/**
			 * @brief Restores the window on the user desktop view.
			 * @return void
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
			 * @return void
			 */
			void maximizeSize () const noexcept;

			/**
			 * @brief Restores the window to previous size.
			 * @return void
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
			 * @return bool
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
			 * @return bool
			 */
			void hide () const noexcept;

			/**
			 * @brief Shows the window.
			 * @return bool
			 */
			void show () const noexcept;

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
			 * @brief Returns the position, in screen coordinates, of the upper-left corner of the content area of the window.
			 * @warning Do not use this function to respond to window events. Data are updated after the event in GLFW.  Use Window::state() instead.
			 * @return std::array< int32_t, 2 >
			 */
			[[nodiscard]]
			std::array< int32_t, 2 > getPosition () const noexcept;

			/**
			 * @brief Returns the size, in screen coordinates, of the content area of the window.
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
			 * @brief Waits for a valid window size.
			 * @return void
			 */
			void waitValidWindowSize () const noexcept;

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
			 * @return void
			 */
			void disableTitleBar () noexcept;

			/**
			 * @brief Returns the instance of the file system.
			 * @return Window *
			 */
			[[nodiscard]]
			static
			Window *
			instance () noexcept
			{
				return s_instance;
			}

			/**
			 * @brief Returns the position, in screen coordinates, of the upper-left corner of the content area of the window.
			 * @param monitor A pointer to the monitor. Default primary.
			 * @return std::array< int32_t, 2 >
			 */
			[[nodiscard]]
			static std::array< int32_t, 2 > getDesktopPosition (GLFWmonitor * monitor = nullptr) noexcept;

			/**
			 * @brief Returns the desktop resolution.
			 * @param monitor A pointer to the monitor. Default primary.
			 * @return std::array< uint32_t, 2 >
			 */
			[[nodiscard]]
			static std::array< uint32_t, 2 > getDesktopSize (GLFWmonitor * monitor = nullptr) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Initializes native window info.
			 * @return bool
			 */
			bool initializeNativeWindow () noexcept;

			/**
			 * @brief Releases the native window info.
			 * @return void
			 */
			void releaseNativeWindow () noexcept;

			/**
			 * @brief Saves the least minimum info on the window state at creation.
			 * @return void
			 */
			void initializeState () noexcept;

			/**
			 * @brief Checks monitors information and keeps it to a static member from here.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkMonitors () const noexcept;

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
			 * @brief Creates the vulkan surface.
			 * @param useNativeCode Use vulkan native code instead of glfw.
			 * @return bool
			 */
			[[nodiscard]]
			bool createSurface (bool useNativeCode) noexcept;

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
			 * @return void
			 */
			static void framebufferSizeCallback (GLFWwindow * window, int width, int height) noexcept;

			/**
			 * @brief This callback for GLFW is called when the window is modified by the system. Example: a resize.
			 * @param window A pointer to the system window handle.
			 * @return void
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
			 * @return void
			 */
			static void windowSizeCallback (GLFWwindow * window, int width, int height) noexcept;

			/**
			 * @brief Notifies a window position change on the user desktop.
			 * @param window A pointer to the system window handle.
			 * @param xPosition The new x position of the window.
			 * @param yPosition The new y position of the window.
			 * @return void
			 */
			static void windowPositionCallback (GLFWwindow * window, int xPosition, int yPosition) noexcept;

			/**
			 * @brief Notifies a monitor configuration change.
			 * @param monitor A pointer to the system monitor handle.
			 * @param event The type of event.
			 * @return void
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
			 * @return void
			 */
			static void windowFocusCallback (GLFWwindow * window, int focused) noexcept;

			/**
			 * @brief Notifies the window is hidden from the desktop in the host system.
			 * @param window A pointer to the system window handle.
			 * @param hidden The state from the OS.
			 * @return void
			 */
			static void windowIconifyCallback (GLFWwindow * window, int hidden) noexcept;

			/**
			 * @brief Notifies whether the window is enlarged to cover the whole desktop in the host system.
			 * @param window A pointer to the system window handle.
			 * @param maximized The state from the OS.
			 * @return void
			 */
			static void windowMaximizeCallback (GLFWwindow * window, int maximized) noexcept;

			/**
			 * @brief Notifies the window content request a scale by the host system.
			 * @param window A pointer to the system window handle.
			 * @param xScale The scale in X of the window content.
			 * @param yScale The scale in Y of the window content.
			 * @return void
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

			/* Flag names. */
			static constexpr auto ShowInformation{0UL};
			static constexpr auto SaveWindowPropertiesAtExit{1UL};

			/** @brief Singleton pointer. */
			static Window * s_instance;

			PrimaryServices & m_primaryServices;
			const Vulkan::Instance & m_instance;
			std::string m_title;
			State m_state{};
			std::unique_ptr< GLFWwindow, std::function< void (GLFWwindow *) > > m_handle;
			std::unique_ptr< Vulkan::Surface > m_surface;
			std::array< bool, 8 > m_flags{
				false/*ShowInformation*/,
				false/*SaveWindowPropertiesAtExit*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
