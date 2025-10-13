/*
 * src/Scenes/Manager.cpp
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
#include <cstddef>
#include <algorithm>
#include <map>
#include <memory>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

/* Local inclusions. */
#include "DefinitionResource.hpp"
#include "Graphics/Renderable/AbstractBackground.hpp"
#include "Graphics/Renderable/SceneAreaInterface.hpp"
#include "Graphics/Renderable/SeaLevelInterface.hpp"
#include "Resources/Manager.hpp"
#include "PrimaryServices.hpp"
#include "Scenes/Component/AbstractLightEmitter.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Graphics;

	bool
	Manager::onInitialize () noexcept
	{
		this->registerToConsole();

		Component::AbstractLightEmitter::s_maxDistance = m_primaryServices.settings().getOrSetDefault< float >(GraphicsShadowMappingMaxDistanceKey, DefaultGraphicsShadowMappingMaxDistance);

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_sceneListAccess};

		/* First, disable the possible current active scene. */
		this->disableActiveScene();

		/* Then, remove all scene one by one. */
		for ( auto sceneIt = m_scenes.cbegin(); sceneIt != m_scenes.cend(); )
		{
			if ( sceneIt->second.use_count() > 1 )
			{
				TraceError{ClassId} << "The scene '" << sceneIt->first << "' smart pointer still have " << sceneIt->second.use_count() << " uses ! Force a call to Scene::destroy().";
			}
			else
			{
				TraceSuccess{ClassId} << "Removing scene '" << sceneIt->first << "' ...";
			}

			m_scenes.erase(sceneIt++);

			this->notify(SceneDestroyed);
		}

		return true;
	}

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

	std::shared_ptr< Scene >
	Manager::newScene (const std::string & sceneName, float boundary, const std::shared_ptr< Renderable::AbstractBackground > & background, const std::shared_ptr< Renderable::SceneAreaInterface > & sceneArea, const std::shared_ptr< Renderable::SeaLevelInterface > & seaLevel) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_sceneListAccess};

		if ( this->hasSceneNamed(sceneName) )
		{
			TraceError{ClassId} << "A scene named '" << sceneName << "' already exists ! Delete it first or enable it.";

			return nullptr;
		}

		auto newScene = std::make_shared< Scene >(m_graphicsRenderer, m_audioManager, sceneName, boundary, background, sceneArea, seaLevel);

		this->notify(SceneCreated, newScene);

		return m_scenes.emplace(sceneName, newScene).first->second;
	}

	Manager::SceneLoading
	Manager::loadScene (const std::string & resourceName) noexcept
	{
		/* Loads the scene definition from store (direct loading) */
		const auto sceneDefinition = m_resourceManager.container< DefinitionResource >()->getResource(resourceName, false);

		if ( sceneDefinition == nullptr )
		{
			TraceError{ClassId} << "There is no scene named '" << resourceName << "' in store ! Loading cancelled ...";

			return {nullptr, nullptr};
		}

		/* If everything ok, let the scene definition load the method continuing the job. */
		return this->loadScene(sceneDefinition);
	}

	Manager::SceneLoading
	Manager::loadScene (const std::filesystem::path & filepath) noexcept
	{
		/* Creates a new resource for the scene definition. */
		auto sceneDefinition = m_resourceManager.container< DefinitionResource >()->createResource(filepath.stem().string());

		if ( sceneDefinition == nullptr )
		{
			TraceError{ClassId} << "Unable to create the new scene '" << filepath.stem() << "' ! Loading cancelled ...";

			return {nullptr, nullptr};
		}

		/* Loads the scene definition from the file. */
		if ( !sceneDefinition->load(m_resourceManager, filepath) )
		{
			TraceError{ClassId} << "Unable to load Definition from '" << filepath << "' file ! Loading cancelled ...";

			return {nullptr, sceneDefinition};
		}

		/* If everything ok, let the scene definition load the method continuing the job. */
		return this->loadScene(sceneDefinition);
	}

	Manager::SceneLoading
	Manager::loadScene (const std::shared_ptr< DefinitionResource > & sceneDefinition) noexcept
	{
		const auto sceneName = sceneDefinition->getSceneName();

		/* Creating a new scene in the manager and build with the definition. */
		auto scene = this->newScene(sceneName, DefaultSceneBoundary);

		if ( scene == nullptr )
		{
			TraceError{ClassId} << "Unable to create scene '" << sceneName << "' !";

			return {nullptr, sceneDefinition};
		}

		/* Load the standard scene definition. */
		if ( !sceneDefinition->buildScene(*scene) )
		{
			TraceError{ClassId} << "Unable to build scene '" << sceneName << "' from definition ! Loading cancelled ...";

			return {nullptr, sceneDefinition};
		}

		this->notify(SceneLoaded, scene);

		return {scene, sceneDefinition};
	}

	bool
	Manager::refreshActiveScene () const noexcept
	{
		const std::unique_lock lock{m_activeSceneSharedAccess};

		if ( m_activeScene == nullptr )
		{
			Tracer::info(ClassId, "There is no active scene !");

			return true;
		}

		TraceInfo{ClassId} << "Refreshing scene '" << m_activeScene->name() << "' ...";

		return m_activeScene->refreshRenderableInstances();
	}

	bool
	Manager::deleteScene (const std::string & sceneName) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_sceneListAccess};

		const auto sceneIt = m_scenes.find(sceneName);

		if ( sceneIt == m_scenes.end() )
		{
			TraceError{ClassId} << "Scene '" << sceneName << "' doesn't exist and so can't be deleted !";

			return false;
		}

		/* NOTE: Disable the scene if this is the one being deleted. */
		if ( sceneIt->second == m_activeScene )
		{
			/* NOTE: This will lock the shared mutex here.
			 * As long as we don't have any other code that locks these
			 * two mutexes in reverse order (m_activeSceneSharedAccess -> m_sceneListAccess),
			 * we are safe from a deadlock. */
			this->disableActiveScene();
		}

		m_scenes.erase(sceneIt);

		this->notify(SceneDestroyed);

		return true;
	}

	bool
	Manager::enableScene (const std::shared_ptr< Scene > & scene) noexcept
	{
		if ( m_activeScene != nullptr )
		{
			TraceWarning{ClassId} << "The scene '" << m_activeScene->name() << "' is still active. Disable it before !";

			return false;
		}

		/* NOTE: Be sure the active is not currently used within the rendering or the logics update tasks. */
		const std::unique_lock< std::shared_mutex > activeSceneLock{m_activeSceneSharedAccess};

		if ( scene == nullptr )
		{
			Tracer::error(ClassId, "The scene pointer is null !");

			return false;
		}

		/* Checks whether the scene is usable and tries to complete it otherwise. */
		if ( !scene->enable(m_inputManager, m_primaryServices.settings()) )
		{
			TraceError{ClassId} << "Unable to initialize the scene '" << scene->name() << "' !";

			return false;
		}

		m_activeScene = scene;

		/* Send out a message that the scene has been activated. */
		this->notify(SceneEnabled, m_activeScene);

		TraceSuccess{ClassId} << "Scene '" << m_activeScene->name() << "' loaded !";

		return true;
	}

	bool
	Manager::disableActiveScene () noexcept
	{
		if ( m_activeScene == nullptr )
		{
			return false;
		}

		/* NOTE: Be sure the active is not currently used within the rendering or the logics update tasks. */
		const std::unique_lock< std::shared_mutex > activeSceneLock{m_activeSceneSharedAccess};

		m_activeScene->disable(m_inputManager);

		/* Send out a message that the scene has been deactivated. */
		this->notify(SceneDisabled, m_activeScene);

		m_activeScene.reset();

		return true;
	}

	std::vector< std::string >
	Manager::getSceneNames () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_sceneListAccess};

		if ( m_scenes.empty() )
		{
			return {};
		}

		std::vector< std::string > names;

		std::ranges::transform(m_scenes, std::back_inserter(names), [] (const auto & sceneIt) -> std::string {
			return sceneIt.first;
		});

		return names;
	}

	std::shared_ptr< Scene >
	Manager::getScene (const std::string & sceneName) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_sceneListAccess};

		const auto sceneIt = m_scenes.find(sceneName);

		if ( sceneIt == m_scenes.end() )
		{
			return nullptr;
		}

		return sceneIt->second;
	}

	std::shared_ptr< const Scene >
	Manager::getScene (const std::string & sceneName) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_sceneListAccess};

		const auto sceneIt = m_scenes.find(sceneName);

		if ( sceneIt == m_scenes.cend() )
		{
			return nullptr;
		}

		return sceneIt->second;
	}
}
