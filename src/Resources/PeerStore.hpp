/*
 * src/Resources/PeerStore.hpp
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

/* STL inclusions. */
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "json/json.h"

/* Local inclusions for usages. */
#include "Network/HTTPSClient.hpp"
#include "Network/URI.hpp"

/* Forward declarations. */
namespace EmEn
{
	class FileSystem;
}

namespace EmEn::Base
{
	class ThreadPool;
}

namespace EmEn::Resources
{
	/**
	 * @brief The client side of resource sharing: a peer engine's SharingServer (the Core/Resources/Peer settings).
	 * @note Two uses: fetchIndex() reads the peer's store index once, at start-up (Resources::Manager merges the
	 * names it lacks as downloads); fetch() copies a data-store file or directory of the peer into this engine's
	 * data stores, on the thread pool, each file verified by its SHA-256.
	 * @note Cleartext HTTP, to a private address only (Base::Network::HTTPSClientOptions::allowPrivateCleartext):
	 * owner decision 2026-10-04, engine `docs/subsystems/resources/12-resource-sharing.md`.
	 */
	class PeerStore final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ResourcePeerStore"};

			/** @brief Most errors kept in the status. */
			static constexpr size_t MaxReportedErrors{20};

			/**
			 * @brief Constructs a peer store.
			 * @param fileSystem A reference to the file system (where fetched files go).
			 * @param threadPool The thread pool running fetch().
			 * @param baseURL The peer's "http://host:port".
			 * @param bearerToken The peer's token, empty for none.
			 */
			PeerStore (const FileSystem & fileSystem, std::shared_ptr< Base::ThreadPool > threadPool, const Base::Network::URI & baseURL, std::string bearerToken) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PeerStore (const PeerStore & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PeerStore (PeerStore && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return PeerStore &
			 */
			PeerStore & operator= (const PeerStore & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return PeerStore &
			 */
			PeerStore & operator= (PeerStore && copy) noexcept = delete;

			/**
			 * @brief Cancels a fetch in flight and waits for it.
			 */
			~PeerStore ();

			/**
			 * @brief Returns the peer's base URL, without a trailing '/'.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			baseURL () const noexcept
			{
				return m_baseURL;
			}

			/**
			 * @brief Returns the URL of a data-store file on the peer.
			 * @param relativePath The path relative to `data-stores/` ('/' separated).
			 * @return std::string
			 */
			[[nodiscard]]
			std::string fileURL (const std::string & relativePath) const noexcept;

			/**
			 * @brief Reads the peer's store index. Blocking, bounded (a few seconds when the peer is away).
			 * @return std::optional< Json::Value > The index root (an object), nothing on any failure (logged).
			 */
			[[nodiscard]]
			std::optional< Json::Value > fetchIndex () const noexcept;

			/**
			 * @brief Starts copying a data-store file, or every file under a directory, from the peer.
			 * @note On the thread pool. A file already present with the peer's size is kept. Each file is written to
			 * a side file, renamed, then its SHA-256 compared with the peer's (a mismatch deletes it).
			 * @param relativePath The path relative to `data-stores/` ('/' separated); empty = everything.
			 * @return bool False when a fetch already runs or no thread pool is available.
			 */
			[[nodiscard]]
			bool fetch (const std::string & relativePath) noexcept;

			/**
			 * @brief Returns the state of the last fetch as JSON.
			 * @return Json::Value
			 */
			[[nodiscard]]
			Json::Value status () const noexcept;

			/**
			 * @brief Asks a fetch in flight to stop (at the next read).
			 * @return void
			 */
			void cancel () noexcept;

		private:

			/** @brief One file the peer listed. */
			struct RemoteFile
			{
				std::string path;
				std::optional< uint64_t > size;
			};

			/**
			 * @brief Performs a GET into memory, with the token.
			 * @param target The target, after the base URL (starting with '/').
			 * @param report Receives the outcome.
			 * @return std::optional< Base::Network::HTTPResult >
			 */
			[[nodiscard]]
			std::optional< Base::Network::HTTPResult > get (const std::string & target, Base::Network::DownloadReport & report) const noexcept;

			/**
			 * @brief Lists what a fetch copies: the peer's listing of a directory, or the path itself as a file.
			 * @param relativePath The path.
			 * @return std::optional< std::vector< RemoteFile > > Nothing when the peer could not be asked.
			 */
			[[nodiscard]]
			std::optional< std::vector< RemoteFile > > remoteFiles (const std::string & relativePath) const noexcept;

			/**
			 * @brief Returns the first data directory's `data-stores/` that exists.
			 * @return std::optional< std::filesystem::path >
			 */
			[[nodiscard]]
			std::optional< std::filesystem::path > destinationRoot () const noexcept;

			/**
			 * @brief Copies one file and verifies it (worker).
			 * @param root The local `data-stores/`.
			 * @param file The remote file.
			 * @return void
			 */
			void fetchFile (const std::filesystem::path & root, const RemoteFile & file) noexcept;

			/**
			 * @brief Runs a fetch (worker).
			 * @param relativePath The path.
			 * @return void
			 */
			void runFetch (const std::string & relativePath) noexcept;

			/**
			 * @brief Records an error in the status (and the log).
			 * @param message The message.
			 * @return void
			 */
			void reportError (const std::string & message) noexcept;

			/** @brief The state a status() reports. */
			struct State
			{
				std::string request;
				std::string currentFile;
				std::vector< std::string > errors;
				uint64_t currentBytes{0};
				uint64_t currentTotal{0};
				uint64_t bytesReceived{0};
				size_t filesListed{0};
				size_t filesFetched{0};
				size_t filesKept{0};
				size_t filesFailed{0};
				bool running{false};
				bool truncatedListing{false};
			};

			const FileSystem & m_fileSystem;
			std::shared_ptr< Base::ThreadPool > m_threadPool;
			std::string m_baseURL;
			std::string m_bearerToken;
			std::unique_ptr< asio::ssl::context > m_tlsContext;
			std::unique_ptr< Base::Network::HTTPSClient > m_requestClient;
			std::unique_ptr< Base::Network::HTTPSClient > m_downloadClient;
			mutable std::mutex m_stateAccess;
			std::condition_variable m_jobDone;
			State m_state;
			std::atomic< bool > m_cancel{false};
	};
}
