/*
 * src/Console/RemoteListener.cpp
 * This file is part of Emeraude-Engine
 */

#include "RemoteListener.hpp"
#include "Tracer.hpp"
#include "Libs/String.hpp"
#include <istream>

#ifdef ASIO_ENABLED

namespace EmEn::Console
{
	class Session : public std::enable_shared_from_this<Session>
	{
		public:

			Session(std::shared_ptr<asio::ip::tcp::socket> socket, RemoteListener & listener)
				: m_socket(std::move(socket)), m_listener(listener)
			{
			}

			void start()
			{
				asio::error_code ec;
				asio::write(*m_socket, asio::buffer("Welcome to Emeraude-Engine AI Remote Console\n"), ec);
				this->doRead();
			}

		private:

			void doRead()
			{
				auto self(shared_from_this());
				asio::async_read_until(*m_socket, m_buffer, '\n',
					[this, self](const asio::error_code& ec, std::size_t length)
					{
						if (!ec)
						{
							std::istream is(&m_buffer);
							std::string line;
							std::getline(is, line);

							/* Remove potential carriage return. */
							if (!line.empty() && line.back() == '\r')
							{
								line.pop_back();
							}
							
							if (!line.empty())
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

			std::shared_ptr<asio::ip::tcp::socket> m_socket;
			RemoteListener & m_listener;
			asio::streambuf m_buffer;
	};

	RemoteListener::RemoteListener(uint16_t port)
		: m_port{port},
		  m_acceptor{m_ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)}
	{
	}

	RemoteListener::~RemoteListener()
	{
		this->stop();
	}

	void RemoteListener::start()
	{
		if (m_running)
		{
			return;
		}

		m_running = true;

		this->accept();

		m_networkThread = std::thread([this]() {
			Tracer::info("RemoteListener", std::string("Starting ASIO AI Remote Console on port ") + std::to_string(m_port));
			m_ioContext.run();
			Tracer::info("RemoteListener", "ASIO AI Remote Console thread stopped");
		});
	}

	void RemoteListener::stop()
	{
		if (!m_running)
		{
			return;
		}

		m_running = false;

		m_ioContext.stop();

		if (m_networkThread.joinable())
		{
			m_networkThread.join();
		}

		std::lock_guard<std::mutex> lock(m_clientsMutex);
		for (auto & client : m_clients)
		{
			asio::error_code ec;
			client->close(ec);
		}
		m_clients.clear();
	}

	bool RemoteListener::popCommand(std::string & outCommand) noexcept
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);

		if (m_commandsQueue.empty())
		{
			return false;
		}

		outCommand = std::move(m_commandsQueue.front());
		m_commandsQueue.pop();

		return true;
	}

	void RemoteListener::broadcast(const std::string & message) noexcept
	{
		std::lock_guard<std::mutex> lock(m_clientsMutex);
		
		std::string payload = message + "\n";
		
		for (auto it = m_clients.begin(); it != m_clients.end(); )
		{
			auto client = *it;
			asio::error_code ec;
			
			/* NOTE: Synchronous write is acceptable for simple broadcasting since it's fire-and-forget
			 * and we handle basic non-blocking sockets. But if it stalls, we should drop the client. */
			asio::write(*client, asio::buffer(payload), ec);

			if (ec)
			{
				it = m_clients.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	void RemoteListener::accept()
	{
		auto socket = std::make_shared<asio::ip::tcp::socket>(m_ioContext);

		m_acceptor.async_accept(*socket, [this, socket](const asio::error_code& error) {
			if (!error)
			{
				Tracer::info("RemoteListener", "New AI client connected to Remote Console");

				{
					std::lock_guard<std::mutex> lock(m_clientsMutex);
					m_clients.insert(socket);
				}

				std::make_shared<Session>(socket, *this)->start();
			}

			if (m_running)
			{
				this->accept();
			}
		});
	}

	void RemoteListener::enqueueCommand(const std::string & command) noexcept
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);
		m_commandsQueue.push(command);
	}

	void RemoteListener::removeClient(std::shared_ptr<asio::ip::tcp::socket> socket) noexcept
	{
		std::lock_guard<std::mutex> lock(m_clientsMutex);
		m_clients.erase(socket);
	}
}

#endif // ASIO_ENABLED
