/*
 * src/Graphics/RenderableInstance/PathPoints.cpp
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

#include "PathPoints.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <span>

/* Local inclusions. */
#include "Scenes/SceneInstanceTransforms.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	using namespace Base::Math;

	void
	PathPoints::stage (Scenes::SceneInstanceTransforms & instanceTransforms, uint32_t entrySlot, uint32_t readStateIndex, bool advanceHistory, const Matrix< 4, float > & modelMatrix) noexcept
	{
		const auto slot = readStateIndex % RenderStateSlotCount;
		const auto & current = m_published[slot];

		/* A debug path leaves the scene pass: an EMPTY entry (its ribbon collapses), and its world-space copy for the
		 * overlay — from the primary view only, once per frame. */
		if ( m_debug[slot].enabled )
		{
			instanceTransforms.stagePath(entrySlot, {}, {});

			if ( advanceHistory )
			{
				instanceTransforms.stageDebugPath(modelMatrix, std::span< const Vector< 4, float > >{current}, m_debug[slot].color, m_debug[slot].style);
			}

			return;
		}

		/* Paired only when the previous frame had the same points: a different count means different points. */
		const std::span< const Vector< 4, float > > previous = m_history.size() == current.size() ? std::span< const Vector< 4, float > >{m_history} : std::span< const Vector< 4, float > >{};

		instanceTransforms.stagePath(entrySlot, std::span< const Vector< 4, float > >{current}, previous);

		/* Only the primary view advances the history (once per rendered frame), like the model matrix. */
		if ( advanceHistory )
		{
			/* NOTE: A count change leaves the OLD ribbon's pixels to whatever is behind it, and the TAA kept them: the
			 * depth of a ribbon a few centimetres above a surface is inside its ±1 % acceptance, so the old shape stayed
			 * as a ghost for ~30 frames (measured 2026-10-13). Its world box goes to the frame's history invalidations:
			 * the renderer marks that screen region reactive, the TAA replaces the history there for one frame (owner
			 * decision 2026-10-13, option (D)). */
			if ( !m_history.empty() && m_history.size() != current.size() )
			{
				Vector< 3, float > minimum{m_history.front()[X], m_history.front()[Y], m_history.front()[Z]};
				Vector< 3, float > maximum = minimum;

				for ( const auto & point : m_history )
				{
					for ( size_t axis = 0; axis < 3; ++axis )
					{
						minimum[axis] = std::min(minimum[axis], point[axis]);
						maximum[axis] = std::max(maximum[axis], point[axis]);
					}
				}

				instanceTransforms.invalidateTemporalHistory(m_historyModelMatrix, minimum, maximum, m_historyFootprint.halfWidth, m_historyFootprint.inPixels);
			}

			m_history = current;
			m_historyFootprint = *std::next(m_footprints.cbegin(), static_cast< std::ptrdiff_t >(slot));
			m_historyModelMatrix = modelMatrix;
		}
	}
}
