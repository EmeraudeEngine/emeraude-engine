/*
 * src/Saphir/Generator/SceneRendering.cpp
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

#include "SceneRendering.hpp"

/* Local inclusions. */
#include "Libs/SourceCodeParser.hpp"
#include "Scenes/Scene.hpp"
#include "Saphir/Code.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace EmEn::Libs;
	using namespace Graphics;
	using namespace Vulkan;
	using namespace Keys;

	SceneRendering::SceneRendering (const std::string & shaderProgramName, const std::shared_ptr< const RenderTarget::Abstract > & renderTarget, const std::shared_ptr< const RenderableInstance::Abstract > & renderableInstance, uint32_t layerIndex, const Scenes::Scene & scene, RenderPassType renderPassType, const Settings & settings) noexcept
		: Abstract(shaderProgramName, renderTarget, renderableInstance, layerIndex),
		m_renderPassType(renderPassType),
		m_lightGenerator(renderPassType),
		m_scene(&scene)
	{
		if ( settings.get< bool >(NormalMappingEnabledKey, DefaultNormalMappingEnabled) )
		{
			this->enableFlag(NormalMappingEnabled);
		}

		if ( settings.get< bool >(HighQualityLightEnabledKey, DefaultHighQualityLightEnabled) )
		{
			this->enableFlag(HighQualityLightEnabled);
		}

		if ( settings.get< bool >(HighQualityReflectionEnabledKey, DefaultHighQualityReflectionEnabled) )
		{
			this->enableFlag(HighQualityReflectionEnabled);
		}
	}

	void
	SceneRendering::prepareUniformSets (SetIndexes & setIndexes) noexcept
	{
		setIndexes.enableSet(SetType::PerView);

		if ( this->isFlagEnabled(IsLightingEnabled) )
		{
			switch ( m_renderPassType )
			{
				case RenderPassType::DirectionalLightPass :
				case RenderPassType::DirectionalLightPassNoShadow :
				case RenderPassType::PointLightPass :
				case RenderPassType::PointLightPassNoShadow :
				case RenderPassType::SpotLightPass :
				case RenderPassType::SpotLightPassNoShadow :
					setIndexes.enableSet(SetType::PerLight);
					break;

				default:
					break;
			}
		}

		if ( this->materialEnabled() )
		{
			setIndexes.enableSet(SetType::PerModelLayer);
		}
	}

	bool
	SceneRendering::onGenerateShadersCode (Program & program) noexcept
	{
		/* Configure the light generator with the material for all shaders. */
		if ( this->isFlagEnabled(IsLightingEnabled) )
		{
			if ( m_renderPassType == RenderPassType::SimplePass && m_scene->lightSet().isUsingStaticLighting() )
			{
				const auto * staticLighting = m_scene->lightSet().getStaticLighting();

				if ( staticLighting == nullptr )
				{
					TraceError{ClassId} << "The static lighting is not available from the scene !";

					return false;
				}

				m_lightGenerator.setStaticLighting(staticLighting);
			}

			if ( this->materialEnabled() )
			{
				if ( !this->getMaterialInterface()->setupLightGenerator(m_lightGenerator) )
				{
					TraceError{ClassId} << "Unable to configure the light generator with material '" << this->getMaterialInterface()->name() << "' !";

					return false;
				}
			}
		}

		/* Generate the vertex shader stage. */
		if ( this->generateVertexShader(program) )
		{
			if ( this->debuggingEnabled() )
			{
				const auto * shader = program.vertexShader();

				TraceSuccess{ClassId} << "Vertex shader source '" << shader->name() << "' code generated.";

				TraceDebug{ClassId} << "\n" <<
					"****** START OF GENERATED GLSL VERTEX SHADER CODE ******" "\n" <<
					SourceCodeParser::parse(shader->sourceCode()) <<
					"****** END OF GENERATED GLSL VERTEX SHADER CODE ******" "\n" <<
					shader->getDeclarationStats() << "\n";
			}
		}
		else
		{
			Tracer::error(ClassId, "Unable to generate source code for the vertex shader stage !");

			return false;
		}

		/* Generate the fragment shader stage. */
		if ( this->generateFragmentShader(program) )
		{
			if ( this->debuggingEnabled() )
			{
				const auto * shader = program.fragmentShader();

				TraceSuccess{ClassId} << "Fragment shader source '" << shader->name() << "' code generated.";

				TraceDebug{ClassId} << "\n" <<
					"****** START OF GENERATED GLSL FRAGMENT SHADER CODE ******" "\n" <<
					SourceCodeParser::parse(shader->sourceCode()) <<
					"****** END OF GENERATED GLSL FRAGMENT SHADER CODE ******" "\n" <<
					shader->getDeclarationStats() << "\n";
			}
		}
		else
		{
			Tracer::error(ClassId, "Unable to generate source code for the fragment shader stage !");

			return false;
		}

		if ( this->debuggingEnabled() )
		{
			const auto shaderList = program.getShaderList();

			for ( const auto & shader: shaderList )
			{
				TraceDebug{ClassId} <<
					to_string(shader->type()) << " '" << shader->name() << "' generated into "
					"a source code of " << shader->sourceCode().size() << " bytes !";
			}
		}

		return true;
	}

	bool
	SceneRendering::isAdvancedRendering () const noexcept
	{
		if ( this->getMaterialInterface()->isComplex() )
		{
			return true;
		}

		switch ( m_renderPassType )
		{
			case RenderPassType::SimplePass :
				return m_scene->lightSet().isUsingStaticLighting();

			case RenderPassType::DirectionalLightPass :
			case RenderPassType::DirectionalLightPassNoShadow :
			case RenderPassType::PointLightPass :
			case RenderPassType::PointLightPassNoShadow :
			case RenderPassType::SpotLightPass :
			case RenderPassType::SpotLightPassNoShadow :
				return true;

			case RenderPassType::AmbientPass :
			default:
				return false;
		}
	}

	bool
	SceneRendering::onCreateDataLayouts (const SetIndexes & setIndexes, std::vector< std::shared_ptr< DescriptorSetLayout > > & descriptorSetLayouts, std::vector< VkPushConstantRange > & pushConstantRanges) noexcept
	{
		if ( m_scene != nullptr && setIndexes.isSetEnabled(SetType::PerLight) )
		{
			auto descriptorSetLayout = Scenes::LightSet::getDescriptorSetLayout(Renderer::instance()->layoutManager());

			if ( descriptorSetLayout == nullptr )
			{
				Tracer::error(ClassId, "Unable to get the light descriptor set layout !");

				return false;
			}

			descriptorSetLayouts.emplace_back(descriptorSetLayout);
		}

		/* Prepare the descriptor set layout for the model layer. */
		if ( this->materialEnabled() && setIndexes.isSetEnabled(SetType::PerModelLayer) )
		{
			auto descriptorSetLayout = this->getMaterialInterface()->descriptorSetLayout();

			if ( descriptorSetLayout == nullptr )
			{
				Tracer::error(ClassId, "Unable to get the material descriptor set layout !");

				return false;
			}

			descriptorSetLayouts.emplace_back(descriptorSetLayout);
		}

		Abstract::generatePushConstantRanges(this->shaderProgram()->vertexShader()->pushConstantBlockDeclarations(), pushConstantRanges, VK_SHADER_STAGE_VERTEX_BIT);

		return true;
	}

	bool
	SceneRendering::generateVertexShader (Program & program) noexcept
	{
		/* Create the vertex shader. */
		auto * vertexShader = program.initVertexShader(
			this->name( ) + "VertexShader",
			this->isFlagEnabled(IsInstancingEnabled),
			this->isAdvancedRendering(),
			this->isFlagEnabled(IsRenderableFacingCamera)
		);
		vertexShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		if ( !this->declareMatrixPushConstantBlock(*vertexShader) )
		{
			return false;
		}

		/* Declare the view uniform block. */
		if ( !this->declareViewUniformBlock(*vertexShader) )
		{
			return false;
		}

		/* NOTE: The position is always required and available. */
		if ( !vertexShader->requestSynthesizeInstruction(ShaderVariable::PositionScreenSpace) )
		{
			return false;
		}

		/* If present, generate the material shader code. */
		if ( this->materialEnabled() && !this->getMaterialInterface()->generateVertexShaderCode(*this, *vertexShader) )
		{
			TraceError{ClassId} << "Unable to generate vertex shader code part for material '" << this->getMaterialInterface()->name() << "' !";

			return false;
		}

		/* Generate the lighting shader code. */
		if ( m_scene->lightSet().isEnabled() && this->isFlagEnabled(IsLightingEnabled) )
		{
			if ( !m_lightGenerator.generateVertexShaderCode(*this, *vertexShader) )
			{
				Tracer::error(ClassId, "Unable to generate vertex shader code part for lighting !");

				return false;
			}
		}

		return vertexShader->generateSourceCode(*this);
	}

	bool
	SceneRendering::generateFragmentShader (Program & program) noexcept
	{
		/* Create the fragment shader. */
		auto * fragmentShader = program.initFragmentShader(this->name( ) + "FragmentShader");
		fragmentShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		/* Automatic input declarations from vertex shader. */
		if ( !fragmentShader->connectFromPreviousShader(*program.vertexShader()) )
		{
			return false;
		}

		/* Common part of fragment shader. */
		fragmentShader->declareDefaultOutputFragment();

		/* If a material is present, generate the shader code (optional). */
		if ( this->materialEnabled() && !this->getMaterialInterface()->generateFragmentShaderCode(*this, m_lightGenerator, *fragmentShader) )
		{
			TraceError{ClassId} << "Unable to generate fragment shader code part for material '" << this->getMaterialInterface()->name() << "' !";

			return false;
		}

		/* If the light is enabled, generate the shader code (optional). */
		if ( m_scene->lightSet().isEnabled() && this->isFlagEnabled(IsLightingEnabled) )
		{
			/* Declare the view uniform block. */
			if ( !m_scene->lightSet().isUsingStaticLighting() && !this->declareViewUniformBlock(*fragmentShader) )
			{
				return false;
			}

			if ( !m_lightGenerator.generateFragmentShaderCode(*this, *fragmentShader) )
			{
				Tracer::error(ClassId, "Unable to generate fragment shader code part for lighting !");

				return false;
			}
		}

		/* Generates the fragment output. */
		if ( m_scene->lightSet().isEnabled() && this->isFlagEnabled(IsLightingEnabled) )
		{
			Code{*fragmentShader, Location::Output} << ShaderVariable::OutputFragment << " = " << m_lightGenerator.fragmentColor() << ';';
		}
		else if ( this->materialEnabled() )
		{
			Code{*fragmentShader, Location::Output} << ShaderVariable::OutputFragment << " = " << this->getMaterialInterface()->fragmentColor() << ';';
		}
		else
		{
			Code{*fragmentShader, Location::Output} << ShaderVariable::OutputFragment << " = vec4(1.0, 0.0, 1.0, 1.0);";
		}

		/* TODO: Try to discard before ! */
		if ( this->materialEnabled() && this->getMaterialInterface()->blendingMode() != BlendingMode::None )
		{
			Code{*fragmentShader, Location::Output} << "if ( " << ShaderVariable::OutputFragment << ".a <= 0.0 ) discard;";
		}

		return fragmentShader->generateSourceCode(*this);
	}
	
	bool
	SceneRendering::onGraphicsPipelineConfiguration (const Program & /*program*/, GraphicsPipeline & graphicsPipeline) noexcept
	{
		const auto * renderableInstance = this->getRenderableInstance();
		const auto * renderable = renderableInstance->renderable();

		if ( !graphicsPipeline.configureRasterizationState(m_renderPassType, renderable->layerRasterizationOptions(this->layerIndex())) )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline rasterization state !");

			return false;
		}

		if ( !graphicsPipeline.configureDepthStencilState(m_renderPassType, *renderableInstance) )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline depth/stencil state !");

			return false;
		}

		if ( !graphicsPipeline.configureColorBlendState(m_renderPassType, *renderable->material(this->layerIndex())) )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline color blend state !");

			return false;
		}
		
		return true;
	}
}
