/*
 * src/Saphir/Generator/GizmoRendering.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

namespace EmEn::Saphir::Generator
{
	/**
	 * @brief This generator creates the pipeline to draw simple "gizmo" inside the 3D rendering for debugging.
	 * @extends EmEn::Saphir::Generator::Abstract This a generator.
	 */
	class GizmoRendering final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GizmoRendering"};

			/**
			 * @brief Constructs a graphics shader generator for a geometry.
			 * @param renderTarget A reference to a render target smart pointer.
			 */
			explicit GizmoRendering (const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget) noexcept;

		private:

			/** @copydoc EmEn::Saphir::Generator::Abstract::prepareUniformSets() */
			void prepareUniformSets (SetIndexes & setIndexes) noexcept override;

			/** @copydoc EmEn::Saphir::Generator::Abstract::onGenerateShadersCode() */
			[[nodiscard]]
			bool onGenerateShadersCode (Program & program) noexcept override;

			/** @copydoc EmEn::Saphir::Generator::Abstract::onCreateDataLayouts() */
			[[nodiscard]]
			bool onCreateDataLayouts (const SetIndexes & setIndexes, std::vector< std::shared_ptr< Vulkan::DescriptorSetLayout > > & descriptorSetLayouts, std::vector< VkPushConstantRange > & pushConstantRanges) noexcept override;

			/** @copydoc EmEn::Saphir::Generator::Abstract::onGraphicsPipelineConfiguration() */
			[[nodiscard]]
			bool onGraphicsPipelineConfiguration (const Program & program, Vulkan::GraphicsPipeline & graphicsPipeline) noexcept override;

			/**
			 * @brief Generates the vertex shader stage of the graphics pipeline.
			 * @param program A reference to the program being constructed.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateVertexShader (Program & program) noexcept;

			/**
			 * @brief Generates the fragment shader stage of the graphics pipeline.
			 * @param program A reference to the program being constructed.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateFragmentShader (Program & program) noexcept;
	};
}
