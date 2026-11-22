/*
 * src/Scenes/Component/Visual.hpp
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
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"
#include "ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Animations/SkeletalAnimator.hpp"
#include "Graphics/Renderable/Abstract.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Graphics/RenderableInstance/Unique.hpp"
#include "Math/Space3D/AACuboid.hpp"
#include "Math/Space3D/Sphere.hpp"
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class AbstractEntity;
}

namespace EmEn::Scenes::Component
{
	/**
	 * @brief Defines a component for the visual part of an entity.
	 * @note [OBS][SHARED-OBSERVER]
	 * @extends EmEn::Scenes::Component::Abstract The base class for each entity component.
	 * @extends EmEn::Base::ObserverTrait This class must dispatch modifications from a renderable instance to the entity.
	 */
	class EMEN_LEAN_API Visual final : public Abstract, public Base::ObserverTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Visual"};

			/**
			 * @brief Constructs a visual component.
			 * @param componentName A reference to a string.
			 * @param parentEntity A reference to the parent entity.
			 * @param renderable A reference to a renderable smart pointer.
			 * @param lighting Whether the instance is LIT. ⚠️ Required on purpose: see Graphics::RenderableInstance::Lighting.
			 */
			Visual (const std::string & componentName, const AbstractEntity & parentEntity, const std::shared_ptr< Graphics::Renderable::Abstract > & renderable, Graphics::RenderableInstance::Lighting lighting) noexcept
				: Abstract{componentName, parentEntity},
				m_renderableInterface{renderable},
				m_renderableInstance{std::make_shared< Graphics::RenderableInstance::Unique >(renderable, Graphics::RenderableInstance::lightingFlags(lighting))}
			{
				/* Computed FIRST, subscribed LAST: the renderable notifies its load on a pool worker, and a
				 * notification must never reach an object whose constructor has not returned. A renderable already
				 * published never sends LoadFinished to this new observer: its bounds are final here. */
				const auto publishedBefore = renderable != nullptr && renderable->isReadyForInstantiation();

				this->refreshScaledBounds();

				this->observe(renderable.get());

				/* Check-then-act: published between the two lines above, its notification may have reached nobody. The
				 * logic thread refreshes (processLogics()); recording twice is harmless. */
				if ( !publishedBefore && renderable != nullptr && renderable->isReadyForInstantiation() )
				{
					this->recordResourceLoaded();
				}
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::getRenderableInstance() const */
			[[nodiscard]]
			std::shared_ptr< Graphics::RenderableInstance::Abstract >
			getRenderableInstance () const noexcept override
			{
				return m_renderableInstance;
			}

			/**
			 * @brief Returns the skeletal animator, or nullptr if this visual has no skeletal data.
			 * @return Animations::SkeletalAnimator *
			 */
			[[nodiscard]]
			Animations::SkeletalAnimator *
			skeletalAnimator () noexcept
			{
				return m_skeletalAnimator.get();
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

			/** @copydoc EmEn::Scenes::Component::Abstract::renderBoundingBox() const */
			[[nodiscard]]
			const Base::Math::Space3D::AACuboid< float > &
			renderBoundingBox () const noexcept override
			{
				/* Skinned mesh with a live pose: the ANIMATED box (joints + flesh margin,
				 * see updateAnimatedBoundingBox), because the bind-pose box culls whatever
				 * the animation moves outside of it. */
				if ( m_animatedBoundingBox.isValid() )
				{
					return m_animatedBoundingBox;
				}

				/* A mesh drawn at its uniform scale is bounded at it too (refreshScaledBounds()). */
				if ( m_scaledBoundingBox.isValid() )
				{
					return m_scaledBoundingBox;
				}

				const auto * renderable = this->publishedRenderable();

				return renderable != nullptr ? renderable->boundingBox() : NullBoundingBox;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::renderBoundingSphere() const */
			[[nodiscard]]
			const Base::Math::Space3D::Sphere< float > &
			renderBoundingSphere () const noexcept override
			{
				if ( m_scaledBoundingBox.isValid() )
				{
					return m_scaledBoundingSphere;
				}

				const auto * renderable = this->publishedRenderable();

				return renderable != nullptr ? renderable->boundingSphere() : NullBoundingSphere;
			}

			/** @copydoc EmEn::Scenes::Component::Abstract::move() */
			void
			move (const Base::Math::CartesianFrame< float > & /*worldCoordinates*/) noexcept override
			{

			}

			/** @copydoc EmEn::Scenes::Component::Abstract::processLogics() */
			void processLogics (const Scene & scene) noexcept override;

			/** @copydoc EmEn::Scenes::Component::Abstract::publishStateForRendering(uint32_t)
			 * @note Publishes the animation time of an animated instance: the frames draw the flipbook frame of the
			 * render state they latched. */
			void publishStateForRendering (uint32_t writeStateIndex) noexcept override;

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

			/** @copydoc EmEn::Scenes::Component::Abstract::onResourceLoaded() */
			void onResourceLoaded () noexcept override;

			/** @copydoc EmEn::Base::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const Base::Any & data) noexcept override;

			std::weak_ptr< Graphics::Renderable::Abstract > m_renderableInterface;
			/**
			 * @brief Refreshes the animated bounding box from the animator's joints box and
			 * notifies the entity (ComponentBoundariesModified) so its collision model and its
			 * render box — hence the frustum culling — follow the pose.
			 */
			void updateAnimatedBoundingBox () noexcept;

			/**
			 * @brief Scales the renderable's bounds by the MESH's uniform scale (the unit of its definition, drawn by
			 * RenderableInstance::Abstract::applyLocalTransformation()), so the collision model and the culling
			 * follow what is drawn. Leaves them invalid for a unit scale, a sprite, or a renderable not published yet
			 * (publishedRenderable()).
			 */
			void refreshScaledBounds () noexcept;

			/**
			 * @brief Returns the mesh's uniform scale, 1 for a sprite (whose scale is not drawn, see
			 * RenderableInstance::Abstract::applyLocalTransformation()).
			 * @return float
			 */
			[[nodiscard]]
			float meshScale () const noexcept;

			/**
			 * @brief Returns the renderable once it is PUBLISHED (ready for instantiation), a null pointer before.
			 * @note Its bounding volumes, its scale and its flags are written by the loading thread until then (the
			 * geometry caches its volumes at the end of its own load): reading them earlier is a data race. The
			 * publication is an atomic stored last (Renderable::Abstract::isReadyForInstantiation()).
			 * @return const Graphics::Renderable::Abstract *
			 */
			[[nodiscard]]
			const Graphics::Renderable::Abstract *
			publishedRenderable () const noexcept
			{
				const auto * renderable = m_renderableInstance->renderable();

				return renderable != nullptr && renderable->isReadyForInstantiation() ? renderable : nullptr;
			}

			std::shared_ptr< Graphics::RenderableInstance::Unique > m_renderableInstance;
			std::unique_ptr< Animations::SkeletalAnimator > m_skeletalAnimator;
			/** @brief Bounding box following the animated pose (invalid until the first pose). */
			Base::Math::Space3D::AACuboid< float > m_animatedBoundingBox;
			/** @brief The renderable's bounds at the mesh's uniform scale (invalid at a unit scale). */
			Base::Math::Space3D::AACuboid< float > m_scaledBoundingBox;
			Base::Math::Space3D::Sphere< float > m_scaledBoundingSphere;
			/** @brief Per-axis flesh margin beyond the joints, measured once on the asset. */
			Base::Math::Vector< 3, float > m_fleshMargin;
			bool m_fleshMarginComputed{false};
	};
}
