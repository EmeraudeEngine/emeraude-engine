/*
 * src/Window.cpp
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

#include "Window.hpp"

/* STL inclusions. */
#include <cmath>
#include <exception>
#include <iostream>
#include <utility>

/* Local inclusions. */
#include "Vulkan/Instance.hpp"
#include "Identification.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"

namespace EmEn
{
	using namespace EmEn::Libs;
	using namespace Graphics;
	using namespace Vulkan;

	const size_t Window::ClassUID{getClassUID(ClassId)};

	Window * Window::s_instance{nullptr};

	Window::Window (PrimaryServices & primaryServices, const Instance & instance, const Identification & identification) noexcept
		: ServiceInterface(ClassId),
		m_primaryServices(primaryServices),
		m_instance(instance),
		m_title(identification.applicationId())
	{
		if ( s_instance != nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

			std::terminate();
		}

		s_instance = this;
	}

	Window::~Window ()
	{
		s_instance = nullptr;
	}

	bool
	Window::onInitialize () noexcept
	{
		m_flags[ShowInformation] = m_primaryServices.settings().get< bool >(VideoWindowShowInformationKey, DefaultVideoWindowShowInformation);
		m_flags[SaveWindowPropertiesAtExit] = m_primaryServices.settings().get< bool >(VideoSavePropertiesAtExitKey, DefaultVideoSavePropertiesAtExit);

		if ( !m_instance.usable() )
		{
			Tracer::fatal(ClassId, "There is no vulkan instance available !");

			return false;
		}

		/* Checks monitor presence. */
		if ( !this->checkMonitors() )
		{
			Tracer::fatal(ClassId, "There is no monitor connected to the system !");

			return false;
		}

		/* NOTE: One multi monitors system, the preferred monitor will be used to display the window and fetch physical information. */
		const auto preferredMonitor = m_primaryServices.settings().get< int32_t >(VideoPreferredMonitorKey, DefaultVideoPreferredMonitor);

		/* NOTE: Disabling OpenGL context creation for Vulkan API */
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		/* Creating window in window or fullscreen mode */
		if ( m_primaryServices.settings().get< bool >(VideoFullscreenEnabledKey, DefaultVideoFullscreenEnabled) )
		{
			const auto fullscreenWidth = m_primaryServices.settings().get< int32_t >(VideoFullscreenWidthKey, DefaultVideoFullscreenWidth);
			const auto fullscreenHeight = m_primaryServices.settings().get< int32_t >(VideoFullscreenHeightKey, DefaultVideoFullscreenHeight);
			const auto refreshRate = m_primaryServices.settings().get< int32_t >(VideoFullscreenRefreshRateKey, DefaultVideoFullscreenRefreshRate);

			if ( refreshRate > 0 )
			{
				glfwWindowHint(GLFW_REFRESH_RATE, refreshRate);

				if ( m_flags[ShowInformation] )
				{
					TraceInfo{ClassId} << refreshRate << "Hz monitor refresh rate requested.";
				}
			}
			else
			{
				glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);
			}

			/* GLFW_AUTO_ICONIFY specifies whether the full-screen window will
			 * automatically iconify and restore the previous video mode on
			 * input focus loss. Possible values are GLFW_TRUE and GLFW_FALSE.
			 * This hint is ignored for windowed mode windows. */
			glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_TRUE);

			if ( !this->create(fullscreenWidth, fullscreenHeight, preferredMonitor, true) )
			{
				Tracer::fatal(ClassId, "Unable to get a valid fullscreen window !");

				/* NOTE: Set back settings to default to avoid failing start in loop. */
				m_primaryServices.settings().set< int32_t >(VideoFullscreenWidthKey, DefaultVideoFullscreenWidth);
				m_primaryServices.settings().set< int32_t >(VideoFullscreenHeightKey, DefaultVideoFullscreenHeight);

				return false;
			}
		}
		else
		{
            const auto windowWidth = m_primaryServices.settings().get< int32_t >(VideoWindowWidthKey, DefaultVideoWindowWidth);
            const auto windowHeight = m_primaryServices.settings().get< int32_t >(VideoWindowHeightKey, DefaultVideoWindowHeight);
			const auto frameless = m_primaryServices.settings().get< bool >(VideoWindowFramelessKey, DefaultVideoWindowFrameless);

			/* GLFW_RESIZABLE specifies whether the windowed mode window
			 * will be resizable by the user. The window will still be resizable
			 * using the glfwSetWindowSize function. Possible values are
			 * GLFW_TRUE and GLFW_FALSE. This hint is ignored for full screen
			 * and undecorated windows. */
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

			/* GLFW_VISIBLE specifies whether the windowed mode window will
			 * be initially visible. Possible values are GLFW_TRUE
			 * and GLFW_FALSE. This hint is ignored for full-screen windows. */
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); /* NOTE: Start hidden! */

			/* GLFW_DECORATED specifies whether the windowed mode window will
			 * have window decorations such as a border, a close widget, etc.
			 * An undecorated window will not be resizable by the user but will
			 * still allow the user to generate close events on some platforms.
			 * Possible values are GLFW_TRUE and GLFW_FALSE. This hint is ignored
			 * for full-screen windows. */
			glfwWindowHint(GLFW_DECORATED, frameless ? GLFW_FALSE : GLFW_TRUE);

			/* GLFW_FOCUSED specifies whether the windowed mode window will be
			 * given input focus when created. Possible values are GLFW_TRUE
			 * and GLFW_FALSE. This hint is ignored for full screen and initially
			 * hidden windows. */
			glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);

			/* GLFW_FLOATING specifies whether the windowed mode window will be
			 * floating above other regular windows, also called topmost or always-on-top.
			 * This is intended primarily for debugging purposes and cannot be used to
			 * implement proper full-screen windows. Possible values are GLFW_TRUE and GLFW_FALSE.
			 * This hint is ignored for full-screen windows. */
			glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);

			/* GLFW_MAXIMIZED specifies whether the windowed mode window will be maximized when created.
			 * Possible values are GLFW_TRUE and GLFW_FALSE. This hint is ignored for full-screen windows. */
			glfwWindowHint(GLFW_MAXIMIZED, GLFW_FALSE);

			/* GLFW_CENTER_CURSOR specifies whether the cursor should be centered over newly
			 * created full-screen windows. Possible values are GLFW_TRUE and GLFW_FALSE.
			 * This hint is ignored for windowed mode windows. */
			glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_TRUE);

			/* GLFW_TRANSPARENT_FRAMEBUFFER specifies whether the window framebuffer will
			 * be transparent. If enabled and supported by the system, the window framebuffer
			 * alpha channel will be used to combine the framebuffer with the background.
			 * This does not affect window decorations. Possible values are GLFW_TRUE and GLFW_FALSE. */
			glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);

			/* GLFW_FOCUS_ON_SHOW specifies whether the window will be given input
			 * focus when glfwShowWindow is called. Possible values are GLFW_TRUE and GLFW_FALSE. */
			glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);

			/* GLFW_SCALE_TO_MONITOR specified whether the window content area should be
			 * resized based on the monitor content scale of any monitor it is placed on.
			 * This includes the initial placement when the window is created.
			 * Possible values are GLFW_TRUE and GLFW_FALSE.
			 *
			 * This hint only affects platforms where screen coordinates
			 * and pixels always map 1:1 such as Windows and X11. On platforms like
			 * macOS, the resolution of the framebuffer is changed independently
			 * of the window size.*/
			glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);

			if constexpr ( IsLinux )
			{
				/* GLFW_X11_CLASS_NAME and GLFW_X11_INSTANCE_NAME specifies the desired ASCII
				 * encoded class and instance parts of the ICCCM WM_CLASS window property. */
				glfwWindowHintString(GLFW_X11_CLASS_NAME, EngineName);
				glfwWindowHintString(GLFW_X11_INSTANCE_NAME, ENGINE_NAME "_instance"); // FIXME: Check to pass the application name here.
			}

			if constexpr ( IsWindows )
			{
				/* GLFW_COCOA_RETINA_FRAMEBUFFER specifies whether to use full resolution framebuffers on Retina displays.
				 * Possible values are GLFW_TRUE and GLFW_FALSE. This is ignored on other platforms. */
				glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_TRUE);
				/* GLFW_COCOA_FRAME_NAME specifies the UTF-8 encoded name to use for autosaving the window frame, or if
				 * empty disables frame autosaving for the window. This is ignored on other platforms. */
				glfwWindowHintString(GLFW_COCOA_FRAME_NAME, EngineName);
				/* GLFW_COCOA_GRAPHICS_SWITCHING specifies whether to in Automatic Graphics Switching, i.e., to allow the
				 * system to choose the integrated GPU for the OpenGL context and move it between GPUs if necessary or
				 * whether to force it to always run on the discrete GPU. This only affects systems with both integrated
				 * and discrete GPUs. Possible values are GLFW_TRUE and GLFW_FALSE. This is ignored on other platforms. */
				glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, GLFW_FALSE);
			}

			if ( this->create(windowWidth, windowHeight, preferredMonitor, false) )
			{
				/* NOTE: If the resize fails (invalid size), set back settings to defaults. */
				if ( !this->resize(windowWidth, windowHeight) )
				{
					m_primaryServices.settings().set< int32_t >(VideoWindowWidthKey, DefaultVideoWindowWidth);
					m_primaryServices.settings().set< int32_t >(VideoWindowHeightKey, DefaultVideoWindowHeight);
				}
			}
			else
			{
				Tracer::fatal(ClassId, "Unable to get a valid window !");

				return false;
			}
		}

		if ( this->isFullscreenMode() )
		{
			const auto gamma = m_primaryServices.settings().get< float >(VideoFullscreenGammaKey, DefaultVideoFullscreenGamma);

			this->setGamma(gamma, preferredMonitor);
		}
		else
		{
			if ( m_primaryServices.settings().get< bool >(VideoWindowAlwaysCenterOnStartupKey, DefaultVideoWindowAlwaysCenterOnStartup) )
			{
				const auto windowWidth = m_primaryServices.settings().get< uint32_t >(VideoWindowWidthKey, DefaultVideoWindowWidth);
				const auto windowHeight = m_primaryServices.settings().get< uint32_t >(VideoWindowHeightKey, DefaultVideoWindowHeight);
				const auto centeredPosition = Window::getCenteredPosition({windowWidth, windowHeight}, preferredMonitor);

				if ( m_flags[ShowInformation] )
				{
					TraceInfo{ClassId} << "Center the window position to X: " << centeredPosition[0] << ", Y: " << centeredPosition[1] << " ...";
				}

				this->setPosition(centeredPosition[0], centeredPosition[1]);
			}
			else
			{
				const auto XPosition = m_primaryServices.settings().get< int32_t >(VideoWindowXPositionKey, DefaultVideoWindowXPosition);
				const auto YPosition = m_primaryServices.settings().get< int32_t >(VideoWindowYPositionKey, DefaultVideoWindowYPosition);

				if ( m_flags[ShowInformation] )
				{
					TraceInfo{ClassId} << "Setting window position to X: " << XPosition << ", Y: " << YPosition << " ...";
				}

				this->setPosition(XPosition, YPosition);
			}

			const auto gamma = m_primaryServices.settings().get< float >(VideoWindowGammaKey, DefaultVideoWindowGamma);

			this->setGamma(gamma, preferredMonitor);
		}

		const auto useNativeCode = m_primaryServices.settings().get< bool >(GLFWEnableNativeCodeForVkSurfaceKey, DefaultEnableNativeCodeForVkSurface);

		if ( !this->createSurface(useNativeCode) )
		{
			Tracer::fatal(ClassId, "Unable to get a valid surface !");

			return false;
		}

		this->initializeState();

		/* NOTE: The window starts non-visible. */
		glfwShowWindow(m_handle.get());

		if ( m_flags[ShowInformation] )
		{
			TraceInfo{ClassId} <<
				this->getWindowStateString(false) << "\n" <<
				this->getWindowAttribString();

			TraceSuccess{ClassId} << "A new " << m_state.framebufferWidth << "x" << m_state.framebufferHeight << " window created.";
		}

		this->notify(Created);

		if ( !this->initializeNativeWindow() )
		{
			Tracer::error(ClassId, "Native dialogs are not available !");
		}

		return true;
	}

	bool
	Window::onTerminate () noexcept
	{
		this->releaseNativeWindow();

		if ( m_flags[SaveWindowPropertiesAtExit] )
		{
			if ( m_flags[ShowInformation] )
			{
				Tracer::info(ClassId, "Saving the window properties ...");
			}

			if ( this->isFullscreenMode() )
			{
				m_primaryServices.settings().set(VideoFullscreenWidthKey, m_state.framebufferWidth);
				m_primaryServices.settings().set(VideoFullscreenHeightKey, m_state.framebufferHeight);
			}
			else
			{
				if constexpr ( IsMacOS )
				{
					const auto scaledWidth = static_cast< float >(m_state.framebufferWidth) / m_state.contentXScale;
					const auto scaledHeight = static_cast< float >(m_state.framebufferHeight) / m_state.contentYScale;

					m_primaryServices.settings().set(VideoWindowWidthKey, static_cast< uint32_t >(scaledWidth));
					m_primaryServices.settings().set(VideoWindowHeightKey, static_cast< uint32_t >(scaledHeight));
				}
				else
				{
					m_primaryServices.settings().set(VideoWindowWidthKey, m_state.framebufferWidth);
					m_primaryServices.settings().set(VideoWindowHeightKey, m_state.framebufferHeight);
				}

				m_primaryServices.settings().set(VideoWindowXPositionKey, m_state.windowXPosition);
				m_primaryServices.settings().set(VideoWindowYPositionKey, m_state.windowYPosition);
			}
		}

		m_surface.reset();

		/* Reset the window pointer.
		 * NOTE: It will automatically remove all callbacks. */
		m_handle.reset();

		return true;
	}

	void
	Window::setTitle (const std::string & title) noexcept
	{
		glfwSetWindowTitle(m_handle.get(), title.c_str());

		this->notify(TitleChanged, title);
	}

	bool
	Window::resize (int32_t width, int32_t height) const noexcept
	{
		if ( width <= 0 || height <= 0 )
		{
			TraceError{ClassId} << width << "x" << height << " is an invalid resolution.";

			return false;
		}

		if ( this->isFullscreenMode() )
		{
			/* Gets the closest acceptable width/height. */
			auto perfectFitFound = false;

			std::array< int32_t, 2 > closest{
				DefaultVideoFullscreenWidth,
				DefaultVideoFullscreenHeight
			};

			auto * monitor = glfwGetWindowMonitor(m_handle.get());

			const auto modes = getMonitorModes(monitor);

			for ( const auto & mode : modes )
			{
				if ( width == mode.width && height == mode.height )
				{
					perfectFitFound = true;

					break;
				}

				/* Checks for the below sizes. */
				if ( mode.width < width && mode.width > closest[0] && mode.height < height && mode.height > closest[1] )
				{
					closest[0] = mode.width;
					closest[1] = mode.height;
				}
			}

			if ( !perfectFitFound )
			{
				TraceWarning{ClassId} <<
					"Monitor cannot window the desired resolution " << width << "x" << height << ". "
					"Setting to " << closest[0] << "x" << closest[1]<< ".";

				width = closest[0];
				height = closest[1];
			}
		}

		glfwSetWindowSize(m_handle.get(), width, height);

		return true;
	}

	bool
	Window::setPosition (int32_t xPosition, int32_t yPosition) noexcept
	{
		glfwSetWindowPos(m_handle.get(),xPosition,yPosition);

		switch ( glfwGetError(nullptr) )
		{
			case GLFW_NOT_INITIALIZED :
			case GLFW_PLATFORM_ERROR :
				return false;

			default:
				this->notify(Centered);

				return true;
		}
	}

	void
	Window::centerPosition (int32_t desiredMonitor) noexcept
	{
		const auto centeredPosition = this->getCenteredPosition(desiredMonitor);

		this->setPosition(centeredPosition[0], centeredPosition[1]);
	}

	std::array< int32_t, 2 >
	Window::getCenteredPosition (const std::array< uint32_t , 2 > & windowSize, int32_t desiredMonitor) const noexcept
	{
		auto * monitor = Window::getMonitor(desiredMonitor);

		/* Get the monitor virtual position and dimensions and the window dimension. */
		const auto desktopPosition = Window::getDesktopPosition(monitor);
		const auto desktopSize = Window::getDesktopSize(monitor);

		TraceDebug{ClassId} <<
			"Desktop position (X: " << desktopPosition[0] << ", Y: " << desktopPosition[1] << ")" "\n"
			"Desktop dimension (Width: " << desktopSize[0] << ", Height: " << desktopSize[1] << ")" "\n"
			"Window dimension (Width: " << windowSize[0] << ", Height: " << windowSize[1] << ")" "\n";

		/* Get the window position center on the selected monitor. */
		return {
			static_cast< int32_t >(desktopPosition[0] + ((desktopSize[0] - windowSize[0]) / 2)),
			static_cast< int32_t >(desktopPosition[1] + ((desktopSize[1] - windowSize[1]) / 2))
		};
	}

	bool
	Window::isVisibleOnDesktop () const noexcept
	{
		return glfwGetWindowAttrib(m_handle.get(), GLFW_VISIBLE) == GLFW_TRUE;
	}

	bool
	Window::isIconified () const noexcept
	{
		return glfwGetWindowAttrib(m_handle.get(), GLFW_ICONIFIED) == GLFW_TRUE;
	}

	void
	Window::iconify () const noexcept
	{
		if ( this->isIconified() )
		{
			return;
		}

		glfwIconifyWindow(m_handle.get());
	}

	void
	Window::getBackOnDesktop () const noexcept
	{
		if ( this->isVisibleOnDesktop() )
		{
			return;
		}

		glfwRestoreWindow(m_handle.get());
	}

	bool
	Window::isSizeMaximized () const noexcept
	{
		return glfwGetWindowAttrib(m_handle.get(), GLFW_MAXIMIZED) == GLFW_TRUE;
	}

	bool
	Window::isSizeReduced () const noexcept
	{
		return !this->isSizeMaximized();
	}

	void
	Window::maximizeSize () const noexcept
	{
		if ( this->isSizeMaximized() )
		{
			return;
		}

		glfwMaximizeWindow(m_handle.get());
	}

	void
	Window::restoreSize () const noexcept
	{
		if ( this->isSizeReduced() )
		{
			return;
		}

		glfwRestoreWindow(m_handle.get());
	}

	bool
	Window::isFocused () const noexcept
	{
		return glfwGetWindowAttrib(m_handle.get(), GLFW_FOCUSED) == GLFW_TRUE;
	}

	bool
	Window::isBlurred () const noexcept
	{
		return !this->isFocused();
	}

	void
	Window::focus () const noexcept
	{
		if ( this->isFocused() )
		{
			return;
		}

		glfwFocusWindow(m_handle.get());
	}

	bool
	Window::isVisible () const noexcept
	{
		return glfwGetWindowAttrib(m_handle.get(), GLFW_VISIBLE) == GLFW_TRUE;
	}

	bool
	Window::isHidden () const noexcept
	{
		return !this->isVisible();
	}

	void
	Window::hide () const noexcept
	{
		if ( this->isHidden() )
		{
			return;
		}

		glfwHideWindow(m_handle.get());
	}

	void
	Window::show () const noexcept
	{
		if ( this->isVisible() )
		{
			return;
		}

		glfwShowWindow(m_handle.get());
	}

	void
	Window::setGamma (float value, int32_t desiredMonitor) const noexcept
	{
		auto * monitor = Window::getMonitor(desiredMonitor);

		if ( monitor != nullptr )
		{
			glfwSetGamma(monitor, value);
		}
	}

	void
	Window::switchToFullscreenMode (bool useNativeResolution, int32_t desiredMonitor) const noexcept
	{
		if ( this->isFullscreenMode() )
		{
			return;
		}

		/* NOTE: Get the right system monitor. */
		auto * monitor = Window::getMonitor(desiredMonitor);

		auto & settings = m_primaryServices.settings();

		/* Save the current window size. */
		{
			const auto currentSize = this->getFramebufferSize();
			settings.set<uint32_t>(VideoWindowWidthKey, currentSize[0]);
			settings.set<uint32_t>(VideoWindowHeightKey, currentSize[1]);
		}

		const auto refreshRate = settings.get< int32_t >(VideoFullscreenRefreshRateKey, DefaultVideoFullscreenRefreshRate);

		if ( useNativeResolution )
		{
			const auto monitorSize = getDesktopSize(monitor);

			glfwSetWindowMonitor(
				m_handle.get(),
				monitor,
				0, 0,
				static_cast< int32_t >(monitorSize[0]), static_cast< int32_t >(monitorSize[1]),
				refreshRate
			);
		}
		else
		{
			auto width = settings.get< int32_t >(VideoFullscreenWidthKey, DefaultVideoFullscreenWidth);
			auto height = settings.get< int32_t >(VideoFullscreenHeightKey, DefaultVideoFullscreenHeight);

			if ( width == 0 || height == 0 )
			{
				width = DefaultVideoFullscreenWidth;
				height = DefaultVideoFullscreenHeight;

				/* Save fullscreen settings. */
				settings.set< uint32_t >(VideoFullscreenWidthKey, width);
				settings.set< uint32_t >(VideoFullscreenHeightKey, height);
			}

			glfwSetWindowMonitor(
				m_handle.get(),
				monitor,
				0, 0,
				width, height,
				refreshRate
			);
		}
	}

	void
	Window::switchToWindowedMode () const noexcept
	{
		if ( this->isWindowedMode() )
		{
			return;
		}

		const auto & settings = m_primaryServices.settings();
		const auto windowWidth = settings.get< int32_t >(VideoWindowWidthKey, DefaultVideoWindowWidth);
		const auto windowHeight = settings.get< int32_t >(VideoWindowHeightKey, DefaultVideoWindowHeight);
		const auto XPosition = settings.get< int32_t >(VideoWindowXPositionKey, DefaultVideoWindowXPosition);
		const auto YPosition = settings.get< int32_t >(VideoWindowYPositionKey, DefaultVideoWindowYPosition);

		glfwSetWindowMonitor(
			m_handle.get(),
			nullptr,
			XPosition, YPosition,
			windowWidth, windowHeight,
			GLFW_DONT_CARE
		);
	}

	bool
	Window::isFullscreenMode () const noexcept
	{
		/* If there is no monitor attached to the handle,
		 * so this is not a fullscreen mode. */
		return glfwGetWindowMonitor(m_handle.get()) != nullptr;
	}

	std::array< int32_t, 2 >
	Window::getPosition () const noexcept
	{
		int xPosition = -1;
		int yPosition = -1;

		glfwGetWindowPos(m_handle.get(), &xPosition, &yPosition);

		/* NOTE: We use the glfw handler to display errors. */
		switch ( glfwGetError(nullptr) )
		{
			case GLFW_NOT_INITIALIZED :
			case GLFW_PLATFORM_ERROR :
				return {0, 0};

			case GLFW_NO_ERROR :
			default:
				break;
		}

		return {xPosition, yPosition};
	}

	std::array< uint32_t, 2 >
	Window::getSize () const noexcept
	{
		int widthPts = -1;
		int heightPts = -1;

		glfwGetWindowSize(m_handle.get(), &widthPts, &heightPts);

		/* NOTE: We use the glfw handler to display errors. */
		switch ( glfwGetError(nullptr) )
		{
			case GLFW_NOT_INITIALIZED :
			case GLFW_PLATFORM_ERROR :
				return {DefaultVideoWindowWidth, DefaultVideoWindowHeight};

			case GLFW_NO_ERROR :
			default:
				break;
		}

		return {
			static_cast< uint32_t >(widthPts),
			static_cast< uint32_t >(heightPts)
		};
	}

	std::array< uint32_t, 4 >
	Window::getBorderSize () const noexcept
	{
		int leftPts = -1;
		int topPts = -1;
		int rightPts = -1;
		int bottomPts = -1;

		glfwGetWindowFrameSize(m_handle.get(), &leftPts, &topPts, &rightPts, &bottomPts);

		/* NOTE: We use the glfw handler to display errors. */
		switch ( glfwGetError(nullptr) )
		{
			case GLFW_NOT_INITIALIZED :
			case GLFW_PLATFORM_ERROR :
				return {0, 0, 0, 0};

			case GLFW_NO_ERROR :
			default:
				break;
		}

		return {
			static_cast< uint32_t >(leftPts),
			static_cast< uint32_t >(topPts),
			static_cast< uint32_t >(rightPts),
			static_cast< uint32_t >(bottomPts)
		};
	}

	std::array< int32_t, 2 >
	Window::getDesktopPosition (GLFWmonitor * monitor) noexcept
	{
		/* Get the proper monitor to query */
		if ( monitor == nullptr )
		{
			monitor = glfwGetPrimaryMonitor();

			if ( monitor == nullptr )
			{
				Tracer::error(ClassId, "Unable to query the primary monitor !");

				return {0, 0};
			}
		}

		int xPosition = -1;
		int yPosition = -1;

		glfwGetMonitorPos(monitor, &xPosition, &yPosition);

		/* NOTE: We use the glfw handler to display errors. */
		switch ( glfwGetError(nullptr) )
		{
			case GLFW_NOT_INITIALIZED :
			case GLFW_PLATFORM_ERROR :
				return {0, 0};

			case GLFW_NO_ERROR :
				default:
					break;
		}

		return {xPosition, yPosition};
	}

	std::array< uint32_t, 2 >
	Window::getDesktopSize (GLFWmonitor * monitor) noexcept
	{
		/* Get the proper monitor to query */
		if ( monitor == nullptr )
		{
			monitor = glfwGetPrimaryMonitor();

			if ( monitor == nullptr )
			{
				Tracer::error(ClassId, "Unable to query the primary monitor !");

				return {0, 0};
			}
		}

		const auto * mode = glfwGetVideoMode(monitor);

		/* NOTE: We use the glfw handler to display errors. */
		if ( mode == nullptr )
		{
			return {0, 0};
		}

		return {
			static_cast< uint32_t >(mode->width),
			static_cast< uint32_t >(mode->height)
		};
	}

	std::array< uint32_t, 2 >
	Window::getFramebufferSize (bool applyScale) const noexcept
	{
		int widthPx = -1;
		int heightPx = -1;

		glfwGetFramebufferSize(m_handle.get(), &widthPx, &heightPx);

		/* NOTE: We use the glfw handler to display errors. */
		switch ( glfwGetError(nullptr) )
		{
			case GLFW_NOT_INITIALIZED :
			case GLFW_PLATFORM_ERROR :
				return {DefaultVideoWindowWidth, DefaultVideoWindowHeight};

			case GLFW_NO_ERROR :
			default:
				break;
		}

		if ( applyScale )
		{
			const auto scale = this->getContentScale();
			const auto scaledWidth = std::floor(static_cast< float >(widthPx) / scale.at(0));
			const auto scaledHeight = std::floor(static_cast< float >(heightPx) / scale.at(0));

			return {
				static_cast< uint32_t >(scaledWidth),
				static_cast< uint32_t >(scaledHeight)
			};
		}

		return {
			static_cast< uint32_t >(widthPx),
			static_cast< uint32_t >(heightPx)
		};
	}

	std::array< float, 2 >
	Window::getContentScale () const noexcept
	{
		std::array< float, 2 > scale{0.0F, 0.0F};

		glfwGetWindowContentScale(m_handle.get(), scale.data(), scale.data()+1);

		/* NOTE: We use the glfw handler to display errors. */
		switch ( glfwGetError(nullptr) )
		{
			case GLFW_NOT_INITIALIZED :
			case GLFW_PLATFORM_ERROR :
				scale[0] = 1.0F;
				scale[1] = 1.0F;
				break;

			case GLFW_NO_ERROR :
			default:
				break;
		}

		return scale;
	}

	float
	Window::aspectRatio () const noexcept
	{
		if ( m_state.framebufferWidth == 0 || m_state.framebufferHeight == 0 )
		{
			return 1.0F;
		}

		return static_cast< float >(m_state.framebufferWidth) / static_cast< float >(m_state.framebufferHeight);
	}

	void
	Window::waitValidWindowSize () const noexcept
	{
		bool validSizeFound = false;

		while ( !validSizeFound )
		{
			const auto size = this->getFramebufferSize();

			if ( size[0] > 0 || size[1] > 0 )
			{
				validSizeFound = true;
			}

			glfwWaitEvents();
		}
	}

	std::string
	Window::getWindowStateString (bool directData) const noexcept
	{
		if ( directData )
		{
			const auto position = this->getPosition();
			const auto size = this->getSize();
			const auto borderSize = this->getBorderSize();
			const auto scale = this->getContentScale();
			const auto framebufferSize = this->getFramebufferSize();

			return (std::stringstream{} <<
				"Desktop direct info :" "\n"
				" - Window X position : " << position[0] << "\n"
				" - Window Y position : " << position[1] << "\n"

				"Window direct info :" "\n"
				" - Width : " << size[0] << "pt" "\n"
				" - Height : " << size[1] << "pt" "\n"
				" - Left border size : " << borderSize[0] << "pt" "\n"
				" - Top border size : " << borderSize[1] << "pt" "\n"
				" - Right border size : " << borderSize[2] << "pt" "\n"
				" - Bottom border size : " << borderSize[3] << "pt" "\n"
				" - Content scale on X axis : " << scale[0] << "\n"
				" - Content scale on Y axis : " << scale[1] << "\n"

				"Framebuffer direct info :" "\n"
				" - Width : " << framebufferSize[0] << "px" "\n"
				" - Height : " << framebufferSize[1] << "px" "\n"
			).str();
		}

		return (std::stringstream{} <<
			"Desktop info :" "\n"
			" - Window X position : " << m_state.windowXPosition << "\n"
			" - Window Y position : " << m_state.windowYPosition << "\n"

			"Window info :" "\n"
			" - Width : " << m_state.windowWidth << "pt" "\n"
			" - Height : " << m_state.windowHeight << "pt" "\n"
			" - Left border size : " << m_state.borderLeftSize << "pt" "\n"
			" - Top border size : " << m_state.borderTopSize << "pt" "\n"
			" - Right border size : " << m_state.borderRightSize << "pt" "\n"
			" - Bottom border size : " << m_state.borderBottomSize << "pt" "\n"
			" - Content scale on X axis : " << m_state.contentXScale << "\n"
			" - Content scale on Y axis : " << m_state.contentYScale << "\n"

			"Framebuffer info :" "\n"
			" - Width : " << m_state.framebufferWidth << "px" "\n"
			" - Height : " << m_state.framebufferHeight << "px" "\n"
		).str();
	}

	std::string
	Window::getWindowAttribString () const noexcept
	{
		return (std::stringstream{} <<
			"Window related attributes :" "\n"
			" - GLFW_FOCUSED : " << glfwGetWindowAttrib(m_handle.get(), GLFW_FOCUSED) << "\n"
			" - GLFW_ICONIFIED : " << glfwGetWindowAttrib(m_handle.get(), GLFW_ICONIFIED) << "\n"
			" - GLFW_MAXIMIZED : " << glfwGetWindowAttrib(m_handle.get(), GLFW_MAXIMIZED) << "\n"
			" - GLFW_HOVERED : " << glfwGetWindowAttrib(m_handle.get(), GLFW_HOVERED) << "\n"
			" - GLFW_VISIBLE : " << glfwGetWindowAttrib(m_handle.get(), GLFW_VISIBLE) << "\n"
			" - GLFW_RESIZABLE : " << glfwGetWindowAttrib(m_handle.get(), GLFW_RESIZABLE) << "\n"
			" - GLFW_DECORATED : " << glfwGetWindowAttrib(m_handle.get(), GLFW_DECORATED) << "\n"
			" - GLFW_AUTO_ICONIFY : " << glfwGetWindowAttrib(m_handle.get(), GLFW_AUTO_ICONIFY) << "\n"
			" - GLFW_FLOATING : " << glfwGetWindowAttrib(m_handle.get(), GLFW_FLOATING) << "\n"
			" - GLFW_TRANSPARENT_FRAMEBUFFER : " << glfwGetWindowAttrib(m_handle.get(), GLFW_TRANSPARENT_FRAMEBUFFER) << "\n"
			" - GLFW_FOCUS_ON_SHOW : " << glfwGetWindowAttrib(m_handle.get(), GLFW_FOCUS_ON_SHOW) << "\n\n"

			"Context related attributes :" "\n"
			" - GLFW_CLIENT_API : " << glfwGetWindowAttrib(m_handle.get(), GLFW_CLIENT_API) << "\n"
			" - GLFW_CONTEXT_CREATION_API : " << glfwGetWindowAttrib(m_handle.get(), GLFW_CONTEXT_CREATION_API) << "\n"
			" - GLFW_CONTEXT_VERSION_MAJOR : " << glfwGetWindowAttrib(m_handle.get(), GLFW_CONTEXT_VERSION_MAJOR) << "\n"
			" - GLFW_CONTEXT_VERSION_MINOR : " << glfwGetWindowAttrib(m_handle.get(), GLFW_CONTEXT_VERSION_MINOR) << "\n"
			" - GLFW_CONTEXT_REVISION : " << glfwGetWindowAttrib(m_handle.get(), GLFW_CONTEXT_REVISION) << "\n"
			" - GLFW_OPENGL_FORWARD_COMPAT : " << glfwGetWindowAttrib(m_handle.get(), GLFW_OPENGL_FORWARD_COMPAT) << "\n"
			" - GLFW_OPENGL_DEBUG_CONTEXT : " << glfwGetWindowAttrib(m_handle.get(), GLFW_OPENGL_DEBUG_CONTEXT) << "\n"
			" - GLFW_OPENGL_PROFILE : " << glfwGetWindowAttrib(m_handle.get(), GLFW_OPENGL_PROFILE) << "\n"
			" - GLFW_CONTEXT_RELEASE_BEHAVIOR : " << glfwGetWindowAttrib(m_handle.get(), GLFW_CONTEXT_RELEASE_BEHAVIOR) << "\n"
			" - GLFW_CONTEXT_NO_ERROR : " << glfwGetWindowAttrib(m_handle.get(), GLFW_CONTEXT_NO_ERROR) << "\n"
			" - GLFW_CONTEXT_ROBUSTNESS : " << glfwGetWindowAttrib(m_handle.get(), GLFW_CONTEXT_ROBUSTNESS) << '\n'
		).str();
	}

	bool
	Window::create (int width, int height, int monitorNumber, bool fullscreenMode) noexcept
	{
		/* Parameters for creating the handle. */
		GLFWmonitor * monitor = nullptr;

		if ( fullscreenMode )
		{
			monitor = Window::getMonitor(monitorNumber);

			/* If one of the size is automatic,
			 * we use the current mode of the monitor */
			if ( width == Automatic || height == Automatic )
			{
				const auto * currentMode = glfwGetVideoMode(monitor);

				width = currentMode->width;
				height = currentMode->height;
			}
		}

		/* Checks sizes parameters. */
		if ( width <= 0 || height <= 0 )
		{
			TraceFatal{ClassId} << "Trying to create a window with invalid size (" << width << "x" << height << ") !";

			return false;
		}

		/* Creates the new handle. */
		m_handle = std::unique_ptr< GLFWwindow, std::function< void (GLFWwindow *) > >(
			glfwCreateWindow(width, height, m_title.c_str(), monitor, nullptr),
			glfwDestroyWindow
		);

		if ( m_handle == nullptr )
		{
			TraceFatal{ClassId} << "Unable to create a " << width << "x" << height << " window !";

			return false;
		}

		m_state.framebufferWidth = width;
		m_state.framebufferHeight = height;

		/* Register all callbacks related to the window object. */
		glfwSetWindowUserPointer(m_handle.get(), this);
		glfwSetWindowPosCallback(m_handle.get(), windowPositionCallback);
		glfwSetWindowSizeCallback(m_handle.get(), windowSizeCallback);
		glfwSetWindowCloseCallback(m_handle.get(), windowCloseCallback);
		glfwSetWindowRefreshCallback(m_handle.get(), windowRefreshCallback);
		glfwSetWindowFocusCallback(m_handle.get(), windowFocusCallback);
		glfwSetWindowIconifyCallback(m_handle.get(), windowIconifyCallback);
		glfwSetWindowMaximizeCallback(m_handle.get(), windowMaximizeCallback);
		glfwSetFramebufferSizeCallback(m_handle.get(), framebufferSizeCallback);
		glfwSetWindowContentScaleCallback(m_handle.get(), windowContentScaleCallback);

		return true;
	}

	void
	Window::initializeState () noexcept
	{
		/* The position (top left) of the window in the desktop screen. */
		const auto position = this->getPosition();

		m_state.windowXPosition = position[0];
		m_state.windowYPosition = position[1];

		/* The window dimension expressed in the OS/desktop screen. */
		const auto size = this->getSize();

		m_state.windowWidth = size[0];
		m_state.windowHeight = size[1];

		/* The window borders sizes (Depend on the OS/desktop). */
		const auto borderSize = this->getBorderSize();

		m_state.borderLeftSize = static_cast< int32_t >(borderSize[0]);
		m_state.borderTopSize = static_cast< int32_t >(borderSize[1]);
		m_state.borderRightSize = static_cast< int32_t >(borderSize[2]);
		m_state.borderBottomSize = static_cast< int32_t >(borderSize[3]);

		/* The content scale factor from the desktop (HDPI screen). */
		const auto scale = this->getContentScale();

		m_state.contentXScale = scale[0];
		m_state.contentYScale = scale[1];

		/* The framebuffer of the window expressed in pixels. */
		const auto framebufferSize = this->getFramebufferSize(); // Gives incorrect data at startup.

		m_state.framebufferWidth = framebufferSize[0];
		m_state.framebufferHeight = framebufferSize[1];
	}

	bool
	Window::checkMonitors () const noexcept
	{
		const auto monitors = Window::getMonitors();

		if ( monitors.empty() )
		{
			Tracer::fatal(ClassId, "There is no monitor on the system !");

			return false;
		}

		if ( m_flags[ShowInformation] )
		{
			TraceInfo{ClassId} << monitors.size() << " monitor(s) available(s).";
		}

		for ( auto * monitor : monitors )
		{
			/* NOTE: Display monitors information. */
			if ( m_flags[ShowInformation] )
			{
				const auto modes = Window::getMonitorModes(monitor);

				TraceInfo info{ClassId};

				info << "Monitor '" << glfwGetMonitorName(monitor) << "' added with " << modes.size() << " modes available :" "\n";

				for ( const auto & mode : modes )
				{
					const auto bits = mode.redBits + mode.greenBits + mode.blueBits;

					info << "\t" " - " << mode.width << 'x' << mode.height << 'x' << bits << '@' << mode.refreshRate << "hz" "\n";
				}
			}
		}

		glfwSetMonitorCallback(monitorConfigurationChanged);

		return true;
	}

	std::vector< GLFWmonitor * >
	Window::getMonitors () noexcept
	{
		std::vector< GLFWmonitor * > list;

		int count = 0;

		GLFWmonitor * * monitors = glfwGetMonitors(&count);

		if ( count > 0 && monitors != nullptr )
		{
			list.reserve(count);

			for ( int index = 0; index < count; index++ )
			{
				list.emplace_back(monitors[index]);
			}
		}

		return list;
	}

	GLFWmonitor *
	Window::getMonitor (int32_t desiredMonitorIndex) const noexcept
	{
		const auto monitors = Window::getMonitors();

		if ( monitors.empty() )
		{
			Tracer::error(ClassId, "The is no monitor on this system !");

			return nullptr;
		}

		const auto monitorIndex = desiredMonitorIndex < 0 ?
			m_primaryServices.settings().get< uint32_t >(VideoPreferredMonitorKey, DefaultVideoPreferredMonitor) :
			static_cast< uint32_t >(desiredMonitorIndex);

		if ( monitorIndex >= monitors.size() )
		{
			TraceError{ClassId} <<
				"The monitor #" << monitorIndex << " doesn't exists. "
				"Using the primary monitor.";

			return glfwGetPrimaryMonitor();
		}

		return monitors[monitorIndex];
	}

	std::vector< GLFWvidmode >
	Window::getMonitorModes (GLFWmonitor * monitor) noexcept
	{
		std::vector< GLFWvidmode > list;

		int count = 0;

		const GLFWvidmode * modes = glfwGetVideoModes(monitor, &count);

		if ( count > 0 && modes != nullptr )
		{
			list.reserve(count);

			for ( int index = 0; index < count; index++ )
			{
				list.emplace_back(modes[index]);
			}
		}

		return list;
	}

	void
	Window::framebufferSizeCallback (GLFWwindow * window, int width, int height) noexcept
	{
		if constexpr ( WindowEventsDebugEnabled )
		{
			TraceDebug{ClassId} << "Framebuffer size changed to " << width << "x" << height << " px." "\n";
		}

		auto * _this = static_cast< Window * >(glfwGetWindowUserPointer(window));

		if ( _this == nullptr )
		{
			return;
		}

		/* NOTE: Save the state. */
		_this->m_state.framebufferWidth = static_cast< uint32_t >(width);
		_this->m_state.framebufferHeight = static_cast< uint32_t >(height);

		TraceInfo{ClassId} << "The framebuffer has been resized to " << width << 'X' << height << " px.";

		_this->notify(OSNotifiesFramebufferResized);
	}

	void
	Window::windowRefreshCallback (GLFWwindow * window) noexcept
	{
		if constexpr ( WindowEventsDebugEnabled )
		{
			TraceDebug{ClassId} << "Window refresh request." "\n";
		}

		auto * _this = static_cast< Window * >(glfwGetWindowUserPointer(window));

		if ( _this == nullptr )
		{
			return;
		}

		_this->notify(OSRequestsToRefreshContent);
	}

	void
	Window::windowSizeCallback (GLFWwindow * window, int width, int height) noexcept
	{
		if constexpr ( WindowEventsDebugEnabled )
		{
			TraceDebug{ClassId} << "Window size changed to " << width << "x" << height << " pt." "\n";
		}

		auto * _this = static_cast< Window * >(glfwGetWindowUserPointer(window));

		if ( _this == nullptr )
		{
			return;
		}

		/* NOTE: Save the state. */
		_this->m_state.windowWidth = static_cast< uint32_t >(width);
		_this->m_state.windowHeight = static_cast< uint32_t >(height);

		/* NOTE: Refresh the window border sizes in case of, there is no event for this. */
		glfwGetWindowFrameSize(
			window,
			&_this->m_state.borderLeftSize,
			&_this->m_state.borderTopSize,
			&_this->m_state.borderRightSize,
			&_this->m_state.borderBottomSize
		);

		_this->notify(OSNotifiesWindowResized);
	}

	void
	Window::windowPositionCallback (GLFWwindow * window, int xPosition, int yPosition) noexcept
	{
		if constexpr ( WindowEventsDebugEnabled )
		{
			TraceDebug{ClassId} << "Window position changed to X:" << xPosition << ", Y:" << yPosition << "." "\n";
		}

		auto * _this = static_cast< Window * >(glfwGetWindowUserPointer(window));

		if ( _this == nullptr )
		{
			return;
		}

		/* NOTE: Save the state. */
		_this->m_state.windowXPosition = xPosition;
		_this->m_state.windowYPosition = yPosition;

		_this->notify(OSNotifiesWindowMovedTo);
	}

	void
	Window::monitorConfigurationChanged (GLFWmonitor * /*monitor*/, int event) noexcept
	{
		if constexpr ( WindowEventsDebugEnabled )
		{
			TraceDebug{ClassId} << "Monitor configuration changed (Monitor " << ( event == GLFW_CONNECTED ? "connected" : "disconnected") << ")." "\n";
		}

		switch ( event )
		{
			case GLFW_CONNECTED :
				Tracer::info(ClassId, "New monitor add to the configuration !");
				break;

			case GLFW_DISCONNECTED :
				Tracer::info(ClassId, "New monitor removed to the configuration !");
				break;

			default:
				break;
		}
	}

	void
	Window::windowCloseCallback (GLFWwindow * window) noexcept
	{
		if constexpr ( WindowEventsDebugEnabled )
		{
			TraceDebug{ClassId} << "Window close requested." "\n";
		}

		auto * _this = static_cast< Window * >(glfwGetWindowUserPointer(window));

		if ( _this == nullptr )
		{
			return;
		}

		/* We clear the close bit.
		 * The engine window itself is the running state. */
		glfwSetWindowShouldClose(window, GLFW_FALSE);

		/* Send the signal to the engine */
		_this->notify(OSRequestsToTerminate);
	}

	void
	Window::windowFocusCallback (GLFWwindow * window, int focused) noexcept
	{
		if constexpr ( WindowEventsDebugEnabled )
		{
			TraceDebug{ClassId} << "Window is " << ( focused == GLFW_TRUE ? "focused" : "blurred" ) << "." "\n";
		}

		auto * _this = static_cast< Window * >(glfwGetWindowUserPointer(window));

		if ( _this == nullptr )
		{
			return;
		}

		_this->notify(focused == GLFW_TRUE ? OSNotifiesWindowGetFocus : OSNotifiesWindowLostFocus);
	}

	void
	Window::windowIconifyCallback (GLFWwindow * window, int hidden) noexcept
	{
		if constexpr ( WindowEventsDebugEnabled )
		{
			TraceDebug{ClassId} << "Window is " << ( hidden == GLFW_TRUE ? "hidden" : "visible" ) << "." "\n";
		}

		auto * _this = static_cast< Window * >(glfwGetWindowUserPointer(window));

		if ( _this == nullptr )
		{
			return;
		}

		_this->notify(hidden == GLFW_TRUE ? OSNotifiesWindowHidden : OSNotifiesWindowVisible);
	}

	void
	Window::windowMaximizeCallback (GLFWwindow * window, int maximized) noexcept
	{
		if constexpr ( WindowEventsDebugEnabled )
		{
			TraceDebug{ClassId} << "Window is " << ( maximized == GLFW_TRUE ? "maximized" : "minimized" ) << "." "\n";
		}

		auto * _this = static_cast< Window * >(glfwGetWindowUserPointer(window));

		if ( _this == nullptr )
		{
			return;
		}

		_this->notify(maximized == GLFW_TRUE ? OSNotifiesWindowSizeMaximized : OSNotifiesWindowSizeMinimized);
	}

	void
	Window::windowContentScaleCallback (GLFWwindow * window, float xScale, float yScale) noexcept
	{
		if constexpr ( WindowEventsDebugEnabled )
		{
			TraceDebug{ClassId} << "Window scale changed to X:" << xScale << ", Y:" << yScale << "." "\n";
		}

		auto * _this = static_cast< Window * >(glfwGetWindowUserPointer(window));

		if ( _this == nullptr )
		{
			return;
		}

		/* NOTE: Save the state. */
		_this->m_state.contentXScale = xScale;
		_this->m_state.contentYScale = yScale;

		_this->notify(OSRequestsToRescaleContentBy);
	}
}
