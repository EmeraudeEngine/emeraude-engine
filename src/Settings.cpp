/*
 * src/Settings.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include <exception>
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

		if ( const auto argument = m_arguments.get("--settings-filepath") )
		{
			m_filepath = argument.value();
		}
		else
		{
			m_filepath = m_fileSystem.configDirectory(Settings::Filename);
		}

		if ( m_filepath.empty() )
		{
			TraceWarning{ClassId} << "The settings file path variable is not set !";

			return false;
		}

		if ( m_showInformation )
		{
			TraceInfo{ClassId} << "Loading settings from file '" << m_filepath.string() << "' ...";
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
				TraceError{ClassId} << "Unable to read settings file from '" << m_filepath.string() << "' path !";

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

		m_serviceInitialized = true;

		return true;
	}

	bool
	Settings::onTerminate () noexcept
	{
		m_serviceInitialized = false;

		if ( this->isSaveAtExitEnabled() )
		{
			if ( m_filepath.empty() )
			{
				TraceError{ClassId} << "File path is empty. Unable to save this settings file !";

				return false;
			}

			const std::shared_lock< std::shared_mutex > lock{m_storeAccess};

			if ( !this->writeFile(m_filepath) )
			{
				TraceError{ClassId} << "Unable to write settings file to '" << m_filepath << "' !";

				return false;
			}

			TraceSuccess{ClassId} << "Settings file saved to '" << m_filepath << "' !";
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
		const auto toAny = [] (const Json::Value & item) -> std::any {
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

			return {};
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
					m_stores[key].setVariableInArray(name, toAny(item));
				}
			}
			else
			{
				m_stores[key].setVariable(name, toAny(items));
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

	bool
	Settings::writeFile (const std::filesystem::path & filepath) const noexcept
	{
		const auto toJson = [] (const std::any & item) -> Json::Value {
			if ( item.type() == typeid(bool) )
			{
				return std::any_cast< bool >(item);
			}

			if ( item.type() == typeid(int32_t) )
			{
				return std::any_cast< int32_t >(item);
			}

			if ( item.type() == typeid(uint32_t) )
			{
				return std::any_cast< uint32_t >(item);
			}

			if ( item.type() == typeid(int64_t) )
			{
				return std::any_cast< int64_t >(item);
			}

			if ( item.type() == typeid(uint64_t) )
			{
				return std::any_cast< uint64_t >(item);
			}

			if ( item.type() == typeid(double) )
			{
				return std::any_cast< double >(item);
			}

			if ( item.type() == typeid(std::string) )
			{
				return std::any_cast< std::string >(item);
			}

			return Json::stringValue;
		};

		const auto getLevel = [] (Json::Value & root, const std::string & key) -> Json::Value & {
			Json::Value * current = &root;

			if ( const auto sections = String::explode(key, '/', false); !sections.empty() )
			{
				for ( const auto & section : sections )
				{
					current = &(*current)[section];
				}
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
				data[name] = toJson(value);
			}

			/* Write an array at this level. */
			for ( const auto & [name, values] : store.arrays() )
			{
				data[name] = Json::arrayValue;

				for ( const auto & value : values )
				{
					data[name].append(toJson(value));
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

	std::any
	Settings::getVariable (std::string_view settingPath) const noexcept
	{
		const auto & [key, variableName] = Settings::parseAccessKey(settingPath);

		const auto storeIt = m_stores.find(key);

		if ( storeIt == m_stores.end() )
		{
			/* NOTE: The store do not exist. */
			return {};
		}

		const auto * value = storeIt->second.getValuePointer(variableName);

		if ( value == nullptr )
		{
			/* NOTE: The variable do not exist. */
			return {};
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
			return false;
		}

		const auto * array = storeIt->second.getArrayPointer(variableName);

		if ( array == nullptr )
		{
			return false;
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

	std::ostream &
	operator<< (std::ostream & out, const Settings & obj)
	{
		const std::shared_lock< std::shared_mutex > lock{obj.m_storeAccess};

		auto printValue = [] (const std::any & value) -> std::string {
			if ( value.type() == typeid(std::string) )
			{
				return std::any_cast< std::string >(value);
			}

			if ( value.type() == typeid(bool) )
			{
				return std::any_cast< bool >(value) ? "On" : "Off";
			}

			if ( value.type() == typeid(int32_t) )
			{
				return std::to_string(std::any_cast< int32_t >(value));
			}

			if ( value.type() == typeid(uint32_t) )
			{
				return std::to_string(std::any_cast< uint32_t >(value));
			}

			if ( value.type() == typeid(float) )
			{
				return std::to_string(std::any_cast< float >(value));
			}

			if ( value.type() == typeid(double) )
			{
				return std::to_string(std::any_cast< double >(value));
			}

			return "UNHANDLED";
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
