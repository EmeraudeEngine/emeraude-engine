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

/* STL inclusions. */
#include <array>
#include <atomic>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <system_error>

/* Third-party inclusions. */
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_WAYLAND
#include "GLFW/glfw3native.h"
#include <vulkan/vulkan_wayland.h>
#include <dlfcn.h>
#include <poll.h>
#include <wayland-client-core.h>
/* NOTE: Under linux, including X.h defines the MACRO "Success"
 * and enter in conflicts with Severity enum. */
#undef Success

/* Local inclusions. */
#include "IO/IO.hpp"
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

	/**
	 * @brief Whether libwayland's log goes to the tracer (Window::initializeNativeWindow() → releaseNativeWindow()).
	 * @note A message logged outside that window is dropped: the handler stays installed in libwayland (nothing can
	 * restore its stderr default without writing to stderr), and the tracer may be gone. A function-local static: the
	 * handler is a C callback without a user pointer.
	 * @return std::atomic_bool &
	 */
	[[nodiscard]]
	std::atomic_bool &
	waylandLogToTracer () noexcept
	{
		static std::atomic_bool enabled{false};

		return enabled;
	}

	/**
	 * @brief libwayland's log handler (wl_log_set_handler_client()): the protocol error that kills a connection is
	 * written there ("error 3: …" with its interface and object), never into the journal before (production, a
	 * downstream application: 13 Linux machines stopped with exit 96 and nothing told why).
	 * @note Any thread that dispatches or reads the connection (the main thread, the rendering thread's reader).
	 * @param format A printf format.
	 * @param arguments Its arguments.
	 */
	[[gnu::format(printf, 1, 0)]]
	void
	waylandLogHandler (const char * format, va_list arguments)
	{
		if ( !waylandLogToTracer().load(std::memory_order_acquire) || format == nullptr )
		{
			return;
		}

		std::array< char, 1024 > buffer{};

		if ( std::vsnprintf(buffer.data(), buffer.size(), format, arguments) < 0 )
		{
			return;
		}

		std::string_view message{buffer.data()};

		/* NOTE: libwayland ends its lines with a newline. */
		while ( !message.empty() && (message.back() == '\n' || message.back() == '\r') )
		{
			message.remove_suffix(1);
		}

		EmEn::TraceError{"Wayland"} << "libwayland: " << message;
	}

	/**
	 * @brief Resolves a libwayland-client entry point on the module GLFW loaded (never linked by the engine).
	 * @tparam function_t The function pointer type.
	 * @param name The symbol.
	 * @return function_t The entry point, or nullptr.
	 */
	template< typename function_t >
	[[nodiscard]]
	function_t
	waylandEntryPoint (const char * name) noexcept
	{
		/* NOTE: POSIX guarantees a data pointer from dlsym() converts to a function pointer of the same size. */
		static_assert(sizeof(function_t) == sizeof(void *));

		function_t function{nullptr};

		/* NOTE: RTLD_NOLOAD: the very module GLFW opened (it stays loaded by GLFW, this handle only drops its reference). */
		if ( auto * module = dlopen("libwayland-client.so.0", RTLD_NOW | RTLD_NOLOAD); module != nullptr )
		{
			function = reinterpret_cast< function_t >(dlsym(module, name));

			dlclose(module);
		}

		return function;
	}
}

namespace EmEn
{
	using namespace Vulkan;

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
	Window::isDisplayConnectionLost () const noexcept
	{
		if ( glfwGetPlatform() != GLFW_PLATFORM_WAYLAND )
		{
			return false;
		}

		auto * display = glfwGetWaylandDisplay();

		if ( display == nullptr )
		{
			return false;
		}

		using GetError = decltype(&wl_display_get_error);

		static const auto getError = waylandEntryPoint< GetError >("wl_display_get_error");

		if ( getError == nullptr )
		{
			return false;
		}

		const auto error = getError(display);

		if ( error == 0 )
		{
			return false;
		}

		/* NOTE: Once per run, the error that killed the connection: a protocol error names its interface, object and
		 * code (wl_display_get_protocol_error()), anything else is an errno (the compositor went away: EPIPE...). */
		static std::atomic_flag described = ATOMIC_FLAG_INIT;

		if ( !described.test_and_set(std::memory_order_acq_rel) )
		{
			using GetProtocolError = decltype(&wl_display_get_protocol_error);

			static const auto getProtocolError = waylandEntryPoint< GetProtocolError >("wl_display_get_protocol_error");

			if ( error == EPROTO && getProtocolError != nullptr )
			{
				const struct wl_interface * interface = nullptr;
				uint32_t objectId = 0;

				const auto code = getProtocolError(display, &interface, &objectId);

				const auto * interfaceName = interface != nullptr ? interface->name : nullptr;

				TraceError{ClassId} << "The Wayland connection is lost: protocol error " << code << " on " << ( interfaceName != nullptr ? interfaceName : "an unknown interface" ) << '#' << objectId << '.';
			}
			else
			{
				TraceError{ClassId} << "The Wayland connection is lost: " << Base::IO::systemErrorText(std::error_code{error, std::generic_category()}) << " (errno " << error << ").";
			}
		}

		return true;
	}

	void
	Window::cancelQuitRequest () noexcept
	{
		/* NOTE: A termination signal has no answer. */
	}

	bool
	Window::initializeNativeWindow () noexcept
	{
		/* NOTE: libwayland's log (the protocol error that kills a connection) goes to the tracer while the window lives. */
		if ( glfwGetPlatform() == GLFW_PLATFORM_WAYLAND )
		{
			using SetLogHandler = decltype(&wl_log_set_handler_client);

			if ( const auto setLogHandler = waylandEntryPoint< SetLogHandler >("wl_log_set_handler_client"); setLogHandler != nullptr )
			{
				waylandLogToTracer().store(true, std::memory_order_release);

				setLogHandler(&waylandLogHandler);
			}
			else
			{
				Tracer::warning(ClassId, "libwayland's log cannot be routed to the journal (wl_log_set_handler_client() not found).");
			}
		}

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

		/* The tracer stops receiving libwayland's log (the handler stays installed, inert). */
		waylandLogToTracer().store(false, std::memory_order_release);
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
