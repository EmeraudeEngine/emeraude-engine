/*
 * src/Scenes/Node.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <any>
#include <map>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "AbstractEntity.hpp"
#include "Physics/MovableTrait.hpp"
#include "Animations/AnimatableInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/Variant.hpp"
#include "Graphics/Frustum.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The Node class is the key component to build the scene node tree.
	 * @extends std::enable_shared_from_this A node need to self replicate its smart pointer.
	 * @extends EmEn::Scenes::AbstractEntity A node is an entity of the 3D world.
	 * @extends EmEn::Physics::MovableTrait A node is a movable entity in the 3D world.
	 * @extends EmEn::Animations::AnimatableInterface A node can be animated by the engine logics.
	 */
	class Node final : public std::enable_shared_from_this< Node >, public AbstractEntity, public Physics::MovableTrait, public Animations::AnimatableInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Node"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				SubNodeCreating = AbstractEntity::MaxEnum,
				SubNodeCreated,
				SubNodeDeleting,
				SubNodeDeleted,
				NodeCollision,
				/* Enumeration boundary. */
				MaxEnum
			};

			/** @brief Animatable Interface key. */
			enum AnimationID : uint8_t
			{
				LocalCoordinates,
				LocalPosition,
				LocalXPosition,
				LocalYPosition,
				LocalZPosition,
				LocalTranslation,
				LocalXTranslation,
				LocalYTranslation,
				LocalZTranslation,
				LocalRotation,
				LocalXRotation,
				LocalYRotation,
				LocalZRotation,

				ParentPosition,
				ParentXPosition,
				ParentYPosition,
				ParentZPosition,
				ParentTranslation,
				ParentXTranslation,
				ParentYTranslation,
				ParentZTranslation,
				ParentRotation,
				ParentXRotation,
				ParentYRotation,
				ParentZRotation,

				WorldPosition,
				WorldXPosition,
				WorldYPosition,
				WorldZPosition,
				WorldTranslation,
				WorldXTranslation,
				WorldYTranslation,
				WorldZTranslation,
				WorldRotation,
				WorldXRotation,
				WorldYRotation,
				WorldZRotation
			};
		
			static constexpr auto Root{"root"};

			/**
			 * @brief Constructs the root node.
			 */
			Node () noexcept;

			/**
			 * @brief Constructs a child node.
			 * @param name The name of the sub child node.
			 * @param parent a reference to the smart pointer of the parent.
			 * @param sceneTimeMS The scene current time in milliseconds.
			 * @param coordinates A reference to a coordinates. Default Origin.
			 */
			Node (const std::string & name, const std::shared_ptr< Node > & parent, uint32_t sceneTimeMS, const Libs::Math::CartesianFrame< float > & coordinates = {}) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Node (const Node & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Node (Node && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Node &
			 */
			Node & operator= (const Node & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Node &
			 */
			Node & operator= (Node && copy) noexcept = delete;

			/**
			 * @brief Destructs the node.
			 */
			~Node () override;

			/** @copydoc EmEn::Scenes::LocatableInterface::setPosition(const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace) */
			void setPosition (const Libs::Math::Vector< 3, float > & position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::setXPosition(float, Libs::Math::TransformSpace) */
			void setXPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::setYPosition(float, Libs::Math::TransformSpace) */
			void setYPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::setZPosition(float, Libs::Math::TransformSpace) */
			void setZPosition (float position, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::move(const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace) */
			void move (const Libs::Math::Vector< 3, float > & distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::moveX(float, Libs::Math::TransformSpace) */
			void moveX (float distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::moveY(float, Libs::Math::TransformSpace) */
			void moveY (float distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::moveZ(float, Libs::Math::TransformSpace) */
			void moveZ (float distance, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::rotate(float, const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace) */
			void rotate (float radian, const Libs::Math::Vector< 3, float > & axis, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::pitch(float, Libs::Math::TransformSpace) */
			void pitch (float radian, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::yaw(float, Libs::Math::TransformSpace) */
			void yaw (float radian, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::roll(float, Libs::Math::TransformSpace) */
			void roll (float radian, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scale(const Libs::Math::Vector< 3, float > &, Libs::Math::TransformSpace) */
			void scale (const Libs::Math::Vector< 3, float > & factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scale(float, Libs::Math::TransformSpace) */
			void scale (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scaleX(float, Libs::Math::TransformSpace) */
			void scaleX (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scaleY(float, Libs::Math::TransformSpace) */
			void scaleY (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::scaleZ(float, Libs::Math::TransformSpace) */
			void scaleZ (float factor, Libs::Math::TransformSpace transformSpace) noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::lookAt(const Libs::Math::Vector< 3, float > &, bool) */
			void
			lookAt (const Libs::Math::Vector< 3, float > & target, bool flipZAxis) noexcept override
			{
				m_cartesianFrame.lookAt(target, flipZAxis);

				this->onLocationDataUpdate();
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::setLocalCoordinates(const Libs::Math::CartesianFrame< float > &) */
			void
			setLocalCoordinates (const Libs::Math::CartesianFrame< float > & coordinates) noexcept override
			{
				m_cartesianFrame = coordinates;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() const */
			[[nodiscard]]
			const Libs::Math::CartesianFrame< float > &
			localCoordinates () const noexcept override
			{
				return m_cartesianFrame;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::localCoordinates() */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float > &
			localCoordinates () noexcept override
			{
				return m_cartesianFrame;
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::getWorldCoordinates() const */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float > getWorldCoordinates () const noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::getWorldBoundingBox() const */
			[[nodiscard]]
			Libs::Math::Space3D::AACuboid< float > getWorldBoundingBox () const noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::getWorldBoundingSphere() const */
			[[nodiscard]]
			Libs::Math::Space3D::Sphere< float > getWorldBoundingSphere () const noexcept override;

			/** @copydoc EmEn::Scenes::LocatableInterface::enableSphereCollision(bool) */
			void
			enableSphereCollision (bool state) noexcept override
			{
				this->setFlag(SphereCollisionEnabled, state);
			}

			/** @copydoc EmEn::Scenes::LocatableInterface::sphereCollisionIsEnabled() const */
			[[nodiscard]]
			bool
			sphereCollisionIsEnabled () const noexcept override
			{
				return this->isFlagEnabled(SphereCollisionEnabled);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

			/** @copydoc EmEn::Scenes::AbstractEntity::hasMovableAbility() const */
			[[nodiscard]]
			bool
			hasMovableAbility () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Scenes::AbstractEntity::isMoving() const */
			[[nodiscard]]
			bool
			isMoving () const noexcept override
			{
				return this->hasVelocity();
			}

			/** @copydoc EmEn::Scenes::AbstractEntity::getMovableTrait() */
			[[nodiscard]]
			MovableTrait *
			getMovableTrait () noexcept override
			{
				return this;
			}

			/** @copydoc EmEn::Scenes::AbstractEntity::getMovableTrait() const */
			[[nodiscard]]
			const MovableTrait *
			getMovableTrait () const noexcept override
			{
				return this;
			}

			/** @copydoc EmEn::Physics::MovableTrait::getWorldVelocity() const */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > getWorldVelocity () const noexcept override;

			/** @copydoc EmEn::Physics::MovableTrait::getWorldCenterOfMass() const */
			[[nodiscard]]
			Libs::Math::Vector< 3, float > getWorldCenterOfMass () const noexcept override;

			/** @copydoc EmEn::Physics::MovableTrait::onHit() */
			void
			onHit (float impactForce) noexcept override
			{
				this->notify(NodeCollision, impactForce);
			}

			/** @copydoc EmEn::Physics::MovableTrait::onImpulse() */
			void
			onImpulse () noexcept override
			{
				this->pauseSimulation(false);
			}

			/**
			 * @brief Returns whether the node is the top tree one. Parent pointer is nullptr.
			 * @return bool.
			 */
			[[nodiscard]]
			bool
			isRoot () const noexcept
			{
				return m_parent.expired();
			}

			/**
			 * @brief Returns true if the node have no child.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLeaf () const noexcept
			{
				return m_children.empty();
			}

			/**
			 * @brief Returns the level below the root of this node.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t getDepth () const noexcept;

			/**
			 * @brief Returns the parent node smart pointer.
			 * @warning If the node is root, it will be nullptr.
			 * @return std::shared_ptr< Node >
			 */
			[[nodiscard]]
			std::shared_ptr< Node >
			parent () noexcept
			{
				return m_parent.lock();
			}

			/**
			 * @brief Returns the parent node smart pointer.
			 * @warning If the node is root, it will be nullptr.
			 * @return std::shared_ptr< const Node >
			 */
			[[nodiscard]]
			std::shared_ptr< const Node >
			parent () const noexcept
			{
				return m_parent.lock();
			}

			/**
			 * @brief Returns the children nodes.
			 * @return const std::map< std::string, std::shared_ptr< Node > > &
			 */
			[[nodiscard]]
			const std::map< std::string, std::shared_ptr< Node > > &
			children () const noexcept
			{
				return m_children;
			}

			/**
			 * @brief Returns the children nodes.
			 * @return std::map< std::string, std::shared_ptr< Node > > &
			 */
			[[nodiscard]]
			std::map< std::string, std::shared_ptr< Node > > &
			children () noexcept
			{
				return m_children;
			}

			/**
			 * @brief Returns the root node.
			 * @return std::shared_ptr< Node >
			 */
			[[nodiscard]]
			std::shared_ptr< Node > getRoot () noexcept;

			/**
			 * @brief Returns the root node.
			 * @return std::shared_ptr< const Node >
			 */
			[[nodiscard]]
			std::shared_ptr< const Node > getRoot () const noexcept;

			/**
			 * @brief Creates a sub node at a given coordinates.
			 * @warning If the node already exists, the method will return a null pointer.
			 * @param name A reference to a string.
			 * @param sceneTimeMS The scene current time in milliseconds.
			 * @param coordinates Set the coordinates of the new node. Default Origin.
			 * @return std::shared_ptr< Node >
			 */
			[[nodiscard]]
			std::shared_ptr< Node > createChild (const std::string & name, uint32_t sceneTimeMS, const Libs::Math::CartesianFrame< float > & coordinates = {}) noexcept;

			/**
			 * @brief Creates a sub node at a given position.
			 * @warning If the node already exists, the method will return a null pointer.
			 * @param name A reference to a string.
			 * @param sceneTimeMS The scene current time in milliseconds.
			 * @param position Set the position of the new node.
			 * @return std::shared_ptr< Node >
			 */
			[[nodiscard]]
			std::shared_ptr< Node >
			createChild (const std::string & name, uint32_t sceneTimeMS, const Libs::Math::Vector< 3, float > & position) noexcept
			{
				return this->createChild(name, sceneTimeMS, Libs::Math::CartesianFrame< float >{position});
			}

			/**
			 * @brief Returns a sub node by its name.
			 * @warning Can be nullptr !
			 * @param name A reference to a string.
			 * @return std::shared_ptr< Node >
			 */
			[[nodiscard]]
			std::shared_ptr< Node > findChild (const std::string & name) const noexcept;

			/**
			 * @brief Destroys a sub node by its name and returns true if the sub node existed.
			 * @param name A reference to a string.
			 * @return bool
			 */
			bool destroyChild (const std::string & name) noexcept;

			/**
			 * @brief Destroys every sub node and their content.
			 * @return void
			 */
			void
			destroyChildren () noexcept
			{
				m_children.clear();
			}

			/**
			 * @brief Returns for how long this node exists in microseconds.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			lifeTime () const noexcept
			{
				return m_lifetime;
			}

			/**
			 * @brief Plans a destruction for this Node. Thread-safe.
			 * @return void
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
			 * @brief Removes directly all sub node below this node.
			 * @return void
			 */
			void destroyTree () noexcept;

			/**
			 * @brief Check all sub node below this node for discarded marked ones.
			 * @return void
			 */
			void trimTree () noexcept;

			/**
			 * @brief Checks is this node is visible to frustum.
			 * @param frustum The frustum where the node is tested.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVisible (const Graphics::Frustum & frustum) const noexcept
			{
				if ( this->sphereCollisionIsEnabled() )
				{
					return frustum.isCollidingWith(this->getWorldBoundingSphere()) != Graphics::Frustum::Result::Outside;
				}

				return frustum.isCollidingWith(this->getWorldBoundingBox()) != Graphics::Frustum::Result::Outside;
			}

			/**
			 * @brief Speeds up the node forward. This is a shortcut.
			 * @param power The power of acceleration. This can be negative to decelerate.
			 * @return void
			 */
			void accelerate (float power) noexcept;

			/**
			 * @brief Returns the distance between two nodes.
			 * @param nodeA A reference to a node.
			 * @param nodeB A reference to a node.
			 * @return float
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

			/** @copydoc EmEn::Physics::MovableTrait::getObjectProperties() */
			[[nodiscard]]
			const Physics::PhysicalObjectProperties &
			getObjectProperties () const noexcept override
			{
				return this->physicalObjectProperties();
			}

			/** @copydoc EmEn::Physics::MovableTrait::getWorldPosition() */
			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			getWorldPosition () const noexcept override
			{
				return this->getWorldCoordinates().position();
			}

			/** @copydoc EmEn::Physics::MovableTrait::simulatedMove() */
			void
			simulatedMove (const Libs::Math::Vector< 3, float > & worldPosition) noexcept override
			{
				this->move(worldPosition, Libs::Math::TransformSpace::World);
			}

			/** @copydoc EmEn::Physics::MovableTrait::simulatedRotation() */
			void
			simulatedRotation (float radianAngle, const Libs::Math::Vector< 3, float > & worldDirection) noexcept override
			{
				this->rotate(Libs::Math::Degree(radianAngle), worldDirection, Libs::Math::TransformSpace::Local);
			}

			/** @copydoc EmEn::Scenes::AbstractEntity::onUnhandledNotification() */
			bool onUnhandledNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Libs::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Scenes::AbstractEntity::onLocationDataUpdate() */
			void onLocationDataUpdate () noexcept override;

			/** @copydoc EmEn::Scenes::AbstractEntity::onProcessLogics() */
			bool onProcessLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::AbstractEntity::onContentModified() */
			void onContentModified () noexcept override;

			/* Flag names. */
			static constexpr auto IsDiscardable{NextFlag + 0UL};

			/* NOTE : If nullptr, this node is the root. */
			std::weak_ptr< Node > m_parent;
			std::map< std::string, std::shared_ptr< Node > > m_children;
			Libs::Math::CartesianFrame< float > m_cartesianFrame;
			uint64_t m_lifetime{0};
	};
}
