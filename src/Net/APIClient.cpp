/*
 * src/Net/APIClient.cpp
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
#include <algorithm>
#include <cctype>
#include <chrono>
#include <optional>
#include <ranges>
#include <string_view>

/* Third-party inclusions. */
#include "Network/asio_throw_exception.hpp"
#include "asio/ssl.hpp"

/* Local inclusions. */
#include "emeraude_base_config.hpp"
#include "FastJSON.hpp"
#include "Network/HTTPSClient.hpp"
#include "Network/TrustStore.hpp"
#include "Settings.hpp"
#include "SettingKeys.hpp"
#include "String.hpp"
#include "Tracer.hpp"

namespace EmEn::Net
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief Compares two header field names, case-insensitively (RFC 9110 §5.1).
		 * @param lhs The first name.
		 * @param rhs The second name.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		headerNameEquals (std::string_view lhs, std::string_view rhs) noexcept
		{
			if ( lhs.size() != rhs.size() )
			{
				return false;
			}

			for ( size_t index = 0; index < lhs.size(); ++index )
			{
				if ( std::tolower(static_cast< unsigned char >(lhs[index])) != std::tolower(static_cast< unsigned char >(rhs[index])) )
				{
					return false;
				}
			}

			return true;
		}

		/**
		 * @brief Returns whether a media type announces a JSON document.
		 * @note Matches the "+json" structured suffix too (application/vnd.github+json), which a
		 * plain equality against "application/json" would miss on half the APIs in existence.
		 * @param contentType The Content-Type header value.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isJSONMediaType (const std::string & contentType) noexcept
		{
			const auto lowered = String::toLower(contentType);

			/* Stop at the parameters: "application/json; charset=utf-8". */
			const auto essence = lowered.substr(0, lowered.find(';'));

			return essence.find("json") != std::string::npos;
		}
	}

	struct APIClient::Impl
	{
		asio::ssl::context tlsContext{asio::ssl::context::tls_client};
		std::unique_ptr< Network::HTTPSClient > client;
	};

	APIClient::APIClient (Settings & settings, const std::shared_ptr< Base::ThreadPool > & threadPool) noexcept
		: ServiceInterface{ClassId},
		ControllableTrait{ClassId},
		m_settings{settings},
		m_threadPool{threadPool},
		m_impl{std::make_unique< Impl >()}
	{

	}

	APIClient::~APIClient () = default;

	bool
	APIClient::onInitialize () noexcept
	{
		const auto enabledBySetting = m_settings.getOrSetDefault< bool >(NetAPIEnabledKey, DefaultNetAPIEnabled);

		if ( !enabledBySetting )
		{
			TraceInfo{ClassId} << "API calls disabled (" << NetAPIEnabledKey << " = false).";

			m_disabledReason = "the setting Core/Net/API/Enabled is false";

			return true;
		}

		/* The trust store decides what a "valid" server certificate is: without it every handshake
		 * fails, so the service stays disabled rather than silently unverified. */
		if ( !Network::TrustStore::applySystemTrustStore(m_impl->tlsContext) )
		{
			TraceError{ClassId} << "Unable to load the system trust store, API calls disabled.";

			m_disabledReason = "the system trust store could not be loaded";

			return true;
		}

		/* Shared with the download manager on purpose: a corporate CA is a property of the machine,
		 * not of one service. */
		const auto bundleFile = m_settings.getOrSetDefault< std::string >(NetCABundleFileKey, DefaultNetCABundleFile);

		if ( !bundleFile.empty() && !Network::TrustStore::applyCABundleFile(m_impl->tlsContext, bundleFile) )
		{
			TraceWarning{ClassId} << "Unable to load the CA bundle '" << bundleFile << "' (" << NetCABundleFileKey << "), continuing with the system trust store alone.";
		}

		m_retentionCeiling = m_settings.getOrSetDefault< uint32_t >(NetAPIMaxRetainedTicketsKey, DefaultNetAPIMaxRetainedTickets);
		m_maxResponseBytes = m_settings.getOrSetDefault< uint64_t >(NetAPIMaxResponseBytesKey, DefaultNetAPIMaxResponseBytes);

		Network::HTTPSClientOptions options;
		options.userAgent = DefaultUserAgent;
		options.totalTimeout = std::chrono::seconds{m_settings.getOrSetDefault< uint32_t >(NetAPITimeoutKey, DefaultNetAPITimeout)};
		/* ⚠️ An API response is held WHOLE in memory, so the ceiling is not a formality: without it
		 * one hostile or runaway endpoint grows the process until the OS kills it. */
		options.maxInMemoryBodySize = m_maxResponseBytes;

		m_impl->client = std::make_unique< Network::HTTPSClient >(m_impl->tlsContext, options);

		m_enabled = true;

		TraceInfo{ClassId} << "API calls enabled (" << Network::TrustStore::certificateCount(m_impl->tlsContext) << " trusted CA certificates, "
			"response ceiling " << m_maxResponseBytes << " bytes, keeping " << m_retentionCeiling << " terminal ticket(s)).";

		return true;
	}

	bool
	APIClient::onTerminate () noexcept
	{
		/* ⚠️ ThreadPool::wait() does not prevent a concurrent enqueue, and the other services
		 * terminate inside that window: a request() landing here would hand a worker a client that
		 * is about to be destroyed. From now on every call is refused. */
		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			m_shuttingDown = true;
		}

		/* Workers still running hold `this`: the thread pool is drained by PrimaryServices before
		 * the services terminate, so nothing is in flight here. */
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		m_items.clear();
		/* ⚠️ The default headers usually carry a bearer token: it must not outlive the service in
		 * a freed-but-not-overwritten allocation any longer than necessary. */
		m_defaultHeaders.clear();
		m_impl->client.reset();

		return true;
	}

	/* ---- Default headers ---- */

	bool
	APIClient::setDefaultHeader (const std::string & name, const std::string & value) noexcept
	{
		if ( !Network::HTTPSClient::isRequestHeaderAcceptable(name, value) )
		{
			TraceError{ClassId} << "The default header '" << name << "' is refused: bad field name, control character in the value, or a framing header the HTTPS client owns.";

			return false;
		}

		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		for ( auto & header : m_defaultHeaders )
		{
			if ( headerNameEquals(header.first, name) )
			{
				header.second = value;

				return true;
			}
		}

		m_defaultHeaders.emplace_back(name, value);

		return true;
	}

	bool
	APIClient::removeDefaultHeader (const std::string & name) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		const auto removed = std::erase_if(m_defaultHeaders, [&name] (const auto & header) {
			return headerNameEquals(header.first, name);
		});

		return removed > 0;
	}

	void
	APIClient::clearDefaultHeaders () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		m_defaultHeaders.clear();
	}

	std::vector< std::pair< std::string, std::string > >
	APIClient::defaultHeaders () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		return m_defaultHeaders;
	}

	/* ---- Requests ---- */

	int
	APIClient::request (Base::Network::HTTPRequest::Method method, const Network::URI & url, Network::HTTPRequestOptions options) noexcept
	{
		if ( !m_enabled )
		{
			TraceDebug{ClassId} << "API calls are disabled, '" << url << "' refused.";

			return InvalidTicket;
		}

		if ( String::toLower(url.scheme()) != "https" )
		{
			TraceError{ClassId} << "Only https:// URLs are callable, '" << url << "' refused.";

			return InvalidTicket;
		}

		/* Merge the defaults UNDER the per-call headers: a call that wants another Authorization
		 * for one endpoint must be able to say so without disturbing the service-wide one. */
		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			if ( m_shuttingDown )
			{
				TraceDebug{ClassId} << "The service is shutting down, '" << url << "' refused.";

				return InvalidTicket;
			}

			std::vector< std::pair< std::string, std::string > > merged;
			merged.reserve(m_defaultHeaders.size() + options.headers.size());

			for ( const auto & header : m_defaultHeaders )
			{
				const auto overridden = std::ranges::any_of(options.headers, [&header] (const auto & callHeader) {
					return headerNameEquals(callHeader.first, header.first);
				});

				if ( !overridden )
				{
					merged.push_back(header);
				}
			}

			merged.insert(merged.end(), options.headers.begin(), options.headers.end());
			options.headers = std::move(merged);
		}

		/* ⚠️ Validated HERE, not only inside HTTPSClient::run(): a malformed header must cost the
		 * caller an InvalidTicket it can see immediately, not a ticket that fails one cycle later
		 * for a reason indistinguishable from the network being down. */
		for ( const auto & [name, value] : options.headers )
		{
			if ( !Network::HTTPSClient::isRequestHeaderAcceptable(name, value) )
			{
				TraceError{ClassId} << "The request header '" << name << "' is refused, '" << url << "' not issued.";

				return InvalidTicket;
			}
		}

		int ticket = InvalidTicket;
		bool firstInFlight = false;

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			ticket = m_nextTicket++;

			/* ⚠️ No deduplication and no cache, unlike Net::Manager: two identical POSTs are two
			 * calls, and a GET used for polling must actually go out. */
			m_items.emplace(ticket, APIRequestItem{method, url, std::move(options)});

			firstInFlight = m_inFlight++ == 0;

			this->enforceRetentionCeiling();
		}

		if ( firstInFlight )
		{
			const std::lock_guard< std::mutex > lock{m_eventsAccess};

			m_events.emplace_back(Event{ticket, RequestStarted});
		}

		auto threadPool = m_threadPool;

		if ( threadPool == nullptr )
		{
			TraceError{ClassId} << "No thread pool available, '" << url << "' fails.";

			{
				const std::lock_guard< std::mutex > lock{m_itemsAccess};

				/* ⚠️ find(), never at(): std::map::at() THROWS, and the whole cascade is built
				 * -fno-exceptions — that is a terminate, not an error path. */
				if ( const auto itemIt = m_items.find(ticket); itemIt != m_items.end() )
				{
					itemIt->second.setError(Network::DownloadOutcome::BadRequest, 0);
				}

				--m_inFlight;
			}

			this->queueTerminalEvent(ticket, false);

			return ticket;
		}

		/* ⚠️ enqueue() returns false when the pool is stopping, and it is not [[nodiscard]]:
		 * ignoring it would leave the ticket Pending forever and whatever waits on it would never
		 * reach a terminal state. */
		if ( !threadPool->enqueue([this, ticket] { this->performRequest(ticket); }) )
		{
			TraceError{ClassId} << "The thread pool refused the task, '" << url << "' fails.";

			{
				const std::lock_guard< std::mutex > lock{m_itemsAccess};

				if ( const auto itemIt = m_items.find(ticket); itemIt != m_items.end() )
				{
					itemIt->second.setError(Network::DownloadOutcome::BadRequest, 0);
				}

				--m_inFlight;
			}

			this->queueTerminalEvent(ticket, false);
		}

		return ticket;
	}

	int
	APIClient::post (const Network::URI & url, std::string body, std::string contentType) noexcept
	{
		Network::HTTPRequestOptions options;
		options.body = std::move(body);
		options.contentType = std::move(contentType);

		return this->request(Network::HTTPRequest::Method::POST, url, std::move(options));
	}

	void
	APIClient::performRequest (int ticket) noexcept
	{
		Network::HTTPRequest::Method method{Network::HTTPRequest::Method::GET};
		Network::URI url;
		Network::HTTPRequestOptions options;

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			const auto itemIt = m_items.find(ticket);

			/* The retention ceiling never drops a non-terminal ticket, so this only happens on a
			 * shutdown that cleared everything. */
			if ( itemIt == m_items.end() )
			{
				--m_inFlight;

				return;
			}

			/* Abandoned before a worker got to it: nothing goes out at all. */
			if ( itemIt->second.isCancelled() )
			{
				itemIt->second.finishAsCancelled();
				--m_inFlight;

				this->queueTerminalEvent(ticket, true);

				return;
			}

			if ( m_shuttingDown || m_impl->client == nullptr )
			{
				itemIt->second.setError(Network::DownloadOutcome::BadRequest, 0);
				--m_inFlight;

				this->queueTerminalEvent(ticket, false);

				return;
			}

			method = itemIt->second.method();
			url = itemIt->second.url();
			/* Copied, not referenced: the mutex is released for the whole exchange. */
			options = itemIt->second.options();

			itemIt->second.setInFlight();
		}

		TraceInfo{ClassId} << "Calling " << Network::HTTPRequest::method(method) << " '" << url << "' (ticket #" << ticket << ") ...";

		Network::DownloadReport report;

		auto result = m_impl->client->request(method, url, std::move(options), &report);

		/* Parsed HERE, on the worker: a megabyte of JSON decoded on the main thread is a dropped
		 * frame, and the whole point of the ticket is that nothing blocks the loop. */
		std::optional< Json::Value > parsed;

		if ( result.has_value() && !result->body.empty() && isJSONMediaType(report.contentType) )
		{
			parsed = FastJSON::getRootFromString(result->body, 16, true);

			if ( !parsed )
			{
				TraceWarning{ClassId} << "'" << url << "' (ticket #" << ticket << ") declared '" << report.contentType << "' but its body is not valid JSON; it stays readable as text.";
			}
		}

		bool cancelled = false;

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			const auto itemIt = m_items.find(ticket);

			if ( itemIt == m_items.end() )
			{
				--m_inFlight;

				return;
			}

			auto & item = itemIt->second;

			cancelled = item.isCancelled();

			if ( cancelled )
			{
				/* The caller stopped caring while this was on the wire: the response is dropped
				 * rather than kept for nobody. */
				item.finishAsCancelled();
			}
			else if ( result.has_value() )
			{
				const auto statusCode = static_cast< uint16_t >(result->response.codeResponse());

				item.setDone(std::move(result->response), std::move(result->body), report.outcome, statusCode);

				if ( parsed )
				{
					item.setJSON(std::move(*parsed));
				}

				TraceSuccess{ClassId} << "'" << url << "' (ticket #" << ticket << ") answered HTTP " << statusCode << " (" << item.responseBody().size() << " bytes).";
			}
			else
			{
				item.setError(report.outcome, report.statusCode);

				TraceError{ClassId} << "'" << url << "' (ticket #" << ticket << ") failed: " << Network::to_cstring(report.outcome)
					<< ( report.statusCode > 0 ? " (HTTP " + std::to_string(report.statusCode) + ")" : std::string{} ) << ".";
			}

			--m_inFlight;
		}

		this->queueTerminalEvent(ticket, cancelled);
	}

	void
	APIClient::queueTerminalEvent (int ticket, bool cancelled) noexcept
	{
		/* ⚠️ An abandoned ticket emits NOTHING: the consumer that cancelled has already moved on,
		 * and waking its observer with a response it asked to forget is exactly what cancel()
		 * promises not to do. The batch edge below still has to see it, hence the event-less path
		 * being handled by dispatchCompleted() reading m_inFlight rather than counting events. */
		if ( cancelled )
		{
			return;
		}

		const std::lock_guard< std::mutex > lock{m_eventsAccess};

		m_events.emplace_back(Event{ticket, ResponseReceived});
	}

	void
	APIClient::enforceRetentionCeiling () noexcept
	{
		/* ⚠️ Called ONLY from request(), which is the only place the map grows — and where the
		 * newest ticket is still Pending, hence protected by the terminal test below.
		 * Calling it from performRequest() after setDone() was a defect: with the ceiling reached
		 * and enough calls in flight, the loop skipped every non-terminal ticket and erased the
		 * one that had JUST completed, destroying the response before its notification went out. */
		if ( m_retentionCeiling == 0 || m_items.size() <= m_retentionCeiling )
		{
			return;
		}

		size_t dropped = 0;

		/* std::map is ordered by ticket, and tickets are monotonic: begin() IS the oldest. */
		for ( auto itemIt = m_items.begin(); itemIt != m_items.end() && m_items.size() > m_retentionCeiling; )
		{
			/* ⚠️ A non-terminal ticket is NEVER dropped, whatever its age: a worker is holding it
			 * and performRequest() would come back to an entry that no longer exists. This means
			 * the ceiling can be exceeded while many calls are in flight — by design. */
			if ( !itemIt->second.isTerminal() )
			{
				++itemIt;

				continue;
			}

			itemIt = m_items.erase(itemIt);
			dropped++;
		}

		if ( dropped == 0 )
		{
			return;
		}

		/* Once, not per eviction: a polling loop that never releases would otherwise print a line
		 * per call for the rest of the session. */
		if ( !m_retentionWarned )
		{
			m_retentionWarned = true;

			TraceWarning{ClassId} << "The retention ceiling (" << m_retentionCeiling << ", " << NetAPIMaxRetainedTicketsKey << ") dropped the oldest terminal ticket(s). "
				"Call release(ticket) once a response has been read, otherwise a response may be evicted before anything reads it.";
		}

		TraceDebug{ClassId} << dropped << " terminal ticket(s) dropped by the retention ceiling.";
	}

	void
	APIClient::dispatchCompleted () noexcept
	{
		std::vector< Event > events;

		{
			const std::lock_guard< std::mutex > lock{m_eventsAccess};

			events.swap(m_events);
		}

		/* NOTE: The mutexes are released here: an observer may call back into request(). */
		for ( const auto & event : events )
		{
			this->notify(event.code, event.ticket);
		}

		bool finished = false;

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			/* ⚠️ The 1 -> 0 edge, not the value, and read from m_inFlight rather than from the
			 * events: a cancelled ticket queues no event at all, so counting events would leave
			 * m_batchRunning true forever after a batch that ended on a cancellation. */
			finished = m_batchRunning && m_inFlight == 0;
			m_batchRunning = m_inFlight > 0;
		}

		if ( finished )
		{
			this->notify(RequestsFinished);
		}
	}

	/* ---- Ticket lifetime ---- */

	bool
	APIClient::release (int ticket) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		const auto itemIt = m_items.find(ticket);

		if ( itemIt == m_items.end() )
		{
			return false;
		}

		/* ⚠️ A worker holds this entry by ticket and comes back to it: erasing it now would make
		 * performRequest() look up a ticket that no longer exists. Cancel it instead. */
		if ( !itemIt->second.isTerminal() )
		{
			return false;
		}

		m_items.erase(itemIt);

		return true;
	}

	bool
	APIClient::cancel (int ticket) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		const auto itemIt = m_items.find(ticket);

		if ( itemIt == m_items.end() || itemIt->second.isCancelled() )
		{
			return false;
		}

		itemIt->second.setCancelled();

		return true;
	}

	/* ---- Queries ---- */

	APIRequestStatus
	APIClient::requestStatus (int ticket) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		const auto itemIt = m_items.find(ticket);

		return itemIt == m_items.end() ? APIRequestStatus::Error : itemIt->second.status();
	}

	uint16_t
	APIClient::responseStatusCode (int ticket) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		const auto itemIt = m_items.find(ticket);

		return itemIt == m_items.end() ? uint16_t{0} : itemIt->second.statusCode();
	}

	std::string
	APIClient::responseBody (int ticket) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		const auto itemIt = m_items.find(ticket);

		if ( itemIt == m_items.end() || itemIt->second.status() != APIRequestStatus::Done )
		{
			return {};
		}

		return itemIt->second.responseBody();
	}

	bool
	APIClient::responseJSON (int ticket, Json::Value & json) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		const auto itemIt = m_items.find(ticket);

		if ( itemIt == m_items.end() || itemIt->second.status() != APIRequestStatus::Done || !itemIt->second.isJSONParsed() )
		{
			return false;
		}

		json = itemIt->second.json();

		return true;
	}

	std::string
	APIClient::responseHeader (int ticket, const std::string & name) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		const auto itemIt = m_items.find(ticket);

		if ( itemIt == m_items.end() || itemIt->second.status() != APIRequestStatus::Done )
		{
			return {};
		}

		return itemIt->second.response().value(name);
	}

	std::pair< Base::Network::DownloadOutcome, uint16_t >
	APIClient::requestFailure (int ticket) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		const auto itemIt = m_items.find(ticket);

		if ( itemIt == m_items.end() )
		{
			return {Base::Network::DownloadOutcome::Success, 0};
		}

		return itemIt->second.failure();
	}

	size_t
	APIClient::retainedCount () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		return m_items.size();
	}

	size_t
	APIClient::inFlightCount () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		return m_inFlight;
	}

	std::vector< std::tuple< int, std::string, std::string, APIRequestStatus, uint16_t > >
	APIClient::heldTickets () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		std::vector< std::tuple< int, std::string, std::string, APIRequestStatus, uint16_t > > tickets;
		tickets.reserve(m_items.size());

		for ( const auto & [ticket, item] : m_items )
		{
			tickets.emplace_back(ticket, Network::HTTPRequest::method(item.method()), to_string(item.url()), item.status(), item.statusCode());
		}

		return tickets;
	}
}
