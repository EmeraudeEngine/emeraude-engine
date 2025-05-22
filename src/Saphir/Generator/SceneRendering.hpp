/*
 * src/Saphir/Generator/SceneRendering.hpp
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

/* Local inclusions for usages. */
#include "Saphir/LightGenerator.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class Scene;
}

namespace EmEn::Saphir::Generator
{
	/**
	 * @brief This generator builds the graphics pipeline to display a scene.
	 * @extends EmEn::Saphir::Generator::Abstract This a generator.
	 */
	class SceneRendering final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SceneRendering"};

			/**
			 * @brief Constructs a graphics shader generator for a geometry.
			 * @param shaderProgramName A reference to a string.
			 * @param renderTarget A reference to the render target smart pointer.
			 * @param renderableInstance A reference to the renderable instance smart pointer.
			 * @param layerIndex The renderable instance layer.
			 * @param scene A reference to a scene.
			 * @param renderPassType The render pass type to know which kind of render is implied.
			 * @param settings A reference to the core settings.
			 */
			SceneRendering (const std::string & shaderProgramName, const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget, const std::shared_ptr< const Graphics::RenderableInstance::Abstract > & renderableInstance, uint32_t layerIndex, const Scenes::Scene & scene, Graphics::RenderPassType renderPassType, const Settings & settings) noexcept;

			/**
			 * @brief Returns the render pass type.
			 * @return Graphics::RenderPassType
			 */
			[[nodiscard]]
			Graphics::RenderPassType
			renderPassType () const noexcept
			{
				return m_renderPassType;
			}

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
			 * @brief Returns whether the rendering is advanced and needs more specific matrices.
			 * @return bool
			 */
			[[nodiscard]]
			bool isAdvancedRendering () const noexcept;

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

			Graphics::RenderPassType m_renderPassType;
			LightGenerator m_lightGenerator;
			const Scenes::Scene * m_scene{nullptr};
	};
}
