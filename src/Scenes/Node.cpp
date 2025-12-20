/*
 * src/Scenes/Node.cpp
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

#include "Node.hpp"

/* STL inclusions. */
#include <algorithm>
#include <memory>
#include <ranges>
#include <vector>

/* Local inclusions. */
#include "Libs/Math/OrientedCuboid.hpp"
#include "Scenes/Component/AbstractModifier.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Libs::VertexFactory;
	using namespace Graphics;
	using namespace Physics;

	Node::~Node () noexcept
	{
		const auto parentNode = m_parent.lock();

		if ( parentNode != nullptr )
		{
			parentNode->forget(this);
		}
	}

	void
	Node::setPosition (const Vector< 3, float > & position, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.setPosition(m_logicStateCoordinates.getRotationMatrix3() * position);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.setPosition(position);
				}
				else
				{
					m_logicStateCoordinates.setPosition(parentNode->m_logicStateCoordinates.getRotationMatrix3() * position);
				}
			}
				break;

			case TransformSpace::World :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.setPosition(position);
				}
				else
				{
					/* Convert world position to local position using inverse of parent's world matrix. */
					const auto parentWorldMatrix = parentNode->getWorldCoordinates().getModelMatrix();
					const auto invParentMatrix = parentWorldMatrix.inverse();
					const auto localPos = invParentMatrix * Vector< 4, float >{position, 1.0F};
					m_logicStateCoordinates.setPosition(Vector< 3, float >{localPos[X], localPos[Y], localPos[Z]});
				}
			}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::setXPosition (float position, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.setPosition(m_logicStateCoordinates.rightVector() * position);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.setXPosition(position);
				}
				else
				{
					m_logicStateCoordinates.setPosition(parentNode->m_logicStateCoordinates.rightVector() * position);
				}
			}
				break;

			case TransformSpace::World :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.setXPosition(position);
				}
				else
				{
					/* Get current world position, modify X, convert back to local. */
					auto worldPos = this->getWorldCoordinates().position();
					worldPos[X] = position;
					const auto parentWorldMatrix = parentNode->getWorldCoordinates().getModelMatrix();
					const auto invParentMatrix = parentWorldMatrix.inverse();
					const auto localPos = invParentMatrix * Vector< 4, float >{worldPos, 1.0F};
					m_logicStateCoordinates.setPosition(Vector< 3, float >{localPos[X], localPos[Y], localPos[Z]});
				}
			}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::setYPosition (float position, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.setPosition(m_logicStateCoordinates.downwardVector() * position);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.setYPosition(position);
				}
				else
				{
					m_logicStateCoordinates.setPosition(parentNode->m_logicStateCoordinates.downwardVector() * position);
				}
			}
				break;

			case TransformSpace::World :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.setYPosition(position);
				}
				else
				{
					/* Get current world position, modify Y, convert back to local. */
					auto worldPos = this->getWorldCoordinates().position();
					worldPos[Y] = position;
					const auto parentWorldMatrix = parentNode->getWorldCoordinates().getModelMatrix();
					const auto invParentMatrix = parentWorldMatrix.inverse();
					const auto localPos = invParentMatrix * Vector< 4, float >{worldPos, 1.0F};
					m_logicStateCoordinates.setPosition(Vector< 3, float >{localPos[X], localPos[Y], localPos[Z]});
				}
			}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::setZPosition (float position, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.setPosition(m_logicStateCoordinates.backwardVector() * position);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.setZPosition(position);
				}
				else
				{
					m_logicStateCoordinates.setPosition(parentNode->m_logicStateCoordinates.backwardVector() * position);
				}
			}
				break;

			case TransformSpace::World :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.setZPosition(position);
				}
				else
				{
					/* Get current world position, modify Z, convert back to local. */
					auto worldPos = this->getWorldCoordinates().position();
					worldPos[Z] = position;
					const auto parentWorldMatrix = parentNode->getWorldCoordinates().getModelMatrix();
					const auto invParentMatrix = parentWorldMatrix.inverse();
					const auto localPos = invParentMatrix * Vector< 4, float >{worldPos, 1.0F};
					m_logicStateCoordinates.setPosition(Vector< 3, float >{localPos[X], localPos[Y], localPos[Z]});
				}
			}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::move (const Vector< 3, float > & distance, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.translate(distance, true);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.translate(distance, false);
				}
				else
				{
					m_logicStateCoordinates.translate(distance, parentNode->localCoordinates());
				}
			}
				break;

			case TransformSpace::World :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.translate(distance, false);
				}
				else
				{
					/* Calculate new world position and convert to local. */
					const auto newWorldPos = this->getWorldCoordinates().position() + distance;
					const auto parentWorldMatrix = parentNode->getWorldCoordinates().getModelMatrix();
					const auto invParentMatrix = parentWorldMatrix.inverse();
					const auto localPos = invParentMatrix * Vector< 4, float >{newWorldPos, 1.0F};
					m_logicStateCoordinates.setPosition(Vector< 3, float >{localPos[X], localPos[Y], localPos[Z]});
				}
			}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::moveX (float distance, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.translateX(distance, true);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.translateX(distance, false);
				}
				else
				{
					m_logicStateCoordinates.translateX(distance, parentNode->localCoordinates());
				}
			}
				break;

			case TransformSpace::World :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.translateX(distance, false);
				}
				else
				{
					/* Move along world X axis, convert result to local. */
					auto newWorldPos = this->getWorldCoordinates().position();
					newWorldPos[X] += distance;
					const auto parentWorldMatrix = parentNode->getWorldCoordinates().getModelMatrix();
					const auto invParentMatrix = parentWorldMatrix.inverse();
					const auto localPos = invParentMatrix * Vector< 4, float >{newWorldPos, 1.0F};
					m_logicStateCoordinates.setPosition(Vector< 3, float >{localPos[X], localPos[Y], localPos[Z]});
				}
			}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::moveY (float distance, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.translateY(distance, true);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.translateY(distance, false);
				}
				else
				{
					m_logicStateCoordinates.translateY(distance, parentNode->localCoordinates());
				}
			}
				break;

			case TransformSpace::World :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.translateY(distance, false);
				}
				else
				{
					/* Move along world Y axis, convert result to local. */
					auto newWorldPos = this->getWorldCoordinates().position();
					newWorldPos[Y] += distance;
					const auto parentWorldMatrix = parentNode->getWorldCoordinates().getModelMatrix();
					const auto invParentMatrix = parentWorldMatrix.inverse();
					const auto localPos = invParentMatrix * Vector< 4, float >{newWorldPos, 1.0F};
					m_logicStateCoordinates.setPosition(Vector< 3, float >{localPos[X], localPos[Y], localPos[Z]});
				}
			}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::moveZ (float distance, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.translateZ(distance, true);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.translateZ(distance, false);
				}
				else
				{
					m_logicStateCoordinates.translateZ(distance, parentNode->localCoordinates());
				}
			}
				break;

			case TransformSpace::World :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_logicStateCoordinates.translateZ(distance, false);
				}
				else
				{
					/* Move along world Z axis, convert result to local. */
					auto newWorldPos = this->getWorldCoordinates().position();
					newWorldPos[Z] += distance;
					const auto parentWorldMatrix = parentNode->getWorldCoordinates().getModelMatrix();
					const auto invParentMatrix = parentWorldMatrix.inverse();
					const auto localPos = invParentMatrix * Vector< 4, float >{newWorldPos, 1.0F};
					m_logicStateCoordinates.setPosition(Vector< 3, float >{localPos[X], localPos[Y], localPos[Z]});
				}
			}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::rotate (float radian, const Vector< 3, float > & axis, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.rotate(radian, axis, true);
				break;

			case TransformSpace::Parent :
				m_logicStateCoordinates.rotate(radian, axis, m_parent.lock()->localCoordinates());
				break;

			case TransformSpace::World :
				m_logicStateCoordinates.rotate(radian, axis, false);
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::pitch (float radian, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.pitch(radian, true);
				break;

			case TransformSpace::Parent :
				m_logicStateCoordinates.pitch(radian, m_parent.lock()->localCoordinates());
				break;

			case TransformSpace::World :
				m_logicStateCoordinates.pitch(radian, false);
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::yaw (float radian, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.yaw(radian, true);
				break;

			case TransformSpace::Parent :
				m_logicStateCoordinates.yaw(radian, m_parent.lock()->localCoordinates());
				break;

			case TransformSpace::World :
				m_logicStateCoordinates.yaw(radian, false);
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::roll (float radian, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.roll(radian, true);
				break;

			case TransformSpace::Parent :
				m_logicStateCoordinates.roll(radian, m_parent.lock()->localCoordinates());
				break;

			case TransformSpace::World :
				m_logicStateCoordinates.roll(radian, false);
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::scale (const Vector< 3, float > & factor, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.setScalingFactor(factor);
				break;

			case TransformSpace::Parent :
			case TransformSpace::World :
				/* TODO: Reorient the scale vector to world. */
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::scale (float factor, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.setScalingFactor(factor);
				break;

			case TransformSpace::Parent :
			case TransformSpace::World :
				/* TODO: Reorient the scale vector to world. */
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::scaleX (float factor, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.setScalingXFactor(factor);
				break;

			case TransformSpace::Parent :
			case TransformSpace::World :
				/* TODO: Reorient the scale vector to world. */
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::scaleY (float factor, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.setScalingYFactor(factor);
				break;

			case TransformSpace::Parent :
			case TransformSpace::World :
				/* TODO: Reorient the scale vector to world. */
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::scaleZ (float factor, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() ) [[unlikely]]
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_logicStateCoordinates.setScalingZFactor(factor);
				break;

			case TransformSpace::Parent :
			case TransformSpace::World :
				/* TODO: Reorient the scale vector to world. */
				break;
		}

		this->onLocationDataUpdate();
	}

	CartesianFrame< float >
	Node::getWorldCoordinates () const noexcept
	{
		/* NOTE: As root, return the origin! */
		if ( this->isRoot() )
		{
			return m_logicStateCoordinates;
		}

		/* Check if parent is root without creating shared_ptr twice. */
		{
			const auto parentNode = m_parent.lock();

			if ( parentNode->isRoot() )
			{
				return m_logicStateCoordinates;
			}
		}

		/* Use vector instead of stack - better cache locality.
		 * Store pointers to avoid copying CartesianFrame objects.
		 * Most scene graphs are shallow (depth < 8). */
		std::vector< const CartesianFrame< float > * > frames;
		frames.reserve(8);

		const auto * node = this;

		while ( node != nullptr && !node->isRoot() )
		{
			frames.push_back(&node->m_logicStateCoordinates);
			node = node->m_parent.lock().get();
		}

		Matrix< 4, float > matrix;
		Vector< 3, float > scalingVector{1.0F, 1.0F, 1.0F};

		/* Traverse in reverse (root to leaf). */
		for ( auto it = frames.rbegin(); it != frames.rend(); ++it )
		{
			matrix *= (*it)->getModelMatrix();
			scalingVector *= (*it)->scalingFactor();
		}

		return CartesianFrame< float >{matrix, scalingVector};
	}

	bool
	Node::isVisibleTo (const Frustum & frustum) const noexcept
	{
		if ( !this->hasCollisionModel() )
		{
			/* No collision model: use point visibility (position only). */
			return frustum.isSeeing(this->getWorldPosition());
		}

		/* Use AABB from collision model for frustum culling. */
		const auto worldCoords = this->getWorldCoordinates();
		const auto worldAABB = this->collisionModel()->getAABB(worldCoords);

		return frustum.isSeeing(worldAABB);
	}

	void
	Node::publishStateForRendering (uint32_t writeStateIndex) noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( writeStateIndex >= m_renderStateCoordinates.size() )
			{
				Tracer::error(ClassId, "Index overflow !");

				return;
			}
		}

		m_renderStateCoordinates[writeStateIndex] = this->getWorldCoordinates();
	}

	std::shared_ptr< Node >
	Node::createChild (const std::string & name, const CartesianFrame< float > & coordinates, uint32_t sceneTimeMS) noexcept
	{
		if ( name == Root )
		{
			TraceError{ClassId} << "The node name '" << Root << "' is reserved !";

			return nullptr;
		}

		if ( m_children.contains(name) )
		{
			TraceError{ClassId} << "The node name '" << name << "' is already used at this level !";

			return nullptr;
		}

		this->notify(SubNodeCreating, this->shared_from_this());

		auto subNode = m_children.emplace(name, std::make_shared< Node >(name, this->shared_from_this(), sceneTimeMS, coordinates)).first->second;

		this->observe(subNode.get());

		this->notify(SubNodeCreated, subNode);

		return subNode;
	}

	void
	Node::onLocationDataUpdate () noexcept
	{
		if ( this->isRoot() )
		{
			Tracer::warning(ClassId, "The root node cannot changes its location !");

			return;
		}

		/* Dispatch the movement to every component. */
		if ( this->parent()->isRoot() )
		{
			this->onContainerMove(m_logicStateCoordinates);
		}
		else
		{
			this->onContainerMove(this->getWorldCoordinates());
		}

		/* Update the inverse world inertia tensor when rotation changes.
		 * This is needed for correct angular physics response. */
		if ( this->isMovable() && this->isRotationPhysicsEnabled() )
		{
			this->updateInverseWorldInertia(m_logicStateCoordinates.getRotationMatrix3());
		}

		/* Dispatch the movement to every sub node. */
		for ( const auto & subNode : m_children | std::views::values )
		{
			subNode->onLocationDataUpdate();
		}

		/* The location has been changed, so the physics simulation must be relaunched. */
		this->pauseSimulation(false);
	}

	bool
	Node::onProcessLogics (const Scene & scene) noexcept
	{
		this->updateAnimations(scene.cycle());

		m_lifetime += EngineUpdateCycleDurationUS< uint64_t >;

		/* NOTE: Check if the node has disabled its ability to move. */
		if ( !this->isMovable() || !this->isCollidable() )
		{
			return false;
		}

		/* NOTE: Apply scene modifiers to modify acceleration vectors.
		 * This can resume the physics simulation. */
		scene.forEachModifiers([this] (const auto & modifier) {
			/* NOTE: Avoid working on the same Node. */
			if ( this == &modifier.parentEntity() )
			{
				return;
			}

			const auto modifierForce = modifier.getForceAppliedTo(*this);

			this->addForce(modifierForce);
		});

		/* NOTE: If the physics engine has determined that the entity
		 * does not need physics calculation, we stop here. */
		if ( this->isSimulationPaused() )
		{
			return false;
		}

		const auto result = this->updateSimulation(scene.physicalEnvironmentProperties());

		/* Sleep/Wake: check if entity has been stable long enough to pause simulation. */
		if ( this->checkSimulationInertia() )
		{
			Tracer::debug(ClassId, "Physics simulation paused (entity at rest).");

			this->pauseSimulation(true);
		}

		return result;
	}

	void
	Node::onContentModified () noexcept
	{
		this->notify(EntityContentModified, this->shared_from_this());
	}

	std::shared_ptr< Node >
	Node::getRoot () noexcept
	{
		auto currentNode = this->shared_from_this();

		while ( !currentNode->isRoot() )
		{
			currentNode = currentNode->m_parent.lock();
		}

		return currentNode;
	}

	std::shared_ptr< const Node >
	Node::getRoot () const noexcept
	{
		auto currentNode = this->shared_from_this();

		while ( !currentNode->isRoot() )
		{
			currentNode = currentNode->m_parent.lock();
		}

		return currentNode;
	}

	std::shared_ptr< Node >
	Node::findChild (const std::string & name) const noexcept
	{
		const auto nodeIt = m_children.find(name);

		return nodeIt != m_children.cend() ? nodeIt->second : nullptr;
	}

	bool
	Node::destroyChild (const std::string & name) noexcept
	{
		const auto nodeIt = m_children.find(name);

		if ( nodeIt == m_children.end() )
		{
			return false;
		}

		m_children.erase(nodeIt);

		return true;
	}

	void
	Node::discard () noexcept
	{
		if ( this->isRoot() )
		{
			Tracer::error(ClassId, "You cannot discard the root Node !");

			return;
		}

		this->enableFlag(IsDiscardable);
	}

	void
	Node::destroyTree () noexcept
	{
		this->clearComponents();

		this->destroyChildren();
	}

	void
	Node::trimTree () noexcept
	{
		auto nodeIt = std::begin(m_children);

		while ( nodeIt != std::end(m_children) )
		{
			const auto & subNode = nodeIt->second;

			if ( subNode->isDiscardable() )
			{
				this->notify(SubNodeDeleting, subNode);

				subNode->destroyTree();

				nodeIt = m_children.erase(nodeIt);

				this->notify(SubNodeDeleted, this->shared_from_this());
			}
			else
			{
				/* NOTE: We go deeper in this node. */
				subNode->trimTree();

				++nodeIt;
			}
		}
	}

	size_t
	Node::getDepth () const noexcept
	{
		if ( this->isRoot() )
		{
			return 0;
		}

		size_t depth = 0;

		auto parent = m_parent.lock();

		while ( !parent->isRoot() )
		{
			depth++;

			parent = parent->m_parent.lock();
		}

		return depth;
	}

	void
	Node::accelerate (float power) noexcept
	{
		if ( this->isRoot() )
		{
			Tracer::warning(ClassId, "You can't set impulse to the root node !");

			return;
		}

		if ( this->parent()->isRoot() )
		{
			this->addForce(m_logicStateCoordinates.forwardVector().scale(power));
		}
		else
		{
			this->addForce(this->getWorldCoordinates().forwardVector().scale(power));
		}
	}
	
	Vector< 3, float >
	Node::getWorldVelocity () const noexcept
	{
		auto velocity = this->linearVelocity();

		auto parent = m_parent.lock();

		while ( parent != nullptr )
		{
			velocity += parent->linearVelocity();

			parent = parent->m_parent.lock();
		}

		return velocity;
	}

	Vector< 3, float >
	Node::getWorldCenterOfMass () const noexcept
	{
		if ( this->isRoot() )
		{
			/* NOTE: Returns the origin. */
			return {};
		}

		if ( this->parent()->isRoot() )
		{
			return m_logicStateCoordinates.position() + this->centerOfMass();
		}

		return this->getWorldCoordinates().position() + this->centerOfMass();
	}

	bool
	Node::onUnhandledNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept
	{
		if ( observable->is(Component::Abstract::getClassUID()) || observable->is(BodyPhysicalProperties::getClassUID()) )
		{
			/* NOTE: Avoid an automatic observer release. */
			return true;
		}

		if ( observable->is(Node::getClassUID()) )
		{
			if ( std::ranges::any_of(m_children, [observable] (const auto & subNode) {return subNode.second.get() == observable;}) )
			{
				this->notify(notificationCode, data);

				return true;
			}
		}

		/* NOTE: Don't know what it is, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
	}

	bool
	Node::playAnimation (uint8_t animationID, const Variant & value, size_t /*cycle*/) noexcept
	{
		switch ( animationID )
		{
			case LocalCoordinates :
				this->setLocalCoordinates(value.asCartesianFrameFloat());
				return true;

			case LocalPosition :
				this->setPosition(value.asVector3Float(), TransformSpace::Local);
				return true;

			case LocalXPosition :
				this->setXPosition(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalYPosition :
				this->setYPosition(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalZPosition :
				this->setZPosition(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalTranslation :
				this->move(value.asVector3Float(), TransformSpace::Local);
				return true;

			case LocalXTranslation :
				this->moveX(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalYTranslation :
				this->moveY(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalZTranslation :
				this->moveZ(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalRotation :
				// ...
				return true;

			case LocalXRotation :
				this->pitch(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalYRotation :
				this->yaw(value.asFloat(), TransformSpace::Local);
				return true;

			case LocalZRotation :
				this->roll(value.asFloat(), TransformSpace::Local);
				return true;

			case ParentPosition :
				this->setPosition(value.asVector3Float(), TransformSpace::Parent);
				return true;

			case ParentXPosition :
				this->setXPosition(value.asFloat(), TransformSpace::Parent);
				return true;

			case ParentYPosition :
				this->setYPosition(value.asFloat(), TransformSpace::Parent);
				return true;

			case ParentZPosition :
				this->setZPosition(value.asFloat(), TransformSpace::Parent);
				return true;

			case ParentTranslation :
				this->move(value.asVector3Float(), TransformSpace::Parent);
				return true;

			case ParentXTranslation :
				this->moveX(value.asFloat(), TransformSpace::Parent);
				return true;

			case ParentYTranslation :
				this->moveY(value.asFloat(), TransformSpace::Parent);
				return true;

			case ParentZTranslation :
				this->moveZ(value.asFloat(), TransformSpace::Parent);
				return true;

			case ParentRotation :
				// ...
				return true;

			case ParentXRotation :
				this->pitch(value.asFloat(), TransformSpace::Parent);
				return true;

			case ParentYRotation :
				this->yaw(value.asFloat(), TransformSpace::Parent);
				return true;

			case ParentZRotation :
				this->roll(value.asFloat(), TransformSpace::Parent);
				return true;

			case WorldPosition :
				this->setPosition(value.asVector3Float(), TransformSpace::World);
				return true;

			case WorldXPosition :
				this->setXPosition(value.asFloat(), TransformSpace::World);
				return true;

			case WorldYPosition :
				this->setYPosition(value.asFloat(), TransformSpace::World);
				return true;

			case WorldZPosition :
				this->setZPosition(value.asFloat(), TransformSpace::World);
				return true;

			case WorldTranslation :
				this->move(value.asVector3Float(), TransformSpace::World);
				return true;

			case WorldXTranslation :
				this->moveX(value.asFloat(), TransformSpace::World);
				return true;

			case WorldYTranslation :
				this->moveY(value.asFloat(), TransformSpace::World);
				return true;

			case WorldZTranslation :
				this->moveZ(value.asFloat(), TransformSpace::World);
				return true;

			case WorldRotation :
				// ...
				return true;

			case WorldXRotation :
				this->pitch(value.asFloat(), TransformSpace::World);
				return true;

			case WorldYRotation :
				this->yaw(value.asFloat(), TransformSpace::World);
				return true;

			case WorldZRotation :
				this->roll(value.asFloat(), TransformSpace::World);
				return true;

			default:
				break;
		}

		return false;
	}
}
