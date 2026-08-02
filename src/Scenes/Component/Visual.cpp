/*
 * src/Scenes/Component/Visual.cpp
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

#include "Visual.hpp"

/* Local inclusions. */
#include "Animations/AnimationClipResource.hpp"
#include "Animations/SkeletonResource.hpp"
#include "Constants.hpp"
#include "Graphics/Renderable/SkeletalDataTrait.hpp"
#include "Graphics/Renderer.hpp"
#include "Saphir/Generator/SkinningLayoutHelper.hpp"
#include "Scenes/Scene.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Animations;
	using namespace Physics;
	using namespace Graphics;

	void
	Visual::processLogics (const Scene & scene) noexcept
	{
		if ( m_renderableInstance->isBroken() )
		{
			return;
		}

		if ( m_renderableInstance->isAnimated() )
		{
			m_renderableInstance->updateFrameIndex(scene.lifetimeMS() - this->parentEntity().birthTime());
		}

		/* Skeletal animation: lazily initialize from renderable, then update each frame. */
		if ( m_skeletalAnimator != nullptr )
		{
			m_skeletalAnimator->update(WorldPhysicsUpdateCycleDurationS< float >);

			/* Upload skinning matrices to GPU. */
			if ( m_skeletalAnimator->hasPose() && m_renderableInstance->hasSkinningResources() )
			{
				m_renderableInstance->updateSkinningMatrices(m_skeletalAnimator->skinningMatrices());

				/* The culling volume follows the pose (joints box + flesh margin). */
				this->updateAnimatedBoundingBox();
			}
		}
		else if ( !m_renderableInterface.expired() )
		{
			if ( const auto * skeletalData = dynamic_cast< const Renderable::SkeletalDataTrait * >(m_renderableInterface.lock().get()) )
			{
				if ( skeletalData->hasSkeletalData() )
				{
					m_skeletalAnimator = std::make_unique< SkeletalAnimator >();
					m_skeletalAnimator->setSkeleton(skeletalData->skeletonResource());
					m_skeletalAnimator->setSkin(skeletalData->skin());

					for ( const auto & clip : skeletalData->animationClips() )
					{
						m_skeletalAnimator->addClip(clip);
					}

					/* Auto-play the first available clip. */
					if ( !skeletalData->animationClips().empty() )
					{
						m_skeletalAnimator->play(skeletalData->animationClips()[0]->clip().name());
					}

					/* Create GPU resources for skinning matrices. */
					const auto boneCount = static_cast< uint32_t >(skeletalData->skin().jointCount());

					if ( boneCount > 0 )
					{
						auto & renderer = skeletalData->skeletonResource()->serviceProvider().graphicsRenderer();
						auto descriptorSetLayout = Saphir::Generator::getSkinningDescriptorSetLayout(renderer.layoutManager());

						m_renderableInstance->createSkinningResources(renderer.device(), descriptorSetLayout, boneCount, renderer.framesInFlight());
					}
				}
			}
		}

		this->updateAnimations(scene.cycle());
	}

	void
	Visual::updateAnimatedBoundingBox () noexcept
	{
		const auto & jointsBox = m_skeletalAnimator->jointsBoundingBox();

		if ( !jointsBox.isValid() )
		{
			return;
		}

		/* Flesh margin, measured ONCE on the asset itself: how far the bind-pose MESH box
		 * exceeds the first joints box, per axis. The joints are the skeleton — the skinned
		 * vertices (wing membranes, muscles) extend beyond them by roughly this much in any
		 * pose. Computed from the first pose (≈ frame 0), a deliberate approximation. */
		if ( !m_fleshMarginComputed )
		{
			const auto & bindBox = m_renderableInstance->renderable()->boundingBox();

			if ( bindBox.isValid() )
			{
				for ( size_t axis = 0; axis < 3; ++axis )
				{
					const auto bindSize = bindBox.maximum(axis) - bindBox.minimum(axis);
					const auto jointsSize = jointsBox.maximum(axis) - jointsBox.minimum(axis);

					m_fleshMargin[axis] = std::max(0.0F, (bindSize - jointsSize) * 0.5F);
				}
			}

			m_fleshMarginComputed = true;
		}

		m_animatedBoundingBox.set(jointsBox.maximum() + m_fleshMargin, jointsBox.minimum() - m_fleshMargin);

		/* The entity refreshes its collision model shape (the frustum culling volume). */
		this->notify(ComponentBoundariesModified);
	}

	bool
	Visual::playAnimation (uint8_t /*animationID*/, const Variant & /*value*/, size_t /*cycle*/) noexcept
	{
		return false;
	}

	bool
	Visual::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable == m_renderableInterface.lock().get() )
		{
			if ( notificationCode == Resources::ResourceTrait::LoadFinished )
			{
				this->notify(ComponentContentModified);
			}

			return true;
		}

		/* NOTE: Auto-forget */
		return false;
	}
}
