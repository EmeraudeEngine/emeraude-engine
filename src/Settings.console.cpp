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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Settings.hpp"

namespace EmEn
{
	void
	Settings::onRegisterToConsole () noexcept
	{
		this->bindCommand("getAll,print", "Prints all settings.", [this] () {
			std::stringstream text;
			text << *this;

			return Console::CommandResult::info(text.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("getJson", "Returns all settings as a JSON document.", [this] () {
			return Console::CommandResult::json(this->toJsonString());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("set", "Sets a setting value for this session (save() writes it to disk). Most keys are read once at launch.",
			{
				{"key", "The setting path, e.g. 'Core/Graphics/PostProcessing/LightingLane'."},
				{"value", "The new value; its type (boolean, integer, float, string) is the one stored."}
			},
			[this] (const std::string & key, const Console::Argument & value) {
				switch ( value.type() )
				{
					case Console::ArgumentType::Boolean :
						this->set(key, value.asBoolean());
						break;

					case Console::ArgumentType::Integer :
						this->set(key, value.asInteger());
						break;

					case Console::ArgumentType::Float :
						this->set(key, static_cast< double >(value.asFloat()));
						break;

					case Console::ArgumentType::String :
						this->set(key, value.asString());
						break;

					case Console::ArgumentType::Undefined :
						return Console::CommandResult::error("Unsupported value type !");
				}

				return Console::CommandResult::success("Setting '" + key + "' updated.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("save", "Forces settings save to disk.", [this] () {
			if ( !this->save() )
			{
				return Console::CommandResult::error("Failed to save settings !");
			}

			return Console::CommandResult::success("Settings saved.");
		}, Console::CommandHint::Idempotent);
	}
}
