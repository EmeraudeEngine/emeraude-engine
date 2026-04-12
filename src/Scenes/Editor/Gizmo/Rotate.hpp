/*
 * src/Scenes/Editor/Gizmo/Rotate.hpp
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
	 * @brief Rotation gizmo — 3 colored torus rings (X red, Y green, Z blue).
	 *
	 * Each ring represents rotation around one axis. Uses ResourceGenerator::torus()
	 * and GizmoRendering for the pipeline. Same standalone render pattern as Translate.
	 *
	 * @extends EmEn::Scenes::Editor::Gizmo::Abstract
	 */
	class Rotate final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GizmoRotate"};

			/**
			 * @brief Constructs a rotation gizmo.
			 */
			Rotate () noexcept = default;

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

			/** @brief Sub-element count: one ring per axis. */
			static constexpr size_t SubElementCount{3};

			/** @brief Indices. */
			static constexpr size_t RingX{0};
			static constexpr size_t RingY{1};
			static constexpr size_t RingZ{2};

			/** @brief Ring geometry parameters. */
			static constexpr float RingMajorRadius{0.85F};
			static constexpr float RingMinorRadius{0.015F};

			/** @brief Scale multiplier when highlighted. */
			static constexpr float HighlightScale{1.15F};

			/** @brief Highlight brightness factor. */
			static constexpr float HighlightBrightness{1.6F};

			/**
			 * @brief Renders a single ring sub-element.
			 */
			void renderSubElement (const Vulkan::CommandBuffer & commandBuffer, const Graphics::ViewMatricesInterface & viewMatrices, size_t subElementIndex, const Libs::Math::Matrix< 4, float > & axisRotation, bool isHighlighted) const noexcept;

			/** @brief Geometry for each ring. */
			std::array< std::shared_ptr< Graphics::Geometry::Interface >, SubElementCount > m_subGeometries{};
	};
}
