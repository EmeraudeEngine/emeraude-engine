/*
 * src/Resources/LoadingRequest.cpp
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

#include "LoadingRequest.hpp"

/* STL inclusions. */
#include <utility>

/* Local inclusions. */
#include "FileSystem.hpp"
#include "Network/URL.hpp"
#include "String.hpp"
#include "Tracer.hpp"
#include "Types.hpp"

namespace EmEn::Resources
{
	using namespace Base;

	LoadingRequest::LoadingRequest (BaseInformation baseInformation, std::shared_ptr< ResourceTrait > resource, const char * resourceClassId) noexcept
		: m_baseInformation{std::move(baseInformation)},
		m_resource{std::move(resource)},
		m_resourceClassId{resourceClassId}
	{
		switch ( m_baseInformation.sourceType() )
		{
			case SourceType::Undefined :
				Tracer::error(ClassId, "Undefined type for resource request !");
				break;

			case SourceType::LocalData :
				break;

			case SourceType::ExternalData :
			{
				const Network::URL resourceUrl{m_baseInformation.data().asString()};

				if ( resourceUrl.isValid() )
				{
					m_downloadTicket = DownloadPending;
				}
				else
				{
					TraceError{ClassId} << "'" << resourceUrl << "' is not a valid URL ! Download cancelled ...";

					m_downloadTicket = DownloadError;
				}
			}
				break;

			case SourceType::DirectData :
				break;
		}
	}

	std::filesystem::path
	LoadingRequest::cacheFilepath (const FileSystem & fileSystem) const noexcept
	{
		std::filesystem::path filepath{fileSystem.cacheDirectory()};
		filepath.append("data");
		filepath.append(m_resourceClassId);
		filepath.append(String::extractFilename(m_baseInformation.data().asString()));

		return filepath;
	}

	bool
	LoadingRequest::isDownloadable () const noexcept
	{
		if ( m_baseInformation.sourceType() != SourceType::ExternalData ) [[unlikely]]
		{
			Tracer::error(ClassId, "This request is not external !");

			return false;
		}

		return m_downloadTicket == DownloadPending;
	}

	Network::URL
	LoadingRequest::url () const noexcept
	{
		if ( m_baseInformation.sourceType() != SourceType::ExternalData )
		{
			return {};
		}

		return Network::URL{m_baseInformation.data().asString()};
	}

	bool
	LoadingRequest::isDownloading () const noexcept
	{
		if ( m_baseInformation.sourceType() != SourceType::ExternalData ) [[unlikely]]
		{
			Tracer::error(ClassId, "This request is not external !");

			return false;
		}

		/* NOTE: Check the networkManager ticket.
		 * If it's still present, the download
		 * is not yet finished. */
		if ( m_downloadTicket >= 0 )
		{
			return false;
		}

		return true;
	}

	void
	LoadingRequest::setDownloadTicket (int ticket) noexcept
	{
		if ( m_baseInformation.sourceType() != SourceType::ExternalData ) [[unlikely]]
		{
			Tracer::error(ClassId, "This request is not external !");

			return;
		}

		if ( m_downloadTicket != DownloadPending ) [[unlikely]]
		{
			Tracer::error(ClassId, "Cannot set a ticket to a request which is not in 'DownloadPending' status !");

			return;
		}

		m_downloadTicket = ticket;
	}

	void
	LoadingRequest::setDownloadProcessed (const FileSystem & fileSystem, bool success) noexcept
	{
		if ( m_baseInformation.sourceType() != SourceType::ExternalData ) [[unlikely]]
		{
			Tracer::error(ClassId, "This request is not external !");

			return;
		}

		/* Invalidate the networkManager ticket. */
		if ( success ) [[likely]]
		{
			m_downloadTicket = DownloadSuccess;

			m_baseInformation.updateFromDownload(this->cacheFilepath(fileSystem));
		}
		else
		{
			m_downloadTicket = DownloadError;
		}
	}
}
