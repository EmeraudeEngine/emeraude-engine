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
#include <stack>

/* Local inclusions. */
#include "Libs/Math/OrientedCuboid.hpp"
#include "Scenes/Scene.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;
	using namespace EmEn::Libs::VertexFactory;
	using namespace Graphics;
	using namespace Physics;

	const size_t Node::ClassUID{getClassUID(ClassId)};

	Node::Node () noexcept
		: AbstractEntity(Root, 0)
	{
		this->setMovingAbility(false);
	}

	Node::Node (const std::string & name, const std::shared_ptr< Node > & parent, uint32_t sceneTimeMS, const CartesianFrame< float > & coordinates) noexcept
		: AbstractEntity(name, sceneTimeMS), m_parent(parent), m_cartesianFrame(coordinates)
	{

	}

	Node::~Node ()
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
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setPosition(m_cartesianFrame.getRotationMatrix3() * position);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_cartesianFrame.setPosition(position);
				}
				else
				{
					m_cartesianFrame.setPosition(parentNode->m_cartesianFrame.getRotationMatrix3() * position);
				}
			}
				break;

			case TransformSpace::World :
				if ( m_parent.lock()->isRoot() )
				{
					m_cartesianFrame.setPosition(position);
				}
				else
				{
					/* TODO: Pay attention to the node level. */
				}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::setXPosition (float position, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setPosition(m_cartesianFrame.rightVector() * position);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_cartesianFrame.setXPosition(position);
				}
				else
				{
					m_cartesianFrame.setPosition(parentNode->m_cartesianFrame.rightVector() * position);
				}
			}
				break;

			case TransformSpace::World :
				if ( m_parent.lock()->isRoot() )
				{
					m_cartesianFrame.setXPosition(position);
				}
				else
				{
					/* TODO: Pay attention to the node level. */
				}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::setYPosition (float position, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setPosition(m_cartesianFrame.downwardVector() * position);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_cartesianFrame.setYPosition(position);
				}
				else
				{
					m_cartesianFrame.setPosition(parentNode->m_cartesianFrame.downwardVector() * position);
				}
			}
				break;

			case TransformSpace::World :
				if ( m_parent.lock()->isRoot() )
				{
					m_cartesianFrame.setYPosition(position);
				}
				else
				{
					/* TODO: Pay attention to the node level. */
				}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::setZPosition (float position, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setPosition(m_cartesianFrame.backwardVector() * position);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_cartesianFrame.setZPosition(position);
				}
				else
				{
					m_cartesianFrame.setPosition(parentNode->m_cartesianFrame.backwardVector() * position);
				}
			}
				break;

			case TransformSpace::World :
				if ( m_parent.lock()->isRoot() )
				{
					m_cartesianFrame.setZPosition(position);
				}
				else
				{
					/* TODO: Pay attention to the node level. */
				}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::move (const Vector< 3, float > & distance, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.translate(distance, true);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_cartesianFrame.translate(distance, false);
				}
				else
				{
					m_cartesianFrame.translate(distance, parentNode->localCoordinates());
				}
			}
				break;

			case TransformSpace::World :
				if ( m_parent.lock()->isRoot() )
				{
					m_cartesianFrame.translate(distance, false);
				}
				else
				{
					/* TODO: Pay attention to the node level. */
				}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::moveX (float distance, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.translateX(distance, true);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_cartesianFrame.translateX(distance, false);
				}
				else
				{
					m_cartesianFrame.translateX(distance, parentNode->localCoordinates());
				}
			}
				break;

			case TransformSpace::World :
				if ( m_parent.lock()->isRoot() )
				{
					m_cartesianFrame.translateX(distance, false);
				}
				else
				{
					/* TODO: Pay attention to the node level. */
				}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::moveY (float distance, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.translateY(distance, true);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_cartesianFrame.translateY(distance, false);
				}
				else
				{
					m_cartesianFrame.translateY(distance, parentNode->localCoordinates());
				}
			}
				break;

			case TransformSpace::World :
				if ( m_parent.lock()->isRoot() )
				{
					m_cartesianFrame.translateY(distance, false);
				}
				else
				{
					/* TODO: Pay attention to the node level. */
				}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::moveZ (float distance, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.translateZ(distance, true);
				break;

			case TransformSpace::Parent :
			{
				const auto parentNode = m_parent.lock();

				if ( parentNode->isRoot() )
				{
					m_cartesianFrame.translateZ(distance, false);
				}
				else
				{
					m_cartesianFrame.translateZ(distance, parentNode->localCoordinates());
				}
			}
				break;

			case TransformSpace::World :
				if ( m_parent.lock()->isRoot() )
				{
					m_cartesianFrame.translateZ(distance, false);
				}
				else
				{
					/* TODO: Pay attention to the node level. */
				}
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::rotate (float radian, const Vector< 3, float > & axis, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.rotate(radian, axis, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.rotate(radian, axis, m_parent.lock()->localCoordinates());
				break;

			case TransformSpace::World :
				m_cartesianFrame.rotate(radian, axis, false);
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::pitch (float radian, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.pitch(radian, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.pitch(radian, m_parent.lock()->localCoordinates());
				break;

			case TransformSpace::World :
				m_cartesianFrame.pitch(radian, false);
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::yaw (float radian, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.yaw(radian, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.yaw(radian, m_parent.lock()->localCoordinates());
				break;

			case TransformSpace::World :
				m_cartesianFrame.yaw(radian, false);
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::roll (float radian, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.roll(radian, true);
				break;

			case TransformSpace::Parent :
				m_cartesianFrame.roll(radian, m_parent.lock()->localCoordinates());
				break;

			case TransformSpace::World :
				m_cartesianFrame.roll(radian, false);
				break;
		}

		this->onLocationDataUpdate();
	}

	void
	Node::scale (const Vector< 3, float > & factor, TransformSpace transformSpace) noexcept
	{
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingFactor(factor);
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
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingFactor(factor);
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
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingXFactor(factor);
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
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingYFactor(factor);
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
		if ( this->isRoot() )
		{
			return;
		}

		switch ( transformSpace )
		{
			case TransformSpace::Local :
				m_cartesianFrame.setScalingZFactor(factor);
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
		/* NOTE: As root, return the origin!
		 * If the parent is the root node, return the frame. */
		if ( this->isRoot() || this->parent()->isRoot() )
		{
			return m_cartesianFrame;
		}

		/* Stack up a reversed tree for each cartesian frame. */
		std::stack< CartesianFrame< float > > reversedTree;

		{
			const auto * node = this;

			/* While parent is not root. */
			while ( node != nullptr )
			{
				reversedTree.emplace(node->localCoordinates());

				node = node->m_parent.lock().get();
			}
		}

		Matrix< 4, float > matrix;
		Vector< 3, float > scalingVector{1.0F, 1.0F, 1.0F};

		while ( !reversedTree.empty() )
		{
			matrix *= reversedTree.top().getModelMatrix();
			scalingVector *= reversedTree.top().scalingFactor();

			reversedTree.pop();
		}

		return CartesianFrame< float >{matrix, scalingVector};
	}

	Space3D::AACuboid< float >
	Node::getWorldBoundingBox () const noexcept
	{
		if ( this->isRoot() )
		{
			/* NOTE: Returns a null box! */
			return {};
		}

		if ( this->parent()->isRoot() )
		{
			return OrientedCuboid< float >{this->localBoundingBox(), m_cartesianFrame}.getAxisAlignedBox();
		}

		return OrientedCuboid< float >{this->localBoundingBox(), this->getWorldCoordinates()}.getAxisAlignedBox();
	}

	Space3D::Sphere< float >
	Node::getWorldBoundingSphere () const noexcept
	{
		if ( this->isRoot() )
		{
			/* NOTE: Returns a null sphere! */
			return {};
		}

		if ( this->parent()->isRoot() )
		{
			return {
				this->localBoundingSphere().radius(),
				m_cartesianFrame.position() + this->localBoundingSphere().position()
			};
		}

		return {
			this->localBoundingSphere().radius(),
			this->getWorldCoordinates().position() + this->localBoundingSphere().position()
		};
	}

	std::shared_ptr< Node >
	Node::createChild (const std::string & name, uint32_t sceneTimeMS, const CartesianFrame< float > & coordinates) noexcept
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
			this->onContainerMove(m_cartesianFrame);
		}
		else
		{
			this->onContainerMove(this->getWorldCoordinates());
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
		if ( !this->isMovable() )
		{
			return false;
		}

		/* NOTE: Apply scene modifiers to modify acceleration vectors.
		 * This can resume the physics simulation. */
		scene.applyModifiers(*this);

		/* NOTE: If the physics engine has determined that the entity
		 * does not need physics calculation, we stop here. */
		if ( this->isSimulationPaused() )
		{
			return false;
		}

		return this->updateSimulation(scene.physicalEnvironmentProperties());
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
			this->addForce(m_cartesianFrame.forwardVector().scale(power));
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
			return m_cartesianFrame.position() + this->centerOfMass();
		}

		return this->getWorldCoordinates().position() + this->centerOfMass();
	}

	bool
	Node::onUnhandledNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept
	{
		if ( observable->is(Component::Abstract::ClassUID) || observable->is(PhysicalObjectProperties::ClassUID) )
		{
			/* NOTE: Avoid an automatic observer release. */
			return true;
		}

		if ( observable->is(ClassUID) )
		{
			if ( std::ranges::any_of(m_children, [observable] (const auto & subNode) {return subNode.second.get() == observable;}) )
			{
				this->notify(notificationCode, data);

				return true;
			}
		}

		/* NOTE: Don't know what is it, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable '" << whoIs(observable->classUID()) << "' (UID:" << observable->classUID() << ")  ! "
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
