/*
 * src/Scenes/Editor/Gizmo/Translate.hpp
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
#include <array>

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

namespace EmEn::Scenes::Editor::Gizmo
{
	/**
	 * @brief Translation gizmo — 3 independent colored arrows (X red, Y green, Z blue) + center sphere.
	 *
	 * Each axis is a separate geometry rendered independently with its own model matrix,
	 * enabling per-axis hover highlighting. Uses ResourceGenerator::arrow() for each axis
	 * and GizmoRendering for the pipeline.
	 *
	 * @extends EmEn::Scenes::Editor::Gizmo::Abstract
	 */
	class Translate final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GizmoTranslate"};

			/**
			 * @brief Constructs a translation gizmo.
			 */
			Translate () noexcept = default;

			/** @copydoc EmEn::Scenes::Editor::Gizmo::Abstract::create() */
			[[nodiscard]]
			bool create (Graphics::Renderer & renderer, Resources::Manager & resourceManager, const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget) noexcept override;

			/** @copydoc EmEn::Scenes::Editor::Gizmo::Abstract::destroy() */
			void destroy () noexcept override;

			/** @copydoc EmEn::Scenes::Editor::Gizmo::Abstract::hitTest() */
			[[nodiscard]]
			AxisID hitTest (const Libs::Math::Space3D::Segment< float > & ray) const noexcept override;

			/** @copydoc EmEn::Scenes::Editor::Gizmo::Abstract::render() */
			void render (const Vulkan::CommandBuffer & commandBuffer, const Graphics::ViewMatricesInterface & viewMatrices) const noexcept override;

		private:

			/** @brief Sub-element count: X arrow + Y arrow + Z arrow + center sphere. */
			static constexpr size_t SubElementCount{4};

			/** @brief Indices into the sub-element array. */
			static constexpr size_t ArrowX{0};
			static constexpr size_t ArrowY{1};
			static constexpr size_t ArrowZ{2};
			static constexpr size_t CenterSphere{3};

			/** @brief Hit-test tolerance radius for axis cylinders (in gizmo local space). */
			static constexpr float HitRadius{0.2F};

			/** @brief Axis length in gizmo local space. */
			static constexpr float AxisLength{1.0F};

			/** @brief Center sphere radius in gizmo local space. */
			static constexpr float CenterRadius{0.06F};

			/** @brief Scale multiplier when an axis is highlighted. */
			static constexpr float HighlightScale{1.2F};

			/** @brief Highlight factor for the shader (1.0 = normal, >1.0 = brighter). */
			static constexpr float HighlightBrightness{1.6F};

			/**
			 * @brief Renders a single sub-element with its own model matrix and highlight state.
			 * @param commandBuffer The command buffer.
			 * @param viewMatrices The view matrices.
			 * @param subElementIndex The sub-element index.
			 * @param axisRotation The rotation matrix to orient the arrow along its axis.
			 * @param isHighlighted Whether this sub-element is currently hovered.
			 * @return void
			 */
			void renderSubElement (const Vulkan::CommandBuffer & commandBuffer, const Graphics::ViewMatricesInterface & viewMatrices, size_t subElementIndex, const Libs::Math::Matrix< 4, float > & axisRotation, bool isHighlighted) const noexcept;

			/** @brief Geometry for each sub-element (3 arrows + 1 sphere). */
			std::array< std::shared_ptr< Graphics::Geometry::Interface >, SubElementCount > m_subGeometries{};
	};
}
