/*
 * src/Window.windows.cpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

#if IS_WINDOWS
#include "Window.hpp"

/* Third-party inclusions. */
#define GLFW_EXPOSE_NATIVE_WIN32
#include "GLFW/glfw3native.h"

#include <vulkan/vulkan_win32.h>
#include <shobjidl.h>

/* Local inclusions. */
#include "Vulkan/Utility.hpp"
#include "Vulkan/Instance.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	using namespace Vulkan;

	bool
	Window::createSurface (bool useNativeCode) noexcept
	{
		VkResult result = VK_SUCCESS;

		VkSurfaceKHR surfaceHandle{VK_NULL_HANDLE};

		if ( useNativeCode )
		{
			VkWin32SurfaceCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.pNext = nullptr;
			createInfo.hwnd = glfwGetWin32Window(m_handle.get());
			createInfo.hinstance = GetModuleHandle(nullptr);

			std::cout << "[DEBUG-SURFACE-CREATE] 1/2 : before vkCreateWin32SurfaceKHR()" << std::endl;

			result = vkCreateWin32SurfaceKHR(m_instance.handle(), &createInfo, nullptr, &surfaceHandle);

			std::cout << "[DEBUG-SURFACE-CREATE] 2/2 : after vkCreateWin32SurfaceKHR()" << std::endl;
		}
		else
		{
			result = glfwCreateWindowSurface(m_instance.handle(), m_handle.get(), nullptr, &surfaceHandle);
		}

		if ( result != VK_SUCCESS )
		{
			TraceFatal{ClassId} << "Unable to create the Vulkan surface : " << vkResultToCString(result) << " !";

			return false;
		}

		m_surface = std::make_unique< Surface >(m_instance, surfaceHandle);
		m_surface->setIdentifier(ClassId, "OSVideoFramebuffer", "Surface");

		return true;
	}

	void
	Window::destroySurface () noexcept
	{
		if ( m_surface != nullptr )
		{
			Tracer::debug(ClassId, "Destroying Vulkan surface...");

			m_surface.reset();
		}
	}

	bool
	Window::recreateSurface (bool useNativeCode) noexcept
	{
		Tracer::debug(ClassId, "Recreating Vulkan surface...");

		this->destroySurface();

		return this->createSurface(useNativeCode);
	}

	void
	Window::disableTitleBar () noexcept
	{

	}

	bool
	Window::initializeNativeWindow () noexcept
	{
		if ( CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE) < 0 )
		{
			return false;
		}

		return true;
	}

	void
	Window::releaseNativeWindow () noexcept
	{
		CoUninitialize();
	}

	HWND
	Window::getWin32Window () const noexcept
	{
		return glfwGetWin32Window(m_handle.get());
	}

	void
	Window::setupWindowsResizeHandling () noexcept
	{
		HWND hwnd = glfwGetWin32Window(m_handle.get());

		if ( hwnd == nullptr )
		{
			Tracer::warning(ClassId, "Unable to get Win32 window handle for resize handling setup.");
			return;
		}

		/* Store the Window pointer in the HWND user data for retrieval in the WndProc. */
		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast< LONG_PTR >(this));

		/* Subclass the window to intercept WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE messages. */
		m_originalWndProc = reinterpret_cast< WNDPROC >(SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast< LONG_PTR >(windowProc)));

		if ( m_originalWndProc == nullptr )
		{
			Tracer::warning(ClassId, "Unable to subclass Win32 window for resize handling.");
		}
		else
		{
			Tracer::info(ClassId, "Windows resize pause handling enabled.");
		}
	}

	LRESULT CALLBACK
	Window::windowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		auto * window = reinterpret_cast< Window * >(GetWindowLongPtr(hWnd, GWLP_USERDATA));

		if ( window != nullptr )
		{
			switch ( uMsg )
			{
				case WM_ENTERSIZEMOVE :
					/* User started dragging/resizing the window - pause rendering. */
					window->m_isUserResizing = true;
					break;

				case WM_EXITSIZEMOVE :
					/* User finished dragging/resizing the window - resume rendering. */
					window->m_isUserResizing = false;

					/* Notify that the framebuffer needs to be resized now that the user finished resizing. */
					window->notify(OSNotifiesFramebufferResized);
					break;

				default :
					break;
			}
		}

		/* Call the original GLFW window procedure. */
		if ( window != nullptr && window->m_originalWndProc != nullptr )
		{
			return CallWindowProc(window->m_originalWndProc, hWnd, uMsg, wParam, lParam);
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}

#endif
