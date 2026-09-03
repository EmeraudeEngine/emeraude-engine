/*
 * src/Net/Manager.cpp
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


#include "Manager.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <optional>
#include <ranges>
#include <sstream>
#include <system_error>

/* Third-party inclusions. */
#include "Network/asio_throw_exception.hpp"
#include "asio/ssl.hpp"

/* Local inclusions. */
#include "emeraude_base_config.hpp"
#include "FastJSON.hpp"
#include "FileSystem.hpp"
#include "IO/IO.hpp"
#include "Network/HTTPSClient.hpp"
#include "Network/TrustStore.hpp"
#include "Settings.hpp"
#include "SettingKeys.hpp"
#include "String.hpp"
#include "Tracer.hpp"

namespace EmEn::Net
{
	using namespace Base;

	struct Manager::Impl
	{
		asio::ssl::context tlsContext{asio::ssl::context::tls_client};
		std::unique_ptr< Network::HTTPSClient > client;
	};

	Manager::Manager (FileSystem & fileSystem, Settings & settings, const std::shared_ptr< Base::ThreadPool > & threadPool) noexcept
		: ServiceInterface{ClassId},
		ControllableTrait{ClassId},
		m_fileSystem{fileSystem},
		m_settings{settings},
		m_threadPool{threadPool},
		m_impl{std::make_unique< Impl >()}
	{

	}

	Manager::~Manager () = default;

	bool
	Manager::onInitialize () noexcept
	{
		const auto enabledBySetting = m_settings.getOrSetDefault< bool >(NetDownloadEnabledKey, DefaultNetDownloadEnabled);
		const auto bundleFile = m_settings.getOrSetDefault< std::string >(NetCABundleFileKey, DefaultNetCABundleFile);

		m_cacheDirectory = m_fileSystem.cacheDirectory(CacheDirectory);

		if ( !enabledBySetting )
		{
			TraceInfo{ClassId} << "Downloads disabled (" << NetDownloadEnabledKey << " = false).";

			m_disabledReason = "the setting Core/Net/DownloadEnabled is false";

			return true;
		}

		if ( !IO::isDirectoryUsable(m_cacheDirectory) )
		{
			TraceError{ClassId} << "The download cache directory '" << IO::toU8String(m_cacheDirectory) << "' is not usable, downloads disabled.";

			m_disabledReason = "the download cache directory is not usable";

			return true;
		}

		/* The trust store decides what a "valid" server certificate is: without it every
		 * handshake fails, so downloads stay disabled rather than silently unverified. */
		if ( !Network::TrustStore::applySystemTrustStore(m_impl->tlsContext) )
		{
			TraceError{ClassId} << "Unable to load the system trust store, downloads disabled.";

			m_disabledReason = "the system trust store could not be loaded";

			return true;
		}

		if ( !bundleFile.empty() && !Network::TrustStore::applyCABundleFile(m_impl->tlsContext, bundleFile) )
		{
			TraceWarning{ClassId} << "Unable to load the CA bundle '" << bundleFile << "' (" << NetCABundleFileKey << "), continuing with the system trust store alone.";
		}

		m_cacheBudget = m_settings.getOrSetDefault< uint64_t >(NetCacheMaxBytesKey, DefaultNetCacheMaxBytes);

		Network::HTTPSClientOptions options;
		options.userAgent = DefaultUserAgent;
		options.totalTimeout = std::chrono::seconds{m_settings.getOrSetDefault< uint32_t >(NetDownloadTimeoutKey, DefaultNetDownloadTimeout)};

		m_impl->client = std::make_unique< Network::HTTPSClient >(m_impl->tlsContext, options);

		if ( !this->loadCacheIndex() )
		{
			TraceWarning{ClassId} << "The download cache index could not be read, starting with an empty cache.";
		}

		/* A kill during a transfer leaves a .part file nothing else would ever reclaim: the index
		 * does not know about it, and clearCache() only walks the index. */
		this->sweepPartialFiles();

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			this->enforceCacheBudget();
		}

		m_downloadEnabled = true;

		TraceSuccess{ClassId} << "Downloads enabled, " << m_cache.size() << " file(s) in cache (" << Network::TrustStore::certificateCount(m_impl->tlsContext) << " trusted CA certificates).";

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
		/* ⚠️ ThreadPool::wait() does not prevent a concurrent enqueue, and the other services
		 * terminate inside that window: a download() landing here would hand a worker a client
		 * that is about to be destroyed. From now on every request is refused. */
		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			m_shuttingDown = true;
		}

		/* Workers still running hold `this`: the thread pool is drained by PrimaryServices
		 * before the services terminate, so nothing is in flight here. */
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		if ( m_indexDirty && !this->writeCacheIndex(this->serializeCacheIndex()) )
		{
			return false;
		}

		m_impl->client.reset();

		return true;
	}

	/* ---- Cache ---- */

	std::string
	Manager::cacheFilenameFor (const Network::URI & url) noexcept
	{
		/* The file name is the hash of the whole URL: two URLs sharing a basename never collide,
		 * and the same URL always lands on the same file. The extension is kept because several
		 * loaders sniff the format from it. */
		std::stringstream name;
		name << std::hex << std::setw(16) << std::setfill('0') << Hash::FNV1a(to_string(url));

		auto extension = std::filesystem::path{url.path()}.extension().string();

		if ( extension.size() > 1 && extension.size() <= 16 && std::ranges::all_of(extension.begin() + 1, extension.end(), [] (char character) {
			return std::isalnum(static_cast< unsigned char >(character)) != 0;
		}) )
		{
			name << String::toLower(extension);
		}

		return name.str();
	}

	bool
	Manager::loadCacheIndex () noexcept
	{
		const auto indexFilepath = m_cacheDirectory / CacheIndexFilename;

		if ( !IO::fileExists(indexFilepath) )
		{
			return true;
		}

		const auto rootCheck = FastJSON::getRootFromFile(indexFilepath);

		if ( !rootCheck || !rootCheck->isMember(FilesKey) || !(*rootCheck)[FilesKey].isArray() )
		{
			return false;
		}

		for ( const auto & file : (*rootCheck)[FilesKey] )
		{
			if ( !file.isMember(URLKey) || !file.isMember(FilenameKey) || !file.isMember(BytesKey) || !file[URLKey].isString() || !file[FilenameKey].isString() || !file[BytesKey].isIntegral() )
			{
				TraceWarning{ClassId} << "Malformed entry in the download cache index, skipped.";

				continue;
			}

			CacheEntry entry;
			entry.filename = file[FilenameKey].asString();
			entry.bytes = file[BytesKey].asLargestUInt();
			entry.lastUse = file.isMember(LastUseKey) && file[LastUseKey].isIntegral() ? file[LastUseKey].asLargestUInt() : 0;

			m_useCounter = std::max(m_useCounter, entry.lastUse);

			if ( !IO::fileExists(m_cacheDirectory / entry.filename) )
			{
				TraceWarning{ClassId} << "Cached file '" << entry.filename << "' vanished, entry dropped.";

				m_indexDirty = true;

				continue;
			}

			m_cache.emplace(file[URLKey].asString(), std::move(entry));
		}

		return true;
	}

	std::string
	Manager::serializeCacheIndex () const noexcept
	{
		Json::Value root{Json::objectValue};
		root[FilesKey] = Json::arrayValue;

		for ( const auto & [url, entry] : m_cache )
		{
			Json::Value item{Json::objectValue};
			item[URLKey] = url;
			item[FilenameKey] = entry.filename;
			item[BytesKey] = static_cast< Json::UInt64 >(entry.bytes);
			item[LastUseKey] = static_cast< Json::UInt64 >(entry.lastUse);

			root[FilesKey].append(item);
		}

		Json::StreamWriterBuilder builder;
		builder["commentStyle"] = "None";
		builder["indentation"] = "\t";

		return Json::writeString(builder, root);
	}

	bool
	Manager::writeCacheIndex (const std::string & content) const noexcept
	{

		/* ⚠️ Written to a sibling then renamed: filePutContents() truncates in place, so a crash
		 * mid-write left a truncated index — and every already-downloaded file became an orphan,
		 * since the entries that name them were gone. */
		const auto finalPath = m_cacheDirectory / CacheIndexFilename;
		auto temporaryPath = finalPath;
		temporaryPath += ".tmp";

		if ( !IO::filePutContents(temporaryPath, content) )
		{
			return false;
		}

		std::error_code error;

		std::filesystem::rename(temporaryPath, finalPath, error);

		if ( error )
		{
			std::filesystem::remove(temporaryPath, error);

			return false;
		}

		return true;
	}

	void
	Manager::sweepPartialFiles () const noexcept
	{
		std::error_code error;

		std::filesystem::directory_iterator iterator{m_cacheDirectory, error};

		if ( error )
		{
			return;
		}

		size_t removed = 0;

		for ( const auto & entry : iterator )
		{
			if ( entry.path().extension() != PartialSuffix )
			{
				continue;
			}

			std::filesystem::remove(entry.path(), error);

			if ( !error )
			{
				removed++;
			}

			error.clear();
		}

		if ( removed > 0 )
		{
			TraceInfo{ClassId} << removed << " partial download file(s) left by a previous run removed.";
		}
	}

	void
	Manager::enforceCacheBudget () noexcept
	{
		if ( m_cacheBudget == 0 || m_cache.empty() )
		{
			return;
		}

		uint64_t total = 0;

		for ( const auto & entry : std::views::values(m_cache) )
		{
			total += entry.bytes;
		}

		if ( total <= m_cacheBudget )
		{
			return;
		}

		/* ⚠️ Eviction is strictly least-recently-used, with NOTHING pinned. Pinning the files of
		 * completed tickets was tried first and makes the budget unenforceable: a ticket stays Done
		 * for the whole process lifetime, so in a long session every file would be pinned and the
		 * cache would grow without bound — the very thing the budget exists to prevent.
		 * What makes that safe: the file just downloaded carries the highest use counter, so it is
		 * the last candidate, and downloadedFilepath() checks the file still exists before naming
		 * it. The residual window — a file evicted between its FileDownloaded notification and the
		 * loading task reading it — needs the cache to overflow within those few milliseconds. */
		std::vector< std::pair< uint64_t, std::string > > candidates;
		candidates.reserve(m_cache.size());

		for ( const auto & [url, entry] : m_cache )
		{
			candidates.emplace_back(entry.lastUse, url);
		}

		std::ranges::sort(candidates);

		size_t evicted = 0;

		for ( const auto & [lastUse, url] : candidates )
		{
			if ( total <= m_cacheBudget )
			{
				break;
			}

			const auto entryIt = m_cache.find(url);

			if ( entryIt == m_cache.end() )
			{
				continue;
			}

			std::error_code error;

			std::filesystem::remove(m_cacheDirectory / entryIt->second.filename, error);

			total -= std::min(total, entryIt->second.bytes);
			m_cache.erase(entryIt);
			m_indexDirty = true;
			evicted++;
		}

		if ( evicted > 0 )
		{
			TraceInfo{ClassId} << evicted << " cached file(s) evicted, the cache is back under " << m_cacheBudget << " bytes.";
		}
	}

	std::vector< std::tuple< std::string, std::filesystem::path, uint64_t > >
	Manager::cachedFiles () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		std::vector< std::tuple< std::string, std::filesystem::path, uint64_t > > files;
		files.reserve(m_cache.size());

		for ( const auto & [url, entry] : m_cache )
		{
			files.emplace_back(url, m_cacheDirectory / entry.filename, entry.bytes);
		}

		return files;
	}

	bool
	Manager::clearCache () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		bool success = true;

		/* ⚠️ The directory is walked, not the index: a .part file left by a crash, or a file whose
		 * index entry was lost, is invisible to the map and would never be reclaimed — while the
		 * header promises "every cached file". The index itself is kept. */
		std::error_code error;

		std::filesystem::directory_iterator iterator{m_cacheDirectory, error};

		if ( error )
		{
			return false;
		}

		for ( const auto & entry : iterator )
		{
			if ( entry.path().filename() == CacheIndexFilename )
			{
				continue;
			}

			std::filesystem::remove(entry.path(), error);

			if ( error )
			{
				TraceError{ClassId} << "Unable to remove the cached file '" << IO::toU8String(entry.path()) << "': " << error.message();

				error.clear();
				success = false;
			}
		}

		m_cache.clear();
		m_indexDirty = false;

		/* Tickets already Done still name files that are gone: downloadedFilepath() checks
		 * existence, so they answer empty rather than a dangling path. */
		return this->writeCacheIndex(this->serializeCacheIndex()) && success;
	}

	/* ---- Requests ---- */

	int
	Manager::download (const Network::URI & url) noexcept
	{
		if ( !m_downloadEnabled )
		{
			TraceDebug{ClassId} << "Downloads are disabled, '" << url << "' refused.";

			return InvalidTicket;
		}

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			if ( m_shuttingDown )
			{
				TraceDebug{ClassId} << "The service is shutting down, '" << url << "' refused.";

				return InvalidTicket;
			}
		}

		if ( String::toLower(url.scheme()) != "https" )
		{
			TraceError{ClassId} << "Only https:// URLs are downloadable, '" << url << "' refused.";

			return InvalidTicket;
		}

		const auto urlString = to_string(url);
		int ticket = InvalidTicket;
		bool completeNow = false;
		bool startWorker = false;
		bool firstInFlight = false;

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			/* Same URL already tracked: O(1) now — the linear scan re-serialised every tracked
			 * URL on every request. */
			if ( const auto known = m_ticketByURL.find(urlString); known != m_ticketByURL.end() )
			{
				ticket = known->second;

				auto & item = m_items[static_cast< size_t >(ticket) - 1];

				switch ( item.status() )
				{
					case DownloadStatus::Done :
						/* Serve it again, and mark the cache entry as freshly used. */
						if ( const auto cacheIt = m_cache.find(urlString); cacheIt != m_cache.end() )
						{
							cacheIt->second.lastUse = ++m_useCounter;
							m_indexDirty = true;
						}

						completeNow = true;
						break;

					case DownloadStatus::Error :
						/* ⚠️ A failed URL used to keep its terminal ticket forever: the same
						 * request after the network came back replayed the old Error instead of
						 * trying again. A new attempt is started on the same ticket. */
						TraceInfo{ClassId} << "'" << url << "' failed before; retrying on ticket #" << ticket << ".";

						item.resetForRetry();

						startWorker = true;
						firstInFlight = m_inFlight++ == 0;
						break;

					case DownloadStatus::Pending :
					case DownloadStatus::Transferring :
						/* A worker is already on it; its completion serves this caller too. */
						break;
				}
			}
			else
			{
				const auto cacheIt = m_cache.find(urlString);
				const auto filename = cacheIt != m_cache.end() ? cacheIt->second.filename : cacheFilenameFor(url);

				m_items.emplace_back(url, m_cacheDirectory / filename);
				ticket = static_cast< int >(m_items.size());
				m_ticketByURL.emplace(urlString, ticket);

				if ( cacheIt != m_cache.end() )
				{
					m_items.back().setDone(cacheIt->second.bytes);

					cacheIt->second.lastUse = ++m_useCounter;
					m_indexDirty = true;

					completeNow = true;
				}
				else
				{
					startWorker = true;
					firstInFlight = m_inFlight++ == 0;
				}
			}
		}

		if ( completeNow )
		{
			const std::lock_guard< std::mutex > lock{m_eventsAccess};

			m_events.emplace_back(Event{ticket, FileDownloaded});

			return ticket;
		}

		if ( !startWorker )
		{
			return ticket;
		}

		/* ⚠️ The edge, not the count: a cache hit never increments m_inFlight, so keying on it
		 * emitted DownloadingStarted/Finished for transfers that never happened. */
		if ( firstInFlight )
		{
			const std::lock_guard< std::mutex > lock{m_eventsAccess};

			m_events.emplace_back(Event{ticket, DownloadingStarted});
		}

		auto threadPool = m_threadPool;

		if ( threadPool == nullptr )
		{
			TraceError{ClassId} << "No thread pool available, '" << url << "' fails.";

			{
				const std::lock_guard< std::mutex > lock{m_itemsAccess};

				m_items[static_cast< size_t >(ticket) - 1].setError(Network::DownloadOutcome::LocalIO, 0);
				--m_inFlight;
			}

			const std::lock_guard< std::mutex > lock{m_eventsAccess};

			m_events.emplace_back(Event{ticket, FileDownloaded});

			return ticket;
		}

		/* ⚠️ enqueue() returns false when the pool is stopping, and it is not [[nodiscard]]:
		 * ignoring it left the ticket Transferring forever and the resource waiting on it never
		 * reached a terminal state. */
		if ( !threadPool->enqueue([this, ticket] { this->performDownload(ticket); }) )
		{
			TraceError{ClassId} << "The thread pool refused the task, '" << url << "' fails.";

			{
				const std::lock_guard< std::mutex > lock{m_itemsAccess};

				m_items[static_cast< size_t >(ticket) - 1].setError(Network::DownloadOutcome::LocalIO, 0);
				--m_inFlight;
			}

			const std::lock_guard< std::mutex > lock{m_eventsAccess};

			m_events.emplace_back(Event{ticket, FileDownloaded});
		}

		return ticket;
	}

	void
	Manager::performDownload (int ticket) noexcept
	{
		Network::URI url;
		std::filesystem::path filepath;

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			const auto & item = m_items[static_cast< size_t >(ticket) - 1];
			url = item.url();
			filepath = item.filepath();
		}

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			if ( m_shuttingDown || m_impl->client == nullptr )
			{
				m_items[static_cast< size_t >(ticket) - 1].setError();
				--m_inFlight;

				const std::lock_guard< std::mutex > eventsLock{m_eventsAccess};

				m_events.emplace_back(Event{ticket, FileDownloaded});

				return;
			}
		}

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			/* Pending until a worker actually picks it up — the state exists to be observable. */
			m_items[static_cast< size_t >(ticket) - 1].setTransferring();
		}

		TraceInfo{ClassId} << "Downloading '" << url << "' (ticket #" << ticket << ") ...";

		/* Stream into a side file, then rename: a reader never sees a half-written cache file,
		 * and a failed transfer leaves nothing under the final name. */
		auto partialFilepath = filepath;
		partialFilepath += PartialSuffix;

		uint64_t bytes = 0;

		/* The hook runs on this worker: it only records the counters under the items mutex and
		 * raises the pending flag; dispatchCompleted() turns that into ONE Progress notification
		 * per ticket per main-loop cycle, whatever the read granularity. */
		const auto progressHook = [this, ticket] (uint64_t received, std::optional< uint64_t > total) {
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			m_items[static_cast< size_t >(ticket) - 1].setProgress(received, total.value_or(0));
		};

		Network::DownloadReport report;

		bool success = m_impl->client->download(url, partialFilepath, progressHook, &report);

		if ( success )
		{
			std::error_code error;

			bytes = std::filesystem::file_size(partialFilepath, error);

			if ( !error )
			{
				std::filesystem::rename(partialFilepath, filepath, error);
			}

			if ( error )
			{
				TraceError{ClassId} << "Unable to install '" << IO::toU8String(partialFilepath) << "' as '" << IO::toU8String(filepath) << "': " << error.message();

				std::filesystem::remove(partialFilepath, error);

				success = false;
			}
		}
		else
		{
			TraceError{ClassId} << "Download of '" << url << "' (ticket #" << ticket << ") failed: " << Network::to_cstring(report.outcome) << ( report.statusCode > 0 ? " (HTTP " + std::to_string(report.statusCode) + ")" : std::string{} ) << ".";
		}

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			auto & item = m_items[static_cast< size_t >(ticket) - 1];

			if ( success )
			{
				item.setDone(bytes);

				m_cache[to_string(url)] = CacheEntry{filepath.filename().string(), bytes, ++m_useCounter};
				m_indexDirty = true;

				this->enforceCacheBudget();

				TraceSuccess{ClassId} << "'" << url << "' downloaded (" << bytes << " bytes) into '" << IO::toU8String(filepath) << "'.";
			}
			else
			{
				item.setError(report.outcome, report.statusCode);
			}

			--m_inFlight;
		}

		const std::lock_guard< std::mutex > lock{m_eventsAccess};

		m_events.emplace_back(Event{ticket, FileDownloaded});
	}

	void
	Manager::dispatchCompleted () noexcept
	{
		std::vector< Event > events;

		{
			const std::lock_guard< std::mutex > lock{m_eventsAccess};

			events.swap(m_events);
		}

		if ( events.empty() )
		{
			/* Nothing terminal this cycle: only the throttled Progress notifications may be due. */
			bool anyPending = false;

			{
				const std::lock_guard< std::mutex > lock{m_itemsAccess};

				anyPending = m_inFlight > 0 && std::ranges::any_of(m_items, [] (const DownloadItem & item) {
					return item.hasPendingProgress();
				});
			}

			if ( !anyPending )
			{
				return;
			}
		}

		/* NOTE: The mutexes are released here: an observer may call back into download(). */
		for ( const auto & event : events )
		{
			this->notify(event.code, event.ticket);
		}

		bool finished = false;
		std::string indexContent;
		std::vector< int > progressed;

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			/* ⚠️ The 1 -> 0 edge, not the value: a cache hit carries events without ever having
			 * been in flight, and used to emit DownloadingFinished on every one of them. */
			finished = m_transferRunning && m_inFlight == 0;
			m_transferRunning = m_inFlight > 0;

			for ( size_t index = 0; index < m_items.size(); ++index )
			{
				if ( m_items[index].hasPendingProgress() && m_items[index].status() == DownloadStatus::Transferring )
				{
					m_items[index].clearPendingProgress();
					progressed.push_back(static_cast< int >(index) + 1);
				}
			}

			/* ⚠️ Serialised HERE, under the lock; the disk write happens after it is released.
			 * Reading m_cache outside the lock would race with a worker completing a download. */
			if ( m_indexDirty )
			{
				indexContent = this->serializeCacheIndex();
			}
		}

		for ( const auto ticket : progressed )
		{
			this->notify(Progress, ticket);
		}

		/* ⚠️ Written outside m_itemsAccess: this runs on the main loop, and saveCacheIndex()
		 * does real disk I/O — holding the mutex made a worker wait on a frame's write. */
		if ( !indexContent.empty() )
		{
			if ( this->writeCacheIndex(indexContent) )
			{
				const std::lock_guard< std::mutex > lock{m_itemsAccess};

				m_indexDirty = false;
			}
			else
			{
				TraceError{ClassId} << "Unable to write the download cache index.";
			}
		}

		if ( finished )
		{
			this->notify(DownloadingFinished);
		}
	}

	std::pair< uint64_t, uint64_t >
	Manager::downloadProgress (int ticket) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		if ( ticket < 1 || static_cast< size_t >(ticket) > m_items.size() )
		{
			return {0, 0};
		}

		const auto & item = m_items[static_cast< size_t >(ticket) - 1];

		return {item.bytesReceived(), item.bytesTotal()};
	}

	/* ---- Queries ---- */

	DownloadStatus
	Manager::downloadStatus (int ticket) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		if ( ticket < 1 || static_cast< size_t >(ticket) > m_items.size() )
		{
			return DownloadStatus::Error;
		}

		return m_items[static_cast< size_t >(ticket) - 1].status();
	}

	std::filesystem::path
	Manager::downloadedFilepath (int ticket) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		if ( ticket < 1 || static_cast< size_t >(ticket) > m_items.size() )
		{
			return {};
		}

		const auto & item = m_items[static_cast< size_t >(ticket) - 1];

		if ( item.status() != DownloadStatus::Done )
		{
			return {};
		}

		/* ⚠️ clearCache() (or a user emptying the directory) can delete the file a completed
		 * ticket names. Answering the path anyway made the consumer rewrite its resource onto
		 * something that no longer exists, and fail later with an unrelated message. */
		std::error_code error;

		if ( !std::filesystem::exists(item.filepath(), error) || error )
		{
			return {};
		}

		return item.filepath();
	}

	std::pair< Base::Network::DownloadOutcome, uint16_t >
	Manager::downloadFailure (int ticket) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		if ( ticket < 1 || static_cast< size_t >(ticket) > m_items.size() )
		{
			return {Base::Network::DownloadOutcome::Success, 0};
		}

		return m_items[static_cast< size_t >(ticket) - 1].failure();
	}

	size_t
	Manager::fileCount () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		return m_items.size();
	}

	size_t
	Manager::fileRemainingCount () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		return m_inFlight;
	}
}
