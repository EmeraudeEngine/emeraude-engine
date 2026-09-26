/*
 * src/Net/Manager.console.cpp
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


#include "Manager.hpp"

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include "FastJSON.hpp"

namespace EmEn::Net
{
	void
	Manager::onRegisterToConsole () noexcept
	{
		this->bindCommand("download", "Downloads a https URL into the cache and returns a ticket.",
			{
				{"url", "The https:// URL of the file to download."}
			},
			[this] (const std::string & url) {
				const auto ticket = this->download(Base::Network::URI{url});

				if ( ticket == InvalidTicket )
				{
					return Console::CommandResult::error("Download refused (disabled, not https, or no thread pool). See the log.");
				}

				std::stringstream message;
				message << "Ticket #" << ticket << " (" << to_cstring(this->downloadStatus(ticket)) << "). Poll with status(" << ticket << ").";

				return Console::CommandResult::success(message.str());
			}, Console::CommandHint::Idempotent);

		this->bindCommand("status", "Returns a ticket status as JSON (status, filepath when Done, bytesReceived/bytesTotal, reason + httpStatus when Error, transfers remaining).",
			{
				{"ticket", "The ticket number returned by download()."}
			},
			[this] (int32_t ticket) {
				const auto status = this->downloadStatus(ticket);

				Json::Value json{Json::objectValue};
				json["ticket"] = ticket;
				json["status"] = to_cstring(status);

				if ( status == DownloadStatus::Done )
				{
					json["filepath"] = this->downloadedFilepath(ticket).string();
				}

				const auto [received, total] = this->downloadProgress(ticket);
				json["bytesReceived"] = static_cast< Json::UInt64 >(received);
				json["bytesTotal"] = static_cast< Json::UInt64 >(total);

				if ( status == DownloadStatus::Error )
				{
					const auto [outcome, statusCode] = this->downloadFailure(ticket);

					json["reason"] = Base::Network::to_cstring(outcome);

					if ( statusCode > 0 )
					{
						json["httpStatus"] = static_cast< Json::UInt >(statusCode);
					}
				}

				json["remaining"] = static_cast< Json::UInt64 >(this->fileRemainingCount());

				return Console::CommandResult::json(Base::FastJSON::stringify(json));
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("listCache", "Lists the download cache as JSON (url, filepath, bytes per entry).", [this] () {
			Json::Value json{Json::arrayValue};

			for ( const auto & [url, filepath, bytes] : this->cachedFiles() )
			{
				Json::Value entry{Json::objectValue};
				entry["url"] = url;
				entry["filepath"] = filepath.string();
				entry["bytes"] = static_cast< Json::UInt64 >(bytes);

				json.append(std::move(entry));
			}

			return Console::CommandResult::json(Base::FastJSON::stringify(json));
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("clearCache", "Removes every downloaded file from the cache and rewrites its index.", [this] () {
			if ( !this->clearCache() )
			{
				return Console::CommandResult::error("Some cached files could not be removed, see the log.");
			}

			return Console::CommandResult::success("Download cache cleared.");
		}, Console::CommandHint::Destructive | Console::CommandHint::Idempotent);

		this->bindCommand("isEnabled", "Returns whether downloads are enabled (setting, cache directory and trust store all OK), as JSON.", [this] () {
			/* A bare "false" says nothing: the setting, the cache directory and the trust store can
			 * each disable downloads, and only the log said which. */
			Json::Value json{Json::objectValue};
			json["enabled"] = this->isDownloadEnabled();

			if ( !this->isDownloadEnabled() )
			{
				json["reason"] = this->disabledReason();
			}

			json["cacheBudgetBytes"] = static_cast< Json::UInt64 >(this->cacheBudget());

			return Console::CommandResult::json(Base::FastJSON::stringify(json));
		}, Console::CommandHint::ReadOnly);
	}
}
