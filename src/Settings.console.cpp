/*
 * src/Settings.console.cpp
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

#include "Settings.hpp"

namespace EmEn
{
	void
	Settings::onRegisterToConsole () noexcept
	{
		this->bindCommand("getAll,print", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			outputs.emplace_back(Severity::Info, std::stringstream{} << *this);

			return true;
		}, "Prints all settings.");

		this->bindCommand("getJson", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			outputs.emplace_back(Severity::Info, this->toJsonString());

			return true;
		}, "Returns all settings as a JSON string.");

		this->bindCommand("set", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 2 )
			{
				outputs.emplace_back(Severity::Error, "Usage: set(key, value)");

				return false;
			}

			const auto key = arguments[0].asString();

			switch ( arguments[1].type() )
			{
				case Console::ArgumentType::Boolean :
					this->set(key, arguments[1].asBoolean());
					break;

				case Console::ArgumentType::Integer :
					this->set(key, arguments[1].asInteger());
					break;

				case Console::ArgumentType::Float :
					this->set(key, static_cast< double >(arguments[1].asFloat()));
					break;

				case Console::ArgumentType::String :
					this->set(key, arguments[1].asString());
					break;

				default :
					outputs.emplace_back(Severity::Error, "Unsupported value type !");

					return false;
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Setting '" << key << "' updated.");

			return true;
		}, "Sets a setting value. Usage: set(key, value)");

		this->bindCommand("save", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( this->save() )
			{
				outputs.emplace_back(Severity::Success, "Settings saved.");
			}
			else
			{
				outputs.emplace_back(Severity::Error, "Failed to save settings !");
			}

			return true;
		}, "Forces settings save to disk.");
	}
}
