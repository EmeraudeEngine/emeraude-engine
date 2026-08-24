/*
 * src/Scenes/Component/MultipleVisuals.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <any>
#include <memory>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/RenderableInstance/Multiple.hpp"
#include "Graphics/Renderer.hpp"

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Defines a renderable instance suitable for the scene node tree.
	 * @note [OBS][SHARED-OBSERVER]
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::Base::ObserverTrait This class must dispatch modifications from a renderable instance to the entity.
	 */
	class EMEN_API MultipleVisuals final : public Abstract, public Base::ObserverTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MultipleVisuals"};

			/**
			 * @brief Constructs a multiple visuals component.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param renderable A reference to a renderable smart pointer.
			 * @param coordinates A list of instance frames, expressed in the parent entity's LOCAL
			 * space [std::move]. They are kept as given; what reaches the GPU is their composition
			 * with the entity's world frame.
			 */
			MultipleVisuals (const std::string & componentName, const AbstractEntity & parentEntity, const std::shared_ptr< Graphics::Renderable::Abstract > & renderable, std::vector< Base::Math::CartesianFrame< float > > coordinates) noexcept
				: Abstract{componentName, parentEntity},
				m_renderableInterface{renderable},
				m_renderableInstance{std::make_shared< Graphics::RenderableInstance::Multiple >(this->engineContext().graphicsRenderer.device(), renderable, coordinates, Graphics::RenderableInstance::None)},
				m_localCoordinates{std::move(coordinates)}
			{
				this->observe(renderable.get());

				/* ⚠️ The entity's frame is composed HERE, and not only in move(). move() is
				 * dispatched by onLocationDataUpdate(), which fires when an entity MOVES — never
				 * for one created at its final position and given its components afterwards, which
				 * is the normal construction order. Without this the instances stay in local space
				 * and every cell of a forest draws its content piled at the world origin. */
				this->applyParentWorldFrame();

				this->updateRenderBounds();
			}

			/**
			 * @brief Returns the instance frames as given, in the parent entity's local space.
			 *
			 * @return const std::vector< Base::Math::CartesianFrame< float > > &
			 *
			 * @note These are the authored values, never the composed ones: editing one instance
			 * of a cell is a matter of reading this, changing an entry and calling
			 * setLocalCoordinates() — no need to destroy and rebuild the whole cell.
			 */
			[[nodiscard]]
			const std::vector< Base::Math::CartesianFrame< float > > &
			localCoordinates () const noexcept
			{
				return m_localCoordinates;
			}

			/**
			 * @brief Replaces the instance frames, in the parent entity's local space.
			 * @param coordinates A list of instance frames [std::move].
			 * @return bool True when the instance data was updated.
			 *
			 * @note The count must match the one the component was built with: the instance buffer
			 * is sized once, at construction.
			 */
			bool setLocalCoordinates (std::vector< Base::Math::CartesianFrame< float > > coordinates) noexcept;

			/** @copydoc EmEn::Scenes::Component::Abstract::getRenderableInstance() const */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderableInstance::Abstract >
			getRenderableInstance () const noexcept override
			{
				return m_renderableInstance;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::getComponentType() */
			[[nodiscard]]
			const char *
			getComponentType () const noexcept override
			{
				return ClassId;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::isComponent() */
			[[nodiscard]]
			bool
			isComponent (const char * classID) const noexcept override
			{
				return strcmp(ClassId, classID) == 0;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::boundingBox() const */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			localBoundingBox () const noexcept override
			{
				return m_renderableInstance->renderable()->boundingBox();
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::boundingSphere() const */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			localBoundingSphere () const noexcept override
			{
				return m_renderableInstance->renderable()->boundingSphere();
			}

			/**
			 * @copydoc EmEn::Scenes::Component::Abstract::renderBoundingBox() const
			 * @note The union of every instance, which is what this component actually draws.
			 * The collision extent above deliberately stays that of a SINGLE instance: a forest
			 * cell must be culled as the volume it covers, and collided with as the trees it
			 * contains — not as one solid block.
			 */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			renderBoundingBox () const noexcept override
			{
				return m_renderBoundingBox;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::renderBoundingSphere() const */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			renderBoundingSphere () const noexcept override
			{
				return m_renderBoundingSphere;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void move (const Base::Math::CartesianFrame< float > & worldCoordinates) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::shouldBeRemoved() */
			[[nodiscard]]
			bool
			shouldBeRemoved () const noexcept override
			{
				return m_renderableInstance->isBroken();
			}

		private:

			/** @copydoc EmEn::Scenes::Component::Abstract::onSuspend() */
			void onSuspend () noexcept override { }

			/** @copydoc EmEn::Scenes::Component::Abstract::onWakeup() */
			void onWakeup () noexcept override { }

			/** @copydoc EmEn::Animations::AnimatableInterface::playAnimation() */
			bool playAnimation (uint8_t animationID, const Base::Variant & value, size_t cycle) noexcept override;

			/** @copydoc EmEn::Base::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/**
			 * @brief Recomputes the visual extent as the union of every instance.
			 * @note Must be called whenever the renderable finishes loading — its own bounding
			 * box is empty until then, so a union computed at construction time would be empty
			 * too, and the component would be culled as a point.
			 * @note Computed on the LOCAL frames: AbstractEntity::getWorldRenderBoundingBox()
			 * transforms it by the entity's world frame. Building it from the composed frames
			 * would have that transform applied twice.
			 */
			void updateRenderBounds () noexcept;

			/**
			 * @brief Records the entity's world frame and rebuilds the instance data from it.
			 * @param worldFrame A reference to the parent entity's world frame.
			 *
			 * @note IDEMPOTENT, by construction: the composition always starts from the untouched
			 * local frames. The former implementation added the entity position INTO the stored
			 * frames, so a second call moved everything a second time.
			 */
			void applyWorldFrame (const Base::Math::CartesianFrame< float > & worldFrame) noexcept;

			/**
			 * @brief Reads the parent entity's world frame and applies it.
			 * @note Defined out-of-line: AbstractEntity is only forward-declared here.
			 */
			void applyParentWorldFrame () noexcept;

			/**
			 * @brief Composes an instance's local frame with the entity's world frame.
			 * @param localFrame A reference to the instance frame, in the entity's local space.
			 * @return Base::Math::CartesianFrame< float > The frame in world space.
			 *
			 * @note This is the ONLY place where the two spaces meet. Should CartesianFrame ever
			 * gain a composition operator of its own — Scenes::Node open-codes the same algebra —
			 * this method becomes a one-line forward.
			 *
			 * @note A non-uniform scale on the PARENT is not representable here, and not anywhere
			 * else in CartesianFrame: rotation and scale are stored separately, so a rotated child
			 * under a non-uniformly scaled parent needs a shear the type cannot hold.
			 */
			[[nodiscard]]
			Base::Math::CartesianFrame< float > composeWithWorldFrame (const Base::Math::CartesianFrame< float > & localFrame) const noexcept;

			std::weak_ptr< Graphics::Renderable::Abstract > m_renderableInterface;
			std::shared_ptr< Graphics::RenderableInstance::Multiple > m_renderableInstance;
			std::vector< Base::Math::CartesianFrame< float > > m_localCoordinates;
			/** @brief Scratch buffer holding the composed frames, kept to avoid reallocating on every move. */
			std::vector< Base::Math::CartesianFrame< float > > m_worldCoordinates;
			Base::Math::CartesianFrame< float > m_worldFrame;
			Base::Math::Space3D::AACuboid< float > m_renderBoundingBox;
			Base::Math::Space3D::Sphere< float > m_renderBoundingSphere;
	};
}
