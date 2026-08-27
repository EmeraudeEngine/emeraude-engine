/*
 * src/Net/DownloadItem.hpp
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
#include <cstdint>
#include <filesystem>
#include <utility>

/* Local inclusions for usages. */
#include "Network/URI.hpp"
#include "Types.hpp"

namespace EmEn::Net
{
	/**
	 * @brief One download request tracked by Net::Manager, addressed by its ticket.
	 * @note Plain data guarded by the manager's mutex; it carries no logic of its own.
	 */
	class EMEN_API DownloadItem final
	{
		public:

			/**
			 * @brief Constructs a download item.
			 * @param url The source URL [std::move].
			 * @param filepath The final location of the file in the download cache [std::move].
			 */
			DownloadItem (Base::Network::URI url, std::filesystem::path filepath) noexcept
				: m_url{std::move(url)},
				m_filepath{std::move(filepath)}
			{

			}

			/**
			 * @brief Returns the source URL.
			 * @return const Base::Network::URI &
			 */
			[[nodiscard]]
			const Base::Network::URI &
			url () const noexcept
			{
				return m_url;
			}

			/**
			 * @brief Returns where the file lives once the status is Done.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			filepath () const noexcept
			{
				return m_filepath;
			}

			/**
			 * @brief Returns the size of the downloaded file in bytes, 0 until Done.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			bytes () const noexcept
			{
				return m_bytes;
			}

			/**
			 * @brief Returns the body bytes received so far (the final size once Done).
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			bytesReceived () const noexcept
			{
				return m_bytesReceived;
			}

			/**
			 * @brief Returns the expected total in bytes, 0 when the server did not announce it.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			bytesTotal () const noexcept
			{
				return m_bytesTotal;
			}

			/**
			 * @brief Returns whether a progress update was recorded since the last Progress notification.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasPendingProgress () const noexcept
			{
				return m_progressPending;
			}

			/**
			 * @brief Records a progress update from the transfer thread.
			 * @param received Body bytes received so far.
			 * @param total Expected total, 0 when unknown.
			 * @return void
			 */
			void
			setProgress (uint64_t received, uint64_t total) noexcept
			{
				m_bytesReceived = received;
				m_bytesTotal = total;
				m_progressPending = true;
			}

			/**
			 * @brief Clears the pending-progress flag once the Progress notification went out.
			 * @return void
			 */
			void
			clearPendingProgress () noexcept
			{
				m_progressPending = false;
			}

			/**
			 * @brief Returns the status.
			 * @return DownloadStatus
			 */
			[[nodiscard]]
			DownloadStatus
			status () const noexcept
			{
				return m_status;
			}

			/**
			 * @brief Marks the item as being fetched by a worker.
			 * @return void
			 */
			void
			setTransferring () noexcept
			{
				m_status = DownloadStatus::Transferring;
			}

			/**
			 * @brief Marks the item as complete, the file being at filepath().
			 * @param bytes The file size.
			 * @return void
			 */
			void
			setDone (uint64_t bytes) noexcept
			{
				m_bytes = bytes;
				m_bytesReceived = bytes;
				m_bytesTotal = bytes;
				m_progressPending = false;
				m_status = DownloadStatus::Done;
			}

			/**
			 * @brief Marks the item as failed; no file is left behind.
			 * @return void
			 */
			void
			setError () noexcept
			{
				m_bytes = 0;
				m_progressPending = false;
				m_status = DownloadStatus::Error;
			}

		private:

			Base::Network::URI m_url;
			std::filesystem::path m_filepath;
			uint64_t m_bytes{0};
			uint64_t m_bytesReceived{0};
			uint64_t m_bytesTotal{0};
			DownloadStatus m_status{DownloadStatus::Pending};
			bool m_progressPending{false};
	};
}
