/*
 * src/Saphir/Generator/Abstract.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"

/* Local inclusions for usages. */
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Saphir/Program.hpp"

namespace EmEn
{
	namespace Graphics
	{
		namespace RenderTarget
		{
			class Abstract;
		}

		class Renderer;
	}

	namespace Vulkan
	{
		class DescriptorSetLayout;
	}

	class Settings;
}

namespace EmEn::Saphir::Generator
{
	/** @brief Generator flag bits. */
	enum GeneratorFlagBits : uint32_t
	{
		None = 0,
		Debug = 1 << 0,
		NormalMappingEnabled = 1 << 1,
		HighQualityLightEnabled = 1 << 2,
		HighQualityReflectionEnabled = 1 << 3,
		IsInstancingEnabled = 1 << 4,
		IsRenderableFacingCamera = 1 << 5,
		IsLightingEnabled = 1 << 6,
	};

	/**
	 * @brief The base class for every shader program generator.
	 * @extends EmEn::Libs::NameableTrait This will hold the name of the program generated.
	 * @extends EmEn::Libs::FlagTrait Enables flag ability for parameters.
	 */
	class Abstract : public Libs::NameableTrait, public Libs::FlagTrait< uint32_t >
	{
		public:

			static constexpr auto DefaultGLSLVersion{"460"};
			static constexpr auto DefaultGLSLProfile{"core"};

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (const Abstract & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (Abstract && copy) noexcept = default;

			/**
			 * @brief Assignment operator.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (const Abstract & copy) noexcept = default;

			/**
			 * @brief Move operator.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (Abstract && copy) noexcept = default;

			/** 
			 * @brief Destructs the abstract shader generator.
			 */
			~Abstract () override = default;

			/**
			 * @brief Enables debugging. This will print stats and generated source code.
			 * @param state The state
			 * @return void
			 */
			void
			enableDebugging (bool state) noexcept
			{
				if ( state )
				{
					this->enableFlag(Debug);
				}
				else
				{
					this->disableFlag(Debug);
				}
			}

			/**
			 * @brief Returns whether the debugging is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			debuggingEnabled () const noexcept
			{
				return this->isFlagEnabled(Debug);
			}

			/**
			 * @brief Returns whether the use of normal mapping is allowed.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			normalMappingEnabled () const noexcept
			{
				return this->isFlagEnabled(NormalMappingEnabled);
			}

			/**
			 * @brief Returns whether the light shader generation is high quality (most operation is performed per-fragment).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			highQualityLightEnabled () const noexcept
			{
				return this->isFlagEnabled(HighQualityLightEnabled);
			}

			/**
			 * @brief Returns whether the reflection shader generation is high quality (most operation is performed per-fragment).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			highQualityReflectionEnabled () const noexcept
			{
				return this->isFlagEnabled(HighQualityReflectionEnabled);
			}

			/**
			 * @brief Returns whether the renderable is using instancing.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInstancingEnabled () const noexcept
			{
				return this->isFlagEnabled(IsInstancingEnabled);
			}

			/**
			 * @brief Returns whether the renderable is facing the camera like sprites.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRenderableFacingCamera () const noexcept
			{
				return this->isFlagEnabled(IsRenderableFacingCamera);
			}

			/**
			 * @brief Returns the render target pointer.
			 * @return std::shared_ptr< const Graphics::RenderTarget::Abstract >
			 */
			[[nodiscard]]
			virtual
			std::shared_ptr< const Graphics::RenderTarget::Abstract >
			renderTarget () const noexcept
			{
				return m_renderTarget;
			}

			/**
			 * @brief Returns whether the generator have access to the renderable instance.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRenderableInstanceAvailable () const noexcept
			{
				return m_renderableInstance != nullptr;
			}

			/**
			 * @brief Returns the renderable instance pointer.
			 * @return const Graphics::Renderable::Interface *
			 */
			[[nodiscard]]
			const Graphics::RenderableInstance::Abstract *
			getRenderableInstance () const noexcept
			{
				return m_renderableInstance.get();
			}

			/**
			 * @brief Returns the renderable interface pointer.
			 * @return const Graphics::Renderable::Interface *
			 */
			[[nodiscard]]
			const Graphics::Renderable::Interface *
			getRenderable () const noexcept
			{
				if ( !this->isRenderableInstanceAvailable() )
				{
					return nullptr;
				}

				return m_renderableInstance->renderable();
			}

			/**
			 * @brief Returns the geometry interface pointer.
			 * @return const Graphics::Geometry::Interface *
			 */
			[[nodiscard]]
			const Graphics::Geometry::Interface *
			getGeometryInterface () const noexcept
			{
				if ( !this->isRenderableInstanceAvailable() )
				{
					return nullptr;
				}

				return m_renderableInstance->renderable()->geometry();
			}

			/**
			 * @brief Returns whether the generator will use a material.
			 * @note The material is provided by the constructor with a renderable instance.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			materialEnabled () const noexcept
			{
				if ( !this->isRenderableInstanceAvailable() )
				{
					return false;
				}

				return m_renderableInstance->renderable()->material(m_layerIndex) != nullptr;
			}

			/**
			 * @brief Returns the renderable instance layer index being in use.
			 * @return
			 */
			[[nodiscard]]
			uint32_t
			layerIndex () const noexcept
			{
				return m_layerIndex;
			}

			/**
			 * @brief Returns the material interface pointer.
			 * @warning Can be a null pointer, check with GraphicsShaderGenerator::materialEnabled().
			 * @return const Graphics::Material::Interface *
			 */
			[[nodiscard]]
			const Graphics::Material::Interface *
			getMaterialInterface () const noexcept
			{
				if ( !this->isRenderableInstanceAvailable() )
				{
					return nullptr;
				}

				return m_renderableInstance->renderable()->material(m_layerIndex);
			}

			/**
			 * @brief Returns the shader program.
			 * @return std::shared_ptr< Program >
			 */
			[[nodiscard]]
			std::shared_ptr< Program >
			shaderProgram () const noexcept
			{
				return m_shaderProgram;
			}

			/**
			 * @brief Returns the next available location for shaders.
			 * @param increment The increment value for the next location. Default 1.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t getNextShaderVariableLocation (uint32_t increment = 1) noexcept;

			/**
			 * @brief Declares the view uniform block according to input information.
			 * @param shader A reference to a shader where to declare the uniform block.
			 * @param binding The binding point number. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool declareViewUniformBlock (AbstractShader & shader, uint32_t binding = 0) const noexcept;

			/**
			 * @brief Declares the view uniform block according to input information.
			 * @param material A reference to a material.
			 * @param shader A reference to a shader where to declare the uniform block.
			 * @param binding The binding point number. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool declareMaterialUniformBlock (const Graphics::Material::Interface & material, AbstractShader & shader, uint32_t binding = 0) const noexcept;

			/**
			 * @brief Declares the push constant block according to input information.
			 * @param shader A reference to a shader where to declare the uniform block.
			 * @return bool
			 */
			[[nodiscard]]
			bool declareMatrixPushConstantBlock (AbstractShader & shader) const noexcept;

			/**
			 * @brief Generates the shader program.
			 * @param renderer A reference to the graphics renderer.
			 * @param GLSLVersion The GLSL version in use. Default "460".
			 * @param GLSLProfile The GLSL profile in use. Default "core".
			 * @return bool
			 */
			[[nodiscard]]
			bool generateShaderProgram (Graphics::Renderer & renderer, const std::string & GLSLVersion = DefaultGLSLVersion, const std::string & GLSLProfile = DefaultGLSLProfile) noexcept;

		protected:

			/**
			 * @brief Constructs an abstract shader program generator.
			 * @param shaderProgramName A reference to string for the program being generated.
			 * @param renderTarget A reference to a renderTarget smart pointer.
			 */
			Abstract (const std::string & shaderProgramName, const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget) noexcept;

			/**
			 * @brief Constructs an abstract shader program generator with a specified renderable instance.
			 * @param shaderProgramName A reference to string for the program being generated.
			 * @param renderTarget A reference to a renderTarget smart pointer.
			 * @param renderableInstance A reference to a renderTarget smart pointer.
			 * @param layerIndex The renderable instance layer targeted.
			 */
			Abstract (const std::string & shaderProgramName, const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget, const std::shared_ptr< const Graphics::RenderableInstance::Abstract > & renderableInstance, uint32_t layerIndex) noexcept;

			/**
			 * @brief Constructs an abstract shader program generator with a generic geometry specification.
			 * @param shaderProgramName A reference to string for the program being generated.
			 * @param renderTarget A reference to a renderTarget smart pointer.
			 * @param topology A reference to a renderTarget smart pointer.
			 * @param geometryFlags The renderable instance layer targeted.
			 */
			Abstract (const std::string & shaderProgramName, const std::shared_ptr< const Graphics::RenderTarget::Abstract > & renderTarget, Graphics::Topology topology, uint32_t geometryFlags) noexcept;

			/**
			 * @brief Generates the fall-back vertex shader stage of the graphics pipeline.
			 * @param program A reference to the program being constructed.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateFallBackVertexShader (Program & program) noexcept;

			/**
			 * @brief Generates the fall-back fragment shader stage of the graphics pipeline.
			 * @param program A reference to the program being constructed.
			 * @return bool
			 */
			[[nodiscard]]
			bool generateFallBackFragmentShader (Program & program) noexcept;

			/**
			 * @brief Prepares the uniform sets according to incoming rendering information.
			 * @param setIndexes A reference to the set indexes structure.
			 * @return void
			 */
			virtual void prepareUniformSets (SetIndexes & setIndexes) noexcept = 0;

			/**
			 * @brief Main method to generate program shaders.
			 * @param program A reference to the program being constructed.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool onGenerateShadersCode (Program & program) noexcept = 0;

			/**
			 * @brief Methods to override for generating specific program layout.
			 * @note The render target descriptor set layout is already present.
			 * @param setIndexes A reference to a set indexes structure.
			 * @param descriptorSetLayouts A reference to as list of descriptor set layouts to complete.
			 * @param pushConstantRanges A reference to as list of push constant ranges to complete.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool onCreateDataLayouts (const SetIndexes & setIndexes, std::vector< std::shared_ptr< Vulkan::DescriptorSetLayout > > & descriptorSetLayouts, std::vector< VkPushConstantRange > & pushConstantRanges) noexcept = 0;

			/**
			 * @brief Configures the graphics pipeline from child shader generators.
			 * @param program A reference to the constructed program.
			 * @param graphicsPipeline A reference to the graphics pipeline to set up.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool onGraphicsPipelineConfiguration (const Program & program, Vulkan::GraphicsPipeline & graphicsPipeline) noexcept = 0;

			/**
			 * @brief Generates push constant ranges from push constant blocks code declaration.
			 * @todo: Check for a better way to use push constant from the right shader declaration to the right stage in vulkan, maybe store push constant at program level instead of shader.
			 * @param pushConstantBlocks A reference to a vector of push constant blocks.
			 * @param pushConstantRanges A reference to a vector of push constant ranges.
			 * @param stageFlags
			 * @return void
			 */
			static void generatePushConstantRanges (const std::vector< Declaration::PushConstantBlock > & pushConstantBlocks, std::vector< VkPushConstantRange > & pushConstantRanges, VkShaderStageFlags stageFlags) noexcept;

		private:

			/**
			 * @brief Creates all data description layouts for the graphics pipeline.
			 * @param renderer A reference to the renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDataLayout (Graphics::Renderer & renderer) noexcept;

			/**
			 * @brief Creates the final Vulkan graphics pipeline.
			 * @param renderer A reference to the renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createGraphicsPipeline (Graphics::Renderer & renderer) noexcept;

			std::shared_ptr< const Graphics::RenderTarget::Abstract > m_renderTarget;
			std::shared_ptr< const Graphics::RenderableInstance::Abstract > m_renderableInstance;
			uint32_t m_layerIndex{0};
			Graphics::Topology m_topology{Graphics::Topology::TriangleList};
			uint32_t m_geometryFlags{0};
			std::shared_ptr< Program > m_shaderProgram;
			uint32_t m_nextShaderVariableLocation{0};
	};
}
