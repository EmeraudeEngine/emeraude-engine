/*
 * src/Console/MCP/Server.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <thread>
#include <vector>

/* Third-party inclusions. */
#include "asio.hpp"
#include "Network/asio_throw_exception.hpp"
#include "json/json.h"

namespace EmEn::Console
{
	class Controller;
}

namespace EmEn::Console::MCP
{
	class Connection;

	/**
	 * @brief The engine's MCP server: Streamable HTTP on one endpoint (`/mcp`), dual-era (see Protocol.hpp).
	 * @note Threading, the rule that makes it safe: every socket operation happens on the server's own
	 * network thread, and the console tree is only read on the MAIN thread. A request that needs the tree
	 * (`tools/list`, `tools/call`) is queued; Controller::poll() calls processPendingRequests() between two
	 * frames, which runs it and posts the answer back to the network thread. Nothing else is shared.
	 * @note Security: bound to loopback by default; a non-loopback address refuses to start without a bearer
	 * token; the `Origin` header (DNS rebinding — CEF renders web pages in this very process) and the `Host`
	 * header are validated on every request; sizes, connections and idle time are bounded.
	 */
	class Server final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MCPServer"};

			/** @brief The only path served. */
			static constexpr auto EndpointPath{"/mcp"};

			/** @brief Maximum size of the request line and headers. */
			static constexpr size_t MaxHeaderBytes{16384};

			/** @brief Maximum size of a request body. */
			static constexpr size_t MaxBodyBytes{1048576};

			/** @brief Maximum number of simultaneous connections (notification streams included). */
			static constexpr size_t MaxConnections{16};

			/** @brief Maximum number of requests waiting for the main thread, all connections together. */
			static constexpr size_t MaxPendingRequests{64};

			/** @brief Time allowed to receive a whole request once its first byte arrived, in seconds. */
			static constexpr uint32_t RequestTimeoutSeconds{30};

			/** @brief Time an idle keep-alive connection is kept open, in seconds. */
			static constexpr uint32_t IdleTimeoutSeconds{120};

			/** @brief Period of the keep-alive comment on a notification stream, in seconds. */
			static constexpr uint32_t StreamKeepAliveSeconds{15};

			/**
			 * @brief Constructs and starts the server.
			 * @param address The bind address (an IP address).
			 * @param port The TCP port.
			 * @param bearerToken The token every request must carry; empty = none (loopback only).
			 */
			Server (const std::string & address, uint16_t port, std::string bearerToken) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Server (const Server & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Server (Server && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Server &
			 */
			Server & operator= (const Server & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Server &
			 */
			Server & operator= (Server && copy) noexcept = delete;

			/**
			 * @brief Stops the server: notification streams get a graceful closure, then every socket closes.
			 */
			~Server ();

			/**
			 * @brief Returns whether the server is accepting connections.
			 * @return bool
			 */
			[[nodiscard]]
			bool isRunning () const noexcept;

			/**
			 * @brief Returns the endpoint URL, e.g. "http://127.0.0.1:17778/mcp" (empty when not running).
			 * @return std::string
			 */
			[[nodiscard]]
			std::string endpoint () const noexcept;

			/**
			 * @brief Runs the queued requests that need the console tree, and announces a changed tool list.
			 * @note MAIN THREAD ONLY (Controller::poll()).
			 * @param controller The console controller.
			 * @return void
			 */
			void processPendingRequests (const Controller & controller) noexcept;

		private:

			friend class Connection;

			/** @brief A request that needs the main thread. */
			struct PendingRequest
			{
				std::weak_ptr< Connection > connection;
				Json::Value id;
				std::string method;
				Json::Value params;
				bool modern{false};
			};

			/**
			 * @brief Starts accepting connections (network thread).
			 * @return void
			 */
			void accept () noexcept;

			/**
			 * @brief Queues a request for the main thread (network thread).
			 * @param request The request [std::move].
			 * @return bool False when the queue is full.
			 */
			[[nodiscard]]
			bool enqueue (PendingRequest request) noexcept;

			/**
			 * @brief Forgets a closed connection (network thread).
			 * @param connection The connection.
			 * @return void
			 */
			void removeConnection (const std::shared_ptr< Connection > & connection) noexcept;

			/**
			 * @brief Sends `notifications/tools/list_changed` on every notification stream that asked for it
			 * (network thread).
			 * @return void
			 */
			void broadcastToolsListChanged () noexcept;

			/**
			 * @brief Returns whether a Host header names this server (loopback binding only).
			 * @param host The Host header value.
			 * @return bool
			 */
			[[nodiscard]]
			bool isAcceptedHost (const std::string & host) const noexcept;

			/**
			 * @brief Returns whether an Origin header is acceptable (absent, or this server itself).
			 * @param origin The Origin header value.
			 * @return bool
			 */
			[[nodiscard]]
			bool isAcceptedOrigin (const std::string & origin) const noexcept;

			/**
			 * @brief Returns whether an Authorization header carries the bearer token (constant-time compare).
			 * @param authorization The Authorization header value.
			 * @return bool
			 */
			[[nodiscard]]
			bool isAuthorized (const std::string & authorization) const noexcept;

			std::string m_address;
			std::string m_bearerToken;
			asio::io_context m_ioContext;
			std::optional< asio::executor_work_guard< asio::io_context::executor_type > > m_workGuard;
			std::unique_ptr< asio::ip::tcp::acceptor > m_acceptor;
			std::thread m_networkThread;
			/** @brief The open connections, touched on the network thread only. */
			std::set< std::shared_ptr< Connection > > m_connections;
			std::mutex m_queueMutex;
			std::vector< PendingRequest > m_pendingRequests;
			/** @brief The tool-name refusals last traced (main thread), so each is reported once. */
			std::vector< std::string > m_reportedRejections;
			uint64_t m_announcedTreeRevision{0};
			uint16_t m_port;
			bool m_running{false};
			bool m_loopback{false};
	};
}
