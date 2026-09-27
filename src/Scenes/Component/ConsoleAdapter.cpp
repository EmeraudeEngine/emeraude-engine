/*
 * src/Scenes/Component/ConsoleAdapter.cpp
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

#include "ConsoleAdapter.hpp"

/* Local inclusions. */
#include "Scenes/Manager.hpp"
#include "Scenes/Node.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/StaticEntity.hpp"

namespace EmEn::Scenes::Component
{
	bool
	resolveComponent (const Scene & scene, const std::string & entityName, const std::string & componentName, std::shared_ptr< Abstract > & component, std::string & error) noexcept
	{
		const auto node = scene.findNode(entityName);
		const auto staticEntity = scene.findStaticEntity(entityName);

		/* NOTE: a node and a static entity may share a name; picking one would act on whichever was
		 * searched first, which the caller cannot see. */
		if ( node != nullptr && staticEntity != nullptr )
		{
			error = "'" + entityName + "' names both a node and a static entity of the scene '" + scene.name() + "': rename one of them.";

			return false;
		}

		std::shared_ptr< AbstractEntity > entity;

		if ( node != nullptr )
		{
			entity = node;
		}
		else if ( staticEntity != nullptr )
		{
			entity = staticEntity;
		}
		else
		{
			error = "No node nor static entity named '" + entityName + "' in the scene '" + scene.name() + "' (SceneManager listEntities() lists them).";

			return false;
		}

		component = entity->getComponent(componentName);

		if ( component == nullptr )
		{
			error = "'" + entityName + "' has no component named '" + componentName + "' (SceneManager listEntityComponents(entity) lists them).";

			return false;
		}

		return true;
	}

	Console::CommandResult
	withComponent (const Manager & sceneManager, const std::string & entityName, const std::string & componentName, const std::function< Console::CommandResult (Abstract &) > & action) noexcept
	{
		auto result = Console::CommandResult::error("No active scene !");

		sceneManager.withExclusiveActiveScene([&] (const std::shared_ptr< Scene > & scene) {
			std::shared_ptr< Abstract > component;
			std::string error;

			if ( !resolveComponent(*scene, entityName, componentName, component, error) )
			{
				result = Console::CommandResult::error(error);

				return;
			}

			result = action(*component);
		}, true);

		return result;
	}
}
