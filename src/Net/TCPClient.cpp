/*
 * src/Net/TCPClient.cpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "TCPClient.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <chrono>
#include <limits>
#include <utility>

/* Third-party — Asio is used only inside connect() for DNS resolution and a
 * timed async_connect. Once the connection is up, the kernel handle is
 * detached from Asio and the runtime I/O path bypasses Asio entirely. */
#include "Libs/Network/asio_throw_exception.hpp"
#include "asio.hpp"

/* Platform-specific socket headers. */
#ifdef _WIN32
	#ifndef NOMINMAX
	#define NOMINMAX
	#endif

	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif

	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <mstcpip.h>
#else
	#include <arpa/inet.h>
	#include <fcntl.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <sys/socket.h>
	#include <sys/time.h>
	#include <unistd.h>

	#include <cerrno>
#endif

namespace
{
#ifdef _WIN32
	using native_socket_t = SOCKET;
	using io_size_t       = int;
	constexpr native_socket_t NativeInvalid{INVALID_SOCKET};
	constexpr int             NativeShutdownBoth{SD_BOTH};

	inline int closeNative (native_socket_t s) noexcept { return ::closesocket(s); }
	inline int lastNativeError () noexcept              { return ::WSAGetLastError(); }
	inline std::error_code lastNativeCode () noexcept   { return { lastNativeError(), std::system_category() }; }
#else
	using native_socket_t = int;
	using io_size_t       = size_t;
	constexpr native_socket_t NativeInvalid{-1};
	constexpr int             NativeShutdownBoth{SHUT_RDWR};

	inline int closeNative (native_socket_t s) noexcept { return ::close(s); }
	inline int lastNativeError () noexcept              { return errno; }
	inline std::error_code lastNativeCode () noexcept   { return { errno, std::generic_category() }; }
#endif

	inline native_socket_t
	toNative (EmEn::Net::TCPClient::native_handle_type h) noexcept
	{
		return static_cast< native_socket_t >(h);
	}

	inline EmEn::Net::TCPClient::native_handle_type
	fromNative (native_socket_t s) noexcept
	{
		return static_cast< EmEn::Net::TCPClient::native_handle_type >(s);
	}

	/* Force the kernel handle into blocking mode. Asio sets it non-blocking
	 * to plug into the reactor / IOCP; we want plain blocking syscalls. */
	inline bool
	setBlocking (native_socket_t handle) noexcept
	{
#ifdef _WIN32
		u_long mode = 0;  /* 0 = blocking */

		return ::ioctlsocket(handle, FIONBIO, &mode) == 0;
#else
		const int flags = ::fcntl(handle, F_GETFL, 0);

		if ( flags < 0 )
		{
			return false;
		}

		return ::fcntl(handle, F_SETFL, flags & ~O_NONBLOCK) == 0;
#endif
	}

	/* Suppress SIGPIPE on the socket. Linux uses MSG_NOSIGNAL per send call
	 * (see writeNative()); macOS uses SO_NOSIGPIPE per socket. Windows does
	 * not have SIGPIPE. */
	inline void
	suppressSigPipe ([[maybe_unused]] native_socket_t handle) noexcept
	{
#if defined(__APPLE__) && defined(SO_NOSIGPIPE)
		const int yes = 1;
		static_cast< void >(::setsockopt(handle, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof(yes)));
#endif
	}

	/* Single send call. Returns the number of bytes written, or -1 with
	 * lastNativeError() set on failure. */
	inline io_size_t
	writeNative (native_socket_t handle, const char * data, size_t length) noexcept
	{
#ifdef _WIN32
		const int chunk = static_cast< int >(std::min< size_t >(length,
			static_cast< size_t >(std::numeric_limits< int >::max())));

		return ::send(handle, data, chunk, 0);
#elif defined(MSG_NOSIGNAL)
		return ::send(handle, data, length, MSG_NOSIGNAL);
#else
		/* macOS path — SIGPIPE was suppressed via SO_NOSIGPIPE at socket setup. */
		return ::send(handle, data, length, 0);
#endif
	}

	inline io_size_t
	readNative (native_socket_t handle, char * data, size_t length) noexcept
	{
#ifdef _WIN32
		const int chunk = static_cast< int >(std::min< size_t >(length,
			static_cast< size_t >(std::numeric_limits< int >::max())));

		return ::recv(handle, data, chunk, 0);
#else
		return ::recv(handle, data, length, 0);
#endif
	}

	inline bool
	applyTimeout (native_socket_t handle, int optname, uint32_t timeoutMs) noexcept
	{
#ifdef _WIN32
		const DWORD t = static_cast< DWORD >(timeoutMs);

		return ::setsockopt(handle, SOL_SOCKET, optname,
			reinterpret_cast< const char * >(&t), sizeof(t)) == 0;
#else
		struct timeval tv{};
		tv.tv_sec  = static_cast< time_t >(timeoutMs / 1000);
		tv.tv_usec = static_cast< suseconds_t >((timeoutMs % 1000) * 1000);

		return ::setsockopt(handle, SOL_SOCKET, optname, &tv, sizeof(tv)) == 0;
#endif
	}

	/* Distinguish a true timeout from a real I/O error after recv() returns -1. */
	inline bool
	isTimeoutError (int code) noexcept
	{
#ifdef _WIN32
		return code == WSAETIMEDOUT || code == WSAEWOULDBLOCK;
#else
		return code == EAGAIN || code == EWOULDBLOCK
		#ifdef ETIMEDOUT
			|| code == ETIMEDOUT
		#endif
		;
#endif
	}
}

namespace EmEn::Net
{
	/* ---- Lifecycle ---- */

	TCPClient::TCPClient () noexcept
		: m_handleMutex{std::make_unique< std::shared_mutex >()}
	{

	}

	TCPClient::TCPClient (native_handle_type handle) noexcept
		: m_handleMutex{std::make_unique< std::shared_mutex >()},
		m_handle{handle}
	{
		/* The handle comes from TCPServer::accept(), which extracts it from a
		 * fully-connected asio::ip::tcp::socket via release(). Asio leaves
		 * the kernel handle in non-blocking mode and without SIGPIPE
		 * suppression — restore the same baseline the public connect() path
		 * produces, so the rest of the client API behaves uniformly. */
		if ( handle != InvalidHandle )
		{
			const auto native = toNative(handle);

			if ( !setBlocking(native) )
			{
				m_lastError = lastNativeCode();
				static_cast< void >(closeNative(native));
				m_handle = InvalidHandle;

				return;
			}

			suppressSigPipe(native);
		}
	}

	TCPClient::~TCPClient () noexcept
	{
		this->close();
	}

	TCPClient::TCPClient (TCPClient && other) noexcept
		: m_handleMutex{std::move(other.m_handleMutex)},
		m_handle{std::exchange(other.m_handle, InvalidHandle)},
		m_lastError{std::exchange(other.m_lastError, std::error_code{})}
	{

	}

	TCPClient &
	TCPClient::operator= (TCPClient && other) noexcept
	{
		if ( this != &other )
		{
			this->close();

			m_handleMutex = std::move(other.m_handleMutex);
			m_handle = std::exchange(other.m_handle, InvalidHandle);
			m_lastError = std::exchange(other.m_lastError, std::error_code{});
		}

		return *this;
	}

	/* ---- Connection ---- */

	bool
	TCPClient::connect (const std::string & host, uint16_t port, uint32_t timeoutMs) noexcept
	{
		/* Drop any previous connection first. */
		this->close();

		/* Asio scaffolding lives on the stack of this function — destroyed
		 * on every exit path. We only use it for resolve() and the timed
		 * async_connect; the connected handle is detached at the end. */
		auto ioContext = std::make_unique< asio::io_context >();
		auto socket    = std::make_unique< asio::ip::tcp::socket >(*ioContext);

		asio::ip::tcp::resolver resolver{*ioContext};
		asio::error_code resolveEc;
		const auto results = resolver.resolve(host, std::to_string(port), resolveEc);

		if ( resolveEc )
		{
			m_lastError = resolveEc;

			return false;
		}

		/* Async connect with a deadline. would_block is overwritten by the
		 * completion handler whenever it eventually fires. */
		asio::error_code connectEc = asio::error::would_block;

		asio::async_connect(*socket, results,
			[&connectEc] (const asio::error_code & ec, const asio::ip::tcp::endpoint &) noexcept {
				connectEc = ec;
			}
		);

		ioContext->restart();
		static_cast< void >(ioContext->run_for(std::chrono::milliseconds(timeoutMs)));

		if ( connectEc == asio::error::would_block )
		{
			/* Deadline expired before any endpoint accepted — cancel the
			 * pending op and drain. Safe here because no other I/O is in
			 * flight on a not-yet-connected socket. */
			asio::error_code cancelEc;
			socket->cancel(cancelEc);
			static_cast< void >(ioContext->run());

			m_lastError = asio::error::timed_out;

			return false;
		}

		if ( connectEc )
		{
			m_lastError = connectEc;

			return false;
		}

		/* Detach the kernel handle from Asio. After release() the asio::socket
		 * no longer owns it, so the unique_ptr destruction at end-of-function
		 * will not closesocket() under our feet. */
		asio::error_code releaseEc;
		const auto nativeHandle = static_cast< native_socket_t >(socket->release(releaseEc));

		if ( releaseEc || nativeHandle == NativeInvalid )
		{
			m_lastError = releaseEc;

			return false;
		}

		/* Asio leaves the handle in non-blocking mode (it integrates with
		 * the reactor / IOCP). We want blocking syscalls. */
		if ( !setBlocking(nativeHandle) )
		{
			const auto err = lastNativeCode();
			static_cast< void >(closeNative(nativeHandle));
			m_lastError = err;

			return false;
		}

		suppressSigPipe(nativeHandle);

		/* Publish the handle under the exclusive lock so no concurrent reader
		 * can observe a half-initialised state. */
		{
			const std::unique_lock< std::shared_mutex > writer{*m_handleMutex};
			m_handle = fromNative(nativeHandle);
		}

		return true;
	}

	void
	TCPClient::close () noexcept
	{
		if ( m_handleMutex == nullptr )
		{
			return;
		}

		/* Phase 1: under a shared lock, shutdown both directions to wake up
		 * any thread blocked in ::recv() or ::send() on this handle. The
		 * kernel returns the blocked recv with 0 (EOF) and the blocked send
		 * with an error. The handle stays valid here — invalidating it now
		 * would race with the very threads we are trying to wake up. */
		{
			const std::shared_lock< std::shared_mutex > readers{*m_handleMutex};

			if ( m_handle != InvalidHandle )
			{
				static_cast< void >(::shutdown(toNative(m_handle), NativeShutdownBoth));
			}
		}

		/* Phase 2: acquire the exclusive lock. This blocks until every
		 * in-flight send/receive has released its shared lock. Once we hold
		 * the writer lock, no other thread can be inside a syscall on the
		 * handle — closing it is now safe and there is no risk of the
		 * kernel reusing the freed handle value behind our back. */
		const std::unique_lock< std::shared_mutex > writer{*m_handleMutex};

		if ( m_handle != InvalidHandle )
		{
			static_cast< void >(closeNative(toNative(m_handle)));
			m_handle = InvalidHandle;
		}
	}

	bool
	TCPClient::isConnected () const noexcept
	{
		if ( m_handleMutex == nullptr )
		{
			return false;
		}

		const std::shared_lock< std::shared_mutex > readers{*m_handleMutex};

		return m_handle != InvalidHandle;
	}

	/* ---- Send / Receive ---- */

	int
	TCPClient::send (const void * data, size_t length) noexcept
	{
		if ( length == 0 )
		{
			return 0;
		}

		if ( m_handleMutex == nullptr )
		{
			return -1;
		}

		const std::shared_lock< std::shared_mutex > readers{*m_handleMutex};

		if ( m_handle == InvalidHandle )
		{
			return -1;
		}

		const auto handle  = toNative(m_handle);
		const auto * cursor = static_cast< const char * >(data);
		size_t remaining   = length;

		while ( remaining > 0 )
		{
			const auto sent = writeNative(handle, cursor, remaining);

			if ( sent < 0 )
			{
				const auto err = lastNativeCode();

				/* If SO_SNDTIMEO fired or the send buffer is full, treat as
				 * a partial write rather than an outright error so the
				 * caller can resume later. */
				if ( isTimeoutError(lastNativeError()) )
				{
					const auto bytesSent = length - remaining;

					if ( bytesSent > 0 )
					{
						return static_cast< int >(bytesSent);
					}

					/* No progress at all — surface the timeout. */
					m_lastError = err;

					return -1;
				}

				m_lastError = err;

				const auto bytesSent = length - remaining;

				return bytesSent > 0 ? static_cast< int >(bytesSent) : -1;
			}

			if ( sent == 0 )
			{
				/* Peer closed — treat as truncated write. */
				break;
			}

			cursor    += static_cast< size_t >(sent);
			remaining -= static_cast< size_t >(sent);
		}

		return static_cast< int >(length - remaining);
	}

	int
	TCPClient::send (const std::string & data) noexcept
	{
		return this->send(data.data(), data.size());
	}

	int
	TCPClient::receive (void * buffer, size_t maxLength, uint32_t timeoutMs) noexcept
	{
		if ( maxLength == 0 )
		{
			return 0;
		}

		if ( m_handleMutex == nullptr )
		{
			return -1;
		}

		const std::shared_lock< std::shared_mutex > readers{*m_handleMutex};

		if ( m_handle == InvalidHandle )
		{
			return -1;
		}

		const auto handle = toNative(m_handle);

		/* Save SO_RCVTIMEO, arm the per-call timeout, restore on exit. The
		 * save/restore preserves any prior setRecvTimeout() configuration.
		 * Concurrent receive() calls on the same instance would race on
		 * this state — but the canonical use case is a single recv-poll
		 * thread, so we don't pay for serialization here. */
		bool restoreTimeout = false;
#ifdef _WIN32
		DWORD savedTimeoutWin = 0;
		int   savedLenWin     = sizeof(savedTimeoutWin);

		if ( ::getsockopt(handle, SOL_SOCKET, SO_RCVTIMEO,
				reinterpret_cast< char * >(&savedTimeoutWin), &savedLenWin) == 0 )
		{
			restoreTimeout = true;
		}
#else
		struct timeval savedTv{};
		socklen_t      savedLen = sizeof(savedTv);

		if ( ::getsockopt(handle, SOL_SOCKET, SO_RCVTIMEO, &savedTv, &savedLen) == 0 )
		{
			restoreTimeout = true;
		}
#endif

		static_cast< void >(applyTimeout(handle, SO_RCVTIMEO, timeoutMs));

		const auto n = readNative(handle, static_cast< char * >(buffer), maxLength);
		const auto rawErr = (n < 0) ? lastNativeError() : 0;

		if ( restoreTimeout )
		{
#ifdef _WIN32
			static_cast< void >(::setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO,
				reinterpret_cast< const char * >(&savedTimeoutWin), sizeof(savedTimeoutWin)));
#else
			static_cast< void >(::setsockopt(handle, SOL_SOCKET, SO_RCVTIMEO,
				&savedTv, sizeof(savedTv)));
#endif
		}

		if ( n > 0 )
		{
			return static_cast< int >(n);
		}

		if ( n == 0 )
		{
			/* Peer gracefully closed the connection. */
			return 0;
		}

		/* n < 0 — distinguish timeout from a real error. */
		if ( isTimeoutError(rawErr) )
		{
			return 0;
		}

#ifdef _WIN32
		m_lastError = { rawErr, std::system_category() };
#else
		m_lastError = { rawErr, std::generic_category() };
#endif

		return -1;
	}

	std::string
	TCPClient::receiveString (size_t maxLength, uint32_t timeoutMs) noexcept
	{
		std::string result;
		result.resize(maxLength);

		const auto n = this->receive(result.data(), maxLength, timeoutMs);

		if ( n > 0 )
		{
			result.resize(static_cast< size_t >(n));
		}
		else
		{
			result.clear();
		}

		return result;
	}

	/* ---- Address queries ---- */

	namespace
	{
		bool
		formatEndpoint (const sockaddr_storage & storage, std::string & address, uint16_t & port) noexcept
		{
			std::array< char, INET6_ADDRSTRLEN > buffer{};

			if ( storage.ss_family == AF_INET )
			{
				const auto * v4 = reinterpret_cast< const sockaddr_in * >(&storage);

				if ( ::inet_ntop(AF_INET, &v4->sin_addr, buffer.data(), buffer.size()) == nullptr )
				{
					return false;
				}

				address = buffer.data();
				port    = ntohs(v4->sin_port);

				return true;
			}

			if ( storage.ss_family == AF_INET6 )
			{
				const auto * v6 = reinterpret_cast< const sockaddr_in6 * >(&storage);

				if ( ::inet_ntop(AF_INET6, &v6->sin6_addr, buffer.data(), buffer.size()) == nullptr )
				{
					return false;
				}

				address = buffer.data();
				port    = ntohs(v6->sin6_port);

				return true;
			}

			return false;
		}
	}

	bool
	TCPClient::getLocalAddress (std::string & address, uint16_t & port) const noexcept
	{
		if ( m_handleMutex == nullptr )
		{
			return false;
		}

		const std::shared_lock< std::shared_mutex > readers{*m_handleMutex};

		if ( m_handle == InvalidHandle )
		{
			return false;
		}

		sockaddr_storage storage{};
#ifdef _WIN32
		int len = sizeof(storage);
#else
		socklen_t len = sizeof(storage);
#endif

		if ( ::getsockname(toNative(m_handle), reinterpret_cast< sockaddr * >(&storage), &len) != 0 )
		{
			return false;
		}

		return formatEndpoint(storage, address, port);
	}

	bool
	TCPClient::getRemoteAddress (std::string & address, uint16_t & port) const noexcept
	{
		if ( m_handleMutex == nullptr )
		{
			return false;
		}

		const std::shared_lock< std::shared_mutex > readers{*m_handleMutex};

		if ( m_handle == InvalidHandle )
		{
			return false;
		}

		sockaddr_storage storage{};
#ifdef _WIN32
		int len = sizeof(storage);
#else
		socklen_t len = sizeof(storage);
#endif

		if ( ::getpeername(toNative(m_handle), reinterpret_cast< sockaddr * >(&storage), &len) != 0 )
		{
			return false;
		}

		return formatEndpoint(storage, address, port);
	}

	/* ---- Socket options ---- */

	bool
	TCPClient::setNoDelay (bool enable) noexcept
	{
		if ( m_handleMutex == nullptr )
		{
			return false;
		}

		const std::shared_lock< std::shared_mutex > readers{*m_handleMutex};

		if ( m_handle == InvalidHandle )
		{
			return false;
		}

		const int flag = enable ? 1 : 0;

#ifdef _WIN32
		return ::setsockopt(toNative(m_handle), IPPROTO_TCP, TCP_NODELAY,
			reinterpret_cast< const char * >(&flag), sizeof(flag)) == 0;
#else
		return ::setsockopt(toNative(m_handle), IPPROTO_TCP, TCP_NODELAY,
			&flag, sizeof(flag)) == 0;
#endif
	}

	bool
	TCPClient::setKeepAlive (bool enable, uint32_t initialDelaySeconds) noexcept
	{
		if ( m_handleMutex == nullptr )
		{
			return false;
		}

		const std::shared_lock< std::shared_mutex > readers{*m_handleMutex};

		if ( m_handle == InvalidHandle )
		{
			return false;
		}

		const auto handle = toNative(m_handle);
		const int  flag   = enable ? 1 : 0;

#ifdef _WIN32
		if ( ::setsockopt(handle, SOL_SOCKET, SO_KEEPALIVE,
				reinterpret_cast< const char * >(&flag), sizeof(flag)) != 0 )
		{
			m_lastError = lastNativeCode();

			return false;
		}

		if ( !enable )
		{
			return true;
		}

		struct tcp_keepalive ka
		{
			.onoff             = 1,
			.keepalivetime     = static_cast< ULONG >(initialDelaySeconds) * 1000,
			.keepaliveinterval = 1000
		};
		DWORD bytesReturned = 0;
		static_cast< void >(::WSAIoctl(handle, SIO_KEEPALIVE_VALS, &ka, sizeof(ka),
			nullptr, 0, &bytesReturned, nullptr, nullptr));

		return true;
#else
		if ( ::setsockopt(handle, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)) != 0 )
		{
			m_lastError = lastNativeCode();

			return false;
		}

		if ( !enable )
		{
			return true;
		}

		const int delay = static_cast< int >(initialDelaySeconds);

	#if defined(__linux__)
		static_cast< void >(::setsockopt(handle, IPPROTO_TCP, TCP_KEEPIDLE, &delay, sizeof(delay)));
	#elif defined(__APPLE__)
		static_cast< void >(::setsockopt(handle, IPPROTO_TCP, TCP_KEEPALIVE, &delay, sizeof(delay)));
	#else
		static_cast< void >(delay);
	#endif

		return true;
#endif
	}

	bool
	TCPClient::setRecvTimeout (uint32_t timeoutMs) noexcept
	{
		if ( m_handleMutex == nullptr )
		{
			return false;
		}

		const std::shared_lock< std::shared_mutex > readers{*m_handleMutex};

		if ( m_handle == InvalidHandle )
		{
			return false;
		}

		return applyTimeout(toNative(m_handle), SO_RCVTIMEO, timeoutMs);
	}

	bool
	TCPClient::setSendTimeout (uint32_t timeoutMs) noexcept
	{
		if ( m_handleMutex == nullptr )
		{
			return false;
		}

		const std::shared_lock< std::shared_mutex > readers{*m_handleMutex};

		if ( m_handle == InvalidHandle )
		{
			return false;
		}

		return applyTimeout(toNative(m_handle), SO_SNDTIMEO, timeoutMs);
	}
}