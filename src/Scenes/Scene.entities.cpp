/*
 * src/Scenes/Scene.entities.cpp
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
#include "NodeCrawler.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/Component/Microphone.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;
	using Graphics::CelestialBody;

	std::shared_ptr< Node >
	Scene::findNode (const std::string & nodeName) const noexcept
	{
		NodeCrawler< Node > crawler{m_rootNode};

		while ( crawler.fetchNextNode() )
		{
			const auto & currentNode = crawler.currentNode();

			if ( currentNode->name() == nodeName )
			{
				return currentNode;
			}
		}

		return nullptr;
	}

	void
	Scene::resetNodeTree () const noexcept
	{
		const std::scoped_lock lock{m_sceneNodesAccess};

		m_rootNode->destroyTree();
	}

	std::array< size_t, 2 >
	Scene::getNodeStatistics () const noexcept
	{
		std::array< size_t, 2 > stats{0UL, 0UL};

		NodeCrawler< const Node > crawler{m_rootNode};

		const auto accountFor = [&stats] (const std::shared_ptr< const Node > & node) {
			stats[0] += node->children().size();

			stats[1] = std::max(stats[1], node->getDepth());
		};

		/* ⚠️ The crawler sits on the BASE node before the first fetch and never yields it, so
		 * the root is accounted for HERE and the loop walks its descendants only. Folding this
		 * into the loop would silently drop the root from the statistics. */
		accountFor(crawler.currentNode());

		while ( crawler.fetchNextNode() )
		{
			accountFor(crawler.currentNode());
		}

		return stats;
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
				NodeCrawler< const Node > crawler{m_rootNode};

				const auto dumpNode = [&output] (const std::shared_ptr< const Node > & node) {
					const std::string pad(node->getDepth() * 2, ' ');

					const auto wCoords = node->getWorldCoordinates();

					output << pad <<
						"[Node:" << node->name() << "]"
						"[Location: " << wCoords.position() << ", direction: " << wCoords.forwardVector() << "] ";

					if ( node->hasComponent() )
					{
						output << '\n';

						node->forEachComponent([&output] (const Component::Abstract & component) {
							output << "   {" << component.getComponentType() << ":" << component.name() << "}" "\n";
						});
					}
					else
					{
						output << "(Empty node)" "\n";
					}
				};

				/* ⚠️ Same contract as getNodeStatistics(): the base node is never yielded by the
				 * iteration, so the root line is emitted HERE, before walking its descendants. */
				dumpNode(crawler.currentNode());

				while ( crawler.fetchNextNode() )
				{
					dumpNode(crawler.currentNode());
				}
			}
		}
		else
		{
			output << "No root node !" "\n";
		}

		return output.str();
	}

	std::shared_ptr< StaticEntity >
	Scene::createStaticEntity (const std::string & name, const CartesianFrame< float > & coordinates) noexcept
	{
		auto staticEntity = std::make_shared< StaticEntity >(*this, name, m_lifetimeMS, coordinates);

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

		/* ⚠️ Unlink the components while the scene still OBSERVES the entity: the destruction
		 * notifications (DirectionalLightDestroyed, ...) are what unregister lights from the
		 * LightSet. Forgetting first sent them into the void — the render thread then hit a
		 * destroyed light component through the still-registered pointer (pure virtual call,
		 * found via core dump on the geometry-loader background switch, Jul 2026). */
		staticEntity->clearComponents();

		this->forget(staticEntity.get());

		if ( m_renderingOctree != nullptr && staticEntity->isRenderable() )
		{
			const std::scoped_lock lock{m_renderingOctreeAccess};

			m_renderingOctree->erase(staticEntity);
		}

		if ( m_physicsOctree != nullptr )
		{
			const std::scoped_lock lock{m_physicsOctreeAccess};

			m_physicsOctree->erase(staticEntity);
		}

		staticEntity->clearComponents();

		m_staticEntities.erase(staticEntityIt);

		return true;
	}

	void
	Scene::setBackground (const std::shared_ptr< Graphics::Renderable::AbstractBackground > & background) noexcept
	{
		m_backgroundResource = background;

		/* The sky IS scene content for a probe: on-demand render targets are re-baked. */
		this->signalOnDemandRenderTargets();

		/* Extract environment cubemap from background if available. */
		if ( background != nullptr )
		{
			if ( const auto cubemap = background->environmentCubemap(); cubemap != nullptr )
			{
				m_environmentCubemap = cubemap;
			}
		}

		/* Describe the new environment cubemap in the bindless SET — the per-frame
		 * syncTextureSet mirrors it to the reserved slot, and updateEnvironmentIBL
		 * (logic thread) re-bakes the IBL when the identity changes. A cubemap still
		 * loading is adopted later by getRenderableInstanceReadyForRendering. */
		if ( m_environmentCubemap != nullptr && m_environmentCubemap->isCreated() )
		{
			m_bindlessTextureSet.setEnvironmentCubemap(m_environmentCubemap);

			TraceSuccess{ClassId} << "Scene will use environment cubemap '" << m_environmentCubemap->name() << "' !";
		}

		/* The IBL scale (environment luminance) follows the background — applied on the
		 * logic thread once the resource is loaded (this can be called from any thread). */
		m_backgroundPhotometryDirty = true;

		this->registerSceneVisualComponents();
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

					const auto wCoords = staticEntity->getWorldCoordinates();

					output <<
						"[Static entity #" << std::distance(m_staticEntities.cbegin(), staticEntityIt) << ":" << staticEntityIt->first << "]"
						"[Location: " << wCoords.position() << ", direction: " << wCoords.forwardVector() << "] ";

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

	void
	Scene::suspendAllEntities () noexcept
	{
		/* Suspend ambience (release audio sources back to pool). */
		if ( m_ambience != nullptr )
		{
			m_ambience->suspend();
		}

		/* Suspend all static entities. */
		{
			const std::scoped_lock lock{m_staticEntitiesAccess};

			for ( const auto & entity : m_staticEntities | std::views::values )
			{
				entity->suspend();
			}
		}

		/* Suspend all nodes in the tree. */
		{
			const std::scoped_lock lock{m_sceneNodesAccess};

			NodeCrawler< Node > crawler{m_rootNode};

			while ( crawler.fetchNextNode() )
			{
				crawler.currentNode()->suspend();
			}
		}
	}

	void
	Scene::wakeupAllEntities () noexcept
	{
		/* Wakeup ambience (reacquire audio sources from pool). */
		if ( m_ambience != nullptr )
		{
			m_ambience->wakeup();
		}

		/* Wakeup all static entities. */
		{
			const std::scoped_lock lock{m_staticEntitiesAccess};

			for ( const auto & entity : m_staticEntities | std::views::values )
			{
				entity->wakeup();
			}
		}

		/* Wakeup all nodes in the tree. */
		{
			const std::scoped_lock lock{m_sceneNodesAccess};

			NodeCrawler< Node > crawler{m_rootNode};

			while ( crawler.fetchNextNode() )
			{
				crawler.currentNode()->wakeup();
			}
		}
	}

	void
	Scene::checkEntityLocationInOctrees (const std::shared_ptr< AbstractEntity > & entity) const noexcept
	{
		/* Check the entity in the rendering octree. */
		if ( m_renderingOctree != nullptr && entity->isRenderable() )
		{
			const std::scoped_lock lockGuard{m_renderingOctreeAccess};

			m_renderingOctree->updateOrInsert(entity);
		}

		/* Check the entity in the physics octree. */
		if ( m_physicsOctree != nullptr && entity->isCollidable() )
		{
			/* NOTE: If there is no collision model, no physics simulation is possible. */
			const auto * collisionModel = entity->collisionModel();

			if ( collisionModel == nullptr )
			{
				return;
			}

			/* NOTE: Skip entities with uninitialized collision models (invalid AABBs).
			 * They will be added later when their collision geometry is loaded. */
			if ( !collisionModel->getAABB(entity->getWorldCoordinates()).isValid() )
			{
				return;
			}

			const std::scoped_lock lock{m_physicsOctreeAccess};

			m_physicsOctree->updateOrInsert(entity);
		}
	}

	bool
	Scene::checkRootNodeNotification (int notificationCode, const std::any & data) noexcept
	{
		switch ( notificationCode )
		{
			/* NOTE: A node is creating a child. The data will be a smart pointer to the parent node. */
			case Node::SubNodeCreating :
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
					const std::scoped_lock lock{m_renderingOctreeAccess};

					m_renderingOctree->erase(node);
				}

				if ( m_physicsOctree != nullptr )
				{
					const std::scoped_lock lock{m_physicsOctreeAccess};

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
			{
				auto camera = std::any_cast< std::shared_ptr< Component::Camera > >(data);
				m_AVConsoleManager.addVideoDevice(camera, true);
				this->setActiveCamera(camera);

				return true;
			}

			case AbstractEntity::CameraDestroyed :
			{
				auto camera = std::any_cast< std::shared_ptr< Component::Camera > >(data);

				/* NOTE: The weak reference would self-heal anyway (activeCamera() resolves a
				 * dead camera to nullptr); clearing eagerly just keeps the state tidy. */
				if ( this->activeCamera() == camera )
				{
					this->setActiveCamera(nullptr);
				}

				m_AVConsoleManager.removeVideoDevice(camera);

				return true;
			}

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
}
