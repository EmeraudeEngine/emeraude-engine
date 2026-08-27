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

namespace EmEn::Net
{
	void
	Manager::onRegisterToConsole () noexcept
	{
		this->bindCommand("download", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: download(https://host/path/file.ext)");

				return false;
			}

			const auto ticket = this->download(Base::Network::URI{arguments[0].asString()});

			if ( ticket == InvalidTicket )
			{
				outputs.emplace_back(Severity::Error, "Download refused (disabled, not https, or no thread pool). See the log.");

				return false;
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Ticket #" << ticket << " (" << to_cstring(this->downloadStatus(ticket)) << "). Poll with status(" << ticket << ").");

			return true;
		}, "Downloads a https URL into the cache and returns a ticket. Usage: download(url)");

		this->bindCommand("status", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: status(ticket)");

				return false;
			}

			const auto ticket = arguments[0].asInteger();
			const auto status = this->downloadStatus(ticket);

			std::stringstream json;
			json << "{\"ticket\":" << ticket << ",\"status\":\"" << to_cstring(status) << "\"";

			if ( status == DownloadStatus::Done )
			{
				json << ",\"filepath\":\"" << this->downloadedFilepath(ticket).string() << "\"";
			}

			const auto [received, total] = this->downloadProgress(ticket);
			json << ",\"bytesReceived\":" << received << ",\"bytesTotal\":" << total;

			json << ",\"remaining\":" << this->fileRemainingCount() << "}";

			outputs.emplace_back(Severity::Info, json.str());

			return true;
		}, "Returns a ticket status as JSON (status, filepath when Done, bytesReceived, bytesTotal — 0 when unknown —, transfers remaining). Usage: status(ticket)");

		this->bindCommand("listCache", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			std::stringstream json;
			json << "[";

			bool first = true;

			for ( const auto & [url, filepath, bytes] : this->cachedFiles() )
			{
				if ( !first )
				{
					json << ",";
				}

				json << "{\"url\":\"" << url << "\",\"filepath\":\"" << filepath.string() << "\",\"bytes\":" << bytes << "}";

				first = false;
			}

			json << "]";

			outputs.emplace_back(Severity::Info, json.str());

			return true;
		}, "Lists the download cache as JSON (url, filepath, bytes per entry).");

		this->bindCommand("clearCache", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( this->clearCache() )
			{
				outputs.emplace_back(Severity::Success, "Download cache cleared.");

				return true;
			}

			outputs.emplace_back(Severity::Error, "Some cached files could not be removed, see the log.");

			return false;
		}, "Removes every downloaded file from the cache and rewrites its index.");

		this->bindCommand("isEnabled", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			outputs.emplace_back(Severity::Info, this->isDownloadEnabled() ? "true" : "false");

			return true;
		}, "Returns whether downloads are enabled (setting, cache directory and trust store all OK).");
	}
}
