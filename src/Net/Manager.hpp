/*
 * src/Net/Manager.hpp
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
#include <filesystem>
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
#include "DownloadItem.hpp"
#include "Network/URI.hpp"
#include "ThreadPool.hpp"

/* Forward declarations. */
namespace EmEn
{
	class FileSystem;
	class Settings;
}

namespace EmEn::Net
{
	/**
	 * @brief The download manager service: fetches files over HTTPS into a URL-keyed cache.
	 * @details A consumer calls download(url) and receives a ticket. The transfer runs on the
	 * shared thread pool through emeraude-base's HTTPSClient (TLS verified against the system
	 * trust store); completion is notified to observers **on the main thread**, when Core calls
	 * dispatchCompleted() at the top of each loop cycle — a URL already in the cache completes the
	 * same way, one cycle later, so a consumer has a single code path. The downloaded file is
	 * then available at downloadedFilepath(ticket).
	 * @note HTTPS only: plaintext http:// is refused (the base client has no cleartext path by
	 * decision). Governed by Core/Net/DownloadEnabled and Core/Net/CABundleFile.
	 * @note [OBS][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Base::ObservableTrait Emits the download lifecycle notifications.
	 * @extends EmEn::Console::ControllableTrait Drivable from the console (download, status, listCache, clearCache).
	 */
	class EMEN_LEAN_API Manager final : public ServiceInterface, public Base::ObservableTrait, public Console::ControllableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"NetManagerService"};

			/** @brief download() result when the request was refused; valid tickets are >= 1. */
			static constexpr int InvalidTicket{0};

			/** @brief Observable notification codes. Payload: the ticket (int), except DownloadingFinished. */
			enum NotificationCode : uint8_t
			{
				Unknown,
				DownloadingStarted,		/* The first transfer of a batch began (payload: its ticket). */
				FileDownloaded,			/* A ticket reached Done or Error — read downloadStatus(ticket). */
				DownloadingFinished,	/* No transfer is left in flight (no payload). */
				Progress,				/* A transferring ticket advanced — read downloadProgress(ticket). At most one per ticket per cycle. */
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs the download manager.
			 * @param fileSystem A reference to the file system service (cache directory).
			 * @param settings A reference to the settings service.
			 * @param threadPool A reference to the OWNER's thread pool smart-pointer. It is kept by
			 * reference, not copied: PrimaryServices creates the pool in initialize(), after this
			 * constructor ran — a weak_ptr taken here would be empty forever (the bug that kept
			 * every download from starting until 2026-08-27).
			 */
			Manager (FileSystem & fileSystem, Settings & settings, const std::shared_ptr< Base::ThreadPool > & threadPool) noexcept;

			/** @brief Non-copyable, non-movable: observers and workers hold its address. */
			Manager (const Manager &) = delete;
			Manager (Manager &&) = delete;
			Manager & operator= (const Manager &) = delete;
			Manager & operator= (Manager &&) = delete;

			/** @brief Destructor (the TLS state lives behind a private implementation). */
			~Manager () override;

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
			 * @brief Requests a file and returns its ticket [Thread-safe].
			 * @note A URL already in flight returns the ticket of that transfer; a URL already in
			 * the cache returns a ticket that completes on the next dispatchCompleted(). In every
			 * case the consumer waits for FileDownloaded carrying that ticket.
			 * @param url The https:// URL.
			 * @return int The ticket (>= 1), or InvalidTicket when downloads are disabled, the URL
			 * is not https, or no thread pool is available.
			 */
			[[nodiscard]]
			int download (const Base::Network::URI & url) noexcept;

			/**
			 * @brief Returns the status of a ticket [Thread-safe].
			 * @param ticket A ticket from download().
			 * @return DownloadStatus Error for an unknown ticket.
			 */
			[[nodiscard]]
			DownloadStatus downloadStatus (int ticket) const noexcept;

			/**
			 * @brief Returns the local file of a completed ticket [Thread-safe].
			 * @param ticket A ticket from download().
			 * @return std::filesystem::path Empty unless the status is Done.
			 */
			[[nodiscard]]
			std::filesystem::path downloadedFilepath (int ticket) const noexcept;

			/**
			 * @brief Returns the progress of a ticket: bytes received, expected total (0 when unknown) [Thread-safe].
			 * @param ticket A ticket from download().
			 * @return std::pair< uint64_t, uint64_t > {0, 0} for an unknown ticket.
			 */
			[[nodiscard]]
			std::pair< uint64_t, uint64_t > downloadProgress (int ticket) const noexcept;

			/**
			 * @brief Emits the pending lifecycle notifications and persists the cache index.
			 * @note Called by Core at the top of every main-loop cycle: this is what makes the
			 * observers run on the main thread, whatever thread finished the transfer.
			 * @return void
			 */
			void dispatchCompleted () noexcept;

			/**
			 * @brief Returns whether downloads are allowed (setting + usable cache + trust store).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDownloadEnabled () const noexcept
			{
				return m_downloadEnabled;
			}

			/**
			 * @brief Returns the number of tickets issued since startup [Thread-safe].
			 * @return size_t
			 */
			[[nodiscard]]
			size_t fileCount () const noexcept;

			/**
			 * @brief Returns the number of tickets not yet terminal [Thread-safe].
			 * @return size_t
			 */
			[[nodiscard]]
			size_t fileRemainingCount () const noexcept;

			/**
			 * @brief Returns the cache content as (URL, file path, bytes) tuples [Thread-safe].
			 * @return std::vector< std::tuple< std::string, std::filesystem::path, uint64_t > >
			 */
			[[nodiscard]]
			std::vector< std::tuple< std::string, std::filesystem::path, uint64_t > > cachedFiles () const noexcept;

			/**
			 * @brief Removes every cached file and rewrites the index [Thread-safe].
			 * @note Tickets already Done keep pointing at files that no longer exist.
			 * @return bool
			 */
			bool clearCache () noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Console::ControllableTrait::onRegisterToConsole() */
			void onRegisterToConsole () noexcept override;

			/** @brief One cache index entry, keyed by the URL string in m_cache. */
			struct CacheEntry
			{
				std::string filename;
				uint64_t bytes{0};
			};

			/** @brief A lifecycle event queued by any thread, emitted by dispatchCompleted(). */
			struct Event
			{
				int ticket{InvalidTicket};
				NotificationCode code{Unknown};
			};

			/** @brief TLS context + HTTPS client, confined to the TU. */
			struct Impl;

			/**
			 * @brief Builds the cache file name of a URL: hash of the URL plus its extension.
			 * @param url The URL.
			 * @return std::string
			 */
			[[nodiscard]]
			static std::string cacheFilenameFor (const Base::Network::URI & url) noexcept;

			/**
			 * @brief Loads the cache index, dropping entries whose file vanished.
			 * @return bool
			 */
			bool loadCacheIndex () noexcept;

			/**
			 * @brief Writes the cache index. Caller holds m_itemsAccess.
			 * @return bool
			 */
			bool saveCacheIndex () const noexcept;

			/**
			 * @brief Worker body: fetches one ticket and records the outcome.
			 * @param ticket The ticket.
			 * @return void
			 */
			void performDownload (int ticket) noexcept;

			static constexpr auto CacheDirectory{"downloads"};
			static constexpr auto CacheIndexFilename{"index.json"};
			static constexpr auto PartialSuffix{".part"};
			static constexpr auto FilesKey{"Files"};
			static constexpr auto URLKey{"URL"};
			static constexpr auto FilenameKey{"Filename"};
			static constexpr auto BytesKey{"Bytes"};

			FileSystem & m_fileSystem;
			Settings & m_settings;
			const std::shared_ptr< Base::ThreadPool > & m_threadPool;
			std::unique_ptr< Impl > m_impl;
			std::filesystem::path m_cacheDirectory;
			mutable std::mutex m_itemsAccess;
			std::vector< DownloadItem > m_items;
			std::map< std::string, CacheEntry > m_cache;
			std::mutex m_eventsAccess;
			std::vector< Event > m_events;
			size_t m_inFlight{0};
			bool m_indexDirty{false};
			bool m_downloadEnabled{false};
	};
}
