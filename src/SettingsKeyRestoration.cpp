/*
 * src/SettingsKeyRestoration.cpp
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

#include "SettingsKeyRestoration.hpp"

/* STL inclusions. */
#include <algorithm>

/* Local inclusions. */
#include "FastJSON.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	using namespace Base;

	SettingsKeyRestoration::SettingsKeyRestoration () noexcept
	{
		for ( const auto key : EngineKeys )
		{
			this->keep(key);
		}
	}

	SettingsKeyRestoration::SettingsKeyRestoration (std::span< const std::string_view > keys, std::span< const std::string_view > stores) noexcept
		: SettingsKeyRestoration{}
	{
		for ( const auto key : keys )
		{
			this->keep(key);
		}

		for ( const auto store : stores )
		{
			this->keepStore(store);
		}
	}

	SettingsKeyRestoration
	SettingsKeyRestoration::blank () noexcept
	{
		SettingsKeyRestoration restoration;
		restoration.m_keys.clear();

		return restoration;
	}

	SettingsKeyRestoration &
	SettingsKeyRestoration::keep (std::string_view settingPath) noexcept
	{
		if ( settingPath.empty() )
		{
			TraceWarning{ClassId} << "An empty setting path cannot be kept across a reset. Ignored.";

			return *this;
		}

		if ( SettingsKeyRestoration::isReservedKey(settingPath) )
		{
			TraceWarning{ClassId} << "The file header stamp '" << settingPath << "' cannot be kept across a reset: it would defeat the version guard. Ignored.";

			return *this;
		}

		if ( std::ranges::find(m_keys, settingPath) == m_keys.end() )
		{
			m_keys.emplace_back(settingPath);
		}

		return *this;
	}

	SettingsKeyRestoration &
	SettingsKeyRestoration::keepStore (std::string_view storePath) noexcept
	{
		if ( storePath.empty() )
		{
			TraceWarning{ClassId} << "The root store cannot be kept across a reset: it carries the file header stamps. Ignored.";

			return *this;
		}

		if ( std::ranges::find(m_stores, storePath) == m_stores.end() )
		{
			m_stores.emplace_back(storePath);
		}

		return *this;
	}

	SettingsKeyRestoration &
	SettingsKeyRestoration::forget (std::string_view path) noexcept
	{
		std::erase(m_keys, path);
		std::erase(m_stores, path);

		return *this;
	}

	bool
	SettingsKeyRestoration::isReservedKey (std::string_view settingPath) noexcept
	{
		return
			settingPath == Settings::EngineVersionKey ||
			settingPath == Settings::ApplicationVersionKey ||
			settingPath == Settings::DateKey;
	}

	const Json::Value *
	SettingsKeyRestoration::findNode (const Json::Value & root, std::string_view path) noexcept
	{
		const Json::Value * node = &root;

		while ( !path.empty() )
		{
			const auto slash = path.find('/');
			const auto segment = std::string{path.substr(0, slash)};

			if ( !node->isObject() || !node->isMember(segment) )
			{
				return nullptr;
			}

			node = &(*node)[segment];

			path = slash == std::string_view::npos ? std::string_view{} : path.substr(slash + 1);
		}

		return node;
	}

	bool
	SettingsKeyRestoration::restoreKey (const Json::Value & root, const std::string & settingPath, Settings & target) noexcept
	{
		const auto * node = SettingsKeyRestoration::findNode(root, settingPath);

		if ( node == nullptr )
		{
			return false;
		}

		if ( node->isArray() )
		{
			/* NOTE: An array is restored whole, so a stale element of the fresh store cannot survive next to the old ones. */
			target.clearArray(settingPath);

			for ( const auto & item : *node )
			{
				if ( const auto value = Settings::jsonToSettingValue(item) )
				{
					target.setValueInArray(settingPath, *value);
				}
			}

			return true;
		}

		if ( const auto value = Settings::jsonToSettingValue(*node) )
		{
			target.setValue(settingPath, *value);

			return true;
		}

		/* NOTE: An object where a key was declared: the caller meant keepStore(). Reported as absent. */
		TraceWarning{ClassId} << "The backup holds a store at '" << settingPath << "', not a variable: declare it with keepStore() to carry it over. Skipped.";

		return false;
	}

	bool
	SettingsKeyRestoration::restore (const std::filesystem::path & backupFilepath, Settings & target, Report & report) const noexcept
	{
		report.restored.clear();
		report.absent.clear();

		if ( this->empty() )
		{
			return true;
		}

		const auto root = FastJSON::getRootFromFile(backupFilepath);

		if ( !root )
		{
			TraceError{ClassId} << "Unable to parse the settings backup " << backupFilepath << ": no entry restored.";

			return false;
		}

		for ( const auto & settingPath : m_keys )
		{
			if ( SettingsKeyRestoration::restoreKey(root.value(), settingPath, target) )
			{
				report.restored.emplace_back(settingPath);
			}
			else
			{
				report.absent.emplace_back(settingPath);
			}
		}

		for ( const auto & storePath : m_stores )
		{
			const auto * node = SettingsKeyRestoration::findNode(root.value(), storePath);

			if ( node != nullptr && target.importJSON(*node, storePath) )
			{
				report.restored.emplace_back(storePath + "/*");
			}
			else
			{
				report.absent.emplace_back(storePath + "/*");
			}
		}

		return true;
	}
}
