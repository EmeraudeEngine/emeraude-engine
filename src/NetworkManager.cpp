/*
 * src/NetworkManager.cpp
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

#include "NetworkManager.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstddef>
#include <utility>
#include <tuple>
#include <numeric>
#include <ranges>

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Libs/Network/Network.hpp"
#include "Libs/Network/URL.hpp"
#include "Libs/Network/URI.hpp"
#include "Libs/ParallelizableTrait.hpp"
#include "Libs/IO/IO.hpp"
#include "PrimaryServices.hpp"

namespace EmEn
{
	using namespace EmEn::Libs;

	const size_t NetworkManager::ClassUID{getClassUID(ClassId)};

	NetworkManager::NetworkManager (PrimaryServices & primaryServices) noexcept
		: ServiceInterface(ClassId), ParallelizableTrait(4),
		  m_primaryServices(primaryServices)
	{

	}

	size_t
	NetworkManager::classUID () const noexcept
	{
		return ClassUID;
	}

	bool
	NetworkManager::is (size_t classUID) const noexcept
	{
		return classUID == ClassUID;
	}

	bool
	NetworkManager::usable () const noexcept
	{
		return m_flags[ServiceInitialized];
	}

	bool
	NetworkManager::onInitialize () noexcept
	{
		m_downloadCacheDirectory = m_primaryServices.fileSystem().cacheDirectory(DownloadCacheDirectory);

		if ( IO::isDirectoryUsable(m_downloadCacheDirectory) )
		{
			m_flags[DownloadEnabled] = true;

			return this->checkDownloadCacheDBFile();
		}

		TraceWarning{ClassId} << "Unable to get the cache directory '" << m_downloadCacheDirectory << "' for download !";

		if ( !Network::hasInternetConnexion() )
		{
			TraceWarning{ClassId} << "There is no internet connexion yet.";
		}

		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	NetworkManager::onTerminate () noexcept
	{
		this->waitForAllJobsDone();

		return this->updateDownloadCacheDBFile();
	}

	std::filesystem::path
	NetworkManager::getDownloadCacheDBFilepath () const noexcept
	{
		return m_primaryServices.fileSystem().cacheDirectory(DownloadCacheDBFilename);
	}

	std::filesystem::path
	NetworkManager::getDownloadedCacheFilepath (size_t cacheId) const noexcept
	{
		std::stringstream filename;
		filename << "dlcached_" << cacheId;

		auto filepath = m_downloadCacheDirectory;
		filepath.append(filename.str());

		return filepath;
	}

	bool
	NetworkManager::updateDownloadCacheDBFile () const noexcept
	{
		const auto filepath = this->getDownloadCacheDBFilepath();

		Json::Value root{};
		root[FileDataBaseKey] = Json::arrayValue;

		if ( !m_downloadCache.empty() )
		{
			auto & fileDataBase = root[FileDataBaseKey];

			for ( const auto & [url, downloadedItem] : m_downloadCache )
			{
				TraceInfo{ClassId} << "Cached downloaded file ID #" << downloadedItem.cacheId() << " '" << downloadedItem.originalFilename() << "' (" << downloadedItem.filesize() << " bytes) registered.";

				Json::Value DBEntry = Json::objectValue;
				DBEntry[FileURLKey] = Json::Value{url.c_str()};
				DBEntry[CacheIdKey] = Json::Value{static_cast< Json::UInt64 >(downloadedItem.cacheId())};
				DBEntry[FilenameKey] = Json::Value{downloadedItem.originalFilename().c_str()};
				DBEntry[FilesizeKey] = Json::Value{static_cast< Json::UInt64 >(downloadedItem.filesize())};

				fileDataBase.append(DBEntry);
			}
		}

		Json::StreamWriterBuilder builder{};
		builder["commentStyle"] = "None";
		builder["indentation"] =  "\t";
		builder["enableYAMLCompatibility"] = false;
		builder["dropNullPlaceholders"] = true;
		builder["useSpecialFloats"] = true;
		builder["precision"] = 8;
		builder["precisionType"] = "significant";
		builder["emitUTF8"] = true;

		const auto jsonString = Json::writeString(builder, root);

		if ( jsonString.empty() )
		{
			TraceError{ClassId} << "Unable to write the download cache db file '" << filepath << "' !";

			return false;
		}

		return IO::filePutContents(filepath, jsonString);
	}

	bool
	NetworkManager::checkDownloadCacheDBFile () noexcept
	{
		const auto filepath = this->getDownloadCacheDBFilepath();

		/* Simply create an empty file, if it doesn't exist. */
		if ( !IO::fileExists(filepath) )
		{
			return this->updateDownloadCacheDBFile();
		}

		/* Read the JSON content. */
		Json::Value root;

		if ( !FastJSON::getRootFromFile(filepath, root) )
		{
			TraceError{ClassId} << "Unable to read the download cache db file '" << filepath << "' !";

			return false;
		}

		/* Check the root node of the JSON for files array. */
		if ( !root.isMember(FileDataBaseKey) )
		{
			TraceError{ClassId} << "The download cache db file do not have '" << FileDataBaseKey << "' key !";

			return false;
		}

		const auto & files = root[FileDataBaseKey];

		if ( !files.isArray() )
		{
			TraceError{ClassId} << "The '" << FileDataBaseKey << "' key in the download cache db file is not an array !";

			return false;
		}

		size_t highestCacheItemId = 0;

		for ( const auto & file : files )
		{
			/* Check file item JSON keys presence. */
			if ( !file.isMember(FileURLKey) || !file.isMember(CacheIdKey) || !file.isMember(FilenameKey) || !file.isMember(FilesizeKey) )
			{
				TraceWarning{ClassId} << "A file description in the download cache db file has not the required keys !";

				continue;
			}

			/* Check file item JSON keys value. */
			const auto & _fileURL = file[FileURLKey];
			const auto & _cacheId = file[CacheIdKey];
			const auto & _filename = file[FilenameKey];
			const auto & _filesize = file[FilesizeKey];

			if ( !_fileURL.isString() || !_cacheId.isIntegral() || !_filename.isString() || !_filesize.isIntegral() )
			{
				TraceWarning{ClassId} << "A file description in the download cache db file is invalid !";

				continue;
			}

			const auto fileURL = _fileURL.asString();
			const auto cacheId = static_cast< size_t >(_cacheId.asLargestUInt());
			const auto filename = _filename.asString();
			const auto filesize = static_cast< size_t >(_filesize.asLargestUInt());

			/* NOTE: Check the existence of the file in the directory cache. */
			const auto cacheFilepath = this->getDownloadedCacheFilepath(cacheId);

			if ( !IO::fileExists(cacheFilepath) )
			{
				TraceWarning{ClassId} << "The cached downloaded file ID #" << cacheId << " '" << cacheFilepath << "' no more exists !";

				continue;
			}

			TraceInfo{ClassId} << "Cached downloaded file ID #" << cacheId << " '" << filename << "' (" << filesize << " bytes) registered.";

			m_downloadCache.emplace(
				std::piecewise_construct,
				std::forward_as_tuple(fileURL),
				std::forward_as_tuple(cacheId, filename, filesize)
			);

			if ( cacheId > highestCacheItemId )
			{
				highestCacheItemId = cacheId;
			}
		}

		m_nextCacheItemId = highestCacheItemId + 1;

		return true;
	}

	bool
	NetworkManager::clearDownloadCache () noexcept
	{
		for ( const auto & downloadedItem : std::ranges::views::values(m_downloadCache) )
		{
			const auto cacheFilepath = this->getDownloadedCacheFilepath(downloadedItem.cacheId());

			if ( !IO::fileExists(cacheFilepath) )
			{
				continue;
			}

			if ( !IO::eraseFile(cacheFilepath) )
			{
				TraceError{ClassId} << "Unable to remove file ID #" << downloadedItem.cacheId() << " '" << cacheFilepath << "' no more exists !";

				return false;
			}
		}

		m_downloadCache.clear();

		return true;
	}

	bool
	NetworkManager::task (const size_t & data) noexcept
	{
		TraceInfo{ClassId} << "Launching the downloading task (" << data << ") ...";

		const auto & item = m_downloadItems.at(data);

		return Network::download(item.url(), item.output(), true);
	}

	int
	NetworkManager::download (const Network::URL & url, const std::filesystem::path & output, bool replaceExistingFile) noexcept
	{
		/* 1. Check if the download request is not already in queue. */
		int ticket = 0;

		for ( const auto & downloadRequest : m_downloadItems )
		{
			if ( to_string(url) == to_string(downloadRequest.url()) )
			{
				TraceInfo{ClassId} << url << " is already in downloading queue !";

				return ticket;
			}

			ticket++;
		}

		/* 2. If not, we create a new task. */
		ticket = static_cast< int >(m_downloadItems.size());

		m_downloadItems.emplace_back(url, output, replaceExistingFile);

		this->createTask(ticket);

		return ticket;
	}

	uint64_t
	NetworkManager::totalBytesTotal () const noexcept
	{
		return std::accumulate(m_downloadItems.cbegin(), m_downloadItems.cend(), 0UL, [] (uint64_t sum, const auto & item) {
			return sum + item.bytesTotal();
		});
	}

	uint64_t
	NetworkManager::totalBytesReceived () const noexcept
	{
		return std::accumulate(m_downloadItems.cbegin(), m_downloadItems.cend(), 0UL, [] (uint64_t sum, const auto & item) {
			return sum + item.bytesReceived();
		});
	}

	size_t
	NetworkManager::fileCount () const noexcept
	{
		return m_downloadItems.size();
	}

	size_t
	NetworkManager::fileCount (DownloadItem::Status filter) const noexcept
	{
		return std::ranges::count_if(m_downloadItems, [filter] (const auto & request){
			return request.status() == filter;
		});
	}

	size_t
	NetworkManager::fileRemainingCount () const noexcept
	{
		size_t count = 0;

		for ( const auto & request : m_downloadItems )
		{
			switch ( request.status() )
			{
				case DownloadItem::Status::Pending :
				case DownloadItem::Status::Transferring :
				case DownloadItem::Status::OnHold :
					count++;
					break;

				case DownloadItem::Status::Error :
				case DownloadItem::Status::Done :
					break;
			}
		}

		return count;
	}

	DownloadItem::Status
	NetworkManager::downloadStatus (int ticket) const noexcept
	{
		if ( ticket >= static_cast< int >(m_downloadItems.size()) )
		{
			return DownloadItem::Status::Error;
		}

		return m_downloadItems[ticket].status();
	}

	void
	NetworkManager::showProgressionInConsole (bool state) noexcept
	{
		m_flags[ShowProgression] = state;
	}

	bool
	NetworkManager::showProgressionInConsole () const noexcept
	{
		return m_flags[ShowProgression];
	}
}
