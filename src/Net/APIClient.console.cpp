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
		this->bindCommand("request", R"(Issues an API call and returns a ticket, e.g. request(POST, https://api.example/actors, '{"name":"x"}', application/json).)",
			{
				{"method", "The HTTP method: GET, POST, PUT, PATCH, DELETE, HEAD or OPTIONS."},
				{"url", "The https:// URL of the call."},
				{"body", "The request body, verbatim (quote it when it contains a comma or a parenthesis); none when omitted."},
				{"contentType", "The Content-Type of the body; application/json when omitted (ignored without a body)."}
			},
			[this] (const std::string & methodName, const std::string & url, const std::optional< std::string > & body, const std::optional< std::string > & contentType) {
				const auto method = Base::Network::HTTPRequest::parseMethod(methodName);

				if ( method == Base::Network::HTTPRequest::Method::NONE )
				{
					return Console::CommandResult::error("Unknown HTTP method '" + methodName + "'.");
				}

				Base::Network::HTTPRequestOptions options;

				if ( body.has_value() )
				{
					options.body = *body;
					options.contentType = contentType.value_or("application/json");
				}

				const auto ticket = this->request(method, Base::Network::URI{url}, std::move(options));

				if ( ticket == InvalidTicket )
				{
					return Console::CommandResult::error("Call refused (disabled, not https, malformed header, or no thread pool). See the log.");
				}

				std::stringstream message;
				message << "Ticket #" << ticket << " (" << to_cstring(this->requestStatus(ticket)) << "). Poll with status(" << ticket << ").";

				return Console::CommandResult::success(message.str());
			});

		this->bindCommand("get", "Issues a GET and returns a ticket.",
			{
				{"url", "The https:// URL of the call."}
			},
			[this] (const std::string & url) {
				const auto ticket = this->get(Base::Network::URI{url});

				if ( ticket == InvalidTicket )
				{
					return Console::CommandResult::error("Call refused (disabled, not https, malformed header, or no thread pool). See the log.");
				}

				return Console::CommandResult::success("Ticket #" + std::to_string(ticket) + ". Poll with status(" + std::to_string(ticket) + ").");
			});

		this->bindCommand("post", "Issues a POST carrying a body and returns a ticket.",
			{
				{"url", "The https:// URL of the call."},
				{"body", "The request body, verbatim (quote it when it contains a comma or a parenthesis)."},
				{"contentType", "The Content-Type of the body.", "application/json"}
			},
			[this] (const std::string & url, const std::string & body, const std::string & contentType) {
				const auto ticket = this->post(Base::Network::URI{url}, body, contentType);

				if ( ticket == InvalidTicket )
				{
					return Console::CommandResult::error("Call refused (disabled, not https, malformed header, or no thread pool). See the log.");
				}

				return Console::CommandResult::success("Ticket #" + std::to_string(ticket) + ". Poll with status(" + std::to_string(ticket) + ").");
			});

		this->bindCommand("status", "Returns a ticket as JSON (status, httpStatus, contentType, body when Done, reason when Error). A ticket is Done even on a 404: read httpStatus.",
			{
				{"ticket", "The ticket number returned by request(), get() or post()."}
			},
			[this] (int32_t ticket) {
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

				return Console::CommandResult::json(json.str());
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("header", "Returns one response header of a ticket as JSON (empty value when absent).",
			{
				{"ticket", "The ticket number."},
				{"name", "The header field name, case-insensitive (e.g. Content-Type)."}
			},
			[this] (int32_t ticket, const std::string & name) {
				const auto value = this->responseHeader(ticket, name);

				return Console::CommandResult::json(R"({"name":")" + escapeJSON(name) + R"(","value":")" + escapeJSON(value) + R"("})");
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("list", "Lists every held ticket as JSON (ticket, method, url, status, httpStatus).", [this] () {
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

			return Console::CommandResult::json(json.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("release", "Drops a terminal ticket and the response it holds (a response lives in RAM until released).",
			{
				{"ticket", "The ticket number; it must be terminal (Done, Error or Cancelled)."}
			},
			[this] (int32_t ticket) {
				if ( !this->release(ticket) )
				{
					return Console::CommandResult::error("Unknown ticket, or it is still in flight (cancel it instead).");
				}

				return Console::CommandResult::success("Ticket released.");
			}, Console::CommandHint::Destructive);

		this->bindCommand("cancel", "Abandons a ticket: its result is dropped and no notification is emitted. The exchange itself is NOT interrupted.",
			{
				{"ticket", "The ticket number."}
			},
			[this] (int32_t ticket) {
				if ( !this->cancel(ticket) )
				{
					return Console::CommandResult::error("Unknown ticket, or it was already cancelled.");
				}

				/* Saying "cancelled" alone would suggest the socket was closed. It was not. */
				return Console::CommandResult::success("Ticket abandoned; a call already on the wire still runs to completion, its response is dropped.");
			}, Console::CommandHint::Destructive);

		this->bindCommand("setHeader", "Sets a header sent with every subsequent call (where an Authorization belongs), e.g. setHeader(Authorization, Bearer xxx).",
			{
				{"name", "The header field name."},
				{"value", "The header value, verbatim (quote it when it contains a comma or a parenthesis)."}
			},
			[this] (const std::string & name, const std::string & value) {
				if ( !this->setDefaultHeader(name, value) )
				{
					return Console::CommandResult::error("Header refused: bad field name, control character in the value, or a framing header the HTTPS client owns.");
				}

				return Console::CommandResult::success("Default header set; it is sent with every subsequent call.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("removeHeader", "Removes a default header.",
			{
				{"name", "The header field name."}
			},
			[this] (const std::string & name) {
				if ( !this->removeDefaultHeader(name) )
				{
					return Console::CommandResult::error("No such default header.");
				}

				return Console::CommandResult::success("Default header removed.");
			});

		this->bindCommand("headers", "Lists the default headers, VALUES IN CLEAR (a bearer token shows). Owner decision, not an oversight.", [this] () {
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

			return Console::CommandResult::json(json.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("isEnabled", "Returns whether API calls are enabled, and the ticket accounting, as JSON.", [this] () {
			std::stringstream json;
			json << R"({"enabled":)" << ( this->isEnabled() ? "true" : "false" );

			if ( !this->isEnabled() )
			{
				json << R"(,"reason":")" << escapeJSON(this->disabledReason()) << '"';
			}

			json << R"(,"retentionCeiling":)" << this->retentionCeiling()
				<< R"(,"retained":)" << this->retainedCount()
				<< R"(,"inFlight":)" << this->inFlightCount() << '}';

			return Console::CommandResult::json(json.str());
		}, Console::CommandHint::ReadOnly);
	}
}
