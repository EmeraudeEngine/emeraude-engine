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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "RemoteListener.hpp"

/* STL inclusions. */
#include <istream>
#include <string_view>

/* Third-party inclusions. */
#ifndef _WIN32
	#include <sys/socket.h>
	#include <sys/time.h>
#endif

/* Local inclusions. */
#include "RemoteProtocol.hpp"
#include "Tracer.hpp"

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

				/* The banner is a response line like any other (RemoteProtocol), carrying the protocol version. */
				const auto banner = RemoteProtocol::serializeWelcome() + '\n';

				static_cast< void >(asio::write(*m_socket, asio::buffer(banner), ec));

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
				/* Disconnected (too many pending commands, from enqueueCommand() in the handler below): its socket now
				 * belongs to the GracefulCloser, which drains it. */
				if ( !m_listener.isClient(m_socket) )
				{
					return;
				}

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
							m_listener.enqueueCommand(line, m_socket);
						}

						this->doRead();
					}
					else
					{
						/* A line longer than the buffer trips not_found: the peer is either broken
						 * or feeding us bytes to grow the buffer. Say so, then drop it. */
						if ( ec == asio::error::not_found )
						{
							TraceWarning{RemoteListener::ClassId} << "A client sent a line longer than " << RemoteListener::MaxLineLength << " bytes, disconnecting it.";

							m_listener.disconnect(m_socket, RemoteProtocol::serializeError("Line too long (more than " + std::to_string(RemoteListener::MaxLineLength) + " bytes): disconnected."));
						}

						m_listener.removeClient(m_socket);
					}
				});
			}

			std::shared_ptr< asio::ip::tcp::socket > m_socket;
			RemoteListener & m_listener;
			/* Bounded: an unbounded streambuf lets a peer that never sends a newline grow the
			 * process until the OOM killer fires — on an unauthenticated port. */
			asio::streambuf m_buffer{RemoteListener::MaxLineLength};
	};

	RemoteListener::RemoteListener (std::string address, uint16_t port) noexcept
		: m_address{std::move(address)},
		m_port{port},
		m_acceptor{std::make_unique< asio::ip::tcp::acceptor >(m_ioContext)}
	{
		asio::error_code ec;

		/* The bind address decides who can reach an unauthenticated command channel:
		 * loopback = this host only, "0.0.0.0" / "::" = every peer on the network. A
		 * value that does not parse must therefore fall back to the SAFE side. */
		auto bindAddress = asio::ip::make_address(m_address, ec);

		if ( ec )
		{
			TraceWarning{ClassId} << "Invalid remote console bind address '" << m_address << "' (" << ec.message() << "), falling back to loopback.";

			bindAddress = asio::ip::address_v4::loopback();
			m_address = bindAddress.to_string();
			ec.clear();
		}

		const asio::ip::tcp::endpoint endpoint(bindAddress, m_port);

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
			/* ⚠️ Windows reports a port another process holds exclusively as ACCESS DENIED, not "address in use". */
			const auto * const hint = ( ec == asio::error::address_in_use || ec == asio::error::access_denied ) ?
				" — the port is most likely taken by another process (Windows reports that as access denied); choose another one in Core/Console/RemoteListenerPort" :
				"";

			TraceError{ClassId} << "Failed to bind to " << m_address << ':' << m_port << ": " << ec.message() << hint;

			return;
		}

		m_acceptor->listen(asio::socket_base::max_listen_connections, ec);

		if ( ec )
		{
			TraceError{ClassId} << "Failed to listen on " << m_address << ':' << m_port << ": " << ec.message();

			return;
		}

		m_running = true;

		this->accept();

		m_networkThread = std::thread([this] () {
			TraceInfo{ClassId} << "Starting ASIO AI Remote Console on " << m_address << ':' << m_port << ".";

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

		/* ⚠️ Order matters: the acceptor and every client socket are closed BEFORE stopping the
		 * context and joining. io_context::stop() does not interrupt a handler that is already
		 * running, and a write to a peer that stopped reading only ends when its socket dies —
		 * closing after the join is how the whole process used to hang on exit. */
		if ( m_acceptor != nullptr && m_acceptor->is_open() )
		{
			asio::error_code ec;

			/* NOTE: best effort — the acceptor is going away, a failure leaves nothing to do. */
			m_acceptor->cancel(ec);
			m_acceptor->close(ec);
		}

		{
			const std::scoped_lock lock{m_clientsMutex};

			for ( const auto & client : m_clients )
			{
				asio::error_code ec;

				/* NOTE: best effort — the client is dropped either way. */
				client->shutdown(asio::ip::tcp::socket::shutdown_both, ec);
				client->close(ec);
			}

			m_clients.clear();
		}

		m_ioContext.stop();

		if ( m_networkThread.joinable() )
		{
			m_networkThread.join();
		}

		m_acceptor.reset();
	}

	bool
	RemoteListener::popCommand (PendingCommand & outCommand) noexcept
	{
		const std::scoped_lock lock{m_queueMutex};

		if ( m_commandsQueue.empty() )
		{
			return false;
		}

		outCommand = std::move(m_commandsQueue.front());
		m_commandsQueue.pop();

		if ( const auto countIt = m_pendingPerClient.find(outCommand.client.get()); countIt != m_pendingPerClient.end() )
		{
			if ( --countIt->second == 0 )
			{
				m_pendingPerClient.erase(countIt);
			}
		}

		return true;
	}

	void
	RemoteListener::respond (const std::shared_ptr< asio::ip::tcp::socket > & client, const std::string & line) noexcept
	{
		if ( client == nullptr )
		{
			return;
		}

		asio::error_code ec;

		{
			const std::scoped_lock writeLock{m_writeMutex};

			/* NOTE: checked under the write lock — disconnect() removes the client and hands its socket over under
			 * the same lock; a socket lingering in the GracefulCloser is still open but no longer a client. */
			if ( !client->is_open() || !this->isClient(client) )
			{
				return;
			}

			static_cast< void >(asio::write(*client, asio::buffer(line + '\n'), ec));
		}

		if ( ec )
		{
			const std::scoped_lock lock{m_clientsMutex};

			m_clients.erase(client);
		}
	}

	void
	RemoteListener::accept () noexcept
	{
		auto socket = std::make_shared< asio::ip::tcp::socket >(m_ioContext);

		m_acceptor->async_accept(*socket, [this, socket] (const asio::error_code & error) {
			if ( !error )
			{
				size_t clientCount = 0;

				{
					const std::scoped_lock lock{m_clientsMutex};

					clientCount = m_clients.size();
				}

				if ( clientCount >= MaxClients )
				{
					TraceWarning{ClassId} << "Refusing a client: " << MaxClients << " already connected.";

					asio::error_code ec;
					const auto refusal = RemoteProtocol::serializeError("Too many clients (" + std::to_string(MaxClients) + " already connected).") + '\n';
					static_cast< void >(asio::write(*socket, asio::buffer(refusal), ec));
					/* NOTE: best effort — the refused socket is dropped either way. */
					socket->close(ec);
				}
				else
				{
					/* A send timeout is what keeps a peer that stops reading from blocking the io
					 * thread (banner) or the main thread (respond()) forever. */
					asio::error_code optionError;

#ifdef _WIN32
					const DWORD sendTimeout = static_cast< DWORD >(SendTimeoutMilliseconds);
#else
					timeval sendTimeout{};
					sendTimeout.tv_sec = static_cast< time_t >(SendTimeoutMilliseconds / 1000);
					sendTimeout.tv_usec = 0;
#endif
					if ( ::setsockopt(socket->native_handle(), SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast< const char * >(&sendTimeout), sizeof(sendTimeout)) != 0 )
					{
						TraceWarning{ClassId} << "Unable to set the send timeout of a client: a peer that stops reading may block its writer.";
					}

					socket->set_option(asio::ip::tcp::no_delay{true}, optionError);

					if ( optionError )
					{
						TraceWarning{ClassId} << "Unable to disable Nagle's algorithm for a client: " << optionError.message();
					}

					TraceInfo{ClassId} << "New AI client connected to Remote Console.";

					{
						const std::scoped_lock lock{m_clientsMutex};

						m_clients.insert(socket);
					}

					std::make_shared< Session >(socket, *this)->start();
				}
			}
			else if ( error != asio::error::operation_aborted )
			{
				/* Never re-arm blindly: a permanent failure (EMFILE) would spin this thread at
				 * 100 % with no trace of why. */
				TraceError{ClassId} << "Remote console accept failed: " << error.message() << ".";

				if ( error == asio::error::no_descriptors || error == asio::error::no_memory )
				{
					TraceError{ClassId} << "Stopping the accept loop; the remote console is now unreachable.";

					m_running = false;
				}
			}

			if ( m_running )
			{
				this->accept();
			}
		});
	}

	void
	RemoteListener::enqueueCommand (const std::string & command, const std::shared_ptr< asio::ip::tcp::socket > & client) noexcept
	{
		bool overflow = false;

		{
			const std::scoped_lock lock{m_queueMutex};

			auto & pendingCount = m_pendingPerClient[client.get()];

			/* NOTE: the global bound is unreachable while every client stays under its share; it remains
			 * as the memory guarantee (a disconnected client's commands are still queued). */
			if ( pendingCount >= MaxPendingCommandsPerClient || m_commandsQueue.size() >= MaxPendingCommands )
			{
				overflow = true;
			}
			else
			{
				m_commandsQueue.push({command, client});

				++pendingCount;
			}
		}

		if ( overflow )
		{
			TraceWarning{ClassId} << "A client exceeded " << MaxPendingCommandsPerClient << " pending commands, disconnecting it.";

			/* NOTE: outside the queue lock — disconnect() writes to the socket. The client is dropped
			 * rather than answered and kept: an answer to this request would overtake the answers to
			 * its requests still queued, and every later line would be matched to the wrong request. */
			this->disconnect(client, RemoteProtocol::serializeError("Too many pending commands (more than " + std::to_string(MaxPendingCommandsPerClient) + "): disconnected. Wait for each answer before sending more."));
		}
	}

	void
	RemoteListener::disconnect (const std::shared_ptr< asio::ip::tcp::socket > & client, const std::string & line) noexcept
	{
		if ( client == nullptr )
		{
			return;
		}

		{
			const std::scoped_lock writeLock{m_writeMutex};

			/* Removed under the write lock: respond() checks the membership under it. */
			this->removeClient(client);

			if ( client->is_open() )
			{
				asio::error_code ec;
				/* NOTE: best effort — the client is disconnected either way. */
				static_cast< void >(asio::write(*client, asio::buffer(line + '\n'), ec));

				/* NOT shutdown(both) + close(): the client may still be sending (the flood, the rest of an over-long
				 * line), and closing over unread bytes is a RST that, on Windows, discards this last line. */
				m_gracefulCloser.close(client);
			}
		}
	}

	bool
	RemoteListener::isClient (const std::shared_ptr< asio::ip::tcp::socket > & socket) noexcept
	{
		const std::scoped_lock lock{m_clientsMutex};

		return m_clients.contains(socket);
	}

	void
	RemoteListener::removeClient (const std::shared_ptr< asio::ip::tcp::socket > & socket) noexcept
	{
		const std::scoped_lock lock{m_clientsMutex};

		m_clients.erase(socket);
	}
}
