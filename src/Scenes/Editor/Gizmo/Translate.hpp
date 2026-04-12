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

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

namespace EmEn::Scenes::Editor::Gizmo
{
	/**
	 * @brief Translation gizmo — 3 colored arrows (X red, Y green, Z blue).
	 *
	 * Uses ResourceGenerator::axis() for geometry and GizmoRendering for the pipeline.
	 * Renders standalone with depth test disabled and constant screen size.
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

			/** @copydoc EmEn::Scenes::Editor::Gizmo::Abstract::hitTest() */
			[[nodiscard]]
			AxisID hitTest (const Libs::Math::Space3D::Segment< float > & ray) const noexcept override;

			/** @copydoc EmEn::Scenes::Editor::Gizmo::Abstract::render() */
			void render (const Vulkan::CommandBuffer & commandBuffer, const Graphics::ViewMatricesInterface & viewMatrices) const noexcept override;

		private:

			/** @brief Hit-test tolerance radius for axis cylinders (in gizmo local space). */
			static constexpr float HitRadius{0.08F};

			/** @brief Axis length in gizmo local space (matches ResourceGenerator::axis size). */
			static constexpr float AxisLength{1.0F};
	};
}
