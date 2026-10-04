/*
 * src/Resources/SharingServer.cpp
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


#include "SharingServer.hpp"

/* STL inclusions. */
#include <array>
#include <fstream>
#include <utility>
#include <vector>

/* Local inclusions. */
#include "FastJSON.hpp"
#include "FileSystem.hpp"
#include "Hash/SHA256.hpp"
#include "IO/IO.hpp"
#include "Network/PercentEncoding.hpp"
#include "String.hpp"
#include "ThreadPool.hpp"
#include "Tracer.hpp"
#include "Types.hpp"

namespace EmEn::Resources
{
	using namespace Base;

	namespace
	{
		constexpr std::string_view IndexPath{"/index.json"};
		constexpr std::string_view FilesPrefix{"/files/"};
		constexpr std::string_view ListPrefix{"/list/"};
		constexpr std::string_view DigestPrefix{"/sha256/"};

		/** @brief The index and the listings change with the stores: never cached by a client. */
		constexpr std::string_view NoStore{"Cache-Control: no-store\r\n"};

		/**
		 * @brief Returns a path relative to 'base' in the '/' form an URL carries.
		 * @param path The path.
		 * @param base The base.
		 * @return std::string
		 */
		[[nodiscard]]
		std::string
		genericRelative (const std::filesystem::path & path, const std::filesystem::path & base) noexcept
		{
			auto relative = IO::toU8String(path.lexically_relative(base));

			if constexpr ( IsWindows )
			{
				relative = String::replace('\\', '/', relative);
			}

			return relative;
		}
	}

	SharingServer::SharingServer (const FileSystem & fileSystem, std::shared_ptr< ThreadPool > threadPool, IndexBuilder indexBuilder) noexcept
		: m_fileSystem{fileSystem},
		m_threadPool{std::move(threadPool)},
		m_indexBuilder{std::move(indexBuilder)}
	{

	}

	SharingServer::~SharingServer ()
	{
		if ( m_http == nullptr )
		{
			return;
		}

		const auto wasRunning = m_http->isRunning();

		/* No new digest can start once the network thread is gone; the ones in flight post into a stopped loop,
		 * which drops them, but they read m_http: wait for them before it goes. */
		m_http->stop();

		{
			std::unique_lock< std::mutex > lock{m_jobsAccess};

			m_jobsDone.wait(lock, [this] () {
				return m_jobsInFlight == 0;
			});
		}

		if ( wasRunning )
		{
			TraceInfo{ClassId} << "Resource sharing server stopped.";
		}
	}

	bool
	SharingServer::start (const std::string & address, uint16_t port, std::string bearerToken) noexcept
	{
		if ( m_http != nullptr && m_http->isRunning() )
		{
			return true;
		}

		Network::HTTPServerOptions options;
		options.address = address;
		options.port = port;
		options.bearerToken = std::move(bearerToken);
		options.name = "Resource sharing server";
		/* Requests carry no body here; a listing or a large file keeps a connection busy longer. */
		options.maxBodyBytes = 0;
		options.maxConnections = 32;

		m_http = std::make_unique< Network::HTTPServer >(std::move(options));

		if ( !m_http->start([this] (const std::shared_ptr< Connection > & connection) {
			this->handleRequest(connection);
		}) )
		{
			TraceError{ClassId} << "The resource sharing server stays closed (see the error above; Core/Resources/Sharing/*).";

			return false;
		}

		TraceSuccess{ClassId} << "Resource sharing server listening on " << m_http->baseURL() << " (read-only, " << ( m_http->options().bearerToken.empty() ? "no token: loopback only" : "bearer token required" ) << ").";

		return true;
	}

	bool
	SharingServer::isRunning () const noexcept
	{
		return m_http != nullptr && m_http->isRunning();
	}

	std::string
	SharingServer::baseURL () const noexcept
	{
		return m_http != nullptr ? m_http->baseURL() : std::string{};
	}

	std::string_view
	SharingServer::contentType (const std::filesystem::path & filepath) noexcept
	{
		const auto extension = String::toLower(IO::toU8String(filepath.extension()));

		if ( extension == ".json" ) { return "application/json"; }
		if ( extension == ".png" ) { return "image/png"; }
		if ( extension == ".jpg" || extension == ".jpeg" ) { return "image/jpeg"; }
		if ( extension == ".webp" ) { return "image/webp"; }
		if ( extension == ".ktx2" ) { return "image/ktx2"; }
		if ( extension == ".gltf" ) { return "model/gltf+json"; }
		if ( extension == ".glb" ) { return "model/gltf-binary"; }
		if ( extension == ".usdz" ) { return "model/vnd.usdz+zip"; }
		if ( extension == ".wav" ) { return "audio/wav"; }
		if ( extension == ".ogg" ) { return "audio/ogg"; }
		if ( extension == ".mp3" ) { return "audio/mpeg"; }
		if ( extension == ".txt" ) { return "text/plain; charset=utf-8"; }

		return "application/octet-stream";
	}

	std::optional< std::string >
	SharingServer::fileSHA256 (const std::filesystem::path & filepath, const std::atomic< bool > * cancel) noexcept
	{
		std::ifstream file{filepath, std::ios::binary};

		if ( !file.is_open() )
		{
			return std::nullopt;
		}

		Hash::SHA256 hash;
		std::vector< char > buffer(size_t{1024} * 1024);

		while ( file )
		{
			if ( cancel != nullptr && cancel->load() )
			{
				return std::nullopt;
			}

			file.read(buffer.data(), static_cast< std::streamsize >(buffer.size()));

			const auto readBytes = file.gcount();

			if ( readBytes > 0 )
			{
				hash.update(reinterpret_cast< const uint8_t * >(buffer.data()), static_cast< size_t >(readBytes));
			}
		}

		if ( file.bad() )
		{
			return std::nullopt;
		}

		std::array< uint8_t, 32 > digest{};
		hash.final(digest);

		constexpr std::string_view Hexadecimal{"0123456789abcdef"};

		std::string text;
		text.reserve(digest.size() * 2);

		for ( const auto byte : digest )
		{
			text += Hexadecimal[byte >> 4U];
			text += Hexadecimal[byte & 0x0FU];
		}

		return text;
	}

	std::optional< std::filesystem::path >
	SharingServer::resolve (std::string_view encodedPath, bool directory) const noexcept
	{
		const auto decoded = Network::PercentEncoding::decode(encodedPath);

		/* NOTE: a NUL would cut the path in a system call; a backslash is a separator on Windows only, so the same
		 * request would mean two things on two OS. Both refused. */
		if ( decoded.find('\0') != std::string::npos || decoded.find('\\') != std::string::npos )
		{
			return std::nullopt;
		}

		const auto relative = IO::u8path(decoded);

		for ( const auto & dataDirectory : m_fileSystem.dataDirectories() )
		{
			auto confined = IO::confinedPath(dataDirectory / DataStores, relative);

			if ( !confined )
			{
				return std::nullopt;
			}

			std::error_code errorCode;

			if ( directory ? std::filesystem::is_directory(*confined, errorCode) : std::filesystem::is_regular_file(*confined, errorCode) )
			{
				return confined;
			}
		}

		return std::nullopt;
	}

	void
	SharingServer::handleRequest (const std::shared_ptr< Connection > & connection) noexcept
	{
		const auto & request = connection->request();
		const auto isGet = request.method == "GET";

		if ( !isGet && request.method != "HEAD" )
		{
			connection->respondEmpty(405, "Allow: GET, HEAD\r\n");

			return;
		}

		const auto path = request.path();

		if ( path == IndexPath )
		{
			connection->respond(200, "application/json", m_indexBuilder ? m_indexBuilder() : std::string{"{}"}, NoStore);

			return;
		}

		if ( path.starts_with(FilesPrefix) )
		{
			const auto filepath = this->resolve(std::string_view{path}.substr(FilesPrefix.size()), false);

			if ( !filepath )
			{
				connection->respondEmpty(404);

				return;
			}

			connection->respondFile(*filepath, SharingServer::contentType(*filepath));

			return;
		}

		if ( path.starts_with(ListPrefix) || path == ListPrefix.substr(0, ListPrefix.size() - 1) )
		{
			const auto encoded = path.size() > ListPrefix.size() ? std::string_view{path}.substr(ListPrefix.size()) : std::string_view{};
			const auto directory = this->resolve(encoded, true);

			if ( !directory )
			{
				connection->respondEmpty(404);

				return;
			}

			auto relativeRoot = Network::PercentEncoding::decode(encoded);

			while ( relativeRoot.ends_with('/') )
			{
				relativeRoot.pop_back();
			}

			SharingServer::answerList(*connection, *directory, relativeRoot);

			return;
		}

		if ( path.starts_with(DigestPrefix) && isGet )
		{
			const auto filepath = this->resolve(std::string_view{path}.substr(DigestPrefix.size()), false);

			if ( !filepath )
			{
				connection->respondEmpty(404);

				return;
			}

			this->answerDigest(connection, *filepath);

			return;
		}

		connection->respondEmpty(404);
	}

	void
	SharingServer::answerList (Connection & connection, const std::filesystem::path & directory, const std::string & relativeRoot) noexcept
	{
		Json::Value files{Json::arrayValue};
		bool truncated = false;

		const auto walked = IO::forEachDirectoryEntry(directory, true, [&] (const std::filesystem::directory_entry & entry) {
			std::error_code errorCode;

			if ( !entry.is_regular_file(errorCode) || errorCode )
			{
				return true;
			}

			if ( files.size() >= MaxListedFiles )
			{
				truncated = true;

				return false;
			}

			const auto size = entry.file_size(errorCode);

			if ( errorCode )
			{
				return true;
			}

			const auto relative = genericRelative(entry.path(), directory);

			Json::Value file{Json::objectValue};
			file["path"] = relativeRoot.empty() ? relative : relativeRoot + '/' + relative;
			file["size"] = static_cast< Json::UInt64 >(size);

			files.append(std::move(file));

			return true;
		});

		if ( !walked )
		{
			connection.respondEmpty(500);

			return;
		}

		Json::Value answer{Json::objectValue};
		answer["files"] = std::move(files);
		answer["truncated"] = truncated;

		connection.respond(200, "application/json", FastJSON::stringify(answer), NoStore);
	}

	void
	SharingServer::answerDigest (const std::shared_ptr< Connection > & connection, const std::filesystem::path & filepath) noexcept
	{
		{
			const std::scoped_lock lock{m_jobsAccess};

			if ( m_threadPool == nullptr || m_jobsInFlight >= MaxDigestJobs )
			{
				connection->respondEmpty(503, "Retry-After: 5\r\n");

				return;
			}

			++m_jobsInFlight;
		}

		const std::weak_ptr< Connection > weakConnection = connection;

		const auto enqueued = m_threadPool->enqueue([this, weakConnection, filepath] () {
			auto digest = SharingServer::fileSHA256(filepath);

			std::error_code errorCode;
			const auto size = std::filesystem::file_size(filepath, errorCode);

			m_http->post([weakConnection, digest = std::move(digest), size = errorCode ? 0 : size] () {
				const auto alive = weakConnection.lock();

				if ( alive == nullptr )
				{
					return;
				}

				if ( !digest )
				{
					alive->respondEmpty(500);

					return;
				}

				Json::Value answer{Json::objectValue};
				answer["sha256"] = *digest;
				answer["size"] = static_cast< Json::UInt64 >(size);

				alive->respond(200, "application/json", FastJSON::stringify(answer), NoStore);
			});

			{
				const std::scoped_lock lock{m_jobsAccess};

				--m_jobsInFlight;
			}

			m_jobsDone.notify_all();
		});

		if ( !enqueued )
		{
			{
				const std::scoped_lock lock{m_jobsAccess};

				--m_jobsInFlight;
			}

			connection->respondEmpty(503, "Retry-After: 5\r\n");
		}
	}
}
