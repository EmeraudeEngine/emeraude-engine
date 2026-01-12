/*
 * src/Libs/FastJSON.cpp
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

#include "FastJSON.hpp"

/* STL inclusions. */
#include <fstream>
#include <iostream>
#include <string>

namespace EmEn::Libs::FastJSON
{
	std::optional< Json::Value >
	getRootFromFile (const std::filesystem::path & filepath, int stackLimit, bool quiet)
	{
		Json::CharReaderBuilder builder{};
		builder["collectComments"] = false;
		builder["allowComments"] = false;
		builder["allowTrailingCommas"] = false;
		builder["strictRoot"] = true;
		builder["allowDroppedNullPlaceholders"] = false;
		builder["allowNumericKeys"] = false;
		builder["allowSingleQuotes"] = false;
		builder["stackLimit"] = stackLimit;
		builder["failIfExtra"] = true;
		builder["rejectDupKeys"] = true;
		builder["allowSpecialFloats"] = true;
		builder["skipBom"] = true;

		std::ifstream json{filepath, std::ifstream::binary};

		if ( !json.is_open() )
		{
			if ( !quiet )
			{
				std::cerr << "[FastJSON-DEBUG] Unable to open the file " << filepath << " !" "\n";
			}

			return std::nullopt;
		}

		Json::Value root;
		std::string errors;

		if ( !parseFromStream(builder, json, &root, &errors) )
		{
			if ( !quiet )
			{
				std::cerr << "[FastJSON-DEBUG] Unable to parse JSON file " << filepath << " ! Errors :" "\n" << errors << "\n";
			}

			return std::nullopt;
		}

		return root;
	}

	std::optional< Json::Value >
	getRootFromString (const std::string & json, int stackLimit, bool quiet)
	{
		Json::CharReaderBuilder builder{};
		builder["collectComments"] = false;
		builder["allowComments"] = false;
		builder["allowTrailingCommas"] = false;
		builder["strictRoot"] = true;
		builder["allowDroppedNullPlaceholders"] = false;
		builder["allowNumericKeys"] = false;
		builder["allowSingleQuotes"] = false;
		builder["stackLimit"] = stackLimit;
		builder["failIfExtra"] = true;
		builder["rejectDupKeys"] = true;
		builder["allowSpecialFloats"] = true;
		builder["skipBom"] = true;

		const std::unique_ptr< Json::CharReader > reader{builder.newCharReader()};

		Json::Value root;
		std::string errors;

		if ( !reader->parse(json.data(), json.data() + json.size(), &root, &errors) )
		{
			if ( !quiet )
			{
				std::cerr << "[FastJSON-DEBUG] Unable to parse JSON from a string ! Errors :" "\n" << errors << "\n";
			}

			return std::nullopt;
		}

		return root;
	}

	std::string
	stringify (const Json::Value & root)
	{
		Json::StreamWriterBuilder builder{};
		builder["commentStyle"] = "None";
		builder["indentation"] = "";
		builder["enableYAMLCompatibility"] = false;
		builder["dropNullPlaceholders"] = false;
		builder["useSpecialFloats"] = false;
		builder["precision"] = 5;
		builder["precisionType"] = "significant";
		builder["emitUTF8"] = true;

		return Json::writeString(builder, root);
	}
}
