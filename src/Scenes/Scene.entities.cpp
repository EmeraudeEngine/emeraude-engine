/*
 * src/Scenes/Scene.entities.cpp
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

/* Local inclusions. */
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Component/Microphone.hpp"
#include "NodeCrawler.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;

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

	void
	Scene::resetNodeTree () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

		m_rootNode->destroyTree();
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
			const std::lock_guard< std::mutex > lock{m_staticEntitiesAccess};

			for ( const auto & entity : m_staticEntities | std::views::values )
			{
				entity->suspend();
			}
		}

		/* Suspend all nodes in the tree. */
		{
			const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

			NodeCrawler< Node > crawler{m_rootNode};

			while ( crawler.hasNextNode() )
			{
				if ( auto node = crawler.nextNode() )
				{
					node->suspend();
				}
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
			const std::lock_guard< std::mutex > lock{m_staticEntitiesAccess};

			for ( const auto & entity : m_staticEntities | std::views::values )
			{
				entity->wakeup();
			}
		}

		/* Wakeup all nodes in the tree. */
		{
			const std::lock_guard< std::mutex > lock{m_sceneNodesAccess};

			NodeCrawler< Node > crawler{m_rootNode};

			while ( crawler.hasNextNode() )
			{
				if ( const auto node = crawler.nextNode() )
				{
					node->wakeup();
				}
			}
		}
	}

	void
	Scene::checkEntityLocationInOctrees (const std::shared_ptr< AbstractEntity > & entity) const noexcept
	{
		/* Check the entity in the rendering octree. */
		if ( m_renderingOctree != nullptr && entity->isRenderable() )
		{
			const std::lock_guard< std::mutex > lockGuard{m_renderingOctreeAccess};

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

			const std::lock_guard< std::mutex > lock{m_physicsOctreeAccess};

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
}
