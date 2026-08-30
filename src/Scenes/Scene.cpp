/*
 * src/Scenes/Scene.cpp
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

#include "Scene.hpp"

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "Audio/HardwareOutput.hpp"
#include "Graphics/Compute/IBLBaker.hpp"
#include "Graphics/Renderer.hpp"
#include "Input/Manager.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Component/Microphone.hpp"
#include "Scenes/NodeCrawler.hpp"
#include "SettingKeys.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Physics;
	using namespace Graphics;

	Scene::Scene (Renderer & graphicsRenderer, Audio::Manager & audioManager, const std::string & name, float boundary, const std::shared_ptr< Renderable::AbstractBackground > & background, const std::shared_ptr< GroundLevelInterface > & ground, const std::shared_ptr< SeaLevelInterface > & seaLevel, const SceneOctreeOptions & octreeOptions) noexcept
		: NameableTrait{name},
		m_graphicsRenderer{graphicsRenderer},
		m_rootNode{std::make_shared< Node >(*this)},
		m_backgroundResource{background},
		m_environmentCubemap{graphicsRenderer.getDefaultTextureCubemap()},
		m_groundLevelRenderable{std::dynamic_pointer_cast< Renderable::Abstract >(ground)},
		m_groundLevel{ground},
		m_seaLevelRenderable{std::dynamic_pointer_cast< Renderable::Abstract >(seaLevel)},
		m_seaLevel{seaLevel},
		m_AVConsoleManager{name, graphicsRenderer, audioManager},
		m_sceneMetaData{graphicsRenderer.device(), graphicsRenderer.accelerationStructureBuilder(), &graphicsRenderer.deferredDestructor()},
		m_instanceTransforms{graphicsRenderer.device(), &graphicsRenderer.deferredDestructor()},
		m_boundary{boundary},
		m_backgroundPhotometryDirty{m_backgroundResource != nullptr}
	{
		this->observe(&m_AVConsoleManager);
		this->observe(m_rootNode.get());
		this->observe(&graphicsRenderer);

		/* Mirror the GPU descriptor table capacities into the per-scene set: they are resolved from
		 * the device's update-after-bind budget at renderer initialization and are lower than the
		 * desired ones on MoltenVK (Metal caps argument buffers at 1024 samplers). Allocating a slot
		 * the table cannot hold would silently drop the texture. */
		{
			const auto & bindlessTextureManager = graphicsRenderer.bindlessTextureManager();

			m_bindlessTextureSet.setCapacities(
				bindlessTextureManager.maxTextures2D(),
				bindlessTextureManager.maxTexturesCube(),
				bindlessTextureManager.maxTexturesCubeArray()
			);
		}

		/* An asynchronously loading background pushes its photometry (luminance) once
		 * loaded — see the polling block at the top of processLogics(). */


		auto & settings = graphicsRenderer.primaryServices().settings();

		/* Initialize per-frame RT SSBOs to match the renderer's frames-in-flight count. */
		if ( m_sceneMetaData.isRayTracingEnabled() )
		{
			static_cast< void >(m_sceneMetaData.initializePerFrameBuffers(graphicsRenderer.framesInFlight()));

			m_TLASDistance = settings.getOrSetDefault< float >(GraphicsRayTracingTLASDistanceKey, DefaultGraphicsRayTracingTLASDistance);
		}

		/* Initialize per-frame instance transforms SSBOs and descriptor sets (non-instanced rendering path). */
		static_cast< void >(m_instanceTransforms.initializePerFrameBuffers(graphicsRenderer));

		m_LODScreenCoverageThreshold = settings.getOrSetDefault< float >(GraphicsLODScreenCoverageThresholdKey, DefaultGraphicsLODScreenCoverageThreshold);

		this->buildOctrees(octreeOptions);
	}

	Scene::~Scene ()
	{
		/* From 'Scene setup data' */
		{
			m_initialized = false;

			/* NOTE: Destroy and release per-scene post-process stack. */
			if ( m_postProcessStack != nullptr )
			{
				m_postProcessStack->destroyAll();
				m_postProcessStack.reset();
			}

			/* NOTE: Stop and release ambience. */
			if ( m_ambience != nullptr )
			{
				m_ambience->stop();
				m_ambience = nullptr;
			}

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
			m_physicsOctree = nullptr;
			m_renderingOctree = nullptr;
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
			m_seaLevelRenderable.reset();
			m_seaLevel.reset();
			m_groundLevelRenderable.reset();
			m_groundLevel.reset();
			m_backgroundResource.reset();

			/* NOTE: Release all shared_ptr */
			m_staticEntities.clear();

			/* NOTE: Destroy the node tree and reset the root node. */
			this->resetNodeTree();
			m_rootNode.reset();
		}
	}

	/* Post-processing. */

	void
	Scene::setPostProcessStack (std::unique_ptr< PostProcessStack > stack) noexcept
	{
		/* Destroy previous stack GPU resources before replacing. */
		if ( m_postProcessStack != nullptr )
		{
			m_postProcessStack->destroyAll();
		}

		m_postProcessStack = std::move(stack);
	}

	PostProcessStack &
	Scene::requirePostProcessStack () noexcept
	{
		if ( m_postProcessStack == nullptr )
		{
			m_postProcessStack = std::make_unique< PostProcessStack >();
		}

		return *m_postProcessStack;
	}

	std::shared_ptr< Component::Camera >
	Scene::activeCamera () const noexcept
	{
		const std::scoped_lock lock{m_activeCameraAccess};

		/* Self-healing: a camera whose entity died resolves to nullptr, and the
		 * photographic effects dematerialize through the regular per-frame polling. */
		return m_activeCamera.lock();
	}

	void
	Scene::setActiveCamera (const std::shared_ptr< Component::Camera > & camera) noexcept
	{
		const std::scoped_lock lock{m_activeCameraAccess};

		m_activeCamera = camera;
	}

	bool
	Scene::switchToCamera (const std::shared_ptr< Component::Camera > & camera) noexcept
	{
		if ( camera == nullptr )
		{
			return false;
		}

		/* Camera cut (physical camera contract): the camera becomes the RENDERED point
		 * of view (primary video source reroute through the AVConsole) AND the
		 * photographic authority (active camera) — its DoF/HDR/lens setup reshapes the
		 * post-process pipeline within a frame. */
		if ( !m_AVConsoleManager.switchPrimaryVideoSource(camera->id()) )
		{
			return false;
		}

		this->setActiveCamera(camera);

		return true;
	}

	bool
	Scene::enable (Input::Manager & inputManager, Settings & /*settings*/) noexcept
	{
		/* NOTE: First initialization. */
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

		/* Describe the scene's environment cubemap in the bindless set; the manager writes it to
		 * the reserved slot when it syncs the active scene's set. */
		if ( m_environmentCubemap != nullptr )
		{
			m_bindlessTextureSet.setEnvironmentCubemap(m_environmentCubemap);

			TraceSuccess{ClassId} << "Scene will use environment cubemap '" << m_environmentCubemap->name() << "' !";
		}

		inputManager.addKeyboardListener(&m_nodeController);
		inputManager.addPointerListener(&m_orbitController);

		this->wakeupAllEntities();

		return true;
	}

	void
	Scene::disable (Input::Manager & inputManager) noexcept
	{
		/* FIXME: Find a better way to stop the node controller! */
		m_nodeController.releaseNode();
		m_nodeController.disconnectDevice();

		inputManager.removeKeyboardListener(&m_nodeController);

		m_orbitController.releaseNode();

		inputManager.removePointerListener(&m_orbitController);

		this->suspendAllEntities();
	}

	void
	Scene::processLogics (size_t engineCycle, bool enablePhysicalSimulation) noexcept
	{
		m_lifetimeUS += WorldPhysicsUpdateCycleDurationUS< uint64_t >;
		/* ⚠️ DERIVED from the microsecond clock, never accumulated on its own.
		 * WorldPhysicsUpdateCycleDurationMS is 1000/60 in INTEGER arithmetic = 16, so accumulating
		 * it gained only 960 ms per real second: every consumer of lifetimeMS ran 4.2% SLOW, which
		 * on a flipbook is an audible-scale error (a 228 ms Doom animation frame drifts a full
		 * frame behind every ~24 cycles). The microsecond cycle is exact (16666), so millisecond
		 * time is a projection of it. Still monotonic, still cheap. */
		m_lifetimeMS = static_cast< uint32_t >(m_lifetimeUS / 1000ULL);

		/* Deferred background photometry: requests come from ANY thread (console, input
		 * callbacks, resource loading), the application mutates the scene (entities, lights,
		 * view UBOs) and therefore only ever happens here, on the logic thread, once the
		 * background resource is loaded. */
		if ( m_backgroundResource != nullptr && m_backgroundResource->isLoaded() )
		{
			if ( m_backgroundPhotometryDirty.exchange(false) )
			{
				this->refreshAmbientLightProperties();
			}

			if ( m_backgroundLightingRequested )
			{
				this->applyBackgroundLightingNow();
			}
		}

		/* Environment IBL follows the adopted environment cubemap (any thread may have
		 * changed the description; idle cost is one mutex lock + a pointer compare). */
		this->updateEnvironmentIBL();

		/* ... and the ambient pass' diffuse IBL leg follows whoever owns the indirect diffuse
		 * this frame (an enabled RTGI gathers the same sky itself). */
		this->updateIBLDiffuseOwnership();

		m_nodeController.update();

		/* Update scene static entities logics. */
		{
			const std::scoped_lock lock{m_staticEntitiesAccess};

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
			const std::scoped_lock lock{m_sceneNodesAccess};

			NodeCrawler< Node > crawler{m_rootNode};

			while ( crawler.fetchNextNode() )
			{
				const auto & currentNode = crawler.currentNode();

				if ( currentNode->processLogics(*this, engineCycle) )
				{
					this->checkEntityLocationInOctrees(currentNode);
				}
			}

			/* Clean all dead nodes. */
			m_rootNode->trimTree();
		}

		/* Update scene-level visual components (background, ground, sea).
		 * These are not entity components, so they must be updated explicitly. */
		for ( const auto & component : m_sceneVisualComponents )
		{
			if ( component != nullptr )
			{
				component->processLogics(*this);
			}
		}

		/* NOTE: Simulate physical collisions. */
		if ( enablePhysicalSimulation )
		{
			this->resolveCollisions();
		}

		if ( m_groundLevel != nullptr )
		{
			const auto worldCoordinates = m_AVConsoleManager.getPrimaryVideoDevice()->getWorldCoordinates();

			m_groundLevel->updateVisibility(worldCoordinates.position());
		}

		/* Update Cascaded Shadow Maps for directional lights.
		 * CSM needs the camera frustum corners to compute tight-fit cascade projections each frame. */
		this->updateCSMCascades(m_AVConsoleManager.graphicsRenderer().mainRenderTarget());

		/* Update audio ambience if active. */
		if ( m_ambience != nullptr && m_ambience->isPlaying() )
		{
			m_ambience->update();
		}

		m_cycle++;
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

	bool
	Scene::rebuildRenderingOctree (bool keepElements) noexcept
	{
		const std::scoped_lock lock{m_renderingOctreeAccess};

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
				if ( element->isRenderable() )
				{
					newOctree->insert(element);
				}
			}
		}

		m_renderingOctree = nullptr;
		m_renderingOctree = newOctree;

		return true;
	}

	bool
	Scene::rebuildPhysicsOctree (bool keepElements) noexcept
	{
		const std::scoped_lock lock{m_physicsOctreeAccess};

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
				if ( element->isCollidable() )
				{
					newOctree->insert(element);
				}
			}
		}

		m_physicsOctree = nullptr;
		m_physicsOctree = newOctree;

		return true;
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
			const std::scoped_lock lock{m_renderingOctreeAccess};

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
			const std::scoped_lock lock{m_physicsOctreeAccess};

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

	Audio::Ambience &
	Scene::ambience () noexcept
	{
		if ( m_ambience == nullptr )
		{
			TraceDebug{ClassId} << "Creating the ambience for the scene '" << this->name() << "' ...";

			m_ambience = std::make_unique< Audio::Ambience >(m_AVConsoleManager.audioManager());
		}

		return *m_ambience;
	}

	bool
	Scene::loadAmbience (Resources::Manager & resourceManager, const std::filesystem::path & filepath) noexcept
	{
		return this->ambience().loadSoundSet(resourceManager, filepath);
	}

	void
	Scene::startAmbience () const noexcept
	{
		if ( m_ambience == nullptr )
		{
			TraceDebug{ClassId} << "The scene '" << this->name() << "' doesn't have an Ambience to start!";

			return;
		}

		m_ambience->start();
	}

	void
	Scene::stopAmbience () const noexcept
	{
		if ( m_ambience == nullptr )
		{
			TraceDebug{ClassId} << "The scene '" << this->name() << "' doesn't have an Ambience to stop!";

			return;
		}

		m_ambience->stop();
	}

	void
	Scene::resetAmbience () const noexcept
	{
		if ( m_ambience == nullptr )
		{
			TraceDebug{ClassId} << "The scene '" << this->name() << "' doesn't have an Ambience to reset!";

			return;
		}

		m_ambience->resetSoundSet();
	}

	bool
	Scene::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept
	{
		/* Handle Renderer resize notifications to resize the post-process stack. */
		if ( observable->is(Renderer::getClassUID()) )
		{
			if ( notificationCode == Renderer::WindowContentRefreshed && m_postProcessStack != nullptr )
			{
				const auto mainRT = m_AVConsoleManager.graphicsRenderer().mainRenderTarget();

				if ( mainRT != nullptr )
				{
					const auto & extent = mainRT->extent();

					if ( !m_postProcessStack->resizeAll(extent.width, extent.height) )
					{
						TraceError{ClassId} << "Failed to resize the post-process stack on window resize !";
					}
				}
			}

			return true;
		}

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

	bool
	Scene::initializeBaseComponents () const noexcept
	{
		auto hasCamera = false;
		auto hasMicrophone = false;

		const auto inspect = [&hasCamera, &hasMicrophone] (const Component::Abstract & component) {
			if ( component.isComponent(Component::Camera::ClassId) )
			{
				hasCamera = true;
			}
			else if ( component.isComponent(Component::Microphone::ClassId) )
			{
				hasMicrophone = true;
			}
		};

		/* The node tree first ... */
		{
			NodeCrawler< const Node > crawler{m_rootNode};

			while ( !(hasCamera && hasMicrophone) && crawler.fetchNextNode() )
			{
				crawler.currentNode()->forEachComponent(inspect);
			}
		}

		/* ... then the static entities. ⚠️ A camera or a microphone lives on EITHER kind of entity
		 * (a fixed camera is a static entity carrying the primary camera — Toolkit::
		 * generatePerspectiveCamera< StaticEntity >()); until Aug 2026 only the node tree was
		 * inspected, so such a scene got a "DefaultCamera" created on the root node, declared
		 * primary AFTER the real one — and the default camera took the video output: identity
		 * pose at the origin, no HDR, a white-over-black frame. */
		if ( !(hasCamera && hasMicrophone) )
		{
			const std::scoped_lock lock{m_staticEntitiesAccess};

			for ( const auto & staticEntity : m_staticEntities | std::views::values )
			{
				staticEntity->forEachComponent(inspect);

				if ( hasCamera && hasMicrophone )
				{
					break;
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

		/* Set audio properties for this scene.
		 * NOTE: Until Aug 2026 an early `return true` inside the node crawl skipped this call
		 * whenever the scene already had both devices — i.e. in every scene with a player. It
		 * now runs unconditionally, as its comment always said it should. */
		m_AVConsoleManager.audioManager().setEnvironmentSoundProperties(m_environmentPhysicalProperties);

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
			const std::scoped_lock lock{m_renderingOctreeAccess};

			m_renderingOctree = nullptr;
		}

		if ( m_physicsOctree != nullptr )
		{
			const std::scoped_lock lock{m_physicsOctreeAccess};

			m_physicsOctree = nullptr;
		}
	}
}
