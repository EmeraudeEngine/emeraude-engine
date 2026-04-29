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
#include <chrono>
#include <utility>

/* Platform-specific socket inclusions for setRecvTimeout / setSendTimeout /
 * setKeepAlive's optional initial-delay parameter. Asio already pulls in the
 * basic socket headers transitively, but TCP_KEEPIDLE / SIO_KEEPALIVE_VALS
 * live behind explicit headers. */
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
	#include <netinet/tcp.h>
	#include <sys/socket.h>
	#include <sys/time.h>
#endif

namespace EmEn::Net
{
	/* ---- Lifecycle ---- */

	TCPClient::TCPClient () noexcept
		: m_ioContext{std::make_unique< asio::io_context >()},
		m_socket{std::make_unique< asio::ip::tcp::socket >(*m_ioContext)}
	{

	}

	TCPClient::TCPClient (std::unique_ptr< asio::io_context > ioContext, std::unique_ptr< asio::ip::tcp::socket > socket) noexcept
		: m_ioContext{std::move(ioContext)},
		m_socket{std::move(socket)}
	{

	}

	TCPClient::~TCPClient () noexcept
	{
		this->close();
	}

	TCPClient::TCPClient (TCPClient && other) noexcept
		: m_ioContext{std::move(other.m_ioContext)},
		m_socket{std::move(other.m_socket)},
		m_lastError{std::exchange(other.m_lastError, std::error_code{})}
	{

	}

	TCPClient &
	TCPClient::operator= (TCPClient && other) noexcept
	{
		if ( this != &other )
		{
			this->close();

			m_ioContext = std::move(other.m_ioContext);
			m_socket = std::move(other.m_socket);
			m_lastError = std::exchange(other.m_lastError, std::error_code{});
		}

		return *this;
	}

	/* ---- Connection ---- */

	bool
	TCPClient::connect (const std::string & host, uint16_t port, uint32_t timeoutMs) noexcept
	{
		if ( m_socket == nullptr || m_ioContext == nullptr )
		{
			m_lastError = std::make_error_code(std::errc::bad_file_descriptor);

			return false;
		}

		/* If already connected, drop the previous connection first. */
		if ( m_socket->is_open() )
		{
			asio::error_code closeEc;
			m_socket->close(closeEc);
		}

		/* Resolve the host (supports both IPv4 and IPv6). */
		asio::ip::tcp::resolver resolver{*m_ioContext};
		asio::error_code resolveEc;
		const auto results = resolver.resolve(host, std::to_string(port), resolveEc);

		if ( resolveEc )
		{
			m_lastError = resolveEc;

			return false;
		}

		/* Issue an async connect and drive the io_context for at most timeoutMs.
		 * The would_block sentinel is overwritten by the completion handler. */
		asio::error_code connectEc = asio::error::would_block;

		asio::async_connect(*m_socket, results,
			[&connectEc] (const asio::error_code & ec, const asio::ip::tcp::endpoint &) noexcept {
				connectEc = ec;
			}
		);

		m_ioContext->restart();
		static_cast< void >(m_ioContext->run_for(std::chrono::milliseconds(timeoutMs)));

		if ( connectEc == asio::error::would_block )
		{
			/* run_for() exited due to deadline; cancel pending op and drain. */
			asio::error_code cancelEc;
			m_socket->cancel(cancelEc);
			static_cast< void >(m_ioContext->run());

			m_lastError = asio::error::timed_out;

			return false;
		}

		if ( connectEc )
		{
			m_lastError = connectEc;

			return false;
		}

		return true;
	}

	void
	TCPClient::close () noexcept
	{
		if ( m_socket != nullptr && m_socket->is_open() )
		{
			asio::error_code shutdownEc;
			m_socket->shutdown(asio::socket_base::shutdown_both, shutdownEc);

			asio::error_code closeEc;
			m_socket->close(closeEc);
		}
	}

	bool
	TCPClient::isConnected () const noexcept
	{
		return m_socket != nullptr && m_socket->is_open();
	}

	/* ---- Send / Receive ---- */

	int
	TCPClient::send (const void * data, size_t length) noexcept
	{
		if ( !this->isConnected() )
		{
			return -1;
		}

		if ( length == 0 )
		{
			return 0;
		}

		asio::error_code ec;
		const auto written = asio::write(*m_socket, asio::buffer(data, length), ec);

		if ( ec )
		{
			m_lastError = ec;

			return written > 0 ? static_cast< int >(written) : -1;
		}

		return static_cast< int >(written);
	}

	int
	TCPClient::send (const std::string & data) noexcept
	{
		return this->send(data.data(), data.size());
	}

	int
	TCPClient::receive (void * buffer, size_t maxLength, uint32_t timeoutMs) noexcept
	{
		if ( !this->isConnected() )
		{
			return -1;
		}

		if ( maxLength == 0 )
		{
			return 0;
		}

		asio::error_code readEc = asio::error::would_block;
		size_t bytesRead = 0;

		m_socket->async_read_some(asio::buffer(buffer, maxLength),
			[&readEc, &bytesRead] (const asio::error_code & ec, size_t n) noexcept {
				readEc = ec;
				bytesRead = n;
			}
		);

		m_ioContext->restart();

		if ( timeoutMs > 0 )
		{
			static_cast< void >(m_ioContext->run_for(std::chrono::milliseconds(timeoutMs)));
		}
		else
		{
			static_cast< void >(m_ioContext->run());
		}

		if ( readEc == asio::error::would_block )
		{
			/* Deadline elapsed before any data arrived: cancel and drain. */
			asio::error_code cancelEc;
			m_socket->cancel(cancelEc);
			static_cast< void >(m_ioContext->run());

			return 0;
		}

		if ( readEc == asio::error::eof )
		{
			/* Peer closed the connection cleanly. */
			m_lastError = readEc;

			return 0;
		}

		if ( readEc )
		{
			m_lastError = readEc;

			return -1;
		}

		return static_cast< int >(bytesRead);
	}

	std::string
	TCPClient::receiveString (size_t maxLength, uint32_t timeoutMs) noexcept
	{
		std::string result;
		result.resize(maxLength);

		const auto bytesRead = this->receive(result.data(), maxLength, timeoutMs);

		if ( bytesRead > 0 )
		{
			result.resize(static_cast< size_t >(bytesRead));
		}
		else
		{
			result.clear();
		}

		return result;
	}

	/* ---- Address queries ---- */

	bool
	TCPClient::getLocalAddress (std::string & address, uint16_t & port) const noexcept
	{
		if ( !this->isConnected() )
		{
			return false;
		}

		asio::error_code ec;
		const auto endpoint = m_socket->local_endpoint(ec);

		if ( ec )
		{
			return false;
		}

		address = endpoint.address().to_string();
		port = endpoint.port();

		return true;
	}

	bool
	TCPClient::getRemoteAddress (std::string & address, uint16_t & port) const noexcept
	{
		if ( !this->isConnected() )
		{
			return false;
		}

		asio::error_code ec;
		const auto endpoint = m_socket->remote_endpoint(ec);

		if ( ec )
		{
			return false;
		}

		address = endpoint.address().to_string();
		port = endpoint.port();

		return true;
	}

	/* ---- Socket options ---- */

	bool
	TCPClient::setNoDelay (bool enable) noexcept
	{
		if ( !this->isConnected() )
		{
			return false;
		}

		asio::error_code ec;
		m_socket->set_option(asio::ip::tcp::no_delay(enable), ec);

		if ( ec )
		{
			m_lastError = ec;

			return false;
		}

		return true;
	}

	bool
	TCPClient::setKeepAlive (bool enable, uint32_t initialDelaySeconds) noexcept
	{
		if ( !this->isConnected() )
		{
			return false;
		}

		asio::error_code ec;
		m_socket->set_option(asio::socket_base::keep_alive(enable), ec);

		if ( ec )
		{
			m_lastError = ec;

			return false;
		}

		if ( !enable )
		{
			return true;
		}

		/* Best-effort initial-delay setting (kernel granularity is platform dependent). */
		const auto fd = m_socket->native_handle();
		const int delay = static_cast< int >(initialDelaySeconds);

#if defined(__linux__)
		static_cast< void >(::setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &delay, sizeof(delay)));
#elif defined(__APPLE__)
		static_cast< void >(::setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &delay, sizeof(delay)));
#elif defined(_WIN32)
		struct tcp_keepalive ka
		{
			.onoff = 1,
			.keepalivetime = static_cast< ULONG >(delay) * 1000,
			.keepaliveinterval = 1000
		};
		DWORD bytesReturned = 0;
		static_cast< void >(::WSAIoctl(fd, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &bytesReturned, nullptr, nullptr));
#else
		static_cast< void >(delay);
		static_cast< void >(fd);
#endif

		return true;
	}

	bool
	TCPClient::setRecvTimeout (uint32_t timeoutMs) noexcept
	{
		if ( !this->isConnected() )
		{
			return false;
		}

		const auto fd = m_socket->native_handle();

#ifdef _WIN32
		const DWORD timeout = static_cast< DWORD >(timeoutMs);

		return ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast< const char * >(&timeout), sizeof(timeout)) == 0;
#else
		struct timeval tv{};
		tv.tv_sec = static_cast< time_t >(timeoutMs / 1000);
		tv.tv_usec = static_cast< suseconds_t >((timeoutMs % 1000) * 1000);

		return ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
#endif
	}

	bool
	TCPClient::setSendTimeout (uint32_t timeoutMs) noexcept
	{
		if ( !this->isConnected() )
		{
			return false;
		}

		const auto fd = m_socket->native_handle();

#ifdef _WIN32
		const DWORD timeout = static_cast< DWORD >(timeoutMs);

		return ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast< const char * >(&timeout), sizeof(timeout)) == 0;
#else
		struct timeval tv{};
		tv.tv_sec = static_cast< time_t >(timeoutMs / 1000);
		tv.tv_usec = static_cast< suseconds_t >((timeoutMs % 1000) * 1000);

		return ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0;
#endif
	}
}
