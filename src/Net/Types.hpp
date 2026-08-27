/*
 * src/Net/Types.hpp
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
#include <cstdint>

namespace EmEn::Net
{
	/** @brief Lifecycle of one download request held by Net::Manager. */
	enum class DownloadStatus : uint8_t
	{
		Pending,		/* Accepted, not yet picked by a worker. */
		Transferring,	/* A worker is fetching it. */
		Error,			/* Terminal: refused, transport or filesystem failure — the file is absent. */
		Done			/* Terminal: the file sits at DownloadItem::filepath(). */
	};

	/**
	 * @brief Returns the textual name of a download status.
	 * @param status The status.
	 * @return const char *
	 */
	[[nodiscard]]
	constexpr
	const char *
	to_cstring (DownloadStatus status) noexcept
	{
		switch ( status )
		{
			case DownloadStatus::Pending :
				return "Pending";

			case DownloadStatus::Transferring :
				return "Transferring";

			case DownloadStatus::Error :
				return "Error";

			case DownloadStatus::Done :
				return "Done";
		}

		return "Unknown";
	}
}
