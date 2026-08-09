/*
 * src/Scenes/Component/MultipleVisuals.cpp
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

#include "MultipleVisuals.hpp"

/* Local inclusions. */
#include "Scenes/Scene.hpp"

namespace EmEn::Scenes::Component
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Animations;
	using namespace Physics;
	using namespace Graphics;

	void
	MultipleVisuals::move (const CartesianFrame< float > & worldCoordinates) noexcept
	{
		this->applyWorldFrame(worldCoordinates);
	}

	CartesianFrame< float >
	MultipleVisuals::composeWithWorldFrame (const CartesianFrame< float > & localFrame) const noexcept
	{
		const auto & right = m_worldFrame.rightVector();
		const auto & downward = m_worldFrame.downwardVector();
		const auto & backward = m_worldFrame.backwardVector();

		/* The local axes are only ROTATED by the parent: a direction carries no translation, and
		 * applying the parent scale to it would leave it non-unit. */
		const auto rotate = [&right, &downward, &backward] (const Vector< 3, float > & direction) {
			return right * direction[X] + downward * direction[Y] + backward * direction[Z];
		};

		const auto & localPosition = localFrame.position();
		const auto & parentScaling = m_worldFrame.scalingFactor();

		/* A position, unlike a direction, is scaled then rotated then translated — the order
		 * getModelMatrix() itself applies. */
		const auto scaledPosition = Vector< 3, float >{
			localPosition[X] * parentScaling[X],
			localPosition[Y] * parentScaling[Y],
			localPosition[Z] * parentScaling[Z]
		};

		const auto & localScaling = localFrame.scalingFactor();

		return CartesianFrame< float >{
			m_worldFrame.position() + rotate(scaledPosition),
			rotate(localFrame.downwardVector()),
			rotate(localFrame.backwardVector()),
			{
				parentScaling[X] * localScaling[X],
				parentScaling[Y] * localScaling[Y],
				parentScaling[Z] * localScaling[Z]
			}
		};
	}

	void
	MultipleVisuals::applyParentWorldFrame () noexcept
	{
		this->applyWorldFrame(this->parentEntity().getWorldCoordinates());
	}

	void
	MultipleVisuals::applyWorldFrame (const CartesianFrame< float > & worldFrame) noexcept
	{
		m_worldFrame = worldFrame;

		m_worldCoordinates.resize(m_localCoordinates.size());

		for ( size_t index = 0; index < m_localCoordinates.size(); ++index )
		{
			m_worldCoordinates[index] = this->composeWithWorldFrame(m_localCoordinates[index]);
		}

		if ( !m_renderableInstance->updateLocalData(m_worldCoordinates, 0) )
		{
			m_renderableInstance->setBroken("Something goes wrong when updating instances model matrices !");
		}
	}

	bool
	MultipleVisuals::setLocalCoordinates (std::vector< CartesianFrame< float > > coordinates) noexcept
	{
		if ( coordinates.size() != m_localCoordinates.size() )
		{
			TraceError{ClassId} <<
				"The instance count cannot change: the buffer holds " << m_localCoordinates.size() <<
				" instances, got " << coordinates.size() << ".";

			return false;
		}

		m_localCoordinates = std::move(coordinates);

		this->applyWorldFrame(m_worldFrame);

		this->updateRenderBounds();

		this->notify(ComponentContentModified);

		return true;
	}

	void
	MultipleVisuals::processLogics (const Scene & scene) noexcept
	{
		if ( m_renderableInstance->isBroken() )
		{
			return;
		}

		if ( m_renderableInstance->isAnimated() )
		{
			m_renderableInstance->updateFrameIndex(scene.lifetimeMS() - this->parentEntity().birthTime());
		}

		this->updateAnimations(scene.cycle());

		m_renderableInstance->updateVideoMemory();
	}

	bool
	MultipleVisuals::playAnimation (uint8_t /*animationID*/, const Variant & /*value*/, size_t /*cycle*/) noexcept
	{
		return false;
	}

	void
	MultipleVisuals::updateRenderBounds () noexcept
	{
		m_renderBoundingBox.reset();

		const auto renderable = m_renderableInterface.lock();

		if ( renderable == nullptr )
		{
			return;
		}

		const auto & sourceBox = renderable->boundingBox();

		if ( !sourceBox.isValid() )
		{
			return;
		}

		const auto & minimum = sourceBox.minimum();
		const auto & maximum = sourceBox.maximum();

		/* The eight corners are transformed individually rather than the min/max pair: under a
		 * rotation, transforming only two opposite corners yields a box that does not contain
		 * the shape. */
		const std::array< Base::Math::Vector< 3, float >, 8 > corners{{
			{minimum[Base::Math::X], minimum[Base::Math::Y], minimum[Base::Math::Z]},
			{maximum[Base::Math::X], minimum[Base::Math::Y], minimum[Base::Math::Z]},
			{minimum[Base::Math::X], maximum[Base::Math::Y], minimum[Base::Math::Z]},
			{maximum[Base::Math::X], maximum[Base::Math::Y], minimum[Base::Math::Z]},
			{minimum[Base::Math::X], minimum[Base::Math::Y], maximum[Base::Math::Z]},
			{maximum[Base::Math::X], minimum[Base::Math::Y], maximum[Base::Math::Z]},
			{minimum[Base::Math::X], maximum[Base::Math::Y], maximum[Base::Math::Z]},
			{maximum[Base::Math::X], maximum[Base::Math::Y], maximum[Base::Math::Z]}
		}};

		for ( const auto & coordinates : m_localCoordinates )
		{
			const auto modelMatrix = coordinates.getModelMatrix();

			for ( const auto & corner : corners )
			{
				const auto transformed = modelMatrix * Base::Math::Vector< 4, float >{corner, 1.0F};

				m_renderBoundingBox.merge(Base::Math::Vector< 3, float >{transformed[Base::Math::X], transformed[Base::Math::Y], transformed[Base::Math::Z]});
			}
		}

		if ( !m_renderBoundingBox.isValid() )
		{
			return;
		}

		/* The sphere is derived from the final box rather than accumulated: a sphere fitted on
		 * the union is what the culling primitive needs, not the union of per-instance spheres. */
		const auto centroid = m_renderBoundingBox.centroid();

		m_renderBoundingSphere.setPosition(centroid);
		m_renderBoundingSphere.setRadius((m_renderBoundingBox.maximum() - centroid).length());
	}

	bool
	MultipleVisuals::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable == m_renderableInterface.lock().get() )
		{
			if ( notificationCode == Resources::ResourceTrait::LoadFinished )
			{
				/* ⚠️ The renderable's own box is empty until now, so the union computed at
				 * construction was empty too. Without this refresh the component keeps an
				 * invalid visual extent and gets culled as a point. */
				this->updateRenderBounds();

				this->notify(ComponentContentModified);
			}

			return true;
		}

		/* NOTE: Auto-forget */
		return false;
	}
}
