/*
 * src/Net/APIRequestItem.hpp
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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <string>
#include <utility>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions for usages. */
#include "Network/HTTPRequest.hpp"
#include "Network/HTTPResponse.hpp"
#include "Network/HTTPSClient.hpp"
#include "Network/URI.hpp"
#include "Types.hpp"

namespace EmEn::Net
{
	/**
	 * @brief One API call tracked by Net::APIClient, addressed by its ticket.
	 * @note Plain data guarded by the client's mutex; it carries no logic of its own.
	 * @note ⚠️ Unlike Net::DownloadItem this holds the RESPONSE BODY IN MEMORY, so its lifetime is
	 * the caller's problem: APIClient::release() drops it, and a retention ceiling drops the oldest
	 * terminal ones when the caller forgets. A polling loop that never released would otherwise
	 * grow without bound.
	 */
	class EMEN_API APIRequestItem final
	{
		public:

			/**
			 * @brief Constructs an API request item.
			 * @param method The HTTP method.
			 * @param url The target URL [std::move].
			 * @param options The headers, body and media type of the request [std::move].
			 */
			APIRequestItem (Base::Network::HTTPRequest::Method method, Base::Network::URI url, Base::Network::HTTPRequestOptions options) noexcept
				: m_url{std::move(url)},
				m_options{std::move(options)},
				m_method{method}
			{

			}

			/**
			 * @brief Returns the HTTP method.
			 * @return Base::Network::HTTPRequest::Method
			 */
			[[nodiscard]]
			Base::Network::HTTPRequest::Method
			method () const noexcept
			{
				return m_method;
			}

			/**
			 * @brief Returns the target URL.
			 * @return const Base::Network::URI &
			 */
			[[nodiscard]]
			const Base::Network::URI &
			url () const noexcept
			{
				return m_url;
			}

			/**
			 * @brief Returns the request options, to be handed to the HTTPS client.
			 * @return const Base::Network::HTTPRequestOptions &
			 */
			[[nodiscard]]
			const Base::Network::HTTPRequestOptions &
			options () const noexcept
			{
				return m_options;
			}

			/**
			 * @brief Returns the status.
			 * @return APIRequestStatus
			 */
			[[nodiscard]]
			APIRequestStatus
			status () const noexcept
			{
				return m_status;
			}

			/**
			 * @brief Returns whether the status is terminal (nothing more will happen to it).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTerminal () const noexcept
			{
				return m_status == APIRequestStatus::Done || m_status == APIRequestStatus::Error || m_status == APIRequestStatus::Cancelled;
			}

			/**
			 * @brief Returns the HTTP status code, 0 until a response arrived.
			 * @return uint16_t
			 */
			[[nodiscard]]
			uint16_t
			statusCode () const noexcept
			{
				return m_statusCode;
			}

			/**
			 * @brief Returns the response body, empty unless the status is Done.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			responseBody () const noexcept
			{
				return m_responseBody;
			}

			/**
			 * @brief Returns the response headers.
			 * @return const Base::Network::HTTPResponse &
			 */
			[[nodiscard]]
			const Base::Network::HTTPResponse &
			response () const noexcept
			{
				return m_response;
			}

			/**
			 * @brief Returns the body parsed as JSON.
			 * @note Only meaningful when isJSONParsed() is true: the worker parses it once, off the
			 * main thread, and only when the server declared a JSON media type.
			 * @return const Json::Value &
			 */
			[[nodiscard]]
			const Json::Value &
			json () const noexcept
			{
				return m_json;
			}

			/**
			 * @brief Returns whether the body was parsed as JSON.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isJSONParsed () const noexcept
			{
				return m_JSONParsed;
			}

			/**
			 * @brief Returns why the call failed, and the HTTP status when the exchange completed.
			 * @return std::pair< Base::Network::DownloadOutcome, uint16_t >
			 */
			[[nodiscard]]
			std::pair< Base::Network::DownloadOutcome, uint16_t >
			failure () const noexcept
			{
				return {m_outcome, m_statusCode};
			}

			/**
			 * @brief Returns whether the caller abandoned this ticket.
			 * @note The transfer is NOT interrupted — the HTTPS client is synchronous and has no
			 * cancellation point. This only says the result must be dropped on arrival and no
			 * notification emitted.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCancelled () const noexcept
			{
				return m_cancelled;
			}

			/**
			 * @brief Marks the item as being performed by a worker.
			 * @return void
			 */
			void
			setInFlight () noexcept
			{
				m_status = APIRequestStatus::InFlight;
			}

			/**
			 * @brief Marks the caller as no longer interested in the result.
			 * @return void
			 */
			void
			setCancelled () noexcept
			{
				m_cancelled = true;

				if ( !this->isTerminal() )
				{
					/* ⚠️ The payload STAYS while the ticket is queued or in flight. Freeing it here
					 * would hand a worker that has not picked it up yet an empty options object,
					 * and the request would leave without its Authorization header instead of not
					 * leaving at all. The worker drops the ticket at pick-up, or discards the
					 * result on arrival. */
					return;
				}

				this->finishAsCancelled();
			}

			/**
			 * @brief Turns an abandoned ticket terminal, dropping everything it held.
			 * @note Called by the worker when it picks up a ticket already cancelled, and by
			 * setCancelled() when the response had already landed.
			 * @return void
			 */
			void
			finishAsCancelled () noexcept
			{
				m_responseBody.clear();
				m_responseBody.shrink_to_fit();
				m_json = Json::Value{};
				m_JSONParsed = false;
				m_status = APIRequestStatus::Cancelled;

				this->releaseRequestPayload();
			}

			/**
			 * @brief Records a completed exchange, whatever HTTP status it carried.
			 * @note Done means "a response arrived", NOT "the API said yes": a 404 with a body is a
			 * successful exchange, and the caller reads statusCode() to know what happened.
			 * @param response The response headers [std::move].
			 * @param body The response body [std::move].
			 * @param outcome Success, or HTTPStatus for a non-2xx.
			 * @param statusCode The HTTP status.
			 * @return void
			 */
			void
			setDone (Base::Network::HTTPResponse response, std::string body, Base::Network::DownloadOutcome outcome, uint16_t statusCode) noexcept
			{
				m_response = std::move(response);
				m_responseBody = std::move(body);
				m_outcome = outcome;
				m_statusCode = statusCode;
				m_status = APIRequestStatus::Done;

				this->releaseRequestPayload();
			}

			/**
			 * @brief Attaches the JSON parsed from the body by the worker.
			 * @param json The parsed document [std::move].
			 * @return void
			 */
			void
			setJSON (Json::Value json) noexcept
			{
				m_json = std::move(json);
				m_JSONParsed = true;
			}

			/**
			 * @brief Marks the call as failed; there is no response to read.
			 * @param outcome Why it failed.
			 * @param statusCode The HTTP status when the exchange completed, 0 otherwise.
			 * @return void
			 */
			void
			setError (Base::Network::DownloadOutcome outcome, uint16_t statusCode) noexcept
			{
				m_outcome = outcome;
				m_statusCode = statusCode;
				m_status = APIRequestStatus::Error;

				this->releaseRequestPayload();
			}

		private:

			/**
			 * @brief Frees what the request carried once no worker can need it again.
			 * @note ⚠️ An Authorization header and a request body have no reason to sit in memory
			 * for the rest of the process: a ticket is kept for its RESPONSE. This also means a
			 * terminal ticket can never be replayed — deliberate, because silently re-sending a
			 * POST is how a payment gets charged twice.
			 * @return void
			 */
			void
			releaseRequestPayload () noexcept
			{
				m_options.headers.clear();
				m_options.headers.shrink_to_fit();
				m_options.body.clear();
				m_options.body.shrink_to_fit();
				m_options.contentType.clear();
			}

			Base::Network::URI m_url;
			Base::Network::HTTPRequestOptions m_options;
			Base::Network::HTTPResponse m_response;
			std::string m_responseBody;
			Json::Value m_json;
			Base::Network::DownloadOutcome m_outcome{Base::Network::DownloadOutcome::Success};
			Base::Network::HTTPRequest::Method m_method{Base::Network::HTTPRequest::Method::GET};
			APIRequestStatus m_status{APIRequestStatus::Pending};
			uint16_t m_statusCode{0};
			bool m_JSONParsed{false};
			bool m_cancelled{false};
	};
}
