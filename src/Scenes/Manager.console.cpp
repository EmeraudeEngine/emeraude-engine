/*
 * src/Scenes/Manager.console.cpp
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

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "Component/Camera.hpp"
#include "Component/Microphone.hpp"
#include "Component/Visual.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/SkyBoxResource.hpp"
#include "Graphics/Renderable/BasicGroundResource.hpp"
#include "Graphics/Material/BasicResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Graphics/Renderer.hpp"
#include "Resources/Manager.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;

	void
	Manager::onRegisterToConsole () noexcept
	{
		this->bindCommand("createScene", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 6 )
			{
				outputs.emplace_back(Severity::Error, "Usage: createScene(name, boundary, cameraNode, camX, camY, camZ [, backgroundName [, groundMaterial]])");

				return false;
			}

			const auto name = arguments[0].asString();

			if ( this->hasSceneNamed(name) )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Scene '" << name << "' already exists !");

				return false;
			}

			const auto boundary = arguments[1].asFloat();

			/* Optional background (arg 6). */
			std::shared_ptr< Graphics::Renderable::AbstractBackground > background;

			if ( arguments.size() >= 7 )
			{
				const auto bgName = arguments[6].asString();

				auto * skyBoxContainer = m_resourceManager.container< Graphics::Renderable::SkyBoxResource >();

				if ( skyBoxContainer != nullptr )
				{
					background = skyBoxContainer->getResource(bgName);
				}
			}

			/* Optional ground (arg 7). */
			std::shared_ptr< Scenes::GroundLevelInterface > groundLevel;

			if ( arguments.size() >= 8 )
			{
				const auto matName = arguments[7].asString();

				std::shared_ptr< Graphics::Material::Interface > materialResource;

				if ( matName == "default" )
				{
					materialResource = m_resourceManager.container< Graphics::Material::BasicResource >()->getDefaultResource();
				}
				else
				{
					materialResource = m_resourceManager.container< Graphics::Material::StandardResource >()->getResource(matName);
				}

				if ( materialResource == nullptr )
				{
					outputs.emplace_back(Severity::Warning, std::stringstream{} << "Ground material '" << matName << "' not found, skipping ground.");
				}
				else
				{
					auto ground = std::make_shared< Graphics::Renderable::BasicGroundResource >(m_resourceManager, "ConsoleGround");

					if ( ground->load(boundary * 2.0F, 8, materialResource, {}, boundary * 2.0F) )
					{
						groundLevel = ground;
					}
					else
					{
						outputs.emplace_back(Severity::Warning, "Ground geometry failed to load !");
					}
				}
			}

			auto scene = this->newScene(name, boundary, background, groundLevel);

			if ( scene == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Failed to create scene '" << name << "' !");

				return false;
			}

			/* Create camera+microphone node BEFORE enabling the scene. */
			const auto cameraNodeName = arguments[2].asString();
			const auto camX = arguments[3].asFloat();
			const auto camY = arguments[4].asFloat();
			const auto camZ = arguments[5].asFloat();

			auto cameraNode = scene->root()->createChild(cameraNodeName, {}, 0);

			if ( cameraNode != nullptr )
			{
				cameraNode->setPosition({camX, camY, camZ}, Libs::Math::TransformSpace::World);
				cameraNode->componentBuilder< Component::Camera >(cameraNodeName + "Camera").asPrimary().build(true);
				cameraNode->componentBuilder< Component::Microphone >(cameraNodeName + "Microphone").asPrimary().build();
			}

			/* Setup basic directional lighting. */
			scene->lightSet().enableAsStaticLighting()
				.setAmbientParameters({0.3F, 0.4F, 0.6F, 1.0F}, 0.25F)
				.setLightParameters({1.0F, 0.95F, 0.8F, 1.0F}, 1.2F)
				.setAsDirectionalLight({1.0F, 1.0F, 1.0F}, true);

			if ( this->enableScene(scene) )
			{
				outputs.emplace_back(Severity::Success, std::stringstream{} << "Scene '" << name << "' created and enabled (boundary: " << boundary << ", camera: " << cameraNodeName << ").");
			}
			else
			{
				outputs.emplace_back(Severity::Warning, std::stringstream{} << "Scene '" << name << "' created but failed to enable.");
			}

			return true;
		}, "Creates a scene with camera. Usage: createScene(name, boundary, cameraNode, camX, camY, camZ [, backgroundName [, groundMaterial]])");

		this->bindCommand("enableScene", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: enableScene(name)");

				return false;
			}

			const auto name = arguments[0].asString();
			const auto scene = this->getScene(name);

			if ( scene == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Scene '" << name << "' not found !");

				return false;
			}

			if ( this->enableScene(scene) )
			{
				outputs.emplace_back(Severity::Success, std::stringstream{} << "Scene '" << name << "' enabled.");
			}
			else
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Failed to enable scene '" << name << "' !");
			}

			return true;
		}, "Enables a scene. Usage: enableScene(name)");

		this->bindCommand("deleteScene", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: deleteScene(name)");

				return false;
			}

			const auto name = arguments[0].asString();

			if ( this->deleteScene(name) )
			{
				outputs.emplace_back(Severity::Success, std::stringstream{} << "Scene '" << name << "' deleted.");
			}
			else
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Failed to delete scene '" << name << "' !");
			}

			return true;
		}, "Deletes a scene. Usage: deleteScene(name)");

		this->bindCommand("createNode", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: createNode(name [, x, y, z])");

				return false;
			}

			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const auto name = arguments[0].asString();

			auto node = m_activeScene->root()->createChild(name, {}, m_activeScene->lifetimeMS());

			if ( node == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Failed to create node '" << name << "' !");

				return false;
			}

			if ( arguments.size() >= 4 )
			{
				const auto x = arguments[1].asFloat();
				const auto y = arguments[2].asFloat();
				const auto z = arguments[3].asFloat();

				node->setPosition({x, y, z}, Libs::Math::TransformSpace::World);

				outputs.emplace_back(Severity::Success, std::stringstream{} << "Node '" << name << "' created at (" << x << ", " << y << ", " << z << ").");
			}
			else
			{
				outputs.emplace_back(Severity::Success, std::stringstream{} << "Node '" << name << "' created at origin.");
			}

			return true;
		}, "Creates a node in the active scene. Usage: createNode(name [, x, y, z])");

		this->bindCommand("destroyNode", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: destroyNode(name)");

				return false;
			}

			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const auto name = arguments[0].asString();

			if ( m_activeScene->root()->destroyChild(name) )
			{
				outputs.emplace_back(Severity::Success, std::stringstream{} << "Node '" << name << "' destroyed.");
			}
			else
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Node '" << name << "' not found !");
			}

			return true;
		}, "Destroys a node. Usage: destroyNode(name)");

		this->bindCommand("attachCamera", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 2 )
			{
				outputs.emplace_back(Severity::Error, "Usage: attachCamera(nodeName, cameraName)");

				return false;
			}

			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const auto nodeName = arguments[0].asString();
			const auto cameraName = arguments[1].asString();

			auto node = m_activeScene->root()->findChild(nodeName);

			if ( node == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Node '" << nodeName << "' not found !");

				return false;
			}

			const auto camera = node->componentBuilder< Component::Camera >(cameraName).asPrimary().build(true);

			if ( camera == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Failed to attach camera '" << cameraName << "' !");

				return false;
			}

			/* Set as the active camera for the scene. */
			m_activeScene->setActiveCamera(camera.get());

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Camera '" << cameraName << "' attached to node '" << nodeName << "' and set as active.");

			return true;
		}, "Attaches a primary camera to a node and sets it as active. Usage: attachCamera(nodeName, cameraName)");

		this->bindCommand("attachMicrophone", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 2 )
			{
				outputs.emplace_back(Severity::Error, "Usage: attachMicrophone(nodeName, microphoneName)");

				return false;
			}

			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const auto nodeName = arguments[0].asString();
			const auto micName = arguments[1].asString();

			auto node = m_activeScene->root()->findChild(nodeName);

			if ( node == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Node '" << nodeName << "' not found !");

				return false;
			}

			/* Remove default microphone node if it exists. */
			m_activeScene->root()->destroyChild("DefaultMicrophoneNode");

			const auto microphone = node->componentBuilder< Component::Microphone >(micName).asPrimary().build();

			if ( microphone == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Failed to attach microphone '" << micName << "' !");

				return false;
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Microphone '" << micName << "' attached to node '" << nodeName << "'.");

			return true;
		}, "Attaches a primary microphone to a node. Usage: attachMicrophone(nodeName, microphoneName)");

		this->bindCommand("setGround", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const auto matName = arguments.empty() ? std::string{"default"} : arguments[0].asString();
			const auto boundary = m_activeScene->boundary();

			std::shared_ptr< Graphics::Material::Interface > materialResource;

			if ( matName == "default" )
			{
				materialResource = m_resourceManager.container< Graphics::Material::BasicResource >()->getDefaultResource();
			}
			else
			{
				materialResource = m_resourceManager.container< Graphics::Material::StandardResource >()->getResource(matName);
			}

			if ( materialResource == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Material '" << matName << "' not found !");

				return false;
			}

			auto ground = std::make_shared< Graphics::Renderable::BasicGroundResource >(m_resourceManager, "ConsoleGround");

			if ( !ground->load(boundary, 8, materialResource, {}, boundary) )
			{
				outputs.emplace_back(Severity::Error, "Ground geometry failed to load !");

				return false;
			}

			m_activeScene->setGroundLevel(ground);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Ground set with material '" << matName << "'.");

			return true;
		}, "Sets the ground for the active scene. Usage: setGround([materialName])");

		this->bindCommand("addMesh", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 5 )
			{
				outputs.emplace_back(Severity::Error, "Usage: addMesh(meshResource, entityName, x, y, z [, scale])");

				return false;
			}

			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const auto meshName = arguments[0].asString();
			const auto entityName = arguments[1].asString();
			const auto x = arguments[2].asFloat();
			const auto y = arguments[3].asFloat();
			const auto z = arguments[4].asFloat();
			const auto scale = arguments.size() >= 6 ? arguments[5].asFloat() : 1.0F;

			auto * meshContainer = m_resourceManager.container< Graphics::Renderable::MeshResource >();

			if ( meshContainer == nullptr )
			{
				outputs.emplace_back(Severity::Error, "MeshResource container not available !");

				return false;
			}

			auto mesh = meshContainer->getResource(meshName);

			if ( mesh == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Mesh '" << meshName << "' not found !");

				return false;
			}

			mesh->setUniformScale(scale);

			const Libs::Math::Vector< 3, float > position{x, y, z};
			auto entity = m_activeScene->createStaticEntity(entityName, position);

			if ( entity == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Failed to create entity '" << entityName << "' !");

				return false;
			}

			const auto visual = entity->componentBuilder< Component::Visual >(entityName + "Visual")
				.setup([scale] (auto & component) {
					component.getRenderableInstance()->enableLighting();
					component.getRenderableInstance()->setTransformationMatrix(Libs::Math::Matrix4F::scaling(scale));
				}).build(mesh);

			if ( visual == nullptr )
			{
				outputs.emplace_back(Severity::Error, "Failed to create visual component !");

				return false;
			}

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Mesh '" << meshName << "' placed at (" << x << ", " << y << ", " << z << ") as '" << entityName << "' (scale: " << scale << ").");

			return true;
		}, "Adds a mesh to the scene. Usage: addMesh(meshResource, entityName, x, y, z [, scale])");

		this->bindCommand("setBackground", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: setBackground(skyboxName)");

				return false;
			}

			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const auto name = arguments[0].asString();

			auto * skyBoxContainer = m_resourceManager.container< Graphics::Renderable::SkyBoxResource >();

			if ( skyBoxContainer == nullptr )
			{
				outputs.emplace_back(Severity::Error, "SkyBox resource container not available !");

				return false;
			}

			auto skyBox = skyBoxContainer->getResource(name);

			if ( skyBox == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "SkyBox resource '" << name << "' not found !");

				return false;
			}

			m_activeScene->setBackground(skyBox);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Background set to '" << name << "'.");

			return true;
		}, "Sets the scene background skybox. Usage: setBackground(skyboxName)");

		this->bindCommand("listScenes", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			std::stringstream list;

			list << "Scenes : " "\n";

			for ( const auto & sceneName : this->getSceneNames() )
			{
				list << " - '" << sceneName << "'" "\n";
			}

			outputs.emplace_back(Severity::Info, list.str());

			return true;
		}, "Lists every registered scene name.");

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
		}, "Returns the name of the currently active scene.");

		this->bindCommand("targetActiveScene", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {

			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");
			}

			m_consoleMemory.target(m_activeScene);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Now targeting scene '" << m_activeScene->name() << "'.");

			return true;
		}, "Targets the active scene for subsequent node/entity commands (listNodes, targetNode, ...).");

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
		}, "Targets the named scene. Usage: targetScene(sceneName)");

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
		}, "Lists root-level nodes of the currently targeted scene. Requires targetScene() or targetActiveScene() first.");

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
		}, "Targets a node within the currently targeted scene. Usage: targetNode(nodeName)");

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
		}, "Lists static entities of the currently targeted scene.");

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
		}, "Targets a static entity within the currently targeted scene. Usage: targetStaticEntity(name)");

		this->bindCommand("targetEntityComponent", [] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "You must specify a entity component name !");

				return false;
			}

			return true;
		}, "Targets a component on the currently targeted entity. Usage: targetEntityComponent(name) [not fully implemented]");

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
		}, "Moves the currently targeted node to world coordinates. Usage: moveNodeTo(x, y, z)");

		this->bindCommand("getSceneInfo", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const auto & scene = *m_activeScene;

			size_t nodeCount = 0;
			size_t staticEntityCount = 0;

			nodeCount = scene.root()->children().size();

			scene.forEachStaticEntities([&staticEntityCount] (const auto &) {
				++staticEntityCount;
			});

			const auto * camera = scene.activeCamera();

			std::stringstream info;
			info << "Scene: " << scene.name() << "\n";
			info << "  Nodes: " << nodeCount << "\n";
			info << "  Static entities: " << staticEntityCount << "\n";
			info << "  Active camera: " << (camera != nullptr ? camera->name() : "none") << "\n";

			outputs.emplace_back(Severity::Info, info.str());

			return true;
		}, "Returns scene information (name, node count, entity count, active camera).");

		this->bindCommand("getNode", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.empty() )
			{
				outputs.emplace_back(Severity::Error, "Usage: getNode(name)");

				return false;
			}

			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const auto name = arguments[0].asString();
			auto node = m_activeScene->root()->findChild(name);

			if ( node == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Node '" << name << "' not found !");

				return false;
			}

			const auto & coords = node->localCoordinates();
			const auto & pos = coords.position();

			std::stringstream info;
			info << "{";
			info << "\"name\":\"" << node->name() << "\",";
			info << "\"address\":\"" << node.get() << "\",";
			info << "\"position\":[" << pos[0] << "," << pos[1] << "," << pos[2] << "],";
			info << "\"childCount\":" << node->children().size();
			info << "}";

			outputs.emplace_back(Severity::Info, info.str());

			return true;
		}, "Returns node info as JSON. Usage: getNode(name)");

		this->bindCommand("setNodePosition", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 4 )
			{
				outputs.emplace_back(Severity::Error, "Usage: setNodePosition(nodeName, x, y, z)");

				return false;
			}

			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const auto name = arguments[0].asString();
			auto node = m_activeScene->root()->findChild(name);

			if ( node == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Node '" << name << "' not found !");

				return false;
			}

			const auto x = arguments[1].asFloat();
			const auto y = arguments[2].asFloat();
			const auto z = arguments[3].asFloat();

			node->setPosition({x, y, z}, Math::TransformSpace::World);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Node '" << name << "' moved to (" << x << ", " << y << ", " << z << ").");

			return true;
		}, "Moves a node. Usage: setNodePosition(nodeName, x, y, z)");

		this->bindCommand("enableTBN", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const bool state = arguments.empty() ? true : arguments[0].asBoolean();

			m_resourceManager.graphicsRenderer().enableTBNSpaceRendering(state);

			size_t toggled = 0;

			m_activeScene->forEachRenderableInstance([state, &toggled] (const auto & renderableInstance) {
				if ( renderableInstance != nullptr )
				{
					renderableInstance->enableDisplayTBNSpace(state);
					++toggled;
				}
			});

			outputs.emplace_back(Severity::Success, std::stringstream{}
				<< "TBN space rendering " << (state ? "enabled" : "disabled")
				<< " on " << toggled << " renderable instance(s) of scene '" << m_activeScene->name() << "'.");

			return true;
		}, "Toggles TBN space debug rendering on the active scene's renderables. Usage: enableTBN([true|false]) — defaults to true.");

		this->bindCommand("setNodeLookAt", [this] (const Console::Arguments & arguments, Console::Outputs & outputs) {
			if ( arguments.size() < 4 )
			{
				outputs.emplace_back(Severity::Error, "Usage: setNodeLookAt(nodeName, x, y, z)");

				return false;
			}

			if ( m_activeScene == nullptr )
			{
				outputs.emplace_back(Severity::Error, "No active scene !");

				return false;
			}

			const auto name = arguments[0].asString();
			auto node = m_activeScene->root()->findChild(name);

			if ( node == nullptr )
			{
				outputs.emplace_back(Severity::Error, std::stringstream{} << "Node '" << name << "' not found !");

				return false;
			}

			const auto x = arguments[1].asFloat();
			const auto y = arguments[2].asFloat();
			const auto z = arguments[3].asFloat();

			node->lookAt({x, y, z}, false);

			outputs.emplace_back(Severity::Success, std::stringstream{} << "Node '" << name << "' looking at (" << x << ", " << y << ", " << z << ").");

			return true;
		}, "Orients a node to look at a point. Usage: setNodeLookAt(nodeName, x, y, z)");
	}
}
