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
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <ranges>

/* Local inclusions. */
#include "Component/Camera.hpp"
#include "Component/Microphone.hpp"
#include "Component/ConsoleAdapter.hpp"
#include "Component/Visual.hpp"
#include "IO/IO.hpp"
#include "Graphics/Geometry/ResourceGenerator.hpp"
#include "Graphics/ImposterAtlas.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Graphics/Renderable/BasicGroundResource.hpp"
#include "GroundTriangles.hpp"
#include "Component/CharacterController.hpp"
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
#include "FastJSON.hpp"

namespace EmEn::Scenes
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief Writes a 3D vector as a JSON array: a non-finite component is written null (JSON has no NaN nor
		 * infinity), the others with the float round-trip precision (9 significant digits).
		 * @param output A reference to the stream.
		 * @param vector A reference to the vector.
		 * @return void
		 */
		void
		writeJSONVector (std::stringstream & output, const Math::Vector< 3, float > & vector) noexcept
		{
			output << '[';

			for ( size_t index = 0; index < 3; ++index )
			{
				if ( index > 0 )
				{
					output << ',';
				}

				if ( std::isfinite(vector[index]) )
				{
					output << std::setprecision(9) << vector[index];
				}
				else
				{
					output << "null";
				}
			}

			output << ']';
		}
	}

	void
	Manager::onRegisterToConsole () noexcept
	{
		/* Component control (owner decision 2026-09-27): one sub-object per component type, every command naming
		 * its entity and component — no targeting state. */
		Component::appendLightConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendCameraConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendEnvironmentConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendAnimationConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendPhysicsConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendAudioConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendVisualConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendBeamConsoleAdapters(*this, m_componentConsoleAdapters);
		Component::appendPathConsoleAdapters(*this, m_componentConsoleAdapters);

		for ( const auto & adapter : m_componentConsoleAdapters )
		{
			adapter->registerToObject(*this);
		}

		this->bindCommand("listEntities", "Lists the entities of the ACTIVE scene as JSON: every node (whole hierarchy, with its parent) and static entity, with its ADDRESS (what the component commands take as `entity`: the name when unique, else the shortest unique path suffix Parent/Child) and its components (name and type).", [this] () {
			auto result = Console::CommandResult::error("No active scene !");

			this->withExclusiveActiveScene([&result] (const std::shared_ptr< Scene > & scene) {
				Json::Value entities{Json::arrayValue};

				const auto addresses = Component::entityAddresses(*scene);

				const auto describe = [&addresses] (const AbstractEntity & entity, const char * kind) {
					Json::Value entry{Json::objectValue};
					entry["name"] = entity.name();
					entry["kind"] = kind;

					if ( const auto addressIt = addresses.find(&entity); addressIt != addresses.end() )
					{
						entry["address"] = addressIt->second;
					}

					Json::Value components{Json::arrayValue};

					entity.forEachComponent([&components] (const Component::Abstract & component) {
						Json::Value item{Json::objectValue};
						item["name"] = component.name();
						item["type"] = component.getComponentType();
						components.append(std::move(item));
					});

					entry["components"] = std::move(components);

					return entry;
				};

				/* Depth first from the root's children: the root itself is not an entity anyone names. */
				std::vector< std::shared_ptr< Node > > pending;

				for ( const auto & child : scene->root()->children() | std::views::values )
				{
					pending.emplace_back(child);
				}

				while ( !pending.empty() )
				{
					const auto node = pending.back();
					pending.pop_back();

					auto entry = describe(*node, "node");

					if ( const auto parent = node->parent(); parent != nullptr && !parent->isRoot() )
					{
						entry["parent"] = parent->name();
					}

					entities.append(std::move(entry));

					for ( const auto & child : node->children() | std::views::values )
					{
						pending.emplace_back(child);
					}
				}

				scene->forEachStaticEntities([&entities, &describe] (const StaticEntity & entity) {
					entities.append(describe(entity, "staticEntity"));
				});

				result = Console::CommandResult::json(FastJSON::stringify(entities));
			}, true);

			return result;
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("listEntityComponents", "Lists the components of one entity of the ACTIVE scene as JSON (name and type): the names the component commands take.",
			{
				{"entity", "The entity address: its name when unique, else the shortest unique path suffix Parent/Child (listEntities() gives every address)."}
			},
			[this] (const std::string & entityAddress) {
				auto result = Console::CommandResult::error("No active scene !");

				this->withExclusiveActiveScene([&result, &entityAddress] (const std::shared_ptr< Scene > & scene) {
					std::shared_ptr< AbstractEntity > entity;
					std::string error;

					if ( !Component::resolveEntity(*scene, entityAddress, entity, error) )
					{
						result = Console::CommandResult::error(error);

						return;
					}

					Json::Value components{Json::arrayValue};

					entity->forEachComponent([&components] (const Component::Abstract & component) {
						Json::Value item{Json::objectValue};
						item["name"] = component.name();
						item["type"] = component.getComponentType();
						components.append(std::move(item));
					});

					result = Console::CommandResult::json(FastJSON::stringify(components));
				}, true);

				return result;
			}, Console::CommandHint::ReadOnly);

		/* The selection outline's SET (one style for all): a command resolves the entity and applies one change. */
		const auto changeHighlight = [this] (const std::string & entityAddress, const auto & change) {
			auto result = Console::CommandResult::error("No active scene !");

			this->withExclusiveActiveScene([&result, &entityAddress, &change] (const std::shared_ptr< Scene > & scene) {
				std::shared_ptr< AbstractEntity > entity;
				std::string error;

				if ( !Component::resolveEntity(*scene, entityAddress, entity, error) )
				{
					result = Console::CommandResult::error(error);

					return;
				}

				result = change(*scene, entity);
			}, true);

			return result;
		};

		const Console::Parameter highlightEntityParameter{"entity", "The entity address: its name when unique, else the shortest unique path suffix Parent/Child (listEntities() gives every address)."};

		this->bindCommand("highlightEntity", "Outlines ONE entity of the ACTIVE scene (the selection outline: full where visible, dimmed where hidden), replacing the highlighted set.",
			{highlightEntityParameter},
			[changeHighlight] (const std::string & entityAddress) {
				return changeHighlight(entityAddress, [&entityAddress] (Scene & scene, const std::shared_ptr< AbstractEntity > & entity) {
					scene.setHighlightedEntity(entity);

					return Console::CommandResult::success("Entity '" + entityAddress + "' highlighted.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("addHighlight", "Adds an entity of the ACTIVE scene to the highlighted set (one outline style for the whole set).",
			{highlightEntityParameter},
			[changeHighlight] (const std::string & entityAddress) {
				return changeHighlight(entityAddress, [&entityAddress] (Scene & scene, const std::shared_ptr< AbstractEntity > & entity) {
					return scene.addHighlightedEntity(entity) ?
						Console::CommandResult::success("Entity '" + entityAddress + "' added to the highlight.") :
						Console::CommandResult::success("Entity '" + entityAddress + "' was already highlighted.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("removeHighlight", "Removes an entity of the ACTIVE scene from the highlighted set.",
			{highlightEntityParameter},
			[changeHighlight] (const std::string & entityAddress) {
				return changeHighlight(entityAddress, [&entityAddress] (Scene & scene, const std::shared_ptr< AbstractEntity > & entity) {
					return scene.removeHighlightedEntity(entity) ?
						Console::CommandResult::success("Entity '" + entityAddress + "' removed from the highlight.") :
						Console::CommandResult::success("Entity '" + entityAddress + "' was not highlighted.");
				});
			}, Console::CommandHint::Idempotent);

		this->bindCommand("clearHighlight", "Removes the selection outline of the ACTIVE scene (empties the highlighted set).", [this] () {
			auto result = Console::CommandResult::error("No active scene !");

			this->withExclusiveActiveScene([&result] (const std::shared_ptr< Scene > & scene) {
				scene->clearHighlightedEntities();

				result = Console::CommandResult::success("Highlight cleared.");
			}, true);

			return result;
		}, Console::CommandHint::Idempotent);

		this->bindCommand("getHighlights", "Returns the addresses of the highlighted entities of the ACTIVE scene, as a JSON array (in the order they were added).", [this] () {
			auto result = Console::CommandResult::error("No active scene !");

			this->withExclusiveActiveScene([&result] (const std::shared_ptr< Scene > & scene) {
				const auto addresses = Component::entityAddresses(*scene);

				Json::Value list{Json::arrayValue};

				for ( const auto & entity : scene->highlightedEntities() )
				{
					const auto found = addresses.find(entity.get());

					list.append(found != addresses.end() ? found->second : entity->name());
				}

				result = Console::CommandResult::json(FastJSON::stringify(list));
			}, true);

			return result;
		}, Console::CommandHint::ReadOnly);

		this->bindCommand("setHighlightStyle", "Sets the look of the selection outline (engine-wide).",
			{
				{"red", "Displayed (sRGB) red, 0-1."},
				{"green", "Displayed (sRGB) green, 0-1."},
				{"blue", "Displayed (sRGB) blue, 0-1."},
				{"width", "The line width, in pixels (1-8)."},
				{"hiddenOpacity", "The opacity where the entity is hidden behind other geometry, 0-1 (0 = visible parts only, 1 = x-ray)."}
			},
			[this] (float red, float green, float blue, float width, float hiddenOpacity) {
				auto & outline = m_resourceManager.graphicsRenderer().selectionOutline();

				outline.setColor(Base::PixelFactory::Color< float >{red, green, blue, 1.0F});
				outline.setWidth(width);
				outline.setHiddenOpacity(hiddenOpacity);

				return Console::CommandResult::success("Highlight style set.");
			}, Console::CommandHint::Idempotent);

		this->bindCommand("createScene", "Creates a scene with a camera + microphone node and a neutral ambient light (5000 lx), then enables it.",
			{
				{"name", "The scene name (must not exist yet)."},
				{"boundary", "The scene half-size, in metres (the ground, if any, spans twice this)."},
				{"cameraNode", "The name of the node that carries the primary camera and microphone."},
				{"camX", "Camera world X, in metres."},
				{"camY", "Camera world Y, in metres (UP is +Y)."},
				{"camZ", "Camera world Z, in metres."},
				{"backgroundName", "A skybox resource name (listResources(SkyBoxResource)); an unknown name is an error; no background when omitted."},
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

					/* ⚠️ Checked BEFORE the scene exists (owner decision 2026-09-27): getResource() silently
					 * answers the "Default" skybox for an unknown name, so a typo used to build a scene with
					 * the wrong sky and report success. */
					if ( skyBoxContainer == nullptr || !skyBoxContainer->isResourceExists(*backgroundName) )
					{
						return Console::CommandResult::error("Unknown skybox '" + *backgroundName + "' (listResources(SkyBoxResource) lists them). No scene was created.");
					}

					background = skyBoxContainer->getResource(*backgroundName);
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

		this->bindCommand("getGroundLevel", "Returns the ground height of the active scene at a world X/Z as JSON: 'position' from the bilinear height field (getLevelAt()), 'surface' the exact height of the rendered triangle there (what the physics collides; null outside the ground), and the ground normal.",
			{
				{"x", "The world X coordinate."},
				{"z", "The world Z coordinate."}
			},
			[this] (float positionX, float positionZ) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				const auto groundLevel = m_activeScene->groundLevel();

				if ( groundLevel == nullptr )
				{
					return Console::CommandResult::error("The active scene has no ground !");
				}

				if ( !std::isfinite(positionX) || !std::isfinite(positionZ) )
				{
					return Console::CommandResult::error("The coordinates must be finite !");
				}

				const Math::Vector< 3, float > position{positionX, 0.0F, positionZ};
				const auto level = groundLevel->getLevelAt(position);

				/* The exact surface: the rendered triangle over the point. */
				class SurfaceProbe final : public GroundTriangleVisitor
				{
					public:

						SurfaceProbe (float x, float z) noexcept : m_x{x}, m_z{z} { }

						void
						onTriangle (const Math::Space3D::Triangle< float > & triangle, uint32_t /*featureId*/) noexcept override
						{
							if ( !m_found )
							{
								m_found = surfaceHeightOver(triangle, m_x, m_z, m_height);
							}
						}

						[[nodiscard]]
						bool found () const noexcept { return m_found; }

						[[nodiscard]]
						float height () const noexcept { return m_height; }

					private:

						float m_x;
						float m_z;
						float m_height{0.0F};
						bool m_found{false};
				};

				SurfaceProbe probe{positionX, positionZ};
				constexpr float ProbeReach{0.01F};
				static_cast< void >(groundLevel->visitTriangles(Math::Space3D::AACuboid< float >{Math::Vector< 3, float >{positionX + ProbeReach, 0.0F, positionZ + ProbeReach}, Math::Vector< 3, float >{positionX - ProbeReach, 0.0F, positionZ - ProbeReach}}, probe));

				std::stringstream info;
				info << R"({"position":)";
				writeJSONVector(info, Math::Vector< 3, float >{positionX, level, positionZ});
				info << R"(,"surface":)";

				if ( probe.found() && std::isfinite(probe.height()) )
				{
					info << std::setprecision(9) << probe.height();
				}
				else
				{
					info << "null";
				}

				info << R"(,"normal":)";
				writeJSONVector(info, groundLevel->getNormalAt(position));
				info << '}';

				return Console::CommandResult::json(info.str());
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

				if ( !EmEn::Base::IO::exists(filepath) )
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

				/* The requested scale is the AUTHOR's: it goes on the entity frame. It used to be written into the
				 * SHARED resource's uniform scale (every instance of the mesh, and its definition's own unit lost)
				 * and into the instance matrix; since the instance draws the mesh's unit (2026-09-28,
				 * RenderableInstance::Abstract::applyLocalTransformation()), that doubled it. */
				const Base::Math::Vector< 3, float > position{x, y, z};
				auto entity = m_activeScene->createStaticEntity(entityName, Base::Math::CartesianFrame< float >{position, scale});

				if ( entity == nullptr )
				{
					return Console::CommandResult::error("Failed to create entity '" + entityName + "' !");
				}

				const auto visual = entity->componentBuilder< Component::Visual >(entityName + "Visual").build(mesh, Graphics::RenderableInstance::Lighting::Lit);

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
				info << R"("address":")" << node.get() << "\",";
				info << "\"position\":";
				writeJSONVector(info, pos);
				info << ",";
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

		this->bindCommand("getNodePhysics", "Returns the physics state of a root-level node of the active scene as JSON: world/local position, world orientation (upward and backward vectors), linear and angular velocity, movable, simulation paused (asleep), grounded and WHICH surface it rests on (Ground/Boundary/Entity), and the scene's physics cycle the state belongs to.",
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
				const auto worldCoordinates = node->getWorldCoordinates();
				const auto worldPosition = worldCoordinates.position();
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
				info << "\"worldPosition\":";
				writeJSONVector(info, worldPosition);
				info << ",\"localPosition\":";
				writeJSONVector(info, localPosition);
				info << ",\"worldUpward\":";
				writeJSONVector(info, worldCoordinates.upwardVector());
				info << ",\"worldBackward\":";
				writeJSONVector(info, worldCoordinates.backwardVector());
				info << ",\"linearVelocity\":";
				writeJSONVector(info, velocity);
				info << ",\"angularVelocity\":";
				writeJSONVector(info, node->angularVelocity());
				info << ",";
				info << "\"movable\":" << (node->isMovable() ? "true" : "false") << ",";
				info << "\"simulationPaused\":" << (node->isSimulationPaused() ? "true" : "false") << ",";
				info << "\"grounded\":" << (node->isGrounded() ? "true" : "false") << ",";
				info << R"("groundedSource":")" << groundedSource << "\",";
				/* The physics cycle this state belongs to: a client polling at its own rate samples by cycle,
				 * never by wall-clock time (the physics ticks at a fixed WorldPhysicsUpdateFrequency). */
				info << "\"sceneCycle\":" << m_activeScene->cycle();

				/* A kinematic character (P4): its controller's state (its node velocity is not what it does). */
				if ( const auto character = node->characterController(); character != nullptr )
				{
					const auto & controller = character->controller();

					info << R"(,"character":{"grounded":)" << (controller.isGrounded() ? "true" : "false");
					info << R"(,"velocity":)";
					writeJSONVector(info, controller.velocity());
					info << R"(,"groundNormal":)";
					writeJSONVector(info, controller.groundNormal());
					info << R"(,"supportKey":)" << controller.supportKey() << '}';
				}

				info << "}";

				return Console::CommandResult::json(info.str());
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("setCharacterVelocity", "Sets the velocity a root-level node's kinematic character controller wants to move at (world, m/s; its vertical part is ignored).",
			{
				{"name", "The node name."},
				{"x", "The world X velocity (m/s)."},
				{"y", "The world Y velocity (ignored: the controller handles gravity and jumps)."},
				{"z", "The world Z velocity (m/s)."}
			},
			[this] (const std::string & name, float velocityX, float velocityY, float velocityZ) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				const auto node = m_activeScene->root()->findChild(name);

				if ( node == nullptr )
				{
					return Console::CommandResult::error("Node '" + name + "' not found !");
				}

				const auto character = node->characterController();

				if ( character == nullptr )
				{
					return Console::CommandResult::error("Node '" + name + "' has no character controller !");
				}

				if ( !character->controller().setWantedVelocity({velocityX, velocityY, velocityZ}) )
				{
					return Console::CommandResult::error("The velocity must be finite !");
				}

				return Console::CommandResult::success("Character '" + name + "' wants to move.");
			});

		this->bindCommand("characterJump", "Makes a root-level node's kinematic character controller jump (only when it stands on the ground).",
			{
				{"name", "The node name."},
				{"speed", "The launch speed (m/s), > 0."}
			},
			[this] (const std::string & name, float speed) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				const auto node = m_activeScene->root()->findChild(name);

				if ( node == nullptr )
				{
					return Console::CommandResult::error("Node '" + name + "' not found !");
				}

				const auto character = node->characterController();

				if ( character == nullptr )
				{
					return Console::CommandResult::error("Node '" + name + "' has no character controller !");
				}

				if ( !character->controller().jump(speed) )
				{
					return Console::CommandResult::error("The speed must be finite and > 0 !");
				}

				return Console::CommandResult::success("Character '" + name + "' jumps.");
			});

		this->bindCommand("recordNodePhysics", "Records the physics state of root-level nodes of the active scene on EVERY logic cycle of a range, on the logic thread (after the collisions): the same cycles for every run, no sampling gap. Then getPhysicsRecordingStatus() until Complete, and savePhysicsRecording().",
			{
				{"nodeNames", "The root node names, comma-separated, as ONE quoted argument (\"A,B,C\"), 1 to 64."},
				{"firstCycle", "The first scene cycle to record (a cycle already past starts at once)."},
				{"cycleCount", "The number of cycles to record (1 to 36000, 10 min at 60 Hz)."}
			},
			[this] (const std::string & nodeNames, int32_t firstCycle, int32_t cycleCount) {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				if ( firstCycle < 0 || cycleCount <= 0 )
				{
					return Console::CommandResult::error("firstCycle must be >= 0 and cycleCount > 0 !");
				}

				std::vector< std::string > names;

				for ( auto name : String::explode(nodeNames, ',', false) )
				{
					name = String::trim(name);

					if ( !name.empty() )
					{
						names.emplace_back(std::move(name));
					}
				}

				std::string error;

				if ( !m_activeScene->physicsRecorder().start(std::move(names), static_cast< size_t >(firstCycle), static_cast< size_t >(cycleCount), error) )
				{
					return Console::CommandResult::error(error);
				}

				return Console::CommandResult::json(m_activeScene->physicsRecorder().status());
			});

		this->bindCommand("getPhysicsRecordingStatus", "Returns the state of the active scene's physics recording as JSON: state (Idle, Waiting, Recording, Complete), firstCycle, cycleCount, recordedCycles, nodes.",
			[this] () {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				return Console::CommandResult::json(m_activeScene->physicsRecorder().status());
			}, Console::CommandHint::ReadOnly);

		this->bindCommand("stopPhysicsRecording", "Stops the active scene's physics recording; the cycles recorded so far can still be saved.",
			[this] () {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				m_activeScene->physicsRecorder().stop();

				return Console::CommandResult::json(m_activeScene->physicsRecorder().status());
			});

		this->bindCommand("savePhysicsRecording", "Writes the completed physics recording of the active scene to a JSON file in the captures directory, releases it, and answers the file path.",
			[this] () {
				if ( m_activeScene == nullptr )
				{
					return Console::CommandResult::error("No active scene !");
				}

				const auto captureDirectory = m_activeScene->AVConsoleManager().graphicsRenderer().primaryServices().fileSystem().userDataDirectory("captures");
				const auto seconds = std::chrono::duration_cast< std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();

				auto filepath = captureDirectory / ("physics-recording-" + std::to_string(seconds) + ".json");

				/* A unique name: two saves in one second must not overwrite each other. */
				std::error_code existsError;

				for ( int suffix = 1; std::filesystem::exists(filepath, existsError) && suffix < 1000; ++suffix )
				{
					filepath = captureDirectory / ("physics-recording-" + std::to_string(seconds) + "-" + std::to_string(suffix) + ".json");
				}

				std::string error;

				if ( !m_activeScene->physicsRecorder().write(filepath, error) )
				{
					return Console::CommandResult::error(error);
				}

				return Console::CommandResult::info(filepath.string());
			});

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
