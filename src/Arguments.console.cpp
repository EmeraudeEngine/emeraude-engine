/*
 * src/Arguments.console.cpp
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

#include "Arguments.hpp"

/* Local inclusions. */
#include "FastJSON.hpp"

namespace EmEn
{
	void
	Arguments::onRegisterToConsole () noexcept
	{
		this->bindCommand("print", "Prints all launch arguments as text.", [this] () {
			std::stringstream text;
			text << *this;

			return Console::CommandResult::info(text.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("getJson", "Returns all arguments as JSON.", [this] () {
			Json::Value root{Json::objectValue};
			root["binaryFilepath"] = m_binaryFilepath.string();
			root["isChildProcess"] = m_childProcess;

			Json::Value rawArguments{Json::arrayValue};

			for ( const auto & rawArgument : m_rawArguments )
			{
				rawArguments.append(rawArgument);
			}

			root["rawArguments"] = std::move(rawArguments);

			Json::Value switches{Json::arrayValue};

			this->forEachSwitch([&switches] (const std::string & name) {
				switches.append(name);

				return false;
			});

			root["switches"] = std::move(switches);

			Json::Value namedArguments{Json::objectValue};

			this->forEachArgument([&namedArguments] (const std::string & name, const std::string & value) {
				namedArguments[name] = value;

				return false;
			});

			root["arguments"] = std::move(namedArguments);

			return Console::CommandResult::json(Base::FastJSON::stringify(root));
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("get", "Returns one raw launch argument by its position.",
			{
				{"index", "Zero-based position in the raw argument list (0 is the binary path)."}
			},
			[this] (int32_t index) {
				if ( index < 0 || static_cast< size_t >(index) >= m_rawArguments.size() )
				{
					std::stringstream message;
					message << "Index " << index << " out of range (0-" << (m_rawArguments.size() - 1) << ").";

					return Console::CommandResult::error(message.str());
				}

				return Console::CommandResult::info(m_rawArguments[static_cast< size_t >(index)]);
			}, Console::CommandHint::ReadOnly);
	}
}
