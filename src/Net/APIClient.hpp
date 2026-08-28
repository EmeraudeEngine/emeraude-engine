/*
 * src/Net/APIClient.hpp
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
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "ObservableTrait.hpp"
#include "Console/ControllableTrait.hpp"

/* Local inclusions for usages. */
#include "APIRequestItem.hpp"
#include "Network/HTTPRequest.hpp"
#include "Network/URI.hpp"
#include "ThreadPool.hpp"

/* Forward declarations. */
namespace EmEn
{
	class Settings;
}

namespace EmEn::Net
{
	/**
	 * @brief The web API client service: performs HTTPS exchanges and hands the response back on
	 * the main thread.
	 * @details A consumer calls request()/get()/post() and receives a ticket. The exchange runs on
	 * the shared thread pool through emeraude-base's HTTPSClient (TLS verified against the system
	 * trust store); completion is notified to observers **on the main thread**, when Core calls
	 * dispatchCompleted() at the top of each loop cycle. The response body is then readable at
	 * responseBody(ticket), and as a Json::Value at responseJSON(ticket) when the server declared
	 * a JSON media type.
	 *
	 * @note ⚠️ This is NOT Net::Manager with a different verb, and the differences are deliberate:
	 *  - **No cache and no deduplication.** Two identical POSTs are two calls, and a GET used for
	 *    polling must actually go out. Net::Manager folds requests on the same URL onto one ticket;
	 *    doing that here would silently swallow calls.
	 *  - **No retry.** Net::Manager retries a URL that failed before. Replaying a write is how a
	 *    payment gets charged twice, so a terminal ticket here stays terminal and the caller
	 *    decides whether to issue a new one.
	 *  - **The response lives in memory**, so tickets are NOT kept forever: release() drops one,
	 *    and a retention ceiling drops the oldest terminal ones when the caller forgets.
	 *  - **A non-2xx is a SUCCESS** (status Done): an API says what went wrong in the body, and
	 *    the caller reads responseStatusCode(). Only a call that never completed is Error.
	 *
	 * @note Authentication is set at runtime with setDefaultHeader(), never from the settings file:
	 * a token written to settings.json would sit in cleartext on disk. Governed by
	 * Core/Net/API/Enabled, Core/Net/API/TimeoutSeconds, Core/Net/API/MaxRetainedTickets and
	 * Core/Net/API/MaxResponseBytes; the trust store follows Core/Net/CABundleFile.
	 * @note [OBS][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Base::ObservableTrait Emits the request lifecycle notifications.
	 * @extends EmEn::Console::ControllableTrait Drivable from the console (request, get, post, status, list, cancel, release).
	 */
	class EMEN_LEAN_API APIClient final : public ServiceInterface, public Base::ObservableTrait, public Console::ControllableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"NetAPIClientService"};

			/** @brief request() result when the call was refused; valid tickets are >= 1. */
			static constexpr int InvalidTicket{0};

			/** @brief Observable notification codes. Payload: the ticket (int), except RequestsFinished. */
			enum NotificationCode : uint8_t
			{
				Unknown,
				RequestStarted,		/* The first call of a batch left (payload: its ticket). */
				ResponseReceived,	/* A ticket reached Done or Error — read requestStatus(ticket). */
				RequestsFinished,	/* No call is left in flight (no payload). */
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs the API client.
			 * @param settings A reference to the settings service.
			 * @param threadPool A reference to the OWNER's thread pool smart-pointer. It is kept by
			 * reference, not copied: PrimaryServices creates the pool in initialize(), after this
			 * constructor ran — a weak_ptr taken here would be empty forever (the bug that kept
			 * every download from starting until 2026-08-27).
			 */
			APIClient (Settings & settings, const std::shared_ptr< Base::ThreadPool > & threadPool) noexcept;

			/** @brief Non-copyable, non-movable: observers and workers hold its address. */
			APIClient (const APIClient &) = delete;
			APIClient (APIClient &&) = delete;
			APIClient & operator= (const APIClient &) = delete;
			APIClient & operator= (APIClient &&) = delete;

			/** @brief Destructor (the TLS state lives behind a private implementation). */
			~APIClient () override;

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/**
			 * @brief Issues an API call and returns its ticket [Thread-safe].
			 * @note The default headers are merged in first; a header of the same name in @a options
			 * overrides the default for this call alone.
			 * @param method The HTTP method.
			 * @param url The https:// URL.
			 * @param options The per-call headers, body and media type [std::move]. Default none.
			 * @return int The ticket (>= 1), or InvalidTicket when the service is disabled, the URL
			 * is not https, a header is malformed, or no thread pool is available.
			 */
			[[nodiscard]]
			int request (Base::Network::HTTPRequest::Method method, const Base::Network::URI & url, Base::Network::HTTPRequestOptions options = {}) noexcept;

			/**
			 * @brief Issues a GET and returns its ticket [Thread-safe].
			 * @param url The https:// URL.
			 * @return int
			 */
			[[nodiscard]]
			int
			get (const Base::Network::URI & url) noexcept
			{
				return this->request(Base::Network::HTTPRequest::Method::GET, url);
			}

			/**
			 * @brief Issues a POST carrying a body and returns its ticket [Thread-safe].
			 * @param url The https:// URL.
			 * @param body The request body [std::move].
			 * @param contentType The media type of the body. Default "application/json".
			 * @return int
			 */
			[[nodiscard]]
			int post (const Base::Network::URI & url, std::string body, std::string contentType = "application/json") noexcept;

			/**
			 * @brief Returns the status of a ticket [Thread-safe].
			 * @param ticket A ticket from request().
			 * @return APIRequestStatus Error for an unknown or already released ticket.
			 */
			[[nodiscard]]
			APIRequestStatus requestStatus (int ticket) const noexcept;

			/**
			 * @brief Returns the HTTP status code of a ticket, 0 when no response arrived [Thread-safe].
			 * @note ⚠️ A ticket can be Done with a 404: Done says a response arrived, not that the
			 * API accepted the call. This is what tells the two apart.
			 * @param ticket A ticket from request().
			 * @return uint16_t
			 */
			[[nodiscard]]
			uint16_t responseStatusCode (int ticket) const noexcept;

			/**
			 * @brief Returns the response body of a ticket [Thread-safe].
			 * @param ticket A ticket from request().
			 * @return std::string Empty unless the status is Done.
			 */
			[[nodiscard]]
			std::string responseBody (int ticket) const noexcept;

			/**
			 * @brief Returns the response body parsed as JSON [Thread-safe].
			 * @note The parsing happened on the worker, off the main thread, and only when the
			 * server declared a JSON media type. A body that is not JSON is not an error — it
			 * simply answers false here and stays readable through responseBody().
			 * @param ticket A ticket from request().
			 * @param json Where the document is written [out].
			 * @return bool False when the ticket is unknown, not Done, or its body was not parsed.
			 */
			[[nodiscard]]
			bool responseJSON (int ticket, Json::Value & json) const noexcept;

			/**
			 * @brief Returns one response header of a ticket [Thread-safe].
			 * @note Where an API puts its pagination cursor and its rate-limit budget.
			 * @param ticket A ticket from request().
			 * @param name The header field name (case-insensitive).
			 * @return std::string Empty when absent.
			 */
			[[nodiscard]]
			std::string responseHeader (int ticket, const std::string & name) const noexcept;

			/**
			 * @brief Returns why a ticket failed, and the HTTP status when the exchange completed.
			 * @param ticket A ticket from request().
			 * @return std::pair< Base::Network::DownloadOutcome, uint16_t >
			 */
			[[nodiscard]]
			std::pair< Base::Network::DownloadOutcome, uint16_t > requestFailure (int ticket) const noexcept;

			/**
			 * @brief Drops a ticket and everything it holds [Thread-safe].
			 * @note ⚠️ Call it once the response has been read. A response body sits in memory; a
			 * polling loop that never releases relies on the retention ceiling, which drops the
			 * OLDEST terminal ticket — possibly one whose response was never read.
			 * @param ticket A ticket from request().
			 * @return bool False when the ticket is unknown, or still in flight.
			 */
			bool release (int ticket) noexcept;

			/**
			 * @brief Abandons a ticket: its result is dropped and no notification is emitted [Thread-safe].
			 * @note ⚠️ The exchange itself is NOT interrupted. HTTPSClient is synchronous and has no
			 * cancellation point, so a call already on the wire runs to completion on its worker
			 * and its response is thrown away on arrival. This frees the caller, not the socket.
			 * @param ticket A ticket from request().
			 * @return bool False when the ticket is unknown or already cancelled.
			 */
			bool cancel (int ticket) noexcept;

			/**
			 * @brief Sets a header sent with every subsequent call [Thread-safe].
			 * @note Where an Authorization or an X-Api-Key belongs. Deliberately NOT read from the
			 * settings file: a token written there would sit in cleartext on disk and be dumped by
			 * anything that prints the settings.
			 * @param name The header field name.
			 * @param value The header field value.
			 * @return bool False when the name or the value is refused (see
			 * Base::Network::HTTPSClient::isRequestHeaderAcceptable()).
			 */
			bool setDefaultHeader (const std::string & name, const std::string & value) noexcept;

			/**
			 * @brief Removes a default header [Thread-safe].
			 * @param name The header field name (case-insensitive).
			 * @return bool False when there was no such header.
			 */
			bool removeDefaultHeader (const std::string & name) noexcept;

			/**
			 * @brief Removes every default header [Thread-safe].
			 * @return void
			 */
			void clearDefaultHeaders () noexcept;

			/**
			 * @brief Returns the default headers, as (name, value) pairs [Thread-safe].
			 * @return std::vector< std::pair< std::string, std::string > >
			 */
			[[nodiscard]]
			std::vector< std::pair< std::string, std::string > > defaultHeaders () const noexcept;

			/**
			 * @brief Emits the pending lifecycle notifications.
			 * @note Called by Core at the top of every main-loop cycle: this is what makes the
			 * observers run on the main thread, whatever thread finished the exchange.
			 * @return void
			 */
			void dispatchCompleted () noexcept;

			/**
			 * @brief Returns whether API calls are allowed (setting + trust store).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return m_enabled;
			}

			/**
			 * @brief Returns why API calls are disabled, empty when they are enabled.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			disabledReason () const noexcept
			{
				return m_disabledReason;
			}

			/**
			 * @brief Returns how many terminal tickets are kept before the oldest are dropped.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			retentionCeiling () const noexcept
			{
				return m_retentionCeiling;
			}

			/**
			 * @brief Returns the number of tickets currently held [Thread-safe].
			 * @return size_t
			 */
			[[nodiscard]]
			size_t retainedCount () const noexcept;

			/**
			 * @brief Returns the number of tickets not yet terminal [Thread-safe].
			 * @return size_t
			 */
			[[nodiscard]]
			size_t inFlightCount () const noexcept;

			/**
			 * @brief Returns every held ticket as (ticket, method, url, status, HTTP status) tuples [Thread-safe].
			 * @return std::vector< std::tuple< int, std::string, std::string, APIRequestStatus, uint16_t > >
			 */
			[[nodiscard]]
			std::vector< std::tuple< int, std::string, std::string, APIRequestStatus, uint16_t > > heldTickets () const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Console::ControllableTrait::onRegisterToConsole() */
			void onRegisterToConsole () noexcept override;

			/** @brief A lifecycle event queued by any thread, emitted by dispatchCompleted(). */
			struct Event
			{
				int ticket{InvalidTicket};
				NotificationCode code{Unknown};
			};

			/** @brief TLS context + HTTPS client, confined to the TU. */
			struct Impl;

			/**
			 * @brief Worker body: performs one exchange and records the outcome.
			 * @param ticket The ticket.
			 * @return void
			 */
			void performRequest (int ticket) noexcept;

			/**
			 * @brief Drops the oldest terminal tickets until the retention ceiling is met.
			 * @note Caller holds m_itemsAccess. A ticket that is not terminal is never dropped,
			 * whatever its age: a worker still holds it — so the ceiling can be exceeded while many
			 * calls are in flight, by design.
			 * @note ⚠️ Call it ONLY from request(): that is the only place the map grows, and the
			 * only moment the newest ticket is guaranteed non-terminal. Running it after a
			 * completion could erase the very ticket that just completed.
			 * @return void
			 */
			void enforceRetentionCeiling () noexcept;

			/**
			 * @brief Records a terminal event for a ticket and, when it was abandoned, swallows it.
			 * @note Takes m_eventsAccess. The caller MAY hold m_itemsAccess: the lock order across
			 * this class is always items -> events, never the reverse.
			 * @param ticket The ticket.
			 * @param cancelled Whether the caller abandoned it.
			 * @return void
			 */
			void queueTerminalEvent (int ticket, bool cancelled) noexcept;

			Settings & m_settings;
			const std::shared_ptr< Base::ThreadPool > & m_threadPool;
			std::unique_ptr< Impl > m_impl;
			mutable std::mutex m_itemsAccess;
			/* Keyed by ticket, NOT indexed: release() and the retention ceiling erase entries, which
			 * a vector indexed by (ticket - 1) could not do without invalidating every other ticket. */
			std::map< int, APIRequestItem > m_items;
			std::vector< std::pair< std::string, std::string > > m_defaultHeaders;
			std::mutex m_eventsAccess;
			std::vector< Event > m_events;
			int m_nextTicket{1};
			size_t m_inFlight{0};
			size_t m_retentionCeiling{0};
			std::string m_disabledReason;
			uint64_t m_maxResponseBytes{0};
			bool m_batchRunning{false};
			bool m_enabled{false};
			bool m_shuttingDown{false};
			bool m_retentionWarned{false};
	};
}
