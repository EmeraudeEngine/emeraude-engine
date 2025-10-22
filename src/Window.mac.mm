/*
 * src/Window.mac.cpp
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

#if IS_MACOS
#include "Window.hpp"

/* Third-party inclusions. */
#define GLFW_EXPOSE_NATIVE_COCOA
#include "GLFW/glfw3native.h"

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
			// FIXME: Don't know what to write for now ...
			return false;
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
	Window::disableTitleBar () noexcept
	{

	}

	bool
	Window::initializeNativeWindow () noexcept
	{
		return true;
	}

	void
	Window::releaseNativeWindow () noexcept
	{

	}

	id
	Window::getCocoaWindow () const noexcept
	{
		return glfwGetCocoaWindow(m_handle.get());
	}

	id
	Window::getCocoaView () const noexcept
	{
		return glfwGetCocoaView(m_handle.get());
	}
}

#endif
