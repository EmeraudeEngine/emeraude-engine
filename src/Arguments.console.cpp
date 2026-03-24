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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Arguments.hpp"

namespace EmEn
{
	void
	Arguments::onRegisterToConsole () noexcept
	{
		this->bindCommand("print", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			outputs.emplace_back(Severity::Info, std::stringstream{} << *this);

			return true;
		}, "Prints all launch arguments as text.");

		this->bindCommand("getJson", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			std::stringstream json;
			json << "{";

			json << "\"binaryFilepath\":\"" << m_binaryFilepath.string() << "\",";
			json << "\"isChildProcess\":" << (m_childProcess ? "true" : "false") << ",";

			/* Raw arguments. */
			json << "\"rawArguments\":[";

			for ( size_t i = 0; i < m_rawArguments.size(); i++ )
			{
				if ( i > 0 )
				{
					json << ",";
				}

				json << "\"" << m_rawArguments[i] << "\"";
			}

			json << "],";

			/* Switches. */
			json << "\"switches\":[";

			bool first = true;

			this->forEachSwitch([&json, &first] (const std::string & name) {
				if ( !first )
				{
					json << ",";
				}

				first = false;

				json << "\"" << name << "\"";

				return false;
			});

			json << "],";

			/* Named arguments. */
			json << "\"arguments\":{";

			first = true;

			this->forEachArgument([&json, &first] (const std::string & name, const std::string & value) {
				if ( !first )
				{
					json << ",";
				}

				first = false;

				json << "\"" << name << "\":\"" << value << "\"";

				return false;
			});

			json << "}";

			json << "}";

			outputs.emplace_back(Severity::Info, json.str());

			return true;
		}, "Returns all arguments as JSON.");

		this->bindCommand("get", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: get(index)");

				return false;
			}

			const auto index = static_cast< size_t >(arguments[0].asInteger());

			if ( index >= m_rawArguments.size() )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Index " << index << " out of range (0-" << (m_rawArguments.size() - 1) << ").");

				return false;
			}

			outputs.emplace_back(Severity::Info, m_rawArguments[index]);

			return true;
		}, "Returns a specific argument by index. Usage: get(index)");
	}
}
