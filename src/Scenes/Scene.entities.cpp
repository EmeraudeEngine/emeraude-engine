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
#include "AnyValue.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Component/CloudVolume.hpp"
#include "Scenes/Component/DirectionalLight.hpp"
#include "Scenes/Component/Microphone.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;

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

		/* ⚠️⚠️ `emplace` on an EXISTING key is a no-op that DISCARDS its argument, and this used to
		 * return the orphan anyway: the caller then built its components onto an entity that is not
		 * in the scene, and the geometry silently never rendered. Measured on the Khronos
		 * `TransmissionTest`, whose 22 mesh-bearing nodes carry only 16 distinct names — three of
		 * them are called `BlueTransWithMask` — so the scene received 16 entities and two of the
		 * three blue spheres were absent from the grid, with nothing logged anywhere.
		 * Neither glTF nor FBX makes node names unique; the caller must expect a collision. */
		const auto [entityIt, inserted] = m_staticEntities.emplace(name, staticEntity);

		if ( !inserted )
		{
			TraceError{ClassId} <<
				"A static entity named '" << name << "' is already in the scene '" << this->name() << "' ! "
				"The new one is DISCARDED — name it differently.";

			return nullptr;
		}

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

		/* An entity is in an octree only if checkEntityLocationInOctrees() put it there — renderable
		 * for the rendering one, collidable WITH a collision model and a valid AABB for the physics
		 * one — and those predicates may have changed since. So: erase from BOTH, unconditionally
		 * (a stale membership would keep the entity alive through the octree's shared_ptr), and
		 * WITHOUT the "not part of the octree" warning, which is only meaningful for a caller that
		 * asserts membership. ⚠️ The unconditional physics erase used to warn on every entity that
		 * never had a collision model — every explosion and fire of game-logic. */
		if ( m_renderingOctree != nullptr )
		{
			const std::scoped_lock lock{m_renderingOctreeAccess};

			m_renderingOctree->erase(staticEntity, false);
			m_renderingOctreeOverflow.erase(staticEntity);
		}

		if ( m_physicsOctree != nullptr )
		{
			const std::scoped_lock lock{m_physicsOctreeAccess};

			m_physicsOctree->erase(staticEntity, false);
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

			/* An entity outside the octree's bounds is still drawn: the render lists query the octree first
			 * (gatherRenderingCandidates()), and walk the overflow as it is. */
			if ( m_renderingOctree->updateOrInsert(entity) )
			{
				m_renderingOctreeOverflow.erase(entity);
			}
			else
			{
				m_renderingOctreeOverflow.insert(entity);
			}
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

	void
	Scene::onEntityContentModified (const std::shared_ptr< AbstractEntity > & entity) const noexcept
	{
		/* ⚠️ DEFERRED when the physics step raised it: the step holds m_physicsOctreeAccess, which the refiling below
		 * takes again (not recursive). A character whose capsule the step rebuilds (setCollisionModel() notifies since
		 * 2026-10-02) froze the logic thread on itself the first time a paladin died (2026-10-07). Only the stepping
		 * thread compares equal: a notification from any other thread waits for the lock as before. */
		if ( m_physicsStepThread.load(std::memory_order_relaxed) == std::this_thread::get_id() )
		{
			m_physicsDeferredContent.push_back(entity);

			return;
		}

		/* An entity made non-collidable (AbstractEntity::setCollidable(false), or components that no
		 * longer declare a mass) leaves the physics octree: the collision pass does not re-check the
		 * flag per pair, so it would keep colliding. Done HERE, on the rare content notification,
		 * and not in checkEntityLocationInOctrees(), which runs on every frame for every moving
		 * node: erase() walks the whole tree (an expanded root holds no element to test first).
		 * Only an entity with a collision model can have been inserted — or one that HAD one: a withdrawn
		 * model (setCollisionModel(nullptr), a corpse) leaves too, else it stayed filed with no shape and its
		 * character controller stood it up again. */
		if ( m_physicsOctree != nullptr && (entity->collisionModel() != nullptr ? !entity->isCollidable() : entity->isCollisionModelWithdrawn()) )
		{
			const std::scoped_lock lock{m_physicsOctreeAccess};

			m_physicsOctree->erase(entity, false);
		}

		this->checkEntityLocationInOctrees(entity);
	}

	bool
	Scene::checkRootNodeNotification (int notificationCode, const Base::Any & data) noexcept
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
				const auto * const nodePayload = Base::anyValue< std::shared_ptr< Node > >(data, ClassId);

				if ( nodePayload == nullptr )
				{
					return true;
				}

				const auto & node = *nodePayload;

				/* NOTE: If a node controller was set up with this node, we stop it. */
				if ( m_nodeController.node() == node )
				{
					m_nodeController.releaseNode();
				}

				/* Same contract as removeStaticEntity(): membership is decided by
				 * checkEntityLocationInOctrees() and may have changed since, so erase from both
				 * octrees, silently — absence is legitimate (an explosion or a fire never had a
				 * collision model, and was never in the physics octree). */
				if ( m_renderingOctree != nullptr )
				{
					const std::scoped_lock lock{m_renderingOctreeAccess};

					m_renderingOctree->erase(node, false);
					m_renderingOctreeOverflow.erase(node);
				}

				if ( m_physicsOctree != nullptr )
				{
					const std::scoped_lock lock{m_physicsOctreeAccess};

					m_physicsOctree->erase(node, false);
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
	Scene::checkEntityNotification (int notificationCode, const Base::Any & data) noexcept
	{
		switch ( notificationCode )
		{
			case AbstractEntity::ModifierCreated :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::AbstractModifier > >(data, ClassId) )
				{
					m_modifiers.emplace(*payload);
				}
				return true;

			case AbstractEntity::ModifierDestroyed :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::AbstractModifier > >(data, ClassId) )
				{
					m_modifiers.erase(*payload);
				}

				return true;

			case AbstractEntity::CameraCreated :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::Camera > >(data, ClassId) )
				{
					m_AVConsoleManager.addVideoDevice(*payload);
				}

				return true;

			case AbstractEntity::PrimaryCameraCreated :
			{
				const auto * const cameraPayload = Base::anyValue< std::shared_ptr< Component::Camera > >(data, ClassId);

				if ( cameraPayload == nullptr )
				{
					return true;
				}

				const auto & camera = *cameraPayload;
				m_AVConsoleManager.addVideoDevice(camera, true);
				this->setActiveCamera(camera);

				return true;
			}

			case AbstractEntity::CameraDestroyed :
			{
				const auto * const cameraPayload = Base::anyValue< std::shared_ptr< Component::Camera > >(data, ClassId);

				if ( cameraPayload == nullptr )
				{
					return true;
				}

				const auto & camera = *cameraPayload;

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
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::Microphone > >(data, ClassId) )
				{
					m_AVConsoleManager.addAudioDevice(*payload);
				}

				return true;

			case AbstractEntity::PrimaryMicrophoneCreated :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::Microphone > >(data, ClassId) )
				{
					m_AVConsoleManager.addAudioDevice(*payload, true);
				}

				return true;

			case AbstractEntity::MicrophoneDestroyed :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::Microphone > >(data, ClassId) )
				{
					m_AVConsoleManager.removeAudioDevice(*payload);
				}

				return true;

			case AbstractEntity::DirectionalLightCreated :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::DirectionalLight > >(data, ClassId) )
				{
					m_lightSet.add(*this, *payload);
				}

				return true;

			case AbstractEntity::DirectionalLightDestroyed :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::DirectionalLight > >(data, ClassId) )
				{
					m_lightSet.remove(*this, *payload);
				}

				return true;

			case AbstractEntity::PointLightCreated :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::PointLight > >(data, ClassId) )
				{
					m_lightSet.add(*this, *payload);
				}

				return true;

			case AbstractEntity::PointLightDestroyed :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::PointLight > >(data, ClassId) )
				{
					m_lightSet.remove(*this, *payload);
				}

				return true;

			case AbstractEntity::SpotLightCreated :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::SpotLight > >(data, ClassId) )
				{
					m_lightSet.add(*this, *payload);
				}

				return true;

			case AbstractEntity::SpotLightDestroyed :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::SpotLight > >(data, ClassId) )
				{
					m_lightSet.remove(*this, *payload);
				}

				return true;

			case AbstractEntity::LineLightCreated :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::LineLight > >(data, ClassId) )
				{
					m_lightSet.add(*this, *payload);
				}

				return true;

			case AbstractEntity::LineLightDestroyed :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::LineLight > >(data, ClassId) )
				{
					m_lightSet.remove(*this, *payload);
				}

				return true;

			case AbstractEntity::CloudVolumeCreated :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::CloudVolume > >(data, ClassId) )
				{
					m_cloudSet.add(*this, *payload);
				}

				return true;

			case AbstractEntity::CloudVolumeDestroyed :
				if ( const auto * const payload = Base::anyValue< std::shared_ptr< Component::CloudVolume > >(data, ClassId) )
				{
					m_cloudSet.remove(*payload);
				}

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
