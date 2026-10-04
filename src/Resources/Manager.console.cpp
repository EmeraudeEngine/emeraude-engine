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
#include "PeerStore.hpp"
#include "SharingServer.hpp"
#include "String.hpp"

namespace EmEn::Resources
{
	void
	Manager::onRegisterToConsole () noexcept
	{
		this->bindCommand("sharingStatus", "Returns the resource sharing state as JSON: this engine's server (Core/Resources/Sharing/*) and its peer (Core/Resources/Peer/*).", [this] () {
			Json::Value status{Json::objectValue};
			status["serving"] = m_sharingServer != nullptr && m_sharingServer->isRunning();
			status["serverURL"] = m_sharingServer != nullptr ? m_sharingServer->baseURL() : std::string{};
			status["peer"] = m_peerStore != nullptr ? m_peerStore->baseURL() : std::string{};

			return Console::CommandResult::json(Base::FastJSON::stringify(status));
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("fetchFromPeer", "Copies a data-store file, or every file under a data-store directory, from the peer (Core/Resources/Peer/URL) into this engine's data stores, in the background; each file is verified by its SHA-256. Follow it with peerFetchStatus().",
			{
				{"path", "The path relative to data-stores/, '/' separated, e.g. 'USD/WorldLobby.usdz' or 'Images'. Empty = everything."}
			},
			[this] (const std::string & path) {
				if ( m_peerStore == nullptr )
				{
					return Console::CommandResult::error("No peer: set 'Core/Resources/Peer/URL' (and its BearerToken), then restart.");
				}

				if ( !m_peerStore->fetch(path) )
				{
					return Console::CommandResult::error("A fetch from the peer is already running (peerFetchStatus()).");
				}

				return Console::CommandResult::success(Base::String::concatenate("Fetching '", path, "' from ", m_peerStore->baseURL(), " ..."));
			});

		this->bindCommand("peerFetchStatus", "Returns the state of the last fetchFromPeer() as JSON (files listed, fetched, kept, failed; the current file's progress; the errors).", [this] () {
			if ( m_peerStore == nullptr )
			{
				return Console::CommandResult::error("No peer: 'Core/Resources/Peer/URL' is empty.");
			}

			return Console::CommandResult::json(Base::FastJSON::stringify(m_peerStore->status()));
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("cancelPeerFetch", "Stops a running fetchFromPeer() at its next read (the file in flight is not kept).", [this] () {
			if ( m_peerStore == nullptr )
			{
				return Console::CommandResult::error("No peer: 'Core/Resources/Peer/URL' is empty.");
			}

			m_peerStore->cancel();

			return Console::CommandResult::success("Cancellation requested.");
		});

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

		this->bindCommand("acquireLocalData", "Takes a lease on the CPU copy of a held resource, reloading it when released (store source, else GPU readback), then drops it: reports the outcome as JSON (diagnostic).",
			{
				{"container", "The container id or name (listContainers() lists them), e.g. 'ImageResource'."},
				{"resource", "The resource name."}
			},
			[this] (const std::string & containerName, const std::string & resourceName) {
				for ( const auto & container : std::views::values(m_containers) )
				{
					if ( containerName != container->resourceClassId() && containerName != container->name() )
					{
						continue;
					}

					const auto resource = container->findResource(resourceName);

					if ( resource == nullptr )
					{
						return Console::CommandResult::error(Base::String::concatenate("The container '", containerName, "' holds no resource '", resourceName, "' !"));
					}

					Json::Value report{Json::objectValue};
					report["residentBefore"] = resource->isLocalDataResident();
					report["bytesBefore"] = static_cast< Json::UInt64 >(resource->memoryOccupied());

					{
						const auto lease = resource->acquireLocalData();

						report["leased"] = lease.isValid();
						report["bytesLeased"] = static_cast< Json::UInt64 >(resource->memoryOccupied());
					}

					report["residentAfter"] = resource->isLocalDataResident();

					return Console::CommandResult::json(Base::FastJSON::stringify(report));
				}

				return Console::CommandResult::error(Base::String::concatenate("Container '", containerName, "' not found !"));
			});

		this->bindCommand("requestLocalData", "Asks for the CPU copy of a held resource without waiting: answers whether it was leased (resident), otherwise schedules its reload on the thread pool (diagnostic; ask again later).",
			{
				{"container", "The container id or name (listContainers() lists them), e.g. 'ImageResource'."},
				{"resource", "The resource name."}
			},
			[this] (const std::string & containerName, const std::string & resourceName) {
				for ( const auto & container : std::views::values(m_containers) )
				{
					if ( containerName != container->resourceClassId() && containerName != container->name() )
					{
						continue;
					}

					const auto resource = container->findResource(resourceName);

					if ( resource == nullptr )
					{
						return Console::CommandResult::error(Base::String::concatenate("The container '", containerName, "' holds no resource '", resourceName, "' !"));
					}

					Json::Value report{Json::objectValue};

					{
						const auto lease = resource->requestLocalData();

						report["leased"] = lease.isValid();
					}

					report["bytes"] = static_cast< Json::UInt64 >(resource->memoryOccupied());

					return Console::CommandResult::json(Base::FastJSON::stringify(report));
				}

				return Console::CommandResult::error(Base::String::concatenate("Container '", containerName, "' not found !"));
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
