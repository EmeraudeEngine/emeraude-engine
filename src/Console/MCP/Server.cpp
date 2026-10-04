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
#include <string_view>
#include <utility>

/* Local inclusions. */
#include "emeraude_config.hpp"
#include "Console/Controller.hpp"
#include "FastJSON.hpp"
#include "Protocol.hpp"
#include "Tracer.hpp"

namespace EmEn::Console::MCP
{
	using namespace Base;

	namespace
	{
		/** @brief Every JSON answer is private and short-lived. */
		constexpr std::string_view NoStore{"Cache-Control: no-store\r\n"};

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

		/**
		 * @brief Answers a request with a JSON body.
		 * @param connection The connection.
		 * @param status The HTTP status.
		 * @param body The serialized JSON.
		 * @return void
		 */
		void
		respondJSON (Network::HTTPServerConnection & connection, int status, std::string_view body) noexcept
		{
			connection.respond(status, "application/json", body, NoStore);
		}

		/**
		 * @brief Answers a JSON-RPC error.
		 * @param connection The connection.
		 * @param status The HTTP status.
		 * @param id The request id.
		 * @param code The JSON-RPC error code.
		 * @param message The message.
		 * @param data Optional error data.
		 * @return void
		 */
		void
		respondError (Network::HTTPServerConnection & connection, int status, const Json::Value & id, int code, const std::string & message, const Json::Value & data = Json::Value{Json::nullValue}) noexcept
		{
			respondJSON(connection, status, serialize(makeError(id, code, message, data)));
		}

		/**
		 * @brief Returns the HTTP options of the MCP server.
		 * @param address The bind address.
		 * @param port The TCP port.
		 * @param bearerToken The bearer token [std::move].
		 * @return Network::HTTPServerOptions
		 */
		[[nodiscard]]
		Network::HTTPServerOptions
		httpOptions (const std::string & address, uint16_t port, std::string bearerToken) noexcept
		{
			Network::HTTPServerOptions options;
			options.address = address;
			options.port = port;
			options.bearerToken = std::move(bearerToken);
			options.name = "MCP server";
			options.maxHeaderBytes = Server::MaxHeaderBytes;
			options.maxBodyBytes = Server::MaxBodyBytes;
			options.maxConnections = Server::MaxConnections;
			options.requestTimeoutSeconds = Server::RequestTimeoutSeconds;
			options.idleTimeoutSeconds = Server::IdleTimeoutSeconds;

			return options;
		}
	}

	Server::Server (const std::string & address, uint16_t port, std::string bearerToken) noexcept
		: m_http{httpOptions(address, port, std::move(bearerToken))},
		m_announcedTreeRevision{Controller::consoleTreeRevision()}
	{
		const auto started = m_http.start(
			[this] (const std::shared_ptr< Connection > & connection) {
				this->handleRequest(connection);
			},
			[this] (Connection & connection) {
				this->closeStreamGracefully(connection);
			},
			[this] (uint64_t connectionId) {
				m_streams.erase(connectionId);
			}
		);

		if ( !started )
		{
			TraceError{ClassId} << "The MCP server stays closed (see the error above; Core/MCP/Address, Port and BearerToken).";

			return;
		}

		TraceSuccess{ClassId} << "MCP server listening on " << this->endpoint() << " (protocol " << ModernVersion << " and " << LegacyVersions.front() << " era).";
	}

	Server::~Server ()
	{
		if ( !m_http.isRunning() )
		{
			return;
		}

		m_http.stop();

		TraceInfo{ClassId} << "MCP server stopped.";
	}

	bool
	Server::isRunning () const noexcept
	{
		return m_http.isRunning();
	}

	std::string
	Server::endpoint () const noexcept
	{
		if ( !m_http.isRunning() )
		{
			return {};
		}

		return m_http.baseURL() + EndpointPath;
	}

	void
	Server::handleRequest (const std::shared_ptr< Connection > & connection) noexcept
	{
		const auto & request = connection->request();

		if ( request.path() != EndpointPath )
		{
			connection->respondEmpty(404);

			return;
		}

		if ( request.method == "POST" )
		{
			this->handlePost(connection);

			return;
		}

		if ( request.method == "GET" && request.header("accept").find("text/event-stream") != std::string::npos )
		{
			/* The handshake era's server-to-client stream: list changes only, no subscription id. */
			this->startStream(*connection, false, Json::Value{Json::nullValue}, true);

			return;
		}

		connection->respondEmpty(405, "Allow: GET, POST\r\n");
	}

	void
	Server::handlePost (const std::shared_ptr< Connection > & connection) noexcept
	{
		const auto & request = connection->request();

		if ( !lowercase(request.header("content-type")).starts_with("application/json") )
		{
			connection->respondEmpty(415);

			return;
		}

		auto parsed = FastJSON::getRootFromString(request.body, 64, true);

		if ( !parsed.has_value() )
		{
			respondError(*connection, 400, Json::Value{Json::nullValue}, ParseErrorCode, "Parse error: the body is not a JSON document.");

			return;
		}

		const auto & message = *parsed;

		if ( message.isArray() )
		{
			respondError(*connection, 400, Json::Value{Json::nullValue}, InvalidRequestCode, "JSON-RPC batches are not supported.");

			return;
		}

		/* ⚠️ Every read of the client's JSON goes through member()/stringMember(): see Protocol.hpp. */
		const auto & rawId = member(message, "id");
		const auto safeId = rawId.isString() || rawId.isIntegral() ? rawId : Json::Value{Json::nullValue};

		if ( !message.isObject() || stringMember(message, "jsonrpc") != "2.0" || !member(message, "method").isString() )
		{
			respondError(*connection, 400, safeId, InvalidRequestCode, "Not a JSON-RPC 2.0 request.");

			return;
		}

		const auto method = stringMember(message, "method");
		const auto & params = member(message, "params");

		if ( !params.isNull() && !params.isObject() )
		{
			respondError(*connection, 400, safeId, InvalidRequestCode, "'params' must be an object.");

			return;
		}

		/* A notification (no id) is accepted and not answered: initialized, cancelled, … */
		if ( !message.isMember("id") )
		{
			connection->respondEmpty(202);

			return;
		}

		const auto & id = rawId;

		if ( !id.isString() && !id.isIntegral() )
		{
			respondError(*connection, 400, Json::Value{Json::nullValue}, InvalidRequestCode, "The request id must be a string or an integer.");

			return;
		}

		if ( method == "initialize" )
		{
			Server::handleInitialize(*connection, id, params);

			return;
		}

		const auto headerVersion = request.header("mcp-protocol-version");
		const auto & metaVersion = member(member(params, "_meta"), MetaProtocolVersionKey);
		const auto modern = metaVersion.isString() || headerVersion == ModernVersion;

		if ( modern )
		{
			if ( headerVersion.empty() )
			{
				respondError(*connection, 400, id, HeaderMismatchCode, "The MCP-Protocol-Version header is required.");

				return;
			}

			if ( !metaVersion.isString() )
			{
				respondError(*connection, 400, id, InvalidParamsCode, std::string{"The request lacks _meta."} + MetaProtocolVersionKey + ".");

				return;
			}

			if ( FastJSON::asValue< std::string >(metaVersion) != headerVersion )
			{
				respondError(*connection, 400, id, HeaderMismatchCode, "The MCP-Protocol-Version header does not match _meta.");

				return;
			}

			if ( headerVersion != ModernVersion )
			{
				Json::Value data{Json::objectValue};
				data["supported"] = supportedVersions();
				data["requested"] = headerVersion;

				respondError(*connection, 400, id, UnsupportedProtocolVersionCode, "Unsupported protocol version", data);

				return;
			}

			if ( decodeHeaderValue(request.header("mcp-method")) != method )
			{
				respondError(*connection, 400, id, HeaderMismatchCode, "The Mcp-Method header is missing or does not match the body.");

				return;
			}

			if ( method == "tools/call" && decodeHeaderValue(request.header("mcp-name")) != stringMember(params, "name") )
			{
				respondError(*connection, 400, id, HeaderMismatchCode, "The Mcp-Name header is missing or does not match the body.");

				return;
			}
		}
		else if ( !headerVersion.empty() && !isLegacyVersion(headerVersion) )
		{
			Json::Value data{Json::objectValue};
			data["supported"] = supportedVersions();
			data["requested"] = headerVersion;

			respondError(*connection, 400, id, UnsupportedProtocolVersionCode, "Unsupported protocol version", data);

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

			respondJSON(*connection, 200, serialize(makeResult(id, std::move(result), true)));

			return;
		}

		if ( method == "ping" )
		{
			respondJSON(*connection, 200, serialize(makeResult(id, Json::Value{Json::objectValue}, modern)));

			return;
		}

		if ( method == "subscriptions/listen" && modern )
		{
			const auto toolsListChanged = boolMember(member(params, "notifications"), "toolsListChanged", false);

			this->startStream(*connection, true, id, toolsListChanged);

			return;
		}

		if ( method == "tools/list" || method == "tools/call" )
		{
			if ( method == "tools/call" && !member(params, "name").isString() )
			{
				respondError(*connection, 200, id, InvalidParamsCode, "tools/call needs a 'name' string.");

				return;
			}

			PendingRequest pending;
			pending.connection = connection;
			pending.id = id;
			pending.method = method;
			pending.params = params;
			pending.modern = modern;

			if ( !this->enqueue(std::move(pending)) )
			{
				respondError(*connection, 503, id, InternalErrorCode, "The engine is busy: too many requests waiting for its main thread.");
			}

			/* NOTE: answered later, from processPendingRequests(), through the network thread. */
			return;
		}

		/* The specification answers an unknown method with 404 in the modern era. */
		respondError(*connection, modern ? 404 : 200, id, MethodNotFoundCode, "Method not found: " + method);
	}

	void
	Server::handleInitialize (Connection & connection, const Json::Value & id, const Json::Value & params) noexcept
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

		respondJSON(connection, 200, serialize(makeResult(id, std::move(result), false)));
	}

	void
	Server::startStream (Connection & connection, bool modern, const Json::Value & subscriptionId, bool toolsListChanged) noexcept
	{
		m_streams[connection.id()] = Stream{.subscriptionId = subscriptionId, .modern = modern, .toolsListChanged = toolsListChanged};

		/* An SSE comment keeps intermediaries and client idle timers from closing a quiet stream. */
		connection.startStream("text/event-stream", "Cache-Control: no-cache\r\nX-Accel-Buffering: no\r\n", ":\n\n", StreamKeepAliveSeconds);

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

			connection.sendStream(sseEvent(serialize(acknowledged)));
		}
	}

	void
	Server::closeStreamGracefully (Connection & connection) noexcept
	{
		const auto streamIt = m_streams.find(connection.id());

		if ( streamIt == m_streams.end() || !streamIt->second.modern )
		{
			connection.writeNowAndClose({});

			return;
		}

		Json::Value result{Json::objectValue};
		result["_meta"][MetaSubscriptionIdKey] = streamIt->second.subscriptionId;

		connection.writeNowAndClose(sseEvent(serialize(makeResult(streamIt->second.subscriptionId, std::move(result), true))));
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
	Server::broadcastToolsListChanged () noexcept
	{
		m_http.forEachConnection([this] (Connection & connection) {
			const auto streamIt = m_streams.find(connection.id());

			if ( streamIt == m_streams.end() || !streamIt->second.toolsListChanged )
			{
				return;
			}

			Json::Value notification{Json::objectValue};
			notification["jsonrpc"] = "2.0";
			notification["method"] = "notifications/tools/list_changed";

			if ( streamIt->second.modern )
			{
				notification["params"]["_meta"][MetaSubscriptionIdKey] = streamIt->second.subscriptionId;
			}

			connection.sendStream(sseEvent(serialize(notification)));
		});
	}

	void
	Server::postAnswer (const std::weak_ptr< Connection > & connection, std::string body) noexcept
	{
		m_http.post([connection, body = std::move(body)] () {
			if ( auto alive = connection.lock() )
			{
				alive->respond(200, "application/json", body, NoStore);
			}
		});
	}

	void
	Server::processPendingRequests (const Controller & controller) noexcept
	{
		if ( !m_http.isRunning() )
		{
			return;
		}

		/* A changed tree (the active scene's PostProcess node, an act switching) is announced once. */
		if ( const auto revision = Controller::consoleTreeRevision(); revision != m_announcedTreeRevision )
		{
			m_announcedTreeRevision = revision;

			m_http.post([this] () {
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

				this->postAnswer(request.connection, serialize(makeResult(request.id, std::move(result), request.modern)));

				continue;
			}

			/* tools/call */
			const auto name = stringMember(request.params, "name");
			const auto toolIt = std::ranges::find_if(tools, [&name] (const Tool & tool) {
				return tool.name() == name;
			});

			if ( toolIt == tools.end() )
			{
				this->postAnswer(request.connection, serialize(makeError(request.id, InvalidParamsCode, "Unknown tool: " + name)));

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
			m_http.post([connection = request.connection, id = request.id, modern = request.modern, succeeded, outputs = std::move(outputs)] () {
				if ( auto alive = connection.lock() )
				{
					alive->respond(200, "application/json", serialize(makeResult(id, callResult(succeeded, outputs, modern), modern)), NoStore);
				}
			});
		}
	}
}
