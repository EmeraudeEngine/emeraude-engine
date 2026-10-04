/*
 * src/Resources/SharingServer.hpp
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
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

/* Local inclusions for usages. */
#include "Network/HTTPServer.hpp"

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
	 * @brief Serves this engine's data stores to a peer engine over HTTP, read-only (off by default).
	 * @note Endpoints (GET, and HEAD where a body exists):
	 *  - `/index.json` — every store resource: `Path` (relative to `data-stores/`) and `Size` for a local file,
	 *    the entry itself for an external or direct one (Resources::Manager builds it).
	 *  - `/files/<path>` — a data-store file, streamed, with a single `Range` honoured.
	 *  - `/list/<directory>` — the files under a data-store directory, recursively: `[{"path", "size"}]`.
	 *  - `/sha256/<path>` — a file's SHA-256, computed on the thread pool (never on the network thread).
	 * @note Every path comes from the NETWORK: percent-decoded, then confined under each data directory's
	 * `data-stores/` (Base::IO::confinedPath()); the first data directory holding it wins, as for a local load.
	 * @note Security is Base::Network::HTTPServer's: loopback by default, a bearer token mandatory off loopback.
	 * Owner decisions 2026-10-04: engine `docs/subsystems/resources/12-resource-sharing.md`.
	 */
	class SharingServer final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ResourceSharingServer"};

			/** @brief Most entries a `/list/` answer carries (a bound, not a page). */
			static constexpr size_t MaxListedFiles{100000};

			/** @brief Most digests computed at once on the thread pool; more are answered 503. */
			static constexpr size_t MaxDigestJobs{4};

			/** @brief Builds the index JSON. Called on the network thread: it must be thread-safe. */
			using IndexBuilder = std::function< std::string () >;

			/**
			 * @brief Constructs a stopped server.
			 * @param fileSystem A reference to the file system (its data directories).
			 * @param threadPool The thread pool computing the digests.
			 * @param indexBuilder Builds `/index.json`.
			 */
			SharingServer (const FileSystem & fileSystem, std::shared_ptr< Base::ThreadPool > threadPool, IndexBuilder indexBuilder) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			SharingServer (const SharingServer & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			SharingServer (SharingServer && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return SharingServer &
			 */
			SharingServer & operator= (const SharingServer & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return SharingServer &
			 */
			SharingServer & operator= (SharingServer && copy) noexcept = delete;

			/**
			 * @brief Stops the server, after the digests in flight finished.
			 */
			~SharingServer ();

			/**
			 * @brief Starts listening.
			 * @param address The bind address (an IP literal).
			 * @param port The TCP port.
			 * @param bearerToken The token; mandatory on a non-loopback address.
			 * @return bool False (logged) when the server could not start.
			 */
			[[nodiscard]]
			bool start (const std::string & address, uint16_t port, std::string bearerToken) noexcept;

			/**
			 * @brief Returns whether the server accepts connections.
			 * @return bool
			 */
			[[nodiscard]]
			bool isRunning () const noexcept;

			/**
			 * @brief Returns "http://<address>:<port>", empty when not running.
			 * @return std::string
			 */
			[[nodiscard]]
			std::string baseURL () const noexcept;

			/**
			 * @brief Returns the media type served for a file name, by its extension.
			 * @param filepath The file path.
			 * @return std::string_view
			 */
			[[nodiscard]]
			static std::string_view contentType (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Computes the SHA-256 of a file, streamed (1 MiB at a time).
			 * @param filepath The file.
			 * @param cancel Optional: raised by another thread, it stops the computation (a 1.6 GB file takes seconds).
			 * @return std::optional< std::string > The lowercase hexadecimal digest, nothing on a read error or a cancel.
			 */
			[[nodiscard]]
			static std::optional< std::string > fileSHA256 (const std::filesystem::path & filepath, const std::atomic< bool > * cancel = nullptr) noexcept;

		private:

			using Connection = Base::Network::HTTPServerConnection;

			/**
			 * @brief Routes a request that passed the HTTP checks (network thread).
			 * @param connection The connection.
			 * @return void
			 */
			void handleRequest (const std::shared_ptr< Connection > & connection) noexcept;

			/**
			 * @brief Resolves a percent-encoded data-store path to an existing entry, confined.
			 * @param encodedPath The path as it came in the target, after its prefix.
			 * @param directory Whether a directory is wanted instead of a regular file.
			 * @return std::optional< std::filesystem::path >
			 */
			[[nodiscard]]
			std::optional< std::filesystem::path > resolve (std::string_view encodedPath, bool directory) const noexcept;

			/**
			 * @brief Answers `/list/<directory>`.
			 * @param connection The connection.
			 * @param directory The resolved directory.
			 * @param relativeRoot The directory relative to `data-stores/` ('/' separated, may be empty).
			 * @return void
			 */
			static void answerList (Connection & connection, const std::filesystem::path & directory, const std::string & relativeRoot) noexcept;

			/**
			 * @brief Answers `/sha256/<path>` from the thread pool.
			 * @param connection The connection.
			 * @param filepath The resolved file.
			 * @return void
			 */
			void answerDigest (const std::shared_ptr< Connection > & connection, const std::filesystem::path & filepath) noexcept;

			const FileSystem & m_fileSystem;
			std::shared_ptr< Base::ThreadPool > m_threadPool;
			IndexBuilder m_indexBuilder;
			std::unique_ptr< Base::Network::HTTPServer > m_http;
			std::mutex m_jobsAccess;
			std::condition_variable m_jobsDone;
			size_t m_jobsInFlight{0};
	};
}
