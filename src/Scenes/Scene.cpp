/*
 * src/Scenes/Scene.cpp
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

#include "Scene.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdlib>
#include <algorithm>
#include <ranges>

/* Local inclusions. */
#include "Input/Manager.hpp"
#include "Graphics/Renderer.hpp"
#include "Vulkan/SwapChain.hpp" // FIXME: Should not be there
#include "NodeCrawler.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;
	using namespace Graphics;
	using namespace Saphir;
	using namespace Physics;

	const size_t Scene::ClassUID{getClassUID(ClassId)};

	Scene::Scene (Renderer & graphicsRenderer, Audio::Manager & audioManager, const std::string & name, float boundary, const std::shared_ptr< Renderable::AbstractBackground > & background, const std::shared_ptr< Renderable::SceneAreaInterface > & sceneArea, const std::shared_ptr< Renderable::SeaLevelInterface > & seaLevel, const SceneOctreeOptions & octreeOptions) noexcept
		: NameableTrait(name),
		m_graphicsRenderer(graphicsRenderer),
		m_audioManager(audioManager),
		m_AVConsoleManager(name),
		m_background(background),
		m_sceneArea(sceneArea),
		m_seaLevel(seaLevel),
		m_rootNode(std::make_shared<Node>()),
		m_boundary(boundary)
	{
		this->observe(&m_AVConsoleManager);
		this->observe(m_rootNode.get());

		this->buildOctrees(octreeOptions);
	}

	Scene::~Scene ()
	{
		this->destroy();
	}

	void
	Scene::registerSceneVisualComponents () noexcept
	{
		if ( m_background != nullptr )
		{
			m_sceneVisualComponents[0] = std::make_shared< Component::Visual >("Background", *m_rootNode, m_background);

			/* NOTE: Disables lighting model on the background.
			 * TODO: Check to disable at construct time. */
			const auto renderableInstance = m_sceneVisualComponents[0]->getRenderableInstance();
			renderableInstance->setUseInfinityView(true);
			renderableInstance->disableDepthTest(true);
			renderableInstance->disableDepthWrite(true);

			this->checkRenderableInstance(renderableInstance);
		}

		if ( m_sceneArea != nullptr )
		{
			m_sceneVisualComponents[1] = std::make_shared< Component::Visual >("SceneArea", *m_rootNode, m_sceneArea);

			const auto renderableInstance = m_sceneVisualComponents[1]->getRenderableInstance();
			renderableInstance->enableLighting();
			renderableInstance->disableLightDistanceCheck();
			renderableInstance->enableDisplayTBNSpace(false);

			this->checkRenderableInstance(renderableInstance);
		}

		if ( m_seaLevel != nullptr )
		{
			m_sceneVisualComponents[2] = std::make_shared< Component::Visual >("SeaLevel", *m_rootNode, m_seaLevel);

			const auto renderableInstance = m_sceneVisualComponents[2]->getRenderableInstance();
			renderableInstance->enableLighting();
			renderableInstance->disableLightDistanceCheck();
			renderableInstance->enableDisplayTBNSpace(false);

			this->checkRenderableInstance(renderableInstance);
		}
	}

	bool
	Scene::initializeBaseComponents () const noexcept
	{
		auto hasCamera = false;
		auto hasMicrophone = false;

		{
			NodeCrawler< const Node > crawler{m_rootNode};

			std::shared_ptr< const Node > currentNode;

			while ( (currentNode = crawler.nextNode()) != nullptr )
			{
				for ( const auto & component: currentNode->components() | std::views::values )
				{
					if ( component->isComponent(Component::Camera::ClassId) )
					{
						hasCamera = true;
					}
					else if ( component->isComponent(Component::Microphone::ClassId) )
					{
						hasMicrophone = true;
					}
				}

				/* Stop looking in the node tree if at least
				 * one camera and one microphone are found. */
				if ( hasCamera && hasMicrophone )
				{
					return true;
				}
			}
		}

		if ( !hasCamera )
		{
			Tracer::warning(ClassId, "There is no camera in the scene ! Creating a default camera ...");

			const std::string name{"DefaultCamera"};

			if ( m_rootNode->createChild(name + "Node", m_lifetimeMS)->newCamera(true, true, name) == nullptr )
			{
				Tracer::error(ClassId, "Scene initialization error : Unable to create a default camera !");

				return false;
			}
		}

		if ( !hasMicrophone )
		{
			Tracer::warning(ClassId, "There is no microphone in the scene ! Creating a default microphone ...");

			const std::string name{"DefaultMicrophone"};

			if ( m_rootNode->createChild(name + "Node", m_lifetimeMS)->newMicrophone(true, name) == nullptr )
			{
				Tracer::error(ClassId, "Scene initialization error : Unable to create a default microphone !");

				return false;
			}
		}

		/* Set audio properties for this scene. */
		m_audioManager.setSoundEnvironmentProperties(m_soundEnvironmentProperties);

		return true;
	}

	bool
	Scene::initialize (Settings & settings) noexcept
	{
		if ( !m_flags[Initialized] )
		{
			this->registerSceneVisualComponents();

			/* Create a missing camera and/or microphone. */
			if ( !this->initializeBaseComponents() )
			{
				return false;
			}

			const auto & swapChain = m_graphicsRenderer.swapChain();

			if ( swapChain != nullptr )
			{
				m_AVConsoleManager.addVideoDevice(swapChain, true);
			}

			if ( !m_AVConsoleManager.autoConnectPrimaryVideoDevices(m_graphicsRenderer, settings) )
			{
				TraceError{ClassId} << "Unable to auto-connect primary video devices !";

				return false;
			}

			if ( !m_AVConsoleManager.autoConnectPrimaryAudioDevices(m_audioManager, settings) )
			{
				TraceError{ClassId} << "Unable to auto-connect primary audio devices !";

				return false;
			}

			TraceInfo{ClassId} << m_AVConsoleManager.getConnexionStates();

			if ( !m_lightSet.initialize(m_graphicsRenderer, this->name()) )
			{
				TraceError{ClassId} << "Unable to initialize the light set !";

				return false;
			}

			m_flags[Initialized] = true;
		}

		/* FIXME: When re-enabling, the swap-chain do not have the correct ambient light parameters ! */

		Input::Manager::instance()->addKeyboardListener(&m_nodeController);

		return true;
	}

	void
	Scene::shutdown () noexcept
	{
		/* FIXME: Find a better way to stop the node controller ! */
		m_nodeController.releaseNode();
		m_nodeController.disconnectDevice();

		Input::Manager::instance()->removeKeyboardListener(&m_nodeController);
	}

	std::shared_ptr< StaticEntity >
	Scene::createStaticEntity (const std::string & name, const CartesianFrame< float > & coordinates) noexcept
	{
		auto staticEntity = std::make_shared< StaticEntity >(name, m_lifetimeMS, coordinates);

		m_staticEntities.emplace(name, staticEntity);

		this->observe(staticEntity.get());

		return staticEntity;
	}

	bool
	Scene::removeStaticEntity (const std::string & name) noexcept
	{
		/* Fist, check the presence of the entity in the list. */
		const auto staticEntityIt = m_staticEntities.find(name);

		if ( staticEntityIt == m_staticEntities.end() )
		{
			return false;
		}

		const auto staticEntity = staticEntityIt->second;

		this->forget(staticEntity.get());

		if ( m_renderingOctree != nullptr && staticEntity->isRenderable() )
		{
			const std::lock_guard< std::mutex > lockGuard{m_renderingOctreeMutex};

			m_renderingOctree->erase(staticEntity);
		}

		if ( m_physicsOctree != nullptr )
		{
			const std::lock_guard< std::mutex > lockGuard{m_physicsOctreeMutex};

			m_physicsOctree->erase(staticEntity);
		}

		staticEntity->clearComponents();

		m_staticEntities.erase(staticEntityIt);

		return true;
	}

	void
	Scene::destroy () noexcept
	{
		m_environmentEffects.clear();

		/* NOTE: Remove the scene global visuals */
		{
			for ( auto & visual : m_sceneVisualComponents )
			{
				visual.reset();
			}

			m_seaLevel.reset();
			m_sceneArea.reset();
			m_background.reset();
		}

		m_modifiers.clear();

		/* FIXME: Find a better solution ! */
		{
			m_lightSet.removeAllLights();
			m_lightSet.terminate(*Renderer::instance());
		}

		this->destroyOctrees();
		m_nodeController.releaseNode();
		this->resetNodeTree();
		m_staticEntities.clear();
		m_AVConsoleManager.clear();
	}

	bool
	Scene::buildOctrees (const SceneOctreeOptions & octreeOptions) noexcept
	{
		if ( m_boundary <= 0.0F )
		{
			Tracer::error(ClassId, "The scene boundary is null ! Unable to create an octree root sector !");

			return false;
		}

		if ( m_renderingOctree == nullptr )
		{
			m_renderingOctree = std::make_shared< OctreeSector< AbstractEntity, false > >(
				Vector< 3, float >{m_boundary, m_boundary, m_boundary},
				Vector< 3, float >{-m_boundary, -m_boundary, -m_boundary},
				octreeOptions.renderingOctreeAutoExpandAt,
				false
			);

			if ( octreeOptions.renderingOctreeReserve > 0 )
			{
				m_renderingOctree->reserve(octreeOptions.renderingOctreeReserve);
			}
		}
		else
		{
			TraceWarning{ClassId} << "The rendering octree already exists !";
		}

		if ( m_physicsOctree == nullptr )
		{
			m_physicsOctree = std::make_shared< OctreeSector< AbstractEntity, true > >(
				Vector< 3, float >{m_boundary, m_boundary, m_boundary},
				Vector< 3, float >{-m_boundary, -m_boundary, -m_boundary},
				octreeOptions.physicsOctreeAutoExpandAt,
				false
			);

			if ( octreeOptions.physicsOctreeReserve > 0 )
			{
				m_physicsOctree->reserve(octreeOptions.physicsOctreeReserve);
			}
		}
		else
		{
			TraceWarning{ClassId} << "The physics octree already exists !";
		}

		return true;
	}

	void
	Scene::destroyOctrees () noexcept
	{
		if ( m_renderingOctree != nullptr )
		{
			const std::lock_guard< std::mutex > lock{m_renderingOctreeMutex};

			m_renderingOctree.reset();
		}

		if ( m_physicsOctree != nullptr )
		{
			const std::lock_guard< std::mutex > lock{m_physicsOctreeMutex};

			m_physicsOctree.reset();
		}
	}

	bool
	Scene::rebuildRenderingOctree (bool keepElements) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderingOctreeMutex};

		if ( m_boundary <= 0.0F )
		{
			Tracer::error(ClassId, "The scene boundary is null ! Unable to rebuild an octree !");

			return false;
		}

		/* Allocate a new octree. */
		const auto newOctree = std::make_shared< OctreeSector< AbstractEntity, false > >(
			Vector< 3, float >{m_boundary, m_boundary, m_boundary},
			Vector< 3, float >{-m_boundary, -m_boundary, -m_boundary},
				m_renderingOctree->maxElementPerSector(),
			m_renderingOctree->autoCollapseEnabled()
		);

		/* Transfer all elements from the previous oldOctree (only the root sector) to the new one. */
		if ( keepElements )
		{
			for ( const auto & element : m_renderingOctree->elements() )
			{
				newOctree->insert(element);
			}
		}

		m_renderingOctree.reset();
		m_renderingOctree = newOctree;

		return true;
	}

	bool
	Scene::rebuildPhysicsOctree (bool keepElements) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_physicsOctreeMutex};

		if ( m_boundary <= 0.0F )
		{
			Tracer::error(ClassId, "The scene boundary is null ! Unable to rebuild an octree !");

			return false;
		}

		/* Allocate a new octree. */
		const auto newOctree = std::make_shared< OctreeSector< AbstractEntity, true > >(
			Vector< 3, float >{m_boundary, m_boundary, m_boundary},
			Vector< 3, float >{-m_boundary, -m_boundary, -m_boundary},
			m_physicsOctree->maxElementPerSector(),
			m_physicsOctree->autoCollapseEnabled()
		);

		/* Transfer all elements from the previous oldOctree (only the root sector) to the new one. */
		if ( keepElements )
		{
			for ( const auto & element : m_physicsOctree->elements() )
			{
				newOctree->insert(element);
			}
		}

		m_physicsOctree.reset();
		m_physicsOctree = newOctree;

		return true;
	}

	void
	Scene::checkEntityLocationInOctrees (const std::shared_ptr< AbstractEntity > & entity) const noexcept
	{
		/* Check the entity in the rendering octree. */
		if ( m_renderingOctree != nullptr && entity->isRenderable() )
		{
			const std::lock_guard< std::mutex > lockGuard{m_renderingOctreeMutex};

			if ( m_renderingOctree->contains(entity) )
			{
				m_renderingOctree->update(entity);
			}
			else
			{
				m_renderingOctree->insert(entity);
			}
		}

		/* Check the entity in the physics octree. */
		if ( m_physicsOctree != nullptr && entity->isDeflector() )
		{
			const std::lock_guard< std::mutex > lockGuard{m_physicsOctreeMutex};

			if ( m_physicsOctree->contains(entity) )
			{
				m_physicsOctree->update(entity);
			}
			else
			{
				m_physicsOctree->insert(entity);
			}
		}
	}

	void
	Scene::resetNodeTree () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_sceneNodesMutex};

		m_rootNode->destroyTree();
	}

	void
	Scene::processLogics (size_t engineCycle) noexcept
	{
		m_lifetimeUS += EngineUpdateCycleDurationUS< uint64_t >;
		m_lifetimeMS += EngineUpdateCycleDurationMS< uint32_t >;

		m_nodeController.update();

		/* Update scene static entities logics. */
		{
			const std::lock_guard< std::mutex > lock{m_staticEntitiesMutex};

			for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
			{
				if ( staticEntity->processLogics(*this, engineCycle) )
				{
					this->checkEntityLocationInOctrees(staticEntity);
				}
			}
		}

		/* Update scene nodes logics. */
		{
			const std::lock_guard< std::mutex > lock{m_sceneNodesMutex};

			NodeCrawler< Node > crawler(m_rootNode);

			std::shared_ptr< Node > currentNode{};

			while ( (currentNode = crawler.nextNode()) != nullptr )
			{
				if ( currentNode->processLogics(*this, engineCycle) )
				{
					this->checkEntityLocationInOctrees(currentNode);
				}
			}

			/* Clean all dead nodes. */
			m_rootNode->trimTree();
		}

		/* Launch the collision test step. */
		if ( m_physicsOctree != nullptr )
		{
			this->sectorCollisionTest(*m_physicsOctree);

			/* Final collisions check against scene boundaries and ground,
			 * then resolve all collisions detected on movable entities. */
			for ( const auto & entity : m_physicsOctree->elements() )
			{
				auto * movableEntity = entity->getMovableTrait();

				if ( movableEntity == nullptr )
				{
					continue;
				}

				/* Check collision against scene boundaries. */
				if ( !entity->isSimulationPaused() )
				{
					if ( entity->sphereCollisionIsEnabled() )
					{
						this->clipWithBoundingSphere(entity);
					}
					else
					{
						this->clipWithBoundingBox(entity);
					}
				}

				/* Resolve accumulated collisions from the entity collider. */
				if ( auto & collider = movableEntity->collider(); collider.hasCollisions() )
				{
					/* NOTE: Collisions resolution can resume the physics simulation. */
					collider.resolveCollisions(*entity);
				}

				/* This movable entity will never check for a simulation pause. */
				if ( movableEntity->alwaysComputePhysics() )
				{
					continue;
				}

				/* Check for entity inertia to pause the simulation. */
				if ( !entity->isSimulationPaused() && (!movableEntity->isMovable() || movableEntity->checkSimulationInertia()) )
				{
					entity->pauseSimulation(true);
				}
			}
		}

		m_cycle++;
	}

	void
	Scene::applyModifiers (Node & node) const noexcept
	{
		for ( const auto & modifier : m_modifiers )
		{
			/* NOTE: Avoid working on the same Node. */
			if ( &node == &modifier->parentEntity() )
			{
				continue;
			}

			/* FIXME: Use AABB when usable */
			const auto modifierForce = modifier->getForceAppliedToEntity(node.getWorldCoordinates(), node.getWorldBoundingSphere());

			node.addForce(modifierForce);
		}
	}

	void
	Scene::sectorCollisionTest (const OctreeSector< AbstractEntity, true > & sector) noexcept
	{
		/* No element present. */
		if ( sector.empty() )
		{
			return;
		}

		/* If the sector is not a leaf, we test subsectors. */
		if ( !sector.isLeaf() )
		{
			//#pragma omp parallel for
			for ( const auto & subSector : sector.subSectors() )
			{
				this->sectorCollisionTest(*subSector);
			}

			return;
		}

		/* We are in a leaf, we check scene nodes present here. */
		const auto & elements = sector.elements();

		for ( auto elementIt = elements.begin(); elementIt != elements.end(); ++elementIt )
		{
			/* NOTE: The entity A can be a node or a static entity. */
			const auto & entityA = *elementIt;
			const bool entityAHasMovableAbility = entityA->hasMovableAbility();

			/* Copy the iterator to iterate through next elements with it without modify the initial one. */
			auto elementItCopy = elementIt;

			for ( ++elementItCopy; elementItCopy != elements.end(); ++elementItCopy )
			{
				/* NOTE: The entity B can also be a node or a static entity. */
				const auto & entityB = *elementItCopy;
				const bool entityBHasMovableAbility = entityB->hasMovableAbility();

				/* Both entities are static or both entities are paused. */
				if ( (!entityAHasMovableAbility && !entityBHasMovableAbility) || (entityA->isSimulationPaused() && entityB->isSimulationPaused()) )
				{
					continue;
				}

				if ( entityAHasMovableAbility )
				{
					auto & colliderA = entityA->getMovableTrait()->collider();

					/* Check for cross-sector collisions duplicates. */
					if ( colliderA.hasCollisionWith(*entityB) )
					{
						continue;
					}

					/* NOTE: Here the entity A is movable.
					 * We will check the collision from entity A. */
					if ( entityBHasMovableAbility )
					{
						colliderA.checkCollisionAgainstMovable(*entityA, *entityB);
					}
					else
					{
						if ( entityA->isSimulationPaused() )
						{
							continue;
						}

						colliderA.checkCollisionAgainstStatic(*entityA, *entityB);
					}
				}
				else
				{
					if ( entityB->isSimulationPaused() )
					{
						continue;
					}

					auto & colliderB = entityB->getMovableTrait()->collider();

					/* Check for cross-sector collisions duplicates. */
					if ( colliderB.hasCollisionWith(*entityA) )
					{
						continue;
					}

					/* NOTE: Here the entity A is static, and B cannot be static.
					 * We will check the collision from entity B. */
					colliderB.checkCollisionAgainstStatic(*entityB, *entityA);
				}
			}
		}
	}

	void
	Scene::clipWithBoundingSphere (const std::shared_ptr< AbstractEntity > & entity) const noexcept
	{
		const auto worldCoordinates = entity->getWorldCoordinates();
		const auto & worldPosition = worldCoordinates.position();

		auto & collider = entity->getMovableTrait()->collider();

		if ( m_sceneArea != nullptr )
		{
			const auto groundLevel = m_sceneArea->getLevelAt(worldPosition) - entity->getWorldBoundingSphere().radius();

			if ( worldPosition[Y] >= groundLevel )
			{
				entity->setYPosition(groundLevel, TransformSpace::World);

				collider.addCollision(CollisionType::SceneGround, nullptr, worldPosition, m_sceneArea->getNormalAt(worldPosition));
			}
		}

		/* Compute the max boundary. */
		const auto boundaryLimit = m_boundary - entity->getWorldBoundingSphere().radius();

		/* X-Axis test. */
		if ( std::abs(worldPosition[X]) > boundaryLimit )
		{
			if ( worldPosition[X] > boundaryLimit )
			{
				entity->setXPosition(boundaryLimit, TransformSpace::World);

				collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::negativeX());
			}
			else
			{
				entity->setXPosition(-boundaryLimit, TransformSpace::World);

				collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::positiveX());
			}
		}

		/* Y-Axis test. */
		if ( std::abs(worldPosition[Y]) > boundaryLimit )
		{
			if ( worldPosition[Y] > boundaryLimit )
			{
				entity->setYPosition(boundaryLimit, TransformSpace::World);

				collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::negativeY());
			}
			else
			{
				entity->setYPosition(-boundaryLimit, TransformSpace::World);

				collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::positiveY());
			}
		}

		/* Z-Axis test. */
		if ( std::abs(worldPosition[Z]) > boundaryLimit )
		{
			if ( worldPosition[Z] > boundaryLimit )
			{
				entity->setZPosition(boundaryLimit, TransformSpace::World);

				collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::negativeZ());
			}
			else
			{
				entity->setZPosition(-boundaryLimit, TransformSpace::World);

				collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::positiveZ());
			}
		}
	}

	void
	Scene::clipWithBoundingBox (const std::shared_ptr< AbstractEntity > & entity) const noexcept
	{
		const auto worldCoordinates = entity->getWorldCoordinates();
		const auto & worldPosition = worldCoordinates.position();
		const auto AABB = entity->getWorldBoundingBox();

		auto & collider = entity->getMovableTrait()->collider();

		/* Running the subtest first. */
		if ( m_sceneArea != nullptr )
		{
			/* Gets the four points of the box bottom. */
			const std::array< Vector< 3, float >, 4 > points{
				AABB.bottomSouthEast(),
				AABB.bottomSouthWest(),
				AABB.bottomNorthWest(),
				AABB.bottomNorthEast()
			};

			/* These will keep the deepest collision of the four points. */
			const Vector< 3, float > * collisionPosition = nullptr;
			auto highestDistance = 0.0F;

			for ( const auto & position : points )
			{
				const auto groundLevel = m_sceneArea->getLevelAt(position);
				const auto distance = groundLevel - position[Y];

				if ( distance <= highestDistance )
				{
					collisionPosition = &position;
					highestDistance = distance;
				}
			}

			if ( collisionPosition != nullptr )
			{
				entity->moveY(highestDistance, TransformSpace::World);

				collider.addCollision(CollisionType::SceneGround, nullptr, *collisionPosition, m_sceneArea->getNormalAt(*collisionPosition));
			}
		}

		/* X-Axis test */
		if ( AABB.maximum(X) > m_boundary )
		{
			const auto delta = AABB.maximum(X) - m_boundary;

			entity->moveX(-delta, TransformSpace::World);

			collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::negativeX());
		}
		else if ( AABB.minimum(X) < -m_boundary )
		{
			const auto delta = std::abs(AABB.minimum(X)) - m_boundary;

			entity->moveX(delta, TransformSpace::World);

			collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::positiveX());
		}

		/* Y-Axis test */
		if ( AABB.maximum(Y) > m_boundary )
		{
			const auto delta = AABB.maximum(Y) - m_boundary;

			entity->moveY(-delta, TransformSpace::World);

			collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::negativeY());
		}
		else if ( AABB.minimum(Y) <= -m_boundary )
		{
			const auto delta = std::abs(AABB.minimum(Y)) - m_boundary;

			entity->moveY(delta, TransformSpace::World);

			collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::positiveY());
		}

		/* Z-Axis test */
		if ( AABB.maximum(Z) > m_boundary )
		{
			const auto delta = AABB.maximum(Z) - m_boundary;

			entity->moveZ(-delta, TransformSpace::World);

			collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::negativeZ());
		}
		else if ( AABB.minimum(Z) < -m_boundary )
		{
			const auto delta = std::abs(AABB.minimum(Z)) - m_boundary;

			entity->moveZ(delta, TransformSpace::World);

			collider.addCollision(CollisionType::SceneBoundary, nullptr, worldPosition, Vector< 3, float >::positiveZ());
		}
	}

	void
	Scene::insertInShadowCastLists (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, float distance) noexcept
	{
		/* Check whether the renderable instance is ready to cast shadows. */
		if ( !renderableInstance->isReadyToCastShadows(renderTarget) )
		{
			return;
		}

		/* This is a raw pointer to the renderable interface. */
		const auto * renderable = renderableInstance->renderable();

		if constexpr ( IsDebug )
		{
			if ( renderable == nullptr )
			{
				Tracer::error(ClassId, "The renderable interface pointer is a null !");

				return;
			}

			/* NOTE: Check whether the renderable is ready to draw.
			 * Only done in debug mode because a renderable instance ready to
			 * render implies the renderable is ready to draw. */
			if ( !renderable->isReadyForInstantiation() )
			{
				Tracer::error(ClassId, "The renderable interface is not ready !");

				return;
			}
		}

		const auto layerCount = renderable->layerCount();

		for ( size_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
		{
			RenderBatch::create(m_renderLists[Shadows], distance, renderableInstance, layerIndex);
		}
	}

	void
	Scene::insertInRenderLists (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, float distance) noexcept
	{
		/* Check whether the renderable instance is ready to render. */
		if ( !renderableInstance->isReadyToRender(renderTarget) )
		{
			return;
		}

		/* This is a raw pointer to the renderable interface. */
		const auto * renderable = renderableInstance->renderable();

		if constexpr ( IsDebug )
		{
			if ( renderable == nullptr )
			{
				Tracer::error(ClassId, "The renderable interface pointer is a null !");

				return;
			}

			/* NOTE: Check whether the renderable is ready to draw.
			 * Only done in debug mode because a renderable instance ready to
			 * render implies the renderable is ready to draw. */
			if ( !renderable->isReadyForInstantiation() )
			{
				Tracer::error(ClassId, "The renderable interface is not ready !");

				return;
			}
		}

		const auto layerCount = renderable->layerCount();

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
		{
			const auto isOpaque = renderable->isOpaque(layerIndex);

			if ( m_lightSet.isEnabled() && renderableInstance->isLightingEnabled() )
			{
				if ( isOpaque )
				{
					RenderBatch::create(m_renderLists[OpaqueLighted], distance, renderableInstance, layerIndex);
				}
				else
				{
					RenderBatch::create(m_renderLists[TranslucentLighted], distance * -1.0F, renderableInstance, layerIndex);
				}
			}
			else
			{
				if ( isOpaque )
				{
					RenderBatch::create(m_renderLists[Opaque], distance, renderableInstance, layerIndex);
				}
				else
				{
					RenderBatch::create(m_renderLists[Translucent], distance * -1.0F, renderableInstance, layerIndex);
				}
			}
		}
	}

	bool
	Scene::populateRenderLists (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, bool isShadowCasting) noexcept
	{
		/* FIXME: TODO integrate the max viewable distance and the frustum culling. */

		/* NOTE: Clean render lists before. */
		if ( isShadowCasting )
		{
			m_renderLists[Shadows].clear();
		}
		else
		{
			m_renderLists[Opaque].clear();
			m_renderLists[Translucent].clear();
			m_renderLists[OpaqueLighted].clear();
			m_renderLists[TranslucentLighted].clear();
		}

		/* NOTE: The camera position doesn't move during calculation. */
		const auto & cameraPosition = renderTarget->viewMatrices().position();
		Vector< 3, float > entityPosition{};

		/* Sorting global renderable objects from the scene. */
		for ( const auto & visualComponent : m_sceneVisualComponents )
		{
			if ( visualComponent == nullptr )
			{
				continue;
			}

			const auto renderableInstance = visualComponent->getRenderableInstance();

			if ( renderableInstance == nullptr )
			{
				continue;
			}

			if ( isShadowCasting )
			{
				this->insertInShadowCastLists(renderTarget, renderableInstance, 0.0F);
			}
			else
			{
				this->insertInRenderLists(renderTarget, renderableInstance, 0.0F);
			}
		}

		/* Sorting renderable objects from scene static entities. */
		{
			const std::lock_guard< std::mutex > lock{m_staticEntitiesMutex};

			for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
			{
				/* Check whether the static entity contains something to render. */
				if ( !staticEntity->isRenderable() )
				{
					continue;
				}

				/* Prepares the entity position (costly) for the lambda. */
				entityPosition = staticEntity->getWorldCoordinates().position();

				for ( const auto & component: staticEntity->components() | std::views::values )
				{
					const auto renderableInstance = component->getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						continue;
					}

					if ( isShadowCasting )
					{
						this->insertInShadowCastLists(renderTarget, renderableInstance, Vector< 3, float >::distance(cameraPosition, entityPosition));
					}
					else
					{
						this->insertInRenderLists(renderTarget, renderableInstance, Vector< 3, float >::distance(cameraPosition, entityPosition));
					}
				}
			}
		}

		/* Sorting renderable objects from the scene node tree. */
		{
			/* NOTE: Prevent scene node deletion from logic update thread to crash the rendering. */
			const std::lock_guard< std::mutex > lock{m_sceneNodesMutex};

			NodeCrawler< const Node > crawler{m_rootNode};

			std::shared_ptr< const Node > node{};

			while ( (node = crawler.nextNode()) != nullptr )
			{
				/* Check whether the scene node contains something to render. */
				if ( !node->isRenderable() )
				{
					continue;
				}

				/* Prepares the entity position (costly) for the lambda. */
				entityPosition = node->getWorldCoordinates().position();

				for ( const auto & component: node->components() | std::views::values )
				{
					const auto renderableInstance = component->getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						continue;
					}

					if ( isShadowCasting )
					{
						this->insertInShadowCastLists(renderTarget, renderableInstance, Vector< 3, float >::distance(cameraPosition, entityPosition));
					}
					else
					{
						this->insertInRenderLists(renderTarget, renderableInstance, Vector< 3, float >::distance(cameraPosition, entityPosition));
					}
				}
			}
		}

		/* Return true if something can be rendered. */
		return std::ranges::any_of(m_renderLists, [] (const auto & renderList) {
			return !renderList.empty();
		});
	}

	void
	Scene::updateVideoMemory () const noexcept
	{
		if ( !m_lightSet.updateVideoMemory() )
		{
			Tracer::error(ClassId, "Unable to update the light set data to the video memory !");
		}
	}

	void
	Scene::renderSelection (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer, const RenderBatch::List & unlightedObjects, const RenderBatch::List & lightedObjects) const noexcept
	{
		if ( !unlightedObjects.empty() )
		{
			for ( const auto & renderBatch : unlightedObjects | std::views::values )
			{
				renderBatch.renderableInstance()->render(renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), commandBuffer);
			}
		}

		if ( !m_lightSet.isEnabled() || lightedObjects.empty() )
		{
			return;
		}

		if ( m_lightSet.isUsingStaticLighting() )
		{
			for ( const auto & renderBatch : lightedObjects | std::views::values )
			{
				renderBatch.renderableInstance()->render(renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), commandBuffer);
			}

			return;
		}

		/* For all objects. */
		for ( const auto & renderBatch : lightedObjects | std::views::values )
		{
			const std::lock_guard< std::mutex > lock{m_lightSet.mutex()};

			/* Ambient pass. */
			renderBatch.renderableInstance()->render(renderTarget, nullptr, RenderPassType::AmbientPass, renderBatch.subGeometryIndex(), commandBuffer);

			/* Loop through all directional lights. */
			for ( const auto & light : m_lightSet.directionalLights() )
			{
				if ( !light->isEnabled() )
				{
					continue;
				}

				renderBatch.renderableInstance()->render(renderTarget, light.get(), RenderPassType::DirectionalLightPassNoShadow, renderBatch.subGeometryIndex(), commandBuffer);
			}

			/* Loop through all point lights. */
			for ( const auto & light : m_lightSet.pointLights() )
			{
				if ( !light->isEnabled() )
				{
					continue;
				}

				const auto & instance = renderBatch.renderableInstance();

				if ( instance->isLightDistanceCheckDisabled() || light->touch(instance->worldPosition()) )
				{
					instance->render(renderTarget, light.get(), RenderPassType::PointLightPassNoShadow, renderBatch.subGeometryIndex(), commandBuffer);
				}
			}

			/* Loop through all spotlights. */
			for ( const auto & light : m_lightSet.spotLights() )
			{
				if ( !light->isEnabled() )
				{
					continue;
				}

				const auto & instance = renderBatch.renderableInstance();

				if ( instance->isLightDistanceCheckDisabled() || light->touch(instance->worldPosition()) )
				{
					renderBatch.renderableInstance()->render(renderTarget, light.get(), RenderPassType::SpotLightPassNoShadow, renderBatch.subGeometryIndex(), commandBuffer);
				}
			}
		}
	}

	void
	Scene::castShadows (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		if ( !m_lightSet.isEnabled() )
		{
			return;
		}

		/* Sort the scene according to the point of view. */
		if ( !this->populateRenderLists(renderTarget, true) )
		{
			/* There is nothing to shadow to cast ... */
			return;
		}

		//TraceDebug{ClassId} <<
		//	"Shadow map content :" "\n"
		//	" - Plain objects : " << m_renderLists[Shadows].size() << "\n";

		for ( const auto & renderBatch : m_renderLists[Shadows] | std::views::values )
		{
			renderBatch.renderableInstance()->castShadows(renderTarget, renderBatch.subGeometryIndex(), commandBuffer);
		}
	}

	void
	Scene::render (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		/* Sort the scene according to the point of view. */
		if ( !this->populateRenderLists(renderTarget, false) )
		{
			/* There is nothing to render... */
			return;
		}

		//TraceDebug{ClassId} <<
		//	"Frame content :" "\n"
		//	" - Opaque / +lighted : " << m_renderLists[Opaque].size() << " / " << m_renderLists[OpaqueLighted].size() << "\n"
		//	" - Translucent / +lighted : " << m_renderLists[Translucent].size() << " / " << m_renderLists[TranslucentLighted].size() << "\n";

		/* First, we render all opaque renderable objects. */
		this->renderSelection(renderTarget, commandBuffer, m_renderLists[Opaque], m_renderLists[OpaqueLighted]);

		/* After, we render all translucent renderable objects. */
		this->renderSelection(renderTarget, commandBuffer, m_renderLists[Translucent], m_renderLists[TranslucentLighted]);

		/* Optional rendering.
		 * FIXME: Add a master control. */
		/*{
			for ( const auto & renderBatch : m_renderLists[Opaque] )
			{
				const auto * renderableInstance = renderBatch.second.renderableInstance();

				if ( renderableInstance->isDisplayTBNSpaceEnabled() )
				{
					renderableInstance->renderTBNSpace(renderTarget, commandBuffer);
				}
			}

			for ( const auto & renderBatch : m_renderLists[Translucent] )
			{
				const auto * renderableInstance = renderBatch.second.renderableInstance();

				if ( renderableInstance->isDisplayTBNSpaceEnabled() )
				{
					renderableInstance->renderTBNSpace(renderTarget, commandBuffer);
				}
			}

			for ( const auto & renderBatch : m_renderLists[OpaqueLighted] )
			{
				const auto * renderableInstance = renderBatch.second.renderableInstance();

				if ( renderableInstance->isDisplayTBNSpaceEnabled() )
				{
					renderableInstance->renderTBNSpace(renderTarget, commandBuffer);
				}
			}

			for ( const auto & renderBatch : m_renderLists[TranslucentLighted] )
			{
				const auto * renderableInstance = renderBatch.second.renderableInstance();

				if ( renderableInstance->isDisplayTBNSpaceEnabled() )
				{
					renderableInstance->renderTBNSpace(renderTarget, commandBuffer);
				}
			}
		}*/
	}

	void
	Scene::checkAVConsoleNotification (int notificationCode, const std::any & data) const noexcept
	{
		switch ( notificationCode )
		{
			case AVConsole::Manager::VideoDeviceAdded :
				TraceDebug{ClassId} << "A new video device is available.";
				break;

			case AVConsole::Manager::VideoDeviceRemoved :
				TraceDebug{ClassId} << "A video device has been removed.";
				break;

			case AVConsole::Manager::AudioDeviceAdded :
				TraceDebug{ClassId} << "A new audio device is available.";
				break;

			case AVConsole::Manager::AudioDeviceRemoved :
				TraceDebug{ClassId} << "An audio device has been removed.";
				break;

			case AVConsole::Manager::RenderToShadowMapAdded :
			case AVConsole::Manager::RenderToTextureAdded :
			case AVConsole::Manager::RenderToViewAdded :
				this->initializeRenderTarget(std::any_cast< std::shared_ptr< RenderTarget::Abstract > >(data));
				break;

			default :
				if constexpr ( ObserverDebugEnabled )
				{
					TraceDebug{ClassId} << "Event #" << notificationCode << " from a master control console ignored.";
				}
				break;
		}
	}

	bool
	Scene::checkRootNodeNotification (int notificationCode, const std::any & data) noexcept
	{
		switch ( notificationCode )
		{
			/* NOTE: A node is creating a child. The data will be a smart pointer to the parent node. */
			case Node::SubNodeCreating :
				return true;

			/* NOTE: A node created a child. The data will be a smart pointer to the child node. */
			case Node::SubNodeCreated :
				return true;

			/* NOTE: A node is destroying one of its children. The data will be a smart pointer to the child node. */
			case Node::SubNodeDeleting :
			{
				const auto node = std::any_cast< std::shared_ptr< Node > >(data);

				/* NOTE: If a node controller was set up with this node, we stop it. */
				if ( m_nodeController.node() == node )
				{
					m_nodeController.releaseNode();
				}

				if ( m_renderingOctree != nullptr && node->isRenderable() )
				{
					const std::lock_guard< std::mutex > lockGuard{m_renderingOctreeMutex};

					m_renderingOctree->erase(node);
				}

				if ( m_physicsOctree != nullptr )
				{
					const std::lock_guard< std::mutex > lockGuard{m_physicsOctreeMutex};

					m_physicsOctree->erase(node);
				}
			}
				return true;

			case Node::SubNodeDeleted :
				return true;

			default:
				if constexpr ( ObserverDebugEnabled )
				{
					const auto node = std::any_cast< std::shared_ptr< Node > >(data);

					TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << node->name() << "' ignored.";
				}
				return false;
		}
	}

	bool
	Scene::checkEntityNotification (int notificationCode, const std::any & data) noexcept
	{
		switch ( notificationCode )
		{
			case AbstractEntity::ModifierCreated :
				m_modifiers.emplace(std::any_cast< std::shared_ptr< Component::AbstractModifier > >(data));

				return true;

			case AbstractEntity::ModifierDestroyed :
				m_modifiers.erase(std::any_cast< std::shared_ptr< Component::AbstractModifier > >(data));

				return true;

			case AbstractEntity::CameraCreated :
				m_AVConsoleManager.addVideoDevice(std::any_cast< std::shared_ptr< Component::Camera > >(data));

				return true;

			case AbstractEntity::PrimaryCameraCreated :
				m_AVConsoleManager.addVideoDevice(std::any_cast< std::shared_ptr< Component::Camera > >(data), true);

				return true;

			case AbstractEntity::CameraDestroyed :
				m_AVConsoleManager.removeVideoDevice(std::any_cast< std::shared_ptr< Component::Camera > >(data));

				return true;

			case AbstractEntity::MicrophoneCreated :
				m_AVConsoleManager.addAudioDevice(std::any_cast< std::shared_ptr< Component::Microphone > >(data));

				return true;

			case AbstractEntity::PrimaryMicrophoneCreated :
				m_AVConsoleManager.addAudioDevice(std::any_cast< std::shared_ptr< Component::Microphone > >(data), true);

				return true;

			case AbstractEntity::MicrophoneDestroyed :
				m_AVConsoleManager.removeAudioDevice(std::any_cast< std::shared_ptr< Component::Microphone > >(data));

				return true;

			case AbstractEntity::DirectionalLightCreated :
				m_lightSet.add(std::any_cast< std::shared_ptr< Component::DirectionalLight > >(data), m_graphicsRenderer);

				return true;

			case AbstractEntity::DirectionalLightDestroyed :
				m_lightSet.remove(std::any_cast< std::shared_ptr< Component::DirectionalLight > >(data));

				return true;

			case AbstractEntity::PointLightCreated :
				m_lightSet.add(std::any_cast< std::shared_ptr< Component::PointLight > >(data), m_graphicsRenderer);

				return true;

			case AbstractEntity::PointLightDestroyed :
				m_lightSet.remove(std::any_cast< std::shared_ptr< Component::PointLight > >(data));

				return true;

			case AbstractEntity::SpotLightCreated :
				m_lightSet.add(std::any_cast< std::shared_ptr< Component::SpotLight > >(data), m_graphicsRenderer);

				return true;

			case AbstractEntity::SpotLightDestroyed :
				m_lightSet.remove(std::any_cast< std::shared_ptr< Component::SpotLight > >(data));

				return true;

			case AbstractEntity::VisualComponentCreated :
			{
				const auto component = std::any_cast< std::shared_ptr< Component::Visual > >(data);

				this->checkRenderableInstance(component->getRenderableInstance());
			}
				return true;

			case AbstractEntity::VisualComponentDestroyed :
			{
				const auto component = std::any_cast< std::shared_ptr< Component::Visual > >(data);

				this->forget(component->getRenderableInstance().get());
			}
				return true;

			case AbstractEntity::MultipleVisualsComponentCreated :
			{
				const auto component = std::any_cast< std::shared_ptr< Component::MultipleVisuals > >(data);

				this->checkRenderableInstance(component->getRenderableInstance());
			}
				return true;

			case AbstractEntity::MultipleVisualsComponentDestroyed :
			{
				const auto component = std::any_cast< std::shared_ptr< Component::MultipleVisuals > >(data);

				this->forget(component->getRenderableInstance().get());
			}
				return true;

			case AbstractEntity::ParticlesEmitterCreated :
			{
				const auto component = std::any_cast< std::shared_ptr< Component::ParticlesEmitter > >(data);

				this->checkRenderableInstance(component->getRenderableInstance());
			}
				return true;

			case AbstractEntity::ParticlesEmitterDestroyed :
			{
				const auto component = std::any_cast< std::shared_ptr< Component::ParticlesEmitter > >(data);

				this->forget(component->getRenderableInstance().get());
			}
				return true;

			default:
				if constexpr ( ObserverDebugEnabled )
				{
					const auto component = std::any_cast< std::shared_ptr< Component::Abstract > >(data);

					TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << component->name() << "' ignored.";
				}
				return false;
		}
	}

	bool
	Scene::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept
	{
		if ( observable == &m_AVConsoleManager )
		{
			this->checkAVConsoleNotification(notificationCode, data);

			/* Keep listening. */
			return true;
		}

		if ( observable->is(StaticEntity::ClassUID) )
		{
			if ( notificationCode == AbstractEntity::EntityContentModified )
			{
				const auto staticEntity = std::any_cast< std::shared_ptr< StaticEntity > >(data);

				this->checkEntityLocationInOctrees(staticEntity);
			}
			else
			{
				this->checkEntityNotification(notificationCode, data);
			}

			/* Keep listening. */
			return true;
		}

		if ( observable->is(Node::ClassUID) )
		{
			if ( notificationCode == AbstractEntity::EntityContentModified )
			{
				const auto node = std::any_cast< std::shared_ptr< Node > >(data);

				this->checkEntityLocationInOctrees(node);
			}
			else if ( !this->checkRootNodeNotification(notificationCode, data) )
			{
				this->checkEntityNotification(notificationCode, data);
			}

			/* Keep listening. */
			return true;
		}

		if ( observable->is(RenderableInstance::Abstract::ClassUID) )
		{
			if ( notificationCode == RenderableInstance::Abstract::ReadyToSetupOnGPU )
			{
				const auto renderableInstance = std::any_cast< std::shared_ptr< RenderableInstance::Abstract > >(data);

				this->initializeRenderableInstance(renderableInstance);
			}

			/* Keep listening. */
			return true;
		}

		/* NOTE: Don't know what is it, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable '" << whoIs(observable->classUID()) << "' (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
	}

	void
	Scene::checkRenderableInstance (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) noexcept
	{
		if ( renderableInstance == nullptr )
		{
			Tracer::error(ClassId, "The renderable instance pointer is a null !");

			return;
		}

		const auto * renderable = renderableInstance->renderable();

		if ( renderable == nullptr )
		{
			Tracer::error(ClassId, "There is no renderable in the renderable instance !");

			return;
		}

		/* NOTE: If the renderable is ready for instantiation, we configure the rendering now ... */
		if ( renderable->isReadyForInstantiation() )
		{
			this->initializeRenderableInstance(renderableInstance);
		}
		else /* ... Or we delay the update later. */
		{
			/* This will wait for the event "RenderableInstance::Abstract::ReadyToSetupOnGPU". */
			this->observe(renderableInstance.get());
		}
	}

	void
	Scene::initializeRenderableInstance (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) const noexcept
	{
		for ( const auto & renderTarget : m_AVConsoleManager.renderToShadowMaps() )
		{
			if ( !this->getRenderableInstanceReadyForShadowCasting(renderableInstance, renderTarget) )
			{
				TraceError{ClassId} << "The initialization of renderable instance '" << renderableInstance->renderable()->name() << "' for shadow map '" << renderTarget->id() << "' has failed !";
			}
		}

		for ( const auto & renderTarget : m_AVConsoleManager.renderToTextures() )
		{
			if ( !this->getRenderableInstanceReadyForRender(renderableInstance, renderTarget) )
			{
				TraceError{ClassId} << "The initialization of renderable instance '" << renderableInstance->renderable()->name() << "' for render target '" << renderTarget->id() << "' has failed !";
			}
		}

		for ( const auto & renderTarget : m_AVConsoleManager.renderToViews() )
		{
			if ( !this->getRenderableInstanceReadyForRender(renderableInstance, renderTarget) )
			{
				TraceError{ClassId} << "The initialization of renderable instance '" << renderableInstance->renderable()->name() << "' for render target '" << renderTarget->id() << "' has failed !";
			}
		}
	}

	void
	Scene::initializeRenderTarget (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
	{
		if ( renderTarget->renderType() == RenderTargetType::ShadowMap || renderTarget->renderType() == RenderTargetType::ShadowCubemap )
		{
			TraceDebug{ClassId} << "A new shadow map is available " << to_cstring(renderTarget->renderType()) << " ! Updating renderable instances from the scene ...";

			this->forEachRenderableInstance([this, renderTarget] (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) {
				if ( !this->getRenderableInstanceReadyForShadowCasting(renderableInstance, renderTarget) )
				{
					TraceError{ClassId} << "The initialization of renderable instance '" << renderableInstance->renderable()->name() << "' from shadow map '" << renderTarget->id() << "' has failed !";
				}

				return true;
			});
		}
		else
		{
			TraceDebug{ClassId} << "A new render target is available " << to_cstring(renderTarget->renderType()) << " ! Updating renderable instances from the scene ...";

			this->forEachRenderableInstance([this, renderTarget] (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) {
				if ( !this->getRenderableInstanceReadyForRender(renderableInstance, renderTarget) )
				{
					TraceError{ClassId} << "The initialization of renderable instance '" << renderableInstance->renderable()->name() << "' from render target '" << renderTarget->id() << "' has failed !";
				}

				return true;
			});
		}
	}

	void
	Scene::refreshRenderableInstances (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
	{
		this->forEachRenderableInstance([renderTarget] (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) {
			if ( !renderableInstance->refreshGraphicsPipelines(renderTarget) )
			{
				Tracer::error(ClassId, "The renderable instance refresh for rendering has failed !");
			}

			return true;
		});
	}

	void
	Scene::forEachRenderableInstance (const std::function< bool (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) > & function) const noexcept
	{
		/* Check global renderable objects from the scene. */
		for ( const auto & visualComponent : m_sceneVisualComponents )
		{
			if ( visualComponent == nullptr )
			{
				continue;
			}

			const auto renderableInstance = visualComponent->getRenderableInstance();

			if ( renderableInstance == nullptr )
			{
				Tracer::error(ClassId, "The scene visual renderable instance pointer is null !");

				continue;
			}

			if ( !function(renderableInstance) )
			{
				return;
			}
		}

		/* Check renderable objects from scene static entities. */
		{
			const std::lock_guard< std::mutex > lock{m_staticEntitiesMutex};

			for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
			{
				/* Check whether the static entity contains something to render. */
				if ( !staticEntity->isRenderable() )
				{
					continue;
				}

				/* Go through each entity component to update visuals. */
				for ( const auto & [name, component] : staticEntity->components() )
				{
					if ( !component->isRenderable() )
					{
						continue;
					}

					const auto renderableInstance = component->getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						TraceError{ClassId} << "The static entity '" << staticEntity->name() << "' component '" << name << "' renderable instance pointer is null !";

						continue;
					}

					if ( !function(renderableInstance) )
					{
						return;
					}
				}
			}
		}

		/* Check renderable objects from the scene node tree. */
		{
			/* NOTE: Prevent scene node deletion from logic update thread to crash the rendering. */
			const std::lock_guard< std::mutex > lock{m_sceneNodesMutex};

			NodeCrawler< const Node > crawler{m_rootNode};

			std::shared_ptr< const Node > node{};

			while ( (node = crawler.nextNode()) != nullptr )
			{
				/* Check whether the scene node contains something to render. */
				if ( !node->isRenderable() )
				{
					continue;
				}

				/* Go through each entity component to update visuals. */
				for ( const auto & [name, component] : node->components() )
				{
					if ( !component->isRenderable() )
					{
						continue;
					}

					const auto renderableInstance = component->getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						TraceError{ClassId} << "The scene node '" << node->name() << "' component '" << name << "' renderable instance pointer is null !";

						continue;
					}

					if ( !function(renderableInstance) )
					{
						return;
					}
				}
			}
		}
	}

	std::shared_ptr< Node >
	Scene::findNode (const std::string & nodeName) const noexcept
	{
		std::shared_ptr< Node > currentNode = m_rootNode;

		NodeCrawler< Node > crawler(currentNode);

		while ( (currentNode = crawler.nextNode()) != nullptr )
		{
			if ( currentNode->name() == nodeName )
			{
				return currentNode;
			}
		}

		return nullptr;
	}

	std::array< size_t, 2 >
	Scene::getNodeStatistics () const noexcept
	{
		std::array< size_t, 2 > stats{0UL, 0UL};

		std::shared_ptr< const Node > currentNode = m_rootNode;

		NodeCrawler< const Node > crawler(currentNode);

		do
		{
			stats[0] += currentNode->children().size();

			const auto depth = currentNode->getDepth();

			stats[1] = std::max(stats[1], depth);
		}
		while ( (currentNode = crawler.nextNode()) != nullptr );

		return stats;
	}

	bool
	Scene::contains (const Vector< 3, float > & worldPosition) const noexcept
	{
		/* Checks on X axis. */
		if ( worldPosition[X] > m_boundary || worldPosition[X] < -m_boundary )
		{
			return false;
		}

		/* Checks on Y axis. */
		if ( worldPosition[Y] > m_boundary || worldPosition[Y] < -m_boundary )
		{
			return false;
		}

		/* Checks on Z axis. */
		if ( worldPosition[Z] > m_boundary || worldPosition[Z] < -m_boundary )
		{
			return false;
		}

		return true;
	}

	std::vector< RenderPassType >
	Scene::prepareRenderPassTypes (const RenderableInstance::Abstract & renderableInstance) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_lightSet.mutex()};

		std::vector< RenderPassType > renderPassTypes;

		if ( !m_lightSet.isEnabled() || !renderableInstance.isLightingEnabled() || m_lightSet.isUsingStaticLighting() )
		{
			renderPassTypes.emplace_back(RenderPassType::SimplePass);
		}
		else
		{
			renderPassTypes.emplace_back(RenderPassType::AmbientPass);

			//if ( !m_lightSet.directionalLights().empty() )
			{
				//renderPassTypes.emplace_back(RenderPassType::DirectionalLightPass);
				renderPassTypes.emplace_back(RenderPassType::DirectionalLightPassNoShadow);
			}

			//if ( !m_lightSet.pointLights().empty() )
			{
				//renderPassTypes.emplace_back(RenderPassType::PointLightPass);
				renderPassTypes.emplace_back(RenderPassType::PointLightPassNoShadow);
			}

			//if ( !m_lightSet.spotLights().empty() )
			{
				//renderPassTypes.emplace_back(RenderPassType::SpotLightPass);
				renderPassTypes.emplace_back(RenderPassType::SpotLightPassNoShadow);
			}
		}

		return renderPassTypes;
	}

	bool
	Scene::getRenderableInstanceReadyForShadowCasting (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
	{
		/* If the object is ready to shadow cast, there is nothing more to do! */
		if ( renderableInstance->isReadyToCastShadows(renderTarget) )
		{
			return true;
		}

		/* A previous try to set up the renderable instance for rendering has failed ... */
		if ( renderableInstance->isBroken() )
		{
			return false;
		}

		return renderableInstance->getReadyForShadowCasting(renderTarget, m_graphicsRenderer);
	}

	bool
	Scene::getRenderableInstanceReadyForRender (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
	{
		/* If the object is ready to render, there is nothing more to do! */
		if ( renderableInstance->isReadyToRender(renderTarget) )
		{
			return true;
		}

		/* A previous try to set up the renderable instance for rendering has failed ... */
		if ( renderableInstance->isBroken() )
		{
			return false;
		}

		/* NOTE: Check how many render pass this renderable instance needs. */
		const auto renderPassTypes = this->prepareRenderPassTypes(*renderableInstance);

		if ( renderPassTypes.empty() )
		{
			renderableInstance->setBroken();

			return false;
		}

		return renderableInstance->getReadyForRender(*this, renderTarget, renderPassTypes, m_graphicsRenderer);
	}

	std::string
	Scene::getNodeSystemStatistics (bool showTree) const noexcept
	{
		std::stringstream output;

		output << "Node system: " "\n";

		if ( m_rootNode != nullptr )
		{
			const auto stats = this->getNodeStatistics();

			output <<
				"Node count: " << stats[0] << "\n"
				"Node depth: " << stats[1] << '\n';

			if ( showTree )
			{
				std::shared_ptr< const Node > currentNode = m_rootNode;

				NodeCrawler< const Node > crawler(currentNode);

				do
				{
					const std::string pad(currentNode->getDepth() * 2, ' ');

					output << pad <<
						"[Node:" << currentNode->name() << "]"
						"[Location: " << currentNode->getWorldCoordinates().position() << "] ";

					if ( !currentNode->components().empty() )
					{
						output << '\n';

						for ( const auto & component : std::ranges::views::values(currentNode->components()) )
						{
							output << pad << "   {" << component->getComponentType() << ":" << component->name() << "}" "\n";
						}
					}
					else
					{
						output << "(Empty node)" "\n";
					}
				}
				while ( (currentNode = crawler.nextNode()) != nullptr );
			}
		}
		else
		{
			output << "No root node !" "\n";
		}

		return output.str();
	}

	std::string
	Scene::getStaticEntitySystemStatistics (bool showTree) const noexcept
	{
		std::stringstream output;

		output << "Static entity system: " "\n";

		if ( m_staticEntities.empty() )
		{
			output << "No static entity !" "\n";
		}
		else
		{
			output << "Static entity count: " << m_staticEntities.size() << "\n";

			if ( showTree )
			{
				for ( auto staticEntityIt = m_staticEntities.cbegin(); staticEntityIt != m_staticEntities.cend(); ++staticEntityIt )
				{
					const auto & instance = staticEntityIt->second;

					output <<
						"[Static entity #" << std::distance(m_staticEntities.cbegin(), staticEntityIt) << ":" << staticEntityIt->first << "]"
						"[Location: " << instance->getWorldCoordinates().position() << "] ";

					if ( !instance->components().empty() )
					{
						output << '\n';

						for ( const auto & component : std::ranges::views::values(instance->components()) )
						{
							output << "   {" << component->getComponentType() << ":" << component->name() << "}" "\n";
						}
					}
					else
					{
						output << "(Empty static entity)" "\n";
					}
				}
			}
		}

		return output.str();
	}

	std::string
	Scene::getSectorSystemStatistics (bool showTree) const noexcept
	{
		std::stringstream output;

		if ( m_renderingOctree == nullptr )
		{
			output << "No rendering octree enabled !" "\n";
		}
		else
		{
			const std::lock_guard< std::mutex > lock{m_renderingOctreeMutex};

			output <<
				"Rendering octree :" "\n"
				"Sector depth: " << m_renderingOctree->getDepth() << "\n"
				"Sector count: " << m_renderingOctree->getSectorCount() << "\n"
				"Root element count: " << m_renderingOctree->elements().size() << '\n';

			if ( showTree )
			{
				for ( const auto & element : m_renderingOctree->elements() )
				{
					output << "\t" "- " << element->name() << "\n";
				}
			}
		}

		if ( m_physicsOctree == nullptr )
		{
			output << "No physics octree enabled !" "\n";
		}
		else
		{
			const std::lock_guard< std::mutex > lock{m_physicsOctreeMutex};

			output <<
				"Physics octree :" "\n"
				"Sector depth: " << m_physicsOctree->getDepth() << "\n"
				"Sector count: " << m_physicsOctree->getSectorCount() << "\n"
				"Root element count: " << m_physicsOctree->elements().size() << '\n';

			if ( showTree )
			{
				for ( const auto & subSector : m_physicsOctree->subSectors() )
				{
					output << " Sector depth:" << subSector->getDistance() << ", slot:" << subSector->slot() << "\n";

					for ( const auto & element : subSector->elements() )
					{
						output << "\t" "- " << element->name() << "\n";
					}
				}
			}
		}

		return output.str();
	}
}
