/*
 * src/Settings.cpp
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

/* STL inclusions. */
#include <ctime>
#include <fstream>

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Libs/IO/IO.hpp"
#include "Libs/String.hpp"
#include "Arguments.hpp"
#include "FileSystem.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	using namespace Libs;

	bool
	Settings::onInitialize () noexcept
	{
		/* NOTE: In read-only, settings service is a copy from another process. */
		if ( !m_childProcess )
		{
			m_showInformation = m_arguments.isSwitchPresent("--verbose");
		}

		if ( const auto filepath = m_arguments.get("--settings-filepath") )
		{
			m_filepath = filepath.value();
		}
		else if ( const auto filename = m_arguments.get("--settings-filename") )
		{
			m_filepath = m_fileSystem.configDirectory(filename.value());
		}
		else
		{
			m_filepath = m_fileSystem.configDirectory(Filename);
		}

		if ( m_filepath.empty() )
		{
			TraceWarning{ClassId} << "The settings file path variable is not set !";

			return false;
		}

		if ( m_showInformation )
		{
			TraceInfo{ClassId} << "Loading settings from file " << m_filepath << " ...";
		}

		/* Checks the file presence, if not, it will be created and uses the default engine values. */
		if ( !IO::fileExists(m_filepath) )
		{
			if ( !m_childProcess )
			{
				TraceWarning{ClassId} <<
					"Settings file " << m_filepath << " doesn't exist." "\n"
					"The file will be written at the application successful exit.";

				this->saveAtExit(true);
			}

			return true;
		}

		/* Reading the file ... */
		{
			const std::unique_lock< std::shared_mutex > lock{m_storeAccess};

			if ( !this->readFile(m_filepath) )
			{
				TraceError{ClassId} << "Unable to read settings file from " << m_filepath << "'path !";

				this->saveAtExit(false);

				return false;
			}
		}

		if ( m_arguments.isSwitchPresent("--disable-settings-autosave") )
		{
			this->saveAtExit(false);
		}

		if ( m_showInformation )
		{
			TraceInfo{ClassId} << *this;
		}

		return true;
	}

	bool
	Settings::onTerminate () noexcept
	{
		if ( this->isSaveAtExitEnabled() && !m_arguments.isSwitchPresent("--reset-settings") )
		{
			if ( m_filepath.empty() )
			{
				TraceError{ClassId} << "File path is empty. Unable to save this settings file !";

				return false;
			}

			const std::shared_lock< std::shared_mutex > lock{m_storeAccess};

			if ( !this->writeFile(m_filepath) )
			{
				TraceError{ClassId} << "Unable to write settings file to " << m_filepath << " !";

				return false;
			}

			TraceSuccess{ClassId} << "Settings file saved to " << m_filepath << " !";
		}

		return true;
	}

	std::pair< std::string_view, std::string_view >
	Settings::parseAccessKey (std::string_view settingPath) noexcept
	{
		const auto lastSlash = settingPath.find_last_of('/');

		if ( lastSlash == std::string::npos )
		{
			return {"", settingPath};
		}

		return {settingPath.substr(0, lastSlash), settingPath.substr(lastSlash + 1)};
	}

	bool
	Settings::readLevel (const Json::Value & data, const std::string & key) noexcept
	{
		const auto toSettingValue = [] (const Json::Value & item) -> std::optional< SettingValue > {
			if ( item.isBool() )
			{
				return item.asBool();
			}

			if ( item.isInt() )
			{
				return item.asInt();
			}

			if ( item.isUInt() )
			{
				return item.asUInt();
			}

			if ( item.isInt64() )
			{
				return item.asInt64();
			}

			if ( item.isUInt64() )
			{
				return item.asUInt64();
			}

			if ( item.isDouble() )
			{
				return item.asDouble();
			}

			if ( item.isString() )
			{
				return item.asString();
			}

			return std::nullopt;
		};

		for ( const auto & name : data.getMemberNames() )
		{
			if ( const auto & items = data[name]; items.isObject() )
			{
				std::stringstream keyStream;

				if ( key.empty() )
				{
					keyStream << name;
				}
				else
				{
					keyStream << key << '/' << name;
				}

				if ( !this->readLevel(items, keyStream.str()) )
				{
					return false;
				}
			}
			else if ( items.isArray() )
			{
				for ( const auto & item : items )
				{
					if ( auto val = toSettingValue(item) )
					{
						m_stores[key].setVariableInArray(name, *val);
					}
				}
			}
			else
			{
				if ( auto val = toSettingValue(items) )
				{
					m_stores[key].setVariable(name, *val);
				}
			}
		}

		return true;
	}

	bool
	Settings::readFile (const std::filesystem::path & filepath) noexcept
	{
		const auto root = FastJSON::getRootFromFile(filepath);

		if ( !root )
		{
			TraceError{ClassId} << "Unable to parse the settings file " << filepath << " !" "\n";

			return false;
		}

		return this->readLevel(root.value(), "");
	}

	Json::Value
	Settings::settingValueToJson (const SettingValue & value) noexcept
	{
		return std::visit([] < typename value_t >(const value_t  & v) -> Json::Value {
			using T = std::decay_t< value_t  >;

			if constexpr ( std::is_same_v< T, bool > )
			{
				return v;
			}
			else if constexpr ( std::is_same_v< T, std::string > )
			{
				return v;
			}
			else if constexpr ( std::is_same_v< T, int16_t > )
			{
				return static_cast< Json::Int >(v);
			}
			else if constexpr ( std::is_same_v< T, uint16_t > )
			{
				return static_cast< Json::UInt >(v);
			}
			else if constexpr ( std::is_same_v< T, int32_t > )
			{
				return v;
			}
			else if constexpr ( std::is_same_v< T, uint32_t > )
			{
				return v;
			}
			else if constexpr ( std::is_same_v< T, int64_t > )
			{
				return static_cast< Json::Int64 >(v);
			}
			else if constexpr ( std::is_same_v< T, uint64_t > )
			{
				return static_cast< Json::UInt64 >(v);
			}
			else
			{
				return v;
			}
		}, value);
	}

	bool
	Settings::writeFile (const std::filesystem::path & filepath) const noexcept
	{
		const auto getLevel = [] (Json::Value & root, const std::string & key) -> Json::Value & {
			if ( key.empty() )
			{
				return root;
			}

			Json::Value * current = &root;

			for ( const auto & section : String::explode(key, '/', false) )
			{
				current = &(*current)[section];
			}

			return *current;
		};

		Json::Value root;

		/* 1. JSON File header. */
		root[VersionKey] = VersionString;

		{
			const auto timestamp = time(nullptr);
			const auto * now = localtime(&timestamp);

			std::stringstream text;
			text << now->tm_year + 1900 << '-' << now->tm_mon + 1 << '-' << now->tm_mday;

			root[DateKey] = text.str();
		}

		/* 2. JSON File body. */
		for ( const auto & [key, store] : m_stores )
		{
			auto & data = getLevel(root, std::string{key});

			/* Write variables at this level. */
			for ( const auto & [name, value] : store.variables() )
			{
				data[name] = settingValueToJson(value);
			}

			/* Write an array at this level. */
			for ( const auto & [name, values] : store.arrays() )
			{
				data[name] = Json::arrayValue;

				for ( const auto & value : values )
				{
					data[name].append(settingValueToJson(value));
				}
			}
		}

		/* 3. File writing. */
		{
			Json::StreamWriterBuilder builder{};
			builder["commentStyle"] = "None";
			builder["indentation"] =  "\t";
			builder["enableYAMLCompatibility"] = false;
			builder["dropNullPlaceholders"] = true;
			builder["useSpecialFloats"] = true;
			builder["precision"] = 8;
			builder["precisionType"] = "significant";
			builder["emitUTF8"] = true;

			const auto jsonString = Json::writeString(builder, root);

			if ( jsonString.empty() )
			{
				TraceError{ClassId} << "Unable to get JSON string from writer ! (JsonCpp library error)";

				return false;
			}

			return IO::filePutContents(filepath, jsonString);
		}
	}

	bool
	Settings::variableExists (std::string_view settingPath) const noexcept
	{
		const std::shared_lock< std::shared_mutex > lock{m_storeAccess};

		const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

		const auto storeIt = m_stores.find(key);

		if ( storeIt == m_stores.end() )
		{
			return false;
		}

		return storeIt->second.variableExists(variableName);
	}

	bool
	Settings::arrayExists (std::string_view settingPath) const noexcept
	{
		const std::shared_lock< std::shared_mutex > lock{m_storeAccess};

		const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

		const auto storeIt = m_stores.find(key);

		if ( storeIt == m_stores.end() )
		{
			return false;
		}

		return storeIt->second.arrayExists(variableName);
	}

	std::optional< SettingValue >
	Settings::getVariable (std::string_view settingPath) const noexcept
	{
		const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

		const auto storeIt = m_stores.find(key);

		if ( storeIt == m_stores.end() )
		{
			/* NOTE: The store do not exist. */
			return std::nullopt;
		}

		const auto * value = storeIt->second.getValuePointer(variableName);

		if ( value == nullptr )
		{
			/* NOTE: The variable do not exist. */
			return std::nullopt;
		}

		return *value;
	}

	bool
	Settings::isArrayEmpty (std::string_view settingPath) const noexcept
	{
		const std::shared_lock< std::shared_mutex > lock{m_storeAccess};

		const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

		const auto storeIt = m_stores.find(key);

		if ( storeIt == m_stores.end() )
		{
			/* NOTE: The store do not exist. */
			return true;
		}

		const auto * array = storeIt->second.getArrayPointer(variableName);

		if ( array == nullptr )
		{
			return true;
		}

		return array->empty();
	}

	bool
	Settings::save () const noexcept
	{
		const std::shared_lock< std::shared_mutex > lock{m_storeAccess};

		if ( m_filepath.empty() )
		{
			Tracer::warning(ClassId, "No filepath was used to read config !");

			return false;
		}

		return this->writeFile(m_filepath);
	}

	std::string
	Settings::toJsonString () const noexcept
	{
		const std::shared_lock< std::shared_mutex > lock{m_storeAccess};

		const auto getLevel = [] (Json::Value & root, const std::string & key) -> Json::Value & {
			if ( key.empty() )
			{
				return root;
			}

			Json::Value * current = &root;

			for ( const auto & section : String::explode(key, '/', false) )
			{
				current = &(*current)[section];
			}

			return *current;
		};

		Json::Value root;

		for ( const auto & [key, store] : m_stores )
		{
			auto & data = getLevel(root, std::string{key});

			for ( const auto & [name, value] : store.variables() )
			{
				data[name] = settingValueToJson(value);
			}

			for ( const auto & [name, values] : store.arrays() )
			{
				data[name] = Json::arrayValue;

				for ( const auto & value : values )
				{
					data[name].append(settingValueToJson(value));
				}
			}
		}

		Json::StreamWriterBuilder builder{};
		builder["commentStyle"] = "None";
		builder["indentation"] = "";
		builder["enableYAMLCompatibility"] = false;
		builder["dropNullPlaceholders"] = true;
		builder["useSpecialFloats"] = true;
		builder["precision"] = 8;
		builder["precisionType"] = "significant";
		builder["emitUTF8"] = true;

		return Json::writeString(builder, root);
	}

	std::ostream &
	operator<< (std::ostream & out, const Settings & obj)
	{
		const std::shared_lock< std::shared_mutex > lock{obj.m_storeAccess};

		auto printValue = [] (const SettingValue & value) -> std::string {
			return std::visit([] < typename value_t >(const value_t & v) -> std::string {
				using T = std::decay_t< value_t  >;

				if constexpr ( std::is_same_v< T, std::string > )
				{
					return v;
				}
				else if constexpr ( std::is_same_v< T, bool > )
				{
					return v ? "On" : "Off";
				}
				else
				{
					return std::to_string(v);
				}
			}, value);
		};

		out << "Settings (" << obj.m_filepath << ") :" "\n";

		/* Crawling inside all stores. */
		for ( const auto & [key, store] : obj.m_stores )
		{
			if ( key.empty() )
			{
				out << "*(ROOT)*" "\n";
			}
			else
			{
				out << "[" << key << "]" "\n";
			}

			/* Print every variable */
			for ( const auto & [name, value] : store.variables() )
			{
				out << "  " << name << " = " << printValue(value) << '\n';
			}

			/* Print every array */
			for ( const auto & [name, values] : store.arrays() )
			{
				std::string output;

				for ( const auto & value : values )
				{
					output += printValue(value) + ", ";
				}

				out << "  " << name << " = [" << output << "]" "\n";
			}
		}

		return out;
	}
}
