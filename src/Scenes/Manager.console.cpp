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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Manager.hpp"

/* STL inclusions. */
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <ranges>

/* Local inclusions. */
#include "Component/Camera.hpp"
#include "Component/Microphone.hpp"
#include "Component/Visual.hpp"
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/ImposterAtlas.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Graphics/Renderable/BasicGroundResource.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Graphics/Renderable/SkyBoxResource.hpp"
#include "Scenes/Loaders/GLTFLoader.hpp"
#include "Scenes/Loaders/SceneData.hpp"
#include "Scenes/SceneDataConsumer.hpp"
#include "FileSystem.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Graphics/Renderer.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PrimaryServices.hpp"
#include "Resources/Manager.hpp"
#include "json/json.h"

namespace EmEn::Scenes
{
	using namespace Base;

	void
	Manager::onRegisterToConsole () noexcept
	{
		this->bindCommand("createScene", "Creates a scene with a camera + microphone node and a neutral ambient light (5000 lx), then enables it.",
			{
				{"name", "The scene name (must not exist yet)."},
				{"boundary", "The scene half-size, in metres (the ground, if any, spans twice this)."},
				{"cameraNode", "The name of the node that carries the primary camera and microphone."},
				{"camX", "Camera world X, in metres."},
				{"camY", "Camera world Y, in metres (UP is +Y)."},
				{"camZ", "Camera world Z, in metres."},
				{"backgroundName", "A skybox resource name (listResources(SkyBoxResource)); no background when omitted."},
				{"groundMaterial", "A material resource name, or 'default'; no ground when omitted."}
			},
			[this] (const std::string & name, float boundary, const std::string & cameraNodeName, float camX, float camY, float camZ, const std::optional< std::string > & backgroundName, const std::optional< std::string > & groundMaterialName) {
				if ( this->hasSceneNamed(name) )
				{
					return Console::CommandResult::error("Scene '" + name + "' already exists !");
				}

				Console::Outputs outputs;

				/* Optional background. */
				std::shared_ptr< Graphics::Renderable::AbstractBackground > background;

				if ( backgroundName.has_value() )
				{
					auto * skyBoxContainer = m_resourceManager.container< Graphics::Renderable::SkyBoxResource >();

					if ( skyBoxContainer != nullptr )
					{
						background = skyBoxContainer->getResource(*backgroundName);
					}
				}

				/* Optional ground. */
				std::shared_ptr< Scenes::GroundLevelInterface > groundLevel;

				if ( groundMaterialName.has_value() )
				{
					const auto & matName = *groundMaterialName;

					std::shared_ptr< Graphics::Material::Interface > materialResource;

					if ( matName == "default" )
					{
						materialResource = m_resourceManager.container< Graphics::Material::StandardResource >()->getDefaultResource();
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

					return Console::CommandResult::fromOutputs(std::move(outputs), false);
				}

				/* Create camera+microphone node BEFORE enabling the scene. */
				auto cameraNode = scene->root()->createChild(cameraNodeName, {}, 0);

				if ( cameraNode != nullptr )
				{
					cameraNode->setPosition({camX, camY, camZ}, Base::Math::TransformSpace::World);
					cameraNode->componentBuilder< Component::Camera >(cameraNodeName + "Camera").asPrimary().build(true);
					cameraNode->componentBuilder< Component::Microphone >(cameraNodeName + "Microphone").asPrimary().build();
				}

				/* Setup a neutral ambient lighting (photometric: the intensity is an
				 * illuminance in lux, overcast-like default). */
				scene->lightSet().enable();
				scene->lightSet().setAmbientLightColor({1.0F, 1.0F, 1.0F, 1.0F});
				scene->lightSet().setAmbientLightIntensity(5000.0F);

				if ( this->enableScene(scene) )
				{
					outputs.emplace_back(Severity::Success, std::stringstream{} << "Scene '" << name << "' created and enabled (boundary: " << boundary << ", camera: " << cameraNodeName << ").");
				}
				else
				{
					outputs.emplace_back(Severity::Warning, std::stringstream{} << "Scene '" << name << "' created but failed to enable.");
				}

				return Console::CommandResult::fromOutputs(std::move(outputs), true);
			});

		this->bindCommand("enableScene", "Enables (activates) a registered scene.",
			{
				{"name", "The scene name (listScenes() lists them)."}
			},
			[this] (const std::string & name) {
				const auto scene = this->getScene(name);

				if ( scene == nullptr )
				{
					return Console::CommandResult::error("Scene '" + name + "' not found !");
				}

				if ( !this->enableScene(scene) )
				{
					return Console::CommandResult::error("Failed to enable scene '" + name + "' !");
				}

				return Console::CommandResult::success("Scene '" + name + "' enabled.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("deleteScene", "Deletes a registered scene.",
			{
				{"name", "The scene name (listScenes() lists them)."}
			},
			[this] (const std::string & name) {
				if ( !this->deleteScene(name) )
				{
					return Console::CommandResult::error("Failed to delete scene '" + name + "' !");
				}

				return Console::CommandResult::success("Scene '" + name + "' deleted.");
			}, Console::CommandHint::Destructive);

		this->bindCommand("createNode", "Creates a root-level node in the active scene, at the origin or at a world position (give all three coordinates or none).",
			{
				{"name", "The node name."},
				{"x", "World X, in metres."},
				{"y", "World Y, in metres (UP is +Y)."},
				{"z", "World Z, in metres."}
			},
			[this] (const std::string & name, std::optional< float > x, std::optional< float > y, std::optional< float > z) {
				const auto coordinateCount = static_cast< int >(x.has_value()) + static_cast< int >(y.has_value()) + static_cast< int >(z.has_value());

				if ( coordinateCount != 0 && coordinateCount != 3 )
				{
					return Console::CommandResult::error("createNode(): give all three coordinates (x, y, z) or none.");
				}

				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				auto node = m_activeScene->root()->createChild(name, {}, m_activeScene->lifetimeMS());

				if ( node == nullptr )
				{
					return Console::CommandResult::error("Failed to create node '" + name + "' !");
				}

				std::stringstream message;

				if ( coordinateCount == 3 )
				{
					node->setPosition({*x, *y, *z}, Base::Math::TransformSpace::World);

					message << "Node '" << name << "' created at (" << *x << ", " << *y << ", " << *z << ").";
				}
				else
				{
					message << "Node '" << name << "' created at origin.";
				}

				return Console::CommandResult::success(message.str());
			});

		this->bindCommand("destroyNode", "Destroys a root-level node of the active scene.",
			{
				{"name", "The node name."}
			},
			[this] (const std::string & name) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				if ( !m_activeScene->root()->destroyChild(name) )
				{
					return Console::CommandResult::error("Node '" + name + "' not found !");
				}

				return Console::CommandResult::success("Node '" + name + "' destroyed.");
			}, Console::CommandHint::Destructive);

		this->bindCommand("attachCamera", "Attaches a primary camera to a node of the active scene and sets it as the active camera.",
			{
				{"nodeName", "The node that will carry the camera."},
				{"cameraName", "The name of the new camera component."}
			},
			[this] (const std::string & nodeName, const std::string & cameraName) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				auto node = m_activeScene->root()->findChild(nodeName);

				if ( node == nullptr )
				{
					return Console::CommandResult::error("Node '" + nodeName + "' not found !");
				}

				const auto camera = node->componentBuilder< Component::Camera >(cameraName).asPrimary().build(true);

				if ( camera == nullptr )
				{
					return Console::CommandResult::error("Failed to attach camera '" + cameraName + "' !");
				}

				/* Set as the active camera for the scene. */
				m_activeScene->setActiveCamera(camera);

				return Console::CommandResult::success("Camera '" + cameraName + "' attached to node '" + nodeName + "' and set as active.");
			});

		this->bindCommand("attachMicrophone", "Attaches a primary microphone to a node of the active scene (the default microphone node is removed).",
			{
				{"nodeName", "The node that will carry the microphone."},
				{"microphoneName", "The name of the new microphone component."}
			},
			[this] (const std::string & nodeName, const std::string & micName) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				auto node = m_activeScene->root()->findChild(nodeName);

				if ( node == nullptr )
				{
					return Console::CommandResult::error("Node '" + nodeName + "' not found !");
				}

				/* Remove default microphone node if it exists. */
				m_activeScene->root()->destroyChild("DefaultMicrophoneNode");

				const auto microphone = node->componentBuilder< Component::Microphone >(micName).asPrimary().build();

				if ( microphone == nullptr )
				{
					return Console::CommandResult::error("Failed to attach microphone '" + micName + "' !");
				}

				return Console::CommandResult::success("Microphone '" + micName + "' attached to node '" + nodeName + "'.");
			});

		this->bindCommand("setGround", "Sets a flat ground spanning the active scene boundary.",
			{
				{"materialName", "A material resource name, or 'default'.", "default"}
			},
			[this] (const std::string & matName) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				const auto boundary = m_activeScene->boundary();

				std::shared_ptr< Graphics::Material::Interface > materialResource;

				if ( matName == "default" )
				{
					materialResource = m_resourceManager.container< Graphics::Material::StandardResource >()->getDefaultResource();
				}
				else
				{
					materialResource = m_resourceManager.container< Graphics::Material::StandardResource >()->getResource(matName);
				}

				if ( materialResource == nullptr )
				{
					return Console::CommandResult::error("Material '" + matName + "' not found !");
				}

				auto ground = std::make_shared< Graphics::Renderable::BasicGroundResource >(m_resourceManager, "ConsoleGround");

				if ( !ground->load(boundary, 8, materialResource, {}, boundary) )
				{
					return Console::CommandResult::error("Ground geometry failed to load !");
				}

				m_activeScene->setGroundLevel(ground);

				return Console::CommandResult::success("Ground set with material '" + matName + "'.");
			});

		this->bindCommand("loadGLTF", "Loads a glTF file into the active scene, synchronously (AI/bench harness, small assets only).",
			{
				{"filePath", "Path of the .gltf/.glb file."},
				{"ignored", "IGNORED — the former swapZ (the deleted per-asset axis flip); kept so existing scripts that pass it still work."},
				{"createLights", "Whether the file's KHR_lights_punctual lights are created in the scene.", false}
			},
			[this] (const std::string & filePath, const std::optional< Console::Argument > & /*ignored*/, bool createLights) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				const std::filesystem::path filepath{filePath};

				if ( !std::filesystem::exists(filepath) )
				{
					return Console::CommandResult::error("File '" + filepath.string() + "' does not exist !");
				}

				/* NOTE: the second argument used to be swapZ, the per-asset chirality workaround. That
				 * layer is gone: the mirror is now uniform across every asset, and it is corrected once,
				 * in the projection. createLights keeps its position so existing scripts still work. */
				Loaders::GLTFLoader loader{m_resourceManager};

				Loaders::LoaderOptions options;
				options.environmentReflectionIntensity = 1.0F;
				loader.setOptions(options);

				Loaders::SceneData sceneData;

				/* ⚠️ Synchronous load on purpose: this is an AI/bench harness command for SMALL
				 * assets. A multi-second load stalls the main loop and risks the compositor
				 * killing the surface (see the blocking-load caution) — do not feed it scenes. */
				if ( !loader.load(filepath, sceneData) )
				{
					return Console::CommandResult::error("Failed to load glTF file '" + filepath.string() + "' !");
				}

				SceneDataConsumer consumer;
				consumer.setCreateLights(createLights);

				if ( !consumer.build(sceneData, *m_activeScene) )
				{
					return Console::CommandResult::error("Failed to build the scene from '" + filepath.string() + "' !");
				}

				return Console::CommandResult::success("glTF file '" + filepath.string() + "' loaded into scene '" + m_activeScene->name() + "'.");
			});

		this->bindCommand("addMesh", "Places a mesh resource in the active scene as a lit static entity.",
			{
				{"meshResource", "The mesh resource name (listResources(MeshResource))."},
				{"entityName", "The name of the new static entity."},
				{"x", "World X, in metres."},
				{"y", "World Y, in metres (UP is +Y)."},
				{"z", "World Z, in metres."},
				{"scale", "Uniform scale factor (greater than 0).", 1.0F}
			},
			[this] (const std::string & meshName, const std::string & entityName, float x, float y, float z, float scale) {
				if ( scale <= 0.0F )
				{
					return Console::CommandResult::error("addMesh(): scale must be greater than 0.");
				}

				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				auto * meshContainer = m_resourceManager.container< Graphics::Renderable::MultiLayerMeshResource >();

				if ( meshContainer == nullptr )
				{
					return Console::CommandResult::error("MeshResource container not available !");
				}

				auto mesh = meshContainer->getResource(meshName);

				if ( mesh == nullptr )
				{
					return Console::CommandResult::error("Mesh '" + meshName + "' not found !");
				}

				mesh->setUniformScale(scale);

				const Base::Math::Vector< 3, float > position{x, y, z};
				auto entity = m_activeScene->createStaticEntity(entityName, position);

				if ( entity == nullptr )
				{
					return Console::CommandResult::error("Failed to create entity '" + entityName + "' !");
				}

				const auto visual = entity->componentBuilder< Component::Visual >(entityName + "Visual")
					.setup([scale] (auto & component) {
						component.getRenderableInstance()->setTransformationMatrix(Base::Math::Matrix4F::scaling(scale));
					}).build(mesh, Graphics::RenderableInstance::Lighting::Lit);

				if ( visual == nullptr )
				{
					return Console::CommandResult::error("Failed to create visual component !");
				}

				std::stringstream message;
				message << "Mesh '" << meshName << "' placed at (" << x << ", " << y << ", " << z << ") as '" << entityName << "' (scale: " << scale << ").";

				return Console::CommandResult::success(message.str());
			});

		this->bindCommand("setBackground", "Sets the active scene background skybox, optionally deriving the scene lighting from its sky manifest.",
			{
				{"skyboxName", "The skybox resource name (listResources(SkyBoxResource))."},
				{"applyLighting", "Whether the sky manifest drives the scene lighting (ambient illuminance, stars).", false}
			},
			[this] (const std::string & name, bool applyLighting) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				auto * skyBoxContainer = m_resourceManager.container< Graphics::Renderable::SkyBoxResource >();

				if ( skyBoxContainer == nullptr )
				{
					return Console::CommandResult::error("SkyBox resource container not available !");
				}

				auto skyBox = skyBoxContainer->getResource(name);

				if ( skyBox == nullptr )
				{
					return Console::CommandResult::error("SkyBox resource '" + name + "' not found !");
				}

				m_activeScene->setBackground(skyBox);

				/* Optional: derive the scene lighting from the sky manifest. */
				if ( applyLighting )
				{
					if ( m_activeScene->applyBackgroundLighting() )
					{
						std::stringstream message;
						message << "Background set to '" << name << "' with sky-driven lighting (ambient: " <<
							(skyBox->isLoaded() ? std::to_string(skyBox->ambientIlluminance()) : std::string{"deferred"}) << " lx, stars: " <<
							(skyBox->isLoaded() ? std::to_string(skyBox->stars().size()) : std::string{"deferred"}) << ").";

						return Console::CommandResult::success(message.str());
					}

					return Console::CommandResult::warning("Background set, but the lighting derivation failed.");
				}

				return Console::CommandResult::success("Background set to '" + name + "'.");
			});

		this->bindCommand("listScenes", "Lists every registered scene name.", [this] () {
			std::stringstream list;

			list << "Scenes : " "\n";

			for ( const auto & sceneName : this->getSceneNames() )
			{
				list << " - '" << sceneName << "'" "\n";
			}

			return Console::CommandResult::info(list.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("getActiveSceneName", "Returns the name of the currently active scene.", [this] () {
			if ( m_activeScene == nullptr )
			{
				return Console::CommandResult::warning("No active scene !");
			}

			return Console::CommandResult::info("The active scene is '" + m_activeScene->name() + "'");
		}, Console::CommandHint::ReadOnly);

		/* NOTE: The compass is a Core-level debug display, bound to F9 on the keyboard. The console
		 * keeps its OWN entry point: it is the deterministic way to drive it (an injected keyPress()
		 * does reach Core since 2026-09-13, but a command states what it does and answers). Every
		 * debug display an AI session must drive has to be reachable from here, not only from a key
		 * binding. */
		this->bindCommand("toggleCompass", "Toggles the world orientation compass (same as the F9 key). Bright spheres: R=X+, G=Y+, B=Z+; complementary: Cyan=X-, Magenta=Y-, Yellow=Z-.", [this] () {
			if ( m_activeScene == nullptr )
			{
				return Console::CommandResult::error("No active scene !");
			}

			if ( m_activeScene->toggleCompassDisplay(m_resourceManager) )
			{
				return Console::CommandResult::success("World compass displayed.");
			}

			return Console::CommandResult::success("World compass hidden.");
		});

		this->bindCommand("targetActiveScene", "Targets the active scene for subsequent node/entity commands (listNodes, targetNode, ...).", [this] () {
			if ( m_activeScene == nullptr )
			{
				return Console::CommandResult::error("No active scene !");
			}

			m_consoleMemory.target(m_activeScene);

			return Console::CommandResult::success("Now targeting scene '" + m_activeScene->name() + "'.");
		}, Console::CommandHint::Idempotent);

		this->bindCommand("targetScene", "Targets the named scene for subsequent node/entity commands.",
			{
				{"sceneName", "The scene name (listScenes() lists them)."}
			},
			[this] (const std::string & name) {
				const auto scene = this->getScene(name);

				if ( scene == nullptr )
				{
					return Console::CommandResult::error("The scene '" + name + "' doesn't exists !");
				}

				m_consoleMemory.target(scene);

				return Console::CommandResult::success("Now targeting scene '" + scene->name() + "'.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("listNodes", "Lists root-level nodes of the currently targeted scene. Requires targetScene() or targetActiveScene() first.", [this] () {
			const auto scene = m_consoleMemory.scene();

			if ( scene == nullptr )
			{
				return Console::CommandResult::error("You must target a scene before !");
			}

			std::stringstream list;
			list << "Nodes : " "\n";

			for ( const auto & key: scene->root()->children() | std::views::keys )
			{
				list << " - '" <<  key << "'" "\n";
			}

			return Console::CommandResult::info(list.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("targetNode", "Targets a root-level node of the currently targeted scene (for moveNodeTo()).",
			{
				{"nodeName", "The node name (listNodes() lists them)."}
			},
			[this] (const std::string & name) {
				const auto scene = m_consoleMemory.scene();

				if ( scene == nullptr )
				{
					return Console::CommandResult::error("You must target a scene before !");
				}

				const auto sceneNode = scene->root()->findChild(name);

				if ( sceneNode == nullptr )
				{
					return Console::CommandResult::error("The node '" + name + "' doesn't exists !");
				}

				m_consoleMemory.target(sceneNode);

				return Console::CommandResult::success("Now targeting node '" + sceneNode->name() + "' from scene '" + scene->name() + "'.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("listStaticEntities", "Lists static entities of the currently targeted scene.", [this] () {
			const auto scene = m_consoleMemory.scene();

			if ( scene == nullptr )
			{
				return Console::CommandResult::error("You must target a scene before !");
			}

			std::stringstream list;
			list << "Static entities : " "\n";

			scene->forEachStaticEntities([&list] (const auto & entity) {
				list << " - '" <<  entity.name() << "'" "\n";
			});

			return Console::CommandResult::info(list.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("targetStaticEntity", "Targets a static entity of the currently targeted scene.",
			{
				{"name", "The static entity name (listStaticEntities() lists them)."}
			},
			[this] (const std::string & name) {
				const auto scene = m_consoleMemory.scene();

				if ( scene == nullptr )
				{
					return Console::CommandResult::error("You must target a scene before !");
				}

				const auto staticEntity = scene->findStaticEntity(name);

				if ( staticEntity == nullptr )
				{
					return Console::CommandResult::error("The static entity '" + name + "' doesn't exists !");
				}

				m_consoleMemory.target(staticEntity);

				return Console::CommandResult::success("Now targeting static entity '" + staticEntity->name() + "' from scene '" + scene->name() + "'.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("targetEntityComponent", "NOT IMPLEMENTED: would target a component of the currently targeted entity. Always answers an error.",
			{
				{"name", "The component name."}
			},
			[] (const std::string & /*name*/) {
				return Console::CommandResult::error("targetEntityComponent(): not implemented.");
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("moveNodeTo", "Moves the currently targeted node (targetNode()) to world coordinates.",
			{
				{"x", "World X, in metres."},
				{"y", "World Y, in metres (UP is +Y)."},
				{"z", "World Z, in metres."}
			},
			[this] (float positionX, float positionY, float positionZ) {
				const auto sceneNode = m_consoleMemory.sceneNode();

				if ( sceneNode == nullptr )
				{
					return Console::CommandResult::error("You must target a node before !");
				}

				sceneNode->setPosition({positionX, positionY, positionZ}, Math::TransformSpace::World);

				std::stringstream message;
				message << "Node '" << sceneNode->name() << "' moved to (" << positionX << ", " << positionY << ", " << positionZ << ").";

				return Console::CommandResult::success(message.str());
			}, Console::CommandHint::Idempotent);

		this->bindCommand("getSceneInfo", "Returns scene information (name, node count, entity count, active camera).", [this] () {
			if ( m_activeScene == nullptr )
			{
				return Console::CommandResult::error("No active scene !");
			}

			const auto & scene = *m_activeScene;

			size_t nodeCount = 0;
			size_t staticEntityCount = 0;

			nodeCount = scene.root()->children().size();

			scene.forEachStaticEntities([&staticEntityCount] (const auto &) {
				++staticEntityCount;
			});

			const auto camera = scene.activeCamera();

			std::stringstream info;
			info << "Scene: " << scene.name() << "\n";
			info << "  Nodes: " << nodeCount << "\n";
			info << "  Static entities: " << staticEntityCount << "\n";
			info << "  Active camera: " << (camera != nullptr ? camera->name() : "none") << "\n";

			return Console::CommandResult::info(info.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("getRenderStatistics", "Returns what the last frame's render lists submit, per geometry LOD: batches, instances, triangles (view, then shadows).", [this] () {
			if ( m_activeScene == nullptr )
			{
				return Console::CommandResult::error("No active scene !");
			}

			const auto print = [] (std::stringstream & output, const char * title, const Scene::RenderListStatistics & statistics) {
				uint64_t batches = 0;
				uint64_t instances = 0;
				uint64_t triangles = 0;

				output << title;

				if ( statistics.targets > 0 )
				{
					output << " (" << statistics.targets << " pass(es), one per cascade on a cascaded map)";
				}

				output << "\n";

				for ( size_t level = 0; level < statistics.batches.size(); ++level )
				{
					/* The view ladder always prints; a level past it only when something was drawn with it. */
					if ( level >= Graphics::Renderable::MaxLODLevels && statistics.batches[level] == 0 )
					{
						continue;
					}

					output << "  LOD " << level << ": " << statistics.batches[level] << " batches, " << statistics.instances[level] << " instances, " << statistics.triangles[level] << " triangles\n";

					batches += statistics.batches[level];
					instances += statistics.instances[level];
					triangles += statistics.triangles[level];
				}

				output << "  total: " << batches << " batches, " << instances << " instances, " << triangles << " triangles\n";
			};

			std::stringstream info;

			print(info, "View lists (last frame)", m_activeScene->viewRenderStatistics());
			print(info, "Shadow lists (last frame)", m_activeScene->shadowRenderStatistics());

			return Console::CommandResult::info(info.str());
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("getStateSyncStatistics", "Returns how often a rendered frame read a logic state the logic thread was rewriting (must be 0).",
			{
				{"reset", "Whether the measurement window is reset after reading.", false}
			},
			[this] (bool reset) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				const auto statistics = m_activeScene->stateSyncStatistics(reset);

				std::stringstream info;
				info << "Logic-to-render state synchronisation" << (reset ? " (window reset)" : "") << "\n";
				info << "  Frames measured: " << statistics.frames << "\n";

				if ( statistics.frames > 0 )
				{
					const auto frames = static_cast< double >(statistics.frames);

					info << "  Frames whose latched state was overwritten: " << statistics.overwrittenFrames << " (" << (100.0 * static_cast< double >(statistics.overwrittenFrames) / frames) << " %)\n";
					info << "  Logic publications inside a frame: mean " << (static_cast< double >(statistics.publicationsDuringFrames) / frames) << ", max " << statistics.maxPublicationsDuringFrame << "\n";
					info << "  Latch-to-end: mean " << (statistics.latchToEndMSSum / frames) << " ms, max " << statistics.latchToEndMSMax << " ms\n";
				}

				return Console::CommandResult::info(info.str());
			});

		this->bindCommand("writeImposterAtlases", "Writes the albedo (premultiplied, mip 0) of every imposter atlas the active scene baked to the captures directory.", [this] () {
			if ( m_activeScene == nullptr )
			{
				return Console::CommandResult::error("No active scene !");
			}

			const auto bakeTarget = m_activeScene->existingImposterBakeTarget();

			if ( bakeTarget == nullptr )
			{
				return Console::CommandResult::warning("The active scene baked no imposter.");
			}

			auto & renderer = m_activeScene->AVConsoleManager().graphicsRenderer();
			const auto captureDirectory = renderer.primaryServices().fileSystem().userDataDirectory("captures");

			std::stringstream report;
			size_t written = 0;

			for ( const auto & atlas : bakeTarget->bakedAtlases() )
			{
				auto fileName = atlas->name();

				std::ranges::replace(fileName, '/', '-');

				const auto filepath = captureDirectory / (fileName + "-albedo.png");

				if ( atlas->writeAlbedo(renderer.transferManager(), filepath) )
				{
					report << filepath.string() << "\n";

					++written;
				}
			}

			report << written << " imposter atlas(es) written, " << bakeTarget->pendingJobs() << " bake(s) still queued.";

			return Console::CommandResult::info(report.str());
		});

		this->bindCommand("getNode", "Returns a root-level node of the active scene as JSON (name, address, local position, child count).",
			{
				{"name", "The node name."}
			},
			[this] (const std::string & name) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				auto node = m_activeScene->root()->findChild(name);

				if ( node == nullptr )
				{
					return Console::CommandResult::error("Node '" + name + "' not found !");
				}

				const auto & coords = node->localCoordinates();
				const auto & pos = coords.position();

				std::stringstream info;
				info << "{";
				info << "\"name\":" << Json::valueToQuotedString(node->name().c_str(), node->name().size()) << ",";
				info << "\"address\":\"" << node.get() << "\",";
				info << "\"position\":[" << pos[0] << "," << pos[1] << "," << pos[2] << "],";
				info << "\"childCount\":" << node->children().size();
				info << "}";

				return Console::CommandResult::json(info.str());
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("addTexturedShape", "Spawns a generated shape textured with a data-store image — the tool for inspecting UV mapping. Lit: in a scene with no light it renders BLACK.",
			{
				{"shape", "One of: sphere, cuboid, plane, cylinder, cone, torus, disk, capsule, hemisphere."},
				{"imageName", "A Texture2D resource name from the data stores."},
				{"entityName", "The name of the new static entity."},
				{"x", "World X, in metres."},
				{"y", "World Y, in metres (UP is +Y)."},
				{"z", "World Z, in metres."},
				{"size", "The shape's characteristic size (radius or edge), in metres (greater than 0).", 1.0F}
			},
			[this] (const std::string & shapeName, const std::string & imageName, const std::string & entityName, float x, float y, float z, float size) {
				if ( size <= 0.0F )
				{
					return Console::CommandResult::error("addTexturedShape(): size must be greater than 0.");
				}

				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				/* ⚠️ Resource names carry the shape, the size AND the image: two calls that differ in any
				 * of them must not silently serve each other's cached resource. */
				const auto suffix = shapeName + "-" + std::to_string(size) + "-" + imageName;

				/* ⚠️ SYNCHRONOUS on purpose. The unsuffixed getOrCreateResource() runs its factory on the
				 * thread pool, which for a console command means the caller gets a success reply before
				 * anything exists — and the factory would outlive the locals it captured. These are small
				 * procedural assets, so blocking here is the correct trade. */
				const auto material = m_resourceManager.container< Graphics::Material::StandardResource >()
					->getOrCreateResourceSync("ConsoleUV-" + imageName, [this, &imageName] (Graphics::Material::StandardResource & newMaterial) {
						const auto texture = m_resourceManager.container< Graphics::TextureResource::Texture2D >()->getResource(imageName);

						if ( texture == nullptr || !newMaterial.setAlbedoComponent(texture) )
						{
							return newMaterial.setManualLoadSuccess(false);
						}

						return newMaterial.setManualLoadSuccess(true);
					});

				if ( material == nullptr )
				{
					return Console::CommandResult::error("Failed to build a material from image '" + imageName + "' !");
				}

				/* Texture coordinates are the whole point of this command, so they are requested
				 * explicitly rather than left to whatever the default happens to be. */
				const Graphics::Geometry::ResourceGenerator generator{m_resourceManager, Graphics::Geometry::EnableTangentSpace | Graphics::Geometry::EnablePrimaryTextureCoordinates};

				std::shared_ptr< Graphics::Geometry::IndexedVertexResource > geometry;

				if ( shapeName == "sphere" ) { geometry = generator.sphere(size, 32, 16, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "cuboid" ) { geometry = generator.cuboid(size, size, size, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "plane" ) { geometry = generator.plane(size, size, 1, 1, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "cylinder" ) { geometry = generator.cylinder(size, size, size * 2.0F, 32, 4, {}, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "cone" ) { geometry = generator.cone(size, size * 2.0F, 32, 4, {}, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "torus" ) { geometry = generator.torus(size, size * 0.3F, 32, 32, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "disk" ) { geometry = generator.disk(size, size * 0.4F, 32, 1, Base::VertexFactory::CapUVMapping::Planar, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "capsule" ) { geometry = generator.capsule(size * 0.5F, size * 2.0F, 32, 16, "ConsoleUVGeo-" + suffix); }
				else if ( shapeName == "hemisphere" ) { geometry = generator.hemisphere(size, 32, 16, "ConsoleUVGeo-" + suffix); }
				else
				{
					return Console::CommandResult::error("Unknown shape '" + shapeName + "' ! Known: sphere, cuboid, plane, cylinder, cone, torus, disk, capsule, hemisphere");
				}

				if ( geometry == nullptr )
				{
					return Console::CommandResult::error("Failed to generate the '" + shapeName + "' geometry !");
				}

				const auto mesh = m_resourceManager.container< Graphics::Renderable::MultiLayerMeshResource >()
					->getOrCreateResourceSync("ConsoleUVMesh-" + suffix, [&geometry, &material] (Graphics::Renderable::MultiLayerMeshResource & meshResource) {
						return meshResource.load(geometry, material);
					});

				if ( mesh == nullptr )
				{
					return Console::CommandResult::error("Failed to build the mesh !");
				}

				const Base::Math::Vector< 3, float > position{x, y, z};
				auto entity = m_activeScene->createStaticEntity(entityName, position);

				if ( entity == nullptr )
				{
					return Console::CommandResult::error("Failed to create entity '" + entityName + "' !");
				}

				/* Lit, like addMesh: a UV check must be readable in the scene it lives in. ⚠️ In a scene
				 * with no light the shape renders BLACK — that is a missing light, not a broken UV. */
				if ( entity->componentBuilder< Component::Visual >(entityName + "Visual").build(mesh, Graphics::RenderableInstance::Lighting::Lit) == nullptr )
				{
					return Console::CommandResult::error("Failed to attach the visual !");
				}

				return Console::CommandResult::success("'" + shapeName + "' textured with '" + imageName + "' added as '" + entityName + "'.");
			});

		this->bindCommand("getNodePhysics", "Returns the physics state of a root-level node of the active scene as JSON: world/local position, linear velocity, movable, grounded and WHICH surface it rests on (Ground/Boundary/Entity).",
			{
				{"name", "The node name."}
			},
			[this] (const std::string & name) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				const auto node = m_activeScene->root()->findChild(name);

				if ( node == nullptr )
				{
					return Console::CommandResult::error("Node '" + name + "' not found !");
				}

				/* ⚠️ WORLD position, not local: a node parented under another reports a local position
				 * that says nothing about where it rests in the world, which is what a physics
				 * measurement is about. */
				const auto worldPosition = node->getWorldCoordinates().position();
				const auto & localPosition = node->localCoordinates().position();
				const auto & velocity = node->linearVelocity();

				const auto * groundedSource = "None";

				switch ( node->groundedSource() )
				{
					case Physics::GroundedSource::Ground : groundedSource = "Ground"; break;
					case Physics::GroundedSource::Boundary : groundedSource = "Boundary"; break;
					case Physics::GroundedSource::Entity : groundedSource = "Entity"; break;
					case Physics::GroundedSource::None : break;
				}

				std::stringstream info;
				info << "{";
				info << "\"name\":" << Json::valueToQuotedString(node->name().c_str(), node->name().size()) << ",";
				info << "\"worldPosition\":[" << worldPosition[0] << "," << worldPosition[1] << "," << worldPosition[2] << "],";
				info << "\"localPosition\":[" << localPosition[0] << "," << localPosition[1] << "," << localPosition[2] << "],";
				info << "\"linearVelocity\":[" << velocity[0] << "," << velocity[1] << "," << velocity[2] << "],";
				info << "\"movable\":" << (node->isMovable() ? "true" : "false") << ",";
				info << "\"grounded\":" << (node->isGrounded() ? "true" : "false") << ",";
				info << "\"groundedSource\":\"" << groundedSource << "\"";
				info << "}";

				return Console::CommandResult::json(info.str());
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("setNodePosition", "Moves a root-level node of the active scene to world coordinates.",
			{
				{"nodeName", "The node name."},
				{"x", "World X, in metres."},
				{"y", "World Y, in metres (UP is +Y)."},
				{"z", "World Z, in metres."}
			},
			[this] (const std::string & name, float x, float y, float z) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				auto node = m_activeScene->root()->findChild(name);

				if ( node == nullptr )
				{
					return Console::CommandResult::error("Node '" + name + "' not found !");
				}

				node->setPosition({x, y, z}, Math::TransformSpace::World);

				std::stringstream message;
				message << "Node '" << name << "' moved to (" << x << ", " << y << ", " << z << ").";

				return Console::CommandResult::success(message.str());
			}, Console::CommandHint::Idempotent);

		this->bindCommand("enableTBN", "Enables or disables the TBN space debug rendering on the active scene's renderables.",
			{
				{"enabled", "1 (true) draws the tangent/bitangent/normal of every vertex, 0 (false) stops.", true}
			},
			[this] (bool state) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				m_resourceManager.graphicsRenderer().enableTBNSpaceRendering(state);

				size_t toggled = 0;

				m_activeScene->forEachRenderableInstance([state, &toggled] (const auto & renderableInstance) {
					if ( renderableInstance != nullptr )
					{
						renderableInstance->enableDisplayTBNSpace(state);
						++toggled;
					}
				});

				std::stringstream message;
				message << "TBN space rendering " << (state ? "enabled" : "disabled") << " on " << toggled << " renderable instance(s) of scene '" << m_activeScene->name() << "'.";

				return Console::CommandResult::success(message.str());
			}, Console::CommandHint::Idempotent);

		this->bindCommand("setNodeLookAt", "Orients a root-level node of the active scene toward a world point.",
			{
				{"nodeName", "The node name."},
				{"x", "Target world X, in metres."},
				{"y", "Target world Y, in metres (UP is +Y)."},
				{"z", "Target world Z, in metres."}
			},
			[this] (const std::string & name, float x, float y, float z) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				auto node = m_activeScene->root()->findChild(name);

				if ( node == nullptr )
				{
					return Console::CommandResult::error("Node '" + name + "' not found !");
				}

				node->lookAt({x, y, z}, false);

				std::stringstream message;
				message << "Node '" << name << "' looking at (" << x << ", " << y << ", " << z << ").";

				return Console::CommandResult::success(message.str());
			}, Console::CommandHint::Idempotent);

		this->bindCommand("dumpRenderTarget", "Dumps the color image (layer 0) of the first render-to-texture target of the active scene whose id contains a fragment, as a PNG in the captures directory.",
			{
				{"nameFragment", "A substring of the render target id."}
			},
			[this] (const std::string & nameFragment) {
				Console::Outputs outputs;
				bool dumped = false;

				this->withSharedActiveScene([&] (const std::shared_ptr< Scene > & scene) {
					scene->forEachRenderToTexture([&] (const std::shared_ptr< Graphics::RenderTarget::Abstract > & renderTarget) {
						if ( dumped || renderTarget->id().find(nameFragment) == std::string::npos )
						{
							return;
						}

						const auto * textureInterface = dynamic_cast< const Vulkan::TextureInterface * >(renderTarget.get());

						if ( textureInterface == nullptr || textureInterface->image() == nullptr )
						{
							outputs.emplace_back(Severity::Error, std::stringstream{} << "Render target '" << renderTarget->id() << "' has no readable image !");

							return;
						}

						/* A target that never rendered still holds its image in the UNDEFINED layout:
						 * the SHADER_READ_ONLY readback below is then undefined behaviour (measured:
						 * process crash, even after waitIdle). The on-demand policies make this state
						 * perfectly reachable — an unfired "once" probe, a suspended target. */
						if ( !renderTarget->hasBeenRendered() )
						{
							outputs.emplace_back(Severity::Error, std::stringstream{} << "Render target '" << renderTarget->id() << "' has never been rendered — nothing to dump yet.");

							return;
						}

						auto & renderer = scene->AVConsoleManager().graphicsRenderer();

						PixelFactory::Pixmap< uint8_t > pixmap;

						/* Debug tool: this runs on the console thread while the render thread may be
						 * drawing INTO this very target — a raw download crashed the process. Full
						 * device sync first; brutal, but correct for a diagnostic command. */
						renderer.device()->waitIdle("dumpRenderTarget console command");

						/* NOTE: The render pass leaves the color attachment in SHADER_READ_ONLY_OPTIMAL
						 * (it is sampled). Layer 0 only (+X face for a cubemap) — enough to tell a
						 * black/dead probe from a live one. */
						if ( !renderer.transferManager().downloadImage(*textureInterface->image(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, pixmap) )
						{
							outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to download the image of '" << renderTarget->id() << "' !");

							return;
						}

						auto captureDirectory = renderer.primaryServices().fileSystem().userDataDirectory("captures");

						std::stringstream filename;
						filename << "rt-dump-" << std::chrono::duration_cast< std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count() << ".png";

						const auto filepath = captureDirectory.append(filename.str());

						if ( !PixelFactory::FileIO::write(pixmap, filepath) )
						{
							outputs.emplace_back(Severity::Error, std::stringstream{} << "Unable to write the dump to " << filepath);

							return;
						}

						outputs.emplace_back(Severity::Success, std::stringstream{} << "Render target '" << renderTarget->id() << "' dumped: " << filepath);

						dumped = true;
					});
				}, true);

				if ( !dumped && outputs.empty() )
				{
					outputs.emplace_back(Severity::Error, std::stringstream{} << "No render-to-texture target matching '" << nameFragment << "' in the active scene.");
				}

				return Console::CommandResult::fromOutputs(std::move(outputs), dumped);
			});
	}
}
