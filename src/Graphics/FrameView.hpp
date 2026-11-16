/*
 * src/Graphics/FrameView.hpp
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

/* STL inclusions. */
#include <cstdint>

/* Local inclusions for usages. */
#include "Math/Matrix.hpp"
#include "Math/Vector.hpp"
#include "ViewMatricesInterface.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The view ONE frame renders with: the main render target's view matrices, read at the render state the
	 * scene latched for this frame (Scenes::Scene::beginRenderFrame()) — the matrices that rasterized this frame's
	 * depth buffer.
	 * @note The truth is the scene's latch; this is a way to read it, never a copy with a time of its own. The renderer
	 * makes one from the scene it renders, wherever a frame needs it, and the post-process chain receives it in
	 * IndirectPostProcessEffect::FrameContext::frameView: an effect holds neither a state index nor a render target,
	 * so it can pair neither with the wrong one (scenes doc 12 § View Matrix State Index — Critical Trap).
	 * @note RENDER THREAD, within the frame it was made for: the latched slot is the rendering thread's until the next
	 * Scenes::Scene::beginRenderFrame(). The view matrices are a REFERENCE on purpose: a frame's view is made, never
	 * re-pointed nor kept.
	 * @warning The accessors return references INTO the view matrices: valid for the frame, not beyond.
	 */
	class FrameView final
	{
		public:

			/**
			 * @brief Constructs the view of a frame.
			 * @param viewMatrices A reference to the main render target's view matrices.
			 * @param readStateIndex The render state the scene latched for this frame (Scenes::Scene::frameReadStateIndex()).
			 */
			FrameView (const ViewMatricesInterface & viewMatrices, uint32_t readStateIndex) noexcept
				: m_viewMatrices{viewMatrices},
				m_readStateIndex{readStateIndex}
			{

			}

			/**
			 * @brief Returns the view matrix of the frame (world to view).
			 * @return const Base::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Base::Math::Matrix< 4, float > &
			viewMatrix () const noexcept
			{
				return m_viewMatrices.viewMatrix(m_readStateIndex, false, 0);
			}

			/**
			 * @brief Returns the INFINITY view matrix of the frame: the view ROTATION alone, the camera at the origin.
			 * @note What a camera-relative computation multiplies the projection by (docs/caution-points.md § The float
			 * inverse of the full view-projection).
			 * @return const Base::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Base::Math::Matrix< 4, float > &
			infinityViewMatrix () const noexcept
			{
				return m_viewMatrices.viewMatrix(m_readStateIndex, true, 0);
			}

			/**
			 * @brief Returns the projection matrix the frame is rasterized with: JITTERED when a sub-pixel jitter is
			 * active (TAA).
			 * @return const Base::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Base::Math::Matrix< 4, float > &
			projectionMatrix () const noexcept
			{
				return m_viewMatrices.projectionMatrix(m_readStateIndex);
			}

			/**
			 * @brief Returns the projection matrix of the frame without its sub-pixel jitter: what a velocity or a
			 * screen position that must not shimmer is built from.
			 * @return const Base::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Base::Math::Matrix< 4, float > &
			unjitteredProjectionMatrix () const noexcept
			{
				return m_viewMatrices.unjitteredProjectionMatrix(m_readStateIndex);
			}

			/**
			 * @brief Returns the position of the point of view, in world space.
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			position () const noexcept
			{
				return m_viewMatrices.position(m_readStateIndex);
			}

			/**
			 * @brief Returns the ambient chromaticity of the frame (unit luminance): what its shaders read in the view
			 * block.
			 * @note How a frame reads the scene's ambient — never the light set's own values, which the logic thread
			 * writes while the frame is recorded.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			ambientChromaticity () const noexcept
			{
				return m_viewMatrices.ambientChromaticity(m_readStateIndex);
			}

			/**
			 * @brief Returns the ambient illuminance of the frame, in lux: the EFFECTIVE one the shading uses, zero when
			 * the sky drives the ambient.
			 * @return float
			 */
			[[nodiscard]]
			float
			ambientIlluminance () const noexcept
			{
				return m_viewMatrices.ambientIlluminance(m_readStateIndex);
			}

			/**
			 * @brief Returns the far plane distance of the view.
			 * @return float
			 */
			[[nodiscard]]
			float
			farPlane () const noexcept
			{
				return m_viewMatrices.farPlane();
			}

			/**
			 * @brief Returns the view matrix of the PREVIOUS rendered frame (the frame-history contract of the view
			 * matrices: what a temporal effect reprojects from).
			 * @note Identity until a first frame was archived: a consumer handles its own first-frame invalidation.
			 * @return const Base::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Base::Math::Matrix< 4, float > &
			previousViewMatrix () const noexcept
			{
				return m_viewMatrices.previousViewMatrix();
			}

			/**
			 * @brief Returns the projection matrix of the PREVIOUS rendered frame, without jitter (same contract as
			 * previousViewMatrix()).
			 * @return const Base::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Base::Math::Matrix< 4, float > &
			previousProjectionMatrix () const noexcept
			{
				return m_viewMatrices.previousProjectionMatrix();
			}

			/**
			 * @brief Returns the render state the scene latched for this frame.
			 * @note For what the logic thread publishes per state beside the view — a light's published block, a cloud's
			 * render state, an entity's world coordinates: they are read at the same state as the view, or the frame
			 * mixes two logic ticks. The view itself is read through the accessors above, never through this index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			readStateIndex () const noexcept
			{
				return m_readStateIndex;
			}

		private:

			const ViewMatricesInterface & m_viewMatrices;
			uint32_t m_readStateIndex;
	};
}
