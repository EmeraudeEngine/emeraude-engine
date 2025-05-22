/*
 * src/NetworkManager.hpp
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
#include <array>
#include <map>
#include <cstddef>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "Libs/ParallelizableTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/Network/URL.hpp"
#include "CachedDownloadItem.hpp"
#include "DownloadItem.hpp"

/* Forward declarations. */
namespace EmEn
{
	class PrimaryServices;
}

namespace EmEn
{
	/**
	 * @brief The network manager service class.
	 * @extends EmEn::ServiceInterface This is a service.
	 * @extends EmEn::Libs::ParallelizableTrait This needs to parallelize download processes.
	 */
	class NetworkManager final : public ServiceInterface, public Libs::ParallelizableTrait< size_t >
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"NetworkManagerService"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

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
			 * @param primaryServices A reference to primary services.
			 */
			explicit NetworkManager (PrimaryServices & primaryServices) noexcept;

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t classUID () const noexcept override;

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool is (size_t classUID) const noexcept override;

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool usable () const noexcept override;

			/**
			 * @brief Adds a download request and returns a ticket.
			 * @param url A reference to the item url to download.
			 * @param output A reference to a path to set where to save the file.
			 * @param replaceExistingFile A switch to replace on exists file.
			 * @return int
			 */
			int download (const Libs::Network::URL & url, const std::filesystem::path & output, bool replaceExistingFile = true) noexcept;

			/**
			 * @brief Gets the download status using a ticket got from NetworkManager::newDownloadRequest().
			 * @param ticket The download ticket.
			 * @return Status
			 */
			[[nodiscard]]
			DownloadItem::Status downloadStatus (int ticket) const noexcept;

			/**
			 * @brief Returns the total number of files.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t fileCount () const noexcept;

			/**
			 * @brief Returns the total number of files with a filter.
			 * @param filter The status of the file.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t fileCount (DownloadItem::Status filter) const noexcept;

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
			uint64_t totalBytesTotal () const noexcept;

			/**
			 * @brief Return the total bytes received.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t totalBytesReceived () const noexcept;

			/**
			 * @brief Controls download information output from NetworkManager in console.
			 * @param state The state of the option.
			 * @return void
			 */
			void showProgressionInConsole (bool state) noexcept;

			/**
			 * @brief Returns whether the NetworkManager is printing download information in console.
			 * @return bool
			 */
			[[nodiscard]]
			bool showProgressionInConsole () const noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Libs::ParallelizableTrait::task() */
			bool task (const size_t & data) noexcept override;

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

			/* Flag names. */
			static constexpr auto ServiceInitialized{0UL};
			static constexpr auto DownloadEnabled{1UL};
			static constexpr auto ShowProgression{2UL};

			static constexpr auto DownloadCacheDirectory{"downloads"};
			static constexpr auto DownloadCacheDBFilename{"downloads_db.json"};

			static constexpr auto FileDataBaseKey{"FileDataBase"};
			static constexpr auto FileURLKey{"FileURL"};
			static constexpr auto CacheIdKey{"CacheId"};
			static constexpr auto FilenameKey{"Filename"};
			static constexpr auto FilesizeKey{"Filesize"};

			PrimaryServices & m_primaryServices;
			std::filesystem::path m_downloadCacheDirectory;
			std::map< std::string, CachedDownloadItem > m_downloadCache;
			size_t m_nextCacheItemId{1};
			std::vector< DownloadItem > m_downloadItems;
			std::array< bool, 8 > m_flags{
				false/*ServiceInitialized*/,
				false/*DownloadEnabled*/,
				false/*ShowProgression*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
