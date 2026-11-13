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
#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <limits>
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
	 * @brief The libwayland-client entry points the window uses beside GLFW, and the private event queue of the rendering
	 * thread's reader (Window::drainDisplayConnection()).
	 * @note libwayland is loaded at runtime by GLFW (never linked): the entry points are resolved on the module GLFW
	 * already loaded, once, by Window::initializeNativeWindow() on the main thread before the logic and rendering threads
	 * exist, and released by Window::releaseNativeWindow() once they are joined. In between, read-only from any thread.
	 * The private queue is always empty, so wl_display_prepare_read_queue() never refuses: the default queue belongs to
	 * GLFW (main thread) and is never dispatched by the reader.
	 */
	struct WaylandClient
	{
		using CreateQueue = struct wl_event_queue * (*) (struct wl_display *);
		using DestroyQueue = void (*) (struct wl_event_queue *);
		using PrepareRead = int (*) (struct wl_display *);
		using PrepareReadQueue = int (*) (struct wl_display *, struct wl_event_queue *);
		using ReadEvents = int (*) (struct wl_display *);
		using CancelRead = void (*) (struct wl_display *);
		using GetFD = int (*) (struct wl_display *);

		CreateQueue createQueue{nullptr};
		DestroyQueue destroyQueue{nullptr};
		PrepareRead prepareRead{nullptr};
		PrepareReadQueue prepareReadQueue{nullptr};
		ReadEvents readEvents{nullptr};
		CancelRead cancelRead{nullptr};
		GetFD getFD{nullptr};
		struct wl_event_queue * readerQueue{nullptr};
		bool usable{false};
	};

	/**
	 * @brief The process's libwayland-client entry points (WaylandClient): written by Window::initializeNativeWindow()
	 * and Window::releaseNativeWindow() on the main thread, read by the rendering thread in between.
	 * @return WaylandClient &
	 */
	[[nodiscard]]
	WaylandClient &
	waylandClient () noexcept
	{
		static WaylandClient client{};

		return client;
	}

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
		if ( glfwGetPlatform() != GLFW_PLATFORM_WAYLAND )
		{
			return true;
		}

		/* NOTE: libwayland's log (the protocol error that kills a connection) goes to the tracer while the window lives. */
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

		/* NOTE: The entry points of the rendering thread's reader (drainDisplayConnection()) and of the main thread's wait
		 * (waitEvents()), resolved once here: no other thread exists yet. */
		auto & client = waylandClient();

		client.createQueue = waylandEntryPoint< WaylandClient::CreateQueue >("wl_display_create_queue");
		client.destroyQueue = waylandEntryPoint< WaylandClient::DestroyQueue >("wl_event_queue_destroy");
		client.prepareRead = waylandEntryPoint< WaylandClient::PrepareRead >("wl_display_prepare_read");
		client.prepareReadQueue = waylandEntryPoint< WaylandClient::PrepareReadQueue >("wl_display_prepare_read_queue");
		client.readEvents = waylandEntryPoint< WaylandClient::ReadEvents >("wl_display_read_events");
		client.cancelRead = waylandEntryPoint< WaylandClient::CancelRead >("wl_display_cancel_read");
		client.getFD = waylandEntryPoint< WaylandClient::GetFD >("wl_display_get_fd");

		client.usable =
			client.createQueue != nullptr && client.destroyQueue != nullptr && client.prepareRead != nullptr &&
			client.prepareReadQueue != nullptr && client.readEvents != nullptr && client.cancelRead != nullptr &&
			client.getFD != nullptr;

		if ( client.usable )
		{
			if ( auto * display = glfwGetWaylandDisplay(); display != nullptr )
			{
				client.readerQueue = client.createQueue(display);
			}

			client.usable = client.readerQueue != nullptr;
		}

		if ( !client.usable )
		{
			Tracer::warning(ClassId, "Unable to resolve libwayland-client: the display connection is read by the main thread only, and its wait holds the native-surface lock.");
		}

		return true;
	}

	void
	Window::releaseNativeWindow () noexcept
	{
		auto & client = waylandClient();

		/* The rendering thread is joined: nobody reads through the private queue any more, and it must be gone
		 * before GLFW disconnects the display. */
		if ( client.readerQueue != nullptr && client.destroyQueue != nullptr )
		{
			client.destroyQueue(client.readerQueue);

			client.readerQueue = nullptr;
		}

		client.usable = false;

		/* The tracer stops receiving libwayland's log (the handler stays installed, inert). */
		waylandLogToTracer().store(false, std::memory_order_release);
	}

	void
	Window::drainDisplayConnection () const noexcept
	{
		const auto & client = waylandClient();

		if ( !client.usable )
		{
			return;
		}

		auto * display = glfwGetWaylandDisplay();

		if ( display == nullptr )
		{
			return;
		}

		/* The private queue is always empty, so the intention to read is always granted. */
		if ( client.prepareReadQueue(display, client.readerQueue) != 0 )
		{
			return;
		}

		/* Never blocks: read only what is already waiting. When the main thread also prepared a read (it polls),
		 * the same data wakes it, and the last reader performs the read for both. */
		pollfd descriptor{.fd = client.getFD(display), .events = POLLIN, .revents = 0};

		if ( ::poll(&descriptor, 1, 0) > 0 && (descriptor.revents & POLLIN) != 0 )
		{
			static_cast< void >(client.readEvents(display));
		}
		else
		{
			client.cancelRead(display);
		}
	}

	void
	Window::waitEvents (double timeoutSeconds) const noexcept
	{
		const auto & client = waylandClient();

		auto * display = m_nativeSurfaceSerialized && client.usable ? glfwGetWaylandDisplay() : nullptr;

		if ( display == nullptr )
		{
			/* NOTE: X11, a window-less run, or libwayland out of reach: GLFW's own wait, under the lock when it matters
			 * (the lock owns nothing outside Wayland). */
			const auto surfaceLock = this->lockNativeSurface();

			if ( timeoutSeconds > 0.0 )
			{
				glfwWaitEventsTimeout(timeoutSeconds);
			}
			else
			{
				glfwWaitEvents();
			}

			return;
		}

		/* 1. What is pending is dispatched under the lock, and GLFW flushes the requests of its handlers. */
		this->pumpEvents();

		/* 2. Register as a reader of the default queue: refused while it holds events (the rendering thread's reader may
		 * have filed some since the pump above), dispatched again then. Bounded: a queue that never empties means the
		 * main loop must cycle anyway. */
		constexpr uint32_t MaxRegistrationAttempts{8};

		bool registered = false;

		for ( uint32_t attempt = 0; attempt < MaxRegistrationAttempts && !registered; ++attempt )
		{
			registered = client.prepareRead(display) == 0;

			if ( !registered )
			{
				this->pumpEvents();
			}
		}

		if ( !registered )
		{
			return;
		}

		/* 3. The wait itself, WITHOUT the lock: the rendering thread presents meanwhile. GLFW's timers are not polled
		 * here (key repeat, cursor animation), so an indefinite wait is bounded and they are served at the next pass. */
		const auto timeoutMilliseconds = timeoutSeconds > 0.0 ?
			static_cast< int >(std::clamp(std::ceil(timeoutSeconds * 1000.0), 1.0, static_cast< double >(std::numeric_limits< int >::max()))) :
			WaylandIndefiniteWaitMilliseconds;

		pollfd descriptor{.fd = client.getFD(display), .events = POLLIN, .revents = 0};

		const auto ready = ::poll(&descriptor, 1, timeoutMilliseconds);

		/* 4. The events read join their queues (GLFW's, the driver's); a timeout, a signal or a dead socket gives the
		 * read up. */
		if ( ready > 0 && (descriptor.revents & POLLIN) != 0 )
		{
			static_cast< void >(client.readEvents(display));
		}
		else
		{
			client.cancelRead(display);
		}

		/* 5. Dispatched under the lock. A dead connection is seen by GLFW here (its flush fails) and becomes the close
		 * request Window::windowCloseCallback() turns into the display-loss stop. */
		this->pumpEvents();
	}
}
