/*
 * src/Console/MCP/Server.cpp
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

#include "Server.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <map>
#include <sstream>
#include <utility>

/* Local inclusions. */
#include "emeraude_config.hpp"
#include "Console/Controller.hpp"
#include "FastJSON.hpp"
#include "Protocol.hpp"
#include "String.hpp"
#include "Tracer.hpp"

namespace EmEn::Console::MCP
{
	using namespace Base;

	namespace
	{
		/** @brief A parsed HTTP request. */
		struct HTTPRequest
		{
			std::string method;
			std::string target;
			std::string version;
			std::map< std::string, std::string > headers;
			std::string body;
		};

		/**
		 * @brief Returns a lowercase copy of an ASCII string.
		 * @param text The text.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		lowercase (std::string text) noexcept
		{
			std::ranges::transform(text, text.begin(), [] (char character) {
				return ( character >= 'A' && character <= 'Z' ) ? static_cast< char >(character - 'A' + 'a') : character;
			});

			return text;
		}

		/**
		 * @brief Returns a header value, or an empty string.
		 * @param request The request.
		 * @param name The lowercase header name.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		header (const HTTPRequest & request, const char * name) noexcept
		{
			const auto headerIt = request.headers.find(name);

			return headerIt != request.headers.end() ? headerIt->second : std::string{};
		}

		/**
		 * @brief Returns the reason phrase of the status codes this server sends.
		 * @param status The status code.
		 * @return const char *
		 */
		[[nodiscard]]
		const char *
		reasonPhrase (int status) noexcept
		{
			switch ( status )
			{
				case 200 : return "OK";
				case 202 : return "Accepted";
				case 400 : return "Bad Request";
				case 401 : return "Unauthorized";
				case 403 : return "Forbidden";
				case 404 : return "Not Found";
				case 405 : return "Method Not Allowed";
				case 408 : return "Request Timeout";
				case 411 : return "Length Required";
				case 413 : return "Content Too Large";
				case 415 : return "Unsupported Media Type";
				case 431 : return "Request Header Fields Too Large";
				case 501 : return "Not Implemented";
				case 503 : return "Service Unavailable";
				default : return "Error";
			}
		}

		/**
		 * @brief Parses the request line and the headers.
		 * @param head The bytes before the empty line.
		 * @param request Receives the method, target, version and headers.
		 * @return bool False on a malformed request (or a smuggling attempt: duplicated framing headers).
		 */
		[[nodiscard]]
		bool
		parseHead (const std::string & head, HTTPRequest & request) noexcept
		{
			std::istringstream stream{head};
			std::string line;

			if ( !std::getline(stream, line) )
			{
				return false;
			}

			if ( !line.empty() && line.back() == '\r' )
			{
				line.pop_back();
			}

			const auto firstSpace = line.find(' ');
			const auto secondSpace = line.find(' ', firstSpace + 1);

			if ( firstSpace == std::string::npos || secondSpace == std::string::npos )
			{
				return false;
			}

			request.method = line.substr(0, firstSpace);
			request.target = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
			request.version = line.substr(secondSpace + 1);

			if ( request.version != "HTTP/1.1" && request.version != "HTTP/1.0" )
			{
				return false;
			}

			while ( std::getline(stream, line) )
			{
				if ( !line.empty() && line.back() == '\r' )
				{
					line.pop_back();
				}

				if ( line.empty() )
				{
					break;
				}

				const auto colon = line.find(':');

				if ( colon == std::string::npos || colon == 0 )
				{
					return false;
				}

				auto name = lowercase(line.substr(0, colon));
				auto value = String::trim(line.substr(colon + 1));

				/* NOTE: two framing or identity headers that disagree are how requests are smuggled past a
				 * check: refuse any duplicate of them instead of choosing one. */
				if ( request.headers.contains(name) )
				{
					if ( name == "content-length" || name == "host" || name == "authorization" || name == "origin" || name == "transfer-encoding" )
					{
						return false;
					}

					continue;
				}

				request.headers.emplace(std::move(name), std::move(value));
			}

			return true;
		}

		/**
		 * @brief Returns an SSE event carrying one JSON-RPC message.
		 * @param json The serialized message.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		sseEvent (const std::string & json) noexcept
		{
			return "event: message\ndata: " + json + "\n\n";
		}
	}

	/**
	 * @brief One HTTP connection. Every member is touched on the network thread only.
	 */
	class Connection final : public std::enable_shared_from_this< Connection >
	{
		public:

			/**
			 * @brief Constructs a connection.
			 * @param server The owning server.
			 * @param socket The accepted socket [std::move].
			 */
			Connection (Server & server, asio::ip::tcp::socket socket) noexcept
				: m_server{server},
				m_socket{std::move(socket)},
				m_timer{m_socket.get_executor()}
			{

			}

			/**
			 * @brief Starts reading the first request.
			 * @return void
			 */
			void
			start () noexcept
			{
				this->readRequest();
			}

			/**
			 * @brief Closes the socket and forgets the connection.
			 * @return void
			 */
			void
			close () noexcept
			{
				if ( m_closed )
				{
					return;
				}

				m_closed = true;

				asio::error_code ec;
				m_timer.cancel();
				m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
				m_socket.close(ec);

				m_server.removeConnection(this->shared_from_this());
			}

			/**
			 * @brief Answers the request in flight with a JSON body, then reads the next one (or closes).
			 * @param status The HTTP status.
			 * @param body The serialized JSON.
			 * @return void
			 */
			void
			respondJSON (int status, const std::string & body) noexcept
			{
				std::string response = "HTTP/1.1 " + std::to_string(status) + " " + reasonPhrase(status) + "\r\n"
					"Content-Type: application/json\r\n"
					"Cache-Control: no-store\r\n"
					"Content-Length: " + std::to_string(body.size()) + "\r\n" +
					( m_keepAlive ? "Connection: keep-alive\r\n" : "Connection: close\r\n" ) +
					"\r\n" + body;

				this->finishWith(std::move(response));
			}

			/**
			 * @brief Answers the request in flight with no body (202 for a notification, 405, …).
			 * @param status The HTTP status.
			 * @param extraHeaders Complete header lines, each ending with CRLF.
			 * @return void
			 */
			void
			respondEmpty (int status, const std::string & extraHeaders = {}) noexcept
			{
				std::string response = "HTTP/1.1 " + std::to_string(status) + " " + reasonPhrase(status) + "\r\n" +
					extraHeaders +
					"Content-Length: 0\r\n" +
					( m_keepAlive ? "Connection: keep-alive\r\n" : "Connection: close\r\n" ) +
					"\r\n";

				this->finishWith(std::move(response));
			}

			/**
			 * @brief Sends one notification on a notification stream.
			 * @param json The serialized JSON-RPC notification.
			 * @return void
			 */
			void
			sendEvent (const std::string & json) noexcept
			{
				if ( m_streaming && !m_closed )
				{
					this->write(sseEvent(json));
				}
			}

			/**
			 * @brief Ends a notification stream gracefully: a modern subscription gets its final response.
			 * @note Synchronous, for the shutdown path only (the network thread is about to stop).
			 * @return void
			 */
			void
			closeStreamGracefully () noexcept
			{
				if ( m_streaming && m_streamModern && !m_closed )
				{
					Json::Value result{Json::objectValue};
					result["_meta"][MetaSubscriptionIdKey] = m_subscriptionId;

					const auto closure = sseEvent(serialize(makeResult(m_subscriptionId, std::move(result), true)));

					asio::error_code ec;
					static_cast< void >(asio::write(m_socket, asio::buffer(closure), ec));
				}

				this->close();
			}

			/**
			 * @brief Returns whether this connection is a notification stream that asked for list changes.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			wantsToolsListChanged () const noexcept
			{
				return m_streaming && m_toolsListChanged;
			}

			/**
			 * @brief Returns whether this connection is a modern (`subscriptions/listen`) stream.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isModernStream () const noexcept
			{
				return m_streaming && m_streamModern;
			}

			/**
			 * @brief Returns the subscription id of a modern stream.
			 * @return const Json::Value &
			 */
			[[nodiscard]]
			const Json::Value &
			subscriptionId () const noexcept
			{
				return m_subscriptionId;
			}

		private:

			/** @brief What the single timer is doing. */
			enum class TimerRole : uint8_t
			{
				RequestDeadline,
				Idle,
				KeepAlive
			};

			/**
			 * @brief Arms the timer.
			 * @param seconds The delay.
			 * @param role What expiring means.
			 * @return void
			 */
			void
			armTimer (uint32_t seconds, TimerRole role) noexcept
			{
				m_timerRole = role;
				m_timer.expires_after(std::chrono::seconds{seconds});

				auto self = this->shared_from_this();

				m_timer.async_wait([self, role] (const asio::error_code & ec) {
					if ( ec || self->m_closed || self->m_timerRole != role )
					{
						return;
					}

					if ( role == TimerRole::KeepAlive )
					{
						/* An SSE comment: keeps intermediaries and client idle timers from closing a quiet stream. */
						self->write(":\n\n");
						self->armTimer(Server::StreamKeepAliveSeconds, TimerRole::KeepAlive);

						return;
					}

					self->close();
				});
			}

			/**
			 * @brief Reads the next request: its head, then its body.
			 * @return void
			 */
			void
			readRequest () noexcept
			{
				if ( m_closed )
				{
					return;
				}

				/* Idle until the first byte, then the whole request must arrive in time (slowloris). */
				this->armTimer(m_buffer.size() > 0 ? Server::RequestTimeoutSeconds : Server::IdleTimeoutSeconds, m_buffer.size() > 0 ? TimerRole::RequestDeadline : TimerRole::Idle);

				auto self = this->shared_from_this();

				asio::async_read_until(m_socket, m_buffer, "\r\n\r\n", [self] (const asio::error_code & ec, std::size_t headBytes) {
					if ( self->m_closed )
					{
						return;
					}

					if ( ec )
					{
						if ( ec == asio::error::not_found )
						{
							self->m_keepAlive = false;
							self->respondEmpty(431);

							return;
						}

						self->close();

						return;
					}

					self->armTimer(Server::RequestTimeoutSeconds, TimerRole::RequestDeadline);

					if ( headBytes > Server::MaxHeaderBytes )
					{
						self->m_keepAlive = false;
						self->respondEmpty(431);

						return;
					}

					const std::string head{asio::buffers_begin(self->m_buffer.data()), asio::buffers_begin(self->m_buffer.data()) + static_cast< std::ptrdiff_t >(headBytes)};
					self->m_buffer.consume(headBytes);

					self->m_request = HTTPRequest{};

					if ( !parseHead(head, self->m_request) )
					{
						self->m_keepAlive = false;
						self->respondEmpty(400);

						return;
					}

					self->m_keepAlive = self->m_request.version == "HTTP/1.1" && lowercase(header(self->m_request, "connection")) != "close";

					if ( !header(self->m_request, "transfer-encoding").empty() )
					{
						self->m_keepAlive = false;
						self->respondEmpty(501);

						return;
					}

					size_t contentLength = 0;

					if ( const auto lengthText = header(self->m_request, "content-length"); !lengthText.empty() )
					{
						/* Digits only, and parsed with an explicit bound: no sign, no overflow, no locale. */
						if ( lengthText.size() > 19 || !std::ranges::all_of(lengthText, [] (char character) { return character >= '0' && character <= '9'; }) )
						{
							self->m_keepAlive = false;
							self->respondEmpty(400);

							return;
						}

						uint64_t parsed = 0;

						for ( const auto character : lengthText )
						{
							parsed = (parsed * 10) + static_cast< uint64_t >(character - '0');
						}

						if ( parsed > Server::MaxBodyBytes )
						{
							self->m_keepAlive = false;
							self->respondEmpty(413);

							return;
						}

						contentLength = static_cast< size_t >(parsed);
					}
					else if ( self->m_request.method == "POST" )
					{
						self->m_keepAlive = false;
						self->respondEmpty(411);

						return;
					}

					self->readBody(contentLength);
				});
			}

			/**
			 * @brief Reads the body of the current request, then handles it.
			 * @param contentLength The body size.
			 * @return void
			 */
			void
			readBody (size_t contentLength) noexcept
			{
				if ( m_buffer.size() >= contentLength )
				{
					m_request.body.assign(asio::buffers_begin(m_buffer.data()), asio::buffers_begin(m_buffer.data()) + static_cast< std::ptrdiff_t >(contentLength));
					m_buffer.consume(contentLength);

					this->handleRequest();

					return;
				}

				auto self = this->shared_from_this();

				asio::async_read(m_socket, m_buffer, asio::transfer_exactly(contentLength - m_buffer.size()), [self, contentLength] (const asio::error_code & ec, std::size_t /*bytes*/) {
					if ( self->m_closed )
					{
						return;
					}

					if ( ec )
					{
						self->close();

						return;
					}

					self->readBody(contentLength);
				});
			}

			/**
			 * @brief Applies the HTTP-level checks, then dispatches by method.
			 * @return void
			 */
			void
			handleRequest () noexcept
			{
				m_timer.cancel();

				/* ⚠️ Host and Origin BEFORE anything else: a page loaded in a browser — CEF included, it runs
				 * in this very process — can reach 127.0.0.1, and DNS rebinding can make it look same-origin. */
				if ( !m_server.isAcceptedHost(header(m_request, "host")) )
				{
					this->respondError(403, Json::Value{Json::nullValue}, InvalidRequestCode, "Host not accepted.");

					return;
				}

				if ( const auto origin = header(m_request, "origin"); !origin.empty() && !m_server.isAcceptedOrigin(origin) )
				{
					this->respondError(403, Json::Value{Json::nullValue}, InvalidRequestCode, "Origin not accepted.");

					return;
				}

				if ( !m_server.isAuthorized(header(m_request, "authorization")) )
				{
					this->respondEmpty(401, "WWW-Authenticate: Bearer\r\n");

					return;
				}

				auto path = m_request.target;

				if ( const auto query = path.find('?'); query != std::string::npos )
				{
					path.resize(query);
				}

				if ( path != Server::EndpointPath )
				{
					this->respondEmpty(404);

					return;
				}

				if ( m_request.method == "POST" )
				{
					this->handlePost();

					return;
				}

				if ( m_request.method == "GET" && header(m_request, "accept").find("text/event-stream") != std::string::npos )
				{
					/* The handshake era's server-to-client stream: list changes only, no subscription id. */
					this->startStream(false, Json::Value{Json::nullValue}, true);

					return;
				}

				this->respondEmpty(405, "Allow: GET, POST\r\n");
			}

			/**
			 * @brief Answers a JSON-RPC error.
			 * @param status The HTTP status.
			 * @param id The request id.
			 * @param code The JSON-RPC error code.
			 * @param message The message.
			 * @param data Optional error data.
			 * @return void
			 */
			void
			respondError (int status, const Json::Value & id, int code, const std::string & message, const Json::Value & data = Json::Value{Json::nullValue}) noexcept
			{
				this->respondJSON(status, serialize(makeError(id, code, message, data)));
			}

			/**
			 * @brief Handles one JSON-RPC message (validation, era, dispatch).
			 * @return void
			 */
			void
			handlePost () noexcept
			{
				if ( !lowercase(header(m_request, "content-type")).starts_with("application/json") )
				{
					this->respondEmpty(415);

					return;
				}

				auto parsed = FastJSON::getRootFromString(m_request.body, 64, true);

				if ( !parsed.has_value() )
				{
					this->respondError(400, Json::Value{Json::nullValue}, ParseErrorCode, "Parse error: the body is not a JSON document.");

					return;
				}

				const auto & message = *parsed;

				if ( message.isArray() )
				{
					this->respondError(400, Json::Value{Json::nullValue}, InvalidRequestCode, "JSON-RPC batches are not supported.");

					return;
				}

				/* ⚠️ Every read of the client's JSON goes through member()/stringMember(): see Protocol.hpp. */
				const auto & rawId = member(message, "id");
				const auto safeId = rawId.isString() || rawId.isIntegral() ? rawId : Json::Value{Json::nullValue};

				if ( !message.isObject() || stringMember(message, "jsonrpc") != "2.0" || !member(message, "method").isString() )
				{
					this->respondError(400, safeId, InvalidRequestCode, "Not a JSON-RPC 2.0 request.");

					return;
				}

				const auto method = stringMember(message, "method");
				const auto & params = member(message, "params");

				if ( !params.isNull() && !params.isObject() )
				{
					this->respondError(400, safeId, InvalidRequestCode, "'params' must be an object.");

					return;
				}

				/* A notification (no id) is accepted and not answered: initialized, cancelled, … */
				if ( !message.isMember("id") )
				{
					this->respondEmpty(202);

					return;
				}

				const auto & id = rawId;

				if ( !id.isString() && !id.isIntegral() )
				{
					this->respondError(400, Json::Value{Json::nullValue}, InvalidRequestCode, "The request id must be a string or an integer.");

					return;
				}

				if ( method == "initialize" )
				{
					this->handleInitialize(id, params);

					return;
				}

				const auto headerVersion = header(m_request, "mcp-protocol-version");
				const auto & metaVersion = member(member(params, "_meta"), MetaProtocolVersionKey);
				const auto modern = metaVersion.isString() || headerVersion == ModernVersion;

				if ( modern )
				{
					if ( headerVersion.empty() )
					{
						this->respondError(400, id, HeaderMismatchCode, "The MCP-Protocol-Version header is required.");

						return;
					}

					if ( !metaVersion.isString() )
					{
						this->respondError(400, id, InvalidParamsCode, std::string{"The request lacks _meta."} + MetaProtocolVersionKey + ".");

						return;
					}

					if ( metaVersion.asString() != headerVersion )
					{
						this->respondError(400, id, HeaderMismatchCode, "The MCP-Protocol-Version header does not match _meta.");

						return;
					}

					if ( headerVersion != ModernVersion )
					{
						Json::Value data{Json::objectValue};
						data["supported"] = supportedVersions();
						data["requested"] = headerVersion;

						this->respondError(400, id, UnsupportedProtocolVersionCode, "Unsupported protocol version", data);

						return;
					}

					if ( decodeHeaderValue(header(m_request, "mcp-method")) != method )
					{
						this->respondError(400, id, HeaderMismatchCode, "The Mcp-Method header is missing or does not match the body.");

						return;
					}

					if ( method == "tools/call" && decodeHeaderValue(header(m_request, "mcp-name")) != stringMember(params, "name") )
					{
						this->respondError(400, id, HeaderMismatchCode, "The Mcp-Name header is missing or does not match the body.");

						return;
					}
				}
				else if ( !headerVersion.empty() && !isLegacyVersion(headerVersion) )
				{
					Json::Value data{Json::objectValue};
					data["supported"] = supportedVersions();
					data["requested"] = headerVersion;

					this->respondError(400, id, UnsupportedProtocolVersionCode, "Unsupported protocol version", data);

					return;
				}

				if ( method == "server/discover" )
				{
					Json::Value capabilities{Json::objectValue};
					capabilities["tools"]["listChanged"] = true;

					Json::Value result{Json::objectValue};
					result["supportedVersions"] = supportedVersions();
					result["capabilities"] = std::move(capabilities);
					result["instructions"] = instructions();
					result["ttlMs"] = 3600000;
					result["cacheScope"] = "private";

					this->respondJSON(200, serialize(makeResult(id, std::move(result), true)));

					return;
				}

				if ( method == "ping" )
				{
					this->respondJSON(200, serialize(makeResult(id, Json::Value{Json::objectValue}, modern)));

					return;
				}

				if ( method == "subscriptions/listen" && modern )
				{
					const auto toolsListChanged = boolMember(member(params, "notifications"), "toolsListChanged", false);

					this->startStream(true, id, toolsListChanged);

					return;
				}

				if ( method == "tools/list" || method == "tools/call" )
				{
					if ( method == "tools/call" && !member(params, "name").isString() )
					{
						this->respondError(200, id, InvalidParamsCode, "tools/call needs a 'name' string.");

						return;
					}

					Server::PendingRequest request;
					request.connection = this->shared_from_this();
					request.id = id;
					request.method = method;
					request.params = params;
					request.modern = modern;

					if ( !m_server.enqueue(std::move(request)) )
					{
						this->respondError(503, id, InternalErrorCode, "The engine is busy: too many requests waiting for its main thread.");
					}

					/* NOTE: answered later, from processPendingRequests(), through the network thread. */
					return;
				}

				/* The specification answers an unknown method with 404 in the modern era. */
				this->respondError(modern ? 404 : 200, id, MethodNotFoundCode, "Method not found: " + method);
			}

			/**
			 * @brief Answers the handshake era's `initialize`.
			 * @param id The request id.
			 * @param params The request parameters.
			 * @return void
			 */
			void
			handleInitialize (const Json::Value & id, const Json::Value & params) noexcept
			{
				const auto requested = stringMember(params, "protocolVersion");

				Json::Value serverInfo{Json::objectValue};
				serverInfo["name"] = ServerName;
				serverInfo["version"] = VersionString;

				Json::Value result{Json::objectValue};
				/* A requested version we serve is echoed; otherwise the newest handshake-era one is proposed. */
				result["protocolVersion"] = isLegacyVersion(requested) ? requested : std::string{LegacyVersions.front()};
				result["capabilities"]["tools"]["listChanged"] = true;
				result["serverInfo"] = std::move(serverInfo);
				result["instructions"] = instructions();

				this->respondJSON(200, serialize(makeResult(id, std::move(result), false)));
			}

			/**
			 * @brief Turns this connection into a notification stream.
			 * @param modern Whether it is a `subscriptions/listen` stream (acknowledged, tagged).
			 * @param subscriptionId The subscription id (the request id), for a modern stream.
			 * @param toolsListChanged Whether list changes are wanted.
			 * @return void
			 */
			void
			startStream (bool modern, const Json::Value & subscriptionId, bool toolsListChanged) noexcept
			{
				m_streaming = true;
				m_streamModern = modern;
				m_subscriptionId = subscriptionId;
				m_toolsListChanged = toolsListChanged;

				this->write(
					"HTTP/1.1 200 OK\r\n"
					"Content-Type: text/event-stream\r\n"
					"Cache-Control: no-cache\r\n"
					"X-Accel-Buffering: no\r\n"
					"Connection: keep-alive\r\n"
					"\r\n"
				);

				if ( modern )
				{
					Json::Value acknowledged{Json::objectValue};
					acknowledged["jsonrpc"] = "2.0";
					acknowledged["method"] = "notifications/subscriptions/acknowledged";
					acknowledged["params"]["_meta"][MetaSubscriptionIdKey] = subscriptionId;

					/* Only what is honoured: this server announces tool list changes, nothing else. */
					acknowledged["params"]["notifications"] = Json::Value{Json::objectValue};

					if ( toolsListChanged )
					{
						acknowledged["params"]["notifications"]["toolsListChanged"] = true;
					}

					this->write(sseEvent(serialize(acknowledged)));
				}

				this->armTimer(Server::StreamKeepAliveSeconds, TimerRole::KeepAlive);
				this->watchForClose();
			}

			/**
			 * @brief On a stream, reads only to learn that the client went away (closing = cancelling).
			 * @return void
			 */
			void
			watchForClose () noexcept
			{
				auto self = this->shared_from_this();

				m_socket.async_read_some(asio::buffer(m_scratch), [self] (const asio::error_code & ec, std::size_t /*bytes*/) {
					if ( self->m_closed )
					{
						return;
					}

					if ( ec )
					{
						self->close();

						return;
					}

					self->watchForClose();
				});
			}

			/**
			 * @brief Writes a whole response, then reads the next request or closes.
			 * @param response The response bytes [std::move].
			 * @return void
			 */
			void
			finishWith (std::string response) noexcept
			{
				m_closeAfterFlush = !m_keepAlive;
				m_readAfterFlush = m_keepAlive;

				this->write(std::move(response));
			}

			/**
			 * @brief Queues bytes; one asynchronous write runs at a time.
			 * @param bytes The bytes [std::move].
			 * @return void
			 */
			void
			write (std::string bytes) noexcept
			{
				if ( m_closed )
				{
					return;
				}

				m_writeQueue.emplace_back(std::move(bytes));

				if ( !m_writing )
				{
					this->writeNext();
				}
			}

			/**
			 * @brief Writes the next queued bytes.
			 * @return void
			 */
			void
			writeNext () noexcept
			{
				if ( m_writeQueue.empty() )
				{
					m_writing = false;

					if ( m_closeAfterFlush )
					{
						this->close();
					}
					else if ( m_readAfterFlush )
					{
						m_readAfterFlush = false;

						this->readRequest();
					}

					return;
				}

				m_writing = true;

				auto self = this->shared_from_this();

				asio::async_write(m_socket, asio::buffer(m_writeQueue.front()), [self] (const asio::error_code & ec, std::size_t /*bytes*/) {
					if ( self->m_closed )
					{
						return;
					}

					if ( ec )
					{
						self->close();

						return;
					}

					self->m_writeQueue.pop_front();
					self->writeNext();
				});
			}

			Server & m_server;
			asio::ip::tcp::socket m_socket;
			asio::steady_timer m_timer;
			asio::streambuf m_buffer{Server::MaxHeaderBytes + Server::MaxBodyBytes};
			std::array< char, 512 > m_scratch{};
			HTTPRequest m_request;
			std::deque< std::string > m_writeQueue;
			Json::Value m_subscriptionId;
			TimerRole m_timerRole{TimerRole::Idle};
			bool m_keepAlive{true};
			bool m_writing{false};
			bool m_closeAfterFlush{false};
			bool m_readAfterFlush{false};
			bool m_streaming{false};
			bool m_streamModern{false};
			bool m_toolsListChanged{false};
			bool m_closed{false};
	};

	Server::Server (const std::string & address, uint16_t port, std::string bearerToken) noexcept
		: m_address{address},
		m_bearerToken{std::move(bearerToken)},
		m_announcedTreeRevision{Controller::consoleTreeRevision()},
		m_port{port}
	{
		asio::error_code ec;

		const auto bindAddress = asio::ip::make_address(address, ec);

		if ( ec )
		{
			TraceError{ClassId} << "Invalid MCP bind address '" << address << "': " << ec.message() << ". The MCP server stays closed.";

			return;
		}

		m_loopback = bindAddress.is_loopback();

		if ( !m_loopback && m_bearerToken.empty() )
		{
			TraceError{ClassId} << "The MCP server refuses to listen on the non-loopback address '" << address << "' without a bearer token (Core/MCP/BearerToken). It stays closed.";

			return;
		}

		const asio::ip::tcp::endpoint endpoint{bindAddress, port};

		m_acceptor = std::make_unique< asio::ip::tcp::acceptor >(m_ioContext);
		m_acceptor->open(endpoint.protocol(), ec);

		if ( !ec )
		{
			/* NOTE: a failure here only costs the fast restart on the same port: a warning, then bind. */
			m_acceptor->set_option(asio::socket_base::reuse_address(true), ec);

			if ( ec )
			{
				TraceWarning{ClassId} << "Unable to set reuse_address on the MCP acceptor: " << ec.message();

				ec.clear();
			}

			m_acceptor->bind(endpoint, ec);
		}

		if ( !ec )
		{
			m_acceptor->listen(asio::socket_base::max_listen_connections, ec);
		}

		if ( ec )
		{
			/* ⚠️ Windows reports a port another process holds exclusively (SO_EXCLUSIVEADDRUSE) as ACCESS DENIED,
			 * not "address in use": measured on an ASUS laptop whose Armoury Crate listens on 127.0.0.1:7778. */
			const auto * const hint = ( ec == asio::error::address_in_use || ec == asio::error::access_denied ) ?
				" — the port is most likely taken by another process (Windows reports that as access denied); choose another one in Core/MCP/Port" :
				"";

			TraceError{ClassId} << "Unable to listen on " << address << ':' << port << ": " << ec.message() << hint << ". The MCP server stays closed.";

			m_acceptor.reset();

			return;
		}

		m_workGuard.emplace(asio::make_work_guard(m_ioContext));
		m_running = true;

		this->accept();

		m_networkThread = std::thread([this] () {
			m_ioContext.run();
		});

		TraceSuccess{ClassId} << "MCP server listening on " << this->endpoint() << " (protocol " << ModernVersion << " and " << LegacyVersions.front() << " era).";
	}

	Server::~Server ()
	{
		if ( !m_running )
		{
			return;
		}

		/* Streams get their graceful end and every socket closes ON the network thread (the only one that
		 * touches them), then the loop is released. Bounded: a stuck peer cannot hold the shutdown. */
		std::mutex doneMutex;
		std::condition_variable doneSignal;
		bool done = false;

		asio::post(m_ioContext, [this, &doneMutex, &doneSignal, &done] () {
			asio::error_code ec;

			if ( m_acceptor != nullptr )
			{
				m_acceptor->cancel(ec);
				m_acceptor->close(ec);
			}

			/* NOTE: a copy — closing a connection removes it from the set. */
			const auto connections = m_connections;

			for ( const auto & connection : connections )
			{
				connection->closeStreamGracefully();
			}

			{
				const std::scoped_lock lock{doneMutex};

				done = true;
			}

			doneSignal.notify_one();
		});

		{
			std::unique_lock< std::mutex > lock{doneMutex};

			doneSignal.wait_for(lock, std::chrono::seconds{3}, [&done] () {
				return done;
			});
		}

		m_workGuard.reset();
		m_ioContext.stop();

		if ( m_networkThread.joinable() )
		{
			m_networkThread.join();
		}

		/* Handlers that never ran hold connections; drop them now, on this (the last) thread. */
		m_connections.clear();

		TraceInfo{ClassId} << "MCP server stopped.";
	}

	bool
	Server::isRunning () const noexcept
	{
		return m_running;
	}

	std::string
	Server::endpoint () const noexcept
	{
		if ( !m_running )
		{
			return {};
		}

		const auto host = m_address.find(':') != std::string::npos ? "[" + m_address + "]" : m_address;

		return "http://" + host + ":" + std::to_string(m_port) + EndpointPath;
	}

	void
	Server::accept () noexcept
	{
		m_acceptor->async_accept([this] (const asio::error_code & ec, asio::ip::tcp::socket socket) {
			if ( ec )
			{
				if ( ec == asio::error::operation_aborted )
				{
					return;
				}

				TraceError{ClassId} << "MCP accept failed: " << ec.message() << ".";

				/* Never re-arm blindly on a permanent failure: it would spin this thread. */
				if ( ec == asio::error::no_descriptors || ec == asio::error::no_memory )
				{
					return;
				}

				this->accept();

				return;
			}

			asio::error_code optionError;
			socket.set_option(asio::ip::tcp::no_delay{true}, optionError);

			if ( optionError )
			{
				TraceWarning{ClassId} << "Unable to disable Nagle's algorithm for an MCP client: " << optionError.message();
			}

			if ( m_connections.size() >= MaxConnections )
			{
				constexpr std::string_view Refusal{"HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"};

				asio::error_code writeError;
				static_cast< void >(asio::write(socket, asio::buffer(Refusal), writeError));
				socket.close(writeError);
			}
			else
			{
				auto connection = std::make_shared< Connection >(*this, std::move(socket));

				m_connections.insert(connection);

				connection->start();
			}

			this->accept();
		});
	}

	bool
	Server::enqueue (PendingRequest request) noexcept
	{
		const std::scoped_lock lock{m_queueMutex};

		if ( m_pendingRequests.size() >= MaxPendingRequests )
		{
			return false;
		}

		m_pendingRequests.emplace_back(std::move(request));

		return true;
	}

	void
	Server::removeConnection (const std::shared_ptr< Connection > & connection) noexcept
	{
		m_connections.erase(connection);
	}

	void
	Server::broadcastToolsListChanged () noexcept
	{
		for ( const auto & connection : m_connections )
		{
			if ( !connection->wantsToolsListChanged() )
			{
				continue;
			}

			Json::Value notification{Json::objectValue};
			notification["jsonrpc"] = "2.0";
			notification["method"] = "notifications/tools/list_changed";

			if ( connection->isModernStream() )
			{
				notification["params"]["_meta"][MetaSubscriptionIdKey] = connection->subscriptionId();
			}

			connection->sendEvent(serialize(notification));
		}
	}

	void
	Server::processPendingRequests (const Controller & controller) noexcept
	{
		if ( !m_running )
		{
			return;
		}

		/* A changed tree (the active scene's PostProcess node, an act switching) is announced once. */
		if ( const auto revision = Controller::consoleTreeRevision(); revision != m_announcedTreeRevision )
		{
			m_announcedTreeRevision = revision;

			asio::post(m_ioContext, [this] () {
				this->broadcastToolsListChanged();
			});
		}

		std::vector< PendingRequest > requests;

		{
			const std::scoped_lock lock{m_queueMutex};

			requests.swap(m_pendingRequests);
		}

		if ( requests.empty() )
		{
			return;
		}

		std::vector< std::string > rejections;
		auto tools = collectTools(controller, rejections);
		auto toolsRevision = Controller::consoleTreeRevision();

		if ( rejections != m_reportedRejections )
		{
			for ( const auto & rejection : rejections )
			{
				TraceWarning{ClassId} << "Command not exposed as a tool: " << rejection;
			}

			m_reportedRejections = rejections;
		}

		for ( auto & request : requests )
		{
			/* ⚠️ A command run by the previous request may have changed the tree (a scene switch destroys the
			 * PostProcess node): the Command pointers of the list would dangle. Rebuild it first. */
			if ( Controller::consoleTreeRevision() != toolsRevision )
			{
				rejections.clear();
				tools = collectTools(controller, rejections);
				toolsRevision = Controller::consoleTreeRevision();
			}

			if ( request.method == "tools/list" )
			{
				Json::Value list{Json::arrayValue};

				for ( const auto & tool : tools )
				{
					list.append(toolDefinition(tool));
				}

				Json::Value result{Json::objectValue};
				result["tools"] = std::move(list);

				if ( request.modern )
				{
					/* NOTE: short on purpose — the list follows the active scene, and a client that did not
					 * subscribe to list changes should not keep a stale one for long. */
					result["ttlMs"] = 10000;
					result["cacheScope"] = "private";
				}

				auto body = serialize(makeResult(request.id, std::move(result), request.modern));

				asio::post(m_ioContext, [connection = request.connection, body = std::move(body)] () {
					if ( auto alive = connection.lock() )
					{
						alive->respondJSON(200, body);
					}
				});

				continue;
			}

			/* tools/call */
			const auto name = stringMember(request.params, "name");
			const auto toolIt = std::ranges::find_if(tools, [&name] (const Tool & tool) {
				return tool.name() == name;
			});

			if ( toolIt == tools.end() )
			{
				auto body = serialize(makeError(request.id, InvalidParamsCode, "Unknown tool: " + name));

				asio::post(m_ioContext, [connection = request.connection, body = std::move(body)] () {
					if ( auto alive = connection.lock() )
					{
						alive->respondJSON(200, body);
					}
				});

				continue;
			}

			Arguments arguments;
			std::string error;
			Outputs outputs;
			bool succeeded = false;

			if ( jsonToArguments(toolIt->command()->signature(), member(request.params, "arguments"), arguments, error) )
			{
				succeeded = toolIt->command()->binding()(arguments, outputs);
			}
			else
			{
				/* An input error is a tool result the model can correct, not a protocol error. */
				outputs.emplace_back(Severity::Error, error);
			}

			/* The result is built on the network thread: an image output is read and reduced there, never
			 * on the main thread between two frames. */
			asio::post(m_ioContext, [connection = request.connection, id = request.id, modern = request.modern, succeeded, outputs = std::move(outputs)] () {
				if ( auto alive = connection.lock() )
				{
					alive->respondJSON(200, serialize(makeResult(id, callResult(succeeded, outputs, modern), modern)));
				}
			});
		}
	}

	bool
	Server::isAcceptedHost (const std::string & host) const noexcept
	{
		/* NOTE: only a loopback binding can know its legitimate names; a non-loopback one is protected by
		 * its mandatory bearer token instead. */
		if ( !m_loopback )
		{
			return true;
		}

		const auto port = std::to_string(m_port);

		return host == "127.0.0.1:" + port || host == "localhost:" + port || host == "[::1]:" + port;
	}

	bool
	Server::isAcceptedOrigin (const std::string & origin) const noexcept
	{
		const auto port = std::to_string(m_port);

		return origin == "http://127.0.0.1:" + port || origin == "http://localhost:" + port || origin == "http://[::1]:" + port;
	}

	bool
	Server::isAuthorized (const std::string & authorization) const noexcept
	{
		if ( m_bearerToken.empty() )
		{
			return true;
		}

		const std::string expected = "Bearer " + m_bearerToken;

		/* Constant time over the expected length: the comparison must not leak how many characters matched. */
		unsigned char difference = authorization.size() == expected.size() ? 0 : 1;

		for ( size_t index = 0; index < expected.size(); ++index )
		{
			const auto actual = index < authorization.size() ? authorization[index] : '\0';

			difference |= static_cast< unsigned char >(actual ^ expected[index]);
		}

		return difference == 0;
	}
}
