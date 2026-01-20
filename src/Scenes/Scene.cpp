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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Scene.hpp"

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "Audio/HardwareOutput.hpp"
#include "Input/Manager.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/Component/Microphone.hpp"
#include "Scenes/NodeCrawler.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Physics;
	using namespace Graphics;

	Scene::Scene (Renderer & graphicsRenderer, Audio::Manager & audioManager, const std::string & name, float boundary, const std::shared_ptr< Renderable::AbstractBackground > & background, const std::shared_ptr< GroundLevelInterface > & ground, const std::shared_ptr< SeaLevelInterface > & seaLevel, const SceneOctreeOptions & octreeOptions) noexcept
		: NameableTrait{name},
		m_rootNode{std::make_shared< Node >(*this)},
		m_backgroundResource{background},
		m_environmentCubemap{graphicsRenderer.getDefaultTextureCubemap()},
		m_groundLevelRenderable{std::dynamic_pointer_cast< Renderable::Abstract >(ground)},
		m_groundLevel{ground},
		m_seaLevelRenderable{std::dynamic_pointer_cast< Renderable::Abstract >(seaLevel)},
		m_seaLevel{seaLevel},
		m_AVConsoleManager{name, graphicsRenderer, audioManager},
		m_boundary{boundary}
	{
		this->observe(&m_AVConsoleManager);
		this->observe(m_rootNode.get());

		this->buildOctrees(octreeOptions);
	}

	Scene::~Scene ()
	{
		/* From 'Scene setup data' */
		{
			m_initialized = false;

			/* NOTE: Stop and release ambience. */
			if ( m_ambience != nullptr )
			{
				m_ambience->stop();
				m_ambience.reset();
			}

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

		/* Update the bindless textures manager with the scene's environment cubemap (If already usable). */
		if ( m_environmentCubemap != nullptr && m_environmentCubemap->isCreated() )
		{
			const auto & bindlessManager = m_AVConsoleManager.graphicsRenderer().bindlessTextureManager();

			if ( bindlessManager.usable() && bindlessManager.updateTextureCube(BindlessTextureManager::EnvironmentCubemapSlot, *m_environmentCubemap) )
			{
				TraceSuccess{ClassId} << "Scene will use environment cubemap '" << m_environmentCubemap->name() << "' !";
			}
		}

		/* FIXME: When re-enabling, the swap-chain does not have the correct ambient light parameters! */

		inputManager.addKeyboardListener(&m_nodeController);

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

		this->suspendAllEntities();
	}

	void
	Scene::processLogics (size_t engineCycle) noexcept
	{
		m_lifetimeUS += EngineUpdateCycleDurationUS< uint64_t >;
		m_lifetimeMS += EngineUpdateCycleDurationMS< uint32_t >;

		if ( m_groundLevel != nullptr )
		{
			const auto worldCoordinates = m_AVConsoleManager.getPrimaryVideoDevice()->getWorldCoordinates();

			m_groundLevel->updateVisibility(worldCoordinates.position());
		}

		this->simulatePhysics();

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

		/* Update Cascaded Shadow Maps for directional lights.
		 * CSM needs the camera frustum corners to compute tight-fit cascade projections each frame. */
		this->updateCSMCascades();

		/* Update audio ambience if active. */
		if ( m_ambience != nullptr && m_ambience->isPlaying() )
		{
			m_ambience->update();
		}

		m_cycle++;
	}

	void
	Scene::updateCSMCascades () noexcept
	{
		/* Early out if no cascaded shadow maps exist. */
		{
			const std::lock_guard< std::mutex > lock{m_renderToShadowMapCascadedAccess};

			if ( m_renderToShadowMapsCascaded.empty() )
			{
				return;
			}
		}

		/* Get camera frustum from the primary render target (View).
		 * The View contains the camera's view matrices which we need for frustum computation. */
		std::array< Vector< 3, float >, 8 > frustumCorners;
		float nearPlane = 0.1F;
		float farPlane = 1000.0F;
		bool found = false;

		this->forEachRenderToView([&frustumCorners, &nearPlane, &farPlane, &found] (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) {
			if ( found || renderTarget == nullptr )
			{
				return;
			}

			/* Get frustum corners from the View's matrices (which come from the connected camera). */
			frustumCorners = renderTarget->viewMatrices().getFrustumCornersWorld();

			/* Get far distance from the render target. Near is computed from projection. */
			nearPlane = 0.1F;
			farPlane = renderTarget->viewDistance();

			found = true;
		});

		if ( !found )
		{
			return;
		}

		/* Update all CSM-enabled directional lights with the camera frustum.
		 * NOTE: We iterate through lights because they know their direction. */
		for ( const auto & light : m_lightSet.directionalLights() )
		{
			if ( light != nullptr && light->usesCSM() && light->isShadowCastingEnabled() )
			{
				light->updateCascades(frustumCorners, nearPlane, farPlane);
			}
		}
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
				if ( element->isRenderable() )
				{
					newOctree->insert(element);
				}
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
				if ( element->isCollidable() )
				{
					newOctree->insert(element);
				}
			}
		}

		m_physicsOctree.reset();
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

		m_ambience->reset();
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
}
