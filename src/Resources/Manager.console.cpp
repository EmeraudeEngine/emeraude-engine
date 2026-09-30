/*
 * src/Resources/Manager.console.cpp
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

#include "Manager.hpp"

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "FastJSON.hpp"
#include "String.hpp"

namespace EmEn::Resources
{
	void
	Manager::onRegisterToConsole () noexcept
	{
		this->bindCommand("listContainers", "Lists all resource containers with loaded/available counts as JSON.", [this] () {
			Json::Value containers{Json::arrayValue};

			for ( const auto & container : std::views::values(m_containers) )
			{
				Json::Value entry{Json::objectValue};
				entry["id"] = container->resourceClassId();
				entry["name"] = container->name();
				entry["loaded"] = static_cast< Json::UInt64 >(container->resourceCount());
				entry["available"] = static_cast< Json::UInt64 >(container->availableResourceNames().size());

				containers.append(std::move(entry));
			}

			return Console::CommandResult::json(Base::FastJSON::stringify(containers));
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("listResources", "Lists the available resources of a container as JSON.",
			{
				{"container", "The container id or name (listContainers() lists them), e.g. 'SkyBoxResource'."}
			},
			[this] (const std::string & containerName) {
				for ( const auto & [typeIndex, container] : m_containers )
				{
					if ( containerName != container->resourceClassId() && containerName != container->name() )
					{
						continue;
					}

					Json::Value names{Json::arrayValue};

					for ( const auto & name : container->availableResourceNames() )
					{
						names.append(name);
					}

					return Console::CommandResult::json(Base::FastJSON::stringify(names));
				}

				return Console::CommandResult::error(Base::String::concatenate("Container '", containerName, "' not found !"));
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("loadResource", "Requests the asynchronous loading of a resource (downloads it first when its source is ExternalData). Poll it with resourceStatus().",
			{
				{"container", "The container id or name (listContainers() lists them)."},
				{"resource", "The resource name (listResources() lists them)."}
			},
			[this] (const std::string & containerName, const std::string & resourceName) {
				for ( const auto & container : std::views::values(m_containers) )
				{
					if ( containerName != container->resourceClassId() && containerName != container->name() )
					{
						continue;
					}

					if ( !container->requestResource(resourceName) )
					{
						return Console::CommandResult::error(Base::String::concatenate("No resource '", resourceName, "' in container '", containerName, "' !"));
					}

					return Console::CommandResult::success(Base::String::concatenate("Loading of '", resourceName, "' requested. Poll with resourceStatus(", containerName, ", ", resourceName, ")."));
				}

				return Console::CommandResult::error(Base::String::concatenate("Container '", containerName, "' not found !"));
			}, Console::CommandHint::Idempotent);

		this->bindCommand("resourceStatus", "Returns the loading status of a resource: Unloaded, Enqueuing, ManualEnqueuing, Loading, Loaded or Failed.",
			{
				{"container", "The container id or name (listContainers() lists them)."},
				{"resource", "The resource name (listResources() lists them)."}
			},
			[this] (const std::string & containerName, const std::string & resourceName) {
				for ( const auto & container : std::views::values(m_containers) )
				{
					if ( containerName != container->resourceClassId() && containerName != container->name() )
					{
						continue;
					}

					const auto status = container->resourceStatus(resourceName);

					if ( !status )
					{
						return Console::CommandResult::error(Base::String::concatenate("No resource '", resourceName, "' in container '", containerName, "' !"));
					}

					return Console::CommandResult::info(to_cstring(*status));
				}

				return Console::CommandResult::error(Base::String::concatenate("Container '", containerName, "' not found !"));
			}, Console::CommandHint::ReadOnly);
	}
}
