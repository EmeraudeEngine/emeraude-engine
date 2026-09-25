/*
 * src/Window.linux.cpp
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

#include "Window.hpp"

/* Third-party inclusions. */
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#include "GLFW/glfw3native.h"
#include <vulkan/vulkan_wayland.h>
#include <dlfcn.h>
#include <poll.h>
/* NOTE: xcb.h must precede vulkan_xcb.h — vulkan_xcb.h uses xcb_* types. */
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
/* NOTE: Under linux, including X.h defines the MACRO "Success"
 * and enter in conflicts with Severity enum. */
#undef Success

/* Local inclusions. */
#include "PrimaryServices.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/Surface.hpp"
#include "Vulkan/Utility.hpp"

namespace
{
	/**
	 * @brief The reader of the Wayland connection on the rendering thread (Window::drainDisplayConnection()).
	 * @note libwayland is loaded at runtime by GLFW (never linked): its entry points are resolved on the module GLFW
	 * already loaded. The reader uses a PRIVATE event queue, always empty, so wl_display_prepare_read_queue() never
	 * refuses: the default queue belongs to GLFW (main thread) and is never dispatched here.
	 * Rendering thread only; released by Window::releaseNativeWindow() once that thread is joined.
	 */
	struct WaylandConnectionReader
	{
		using CreateQueue = struct wl_event_queue * (*) (struct wl_display *);
		using DestroyQueue = void (*) (struct wl_event_queue *);
		using PrepareReadQueue = int (*) (struct wl_display *, struct wl_event_queue *);
		using ReadEvents = int (*) (struct wl_display *);
		using CancelRead = void (*) (struct wl_display *);
		using GetFD = int (*) (struct wl_display *);

		CreateQueue createQueue{nullptr};
		DestroyQueue destroyQueue{nullptr};
		PrepareReadQueue prepareReadQueue{nullptr};
		ReadEvents readEvents{nullptr};
		CancelRead cancelRead{nullptr};
		GetFD getFD{nullptr};
		struct wl_event_queue * queue{nullptr};
		bool resolved{false};
		bool usable{false};
	};

	WaylandConnectionReader s_waylandReader{};
}

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
			VkXcbSurfaceCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
			createInfo.pNext = nullptr;
			createInfo.flags = 0; // VkXcbSurfaceCreateFlagsKHR
			createInfo.connection = nullptr; // xcb_connection_t *
			createInfo.window = glfwGetX11Window(m_handle.get()); // xcb_window_t

			result = vkCreateXcbSurfaceKHR(m_instance.handle(), &createInfo, nullptr, &surfaceHandle);
		}
		else
		{
			result = glfwCreateWindowSurface(m_instance.handle(), m_handle.get(), nullptr, &surfaceHandle);
		}

		if ( result != VK_SUCCESS )
		{
			TraceFatal{ClassId} << "Unable to create the Vulkan surface : " << Vulkan::vkResultToCString(result) << " !";

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

	void
	Window::applyTitleBarTheme () noexcept
	{
		// Nothing to do
	}

	bool
	Window::initializeNativeWindow () noexcept
	{
		return true;
	}

	void
	Window::releaseNativeWindow () noexcept
	{
		/* The rendering thread is joined: nobody reads through the private queue any more, and it must be gone
		 * before GLFW disconnects the display. */
		if ( s_waylandReader.queue != nullptr && s_waylandReader.destroyQueue != nullptr )
		{
			s_waylandReader.destroyQueue(s_waylandReader.queue);

			s_waylandReader.queue = nullptr;
		}
	}

	void
	Window::drainDisplayConnection () const noexcept
	{
		if ( glfwGetPlatform() != GLFW_PLATFORM_WAYLAND )
		{
			return;
		}

		auto * display = glfwGetWaylandDisplay();

		if ( display == nullptr )
		{
			return;
		}

		auto & reader = s_waylandReader;

		if ( !reader.resolved )
		{
			reader.resolved = true;

			/* RTLD_NOLOAD: the very module GLFW opened, never a second copy. */
			if ( auto * module = dlopen("libwayland-client.so.0", RTLD_NOW | RTLD_NOLOAD); module != nullptr )
			{
				reader.createQueue = reinterpret_cast< WaylandConnectionReader::CreateQueue >(dlsym(module, "wl_display_create_queue"));
				reader.destroyQueue = reinterpret_cast< WaylandConnectionReader::DestroyQueue >(dlsym(module, "wl_event_queue_destroy"));
				reader.prepareReadQueue = reinterpret_cast< WaylandConnectionReader::PrepareReadQueue >(dlsym(module, "wl_display_prepare_read_queue"));
				reader.readEvents = reinterpret_cast< WaylandConnectionReader::ReadEvents >(dlsym(module, "wl_display_read_events"));
				reader.cancelRead = reinterpret_cast< WaylandConnectionReader::CancelRead >(dlsym(module, "wl_display_cancel_read"));
				reader.getFD = reinterpret_cast< WaylandConnectionReader::GetFD >(dlsym(module, "wl_display_get_fd"));

				/* The module stays loaded by GLFW: this handle only drops the reference it took. */
				dlclose(module);
			}

			reader.usable = reader.createQueue != nullptr && reader.destroyQueue != nullptr && reader.prepareReadQueue != nullptr &&
				reader.readEvents != nullptr && reader.cancelRead != nullptr && reader.getFD != nullptr;

			if ( reader.usable )
			{
				reader.queue = reader.createQueue(display);
				reader.usable = reader.queue != nullptr;
			}

			if ( !reader.usable )
			{
				Tracer::warning(ClassId, "Unable to resolve libwayland-client: the display connection is read by the main thread only.");
			}
		}

		if ( !reader.usable )
		{
			return;
		}

		/* The private queue is always empty, so the intention to read is always granted. */
		if ( reader.prepareReadQueue(display, reader.queue) != 0 )
		{
			return;
		}

		/* Never blocks: read only what is already waiting. When the main thread also prepared a read (it polls),
		 * the same data wakes it, and the last reader performs the read for both. */
		pollfd descriptor{.fd = reader.getFD(display), .events = POLLIN, .revents = 0};

		if ( ::poll(&descriptor, 1, 0) > 0 && (descriptor.revents & POLLIN) != 0 )
		{
			static_cast< void >(reader.readEvents(display));
		}
		else
		{
			reader.cancelRead(display);
		}
	}
}
