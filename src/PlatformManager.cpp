/*
 * src/PlatformManager.cpp
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

#include "PlatformManager.hpp"

/* STL inclusions. */
#include <cstddef>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"

/* Local inclusions. */
#include "Arguments.hpp"
#include "ServiceInterface.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	using namespace EmEn::Libs;

	const size_t PlatformManager::ClassUID{getClassUID(ClassId)};

	PlatformManager::PlatformManager (PrimaryServices & primaryServices) noexcept
		: ServiceInterface(ClassId),
		m_primaryServices(primaryServices)
	{

	}

	size_t
	PlatformManager::classUID () const noexcept
	{
		return ClassUID;
	}

	bool
	PlatformManager::is (size_t classUID) const noexcept
	{
		return classUID == ClassUID;
	}

	bool
	PlatformManager::usable () const noexcept
	{
		return m_flags[ServiceInitialized];
	}

	bool
	PlatformManager::onInitialize () noexcept
	{
		auto & settings = m_primaryServices.settings();
		
		m_flags[ShowInformation] = settings.get< bool >(GLFWShowInformationKey, DefaultGLFWShowInformation);

		glfwSetErrorCallback(Tracer::traceGLFW);

		/* GLFW_JOYSTICK_HAT_BUTTONS specifies whether to also expose joystick hats as buttons,
		 * for compatibility with earlier versions of GLFW that did not have glfwGetJoystickHats.
		 * Possible values are GLFW_TRUE and GLFW_FALSE. */
		glfwInitHint(GLFW_JOYSTICK_HAT_BUTTONS, GLFW_TRUE);

		if constexpr ( IsMacOS)
		{
			/* GLFW_COCOA_CHDIR_RESOURCES specifies whether to set the current directory to the application
			 * to the Contents/Resources subdirectory of the application's bundle, if present. */
			glfwInitHint(GLFW_COCOA_CHDIR_RESOURCES, GLFW_TRUE);
			/* GLFW_COCOA_MENUBAR specifies whether to create a basic menu bar, either from a nib or manually,
			 * when the first window is created, which is when AppKit is initialized. */
			glfwInitHint(GLFW_COCOA_MENUBAR, GLFW_TRUE);

			glfwInitVulkanLoader(nullptr);
		}

		if constexpr ( IsLinux )
		{
			const auto enableWaylandLibDecor = settings.get< bool >(GLFWWaylandEnableLibDecorKey, DefaultGLFWWaylandEnableLibDecor);
			const auto useX11XCB = settings.get< bool >(GLFWX11UseXCBInsteadOfXLibKey, DefaultGLFWX11UseXCBInsteadOfXLib);
			const auto usePlatform = settings.get< std::string >(GLFWUsePlatformKey, DefaultGLFWUsePlatform);

			int platform = GLFW_ANY_PLATFORM;
			
			if ( usePlatform == "Wayland" )
			{
				platform = GLFW_PLATFORM_WAYLAND;
			}
			else if ( usePlatform == "X11" )
			{
				platform = GLFW_PLATFORM_X11;
			}

			/* GLFW_PLATFORM specifies the platform to use for windowing and input. Possible values are GLFW_ANY_PLATFORM, GLFW_PLATFORM_WIN32, 
			 * GLFW_PLATFORM_COCOA, GLFW_PLATFORM_WAYLAND, GLFW_PLATFORM_X11 and GLFW_PLATFORM_NULL. The default value is GLFW_ANY_PLATFORM, 
			 * which will choose any platform the library includes support for except for the Null backend. */
			glfwInitHint(GLFW_PLATFORM, platform);

			/* GLFW_WAYLAND_LIBDECOR specifies whether to use libdecor for window decorations where available.
			 * Possible values are GLFW_WAYLAND_PREFER_LIBDECOR and GLFW_WAYLAND_DISABLE_LIBDECOR.
			 * This is ignored on other platforms. */
			glfwInitHint(GLFW_WAYLAND_LIBDECOR, enableWaylandLibDecor ? GLFW_WAYLAND_PREFER_LIBDECOR : GLFW_WAYLAND_DISABLE_LIBDECOR);

			/* GLFW_X11_XCB_VULKAN_SURFACE specifies whether to prefer the VK_KHR_xcb_surface extension for creating
			 * Vulkan surfaces, or whether to use the VK_KHR_xlib_surface extension. Possible values are GLFW_TRUE and GLFW_FALSE.
			 * This is ignored on other platforms. */
			glfwInitHint(GLFW_X11_XCB_VULKAN_SURFACE, useX11XCB);
		}

		if ( glfwInit() == GLFW_FALSE )
		{
			Tracer::fatal(ClassId, "Unable to initialize GLFW. Aborting ...");

			return false;
		}

		/* Checks for Vulkan API support. */
		if ( glfwVulkanSupported() == GLFW_FALSE )
		{
			Tracer::fatal(ClassId, "Vulkan API is not supported by this system ! Aborting ...");

			return false;
		}

		if ( m_flags[ShowInformation] )
		{
			int glfwMajor = 0;
			int glfwMin = 0;
			int glfwRev = 0;

			glfwGetVersion(&glfwMajor, &glfwMin, &glfwRev);

			TraceSuccess{ClassId} << "GLFW version : " << glfwMajor << '.' << glfwMin << '.' << glfwRev << " initialized !";
		}

		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	PlatformManager::onTerminate () noexcept
	{
		m_flags[ServiceInitialized] = false;

		glfwTerminate();

		/* Removing callback. */
		glfwSetErrorCallback(nullptr);

		return true;
	}

	bool
	PlatformManager::isLinux () noexcept
	{
		const auto token = glfwGetPlatform();

		return token == GLFW_PLATFORM_WAYLAND || token == GLFW_PLATFORM_X11;
	}

	bool
	PlatformManager::isMacOS () noexcept
	{
		return glfwGetPlatform() == GLFW_PLATFORM_COCOA;
	}

	bool
	PlatformManager::isWindows () noexcept
	{
		return glfwGetPlatform() == GLFW_PLATFORM_WIN32;
	}

	bool
	PlatformManager::isUsingX11 () noexcept
	{
		return glfwGetPlatform() == GLFW_PLATFORM_X11;
	}

	bool
	PlatformManager::isUsingWayland () noexcept
	{
		return glfwGetPlatform() == GLFW_PLATFORM_WAYLAND;
	}
}
