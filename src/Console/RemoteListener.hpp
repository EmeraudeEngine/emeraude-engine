/*
 * src/Console/RemoteListener.hpp
 * This file is part of Emeraude-Engine
 */

#pragma once

#include "emeraude_config.hpp"

#ifdef ASIO_ENABLED

#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <memory>
#include <set>
#include <asio.hpp>

namespace EmEn::Console
{
	/**
	 * @brief A TCP listener for remote AI console control.
	 * @details Starts a server on a specified port (default 7777).
	 * Accepts incoming commands (line by line) and stores them in a thread-safe queue.
	 * Also acts as a hub to broadcast messages back to all connected clients.
	 */
	class RemoteListener
	{
		friend class Session;

		public:

			RemoteListener(uint16_t port = 7777);
			~RemoteListener();

			void start();
			void stop();

			/**
			 * @brief Pops the oldest command from the queue.
			 * @param outCommand Will contain the command string if successful.
			 * @return true if a command was available, false otherwise.
			 */
			bool popCommand(std::string & outCommand) noexcept;

			/**
			 * @brief Broadcasts a message string to all connected clients.
			 */
			void broadcast(const std::string & message) noexcept;

		private:

			void accept();
			void enqueueCommand(const std::string & command) noexcept;
			void removeClient(std::shared_ptr<asio::ip::tcp::socket> socket) noexcept;

			uint16_t m_port;

			asio::io_context m_ioContext;
			asio::ip::tcp::acceptor m_acceptor;
			std::thread m_networkThread;
			std::atomic<bool> m_running{false};

			std::mutex m_clientsMutex;
			std::set<std::shared_ptr<asio::ip::tcp::socket>> m_clients;

			std::mutex m_queueMutex;
			std::queue<std::string> m_commandsQueue;
	};
}

#endif // ASIO_ENABLED
