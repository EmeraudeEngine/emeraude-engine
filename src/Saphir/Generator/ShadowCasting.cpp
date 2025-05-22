/*
 * src/Saphir/Generator/ShadowRendering.cpp
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

#include "ShadowCasting.hpp"

/* Local inclusions. */
#include "Libs/SourceCodeParser.hpp"
#include "Saphir/Code.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace EmEn::Libs;
	using namespace Graphics;
	using namespace Vulkan;
	using namespace Keys;

	ShadowCasting::ShadowCasting (const std::shared_ptr< const RenderTarget::Abstract > & renderTarget, const std::shared_ptr< const RenderableInstance::Abstract > & renderableInstance, uint32_t layerIndex) noexcept
		: Abstract(ClassId, renderTarget, renderableInstance, layerIndex)
	{
		if ( renderableInstance->instancingEnabled() )
		{
			this->enableFlag(IsInstancingEnabled);
		}

		if ( renderableInstance->isFacingCamera() )
		{
			this->enableFlag(IsRenderableFacingCamera);
		}
	}

	void
	ShadowCasting::prepareUniformSets (SetIndexes & setIndexes) noexcept
	{
		if ( this->isFlagEnabled(IsInstancingEnabled) )
		{
			setIndexes.enableSet(SetType::PerView);
		}
	}

	bool
	ShadowCasting::onGenerateShadersCode (Program & program) noexcept
	{
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

		/* Generate the geometry shader stage. */
		if ( this->renderTarget()->isCubemap() )
		{
			if ( this->generateGeometryShader(program) )
			{
				if ( this->debuggingEnabled() )
				{
					const auto * shader = program.geometryShader();

					TraceSuccess{ClassId} << "Geometry shader source '" << shader->name() << "' code generated.";

					TraceDebug{ClassId} << "\n" <<
						"****** START OF GENERATED GLSL GEOMETRY SHADER CODE ******" "\n" <<
						SourceCodeParser::parse(shader->sourceCode()) <<
						"****** END OF GENERATED GLSL GEOMETRY SHADER CODE ******" "\n" <<
						shader->getDeclarationStats() << "\n";
				}
			}
			else
			{
				Tracer::error(ClassId, "Unable to generate source code for the geometry shader stage !");

				return false;
			}
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
	ShadowCasting::onCreateDataLayouts (const SetIndexes & /*setIndexes*/, std::vector< std::shared_ptr< DescriptorSetLayout > > & /*descriptorSetLayouts*/, std::vector< VkPushConstantRange > & pushConstantRanges) noexcept
	{
		Abstract::generatePushConstantRanges(this->shaderProgram()->vertexShader()->pushConstantBlockDeclarations(), pushConstantRanges, VK_SHADER_STAGE_VERTEX_BIT);

		return true;
	}

	bool
	ShadowCasting::generateVertexShader (Program & program) noexcept
	{
		auto * vertexShader = program.initVertexShader(
			this->name( ) + "VertexShader",
			this->isFlagEnabled(IsInstancingEnabled),
			false,
			this->isFlagEnabled(IsRenderableFacingCamera)
		);
		vertexShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		if ( !this->declareMatrixPushConstantBlock(*vertexShader) )
		{
			return false;
		}

		if ( this->renderTarget()->isCubemap() )
		{
			vertexShader->requestSynthesizeInstruction(ShaderVariable::GLPositionWorldSpace);
		}
		else
		{
			vertexShader->requestSynthesizeInstruction(ShaderVariable::PositionScreenSpace);
		}

		return vertexShader->generateSourceCode(*this);
	}

	bool
	ShadowCasting::generateGeometryShader (Program & program) noexcept
	{
		auto * geometryShader = program.initGeometryShader(
			this->name( ) + "GeometryShader",
			Declaration::InputPrimitive{Declaration::InputPrimitiveType::Triangles},
			Declaration::OutputPrimitive{Declaration::OutputPrimitiveType::TriangleStrip, 18}
		);
		geometryShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		if ( !this->declareMatrixPushConstantBlock(*geometryShader) )
		{
			return false;
		}

		/* Declare the view uniform block. */
		if ( !this->declareViewUniformBlock(*geometryShader) )
		{
			return false;
		}

		geometryShader->declare(Declaration::StageOutput{this->getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::FragCoord, GLSL::Smooth});

		Code{*geometryShader} <<
			"for ( int faceIndex = 0; faceIndex < 6; ++faceIndex )" << Line::End <<
			"{" << Line::End <<
			"	gl_Layer = faceIndex;" << Line::Blank <<

			"	for ( int i = 0; i < gl_in.length(); ++i )" << Line::End <<
			"	{" << Line::End <<
			"		" << ShaderVariable::FragCoord << " = gl_in[i].gl_Position;" << Line::End <<
			"		gl_Position = " << CubeViewUB(UniformBlock::Component::Instance, "faceIndex", UniformBlock::Component::ViewProjectionMatrix) << " * " << ShaderVariable::FragCoord << ";" << Line::End <<
			"		EmitVertex();" << Line::End <<
			"	}" << Line::Blank <<

			"	EndPrimitive();" << Line::End <<
			"}";

		return geometryShader->generateSourceCode(*this);
	}

	bool
	ShadowCasting::generateFragmentShader (Program & program) noexcept
	{
		auto * fragmentShader = program.initFragmentShader(this->name( ) + "FragmentShader");
		fragmentShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		if ( this->renderTarget()->isCubemap() )
		{
			const auto position = CubeViewUB(UniformBlock::Component::Instance, "0", UniformBlock::Component::PositionWorldSpace);
			const auto viewProperties = CubeViewUB(UniformBlock::Component::Instance, "0", UniformBlock::Component::ViewProperties);

			/* Declare the view uniform block. */
			if ( !this->declareViewUniformBlock(*fragmentShader) )
			{
				return false;
			}

			if ( !fragmentShader->connectFromPreviousShader(*program.geometryShader()) )
			{
				return false;
			}

			Code{*fragmentShader} << "const float distance = distance(" << ShaderVariable::FragCoord << ", " << position << ") / " << viewProperties << ".w;";

			/* Fragment as a color output for visualization. */
			if ( m_enableColorOutput )
			{
				fragmentShader->declareDefaultOutputFragment();

				Code{*fragmentShader, Location::Output} << ShaderVariable::OutputFragment << " = vec4(distance, distance, distance, 1.0);";
			}

			Code{*fragmentShader, Location::Output} << "gl_FragDepth = distance;";
		}
		else
		{
			/* Fragment as a color output for visualization. */
			if ( m_enableColorOutput )
			{
				fragmentShader->declareDefaultOutputFragment();

				Code{*fragmentShader, Location::Output} << ShaderVariable::OutputFragment << " = vec4(gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z, 1.0);";
			}

			/* NOTE: Already done by the default behavior. */
			Code{*fragmentShader, Location::Output} << "gl_FragDepth = gl_FragCoord.z;";
		}

		return fragmentShader->generateSourceCode(*this);
	}

	bool
	ShadowCasting::onGraphicsPipelineConfiguration (const Program & /*program*/, GraphicsPipeline & graphicsPipeline) noexcept
	{
		if ( !graphicsPipeline.configureRasterizationState(RenderPassType::SimplePass) )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline rasterization state !");

			return false;
		}

		{
			VkPipelineDepthStencilStateCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.depthTestEnable = VK_FALSE;
			createInfo.depthWriteEnable = VK_FALSE;
			createInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			createInfo.depthBoundsTestEnable = VK_FALSE;
			createInfo.stencilTestEnable = VK_FALSE;
			createInfo.minDepthBounds = 0.0F;
			createInfo.maxDepthBounds = 1.0F;

			if ( !graphicsPipeline.configureDepthStencilState(createInfo) )
			{
				Tracer::error(ClassId, "Unable to configure the graphics pipeline depth/stencil state !");

				return false;
			}
		}

		const std::vector< VkDynamicState > dynamicStates{
			VK_DYNAMIC_STATE_VIEWPORT
		};

		if ( !graphicsPipeline.configureDynamicStates(dynamicStates) )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline dynamic state !");

			return false;
		}

		return true;
	}
}
