/*
 * src/Scenes/Manager.console.cpp
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

#include "Manager.hpp"

/* STL inclusions. */
#include <ranges>

namespace EmEn::Scenes
{
	using namespace Libs;

	void
	Manager::onRegisterToConsole () noexcept
	{
		this->bindCommand("listScenes", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			std::stringstream list;

			list << "Scenes : " "\n";

			for ( const auto & sceneName : this->getSceneNames() )
			{
				list << " - '" << sceneName << "'" "\n";
			}

			outputs.emplace_back(Severity::Info, list.str());

			return true;
		});

		this->bindCommand("getActiveSceneName", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( m_activeScene != nullptr )
			{
				outputs.emplace_back(Severity::Info, std::stringstream{} << "The active scene is '" <<  m_activeScene->name() << "'");
			}
			else
			{
				outputs.emplace_back(Severity::Warning, "No active scene !");
			}

			return true;
		});

		this->bindCommand("targetActiveScene", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {

			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");
			}

			m_consoleMemory.target(m_activeScene);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Now targeting scene '" << m_activeScene->name() << "'.");

			return true;
		});

		this->bindCommand("targetScene", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "You must specify a scene name !");

				return false;
			}

			const auto name = arguments[0].asString();

			const auto scene = this->getScene(name);

			if ( scene == nullptr )
			{
				outputs.emplace_back(Severity::Warning, std::stringstream{} << "The scene '" <<  name << "' doesn't exists !");

				return false;
			}

			m_consoleMemory.target(scene);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Now targeting scene '" << scene->name() << "'.");

			return true;
		});

		this->bindCommand("listNodes", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			const auto scene = m_consoleMemory.scene();

			if ( scene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "You must target a scene before !");

				return false;
			}

			std::stringstream list;
			list << "Nodes : " "\n";

			for ( const auto & key: scene->root()->children() | std::views::keys )
			{
				list << " - '" <<  key << "'" "\n";
			}

			outputs.emplace_back(Severity::Info, list.str());

			return true;
		});

		this->bindCommand("targetNode", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "You must specify a node name !");

				return false;
			}

			const auto name = arguments[0].asString();

			const auto scene = m_consoleMemory.scene();

			if ( scene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "You must target a scene before !");

				return false;
			}

			const auto sceneNode = scene->root()->findChild(name);

			if ( sceneNode == nullptr )
			{
				outputs.emplace_back(Severity::Warning, std::stringstream{} << "The node '" << name << "' doesn't exists !");

				return false;
			}

			m_consoleMemory.target(sceneNode);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Now targeting node '" << sceneNode->name() << "' from scene '" << scene->name() << "'.");

			return true;
		});

		this->bindCommand("listStaticEntities", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			const auto scene = m_consoleMemory.scene();

			if ( scene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "You must target a scene before !");

				return false;
			}

			std::stringstream list;
			list << "Static entities : " "\n";

			scene->forEachStaticEntities([&list] (const auto & entity) {
				list << " - '" <<  entity.name() << "'" "\n";
			});

			outputs.emplace_back(Severity::Info, list.str());

			return true;
		});

		this->bindCommand("targetStaticEntity", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "You must specify a static entity name !");

				return false;
			}

			const auto name = arguments[0].asString();

			const auto scene = m_consoleMemory.scene();

			if ( scene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "You must target a scene before !");

				return false;
			}

			const auto staticEntity = scene->findStaticEntity(name);

			if ( staticEntity == nullptr )
			{
				outputs.emplace_back(Severity::Warning, std::stringstream{} << "The static entity '" << name << "' doesn't exists !");

				return false;
			}

			m_consoleMemory.target(staticEntity);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Now targeting static entity '" << staticEntity->name() << "' from scene '" << scene->name() << "'.");

			return true;
		});

		this->bindCommand("targetEntityComponent", [] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "You must specify a entity component name !");

				return false;
			}

			return true;
		});

		this->bindCommand("moveNodeTo", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 3 )
			{
				outputs.emplace_back(Severity::Error, "You must specify coordinates !");

				return false;
			}

			const auto positionX = arguments[0].asFloat();
			const auto positionY = arguments[1].asFloat();
			const auto positionZ = arguments[2].asFloat();

			const auto sceneNode = m_consoleMemory.sceneNode();

			if ( sceneNode == nullptr )
			{
				outputs.emplace_back(Severity::Error, "You must target a node before !");

				return false;
			}

			sceneNode->setPosition({positionX, positionY, positionZ}, Math::TransformSpace::World);

			return true;
		});
	}
}
