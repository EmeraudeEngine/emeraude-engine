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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Manager.hpp"

namespace EmEn::Resources
{
	void
	Manager::onRegisterToConsole () noexcept
	{
		this->bindCommand("listContainers", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			std::stringstream json;
			json << "[";

			bool first = true;

			for ( const auto & [typeIndex, container] : m_containers )
			{
				if ( !first )
				{
					json << ",";
				}

				first = false;

				/* Build a query-safe ID by removing spaces. */
				auto id = container->name();
				std::erase(id, ' ');

				json << "{\"id\":\"" << id << "\",\"name\":\"" << container->name() << "\",\"loaded\":" << container->resourceCount() << ",\"available\":" << container->availableResourceNames().size() << "}";
			}

			json << "]";

			outputs.emplace_back(Severity::Info, json.str());

			return true;
		}, "Lists all resource containers with loaded/available counts as JSON.");

		this->bindCommand("listResources", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: listResources(containerName)");

				return false;
			}

			const auto containerName = arguments[0].asString();

			for ( const auto & [typeIndex, container] : m_containers )
			{
				auto id = container->name();
				std::erase(id, ' ');

				if ( container->name() == containerName || id == containerName )
				{
					const auto names = container->availableResourceNames();

					std::stringstream json;
					json << "[";

					for ( size_t i = 0; i < names.size(); i++ )
					{
						if ( i > 0 )
						{
							json << ",";
						}

						json << "\"" << names[i] << "\"";
					}

					json << "]";

					outputs.emplace_back(Severity::Info, json.str());

					return true;
				}
			}

			outputs.emplace_back(Severity::Error, std::stringstream{} << "Container '" << containerName << "' not found !");

			return false;
		}, "Lists available resources in a container as JSON. Usage: listResources(containerName)");
	}
}
