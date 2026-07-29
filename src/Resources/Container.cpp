/*
 * src/Resources/Container.cpp
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

#include "Container.hpp"

/* STL inclusions. */
#include <utility>

/* Local inclusions. */
#include "FileSystem.hpp"
#include "IO/IO.hpp"
#include "Net/Manager.hpp"
#include "PrimaryServices.hpp"
#include "ThreadPool.hpp"

namespace EmEn::Resources::ServiceAccess
{
	Base::ObservableTrait *
	netManagerObservable (PrimaryServices & primaryServices) noexcept
	{
		return &primaryServices.netManager();
	}

	bool
	isNetManagerObservable (const Base::ObservableTrait * observable) noexcept
	{
		return observable->is(Net::Manager::getClassUID());
	}

	int
	fileDownloadedNotificationCode () noexcept
	{
		return Net::Manager::FileDownloaded;
	}

	Net::DownloadStatus
	downloadStatus (const PrimaryServices & primaryServices, int ticket) noexcept
	{
		return primaryServices.netManager().downloadStatus(ticket);
	}

	void
	enqueueTask (const PrimaryServices & primaryServices, std::function< void () > task) noexcept
	{
		primaryServices.threadPool()->enqueue(std::move(task));
	}

	int
	startDownload (PrimaryServices & primaryServices, LoadingRequest & request) noexcept
	{
		if ( !request.isDownloadable() )
		{
			return DownloadNotStarted;
		}

		/* NOTE: Check the cache system before downloading. */
		const auto cacheFile = request.cacheFilepath(primaryServices.fileSystem());

		if ( Base::IO::fileExists(cacheFile) )
		{
			request.setDownloadProcessed(primaryServices.fileSystem(), true);

			return DownloadCacheHit;
		}

		const auto ticket = primaryServices.netManager().download(request.url(), cacheFile, false);

		request.setDownloadTicket(ticket);

		return ticket;
	}

	void
	markDownloadProcessed (PrimaryServices & primaryServices, LoadingRequest & request, bool success) noexcept
	{
		request.setDownloadProcessed(primaryServices.fileSystem(), success);
	}
}
