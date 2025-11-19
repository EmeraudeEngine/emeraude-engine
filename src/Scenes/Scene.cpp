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
#include "Audio/HardwareOutput.hpp"
#include "Audio/Manager.hpp"
#include "Graphics/Renderer.hpp"
#include "Input/Manager.hpp"
#include "NodeCrawler.hpp"
#include "Tracer.hpp"
#include "Vulkan/SwapChain.hpp" // FIXME: Should not be there

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Graphics;
	using namespace Saphir;
	using namespace Physics;

	Scene::~Scene ()
	{
		/* From 'Scene setup data' */
		{
			m_initialized = false;

			/* NOTE: Release all shared_ptr. */
			m_environmentEffects.clear();

			/* NOTE: Other data are trivial. */
		}

		/* From 'Structures for faster access to specific data' */
		{
			/* NOTE: Release all shared_ptr on entity components. */
			m_modifiers.clear();

			/* NOTE: Release all shared_ptr on entity components. */
			m_renderToViews.clear();
			m_renderToTextures.clear();
			m_renderToShadowMaps.clear();

			/* NOTE: Release all shared_ptr. */
			for ( auto & visual : m_sceneVisualComponents )
			{
				visual.reset();
			}

			/* NOTE: Releasing octrees provoked by the smart-pointer reset. */
			//this->destroyOctrees();
			m_physicsOctree.reset();
			m_renderingOctree.reset();
		}

		/* From 'Managers deeply linked to the scene content' */
		{
			/* Release the shared_ptr on a scene node. */
			m_nodeController.releaseNode();

			/* Release all shared_ptr on renderable. */
			for ( auto & renderList : m_renderLists )
			{
				renderList.clear();
			}

			/* Release all shared_ptr on entity components, should remove renderTarget (shadow map). */
			m_lightSet.removeAllLights();
			m_lightSet.terminate(*this);

			/* [OFFSCREEN-CLEANUP] Crash here! */
			m_AVConsoleManager.clear();
		}

		/* From 'Real scene content holder' */
		{
			/* NOTE: Release shared_ptr */
			m_seaLevelResource.reset();
			m_sceneAreaResource.reset();
			m_backgroundResource.reset();

			/* NOTE: Release all shared_ptr */
			m_staticEntities.clear();

			/* NOTE: Destroy the node tree and reset the root node. */
			this->resetNodeTree();
			m_rootNode.reset();
		}
	}

	void
	Scene::registerSceneVisualComponents () noexcept
	{
		if ( m_backgroundResource != nullptr )
		{
			m_sceneVisualComponents[0] = std::make_unique< Component::Visual >("Background", *m_rootNode, m_backgroundResource);

			/* NOTE: Disables lighting model on the background.
			 * TODO: Check to disable at construct time. */
			const auto renderableInstance = m_sceneVisualComponents[0]->getRenderableInstance();
			renderableInstance->setUseInfinityView(true);
			renderableInstance->disableDepthTest(true);
			renderableInstance->disableDepthWrite(true);
		}

		if ( m_sceneAreaResource != nullptr )
		{
			m_sceneVisualComponents[1] = std::make_unique< Component::Visual >("SceneArea", *m_rootNode, m_sceneAreaResource);

			const auto renderableInstance = m_sceneVisualComponents[1]->getRenderableInstance();
			renderableInstance->enableLighting();
			renderableInstance->disableLightDistanceCheck();
			renderableInstance->enableDisplayTBNSpace(false);
		}

		if ( m_seaLevelResource != nullptr )
		{
			m_sceneVisualComponents[2] = std::make_unique< Component::Visual >("SeaLevel", *m_rootNode, m_seaLevelResource);

			const auto renderableInstance = m_sceneVisualComponents[2]->getRenderableInstance();
			renderableInstance->enableLighting();
			renderableInstance->disableLightDistanceCheck();
			renderableInstance->enableDisplayTBNSpace(false);
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
				currentNode->forEachComponent([&hasCamera, &hasMicrophone] (const Component::Abstract & component) {
					if ( component.isComponent(Component::Camera::ClassId) )
					{
						hasCamera = true;
					}
					else if ( component.isComponent(Component::Microphone::ClassId) )
					{
						hasMicrophone = true;
					}
				});

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

			const auto camera = m_rootNode->createChild("DefaultCameraNode", {}, m_lifetimeMS)
				->componentBuilder< Component::Camera >("DefaultCamera").asPrimary().build(true);

			if ( camera == nullptr )
			{
				Tracer::error(ClassId, "Scene initialization error : Unable to create a default camera !");

				return false;
			}
		}

		if ( !hasMicrophone )
		{
			Tracer::warning(ClassId, "There is no microphone in the scene ! Creating a default microphone ...");

			const auto microphone = m_rootNode->createChild("DefaultMicrophoneNode", {}, m_lifetimeMS)
				->componentBuilder< Component::Microphone >("DefaultMicrophone").asPrimary().build();

			if ( microphone == nullptr )
			{
				Tracer::error(ClassId, "Scene initialization error : Unable to create a default microphone !");

				return false;
			}
		}

		/* Set audio properties for this scene. */
		m_AVConsoleManager.audioManager().setEnvironmentSoundProperties(m_environmentPhysicalProperties);

		return true;
	}

	bool
	Scene::enable (Input::Manager & inputManager, Settings & /*settings*/) noexcept
	{
		if ( !m_initialized )
		{
			this->registerSceneVisualComponents();

			/* Create a missing camera and/or microphone. */
			if ( !this->initializeBaseComponents() )
			{
				return false;
			}

			/* NOTE: Connecting video devices. */
			{
				if ( !m_AVConsoleManager.hasPrimaryVideoOutput() )
				{
					/* FIXME: Be aware of the offscreen view with window less application. */
					if ( const auto swapChain = m_AVConsoleManager.graphicsRenderer().mainRenderTarget(); swapChain != nullptr )
					{
						m_AVConsoleManager.addVideoDevice(swapChain, true);

						m_renderToViewAccess.lock();
						m_renderToViews.emplace(swapChain);
						m_renderToViewAccess.unlock();

						TraceDebug{ClassId} << "SwapChain added to AVConsole!";
					}
				}

				if ( !m_AVConsoleManager.autoConnectPrimaryVideoDevices() )
				{
					TraceError{ClassId} << "Unable to auto-connect primary video devices !";

					return false;
				}

				if ( !m_lightSet.initialize(*this) )
				{
					TraceError{ClassId} << "Unable to initialize the light set !";

					return false;
				}
			}

			/* NOTE: Connecting audio devices (optional). */
			if ( m_AVConsoleManager.audioManager().usable() )
			{
				if ( !m_AVConsoleManager.hasPrimaryAudioOutput() )
				{
					const auto defaultSpeaker = std::make_shared< Audio::HardwareOutput >(AVConsole::Manager::DefaultSpeakerName, m_AVConsoleManager.audioManager());

					m_AVConsoleManager.addAudioDevice(defaultSpeaker, true);
				}

				if ( !m_AVConsoleManager.autoConnectPrimaryAudioDevices() )
				{
					TraceError{ClassId} << "Unable to auto-connect primary audio devices !";

					return false;
				}
			}
			else
			{
				TraceWarning{ClassId} << "No audio layer available!";
			}

			TraceSuccess{ClassId} << "Scene " << this->name() << " initialized!" "\n" << m_AVConsoleManager.getConnexionStates();

			m_initialized = true;
		}

		/* FIXME: When re-enabling, the swap-chain does not have the correct ambient light parameters! */

		inputManager.addKeyboardListener(&m_nodeController);

		return true;
	}

	void
	Scene::disable (Input::Manager & inputManager) noexcept
	{
		/* FIXME: Find a better way to stop the node controller! */
		m_nodeController.releaseNode();
		m_nodeController.disconnectDevice();

		inputManager.removeKeyboardListener(&m_nodeController);
	}

	std::shared_ptr< RenderTarget::View< ViewMatrices2DUBO > >
	Scene::createRenderToView (const std::string & name, uint32_t width, uint32_t height, const FramebufferPrecisions & precisions, float viewDistance, bool isOrthographicProjection, bool primaryDevice) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToViewAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} << "A virtual device named '" << name << "' already exists ! Render to view creation canceled ...";

			return {};
		}

		/* Create the render target.
		 * TODO: Get the view distance value from settings. */
		auto renderTarget = std::make_shared< RenderTarget::View< ViewMatrices2DUBO > >(name, width, height, precisions, viewDistance, isOrthographicProjection);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to view '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, primaryDevice) )
		{
			TraceError{ClassId} << "Unable to add the render to view '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToViews.emplace(renderTarget);

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::View< ViewMatrices3DUBO > >
	Scene::createRenderToCubicView (const std::string & name, uint32_t size, const FramebufferPrecisions & precisions, float viewDistance, bool isOrthographicProjection, bool primaryDevice) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToViewAccess};

		/* Checks name availability. */
		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} << "A virtual device named '" << name << "' already exists ! Render to cubic view creation canceled ...";

			return {};
		}

		/* Create the render target.
		 * TODO: Get the view distance value from settings. */
		auto renderTarget = std::make_shared< RenderTarget::View< ViewMatrices3DUBO > >(name, size, precisions, viewDistance, isOrthographicProjection);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to cubic view '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, primaryDevice) )
		{
			TraceError{ClassId} << "Unable to add the render to cubic view '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToViews.emplace(renderTarget);

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::Texture< ViewMatrices2DUBO > >
	Scene::createRenderToTexture2D (const std::string & name, uint32_t width, uint32_t height, uint32_t colorCount, float viewDistance, bool isOrthographicProjection) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToTextureAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to texture 2D creation canceled ...";

			return {};
		}

		/* Create the render target.
		 * TODO: Get the view distance value from settings. */
		auto renderTarget = std::make_shared< RenderTarget::Texture< ViewMatrices2DUBO > >(name, width, height, colorCount, viewDistance, isOrthographicProjection);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to texture 2D '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, false) )
		{
			TraceError{ClassId} << "Unable to add the render to texture 2D '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToTextures.emplace(renderTarget);

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::Texture< ViewMatrices3DUBO > >
	Scene::createRenderToCubemap (const std::string & name, uint32_t size, uint32_t colorCount, float viewDistance, bool isOrthographicProjection) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToTextureAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to cubemap creation canceled ...";

			return {};
		}

		/* Create the render target.
		 * TODO: Get the view distance value from settings. */
		auto renderTarget = std::make_shared< RenderTarget::Texture< ViewMatrices3DUBO > >(name, size, colorCount, viewDistance, isOrthographicProjection);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to cubemap '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, false) )
		{
			TraceError{ClassId} << "Unable to add the render to cubemap '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToTextures.emplace(renderTarget);

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::ShadowMap< ViewMatrices2DUBO > >
	Scene::createRenderToShadowMap (const std::string & name, uint32_t resolution, float viewDistance, bool isOrthographicProjection) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToShadowMapAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to shadow map creation canceled ...";

			return {};
		}

		/* Create the render target.
		 * TODO: Get the view distance value from settings. */
		auto renderTarget = std::make_shared< RenderTarget::ShadowMap< ViewMatrices2DUBO > >(name, resolution, viewDistance, isOrthographicProjection);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to shadow map '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, false) )
		{
			TraceError{ClassId} << "Unable to add the render to shadow map '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToShadowMaps.emplace(renderTarget);

		return renderTarget;
	}

	std::shared_ptr< RenderTarget::ShadowMap< ViewMatrices3DUBO > >
	Scene::createRenderToCubicShadowMap (const std::string & name, uint32_t resolution, float viewDistance, bool isOrthographicProjection) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderToShadowMapAccess};

		if ( m_AVConsoleManager.isVideoDeviceExists(name) )
		{
			TraceError{ClassId} <<
				"A virtual video device named '" << name << "' already exists ! "
				"Render to cubic shadow map creation canceled ...";

			return {};
		}

		/* Create the render target. */
		auto renderTarget = std::make_shared< RenderTarget::ShadowMap< ViewMatrices3DUBO > >(name, resolution, viewDistance, isOrthographicProjection);

		if ( !renderTarget->createRenderTarget(m_AVConsoleManager.graphicsRenderer()) )
		{
			TraceError{ClassId} << "Unable to create the render to cubic shadow map '" << name << "' !";

			return {};
		}

		if ( !m_AVConsoleManager.addVideoDevice(renderTarget, false) )
		{
			TraceError{ClassId} << "Unable to add the render to cubic shadow map '" << name << "' as a virtual video device !";

			return {};
		}

		m_renderToShadowMaps.emplace(renderTarget);

		return renderTarget;
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
		/* First, check the presence of the entity in the list. */
		const auto staticEntityIt = m_staticEntities.find(name);

		if ( staticEntityIt == m_staticEntities.end() )
		{
			return false;
		}

		const auto staticEntity = staticEntityIt->second;

		this->forget(staticEntity.get());

		if ( m_renderingOctree != nullptr && staticEntity->isRenderable() )
		{
			const std::lock_guard< std::mutex > lock{m_renderingOctreeAccess};

			m_renderingOctree->erase(staticEntity);
		}

		if ( m_physicsOctree != nullptr )
		{
			const std::lock_guard< std::mutex > lock{m_physicsOctreeAccess};

			m_physicsOctree->erase(staticEntity);
		}

		staticEntity->clearComponents();

		m_staticEntities.erase(staticEntityIt);

		return true;
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
			const std::lock_guard< std::mutex > lock{m_renderingOctreeAccess};

			m_renderingOctree.reset();
		}

		if ( m_physicsOctree != nullptr )
		{
			const std::lock_guard< std::mutex > lock{m_physicsOctreeAccess};

			m_physicsOctree.reset();
		}
	}

	bool
	Scene::rebuildRenderingOctree (bool keepElements) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_renderingOctreeAccess};

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
		const std::lock_guard< std::mutex > lock{m_physicsOctreeAccess};

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
			const std::lock_guard< std::mutex > lockGuard{m_renderingOctreeAccess};

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
			const std::lock_guard< std::mutex > lock{m_physicsOctreeAccess};

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
		const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

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
			const std::lock_guard< std::mutex > lock{m_staticEntitiesAccess};

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
			const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

			NodeCrawler< Node > crawler{m_rootNode};

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
#if ENABLE_NEW_PHYSICS_SYSTEM
			/* [PHYSICS-NEW-SYSTEM]
			 * New impulse-based collision resolution system.
			 * FIXME: Poor memory management ! */
			std::vector< ContactManifold > manifolds;

			/* Detect collisions and generate contact manifolds. */
			this->sectorCollisionTest(*m_physicsOctree, manifolds);
#else
			/* [PHYSICS-OLD-SYSTEM] Legacy collision detection. */
			std::vector< ContactManifold > manifolds;

			this->sectorCollisionTest(*m_physicsOctree, manifolds);
#endif

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
					if ( entity->collisionDetectionModel() == CollisionDetectionModel::Sphere )
					{
						/* [PHYSICS-NEW-SYSTEM] Pass manifolds vector for boundary collision handling. */
						this->clipWithBoundingSphere(entity, manifolds);
					}
					else
					{
						/* [PHYSICS-NEW-SYSTEM] Pass manifolds vector for boundary collision handling. */
						this->clipWithBoundingBox(entity, manifolds);
					}
				}

				/* Reset collider and resolve collisions. */
				if ( auto & collider = movableEntity->collider(); collider.hasCollisions() )
				{
#if ENABLE_NEW_PHYSICS_SYSTEM
					/* [PHYSICS-NEW-SYSTEM] Do NOT resolve collisions here.
					 * All collisions (dynamic AND boundaries) are handled by the ConstraintSolver via impulses.
					 * Boundary hard clipping is still performed (prevents leaving world cube),
					 * but velocity correction is done through manifolds by the solver. */
					collider.reset();  // Clear collision list without resolving
#else
					/* [PHYSICS-OLD-SYSTEM] Resolve all collisions with position correction. */
					collider.resolveCollisions(*entity);
#endif
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

#if ENABLE_NEW_PHYSICS_SYSTEM
			/* Resolve contact constraints with Sequential Impulse Solver. */
			if ( !manifolds.empty() )
			{
				/* FIXME: Invalid deltaTime. */
				m_constraintSolver.solve(manifolds, EngineUpdateCycleDurationS< float >);
			}
#endif
		}

		m_cycle++;
	}

	void
	Scene::publishStateForRendering () noexcept
	{
		/* TODO: Check to copy only relevant data to speed up the transfer. */
		const uint32_t nextTarget = m_renderStateIndex == 0 ? 1 : 0;

		/* Synchronize static entities. */
		for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
		{
			staticEntity->publishStateForRendering(nextTarget);
		}

		/* Synchronize scene nodes. */
		{
			NodeCrawler< Node > crawler{m_rootNode};

			std::shared_ptr< Node > currentNode;

			while ( (currentNode = crawler.nextNode()) != nullptr )
			{
				currentNode->publishStateForRendering(nextTarget);
			}
		}

		/* Synchronize render targets. */
		{
			this->forEachRenderToShadowMap([&nextTarget] (const auto & renderTarget){
			   renderTarget->viewMatrices().publishStateForRendering(nextTarget);
			});

			this->forEachRenderToTexture([&nextTarget] (const auto & renderTarget){
				renderTarget->viewMatrices().publishStateForRendering(nextTarget);
			});

			this->forEachRenderToView([&nextTarget] (const auto & renderTarget){
				renderTarget->viewMatrices().publishStateForRendering(nextTarget);
			});
		}

		/* NOTE: Declare the new target to read from for the rendering thread. */
		m_renderStateIndex.store(nextTarget, std::memory_order_release);
	}

	void
	Scene::castShadows (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		const uint32_t readStateIndex = m_renderStateIndex.load(std::memory_order_acquire);

		if ( !m_lightSet.isEnabled() )
		{
			return;
		}

		/* Sort the scene according to the point of view. */
		if ( !this->populateShadowCastingRenderList(renderTarget, readStateIndex) )
		{
			/* There is nothing to shadow to cast ... */
			return;
		}

		/*TraceDebug{ClassId} <<
			"Shadow map content :" "\n"
			" - Plain objects : " << m_renderLists[Shadows].size() << "\n";*/

		for ( const auto & renderBatch : m_renderLists[Shadows] | std::views::values )
		{
			renderBatch.renderableInstance()->castShadows(readStateIndex, renderTarget, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer);
		}
	}

	void
	Scene::render (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer) noexcept
	{
		const uint32_t readStateIndex = m_renderStateIndex.load(std::memory_order_acquire);

		//std::cout << "Rendering from render state #" << readStateIndex << "... " "\n";

		/* Sort the scene according to the point of view. */
		if ( !this->populateRenderLists(renderTarget, readStateIndex) )
		{
			return;
		}

		/*TraceDebug{ClassId} <<
			"Frame content :" "\n"
			" - Opaque / +lighted : " << m_renderLists[Opaque].size() << " / " << m_renderLists[OpaqueLighted].size() << "\n"
			" - Translucent / +lighted : " << m_renderLists[Translucent].size() << " / " << m_renderLists[TranslucentLighted].size() << "\n";*/

		/* First, we render all opaque renderable objects. */
		{
			if ( !m_renderLists[Opaque].empty() )
			{
				for ( const auto & renderBatch : m_renderLists[Opaque] | std::views::values )
				{
					renderBatch.renderableInstance()->render(readStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer);
				}
			}

			if ( m_lightSet.isEnabled() && !m_renderLists[OpaqueLighted].empty() )
			{
				this->renderLightedSelection(renderTarget, readStateIndex, commandBuffer, m_renderLists[OpaqueLighted]);
			}
		}

		/* After, we render all translucent renderable objects. */
		{
			if ( !m_renderLists[Translucent].empty() )
			{
				for ( const auto & renderBatch : m_renderLists[Translucent] | std::views::values )
				{
					renderBatch.renderableInstance()->render(readStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer);
				}
			}

			if ( m_lightSet.isEnabled() && !m_renderLists[TranslucentLighted].empty() )
			{
				this->renderLightedSelection(renderTarget, readStateIndex, commandBuffer, m_renderLists[TranslucentLighted]);
			}
		}

		/* Optional rendering.
		 * FIXME: Add a main control. */
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
	Scene::applyModifiers (Node & node) const noexcept
	{
		this->forEachModifiers([&node] (const auto & modifier) {
			/* NOTE: Avoid working on the same Node. */
			if ( &node == &modifier.parentEntity() )
			{
				return;
			}

			/* FIXME: Use AABB when usable */
			const auto modifierForce = modifier.getForceAppliedToEntity(node.getWorldCoordinates(), node.getWorldBoundingSphere());

			node.addForce(modifierForce);
		});
	}

	void
	Scene::sectorCollisionTest (const OctreeSector< AbstractEntity, true > & sector, std::vector< ContactManifold > & manifolds) noexcept
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
				this->sectorCollisionTest(*subSector, manifolds);
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

			/* Copy the iterator to iterate through the next elements with it without modify the initial one. */
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
#if ENABLE_NEW_PHYSICS_SYSTEM
						/* [PHYSICS-NEW-SYSTEM] Generate contact manifolds for impulse-based resolution. */
						colliderA.checkCollisionAgainstMovableWithManifold(*entityA, *entityB, manifolds);
#else
						/* [PHYSICS-OLD-SYSTEM] Store collisions for position-based correction. */
						colliderA.checkCollisionAgainstMovable(*entityA, *entityB);
#endif
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

#if ENABLE_NEW_PHYSICS_SYSTEM
	void
	Scene::clipWithBoundingSphere (const std::shared_ptr< AbstractEntity > & entity, std::vector< ContactManifold > & manifolds) const noexcept
	{
		const auto worldCoordinates = entity->getWorldCoordinates();
		const auto & worldPosition = worldCoordinates.position();

		if ( m_sceneAreaResource != nullptr )
		{
			const auto groundLevel = m_sceneAreaResource->getLevelAt(worldPosition);
			const auto minimalYPosition = worldPosition[Y] + entity->getWorldBoundingSphere().radius();

			if ( minimalYPosition >= groundLevel )
			{
				/* Create manifold for velocity correction (bounce). */
				const auto penetrationDepth = minimalYPosition - groundLevel;
				const auto normal = m_sceneAreaResource->getNormalAt(worldPosition);
				// Contact point: move from sphere center along normal (pointing downward from A to B)
				const auto contactPoint = worldPosition + normal * entity->getWorldBoundingSphere().radius();

				ContactManifold manifold{entity->getMovableTrait()}; // Ground = infinite mass static entity
				manifold.addContact(contactPoint, normal, penetrationDepth);
				manifolds.push_back(manifold);

				entity->setYPosition(groundLevel - entity->getWorldBoundingSphere().radius(), TransformSpace::World);
			}
		}

		/* Compute the max boundary. */
		const auto boundaryLimit = m_boundary - entity->getWorldBoundingSphere().radius();

		/* X-Axis test. */
		if ( std::abs(worldPosition[X]) > boundaryLimit )
		{
			if ( worldPosition[X] > boundaryLimit )
			{
				/* [ALWAYS] Hard clipping - prevents entity from leaving the world cube. */
				entity->setXPosition(boundaryLimit, TransformSpace::World);

				/* [PHYSICS-NEW-SYSTEM] Create manifold for velocity correction (bounce). */
				const auto penetrationDepth = worldPosition[X] - boundaryLimit;
				const Vector< 3, float > contactPoint{boundaryLimit, worldPosition[Y], worldPosition[Z]};

				ContactManifold manifold{entity->getMovableTrait()}; // World boundary = infinite mass
				manifold.addContact(contactPoint, Vector< 3, float >::negativeX(), penetrationDepth);
				manifolds.push_back(manifold);
			}
			else
			{
				/* [ALWAYS] Hard clipping - prevents entity from leaving the world cube. */
				entity->setXPosition(-boundaryLimit, TransformSpace::World);

				/* [PHYSICS-NEW-SYSTEM] Create manifold for velocity correction (bounce). */
				const auto penetrationDepth = -boundaryLimit - worldPosition[X];
				const Vector< 3, float > contactPoint{-boundaryLimit, worldPosition[Y], worldPosition[Z]};

				ContactManifold manifold{entity->getMovableTrait()}; // World boundary = infinite mass
				manifold.addContact(contactPoint, Vector< 3, float >::positiveX(), penetrationDepth);
				manifolds.push_back(manifold);
			}
		}

		/* Y-Axis test. */
		if ( std::abs(worldPosition[Y]) > boundaryLimit )
		{
			if ( worldPosition[Y] > boundaryLimit )
			{
				/* [ALWAYS] Hard clipping - prevents entity from leaving the world cube. */
				entity->setYPosition(boundaryLimit, TransformSpace::World);

				/* [PHYSICS-NEW-SYSTEM] Create manifold for velocity correction (bounce). */
				const auto penetrationDepth = worldPosition[Y] - boundaryLimit;
				const Vector< 3, float > contactPoint{worldPosition[X], boundaryLimit, worldPosition[Z]};

				ContactManifold manifold{entity->getMovableTrait()}; // World boundary = infinite mass
				manifold.addContact(contactPoint, Vector< 3, float >::negativeY(), penetrationDepth);
				manifolds.push_back(manifold);
			}
			else
			{
				auto * movableTrait = entity->getMovableTrait();

				/* [PHYSICS-NEW-SYSTEM] Hard clipping and manifold creation - but only if falling.
				 * This allows ascending while preventing falling through the bottom boundary. */
				if ( movableTrait->linearVelocity()[Y] >= 0.0F )
				{
					entity->setYPosition(-boundaryLimit, TransformSpace::World);

					/* Create manifold for velocity correction (bounce). */
					const auto penetrationDepth = -boundaryLimit - worldPosition[Y];
					const Vector< 3, float > contactPoint{worldPosition[X], -boundaryLimit, worldPosition[Z]};

					ContactManifold manifold{movableTrait}; // World boundary = infinite mass
					manifold.addContact(contactPoint, Vector< 3, float >::positiveY(), penetrationDepth);
					manifolds.push_back(manifold);
				}
			}
		}

		/* Z-Axis test. */
		if ( std::abs(worldPosition[Z]) > boundaryLimit )
		{
			if ( worldPosition[Z] > boundaryLimit )
			{
				/* [ALWAYS] Hard clipping - prevents entity from leaving the world cube. */
				entity->setZPosition(boundaryLimit, TransformSpace::World);

				/* [PHYSICS-NEW-SYSTEM] Create manifold for velocity correction (bounce). */
				const auto penetrationDepth = worldPosition[Z] - boundaryLimit;
				const Vector< 3, float > contactPoint{worldPosition[X], worldPosition[Y], boundaryLimit};

				ContactManifold manifold{entity->getMovableTrait()}; // World boundary = infinite mass
				manifold.addContact(contactPoint, Vector< 3, float >::negativeZ(), penetrationDepth);
				manifolds.push_back(manifold);
			}
			else
			{
				/* [ALWAYS] Hard clipping - prevents entity from leaving the world cube. */
				entity->setZPosition(-boundaryLimit, TransformSpace::World);

				/* [PHYSICS-NEW-SYSTEM] Create manifold for velocity correction (bounce). */
				const auto penetrationDepth = -boundaryLimit - worldPosition[Z];
				const Vector< 3, float > contactPoint{worldPosition[X], worldPosition[Y], -boundaryLimit};

				ContactManifold manifold{entity->getMovableTrait()}; // World boundary = infinite mass
				manifold.addContact(contactPoint, Vector< 3, float >::positiveZ(), penetrationDepth);
				manifolds.push_back(manifold);
			}
		}
	}

	void
	Scene::clipWithBoundingBox (const std::shared_ptr< AbstractEntity > & entity, std::vector< ContactManifold > & manifolds) const noexcept
	{
		const auto worldCoordinates = entity->getWorldCoordinates();
		const auto & worldPosition = worldCoordinates.position();
		const auto AABB = entity->getWorldBoundingBox();

		/* Running the subtest first. */
		if ( m_sceneAreaResource != nullptr )
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
				const auto groundLevel = m_sceneAreaResource->getLevelAt(position);
				const auto distance = groundLevel - position[Y];

				if ( distance <= highestDistance )
				{
					collisionPosition = &position;
					highestDistance = distance;
				}
			}

			if ( collisionPosition != nullptr )
			{
				auto * movableTrait = entity->getMovableTrait();

				/* [PHYSICS-NEW-SYSTEM] Hard clipping and manifold creation - but only if falling.
				 * This allows jumping/ascending while preventing falling through. */
				if ( movableTrait->linearVelocity()[Y] >= 0.0F )
				{
					entity->moveY(highestDistance, TransformSpace::World);

					/* Create manifold for velocity correction (bounce). */
					const auto penetrationDepth = -highestDistance; // highestDistance is negative when penetrating
					auto contactPoint = *collisionPosition;
					contactPoint[Y] += highestDistance; // Move to surface
					const auto normal = m_sceneAreaResource->getNormalAt(*collisionPosition);

					ContactManifold manifold{movableTrait}; // Ground = infinite mass static entity
					manifold.addContact(contactPoint, normal, penetrationDepth);
					manifolds.push_back(manifold);
				}
			}
		}

		/* X-Axis test */
		if ( AABB.maximum(X) > m_boundary )
		{
			const auto delta = AABB.maximum(X) - m_boundary;

			/* [ALWAYS] Hard clipping - prevents entity from leaving the world cube. */
			entity->moveX(-delta, TransformSpace::World);

			/* [PHYSICS-NEW-SYSTEM] Create manifold for velocity correction (bounce). */
			const auto penetrationDepth = delta;
			const Vector< 3, float > contactPoint{m_boundary, worldPosition[Y], worldPosition[Z]};

			ContactManifold manifold{entity->getMovableTrait()}; // World boundary = infinite mass
			manifold.addContact(contactPoint, Vector< 3, float >::negativeX(), penetrationDepth);
			manifolds.push_back(manifold);
		}
		else if ( AABB.minimum(X) < -m_boundary )
		{
			const auto delta = std::abs(AABB.minimum(X)) - m_boundary;

			/* [ALWAYS] Hard clipping - prevents entity from leaving the world cube. */
			entity->moveX(delta, TransformSpace::World);

			/* [PHYSICS-NEW-SYSTEM] Create manifold for velocity correction (bounce). */
			const auto penetrationDepth = delta;
			const Vector< 3, float > contactPoint{-m_boundary, worldPosition[Y], worldPosition[Z]};

			ContactManifold manifold{entity->getMovableTrait()}; // World boundary = infinite mass
			manifold.addContact(contactPoint, Vector< 3, float >::positiveX(), penetrationDepth);
			manifolds.push_back(manifold);
		}

		/* Y-Axis test */
		if ( AABB.maximum(Y) > m_boundary )
		{
			const auto delta = AABB.maximum(Y) - m_boundary;

			/* [ALWAYS] Hard clipping - prevents entity from leaving the world cube. */
			entity->moveY(-delta, TransformSpace::World);

			/* [PHYSICS-NEW-SYSTEM] Create manifold for velocity correction (bounce). */
			const auto penetrationDepth = delta;
			const Vector< 3, float > contactPoint{worldPosition[X], m_boundary, worldPosition[Z]};

			ContactManifold manifold{entity->getMovableTrait()}; // World boundary = infinite mass
			manifold.addContact(contactPoint, Vector< 3, float >::negativeY(), penetrationDepth);
			manifolds.push_back(manifold);
		}
		else if ( AABB.minimum(Y) <= -m_boundary )
		{
			const auto delta = std::abs(AABB.minimum(Y)) - m_boundary;

			auto * movableTrait = entity->getMovableTrait();

			/* [PHYSICS-NEW-SYSTEM] Hard clipping and manifold creation - but only if falling.
			 * This allows ascending while preventing falling through the bottom boundary. */
			if ( movableTrait->linearVelocity()[Y] >= 0.0F )
			{
				entity->moveY(delta, TransformSpace::World);

				/* Create manifold for velocity correction (bounce). */
				const auto penetrationDepth = delta;
				const Vector< 3, float > contactPoint{worldPosition[X], -m_boundary, worldPosition[Z]};

				ContactManifold manifold{movableTrait}; // World boundary = infinite mass
				manifold.addContact(contactPoint, Vector< 3, float >::positiveY(), penetrationDepth);
				manifolds.push_back(manifold);
			}
		}

		/* Z-Axis test */
		if ( AABB.maximum(Z) > m_boundary )
		{
			const auto delta = AABB.maximum(Z) - m_boundary;

			/* [ALWAYS] Hard clipping - prevents entity from leaving the world cube. */
			entity->moveZ(-delta, TransformSpace::World);

			/* [PHYSICS-NEW-SYSTEM] Create manifold for velocity correction (bounce). */
			const auto penetrationDepth = delta;
			const Vector< 3, float > contactPoint{worldPosition[X], worldPosition[Y], m_boundary};

			ContactManifold manifold{entity->getMovableTrait()}; // World boundary = infinite mass
			manifold.addContact(contactPoint, Vector< 3, float >::negativeZ(), penetrationDepth);
			manifolds.push_back(manifold);
		}
		else if ( AABB.minimum(Z) < -m_boundary )
		{
			const auto delta = std::abs(AABB.minimum(Z)) - m_boundary;

			/* [ALWAYS] Hard clipping - prevents entity from leaving the world cube. */
			entity->moveZ(delta, TransformSpace::World);

			/* [PHYSICS-NEW-SYSTEM] Create manifold for velocity correction (bounce). */
			const auto penetrationDepth = delta;
			const Vector< 3, float > contactPoint{worldPosition[X], worldPosition[Y], -m_boundary};

			ContactManifold manifold{entity->getMovableTrait()}; // World boundary = infinite mass
			manifold.addContact(contactPoint, Vector< 3, float >::positiveZ(), penetrationDepth);
			manifolds.push_back(manifold);
		}
	}
#else
	void
	Scene::clipWithBoundingSphere (const std::shared_ptr< AbstractEntity > & entity, std::vector< ContactManifold > & /*manifolds*/) const noexcept
	{
		const auto worldCoordinates = entity->getWorldCoordinates();
		const auto & worldPosition = worldCoordinates.position();

		auto & collider = entity->getMovableTrait()->collider();

		if ( m_sceneAreaResource != nullptr )
		{
			const auto groundLevel = m_sceneAreaResource->getLevelAt(worldPosition) - entity->getWorldBoundingSphere().radius();

			if ( worldPosition[Y] >= groundLevel )
			{
				entity->setYPosition(groundLevel, TransformSpace::World);

				collider.addCollision(CollisionType::SceneGround, nullptr, worldPosition, m_sceneAreaResource->getNormalAt(worldPosition));
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
	Scene::clipWithBoundingBox (const std::shared_ptr< AbstractEntity > & entity, std::vector< ContactManifold > & /*manifolds*/) const noexcept
	{
		const auto worldCoordinates = entity->getWorldCoordinates();
		const auto & worldPosition = worldCoordinates.position();
		const auto AABB = entity->getWorldBoundingBox();

		auto & collider = entity->getMovableTrait()->collider();

		/* Running the subtest first. */
		if ( m_sceneAreaResource != nullptr )
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
				const auto groundLevel = m_sceneAreaResource->getLevelAt(position);
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

				collider.addCollision(CollisionType::SceneGround, nullptr, *collisionPosition, m_sceneAreaResource->getNormalAt(*collisionPosition));
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
#endif

	void
	Scene::insertIntoShadowCastingRenderList (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, const CartesianFrame< float > * worldCoordinates, float distance) noexcept
	{
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
			RenderBatch::create(m_renderLists[Shadows], distance, renderableInstance, worldCoordinates, layerIndex);
		}
	}

	void
	Scene::insertIntoRenderLists (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, const CartesianFrame< float > * worldCoordinates, float distance) noexcept
	{
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
					RenderBatch::create(m_renderLists[OpaqueLighted], distance, renderableInstance, worldCoordinates, layerIndex);
				}
				else
				{
					RenderBatch::create(m_renderLists[TranslucentLighted], distance * -1.0F, renderableInstance, worldCoordinates, layerIndex);
				}
			}
			else
			{
				if ( isOpaque )
				{
					RenderBatch::create(m_renderLists[Opaque], distance, renderableInstance, worldCoordinates, layerIndex);
				}
				else
				{
					RenderBatch::create(m_renderLists[Translucent], distance * -1.0F, renderableInstance, worldCoordinates, layerIndex);
				}
			}
		}
	}

	bool
	Scene::checkRenderableInstanceForShadowCasting (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) const noexcept
	{
		/* Check whether the renderable instance is ready for shadow casting. */
		if ( renderableInstance->isReadyToCastShadows(renderTarget) )
		{
			return false; // Render
		}

		/* If it still unloaded. */
		if ( !renderableInstance->renderable()->isReadyForInstantiation() )
		{
			return true; // Continue
		}

		if ( this->getRenderableInstanceReadyForShadowCasting(renderableInstance, renderTarget) )
		{
			return false; // Render
		}

		/* If the object cannot be loaded, mark it as broken! */
		renderableInstance->setBroken("Unable to get ready for shadow casting !");

		return true; // Continue
	}

	bool
	Scene::populateShadowCastingRenderList (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t readStateIndex) noexcept
	{
		/* NOTE: Clean the render list before. */
		m_renderLists[Shadows].clear();

		/* NOTE: The camera position doesn't move during calculation. */
		const auto & cameraPosition = renderTarget->viewMatrices().position();
		const auto & frustum = renderTarget->viewMatrices().frustum(0);
		const auto viewDistance = renderTarget->viewDistance();

		for ( const auto & component : m_sceneVisualComponents )
		{
			if ( component == nullptr )
			{
				continue;
			}

			const auto renderableInstance = component->getRenderableInstance();

			if ( renderableInstance == nullptr )
			{
				continue;
			}

			if ( this->checkRenderableInstanceForShadowCasting(renderTarget, renderableInstance) )
			{
				continue;
			}

			this->insertIntoShadowCastingRenderList(renderableInstance, nullptr, 0.0F);
		}

		/* Sorting renderable objects from scene static entities. */
		{
			const std::lock_guard< std::mutex > lock{m_staticEntitiesAccess};

			for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
			{
				/* Check whether the static entity contains something to render. */
				if ( !staticEntity->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = staticEntity->getWorldCoordinatesStateForRendering(readStateIndex);

				staticEntity->forEachComponent([&] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					if ( this->checkRenderableInstanceForShadowCasting(renderTarget, renderableInstance) )
					{
						return;
					}

					/* Render-target distance check and frustum culling check. */
					const auto distance = Vector< 3, float >::distance(cameraPosition, worldCoordinates.position());

					if ( distance > viewDistance || !staticEntity->isVisibleTo(frustum) )
					{
						return;
					}

					this->insertIntoShadowCastingRenderList(renderableInstance, &worldCoordinates, distance);
				});
			}
		}

		/* Sorting renderable objects from the scene node tree. */
		{
			/* NOTE: Prevent scene node deletion from the logic update thread to crash the rendering. */
			const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

			NodeCrawler< const Node > crawler{m_rootNode};

			std::shared_ptr< const Node > node;

			while ( (node = crawler.nextNode()) != nullptr )
			{
				/* Check whether the scene node contains something to render. */
				if ( !node->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = node->getWorldCoordinatesStateForRendering(readStateIndex);

				node->forEachComponent([&] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					if ( this->checkRenderableInstanceForShadowCasting(renderTarget, renderableInstance) )
					{
						return;
					}

					/* Render-target distance check and frustum culling check. */
					const auto distance = Vector< 3, float >::distance(cameraPosition, worldCoordinates.position());

					if ( distance > viewDistance || !node->isVisibleTo(frustum) )
					{
						return;
					}

					this->insertIntoShadowCastingRenderList(renderableInstance, &worldCoordinates, distance);
				});
			}
		}

		/* Return true if something can be rendered. */
		return !m_renderLists[Shadows].empty();
	}

	bool
	Scene::checkRenderableInstanceForRendering (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) const noexcept
	{
		/* Check whether the renderable instance is ready for shadow casting. */
		if ( renderableInstance->isReadyToRender(renderTarget) )
		{
			return false; // Render
		}

		/* If it still unloaded. */
		if ( !renderableInstance->renderable()->isReadyForInstantiation() )
		{
			return true; // Continue
		}

		if ( this->getRenderableInstanceReadyForRendering(renderableInstance, renderTarget) )
		{
			return false; // Render
		}

		/* If the object cannot be loaded, mark it as broken! */
		renderableInstance->setBroken("Unable to get ready for rendering !");

		return true; // Continue
	}

	bool
	Scene::populateRenderLists (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t readStateIndex) noexcept
	{
		/* NOTE: Clean render lists before. */
		m_renderLists[Opaque].clear();
		m_renderLists[Translucent].clear();
		m_renderLists[OpaqueLighted].clear();
		m_renderLists[TranslucentLighted].clear();

		/* NOTE: The camera position doesn't move during calculation. */
		const auto & cameraPosition = renderTarget->viewMatrices().position();
		const auto & frustum = renderTarget->viewMatrices().frustum(0);
		const auto viewDistance = renderTarget->viewDistance();

		for ( const auto & component : m_sceneVisualComponents )
		{
			if ( component == nullptr )
			{
				continue;
			}

			const auto renderableInstance = component->getRenderableInstance();

			if ( renderableInstance == nullptr )
			{
				continue;
			}

			if ( this->checkRenderableInstanceForRendering(renderTarget, renderableInstance) )
			{
				continue;
			}

			/* NOTE: Scene visual is the skybox or the ground, frustum culling step is not relevant here. */

			this->insertIntoRenderLists(renderableInstance, nullptr, 0.0F);
		}

		/* Sorting renderable objects from scene static entities. */
		{
			const std::lock_guard< std::mutex > lock{m_staticEntitiesAccess};

			for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
			{
				/* Check whether the static entity contains something to render. */
				if ( !staticEntity->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = staticEntity->getWorldCoordinatesStateForRendering(readStateIndex);

				staticEntity->forEachComponent([&] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					if ( this->checkRenderableInstanceForRendering(renderTarget, renderableInstance) )
					{
						return;
					}

					/* Render-target distance check and frustum culling check. */
					const auto distance = Vector< 3, float >::distance(cameraPosition, worldCoordinates.position());

					if ( distance > viewDistance || !staticEntity->isVisibleTo(frustum) )
					{
						return;
					}

					this->insertIntoRenderLists(renderableInstance, &worldCoordinates, distance);
				});
			}
		}

		/* Sorting renderable objects from the scene node tree. */
		{
			/* NOTE: Prevent scene node deletion from the logic update thread to crash the rendering. */
			const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

			NodeCrawler< const Node > crawler{m_rootNode};

			std::shared_ptr< const Node > node;

			while ( (node = crawler.nextNode()) != nullptr )
			{
				/* Check whether the scene node contains something to render. */
				if ( !node->isRenderable() )
				{
					continue;
				}

				const auto & worldCoordinates = node->getWorldCoordinatesStateForRendering(readStateIndex);

				node->forEachComponent([&] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					if ( this->checkRenderableInstanceForRendering(renderTarget, renderableInstance) )
					{
						return;
					}

					/* Render-target distance check and frustum culling check. */
					const auto distance = Vector< 3, float >::distance(cameraPosition, worldCoordinates.position());

					if ( distance > viewDistance || !node->isVisibleTo(frustum) )
					{
						return;
					}

					this->insertIntoRenderLists(renderableInstance, &worldCoordinates, distance);
				});
			}
		}

		/* Return true if something can be rendered. */
		constexpr std::array< uint32_t, 4 > objectTypes{Opaque, Translucent, OpaqueLighted, TranslucentLighted};

		return std::ranges::any_of(objectTypes, [&] (uint32_t objectType) {
			return !m_renderLists[objectType].empty();
		});
	}

	void
	Scene::updateVideoMemory (bool shadowMapEnabled, bool renderToTextureEnabled) const noexcept
	{
		const uint32_t readStateIndex = m_renderStateIndex.load(std::memory_order_acquire);

		if ( shadowMapEnabled && !m_renderToShadowMaps.empty() )
		{
			this->forEachRenderToShadowMap([readStateIndex] (const auto & renderTarget) {
				if ( !renderTarget->viewMatrices().updateVideoMemory(readStateIndex) )
				{
					TraceError{ClassId} << "Failed to update the video memory of the render target (Shadow map) from readStateIndex #" << readStateIndex << " !";
				}
			});
		}

		if ( renderToTextureEnabled && !m_renderToTextures.empty() )
		{
			this->forEachRenderToTexture([readStateIndex] (const auto & renderTarget) {
				if ( !renderTarget->viewMatrices().updateVideoMemory(readStateIndex) )
				{
					TraceError{ClassId} << "Failed to update the video memory of the render target (Texture) from readStateIndex #" << readStateIndex << " !";
				}
			});
		}

		/* NOTE: There should be at least the swap chain! */
		this->forEachRenderToView([readStateIndex] (const auto & renderTarget) {
			if ( !renderTarget->viewMatrices().updateVideoMemory(readStateIndex) )
			{
				TraceError{ClassId} << "Failed to update the video memory of the render target (View) from readStateIndex #" << readStateIndex << " !";
			}
		});

		if ( !m_lightSet.updateVideoMemory() )
		{
			Tracer::error(ClassId, "Unable to update the light set data to the video memory !");
		}
	}

	void
	Scene::renderLightedSelection (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t readStateIndex, const Vulkan::CommandBuffer & commandBuffer, const RenderBatch::List & renderBatches) const noexcept
	{
		if ( m_lightSet.isUsingStaticLighting() )
		{
			for ( const auto & renderBatch : renderBatches | std::views::values )
			{
				renderBatch.renderableInstance()->render(readStateIndex, renderTarget, nullptr, RenderPassType::SimplePass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer);
			}

			return;
		}

		/* For all objects. */
		for ( const auto & renderBatch : renderBatches | std::views::values )
		{
			const std::lock_guard< std::mutex > lock{m_lightSet.mutex()};

			/* Ambient pass. */
			renderBatch.renderableInstance()->render(readStateIndex, renderTarget, nullptr, RenderPassType::AmbientPass, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer);

			/* Loop through all directional lights. */
			for ( const auto & light : m_lightSet.directionalLights() )
			{
				if ( !light->isEnabled() )
				{
					continue;
				}

				renderBatch.renderableInstance()->render(readStateIndex, renderTarget, light.get(), RenderPassType::DirectionalLightPassNoShadow, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer);
			}

			/* Loop through all point lights. */
			for ( const auto & light : m_lightSet.pointLights() )
			{
				if ( !light->isEnabled() )
				{
					continue;
				}

				const auto & instance = renderBatch.renderableInstance();

				/* NOTE: If a light distance check is needed. */
				if ( instance->isLightDistanceCheckEnabled() )
				{
					const auto * worldCoordinates = renderBatch.worldCoordinates();

					if ( worldCoordinates != nullptr && !light->touch(worldCoordinates->position()) )
					{
						continue;
					}
				}

				instance->render(readStateIndex, renderTarget, light.get(), RenderPassType::PointLightPassNoShadow, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer);
			}

			/* Loop through all spotlights. */
			for ( const auto & light : m_lightSet.spotLights() )
			{
				if ( !light->isEnabled() )
				{
					continue;
				}

				const auto & instance = renderBatch.renderableInstance();

				/* NOTE: If a light distance check is needed. */
				if ( instance->isLightDistanceCheckEnabled() )
				{
					const auto * worldCoordinates = renderBatch.worldCoordinates();

					if ( worldCoordinates != nullptr && !light->touch(worldCoordinates->position()) )
					{
						continue;
					}
				}

				renderBatch.renderableInstance()->render(readStateIndex, renderTarget, light.get(), RenderPassType::SpotLightPassNoShadow, renderBatch.subGeometryIndex(), renderBatch.worldCoordinates(), commandBuffer);
			}
		}
	}

	void
	Scene::checkAVConsoleNotification (int notificationCode, const std::any & data) noexcept
	{
		switch ( notificationCode )
		{
			case AVConsole::Manager::VideoDeviceAdded :
				TraceDebug{ClassId} << "A new video device is available for the scene.";
				break;

			case AVConsole::Manager::VideoDeviceRemoved :
			{
				const auto device = std::any_cast< const std::shared_ptr< AVConsole::AbstractVirtualDevice > >(data);

				if ( const auto renderTarget = std::dynamic_pointer_cast< RenderTarget::Abstract >(device) )
				{
					const std::scoped_lock lock{m_renderToShadowMapAccess, m_renderToTextureAccess, m_renderToViewAccess};

					/* NOTE: If the conversion is successful, renderTarget is not null. */
					m_renderToViews.erase(renderTarget);
					m_renderToTextures.erase(renderTarget);
					m_renderToShadowMaps.erase(renderTarget);
				}

				TraceDebug{ClassId} << "A video device has been removed from the scene.";
			}
				break;

			case AVConsole::Manager::AudioDeviceAdded :
				TraceDebug{ClassId} << "A new audio device is available for the scene.";
				break;

			case AVConsole::Manager::AudioDeviceRemoved :
				TraceDebug{ClassId} << "An audio device has been removed from the scene.";
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
					const std::lock_guard< std::mutex > lockGuard{m_renderingOctreeAccess};

					m_renderingOctree->erase(node);
				}

				if ( m_physicsOctree != nullptr )
				{
					const std::lock_guard< std::mutex > lock{m_physicsOctreeAccess};

					m_physicsOctree->erase(node);
				}
			}
				return true;

			case Node::SubNodeDeleted :
				return true;

			default:
				if constexpr ( ObserverDebugEnabled )
				{
					TraceDebug{ClassId} << "Event #" << notificationCode << " from a Node ignored.";
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
				m_lightSet.add(*this, std::any_cast< std::shared_ptr< Component::DirectionalLight > >(data));

				return true;

			case AbstractEntity::DirectionalLightDestroyed :
				m_lightSet.remove(*this, std::any_cast< std::shared_ptr< Component::DirectionalLight > >(data));

				return true;

			case AbstractEntity::PointLightCreated :
				m_lightSet.add(*this, std::any_cast< std::shared_ptr< Component::PointLight > >(data));

				return true;

			case AbstractEntity::PointLightDestroyed :
				m_lightSet.remove(*this, std::any_cast< std::shared_ptr< Component::PointLight > >(data));

				return true;

			case AbstractEntity::SpotLightCreated :
				m_lightSet.add(*this, std::any_cast< std::shared_ptr< Component::SpotLight > >(data));

				return true;

			case AbstractEntity::SpotLightDestroyed :
				m_lightSet.remove(*this, std::any_cast< std::shared_ptr< Component::SpotLight > >(data));

				return true;

			default:
				if constexpr ( ObserverDebugEnabled )
				{
					TraceDebug{ClassId} << "Event #" << notificationCode << " from an entity component ignored.";
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

		if ( observable->is(StaticEntity::getClassUID()) )
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

		if ( observable->is(Node::getClassUID()) )
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

		/* NOTE: Don't know what it is, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
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
				if ( !this->getRenderableInstanceReadyForRendering(renderableInstance, renderTarget) )
				{
					TraceError{ClassId} << "The initialization of renderable instance '" << renderableInstance->renderable()->name() << "' from render target '" << renderTarget->id() << "' has failed !";
				}

				return true;
			});
		}
	}

	bool
	Scene::refreshRenderableInstances () const noexcept
	{
		uint32_t errorCount = 0;

		this->forEachRenderableInstance([&] (const auto & renderableInstance) {
			TraceDebug{ClassId} << "Refreshing renderable '" << renderableInstance->renderable()->name() << "' ...";

			this->forEachRenderToShadowMap([&errorCount, &renderableInstance] (const auto & renderTarget) {
				if ( !renderableInstance->refreshGraphicsPipelines(renderTarget) )
				{
					TraceDebug{ClassId} << "Unable to refresh the renderable '" << renderableInstance->renderable()->name() << "' for shadow map '" << renderTarget->id() << "' !";

					errorCount++;
				}
			});

			this->forEachRenderToTexture([&errorCount, &renderableInstance] (const auto & renderTarget) {
				if ( !renderableInstance->refreshGraphicsPipelines(renderTarget) )
				{
					TraceDebug{ClassId} << "Unable to refresh the renderable '" << renderableInstance->renderable()->name() << "' for texture '" << renderTarget->id() << "' !";

					errorCount++;
				}
			});

			this->forEachRenderToView([&errorCount, &renderableInstance] (const auto & renderTarget) {
				if ( !renderableInstance->refreshGraphicsPipelines(renderTarget) )
				{
					TraceDebug{ClassId} << "Unable to refresh the renderable '" << renderableInstance->renderable()->name() << "' for view '" << renderTarget->id() << "' !";

					errorCount++;
				}
			});

			return true;
		});

		return errorCount == 0;
	}

	void
	Scene::forEachRenderableInstance (const std::function< bool (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance) > & function) const noexcept
	{
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

			function(renderableInstance);
		}

		/* Check renderable objects from scene static entities. */
		{
			const std::lock_guard< std::mutex > lock{m_staticEntitiesAccess};

			for ( const auto & staticEntity : std::ranges::views::values(m_staticEntities) )
			{
				/* Check whether the static entity contains something to render. */
				if ( !staticEntity->isRenderable() )
				{
					continue;
				}

				/* Go through each entity component to update visuals. */
				staticEntity->forEachComponent([&function] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					function(renderableInstance);
				});
			}
		}

		/* Check renderable objects from the scene node tree. */
		{
			/* NOTE: Prevent scene node deletion from the logic update thread to crash the rendering. */
			const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

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
				node->forEachComponent([&function] (const Component::Abstract & component) {
					const auto renderableInstance = component.getRenderableInstance();

					if ( renderableInstance == nullptr )
					{
						return;
					}

					function(renderableInstance);
				});
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

		return renderableInstance->getReadyForShadowCasting(renderTarget, m_AVConsoleManager.graphicsRenderer());
	}

	bool
	Scene::getRenderableInstanceReadyForRendering (const std::shared_ptr< RenderableInstance::Abstract > & renderableInstance, const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
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

		/* NOTE: Check how many render passes this renderable instance needs. */
		const auto renderPassTypes = this->prepareRenderPassTypes(*renderableInstance);

		if ( renderPassTypes.empty() )
		{
			renderableInstance->setBroken();

			return false;
		}

		return renderableInstance->getReadyForRender(*this, renderTarget, renderPassTypes, m_AVConsoleManager.graphicsRenderer());
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

					if ( currentNode->hasComponent() )
					{
						output << '\n';

						currentNode->forEachComponent([&output] (const Component::Abstract & component) {
							output << "   {" << component.getComponentType() << ":" << component.name() << "}" "\n";
						});
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
					const auto & staticEntity = staticEntityIt->second;

					output <<
						"[Static entity #" << std::distance(m_staticEntities.cbegin(), staticEntityIt) << ":" << staticEntityIt->first << "]"
						"[Location: " << staticEntity->getWorldCoordinates().position() << "] ";

					if ( staticEntity->hasComponent() )
					{
						output << '\n';

						staticEntity->forEachComponent([&output] (const Component::Abstract & component) {
							output << "   {" << component.getComponentType() << ":" << component.name() << "}" "\n";
						});
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
			const std::lock_guard< std::mutex > lock{m_renderingOctreeAccess};

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
			const std::lock_guard< std::mutex > lock{m_physicsOctreeAccess};

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
