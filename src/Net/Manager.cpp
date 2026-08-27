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

			return true;
		}

		if ( !IO::isDirectoryUsable(m_cacheDirectory) )
		{
			TraceError{ClassId} << "The download cache directory '" << m_cacheDirectory << "' is not usable, downloads disabled.";

			return true;
		}

		/* The trust store decides what a "valid" server certificate is: without it every
		 * handshake fails, so downloads stay disabled rather than silently unverified. */
		if ( !Network::TrustStore::applySystemTrustStore(m_impl->tlsContext) )
		{
			TraceError{ClassId} << "Unable to load the system trust store, downloads disabled.";

			return true;
		}

		if ( !bundleFile.empty() && !Network::TrustStore::applyCABundleFile(m_impl->tlsContext, bundleFile) )
		{
			TraceWarning{ClassId} << "Unable to load the CA bundle '" << bundleFile << "' (" << NetCABundleFileKey << "), continuing with the system trust store alone.";
		}

		Network::HTTPSClientOptions options;
		options.userAgent = DefaultUserAgent;

		m_impl->client = std::make_unique< Network::HTTPSClient >(m_impl->tlsContext, options);

		if ( !this->loadCacheIndex() )
		{
			TraceWarning{ClassId} << "The download cache index could not be read, starting with an empty cache.";
		}

		m_downloadEnabled = true;

		TraceInfo{ClassId} << "Downloads enabled, " << m_cache.size() << " file(s) in cache (" << Network::TrustStore::certificateCount(m_impl->tlsContext) << " trusted CA certificates).";

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
		/* Workers still running hold `this`: the thread pool is drained by PrimaryServices
		 * before the services terminate, so nothing is in flight here. */
		const std::lock_guard< std::mutex > lock{m_itemsAccess};

		if ( m_indexDirty && !this->saveCacheIndex() )
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

	bool
	Manager::saveCacheIndex () const noexcept
	{
		Json::Value root{Json::objectValue};
		root[FilesKey] = Json::arrayValue;

		for ( const auto & [url, entry] : m_cache )
		{
			Json::Value item{Json::objectValue};
			item[URLKey] = url;
			item[FilenameKey] = entry.filename;
			item[BytesKey] = static_cast< Json::UInt64 >(entry.bytes);

			root[FilesKey].append(item);
		}

		Json::StreamWriterBuilder builder;
		builder["commentStyle"] = "None";
		builder["indentation"] = "\t";

		return IO::filePutContents(m_cacheDirectory / CacheIndexFilename, Json::writeString(builder, root));
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

		for ( const auto & entry : std::views::values(m_cache) )
		{
			const auto filepath = m_cacheDirectory / entry.filename;

			if ( IO::fileExists(filepath) && !IO::eraseFile(filepath) )
			{
				TraceError{ClassId} << "Unable to remove the cached file '" << filepath << "'.";

				success = false;
			}
		}

		m_cache.clear();
		m_indexDirty = true;

		return this->saveCacheIndex() && success;
	}

	/* ---- Requests ---- */

	int
	Manager::download (const Network::URI & url) noexcept
	{
		if ( !m_downloadEnabled )
		{
			TraceWarning{ClassId} << "Downloads are disabled, '" << url << "' refused.";

			return InvalidTicket;
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

			/* Same URL already tracked: share its ticket. A terminal one still has to complete
			 * for this new consumer; an in-flight one will, through its running worker. */
			for ( size_t index = 0; index < m_items.size(); ++index )
			{
				if ( to_string(m_items[index].url()) == urlString )
				{
					ticket = static_cast< int >(index) + 1;

					const auto status = m_items[index].status();
					completeNow = status == DownloadStatus::Done || status == DownloadStatus::Error;

					break;
				}
			}

			if ( ticket == InvalidTicket )
			{
				const auto cacheIt = m_cache.find(urlString);
				const auto filename = cacheIt != m_cache.end() ? cacheIt->second.filename : cacheFilenameFor(url);

				m_items.emplace_back(url, m_cacheDirectory / filename);
				ticket = static_cast< int >(m_items.size());

				if ( cacheIt != m_cache.end() )
				{
					m_items.back().setDone(cacheIt->second.bytes);
					completeNow = true;
				}
				else
				{
					m_items.back().setTransferring();
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

		if ( firstInFlight )
		{
			const std::lock_guard< std::mutex > lock{m_eventsAccess};

			m_events.emplace_back(Event{ticket, DownloadingStarted});
		}

		const auto & threadPool = m_threadPool;

		if ( threadPool == nullptr )
		{
			TraceError{ClassId} << "No thread pool available, '" << url << "' fails.";

			{
				const std::lock_guard< std::mutex > lock{m_itemsAccess};

				m_items[static_cast< size_t >(ticket) - 1].setError();
				--m_inFlight;
			}

			const std::lock_guard< std::mutex > lock{m_eventsAccess};

			m_events.emplace_back(Event{ticket, FileDownloaded});

			return ticket;
		}

		threadPool->enqueue([this, ticket] {
			this->performDownload(ticket);
		});

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

		bool success = m_impl->client->download(url, partialFilepath, progressHook);

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
				TraceError{ClassId} << "Unable to install '" << partialFilepath << "' as '" << filepath << "': " << error.message();

				std::filesystem::remove(partialFilepath, error);

				success = false;
			}
		}
		else
		{
			TraceError{ClassId} << "Download of '" << url << "' (ticket #" << ticket << ") failed.";
		}

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			auto & item = m_items[static_cast< size_t >(ticket) - 1];

			if ( success )
			{
				item.setDone(bytes);

				m_cache[to_string(url)] = CacheEntry{filepath.filename().string(), bytes};
				m_indexDirty = true;

				TraceSuccess{ClassId} << "'" << url << "' downloaded (" << bytes << " bytes) into '" << filepath << "'.";
			}
			else
			{
				item.setError();
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
		std::vector< int > progressed;

		{
			const std::lock_guard< std::mutex > lock{m_itemsAccess};

			finished = m_inFlight == 0;

			for ( size_t index = 0; index < m_items.size(); ++index )
			{
				if ( m_items[index].hasPendingProgress() && m_items[index].status() == DownloadStatus::Transferring )
				{
					m_items[index].clearPendingProgress();
					progressed.push_back(static_cast< int >(index) + 1);
				}
			}

			if ( m_indexDirty )
			{
				if ( this->saveCacheIndex() )
				{
					m_indexDirty = false;
				}
				else
				{
					TraceError{ClassId} << "Unable to write the download cache index.";
				}
			}
		}

		for ( const auto ticket : progressed )
		{
			this->notify(Progress, ticket);
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

		return item.status() == DownloadStatus::Done ? item.filepath() : std::filesystem::path{};
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
