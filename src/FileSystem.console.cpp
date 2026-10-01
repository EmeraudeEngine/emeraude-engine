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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "FileSystem.hpp"

/* Local inclusions. */
#include "FastJSON.hpp"
#include "IO/IO.hpp"

namespace EmEn
{
	void
	FileSystem::onRegisterToConsole () noexcept
	{
		this->bindCommand("print", "Prints all filesystem paths as text.", [this] () {
			std::stringstream text;
			text << *this;

			return Console::CommandResult::info(text.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("getJson", "Returns all filesystem paths as JSON.", [this] () {
			Json::Value root{Json::objectValue};
			root["binaryName"] = m_binaryName;
			/* NOTE: UTF-8 (IO::toU8String), not path::string(): on Windows that is the ANSI code page, and a non-ASCII
			 * directory produced invalid UTF-8 in the JSON (a client refused the line). */
			root["binaryDirectory"] = Base::IO::toU8String(m_binaryDirectory);
			root["userDirectory"] = Base::IO::toU8String(m_userDirectory);
			root["userDataDirectory"] = Base::IO::toU8String(m_userDataDirectory);
			root["configDirectory"] = Base::IO::toU8String(m_configDirectory);
			root["cacheDirectory"] = Base::IO::toU8String(m_cacheDirectory);

			Json::Value dataDirectories{Json::arrayValue};

			for ( const auto & dataDirectory : m_dataDirectories )
			{
				dataDirectories.append(Base::IO::toU8String(dataDirectory));
			}

			root["dataDirectories"] = std::move(dataDirectories);

			return Console::CommandResult::json(Base::FastJSON::stringify(root));
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("get", "Returns one filesystem path by its name.",
			{
				{"name", "The path name: binaryName, binaryDirectory, userDirectory, userDataDirectory, configDirectory or cacheDirectory."}
			},
			[this] (const std::string & name) {
				if ( name == "binaryName" )
				{
					return Console::CommandResult::info(m_binaryName);
				}

				if ( name == "binaryDirectory" )
				{
					return Console::CommandResult::info(Base::IO::toU8String(m_binaryDirectory));
				}

				if ( name == "userDirectory" )
				{
					return Console::CommandResult::info(Base::IO::toU8String(m_userDirectory));
				}

				if ( name == "userDataDirectory" )
				{
					return Console::CommandResult::info(Base::IO::toU8String(m_userDataDirectory));
				}

				if ( name == "configDirectory" )
				{
					return Console::CommandResult::info(Base::IO::toU8String(m_configDirectory));
				}

				if ( name == "cacheDirectory" )
				{
					return Console::CommandResult::info(Base::IO::toU8String(m_cacheDirectory));
				}

				return Console::CommandResult::error("Unknown path name '" + name + "'.");
			}, Console::CommandHint::ReadOnly);
	}
}
