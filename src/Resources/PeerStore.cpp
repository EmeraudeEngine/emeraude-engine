/*
 * src/Resources/PeerStore.cpp
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

#include "PeerStore.hpp"

/* STL inclusions. */
#include <algorithm>
#include <chrono>
#include <compare>
#include <ratio>
#include <system_error>
#include <thread>
#include <utility>

/* Third-party inclusions. */
#include "asio/ssl.hpp"
#include "Network/asio_throw_exception.hpp"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "FastJSON.hpp"
#include "FileSystem.hpp"
#include "IO/IO.hpp"
#include "Network/HTTPRequest.hpp"
#include "Network/PercentEncoding.hpp"
#include "Network/TLSConnection.hpp"
#include "Network/URI.hpp"
#include "SharingServer.hpp"
#include "ThreadPool.hpp"
#include "Tracer.hpp"
#include "Types.hpp"

namespace EmEn::Resources
{
	using namespace Base;

	namespace
	{
		/** @brief How long the peer may take to answer a small request (the index at start-up, a digest). */
		constexpr auto RequestBudget{std::chrono::seconds{30}};

		/** @brief How long a file may take: hours, a multi-gigabyte file over a slow link must finish. */
		constexpr auto DownloadBudget{std::chrono::hours{6}};

		/** @brief How long a digest is waited for WITHOUT progress. The peer hashes a large file in the background and
		 * says how far it is (202): a hash which advances is waited for, however long it takes. */
		constexpr auto DigestPatience{std::chrono::seconds{120}};

		/** @brief The first and the longest pause between two questions about a digest (doubled from one to the other). */
		constexpr auto DigestFirstPause{std::chrono::milliseconds{100}};
		constexpr auto DigestLongestPause{std::chrono::milliseconds{1000}};

		/**
		 * @brief Waits for a duration, unless a stop is requested: it is looked at every 50 ms.
		 * @param duration The duration.
		 * @param stopToken The token of the job.
		 */
		void
		waitUnlessStopped (std::chrono::milliseconds duration, const std::stop_token & stopToken) noexcept
		{
			const auto end = std::chrono::steady_clock::now() + duration;

			while ( !stopToken.stop_requested() )
			{
				const auto remaining = std::chrono::duration_cast< std::chrono::milliseconds >(end - std::chrono::steady_clock::now());

				if ( remaining <= std::chrono::milliseconds::zero() )
				{
					return;
				}

				std::this_thread::sleep_for(std::min(remaining, std::chrono::milliseconds{50}));
			}
		}

		/**
		 * @brief Returns the HTTP client options for a peer.
		 * @param totalTimeout The budget of one exchange.
		 * @return Network::HTTPSClientOptions
		 */
		[[nodiscard]]
		Network::HTTPSClientOptions
		clientOptions (std::chrono::milliseconds totalTimeout) noexcept
		{
			Network::HTTPSClientOptions options;
			options.allowPrivateCleartext = true;
			options.useEnvironmentProxy = false;
			options.totalTimeout = totalTimeout;
			/* A peer on the LAN answers in milliseconds: a peer that is away must not hold the start-up. */
			options.transportTimeouts.connectTimeout = std::chrono::seconds{3};
			options.maxDownloadSize = 64ULL * 1024 * 1024 * 1024;
			options.userAgent = "Emeraude-Engine resource peer";

			return options;
		}
	}

	PeerStore::PeerStore (const FileSystem & fileSystem, std::shared_ptr< ThreadPool > threadPool, const Network::URI & baseURL, std::string bearerToken) noexcept
		: m_fileSystem{fileSystem},
		m_threadPool{std::move(threadPool)},
		m_baseURL{to_string(baseURL)},
		m_bearerToken{std::move(bearerToken)},
		m_tlsContext{std::make_unique< asio::ssl::context >(asio::ssl::context::tls_client)}
	{
		while ( m_baseURL.ends_with('/') )
		{
			m_baseURL.pop_back();
		}

		m_requestClient = std::make_unique< Network::HTTPSClient >(*m_tlsContext, clientOptions(RequestBudget));
		m_downloadClient = std::make_unique< Network::HTTPSClient >(*m_tlsContext, clientOptions(DownloadBudget));
	}

	PeerStore::~PeerStore ()
	{
		Base::MutexLock lock{m_stateAccess};

		m_stopSource.request_stop();

		m_jobDone.wait(lock, [this] () EMEN_REQUIRES(m_stateAccess) {
			return !m_state.running;
		});
	}

	std::string
	PeerStore::fileURL (const std::string & relativePath) const noexcept
	{
		return m_baseURL + "/files/" + Network::PercentEncoding::encode(relativePath, Network::PercentEncoding::Component::Path);
	}

	std::optional< Network::HTTPResult >
	PeerStore::get (const std::string & target, Network::DownloadReport & report) const noexcept
	{
		Network::HTTPRequestOptions options;
		options.stopToken = this->stopToken();

		if ( !m_bearerToken.empty() )
		{
			options.headers.emplace_back("Authorization", "Bearer " + m_bearerToken);
		}

		return m_requestClient->request(Network::HTTPRequest::Method::GET, Network::URI{m_baseURL + target}, std::move(options), &report);
	}

	std::optional< Json::Value >
	PeerStore::fetchIndex () const noexcept
	{
		Network::DownloadReport report;
		const auto result = this->get("/index.json", report);

		if ( !result || report.statusCode != 200 )
		{
			TraceError{ClassId} << "The peer " << m_baseURL << " did not give its index (" << Network::to_cstring(report.outcome) << ", HTTP " << report.statusCode << ").";

			return std::nullopt;
		}

		auto root = FastJSON::getRootFromString(result->body, 16, true);

		if ( !root || !root->isObject() )
		{
			TraceError{ClassId} << "The index of the peer " << m_baseURL << " is not a JSON object.";

			return std::nullopt;
		}

		return root;
	}

	bool
	PeerStore::fetch (const std::string & relativePath) noexcept
	{
		if ( m_threadPool == nullptr )
		{
			return false;
		}

		{
			const Base::MutexLock lock{m_stateAccess};

			if ( m_state.running )
			{
				return false;
			}

			m_state = State{};
			m_state.request = relativePath;
			m_state.running = true;

			/* A stop source is stopped once and for all: each job gets its own, so that a cancel() ends this job and
			 * not the next one. */
			m_stopSource = std::stop_source{};
		}

		const auto enqueued = m_threadPool->enqueue([this, relativePath] () {
			this->runFetch(relativePath);

			/* NOTE: notified UNDER the lock: ~PeerStore() waits for running == false on this condition variable; notified
			 * after the unlock, the destructor could see the flag, return and destroy m_jobDone before this notify. */
			const Base::MutexLock lock{m_stateAccess};

			m_state.running = false;
			m_state.currentFile.clear();

			m_jobDone.notifyAll();
		});

		if ( !enqueued )
		{
			const Base::MutexLock lock{m_stateAccess};

			m_state.running = false;

			/* NOTE: Same contract as the job's end: a waiting destructor must be woken. */
			m_jobDone.notifyAll();
		}

		return enqueued;
	}

	Json::Value
	PeerStore::status () const noexcept
	{
		const Base::MutexLock lock{m_stateAccess};

		Json::Value status{Json::objectValue};
		status["peer"] = m_baseURL;
		status["request"] = m_state.request;
		status["running"] = m_state.running;
		status["currentFile"] = m_state.currentFile;
		status["currentBytes"] = static_cast< Json::UInt64 >(m_state.currentBytes);
		status["currentTotal"] = static_cast< Json::UInt64 >(m_state.currentTotal);
		status["bytesReceived"] = static_cast< Json::UInt64 >(m_state.bytesReceived);
		status["filesListed"] = static_cast< Json::UInt64 >(m_state.filesListed);
		status["filesFetched"] = static_cast< Json::UInt64 >(m_state.filesFetched);
		status["filesKept"] = static_cast< Json::UInt64 >(m_state.filesKept);
		status["filesFailed"] = static_cast< Json::UInt64 >(m_state.filesFailed);
		status["truncatedListing"] = m_state.truncatedListing;

		Json::Value errors{Json::arrayValue};

		for ( const auto & error : m_state.errors )
		{
			errors.append(error);
		}

		status["errors"] = std::move(errors);

		return status;
	}

	void
	PeerStore::cancel () noexcept
	{
		const Base::MutexLock lock{m_stateAccess};

		m_stopSource.request_stop();
	}

	std::stop_token
	PeerStore::stopToken () const noexcept
	{
		const Base::MutexLock lock{m_stateAccess};

		return m_stopSource.get_token();
	}

	void
	PeerStore::reportError (const std::string & message) noexcept
	{
		TraceError{ClassId} << message;

		const Base::MutexLock lock{m_stateAccess};

		if ( m_state.errors.size() < MaxReportedErrors )
		{
			m_state.errors.emplace_back(message);
		}
	}

	std::optional< std::vector< PeerStore::RemoteFile > >
	PeerStore::remoteFiles (const std::string & relativePath) const noexcept
	{
		Network::DownloadReport report;
		const auto result = this->get("/list/" + Network::PercentEncoding::encode(relativePath, Network::PercentEncoding::Component::Path), report);

		if ( !result )
		{
			return std::nullopt;
		}

		std::vector< RemoteFile > files;

		/* Not a directory: the path itself, as a file (its size comes with the download). */
		if ( report.statusCode == 404 && !relativePath.empty() )
		{
			files.push_back(RemoteFile{.path = relativePath, .size = std::nullopt});

			return files;
		}

		if ( report.statusCode != 200 )
		{
			return std::nullopt;
		}

		const auto root = FastJSON::getRootFromString(result->body, 8, true);

		if ( !root || !root->isObject() || !root->isMember("files") || !(*root)["files"].isArray() )
		{
			return std::nullopt;
		}

		for ( const auto & entry : (*root)["files"] )
		{
			if ( !entry.isObject() )
			{
				continue;
			}

			auto path = FastJSON::getValue< std::string >(entry, "path");

			if ( !path || path->empty() )
			{
				continue;
			}

			files.push_back(RemoteFile{.path = std::move(*path), .size = FastJSON::getValue< uint64_t >(entry, "size")});
		}

		return files;
	}

	std::optional< std::filesystem::path >
	PeerStore::destinationRoot () const noexcept
	{
		for ( const auto & dataDirectory : m_fileSystem.dataDirectories() )
		{
			auto root = dataDirectory / DataStores;
			std::error_code errorCode;

			if ( std::filesystem::is_directory(root, errorCode) )
			{
				return root;
			}
		}

		return std::nullopt;
	}

	void
	PeerStore::runFetch (const std::string & relativePath) noexcept
	{
		/* The token of THIS job's source (fetch() made it): cancel() and the destructor stop it. */
		const auto stopToken = this->stopToken();

		const auto root = this->destinationRoot();

		if ( !root )
		{
			this->reportError("No data directory holds a 'data-stores' directory to fetch into.");

			return;
		}

		const auto files = this->remoteFiles(relativePath);

		if ( !files )
		{
			this->reportError("The peer " + m_baseURL + " did not list '" + relativePath + "'.");

			return;
		}

		{
			const Base::MutexLock lock{m_stateAccess};

			m_state.filesListed = files->size();
		}

		TraceInfo{ClassId} << "Fetching " << files->size() << " file(s) of '" << relativePath << "' from " << m_baseURL << " into " << *root << " ...";

		for ( const auto & file : *files )
		{
			if ( stopToken.stop_requested() )
			{
				this->reportError("Fetch cancelled.");

				return;
			}

			this->fetchFile(*root, file, stopToken);
		}

		const Base::MutexLock lock{m_stateAccess};

		TraceSuccess{ClassId} << "Fetch of '" << relativePath << "' done: " << m_state.filesFetched << " fetched, " << m_state.filesKept << " kept, " << m_state.filesFailed << " failed.";
	}

	std::optional< std::string >
	PeerStore::peerDigest (const std::string & relativePath, const std::stop_token & stopToken) const noexcept
	{
		const auto target = "/sha256/" + Network::PercentEncoding::encode(relativePath, Network::PercentEncoding::Component::Path);

		/* NOTE: Each question is a short exchange (2026-11-14). The peer used to answer when its hash was done, on a
		 * connection silent meanwhile: a hash longer than the request budget made this side delete a complete copy,
		 * and a stop waited for that answer. The patience counts the time WITHOUT progress. */
		auto patienceEnd = std::chrono::steady_clock::now() + DigestPatience;
		auto pause = std::chrono::duration_cast< std::chrono::milliseconds >(DigestFirstPause);
		uint64_t hashedBytes = 0;

		while ( !stopToken.stop_requested() && std::chrono::steady_clock::now() < patienceEnd )
		{
			Network::DownloadReport report;
			const auto result = this->get(target, report);

			if ( !result )
			{
				break;
			}

			const auto answer = FastJSON::getRootFromString(result->body, 4, true);
			const bool isObject = answer && answer->isObject();

			if ( report.statusCode == 200 )
			{
				return isObject ? FastJSON::getValue< std::string >(*answer, "sha256") : std::nullopt;
			}

			/* 202: the peer hashes the file in the background and says how far it is. 503: it is busy with other
			 * digests. Anything else is a refusal. */
			if ( report.statusCode != 202 && report.statusCode != 503 )
			{
				break;
			}

			if ( const auto hashed = isObject ? FastJSON::getValue< uint64_t >(*answer, "hashed") : std::nullopt; hashed && *hashed > hashedBytes )
			{
				hashedBytes = *hashed;
				patienceEnd = std::chrono::steady_clock::now() + DigestPatience;
			}

			waitUnlessStopped(pause, stopToken);

			pause = std::min(pause * 2, std::chrono::duration_cast< std::chrono::milliseconds >(DigestLongestPause));
		}

		return std::nullopt;
	}

	void
	PeerStore::fetchFile (const std::filesystem::path & root, const RemoteFile & file, const std::stop_token & stopToken) noexcept
	{
		const auto countFailure = [this] () {
			const Base::MutexLock lock{m_stateAccess};

			++m_state.filesFailed;
		};

		/* ⚠️ The path comes from the NETWORK: it must stay under the local data stores. */
		const auto destination = file.path.find('\\') == std::string::npos ? IO::confinedPath(root, IO::u8path(file.path)) : std::nullopt;

		if ( !destination )
		{
			this->reportError("The peer listed '" + file.path + "', which leaves the data stores: refused.");
			countFailure();

			return;
		}

		std::error_code errorCode;

		if ( file.size && std::filesystem::is_regular_file(*destination, errorCode) && std::filesystem::file_size(*destination, errorCode) == *file.size && !errorCode )
		{
			const Base::MutexLock lock{m_stateAccess};

			++m_state.filesKept;

			return;
		}

		std::filesystem::create_directories(destination->parent_path(), errorCode);

		if ( errorCode )
		{
			this->reportError("Unable to create the directory of '" + file.path + "': " + Base::IO::systemErrorText(errorCode));
			countFailure();

			return;
		}

		{
			const Base::MutexLock lock{m_stateAccess};

			m_state.currentFile = file.path;
			m_state.currentBytes = 0;
			m_state.currentTotal = file.size.value_or(0);
		}

		auto partial = *destination;
		partial += ".part";

		Network::HTTPRequestOptions options;
		options.stopToken = stopToken;

		if ( !m_bearerToken.empty() )
		{
			options.headers.emplace_back("Authorization", "Bearer " + m_bearerToken);
		}

		const auto progress = [this] (uint64_t received, std::optional< uint64_t > total) {
			const Base::MutexLock lock{m_stateAccess};

			m_state.currentBytes = received;
			m_state.currentTotal = total.value_or(m_state.currentTotal);
		};

		Network::DownloadReport report;

		if ( !m_downloadClient->download(Network::URI{this->fileURL(file.path)}, partial, std::move(options), progress, &report) )
		{
			this->reportError("'" + file.path + "' was not downloaded (" + Network::to_cstring(report.outcome) + ", HTTP " + std::to_string(report.statusCode) + ").");
			countFailure();

			return;
		}

		/* The digest the peer computes, compared with the received bytes: a truncated or altered copy is deleted. */
		const auto remoteDigest = this->peerDigest(file.path, stopToken);
		const auto localDigest = SharingServer::fileSHA256(partial, stopToken);

		if ( stopToken.stop_requested() )
		{
			this->reportError("'" + file.path + "': cancelled during its verification, the copy is deleted.");
			std::filesystem::remove(partial, errorCode);
			countFailure();

			return;
		}

		if ( !remoteDigest || !localDigest || *remoteDigest != *localDigest )
		{
			this->reportError("'" + file.path + "': " + ( remoteDigest ? "the SHA-256 does not match, the copy is deleted." : "the peer gave no SHA-256, the copy is deleted."));
			std::filesystem::remove(partial, errorCode);
			countFailure();

			return;
		}

		std::filesystem::rename(partial, *destination, errorCode);

		if ( errorCode )
		{
			this->reportError("Unable to put '" + file.path + "' in place: " + Base::IO::systemErrorText(errorCode));
			std::filesystem::remove(partial, errorCode);
			countFailure();

			return;
		}

		const auto size = std::filesystem::file_size(*destination, errorCode);

		TraceSuccess{ClassId} << "'" << file.path << "' fetched (" << ( errorCode ? 0 : size ) << " bytes, SHA-256 " << *localDigest << ").";

		const Base::MutexLock lock{m_stateAccess};

		++m_state.filesFetched;
		m_state.bytesReceived += errorCode ? 0 : size;
	}
}
