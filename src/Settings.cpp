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
#include <iostream>
#include <stack>

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Libs/IO/IO.hpp"
#include "Libs/String.hpp"
#include "Arguments.hpp"
#include "FileSystem.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	using namespace EmEn::Libs;

	const size_t Settings::ClassUID{getClassUID(ClassId)};

	Settings * Settings::s_instance{nullptr};

	Settings::Settings (const Arguments & arguments, const FileSystem & fileSystem, bool childProcess) noexcept
		: ServiceInterface(ClassId),
		m_arguments(arguments),
		m_fileSystem(fileSystem)
	{
		if ( s_instance != nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

			std::terminate();
		}

		s_instance = this;

		m_flags[ChildProcess] = childProcess;

		if ( !m_flags[ChildProcess] )
		{
			m_flags[SaveAtExit] = true;
		}
	}

	Settings::~Settings ()
	{
		s_instance = nullptr;
	}

	bool
	Settings::onInitialize () noexcept
	{
		/* NOTE: In read-only, settings service is a copy from another process. */
		if ( !m_flags[ChildProcess] )
		{
			m_flags[ShowInformation] = m_arguments.get("--verbose").isPresent();
		}

		const auto argument = m_arguments.get("--settings-filepath");

		if ( argument.isPresent() )
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

		if ( m_flags[ShowInformation] )
		{
			TraceInfo{ClassId} << "Loading settings from file '" << m_filepath.string() << "' ...";
		}

		/* Checks the file presence, if not, it will be created and uses the default engine values. */
		if ( !IO::fileExists(m_filepath) )
		{
			if ( !m_flags[ChildProcess] )
			{
				TraceWarning{ClassId} <<
					"Settings file " << m_filepath << " doesn't exist." "\n"
					"The file will be written at the application successful exit.";

				this->saveAtExit(true);
			}

			return true;
		}

		/* Reading the file ... */
		if ( !this->readFile(m_filepath) )
		{
			TraceError{ClassId} << "Unable to read settings file from '" << m_filepath.string() << "' path !";

			this->saveAtExit(false);

			return false;
		}

		if ( m_arguments.get("--disable-settings-autosave").isPresent() )
		{
			this->saveAtExit(false);
		}

		if ( m_flags[ShowInformation] )
		{
			TraceInfo{ClassId} << *this;
		}

		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	Settings::onTerminate () noexcept
	{
		m_flags[ServiceInitialized] = false;

		if ( this->isSaveAtExitEnabled() )
		{
			if ( m_filepath.empty() )
			{
				TraceError{ClassId} << "File path is empty. Unable to save this settings file !";

				return false;
			}

			if ( !this->writeFile(m_filepath) )
			{
				TraceError{ClassId} << "Unable to write settings file to '" << m_filepath << "' !";

				return false;
			}

			TraceSuccess{ClassId} << "Settings file saved to '" << m_filepath << "' !";
		}

		return true;
	}

	bool
	Settings::readLevel (const Json::Value & data, SettingStore & store) noexcept
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
			const auto & items = data[name];

			if ( items.isObject() )
			{
				auto & subStore = store.getOrCreateSubStore(name);

				if ( !this->readLevel(items, subStore) )
				{
					return false;
				}
			}
			else if ( items.isArray() )
			{
				for ( const auto & item : items )
				{
					store.setVariableInArray(name, toAny(item));
				}
			}
			else
			{
				store.setVariable(name, toAny(items));
			}
		}

		return true;
	}

	bool
	Settings::readFile (const std::filesystem::path & filepath) noexcept
	{
		Json::Value root;

		if ( !FastJSON::getRootFromFile(filepath, root) )
		{
			TraceError{ClassId} << "Unable to parse the settings file " << filepath << " !" "\n";

			return false;
		}

		return this->readLevel(root, m_store);
	}

	bool
	Settings::writeLevel (const SettingStore & store, Json::Value & data) const noexcept
	{
		if ( store.empty() )
		{
			data = Json::objectValue;

			return true;
		}

		/* Write sub-store at this level. */
		for ( const auto & [name, subStore] : store.subStores() )
		{
			auto & json = data[name];

			if ( !this->writeLevel(subStore, json) )
			{
				return false;
			}
		}

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

		/* Write variables at this level. */
		for ( const auto & [name, value] : store.variables() )
		{
			data[name] = toJson(value);
		}

		/* Write array at this level. */
		for ( const auto & [name, values] : store.arrays() )
		{
			data[name] = Json::arrayValue;

			for ( const auto & value : values )
			{
				data[name].append(toJson(value));
			}
		}

		return true;
	}

	bool
	Settings::writeFile (const std::filesystem::path & filepath) const noexcept
	{
		Json::Value root{};

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
		if ( !this->writeLevel(m_store, root) )
		{
			Tracer::error(ClassId, "Unable to generate a store JSON data.");

			return false;
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

	const SettingStore *
	Settings::parseKey (const std::string & key, std::string & variableName) const noexcept
	{
		if ( key.find('/') != std::string::npos )
		{
			const auto sections = String::explode(key, '/', false);
			const auto * currentStore = &m_store;

			for ( const auto & section : sections )
			{
				/* If a sub-store exists with this name, we continue a depth below. */
				if ( currentStore->subStoreExists(section) )
				{
					currentStore = currentStore->getSubStorePointer(section);
				}
				else if ( currentStore->variableExists(section) || currentStore->arrayExists(section) )
				{
					variableName = section;

					return currentStore;
				}
				else
				{
					return nullptr;
				}
			}
		}

		if ( m_store.variableExists(key) && m_store.arrayExists(key) && m_store.subStoreExists(key) )
		{
			return nullptr;
		}

		variableName = key;

		return &m_store;
	}

	SettingStore *
	Settings::parseKey (const std::string & key, std::string & variableName) noexcept
	{
		if ( key.find('/') == std::string::npos )
		{
			return &m_store;
		}

		auto sections = String::explode(key, '/', false);
		auto * currentStore = &m_store;

		/* Remove the last section which is the variable name. */
		variableName = sections.back();
		sections.pop_back();

		for ( const auto & section : sections )
		{
			/* If a sub-store exists with this name, we continue a depth below. */
			if ( currentStore->subStoreExists(section) )
			{
				currentStore = currentStore->getSubStorePointer(section);
			}
			else
			{
				currentStore = &currentStore->getOrCreateSubStore(section);
			}
		}

		return currentStore;
	}

	bool
	Settings::variableExists (const std::string & key) const noexcept
	{
		std::string name;

		const auto * store = this->parseKey(key, name);

		if ( store == nullptr )
		{
			return false;
		}

		return store->variableExists(name);
	}

	bool
	Settings::arrayExists (const std::string & key) const noexcept
	{
		std::string name;

		const auto * store = this->parseKey(key, name);

		if ( store == nullptr )
		{
			return false;
		}

		return store->arrayExists(name);
	}

	bool
	Settings::storeExists (const std::string & key) const noexcept
	{
		std::string name;

		const auto * store = this->parseKey(key, name);

		if ( store == nullptr )
		{
			return false;
		}

		return store->subStoreExists(name);
	}

	std::any
	Settings::get (const std::string & key) const noexcept
	{
		std::string name;

		const auto * store = this->parseKey(key, name);

		if ( store == nullptr )
		{
			return {};
		}

		const auto * value = store->getValuePointer(name);

		if ( value == nullptr )
		{
			return {};
		}

		return *value;
	}

	bool
	Settings::isArrayEmpty (const std::string & key) const noexcept
	{
		std::string name{};

		const auto * store = this->parseKey(key, name);

		if ( store == nullptr || !store->arrayExists(name) )
		{
			return true;
		}

		return store->getArrayPointer(name)->empty();
	}

	bool
	Settings::save () const noexcept
	{
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
		auto dashes = [&out] (size_t depth) {
			out << ' ';

			for ( size_t i = 0; i < depth; i++ )
			{
				out << "----";
			}
		};

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

		struct node_t
		{
			size_t depth;
			std::string name;
			const SettingStore * store;
		};

		std::stack< node_t > stores;

		/* Sets the top Store */
		stores.emplace(node_t{
			.depth = 0,
			.name = "Root",
			.store = &obj.m_store
		});

		out << "Settings (" << obj.m_filepath << ") :" "\n";

		/* Crawling inside all stores. */
		while ( !stores.empty() )
		{
			/* Get a copy and remove it from the stack.*/
			const auto node = stores.top();
			stores.pop();

			dashes(node.depth);

			out << " [" << node.name << "]" "\n";

			/* Print every variable */
			for ( const auto & [name, value] : node.store->variables() )
			{
				dashes(node.depth + 1);

				out << "  " << name << " = " << printValue(value) << '\n';
			}

			/* Print every arrays */
			for ( const auto & [name, values] : node.store->arrays() )
			{
				dashes(node.depth + 1);

				std::string output;

				for ( const auto & value : values )
				{
					output += printValue(value) + ", ";
				}

				out << "  " << name << " = [" << output << "]" "\n";
			}

			/* Then load every sub stores from this store for the next loop cycle. */
			for ( const auto & [name, subStore] : node.store->subStores() )
			{
				stores.emplace(node_t{
					.depth = node.depth + 1,
					.name = name,
					.store = &subStore
				});
			}
		}

		return out;
	}

	std::string
	to_string (const Settings & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
