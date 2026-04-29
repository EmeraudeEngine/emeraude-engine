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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "TCPServer.hpp"

/* STL inclusions. */
#include <chrono>
#include <utility>

namespace EmEn::Net
{
	const int TCPServer::DefaultBacklog{asio::socket_base::max_listen_connections};

	/* ---- Lifecycle ---- */

	TCPServer::TCPServer () noexcept
		: m_ioContext{std::make_unique< asio::io_context >()}
	{

	}

	TCPServer::~TCPServer () noexcept
	{
		this->close();
	}

	TCPServer::TCPServer (TCPServer && other) noexcept
		: m_ioContext{std::move(other.m_ioContext)},
		m_acceptor{std::move(other.m_acceptor)},
		m_lastError{std::exchange(other.m_lastError, std::error_code{})}
	{

	}

	TCPServer &
	TCPServer::operator= (TCPServer && other) noexcept
	{
		if ( this != &other )
		{
			this->close();

			m_ioContext = std::move(other.m_ioContext);
			m_acceptor = std::move(other.m_acceptor);
			m_lastError = std::exchange(other.m_lastError, std::error_code{});
		}

		return *this;
	}

	/* ---- Listening ---- */

	bool
	TCPServer::listen (uint16_t port, int backlog, const std::string & address) noexcept
	{
		if ( m_ioContext == nullptr )
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

		m_acceptor = std::make_unique< asio::ip::tcp::acceptor >(*m_ioContext);

		asio::error_code ec;

		m_acceptor->open(endpoint.protocol(), ec);

		if ( ec )
		{
			m_lastError = ec;
			m_acceptor.reset();

			return false;
		}

		/* Best-effort: SO_REUSEADDR. Failure is non-fatal — the kernel may
		 * still bind, and the option is irrelevant on a clean port. */
		asio::error_code reuseEc;
		m_acceptor->set_option(asio::socket_base::reuse_address(true), reuseEc);

		m_acceptor->bind(endpoint, ec);

		if ( ec )
		{
			m_lastError = ec;

			asio::error_code closeEc;
			m_acceptor->close(closeEc);
			m_acceptor.reset();

			return false;
		}

		m_acceptor->listen(backlog, ec);

		if ( ec )
		{
			m_lastError = ec;

			asio::error_code closeEc;
			m_acceptor->close(closeEc);
			m_acceptor.reset();

			return false;
		}

		return true;
	}

	void
	TCPServer::close () noexcept
	{
		if ( m_acceptor != nullptr )
		{
			if ( m_acceptor->is_open() )
			{
				asio::error_code cancelEc;
				m_acceptor->cancel(cancelEc);

				asio::error_code closeEc;
				m_acceptor->close(closeEc);
			}

			m_acceptor.reset();
		}
	}

	bool
	TCPServer::isListening () const noexcept
	{
		return m_acceptor != nullptr && m_acceptor->is_open();
	}

	/* ---- Accept ---- */

	std::optional< TCPClient >
	TCPServer::accept (uint32_t timeoutMs) noexcept
	{
		if ( !this->isListening() )
		{
			return std::nullopt;
		}

		asio::error_code acceptEc = asio::error::would_block;
		asio::ip::tcp::socket peerSocket{*m_ioContext};

		m_acceptor->async_accept(peerSocket,
			[&acceptEc] (const asio::error_code & ec) noexcept {
				acceptEc = ec;
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

		if ( acceptEc == asio::error::would_block )
		{
			/* Deadline elapsed before any client arrived: cancel and drain. */
			asio::error_code cancelEc;
			m_acceptor->cancel(cancelEc);
			static_cast< void >(m_ioContext->run());

			return std::nullopt;
		}

		if ( acceptEc )
		{
			m_lastError = acceptEc;

			return std::nullopt;
		}

		/* Detach the accepted socket from the server's io_context and migrate
		 * it onto a fresh io_context that the returned TCPClient will own.
		 * This makes the client's lifetime independent from the server's. */
		const auto protocol = peerSocket.local_endpoint().protocol();

		asio::error_code releaseEc;
		const auto nativeHandle = peerSocket.release(releaseEc);

		if ( releaseEc )
		{
			m_lastError = releaseEc;

			return std::nullopt;
		}

		auto clientIoContext = std::make_unique< asio::io_context >();
		auto clientSocket = std::make_unique< asio::ip::tcp::socket >(*clientIoContext);

		asio::error_code assignEc;
		clientSocket->assign(protocol, nativeHandle, assignEc);

		if ( assignEc )
		{
			m_lastError = assignEc;

			return std::nullopt;
		}

		return TCPClient{std::move(clientIoContext), std::move(clientSocket)};
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
		const auto endpoint = m_acceptor->local_endpoint(ec);

		if ( ec )
		{
			return false;
		}

		address = endpoint.address().to_string();
		port = endpoint.port();

		return true;
	}
}
