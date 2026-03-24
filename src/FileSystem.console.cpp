/*
 * src/FileSystem.console.cpp
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

#include "FileSystem.hpp"

namespace EmEn
{
	void
	FileSystem::onRegisterToConsole () noexcept
	{
		this->bindCommand("print", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			outputs.emplace_back(Severity::Info, std::stringstream{} << *this);

			return true;
		}, "Prints all filesystem paths as text.");

		this->bindCommand("getJson", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			std::stringstream json;
			json << "{";

			json << "\"binaryName\":\"" << m_binaryName << "\",";
			json << "\"binaryDirectory\":\"" << m_binaryDirectory.string() << "\",";
			json << "\"userDirectory\":\"" << m_userDirectory.string() << "\",";
			json << "\"userDataDirectory\":\"" << m_userDataDirectory.string() << "\",";
			json << "\"configDirectory\":\"" << m_configDirectory.string() << "\",";
			json << "\"cacheDirectory\":\"" << m_cacheDirectory.string() << "\",";

			json << "\"dataDirectories\":[";

			for ( size_t i = 0; i < m_dataDirectories.size(); i++ )
			{
				if ( i > 0 )
				{
					json << ",";
				}

				json << "\"" << m_dataDirectories[i].string() << "\"";
			}

			json << "]";

			json << "}";

			outputs.emplace_back(Severity::Info, json.str());

			return true;
		}, "Returns all filesystem paths as JSON.");

		this->bindCommand("get", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: get(name) — names: binaryName, binaryDirectory, userDirectory, userDataDirectory, configDirectory, cacheDirectory");

				return false;
			}

			const auto name = arguments[0].asString();

			if ( name == "binaryName" )
			{
				outputs.emplace_back(Severity::Info, m_binaryName);
			}
			else if ( name == "binaryDirectory" )
			{
				outputs.emplace_back(Severity::Info, m_binaryDirectory.string());
			}
			else if ( name == "userDirectory" )
			{
				outputs.emplace_back(Severity::Info, m_userDirectory.string());
			}
			else if ( name == "userDataDirectory" )
			{
				outputs.emplace_back(Severity::Info, m_userDataDirectory.string());
			}
			else if ( name == "configDirectory" )
			{
				outputs.emplace_back(Severity::Info, m_configDirectory.string());
			}
			else if ( name == "cacheDirectory" )
			{
				outputs.emplace_back(Severity::Info, m_cacheDirectory.string());
			}
			else
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Unknown path name '" << name << "'.");

				return false;
			}

			return true;
		}, "Returns a specific path. Usage: get(name)");
	}
}
