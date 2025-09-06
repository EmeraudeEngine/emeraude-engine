/*
 * src/Net/Manager.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Libs/ObservableTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/Network/URL.hpp"
#include "Libs/ThreadPool.hpp"
#include "CachedDownloadItem.hpp"
#include "DownloadItem.hpp"

/* Forward declarations. */
namespace EmEn
{
	class FileSystem;
}

namespace EmEn::Net
{
	/**
	 * @brief The network manager service class.
	 * @note [OBS][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Libs::ObservableTrait This service is observable
	 */
	class Manager final : public ServiceInterface, public Libs::ObservableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Net::ManagerService"};

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				Unknown,
				DownloadingStarted,
				FileDownloaded,
				DownloadingFinished,
				Progress,
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs the network manager.
			 * @param fileSystem A reference to the file system services.
			 * @param threadPool A reference to the thread pool smart-pointer.
			 */
			explicit
			Manager (FileSystem & fileSystem, const std::shared_ptr< Libs::ThreadPool > & threadPool) noexcept
				: ServiceInterface{ClassId},
				m_fileSystem{fileSystem},
				m_threadPool{threadPool}
			{

			}

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				static const size_t classUID = EmEn::Libs::Hash::FNV1a(ClassId);

				return classUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_serviceInitialized;
			}

			/**
			 * @brief Adds a download request and returns a ticket.
			 * @param url A reference to the item url to download.
			 * @param output A reference to a path to set where to save the file.
			 * @param replaceExistingFile A switch to replace on exists file.
			 * @return int
			 */
			int download (const Libs::Network::URL & url, const std::filesystem::path & output, bool replaceExistingFile = true) noexcept;

			/**
			 * @brief Gets the download status using a ticket got from Net::Manager::newDownloadRequest().
			 * @param ticket The download ticket.
			 * @return DownloadStatus
			 */
			[[nodiscard]]
			DownloadStatus downloadStatus (int ticket) const noexcept;

			/**
			 * @brief Returns the total number of files.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			fileCount () const noexcept
			{
				return m_downloadItems.size();
			}

			/**
			 * @brief Returns the total number of files with a filter.
			 * @param filter The status of the file.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			fileCount (DownloadStatus filter) const noexcept
			{
				return std::ranges::count_if(m_downloadItems, [filter] (const auto & request){
					return request.status() == filter;
				});
			}

			/**
			 * @brief Returns the total number of files currently in downloading.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t fileRemainingCount () const noexcept;

			/**
			 * @brief Returns the total bytes to wait.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			totalBytesTotal () const noexcept
			{
				return std::accumulate(m_downloadItems.cbegin(), m_downloadItems.cend(), 0UL, [] (uint64_t sum, const auto & item) {
					return sum + item.bytesTotal();
				});
			}

			/**
			 * @brief Return the total bytes received.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			totalBytesReceived () const noexcept
			{
				return std::accumulate(m_downloadItems.cbegin(), m_downloadItems.cend(), 0UL, [] (uint64_t sum, const auto & item) {
					return sum + item.bytesReceived();
				});
			}

			/**
			 * @brief Controls download information output from Net::Manager in the console.
			 * @param state The state of the option.
			 * @return void
			 */
			void
			showProgressionInConsole (bool state) noexcept
			{
				m_showProgression = state;
			}

			/**
			 * @brief Returns whether the Net::Manager is printing download information in the console.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			showProgressionInConsole () const noexcept
			{
				return m_showProgression;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/**
			 * @brief Returns the download cache db filepath.
			 * @return std::filesystem::path
			 */
			[[nodiscard]]
			std::filesystem::path getDownloadCacheDBFilepath () const noexcept;

			/**
			 * @brief Returns the downloaded item filepath.
			 * @param cacheId The downloaded item ID.
			 * @return std::filesystem::path
			 */
			[[nodiscard]]
			std::filesystem::path getDownloadedCacheFilepath (size_t cacheId) const noexcept;

			/**
			 * @brief Updates the download cache db file.
			 * @return bool
			 */
			[[nodiscard]]
			bool updateDownloadCacheDBFile () const noexcept;

			/**
			 * @brief Checks the download cache.
			 * @return bool
			 */
			 [[nodiscard]]
			bool checkDownloadCacheDBFile () noexcept;

			/**
			 * @brief Removes downloaded files from the cache directory.
			 * @return bool
			 */
			bool clearDownloadCache () noexcept;

			static constexpr auto DownloadCacheDirectory{"downloads"};
			static constexpr auto DownloadCacheDBFilename{"downloads_db.json"};
			static constexpr auto FileDataBaseKey{"FileDataBase"};
			static constexpr auto FileURLKey{"FileURL"};
			static constexpr auto CacheIdKey{"CacheId"};
			static constexpr auto FilenameKey{"Filename"};
			static constexpr auto FilesizeKey{"Filesize"};

			FileSystem & m_fileSystem;
			std::weak_ptr< Libs::ThreadPool > m_threadPool;
			std::filesystem::path m_downloadCacheDirectory;
			std::map< std::string, CachedDownloadItem > m_downloadCache;
			size_t m_nextCacheItemId{1};
			std::vector< DownloadItem > m_downloadItems;
			bool m_serviceInitialized{false};
			bool m_downloadEnabled{false};
			bool m_showProgression{false};
	};
}
