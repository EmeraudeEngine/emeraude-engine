/*
 * src/Console/RemoteListener.cpp
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

#include "RemoteListener.hpp"

#ifdef ASIO_ENABLED

/* Local inclusions. */
#include "Tracer.hpp"

/* STL inclusions. */
#include <istream>

namespace EmEn::Console
{
	/**
	 * @brief Manages a single TCP client session.
	 * @details Reads commands line-by-line from the socket and forwards them
	 * to the RemoteListener command queue.
	 */
	class Session final : public std::enable_shared_from_this< Session >
	{
		public:

			/**
			 * @brief Constructs a session.
			 * @param socket The shared pointer to the client socket.
			 * @param listener A reference to the owning RemoteListener.
			 */
			Session (std::shared_ptr< asio::ip::tcp::socket > socket, RemoteListener & listener) noexcept
				: m_socket{std::move(socket)},
				m_listener{listener}
			{

			}

			/**
			 * @brief Starts the session by sending a welcome message and beginning to read.
			 */
			void
			start () noexcept
			{
				asio::error_code ec;
				static_cast< void >(asio::write(*m_socket, asio::buffer("Welcome to Emeraude-Engine AI Remote Console\n"), ec));

				if ( ec )
				{
					m_listener.removeClient(m_socket);

					return;
				}

				this->doRead();
			}

		private:

			/**
			 * @brief Asynchronously reads a line from the socket.
			 */
			void
			doRead () noexcept
			{
				auto self(this->shared_from_this());

				asio::async_read_until(*m_socket, m_buffer, '\n', [this, self] (const asio::error_code & ec, [[maybe_unused]] std::size_t length) {
					if ( !ec )
					{
						std::istream is(&m_buffer);
						std::string line;
						std::getline(is, line);

						/* Remove potential carriage return. */
						if ( !line.empty() && line.back() == '\r' )
						{
							line.pop_back();
						}

						if ( !line.empty() )
						{
							m_listener.enqueueCommand(line);
						}

						this->doRead();
					}
					else
					{
						m_listener.removeClient(m_socket);
					}
				});
			}

			std::shared_ptr< asio::ip::tcp::socket > m_socket;
			RemoteListener & m_listener;
			asio::streambuf m_buffer;
	};

	RemoteListener::RemoteListener (uint16_t port) noexcept
		: m_port{port},
		m_acceptor{std::make_unique< asio::ip::tcp::acceptor >(m_ioContext)}
	{
		asio::error_code ec;
		const asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), m_port);

		m_acceptor->open(endpoint.protocol(), ec);

		if ( ec )
		{
			TraceError{ClassId} << "Failed to open acceptor: " << ec.message();

			return;
		}

		m_acceptor->set_option(asio::socket_base::reuse_address(true), ec);

		if ( ec )
		{
			TraceWarning{ClassId} << "Failed to set reuse_address: " << ec.message();
			ec.clear();
		}

		m_acceptor->bind(endpoint, ec);

		if ( ec )
		{
			TraceError{ClassId} << "Failed to bind to port " << m_port << ": " << ec.message();

			return;
		}

		m_acceptor->listen(asio::socket_base::max_listen_connections, ec);

		if ( ec )
		{
			TraceError{ClassId} << "Failed to listen on port " << m_port << ": " << ec.message();

			return;
		}

		m_running = true;

		this->accept();

		m_networkThread = std::thread([this] () {
			TraceInfo{ClassId} << "Starting ASIO AI Remote Console on port " << m_port << ".";

			static_cast< void >(m_ioContext.run());

			TraceInfo{ClassId} << "ASIO AI Remote Console thread stopped.";
		});
	}

	RemoteListener::~RemoteListener ()
	{
		if ( !m_running )
		{
			return;
		}

		m_running = false;

		m_ioContext.stop();

		if ( m_networkThread.joinable() )
		{
			m_networkThread.join();
		}

		{
			const std::lock_guard< std::mutex > lock{m_clientsMutex};

			for ( auto & client : m_clients )
			{
				asio::error_code ec;

				client->close(ec);
			}

			m_clients.clear();
		}

		m_acceptor.reset();
	}

	bool
	RemoteListener::popCommand (std::string & outCommand) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_queueMutex};

		if ( m_commandsQueue.empty() )
		{
			return false;
		}

		outCommand = std::move(m_commandsQueue.front());
		m_commandsQueue.pop();

		return true;
	}

	void
	RemoteListener::broadcast (const std::string & message) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_clientsMutex};

		const std::string payload = message + "\n";

		for ( auto it = m_clients.begin(); it != m_clients.end(); )
		{
			asio::error_code ec;
			static_cast< void >(asio::write(**it, asio::buffer(payload), ec));

			if ( ec )
			{
				it = m_clients.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void
	RemoteListener::accept () noexcept
	{
		auto socket = std::make_shared< asio::ip::tcp::socket >(m_ioContext);

		m_acceptor->async_accept(*socket, [this, socket] (const asio::error_code & error) {
			if (!error)
			{
				TraceInfo{ClassId} << "New AI client connected to Remote Console.";

				{
					std::lock_guard< std::mutex > lock(m_clientsMutex);
					m_clients.insert(socket);
				}

				std::make_shared< Session >(socket, *this)->start();
			}

			if ( m_running )
			{
				this->accept();
			}
		});
	}

	void
	RemoteListener::enqueueCommand (const std::string & command) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_queueMutex};

		m_commandsQueue.push(command);
	}

	void
	RemoteListener::removeClient (const std::shared_ptr< asio::ip::tcp::socket > & socket) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_clientsMutex};

		m_clients.erase(socket);
	}
}

#endif // ASIO_ENABLED
