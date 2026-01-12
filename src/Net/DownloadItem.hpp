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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <string>
#include <filesystem>

/* Local inclusions for usages. */
#include "Libs/Network/URL.hpp"
#include "Types.hpp"
#include "Tracer.hpp"

namespace EmEn::Net
{
	/**
	 * @brief The DownloadItem class
	 */
	class DownloadItem final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DownloadItem"};

			/**
			 * @brief Constructs an item to download.
			 * @param url The download URL [std::move].
			 * @param output A reference to a filesystem path [std::move].
			 * @param replaceExistingFile Erase the file on exists if true.
			 */
			DownloadItem (Libs::Network::URL url, std::filesystem::path output, bool replaceExistingFile = true) noexcept
				: m_url{std::move(url)},
				m_output{std::move(output)},
				m_replaceExistingFile{replaceExistingFile}
			{

			}

			/**
			 * @brief Sets the current status.
			 * @param status The status.
			 * @return void
			 */
			void
			setStatus (DownloadStatus status) noexcept
			{
				switch ( status )
				{
					case DownloadStatus::Pending :
						Tracer::error(ClassId, "Cannot reset a download process to 'Pending' status, use 'OnHold' instead !");
						break;

					case DownloadStatus::Transferring :
					case DownloadStatus::OnHold :
					case DownloadStatus::Error :
					case DownloadStatus::Done :
						m_status = status;
						break;
				}
			}

			/**
			 * @brief Sets the downloading progression.
			 * @TODO Split up the total and received bytes.
			 * @param total
			 * @param received
			 * @return void
			 */
			void
			setProgression (uint64_t total, uint64_t received) noexcept
			{
				m_bytesTotal = total;
				m_bytesReceived = received;

				if ( m_bytesReceived >= m_bytesTotal )
				{
					m_status = DownloadStatus::Done;
				}
			}

			/**
			 * @brief Returns the URL of the downloaded item.
			 * @return const Libraries::Network::URL &
			 */
			[[nodiscard]]
			const Libs::Network::URL &
			url () const noexcept
			{
				return m_url;
			}

			/**
			 * @brief Returns the path to the file on disk.
			 * @return const std::filesystem::path &
			 */
			[[nodiscard]]
			const std::filesystem::path &
			output () const noexcept
			{
				return m_output;
			}

			/**
			 * @brief Returns the download response header.
			 * @return std::string &
			 */
			std::string &
			header () noexcept
			{
				return m_header;
			}

			/**
			 * @brief Returns the total bytes of the item.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			bytesTotal () const noexcept
			{
				return m_bytesTotal;
			}

			/**
			 * @brief Returns the actual bytes of the item downloaded.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			bytesReceived () const noexcept
			{
				return m_bytesReceived;
			}

			/**
			 * @brief Returns the download status.
			 * @return DownloadStatus
			 */
			[[nodiscard]]
			DownloadStatus
			status () const noexcept
			{
				return m_status;
			}

			/**
			 * @brief Returns whether the download item will replace an existing file on the disk.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			replaceExistingFile () const noexcept
			{
				return m_replaceExistingFile;
			}

		private:

			Libs::Network::URL m_url;
			std::filesystem::path m_output;
			std::string m_header;
			uint64_t m_bytesTotal{0};
			uint64_t m_bytesReceived{0};
			DownloadStatus m_status{DownloadStatus::Pending};
			bool m_replaceExistingFile;
	};
}
