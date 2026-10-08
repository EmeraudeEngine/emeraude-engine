/*
 * src/Graphics/RenderableInstance/PathPoints.hpp
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
#include <array>
#include <cstdint>
#include <vector>

/* Local inclusions for usages. */
#include "Constants.hpp"
#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class SceneInstanceTransforms;
}

namespace EmEn::Graphics::RenderableInstance
{
	/**
	 * @brief The points of a path, for an instance that PULLS its vertices (Geometry::PulledVertexResource): published
	 * per render state slot by the logic thread, staged by the render thread into the scene's path SSBO (bindings 1-2
	 * of the instance-transforms set) right after the instance's own entry.
	 * @note Composition, not a subclass: an instance holds one through Abstract::setPathPoints() — a null pointer for
	 * every other instance, which pays one null check in stageInstanceTransforms() and nothing else.
	 * @note The PREVIOUS points (the velocity of a moving path) are a RENDER-side history, like the model matrix's: the
	 * points staged at the previous rendered frame by the primary view — not the previous logic tick, which a faster
	 * render would read twice. A frame whose point count differs from the previous one gets previous = current (no
	 * velocity) instead of pairing unrelated points.
	 */
	class EMEN_API PathPoints final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PathPoints"};

			/**
			 * @brief The look of a path drawn by the DEBUG overlay (always on top, after the tone mapping) instead of the
			 * scene pass.
			 */
			struct DebugLook
			{
				/** @brief The colour AS DISPLAYED (sRGB) and the opacity: neither exposed nor tone mapped. */
				Base::Math::Vector< 4, float > color{1.0F, 1.0F, 1.0F, 1.0F};
				/** @brief The Material::PathResource style vector: (half width, 1 if in pixels, 1 if round, miter limit). */
				Base::Math::Vector< 4, float > style{2.0F, 1.0F, 0.0F, 4.0F};
				bool enabled{false};
			};

			/**
			 * @brief Publishes the points of a logic tick into a render state slot.
			 * @note Logic thread, from the owner's publishStateForRendering(): the slot is never the one the render
			 * thread reads (the scene's triple buffer).
			 * @param writeStateIndex The slot to write.
			 * @param points The points in the entity's space, w = the arc length from the first point.
			 * @param debug The debug look: enabled, the path leaves the scene pass for the debug overlay.
			 */
			void
			publish (uint32_t writeStateIndex, const std::vector< Base::Math::Vector< 4, float > > & points, const DebugLook & debug) noexcept
			{
				m_published[writeStateIndex % RenderStateSlotCount] = points;
				m_debug[writeStateIndex % RenderStateSlotCount] = debug;
			}

			/**
			 * @brief Returns the number of points published in a slot.
			 * @param readStateIndex The slot.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			pointCount (uint32_t readStateIndex) const noexcept
			{
				return static_cast< uint32_t >(m_published[readStateIndex % RenderStateSlotCount].size());
			}

			/**
			 * @brief Returns whether the scene pass draws this path in a slot: points to draw (2 at least) and not in debug
			 * mode (the overlay draws that one).
			 * @param readStateIndex The slot.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDrawnInScene (uint32_t readStateIndex) const noexcept
			{
				const auto slot = readStateIndex % RenderStateSlotCount;

				return !m_debug[slot].enabled && m_published[slot].size() >= 2;
			}

			/**
			 * @brief Stages the frame's points (and their previous positions) for the entry slot the instance just took.
			 * @note Render thread, from Abstract::stageInstanceTransforms().
			 * @param instanceTransforms A reference to the scene's instance transforms.
			 * @param entrySlot The instance's entry slot.
			 * @param readStateIndex The frame's read slot.
			 * @param advanceHistory Whether this is the primary view staging (it moves the history forward, and stages the
			 * debug overlay's copy).
			 * @param modelMatrix The instance's model matrix of the frame (the debug copy is staged in world space).
			 */
			void stage (Scenes::SceneInstanceTransforms & instanceTransforms, uint32_t entrySlot, uint32_t readStateIndex, bool advanceHistory, const Base::Math::Matrix< 4, float > & modelMatrix) noexcept;

		private:

			std::array< std::vector< Base::Math::Vector< 4, float > >, RenderStateSlotCount > m_published{};
			std::array< DebugLook, RenderStateSlotCount > m_debug{};
			/** @brief The points staged by the primary view at the previous rendered frame (render thread only). */
			std::vector< Base::Math::Vector< 4, float > > m_history;
	};
}
