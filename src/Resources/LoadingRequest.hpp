/*
 * src/Resources/LoadingRequest.hpp
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
#include <filesystem>
#include <memory>

/* Local inclusions for usages. */
#include "BaseInformation.hpp"

/* Forward declarations. */
namespace EmEn
{
	class FileSystem;

	namespace Base::Network
	{
		class URL;
	}
}

namespace EmEn::Resources
{
	class ResourceTrait;

	/**
	 * @class LoadingRequest
	 * @brief Encapsulates a resource loading request with download state management.
	 *
	 * LoadingRequest handles the complete lifecycle of a resource loading operation, including
	 * local file access, external URL downloads, and direct data loading. It manages download
	 * tickets for asynchronous network operations and tracks the loading state through a
	 * finite state machine.
	 *
	 * **Download Ticket States:**
	 * - DownloadNotRequested (-4): No download needed (local or direct data)
	 * - DownloadError (-3): Download failed
	 * - DownloadSuccess (-2): Download completed successfully
	 * - DownloadPending (-1): Waiting to be submitted to download manager
	 * - Positive values: Active download ticket from the network manager
	 *
	 * **Source Types:**
	 * - LocalData: Load from filesystem path
	 * - ExternalData: Download from URL, cache locally, then load
	 * - DirectData: Load from in-memory JSON data
	 *
	 * The request automatically determines the cache filepath for external resources and handles
	 * the conversion from external URLs to cached local files after successful downloads.
	 *
	 * @note This class is deliberately **not** a template: it only ever needs the polymorphic
	 * ResourceTrait interface (ResourceTrait::load() is virtual) plus the resource type's ClassId,
	 * which is passed as a plain string at construction. Keeping it non-template lets every heavy
	 * method body live in the implementation file, which keeps FileSystem, Network::URL, String
	 * and IO out of Container.hpp — a header parsed by a large part of the engine.
	 *
	 * @see Container For the resource container that uses this request type.
	 * @see BaseInformation For resource metadata.
	 * @see SourceType Enum defining resource data sources.
	 * @version 0.8.40
	 */
	class EMEN_API LoadingRequest final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LoadingRequest"};

			/**
			 * @brief Constructs a loading request with resource metadata.
			 *
			 * Initializes the loading request and sets the appropriate download ticket state based
			 * on the source type. For external data sources, validates the URL and sets the ticket
			 * to DownloadPending if valid, or DownloadError if invalid.
			 *
			 * @param baseInformation Resource metadata including source type and data location [std::move].
			 * @param resource Shared pointer to the target resource object that will be populated [std::move].
			 * @param resourceClassId The ClassId of the concrete resource type, used to build the cache path.
			 * Must be a string with static storage duration (a resource's `ClassId` constant).
			 * @version 0.8.40
			 */
			LoadingRequest (BaseInformation baseInformation, std::shared_ptr< ResourceTrait > resource, const char * resourceClassId) noexcept;

			/**
			 * @brief Returns the cache file path for downloaded external resources.
			 *
			 * Constructs the filesystem path where downloaded external resources are cached locally.
			 * The path structure is: `[cache_dir]/data/[resource_type]/[filename]`
			 *
			 * Example: `~/.cache/emeraude/data/Texture2D/albedo.png`
			 *
			 * @param fileSystem Reference to the filesystem service for cache directory location.
			 * @return Full filesystem path to the cached resource file.
			 * @version 0.8.40
			 */
			[[nodiscard]]
			std::filesystem::path cacheFilepath (const FileSystem & fileSystem) const noexcept;

			/**
			 * @brief Returns the base information metadata for this request.
			 * @return Const reference to the resource's base information (name, source type, data location).
			 * @version 0.8.40
			 */
			[[nodiscard]]
			const BaseInformation &
			baseInformation () const noexcept
			{
				return m_baseInformation;
			}

			/**
			 * @brief Returns the target resource object for this loading request.
			 * @return Const reference to the shared pointer to the resource that will be populated when loading completes.
			 * @version 0.8.40
			 */
			[[nodiscard]]
			const std::shared_ptr< ResourceTrait > &
			resource () const noexcept
			{
				return m_resource;
			}

			/**
			 * @brief Returns the download manager ticket number.
			 *
			 * Returns the ticket assigned by the network download manager for tracking this download.
			 * A return value of 0 indicates no active download (either not needed or already completed).
			 *
			 * @return Download ticket number, or 0 if no active download.
			 * @see isDownloadable() To check if download is pending.
			 * @version 0.8.40
			 */
			[[nodiscard]]
			int
			downloadTicket () const noexcept
			{
				if ( m_downloadTicket < 0 )
				{
					return 0;
				}

				return m_downloadTicket;
			}

			/**
			 * @brief Checks if the request is ready to be submitted for download.
			 *
			 * Returns true only if this is an external data request currently in the DownloadPending
			 * state. Requests in this state are waiting to be submitted to the network download manager.
			 *
			 * @return True if the request can be submitted for download, false otherwise.
			 * @version 0.8.40
			 */
			[[nodiscard]]
			bool isDownloadable () const noexcept;

			/**
			 * @brief Returns the download URL for external data requests.
			 *
			 * Extracts and returns the URL from the base information data field. Returns an
			 * empty URL if this is not an external data request.
			 *
			 * @return URL object for the resource download, or empty URL for non-external requests.
			 * @version 0.8.40
			 */
			[[nodiscard]]
			Base::Network::URL url () const noexcept;

			/**
			 * @brief Checks if the resource download is currently in progress.
			 *
			 * Returns true if this is an external data request with a positive download ticket,
			 * indicating the download has been submitted to the network manager but not yet completed.
			 *
			 * @return True if download is active, false otherwise.
			 * @version 0.8.40
			 */
			[[nodiscard]]
			bool isDownloading () const noexcept;

			/**
			 * @brief Assigns a download manager ticket to this request.
			 *
			 * Updates the request's download ticket after successfully submitting it to the network
			 * download manager. This transitions the state from DownloadPending to actively downloading.
			 *
			 * @param ticket Positive ticket number assigned by the network download manager.
			 * @pre Request must be in DownloadPending state (ticket == -1).
			 * @pre Request must be of SourceType::ExternalData.
			 * @warning Calling with invalid preconditions generates error traces.
			 * @version 0.8.40
			 */
			void setDownloadTicket (int ticket) noexcept;

			/**
			 * @brief Marks the download as completed (successfully or with error).
			 *
			 * Updates the request state after download completion. On success, updates the base
			 * information to point to the cached local file instead of the original URL. On failure,
			 * sets the ticket to DownloadError state.
			 *
			 * @param fileSystem Reference to filesystem service for cache path resolution.
			 * @param success True if download succeeded, false if it failed.
			 * @post On success: ticket becomes DownloadSuccess, baseInformation updated to cache path.
			 * @post On failure: ticket becomes DownloadError.
			 * @version 0.8.40
			 */
			void setDownloadProcessed (const FileSystem & fileSystem, bool success) noexcept;

		private:

			/* Special ticket flags. */
			static constexpr auto DownloadNotRequested{-4};
			static constexpr auto DownloadError{-3};
			static constexpr auto DownloadSuccess{-2};
			static constexpr auto DownloadPending{-1};

			BaseInformation m_baseInformation;
			std::shared_ptr< ResourceTrait > m_resource;
			const char * m_resourceClassId;
			int m_downloadTicket{DownloadNotRequested};
	};
}
