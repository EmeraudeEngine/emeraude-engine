/*
 * src/Scenes/Editor/Gizmo/Scale.hpp
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
	 * @brief Scale gizmo — 3 colored axis shafts with cube endpoints (RGB) + gray center cube for uniform scale.
	 *
	 * @extends EmEn::Scenes::Editor::Gizmo::Abstract
	 */
	class Scale final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GizmoScale"};

			/**
			 * @brief Constructs a scale gizmo.
			 */
			Scale () noexcept = default;

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

			/** @brief Sub-elements: shaft+cube per axis (6) + center cube (1) = 7. */
			static constexpr size_t SubElementCount{7};

			static constexpr size_t ShaftX{0};
			static constexpr size_t CubeX{1};
			static constexpr size_t ShaftY{2};
			static constexpr size_t CubeY{3};
			static constexpr size_t ShaftZ{4};
			static constexpr size_t CubeZ{5};
			static constexpr size_t CenterCube{6};

			/** @brief Geometry parameters. */
			static constexpr float ShaftRadius{0.02F};
			static constexpr float ShaftLength{0.75F};
			static constexpr float EndCubeSize{0.08F};
			static constexpr float CenterCubeSize{0.06F};
			static constexpr float Gap{0.1F};

			/** @brief Highlight parameters. */
			static constexpr float HighlightScale{1.15F};
			static constexpr float HighlightBrightness{1.6F};

			void renderSubElement (const Vulkan::CommandBuffer & commandBuffer, const Graphics::ViewMatricesInterface & viewMatrices, size_t subElementIndex, const Libs::Math::Matrix< 4, float > & axisRotation, bool isHighlighted) const noexcept;

			std::array< std::shared_ptr< Graphics::Geometry::Interface >, SubElementCount > m_subGeometries{};
	};
}
