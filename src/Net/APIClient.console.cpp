/*
 * src/Net/APIClient.console.cpp
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


#include "APIClient.hpp"

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include "Network/HTTPSClient.hpp"

namespace EmEn::Net
{
	namespace
	{
		/**
		 * @brief Escapes a string for embedding in the JSON the console commands print.
		 * @note A response body is arbitrary bytes: pasted raw into the output it would break the
		 * enclosing document at the first quote or newline, which reads as a client bug.
		 * @param text The text.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		escapeJSON (const std::string & text) noexcept
		{
			std::string escaped;
			escaped.reserve(text.size() + 16);

			for ( const auto character : text )
			{
				switch ( character )
				{
					case '"' :
						escaped += "\\\"";
						break;

					case '\\' :
						escaped += "\\\\";
						break;

					case '\n' :
						escaped += "\\n";
						break;

					case '\r' :
						escaped += "\\r";
						break;

					case '\t' :
						escaped += "\\t";
						break;

					default :
						if ( static_cast< unsigned char >(character) < 0x20 )
						{
							constexpr auto Digits = "0123456789abcdef";

							escaped += "\\u00";
							escaped += Digits[(static_cast< unsigned char >(character) >> 4) & 0x0F];
							escaped += Digits[static_cast< unsigned char >(character) & 0x0F];
						}
						else
						{
							escaped += character;
						}
						break;
				}
			}

			return escaped;
		}
	}

	void
	APIClient::onRegisterToConsole () noexcept
	{
		/* ⚠️ OWNER DECISION (2026-08-28): this console surface is FULLY EXPOSED — request() accepts
		 * arbitrary URLs and headers, and headers() prints the default headers, bearer token
		 * included, in clear. It was offered with redaction and the owner chose full exposure for
		 * debugging comfort. Do NOT "fix" this into redaction without asking: it is a decision, not
		 * an oversight. What contains it is that the remote console is closed by default and binds
		 * 127.0.0.1 (Core/Console/EnableRemoteListener). */
		this->bindCommand("request", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 2 )
			{
				outputs.emplace_back(Severity::Error, R"(Usage: request(METHOD, https://host/path[, body[, contentType]]) - e.g. request(POST, https://api.example/actors, {"name":"x"}, application/json))");

				return false;
			}

			const auto method = Base::Network::HTTPRequest::parseMethod(arguments[0].asString());

			if ( method == Base::Network::HTTPRequest::Method::NONE )
			{
				outputs.emplace_back(Severity::Error, "Unknown HTTP method '" + arguments[0].asString() + "'.");

				return false;
			}

			Base::Network::HTTPRequestOptions options;

			if ( arguments.size() > 2 )
			{
				options.body = arguments[2].asString();
				options.contentType = arguments.size() > 3 ? arguments[3].asString() : "application/json";
			}

			const auto ticket = this->request(method, Base::Network::URI{arguments[1].asString()}, std::move(options));

			if ( ticket == InvalidTicket )
			{
				outputs.emplace_back(Severity::Error, "Call refused (disabled, not https, malformed header, or no thread pool). See the log.");

				return false;
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Ticket #" << ticket << " (" << to_cstring(this->requestStatus(ticket)) << "). Poll with status(" << ticket << ").");

			return true;
		}, "Issues an API call and returns a ticket. Usage: request(METHOD, url[, body[, contentType]])");

		this->bindCommand("get", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: get(https://host/path)");

				return false;
			}

			const auto ticket = this->get(Base::Network::URI{arguments[0].asString()});

			if ( ticket == InvalidTicket )
			{
				outputs.emplace_back(Severity::Error, "Call refused (disabled, not https, malformed header, or no thread pool). See the log.");

				return false;
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Ticket #" << ticket << ". Poll with status(" << ticket << ").");

			return true;
		}, "Issues a GET and returns a ticket. Usage: get(url)");

		this->bindCommand("post", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 2 )
			{
				outputs.emplace_back(Severity::Error, R"(Usage: post(https://host/path, body[, contentType]) - contentType defaults to application/json)");

				return false;
			}

			const auto ticket = this->post(
				Base::Network::URI{arguments[0].asString()},
				arguments[1].asString(),
				arguments.size() > 2 ? arguments[2].asString() : std::string{"application/json"}
			);

			if ( ticket == InvalidTicket )
			{
				outputs.emplace_back(Severity::Error, "Call refused (disabled, not https, malformed header, or no thread pool). See the log.");

				return false;
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Ticket #" << ticket << ". Poll with status(" << ticket << ").");

			return true;
		}, "Issues a POST carrying a body and returns a ticket. Usage: post(url, body[, contentType])");

		this->bindCommand("status", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: status(ticket)");

				return false;
			}

			const auto ticket = arguments[0].asInteger();
			const auto status = this->requestStatus(ticket);

			std::stringstream json;
			json << R"({"ticket":)" << ticket << R"(,"status":")" << to_cstring(status) << '"';

			if ( status == APIRequestStatus::Done )
			{
				const auto body = this->responseBody(ticket);

				json << R"(,"httpStatus":)" << this->responseStatusCode(ticket)
					<< R"(,"contentType":")" << escapeJSON(this->responseHeader(ticket, "Content-Type")) << '"'
					<< R"(,"bodyBytes":)" << body.size()
					<< R"(,"jsonParsed":)" << [this, ticket] { Json::Value value; return this->responseJSON(ticket, value) ? "true" : "false"; }()
					<< R"(,"body":")" << escapeJSON(body) << '"';
			}

			if ( status == APIRequestStatus::Error )
			{
				const auto [outcome, statusCode] = this->requestFailure(ticket);

				json << R"(,"reason":")" << Base::Network::to_cstring(outcome) << '"';

				if ( statusCode > 0 )
				{
					json << R"(,"httpStatus":)" << statusCode;
				}
			}

			json << R"(,"inFlight":)" << this->inFlightCount() << R"(,"retained":)" << this->retainedCount() << '}';

			outputs.emplace_back(Severity::Info, json.str());

			return true;
		}, "Returns a ticket as JSON (status, httpStatus, contentType, body when Done, reason when Error). Usage: status(ticket)");

		this->bindCommand("header", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 2 )
			{
				outputs.emplace_back(Severity::Error, "Usage: header(ticket, Name)");

				return false;
			}

			const auto value = this->responseHeader(arguments[0].asInteger(), arguments[1].asString());

			outputs.emplace_back(Severity::Info, std::stringstream{} << R"({"name":")" << escapeJSON(arguments[1].asString()) << R"(","value":")" << escapeJSON(value) << R"("})");

			return true;
		}, "Returns one response header of a ticket. Usage: header(ticket, Name)");

		this->bindCommand("list", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			std::stringstream json;
			json << '[';

			bool first = true;

			for ( const auto & [ticket, method, url, status, statusCode] : this->heldTickets() )
			{
				if ( !first )
				{
					json << ',';
				}

				json << R"({"ticket":)" << ticket
					<< R"(,"method":")" << method << '"'
					<< R"(,"url":")" << escapeJSON(url) << '"'
					<< R"(,"status":")" << to_cstring(status) << '"'
					<< R"(,"httpStatus":)" << statusCode << '}';

				first = false;
			}

			json << ']';

			outputs.emplace_back(Severity::Info, json.str());

			return true;
		}, "Lists every held ticket as JSON (ticket, method, url, status, httpStatus).");

		this->bindCommand("release", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: release(ticket)");

				return false;
			}

			if ( !this->release(arguments[0].asInteger()) )
			{
				outputs.emplace_back(Severity::Error, "Unknown ticket, or it is still in flight (cancel it instead).");

				return false;
			}

			outputs.emplace_back(Severity::Success, "Ticket released.");

			return true;
		}, "Drops a terminal ticket and the response it holds. Usage: release(ticket)");

		this->bindCommand("cancel", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: cancel(ticket)");

				return false;
			}

			if ( !this->cancel(arguments[0].asInteger()) )
			{
				outputs.emplace_back(Severity::Error, "Unknown ticket, or it was already cancelled.");

				return false;
			}

			/* Saying "cancelled" alone would suggest the socket was closed. It was not. */
			outputs.emplace_back(Severity::Success, "Ticket abandoned; a call already on the wire still runs to completion, its response is dropped.");

			return true;
		}, "Abandons a ticket: its result is dropped and no notification is emitted. The exchange itself is NOT interrupted. Usage: cancel(ticket)");

		this->bindCommand("setHeader", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 2 )
			{
				outputs.emplace_back(Severity::Error, "Usage: setHeader(Name, value) - e.g. setHeader(Authorization, Bearer xxx)");

				return false;
			}

			if ( !this->setDefaultHeader(arguments[0].asString(), arguments[1].asString()) )
			{
				outputs.emplace_back(Severity::Error, "Header refused: bad field name, control character in the value, or a framing header the HTTPS client owns.");

				return false;
			}

			outputs.emplace_back(Severity::Success, "Default header set; it is sent with every subsequent call.");

			return true;
		}, "Sets a header sent with every subsequent call (where an Authorization belongs). Usage: setHeader(Name, value)");

		this->bindCommand("removeHeader", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: removeHeader(Name)");

				return false;
			}

			if ( !this->removeDefaultHeader(arguments[0].asString()) )
			{
				outputs.emplace_back(Severity::Error, "No such default header.");

				return false;
			}

			outputs.emplace_back(Severity::Success, "Default header removed.");

			return true;
		}, "Removes a default header. Usage: removeHeader(Name)");

		this->bindCommand("headers", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			/* ⚠️ Values printed IN CLEAR, owner decision — see the note at the top of this file. */
			std::stringstream json;
			json << '[';

			bool first = true;

			for ( const auto & [name, value] : this->defaultHeaders() )
			{
				if ( !first )
				{
					json << ',';
				}

				json << R"({"name":")" << escapeJSON(name) << R"(","value":")" << escapeJSON(value) << R"("})";

				first = false;
			}

			json << ']';

			outputs.emplace_back(Severity::Info, json.str());

			return true;
		}, "Lists the default headers, VALUES IN CLEAR (a bearer token shows). Owner decision, not an oversight.");

		this->bindCommand("isEnabled", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			std::stringstream json;
			json << R"({"enabled":)" << ( this->isEnabled() ? "true" : "false" );

			if ( !this->isEnabled() )
			{
				json << R"(,"reason":")" << escapeJSON(this->disabledReason()) << '"';
			}

			json << R"(,"retentionCeiling":)" << this->retentionCeiling()
				<< R"(,"retained":)" << this->retainedCount()
				<< R"(,"inFlight":)" << this->inFlightCount() << '}';

			outputs.emplace_back(Severity::Info, json.str());

			return true;
		}, "Returns whether API calls are enabled, and the ticket accounting.");
	}
}
