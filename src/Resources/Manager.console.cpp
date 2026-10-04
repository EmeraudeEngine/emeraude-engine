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
#include <algorithm>
#include <functional>
#include <ranges>
#include <string>
#include <vector>

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

		this->bindCommand("memoryCensus", "Returns the CPU memory the loaded resources hold (their local data, not their GPU memory) per container, largest first, as JSON.", [this] () {
			struct Line final
			{
				const ContainerInterface * container{nullptr};
				size_t bytes{0};
				size_t unusedBytes{0};
			};

			std::vector< Line > lines;
			lines.reserve(m_containers.size());

			size_t totalBytes = 0;
			size_t totalUnusedBytes = 0;

			for ( const auto & container : std::views::values(m_containers) )
			{
				const Line line{.container = container.get(), .bytes = container->memoryOccupied(), .unusedBytes = container->unusedMemoryOccupied()};

				totalBytes += line.bytes;
				totalUnusedBytes += line.unusedBytes;

				lines.push_back(line);
			}

			std::ranges::sort(lines, std::ranges::greater{}, &Line::bytes);

			Json::Value containers{Json::arrayValue};

			for ( const auto & line : lines )
			{
				Json::Value entry{Json::objectValue};
				entry["id"] = line.container->resourceClassId();
				entry["loaded"] = static_cast< Json::UInt64 >(line.container->resourceCount());
				entry["bytes"] = static_cast< Json::UInt64 >(line.bytes);
				entry["unusedBytes"] = static_cast< Json::UInt64 >(line.unusedBytes);

				containers.append(std::move(entry));
			}

			Json::Value census{Json::objectValue};
			census["totalBytes"] = static_cast< Json::UInt64 >(totalBytes);
			census["totalUnusedBytes"] = static_cast< Json::UInt64 >(totalUnusedBytes);
			census["containers"] = std::move(containers);

			return Console::CommandResult::json(Base::FastJSON::stringify(census));
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("releaseLocalData", "Releases now, whatever 'Core/Resources/ReleaseLocalData' says and without the grace delay, the CPU copy of every uploaded geometry / image / cubemap no reader holds (diagnostic; a later reader of a released copy fails until the reload exists). Returns the count.", [this] () {
			const auto released = this->releaseLocalData(true);

			return Console::CommandResult::success(Base::String::concatenate(std::to_string(released), " CPU copies released."));
		});

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
