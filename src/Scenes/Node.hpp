/*
 * src/Scenes/Node.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <any>
#include <map>
#include <memory>
#include <string>
#include <array>

/* Local inclusions for inheritances. */
#include "AbstractEntity.hpp"
#include "Physics/MovableTrait.hpp"
#include "Animations/AnimatableInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/Variant.hpp"

namespace EmEn::Scenes
{
	/**
	 * @class Node
	 * @brief The key element for building the hierarchical scene graph.
	 *
	 * Nodes form a tree structure starting from a single root node. Each node maintains:
	 * - A local coordinate frame (position, rotation, scale) relative to its parent
	 * - A list of child nodes
	 * - Components attached to it (visuals, physics bodies, audio sources, etc.)
	 * - Physics simulation state via MovableTrait
	 *
	 * The root node is special: it has no parent, cannot be moved, and represents the world origin.
	 * All world coordinates are computed by traversing the tree from root to node.
	 *
	 * @par Coordinate System
	 * Uses Y-down coordinate system. Positive Y points downward (gravity direction).
	 * The downwardVector() method returns the local "down" direction.
	 *
	 * @par Observable Notifications
	 * Nodes emit notifications for lifecycle events:
	 * - SubNodeCreating/SubNodeCreated: When a child node is added
	 * - SubNodeDeleting/SubNodeDeleted: When a child node is removed
	 * - NodeCollision: When physics detects a collision (via onHit)
	 *
	 * @par Thread Safety
	 * Most methods are NOT thread-safe. Only discard() is explicitly thread-safe.
	 * Scene graph modifications should happen on the main/logic thread.
	 *
	 * @par Known Limitations
	 * - TransformSpace::World is incomplete for nodes deeper than level 1 (direct children of root)
	 *   in methods: setPosition, setXPosition, setYPosition, setZPosition, move, moveX, moveY, moveZ
	 * - TransformSpace::Parent and TransformSpace::World are not implemented for scaling operations
	 *
	 * @note [OBS][SHARED-OBSERVABLE] This class is observable and uses shared_ptr for self-reference.
	 * @extends std::enable_shared_from_this A node needs to self-replicate its smart pointer.
	 * @extends EmEn::Scenes::AbstractEntity A node is an entity of the 3D world.
	 * @extends EmEn::Physics::MovableTrait A node is a movable entity in the 3D world.
	 * @extends EmEn::Animations::AnimatableInterface A node can be animated by the engine logics.
	 * @see Scene, AbstractEntity, Component::Abstract
	 * @version 0.8.35
	 */
	class Node final : public std::enable_shared_from_this< Node >, public AbstractEntity, public Physics::MovableTrait, public Animations::AnimatableInterface
	{
		public:

			/** @brief Class identifier used for runtime type identification. */
			static constexpr auto ClassId{"Node"};

			/**
			 * @enum NotificationCode
			 * @brief Observable notification codes emitted during node lifecycle events.
			 *
			 * Extends AbstractEntity notification codes. Observers can subscribe to these
			 * notifications to react to scene graph changes.
			 */
			enum NotificationCode
			{
				SubNodeCreating = AbstractEntity::MaxEnum, ///< Emitted before a child node is created. Data: parent shared_ptr.
				SubNodeCreated,							 ///< Emitted after a child node is created. Data: child shared_ptr.
				SubNodeDeleting,							///< Emitted before a child node is destroyed. Data: child shared_ptr.
				SubNodeDeleted,							 ///< Emitted after a child node is destroyed. Data: parent shared_ptr.
				NodeCollision,							  ///< Emitted when physics detects a collision. Data: impact force (float).
				/* Enumeration boundary. */
				MaxEnum									 ///< Marks the end of the enumeration range.
			};

			/**
			 * @enum AnimationID
			 * @brief Animation keys for the AnimatableInterface.
			 *
			 * Defines which node properties can be animated by the animation system.
			 * Organized in three coordinate space groups: Local, Parent, and World.
			 * Each group supports position, translation, and rotation animations.
			 */
			enum AnimationID : uint8_t
			{
				LocalCoordinates,	///< Animates the entire local coordinate frame.
				LocalPosition,	   ///< Animates position in local space (absolute).
				LocalXPosition,	  ///< Animates X position in local space.
				LocalYPosition,	  ///< Animates Y position in local space.
				LocalZPosition,	  ///< Animates Z position in local space.
				LocalTranslation,	///< Animates translation in local space (relative).
				LocalXTranslation,   ///< Animates X translation in local space.
				LocalYTranslation,   ///< Animates Y translation in local space.
				LocalZTranslation,   ///< Animates Z translation in local space.
				LocalRotation,	   ///< Animates rotation in local space (full rotation).
				LocalXRotation,	  ///< Animates pitch in local space (rotation around X axis).
				LocalYRotation,	  ///< Animates yaw in local space (rotation around Y axis).
				LocalZRotation,	  ///< Animates roll in local space (rotation around Z axis).

				ParentPosition,	  ///< Animates position in parent space (absolute).
				ParentXPosition,	 ///< Animates X position in parent space.
				ParentYPosition,	 ///< Animates Y position in parent space.
				ParentZPosition,	 ///< Animates Z position in parent space.
				ParentTranslation,   ///< Animates translation in parent space (relative).
				ParentXTranslation,  ///< Animates X translation in parent space.
				ParentYTranslation,  ///< Animates Y translation in parent space.
				ParentZTranslation,  ///< Animates Z translation in parent space.
				ParentRotation,	  ///< Animates rotation in parent space (full rotation).
				ParentXRotation,	 ///< Animates pitch in parent space.
				ParentYRotation,	 ///< Animates yaw in parent space.
				ParentZRotation,	 ///< Animates roll in parent space.

				WorldPosition,	   ///< Animates position in world space (absolute).
				WorldXPosition,	  ///< Animates X position in world space.
				WorldYPosition,	  ///< Animates Y position in world space.
				WorldZPosition,	  ///< Animates Z position in world space.
				WorldTranslation,	///< Animates translation in world space (relative).
				WorldXTranslation,   ///< Animates X translation in world space.
				WorldYTranslation,   ///< Animates Y translation in world space.
				WorldZTranslation,   ///< Animates Z translation in world space.
				WorldRotation,	   ///< Animates rotation in world space (full rotation).
				WorldXRotation,	  ///< Animates pitch in world space.
				WorldYRotation,	  ///< Animates yaw in world space.
				WorldZRotation	   ///< Animates roll in world space.
			};

			/**
			 * @brief Reserved name for the root node.
			 *
			 * Child nodes cannot use this name. Attempting to create a child with
			 * this name will fail and return nullptr.
			 */
			static constexpr auto Root{"root"};

			/**
			 * @brief Constructs the root node.
			 *
			 * Creates the special root node that serves as the world origin.
			 * The root node has no parent, cannot be moved, and has movement ability
			 * disabled by default.
			 *
			 * @param scene Reference to the scene this node belongs to.
			 * @post The node is marked as root (m_parent is nullptr).
			 * @post Movement ability is disabled.
			 * @see isRoot()
			 */
			explicit
			Node (const Scene & scene) noexcept
				: AbstractEntity{scene, Root, 0}
			{
				this->setMovingAbility(false);
			}

			/**
			 * @brief Constructs a child node.
			 *
			 * Creates a new node as a child of the specified parent. The node inherits
			 * the parent's scene and initializes with the provided local coordinates.
			 *
			 * @param name Unique name for this node within its parent's children. Name is moved.
			 * @param parent Shared pointer to the parent node.
			 * @param sceneTimeMS Scene timestamp at creation, used for lifetime tracking.
			 * @param coordinates Initial local coordinate frame relative to parent. Defaults to origin.
			 * @pre Parent must not be nullptr.
			 * @pre Name must be unique among siblings.
			 * @post The node is added to the parent's observer list.
			 * @see createChild()
			 */
			Node (std::string name, const std::shared_ptr< Node > & parent, uint32_t sceneTimeMS, const Libs::Math::CartesianFrame< float > & coordinates = {}) noexcept
				: AbstractEntity{parent->parentScene(), std::move(name), sceneTimeMS},
				m_parent{parent},
				m_logicStateCoordinates{coordinates}
			{

			}

			/**
			 * @brief Copy constructor (deleted).
			 *
			 * Nodes cannot be copied due to their unique scene graph position and
			 * observer relationships.
			 *
			 * @param copy Reference to the node to copy.
			 */
			Node (const Node & copy) noexcept = delete;

			/**
			 * @brief Move constructor (deleted).
			 *
			 * Nodes cannot be moved due to their embedded position in the scene graph
			 * and shared_ptr ownership semantics.
			 *
			 * @param copy Reference to the node to move.
			 */
			Node (Node && copy) noexcept = delete;

			/**
			 * @brief Copy assignment operator (deleted).
			 * @param copy Reference to the node to copy.
			 * @return Node reference.
			 */
			Node & operator= (const Node & copy) noexcept = delete;

			/**
			 * @brief Move assignment operator (deleted).
			 * @param copy Reference to the node to move.
			 * @return Node reference.
			 */
			Node & operator= (Node && copy) noexcept = delete;

			/**
			 * @brief Destructs the node.
			 *
			 * Automatically unregisters from the parent's observer list if this is not
			 * the root node. Child nodes and components are destroyed by their shared_ptr
			 * destructors.
			 *
			 * @post Node is removed from parent's observer list.
			 */
			~Node () noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::setPosition(const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::World is incomplete for nodes deeper than level 1. Currently only works correctly for direct children of root.
			 */
			void setPosition (const Libs::Math::Vector< 3, float > & position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::setXPosition(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::World is incomplete for nodes deeper than level 1. Currently only works correctly for direct children of root.
			 */
			void setXPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::setYPosition(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::World is incomplete for nodes deeper than level 1. Currently only works correctly for direct children of root.
			 */
			void setYPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::setZPosition(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::World is incomplete for nodes deeper than level 1. Currently only works correctly for direct children of root.
			 */
			void setZPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::move(const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::World is incomplete for nodes deeper than level 1. Currently only works correctly for direct children of root.
			 */
			void move (const Libs::Math::Vector< 3, float > & distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::moveX(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::World is incomplete for nodes deeper than level 1. Currently only works correctly for direct children of root.
			 */
			void moveX (float distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::moveY(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::World is incomplete for nodes deeper than level 1. Currently only works correctly for direct children of root.
			 */
			void moveY (float distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::moveZ(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::World is incomplete for nodes deeper than level 1. Currently only works correctly for direct children of root.
			 */
			void moveZ (float distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::rotate(float, const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 */
			void rotate (float radian, const Libs::Math::Vector< 3, float > & axis, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::pitch(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 */
			void pitch (float radian, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::yaw(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 */
			void yaw (float radian, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::roll(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 */
			void roll (float radian, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::scale(const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::Parent and TransformSpace::World are not implemented. Only TransformSpace::Local is supported.
			 */
			void scale (const Libs::Math::Vector< 3, float > & factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::scale(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::Parent and TransformSpace::World are not implemented. Only TransformSpace::Local is supported.
			 */
			void scale (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::scaleX(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::Parent and TransformSpace::World are not implemented. Only TransformSpace::Local is supported.
			 */
			void scaleX (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::scaleY(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::Parent and TransformSpace::World are not implemented. Only TransformSpace::Local is supported.
			 */
			void scaleY (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::scaleZ(float, Libs::Math::TransformSpace)
			 * @note Does nothing if called on root node.
			 * @warning TransformSpace::Parent and TransformSpace::World are not implemented. Only TransformSpace::Local is supported.
			 */
			void scaleZ (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::lookAt(const Libs::Math::Vector< 3, float > &, bool)
			 * @post Calls onLocationDataUpdate() to propagate changes to children and components.
			 */
			void
			lookAt (const Libs::Math::Vector< 3, float > & target, bool flipZAxis) noexcept override
			{
				m_logicStateCoordinates.lookAt(target, flipZAxis);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setLocalCoordinates(const Libs::Math::CartesianFrame< float > &) */
			void
			setLocalCoordinates (const Libs::Math::CartesianFrame< float > & coordinates) noexcept override
			{
				m_logicStateCoordinates = coordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() const */
			[[nodiscard]]
			const Libs::Math::CartesianFrame< float > &
			localCoordinates () const noexcept override
			{
				return m_logicStateCoordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float > &
			localCoordinates () noexcept override
			{
				return m_logicStateCoordinates;
			}

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::getWorldCoordinates() const
			 * @note Traverses the scene graph from this node to root to compute world transform.
			 * @note For root node or direct children of root, returns local coordinates directly.
			 */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float > getWorldCoordinates () const noexcept override;

			/**
			 * @copydoc EmEn::Scenes::LocatableInterface::isVisibleTo(const Graphics::Frustum &) const
			 * @note Uses the collision model AABB to determine visibility.
			 */
			[[nodiscard]]
			bool isVisibleTo (const Graphics::Frustum & frustum) const noexcept override;

			/**
			 * @brief Returns the unique identifier for this class.
			 *
			 * Computes a compile-time hash of the ClassId string. Used for runtime
			 * type identification without RTTI.
			 *
			 * @return Unique 64-bit hash identifying the Node class.
			 * @note Thread-safe. Uses static local variable initialized once.
			 * @see classUID(), is()
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Libs::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const noexcept */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const noexcept */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::hasMovableAbility() const noexcept
			 * @return Always true. Nodes always have physics capability.
			 */
			[[nodiscard]]
			bool
			hasMovableAbility () const noexcept override
			{
				return true;
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::isMoving() const noexcept
			 * @return True if the node has non-zero velocity.
			 */
			[[nodiscard]]
			bool
			isMoving () const noexcept override
			{
				return this->hasVelocity();
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::publishStateForRendering(uint32_t) noexcept
			 * @note Computes world coordinates and stores them in the render state array for double-buffering.
			 */
			void publishStateForRendering (uint32_t writeStateIndex) noexcept override;

			/** @copydoc EmEn::Scenes::AbstractEntity::getWorldCoordinatesStateForRendering(uint32_t) const noexcept */
			[[nodiscard]]
			const Libs::Math::CartesianFrame< float > &
			getWorldCoordinatesStateForRendering (uint32_t readStateIndex) const noexcept override
			{
				return m_renderStateCoordinates[readStateIndex];
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::getMovableTrait() noexcept
			 * @return Pointer to this node's MovableTrait interface.
			 */
			[[nodiscard]]
			MovableTrait *
			getMovableTrait () noexcept override
			{
				return this;
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::getMovableTrait() const noexcept
			 * @return Const pointer to this node's MovableTrait interface.
			 */
			[[nodiscard]]
			const MovableTrait *
			getMovableTrait () const noexcept override
			{
				return this;
			}

			/**
			 * @copydoc EmEn::Physics::MovableTrait::getWorldVelocity() const noexcept
			 * @note Accumulates velocities from this node and all parent nodes up to root.
			 */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > getWorldVelocity () const noexcept override;

			/**
			 * @copydoc EmEn::Physics::MovableTrait::getWorldCenterOfMass() const noexcept
			 * @return World origin if called on root node.
			 */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > getWorldCenterOfMass () const noexcept override;

			/**
			 * @copydoc EmEn::Physics::MovableTrait::getBodyPhysicalProperties() const noexcept
			 */
			[[nodiscard]]
			const Physics::BodyPhysicalProperties &
			getBodyPhysicalProperties () const noexcept override
			{
				/* NOTE: Returns the physical object properties from the abstract entity. */
				return this->bodyPhysicalProperties();
			}

			/**
			 * @copydoc EmEn::Physics::MovableTrait::onCollision() noexcept
			 * @post Emits NodeCollision notification with impact force as data.
			 */
			void
			onCollision (float impactForce) noexcept override
			{
				this->notify(NodeCollision, impactForce);
			}

			/**
			 * @copydoc EmEn::Physics::MovableTrait::onImpulse() noexcept
			 * @post Resumes physics simulation for this node.
			 */
			void
			onImpulse () noexcept override
			{
				this->pauseSimulation(false);
			}

			/**
			 * @brief Returns whether this node is the root of the scene graph.
			 *
			 * The root node has no parent (m_parent is expired weak_ptr) and represents
			 * the world origin.
			 *
			 * @return True if this is the root node, false otherwise.
			 * @see getRoot(), parent()
			 */
			[[nodiscard]]
			bool
			isRoot () const noexcept
			{
				return m_parent.expired();
			}

			/**
			 * @brief Returns whether this node has any children.
			 * @return True if the node has no children, false otherwise.
			 * @see children()
			 */
			[[nodiscard]]
			bool
			isLeaf () const noexcept
			{
				return m_children.empty();
			}

			/**
			 * @brief Computes the depth of this node in the scene graph.
			 *
			 * Traverses from this node to root counting levels.
			 * Root node has depth 0, direct children have depth 1, etc.
			 *
			 * @return Depth level. Root returns 0.
			 */
			[[nodiscard]]
			size_t getDepth () const noexcept;

			/**
			 * @brief Returns the parent node.
			 * @return Shared pointer to parent, or nullptr if this is the root node.
			 * @warning Check for nullptr before dereferencing.
			 * @see isRoot()
			 */
			[[nodiscard]]
			std::shared_ptr< Node >
			parent () noexcept
			{
				return m_parent.lock();
			}

			/**
			 * @brief Returns the parent node (const version).
			 * @return Const shared pointer to parent, or nullptr if this is the root node.
			 * @warning Check for nullptr before dereferencing.
			 * @see isRoot()
			 */
			[[nodiscard]]
			std::shared_ptr< const Node >
			parent () const noexcept
			{
				return m_parent.lock();
			}

			/**
			 * @brief Returns the map of child nodes indexed by name.
			 * @return Const reference to the children map.
			 * @note Uses heterogeneous lookup (std::less<>) for efficient string_view searches.
			 * @see createChild(), destroyChild()
			 */
			[[nodiscard]]
			const std::map< std::string, std::shared_ptr< Node >, std::less<> > &
			children () const noexcept
			{
				return m_children;
			}

			/**
			 * @brief Returns the map of child nodes indexed by name (mutable version).
			 * @return Reference to the children map.
			 * @note Uses heterogeneous lookup (std::less<>) for efficient string_view searches.
			 * @see createChild(), destroyChild()
			 */
			[[nodiscard]]
			std::map< std::string, std::shared_ptr< Node >, std::less<> > &
			children () noexcept
			{
				return m_children;
			}

			/**
			 * @brief Traverses to the root node of the scene graph.
			 *
			 * Walks up the parent chain until reaching the root. If called on root,
			 * returns itself.
			 *
			 * @return Shared pointer to the root node.
			 * @post Result is never nullptr.
			 */
			[[nodiscard]]
			std::shared_ptr< Node > getRoot () noexcept;

			/**
			 * @brief Traverses to the root node of the scene graph (const version).
			 * @return Const shared pointer to the root node.
			 * @post Result is never nullptr.
			 */
			[[nodiscard]]
			std::shared_ptr< const Node > getRoot () const noexcept;

			/**
			 * @brief Creates a child node with specified coordinates.
			 *
			 * Creates a new node as a child of this node. The new node inherits the scene
			 * and starts observing this parent. Notifies observers with SubNodeCreating
			 * before creation and SubNodeCreated after.
			 *
			 * @param name Unique name for the child (must not be "root" or already exist at this level).
			 * @param coordinates Initial local coordinates relative to this node.
			 * @param sceneTimeMS Scene timestamp for creation (affects lifetime tracking). Default 0.
			 * @return The created node, or nullptr if name is "root" or already exists.
			 * @note Emits SubNodeCreating and SubNodeCreated notifications.
			 * @warning The name "root" is reserved and will cause this method to fail.
			 * @see destroyChild(), findChild()
			 */
			[[nodiscard]]
			std::shared_ptr< Node > createChild (const std::string & name, const Libs::Math::CartesianFrame< float > & coordinates, uint32_t sceneTimeMS = 0) noexcept;

			/**
			 * @brief Creates a child node at the origin with default timestamp.
			 *
			 * Convenience overload that creates a child with identity coordinates
			 * and zero timestamp.
			 *
			 * @param name Unique name for the child.
			 * @return Shared pointer to created node, or nullptr if name is invalid or already exists.
			 * @see createChild(const std::string&, const Libs::Math::CartesianFrame<float>&, uint32_t)
			 */
			[[nodiscard]]
			std::shared_ptr< Node >
			createChild (const std::string & name) noexcept
			{
				return this->createChild(name, Libs::Math::CartesianFrame(), 0);
			}

			/**
			 * @brief Searches for a child node by name.
			 *
			 * Performs a map lookup in the children collection. Only searches direct
			 * children, not descendants.
			 *
			 * @param name Name of the child to find.
			 * @return Shared pointer to the child if found, nullptr otherwise.
			 * @note Uses heterogeneous lookup, so string_view can be passed efficiently.
			 * @see createChild(), children()
			 */
			[[nodiscard]]
			std::shared_ptr< Node > findChild (const std::string & name) const noexcept;

			/**
			 * @brief Removes and destroys a child node by name.
			 *
			 * Immediately removes the child from this node's children map. The child's
			 * destructor will be called when all references are released.
			 *
			 * @param name Name of the child to destroy.
			 * @return True if the child existed and was removed, false if not found.
			 * @post If true, the child is no longer in the children map.
			 * @note For deferred destruction, use discard() on the child instead.
			 * @see discard(), destroyChildren()
			 */
			bool destroyChild (const std::string & name) noexcept;

			/**
			 * @brief Immediately removes all child nodes.
			 *
			 * Clears the children map, destroying all direct children. Descendants are
			 * destroyed recursively by their parent's destructor.
			 *
			 * @post children() returns an empty map.
			 * @see destroyTree(), destroyChild()
			 */
			void
			destroyChildren () noexcept
			{
				m_children.clear();
			}

			/**
			 * @brief Returns how long this node has existed.
			 *
			 * Lifetime is accumulated during logic updates (onProcessLogics).
			 * Incremented by EngineUpdateCycleDurationUS each cycle.
			 *
			 * @return Lifetime in microseconds since creation.
			 * @see onProcessLogics()
			 */
			[[nodiscard]]
			uint64_t
			lifeTime () const noexcept
			{
				return m_lifetime;
			}

			/**
			 * @brief Marks this node for deferred destruction.
			 *
			 * The node will be destroyed during the next trimTree() call, typically
			 * at the end of the logic cycle. This allows safe removal during iteration.
			 *
			 * @note Thread-safe. Can be called from any thread.
			 * @warning Cannot discard the root node (will log error and do nothing).
			 * @see trimTree(), isDiscardable()
			 */
			void discard () noexcept;

			/**
			 * @brief Returns whether the Node will be destroyed in the next cycle processLogics.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDiscardable () const noexcept
			{
				return this->isFlagEnabled(IsDiscardable);
			}

			/**
			 * @brief Directly removes all sub nodes below this node.
			 * @return void
			 */
			void destroyTree () noexcept;

			/**
			 * @brief Recursively removes all nodes marked for destruction.
			 *
			 * Traverses the subtree depth-first and removes any node where isDiscardable()
			 * returns true. For each removed node, emits SubNodeDeleting before removal
			 * and SubNodeDeleted after. Also calls destroyTree() on discarded nodes.
			 *
			 * @note Called automatically by Scene at the end of each logic cycle.
			 * @see discard(), isDiscardable(), destroyTree()
			 */
			void trimTree () noexcept;

			/**
			 * @brief Applies a forward force to accelerate the node.
			 *
			 * Convenience method that adds a force along the node's forward vector
			 * (local Z axis, negative direction). Uses world coordinates for physics.
			 *
			 * @param power Force magnitude. Positive moves forward, negative moves backward.
			 * @note Does nothing on root node.
			 * @see MovableTrait::addForce()
			 */
			void accelerate (float power) noexcept;

			/**
			 * @brief Computes the Euclidean distance between two nodes in world space.
			 * @param nodeA First node.
			 * @param nodeB Second node.
			 * @return Distance in world units. Returns 0 if both references point to the same node.
			 */
			[[nodiscard]]
			static
			float
			getDistance (const Node & nodeA, const Node & nodeB) noexcept
			{
				if ( &nodeA == &nodeB )
				{
					return 0.0F;
				}

				return Libs::Math::Vector< 3, float >::distance(nodeA.getWorldCoordinates().position(), nodeB.getWorldCoordinates().position());
			}

		private:

			/** @copydoc EmEn::Physics::MovableTrait::getWorldPosition() */
			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			getWorldPosition () const noexcept override
			{
				return this->getWorldCoordinates().position();
			}

			/**
			 * @copydoc EmEn::Physics::MovableTrait::moveFromPhysics()
			 * @note Called by physics engine to update node position based on simulation.
			 * @note If simulation was paused and movement is below threshold, stays paused.
			 */
			void
			moveFromPhysics (const Libs::Math::Vector< 3, float > & positionDelta) noexcept override
			{
				const bool wasSimulationPaused = this->isSimulationPaused();

				this->move(positionDelta, Libs::Math::TransformSpace::World);

				/* If simulation was paused and movement is not significant, stay paused. */
				if ( wasSimulationPaused && positionDelta.length() < Physics::SI::centimeters(2.0F) )
				{
					this->pauseSimulation(true);
				}
			}

			/**
			 * @copydoc EmEn::Physics::MovableTrait::rotateFromPhysics()
			 * @note Called by physics engine to update node rotation based on simulation.
			 * @note Converts angle from degrees to radians and uses local space.
			 */
			void
			rotateFromPhysics (float radianAngle, const Libs::Math::Vector< 3, float > & worldDirection) noexcept override
			{
				this->rotate(Libs::Math::Degree(radianAngle), worldDirection, Libs::Math::TransformSpace::Local);
			}

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::onUnhandledNotification()
			 * @note Handles notifications from child nodes, components, and physical properties.
			 */
			bool onUnhandledNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/**
			 * @copydoc EmEn::Animations::AnimatableInterface::playAnimation()
			 * @note Supports all AnimationID values for transforming node coordinates.
			 */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::onLocationDataUpdate()
			 * @note Propagates location changes to all components and child nodes recursively.
			 * @post Resumes physics simulation (pauseSimulation(false)).
			 */
			void onLocationDataUpdate () noexcept override;

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::onProcessLogics()
			 * @note Updates animations, increments lifetime, applies scene modifiers, and runs physics simulation.
			 */
			bool onProcessLogics (const Scene & scene) noexcept override;

			/**
			 * @copydoc EmEn::Scenes::AbstractEntity::onContentModified()
			 * @note Emits EntityContentModified notification.
			 */
			void onContentModified () noexcept override;

			static constexpr auto IsDiscardable{NextFlag + 0UL};

			std::weak_ptr< Node > m_parent;
			std::map< std::string, std::shared_ptr< Node >, std::less<> > m_children;
			Libs::Math::CartesianFrame< float > m_logicStateCoordinates;
			std::array< Libs::Math::CartesianFrame< float >, 2 > m_renderStateCoordinates{};
			uint64_t m_lifetime{0};
	};
}
