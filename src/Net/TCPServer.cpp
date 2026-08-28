/*
 * src/Net/TCPServer.cpp
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

#include "TCPServer.hpp"

/* STL inclusions. */
#include <chrono>
#include <utility>

/* Third-party inclusions. */
#include "Network/asio_throw_exception.hpp"
#include "asio.hpp"

namespace EmEn::Net
{
	struct TCPServer::Impl
	{
		asio::io_context ioContext;
		std::unique_ptr< asio::ip::tcp::acceptor > acceptor;
	};

	const int TCPServer::DefaultBacklog{asio::socket_base::max_listen_connections};

	/* ---- Lifecycle ---- */

	TCPServer::TCPServer () noexcept
		: m_impl{std::make_unique< Impl >()}
	{

	}

	TCPServer::~TCPServer () noexcept
	{
		this->close();
	}

	TCPServer::TCPServer (TCPServer && other) noexcept
		: m_impl{std::move(other.m_impl)},
		m_lastError{std::exchange(other.m_lastError, std::error_code{})}
	{

	}

	TCPServer &
	TCPServer::operator= (TCPServer && other) noexcept
	{
		if ( this != &other )
		{
			this->close();

			m_impl = std::move(other.m_impl);
			m_lastError = std::exchange(other.m_lastError, std::error_code{});
		}

		return *this;
	}

	/* ---- Listening ---- */

	bool
	TCPServer::listen (uint16_t port, int backlog, const std::string & address) noexcept
	{
		/* A moved-from server has no implementation left. */
		if ( m_impl == nullptr )
		{
			m_lastError = std::make_error_code(std::errc::bad_file_descriptor);

			return false;
		}

		this->close();

		/* Resolve the bind endpoint. Empty address or "0.0.0.0" listens on
		 * any IPv4 interface; pass an explicit address (including IPv6) to
		 * narrow the binding. */
		asio::ip::tcp::endpoint endpoint;

		if ( address.empty() || address == "0.0.0.0" )
		{
			endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port);
		}
		else
		{
			asio::error_code addrEc;
			const auto addr = asio::ip::make_address(address, addrEc);

			if ( addrEc )
			{
				m_lastError = addrEc;

				return false;
			}

			endpoint = asio::ip::tcp::endpoint(addr, port);
		}

		auto & acceptor = m_impl->acceptor;

		acceptor = std::make_unique< asio::ip::tcp::acceptor >(m_impl->ioContext);

		asio::error_code ec;

		acceptor->open(endpoint.protocol(), ec);

		if ( ec )
		{
			m_lastError = ec;
			acceptor.reset();

			return false;
		}

		/* Binding the IPv6 any-address must ask EXPLICITLY for a dual-stack
		 * socket. Whether IPv4 peers are accepted through it is a kernel
		 * policy, not a contract: Windows defaults to v6-only and refuses
		 * them SILENTLY, while Linux and macOS accept them (both default
		 * net.inet6.ip6.v6only / bindv6only to 0 - measured on macOS 26,
		 * and it is a tunable sysctl there, not a guarantee). Same trap as
		 * the multicast option widths: never read a default as a promise.
		 * Failing the listen here is deliberate - a socket that is up but
		 * invisible to half the network is exactly the bug being prevented,
		 * so a stack refusing the option must say so instead of silently
		 * reproducing it. Bind "0.0.0.0" to listen on IPv4 only. */
		if ( endpoint.address().is_v6() && endpoint.address().is_unspecified() )
		{
			acceptor->set_option(asio::ip::v6_only(false), ec);

			if ( ec )
			{
				m_lastError = ec;

				asio::error_code closeEc;
				acceptor->close(closeEc);
				acceptor.reset();

				return false;
			}
		}

		/* Best-effort: SO_REUSEADDR. Failure is non-fatal — the kernel may
		 * still bind, and the option is irrelevant on a clean port. */
		asio::error_code reuseEc;
		acceptor->set_option(asio::socket_base::reuse_address(true), reuseEc);

		acceptor->bind(endpoint, ec);

		if ( ec )
		{
			m_lastError = ec;

			asio::error_code closeEc;
			acceptor->close(closeEc);
			acceptor.reset();

			return false;
		}

		acceptor->listen(backlog, ec);

		if ( ec )
		{
			m_lastError = ec;

			asio::error_code closeEc;
			acceptor->close(closeEc);
			acceptor.reset();

			return false;
		}

		return true;
	}

	void
	TCPServer::close () noexcept
	{
		if ( m_impl == nullptr || m_impl->acceptor == nullptr )
		{
			return;
		}

		auto & acceptor = m_impl->acceptor;

		if ( acceptor->is_open() )
		{
			asio::error_code cancelEc;
			acceptor->cancel(cancelEc);

			asio::error_code closeEc;
			acceptor->close(closeEc);
		}

		acceptor.reset();
	}

	bool
	TCPServer::isListening () const noexcept
	{
		return m_impl != nullptr && m_impl->acceptor != nullptr && m_impl->acceptor->is_open();
	}

	/* ---- Accept ---- */

	std::optional< TCPClient >
	TCPServer::accept (uint32_t timeoutMs) noexcept
	{
		/* ⚠️ Cleared on entry: a stale error from a previous call used to be reported by the
		 * caller on every subsequent timeout, i.e. several spurious errors per second in a
		 * 200 ms accept loop. */
		m_lastError.clear();
		m_lastAcceptTimedOut = false;

		if ( !this->isListening() )
		{
			return std::nullopt;
		}

		asio::error_code acceptEc = asio::error::would_block;
		asio::ip::tcp::socket peerSocket{m_impl->ioContext};

		m_impl->acceptor->async_accept(peerSocket,
			[&acceptEc] (const asio::error_code & ec) noexcept {
				acceptEc = ec;
			}
		);

		m_impl->ioContext.restart();

		if ( timeoutMs > 0 )
		{
			static_cast< void >(m_impl->ioContext.run_for(std::chrono::milliseconds(timeoutMs)));
		}
		else
		{
			static_cast< void >(m_impl->ioContext.run());
		}

		if ( acceptEc == asio::error::would_block )
		{
			/* Deadline elapsed before any client arrived: cancel and drain. */
			m_lastAcceptTimedOut = true;

			asio::error_code cancelEc;
			m_impl->acceptor->cancel(cancelEc);
			static_cast< void >(m_impl->ioContext.run());

			return std::nullopt;
		}

		if ( acceptEc )
		{
			m_lastError = acceptEc;

			return std::nullopt;
		}

		/* Detach the accepted socket from the server's io_context. From this
		 * point on the peer is managed by raw kernel calls, completely
		 * independent of Asio — see TCPClient's design notes for why we
		 * bypass Asio for the runtime I/O path. The TCPClient's private
		 * constructor takes care of switching the handle to blocking mode
		 * and suppressing SIGPIPE where needed. */
		asio::error_code releaseEc;
		const auto nativeHandle = peerSocket.release(releaseEc);

		if ( releaseEc )
		{
			m_lastError = releaseEc;

			return std::nullopt;
		}

		return TCPClient{static_cast< TCPClient::native_handle_type >(nativeHandle)};
	}

	/* ---- Address query ---- */

	bool
	TCPServer::getLocalAddress (std::string & address, uint16_t & port) const noexcept
	{
		if ( !this->isListening() )
		{
			return false;
		}

		asio::error_code ec;
		const auto endpoint = m_impl->acceptor->local_endpoint(ec);

		if ( ec )
		{
			return false;
		}

		address = endpoint.address().to_string();
		port = endpoint.port();

		return true;
	}
}
