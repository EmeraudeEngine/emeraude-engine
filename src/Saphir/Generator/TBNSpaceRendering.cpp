/*
 * src/Saphir/Generator/TBNSpaceRendering.cpp
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

#include "TBNSpaceRendering.hpp"

/* Local inclusions. */
#include "Libs/SourceCodeParser.hpp"
#include "Saphir/Code.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace EmEn::Libs;
	using namespace Graphics;
	using namespace Vulkan;
	using namespace Keys;

	TBNSpaceRendering::TBNSpaceRendering (const std::shared_ptr< const RenderTarget::Abstract > & renderTarget, const std::shared_ptr< const RenderableInstance::Abstract > & renderableInstance, uint32_t layerIndex) noexcept
		: Abstract(ClassId, renderTarget, renderableInstance, layerIndex)
	{

	}

	bool
	TBNSpaceRendering::onGenerateShadersCode (Program & program) noexcept
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
	TBNSpaceRendering::onCreateDataLayouts (const SetIndexes & /*setIndexes*/, std::vector< std::shared_ptr< DescriptorSetLayout > > & /*descriptorSetLayouts*/, std::vector< VkPushConstantRange > & pushConstantRanges) noexcept
	{
		Abstract::generatePushConstantRanges(
			this->shaderProgram()->vertexShader()->pushConstantBlockDeclarations(),
			pushConstantRanges,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT
		);

		return true;
	}

	bool
	TBNSpaceRendering::generateVertexShader (Program & program) noexcept
	{
		/* Create the vertex shader. */
		auto * vertexShader = program.initVertexShader(
			this->name( ) + "VertexShader",
			this->isFlagEnabled(IsInstancingEnabled),
			false,
			this->isFlagEnabled(IsRenderableFacingCamera)
		);
		vertexShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		/* Input declarations. */
		vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::Position}, true);
		vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::Tangent});
		vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::Binormal});
		vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::Normal}, true);

		if ( !this->declareMatrixPushConstantBlock(*vertexShader) )
		{
			return false;
		}

		if ( this->isFlagEnabled(IsInstancingEnabled) )
		{
			/* Declare the view uniform block. */
			if ( !this->declareViewUniformBlock(*vertexShader) )
			{
				return false;
			}

			vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::ModelMatrixR0});
			vertexShader->declare(Declaration::StageOutput{this->getNextShaderVariableLocation(), GLSL::Matrix4, ShaderVariable::ModelViewProjectionMatrix, GLSL::Flat});

			Code{*vertexShader, Location::Output} << ShaderVariable::ModelViewProjectionMatrix << " = " << ViewUB(UniformBlock::Component::ViewProjectionMatrix) << " * " << Attribute::ModelMatrix << ';';
		}

		/* Output declarations. */
		vertexShader->declare(Declaration::StageOutput{this->getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::TangentWorldSpace});
		vertexShader->declare(Declaration::StageOutput{this->getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::BinormalWorldSpace});
		vertexShader->declare(Declaration::StageOutput{this->getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::NormalWorldSpace});

		/* Instructions. */
		Code{*vertexShader, Location::Top} << "const float length = 10.0;";

		Code{*vertexShader, Location::Output} <<
			"gl_Position = vec4(" << Attribute::Position << ", 1.0);" << Line::End <<
			ShaderVariable::TangentWorldSpace << " = vec4(" << Attribute::Tangent << " * length, 0.0);" << Line::End <<
			ShaderVariable::BinormalWorldSpace << " = vec4(" << Attribute::Binormal << " * length, 0.0);" << Line::End <<
			ShaderVariable::NormalWorldSpace << " = vec4(" << Attribute::Normal << " * length, 0.0);";

		return vertexShader->generateSourceCode(*this);
	}

	bool
	TBNSpaceRendering::generateGeometryShader (Program & program) noexcept
	{
		/* Create the fragment shader. */
		auto * geometryShader = program.initGeometryShader(
			this->name( ) + "GeometryShader",
			Declaration::InputPrimitive{Declaration::InputPrimitiveType::Triangles},
			Declaration::OutputPrimitive{Declaration::OutputPrimitiveType::LineStrip, 18}
		);
		geometryShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		if ( !this->declareMatrixPushConstantBlock(*geometryShader) )
		{
			return false;
		}

		/* Automatic input declarations from vertex shader. */
		if ( !geometryShader->connectFromPreviousShader(*program.vertexShader()) )
		{
			return false;
		}

		std::string modelViewProjectionMatrix{};

		if ( this->isFlagEnabled(IsInstancingEnabled) )
		{
			modelViewProjectionMatrix = (std::stringstream{} << ShaderVariable::ModelViewProjectionMatrix << "[index]").str();
		}
		else
		{
			modelViewProjectionMatrix = MatrixPC(PushConstant::Component::ModelViewProjectionMatrix);
		}

		/* Output declarations. */
		geometryShader->declare(Declaration::StageOutput{this->getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::PrimaryVertexColor, GLSL::Smooth});

		{
			Code{*geometryShader} <<
				"const vec4 Black = vec4(0.13, 0.13, 0.13, 1.0);" << Line::End <<
				"const vec4 Red = vec4(1.0, 0.0, 0.0, 1.0);" << Line::End <<
				"const vec4 Green = vec4(0.0, 1.0, 0.0, 1.0);" << Line::End <<
				"const vec4 Blue = vec4(0.0, 0.0, 1.0, 1.0);" << Line::End <<
				"const float VectorLength = 4.0;" << Line::Blank <<

				"for ( int index = 0; index < gl_in.length(); index++ )" << Line::End <<
				"{" << Line::End <<
				/* Tangent (X-Axis -> R) */
				"	" << ShaderVariable::PrimaryVertexColor << " = Black;" << Line::End <<
				"	gl_Position = " << modelViewProjectionMatrix << " * gl_in[index].gl_Position;" << Line::End <<
				"	EmitVertex();" << Line::Blank <<

				"	" << ShaderVariable::PrimaryVertexColor << " = Red;" << Line::End <<
				"	gl_Position = " << modelViewProjectionMatrix << " * (gl_in[index].gl_Position + " << ShaderVariable::TangentWorldSpace << "[index]) * VectorLength;" << Line::End <<
				"	EmitVertex();" << Line::Blank <<

				"	EndPrimitive();" << Line::Blank <<

				/* Binormal (Y-Axis -> G) */
				"	" << ShaderVariable::PrimaryVertexColor << " = Black;" << Line::End <<
				"	gl_Position = " << modelViewProjectionMatrix << " * gl_in[index].gl_Position;" << Line::End <<
				"	EmitVertex();" << Line::Blank <<

				"	" << ShaderVariable::PrimaryVertexColor << " = Green;" << Line::End <<
				"	gl_Position = " << modelViewProjectionMatrix << " * (gl_in[index].gl_Position + " << ShaderVariable::BinormalWorldSpace << "[index]) * VectorLength;" << Line::End <<
				"	EmitVertex();" << Line::Blank <<

				"	EndPrimitive();" << Line::Blank <<

				/* Normal (Z-Axis -> B) */
				"	" << ShaderVariable::PrimaryVertexColor << " = Black;" << Line::End <<
				"	gl_Position = " << modelViewProjectionMatrix << " * gl_in[index].gl_Position;" << Line::End <<
				"	EmitVertex();" << Line::Blank <<

				"	" << ShaderVariable::PrimaryVertexColor << " = Blue;" << Line::End <<
				"	gl_Position = " << modelViewProjectionMatrix << " * (gl_in[index].gl_Position + " << ShaderVariable::NormalWorldSpace << "[index]) * VectorLength;" << Line::End <<
				"	EmitVertex();" << Line::Blank <<

				"	EndPrimitive();" << Line::End <<
				"}";
		}

		return geometryShader->generateSourceCode(*this);
	}

	bool
	TBNSpaceRendering::generateFragmentShader (Program & program) noexcept
	{
		/* Create the fragment shader. */
		auto * fragmentShader = program.initFragmentShader(this->name( ) + "FragmentShader");
		fragmentShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		/* Automatic input declarations from vertex shader. */
		if ( !fragmentShader->connectFromPreviousShader(*program.geometryShader()) )
		{
			return false;
		}

		/* Output declarations. */
		fragmentShader->declareDefaultOutputFragment();

		/* Instructions. */
		Code{*fragmentShader, Location::Output} << ShaderVariable::OutputFragment << " = " << ShaderVariable::PrimaryVertexColor << ';';

		return fragmentShader->generateSourceCode(*this);
	}

	bool
	TBNSpaceRendering::onGraphicsPipelineConfiguration (const Program & /*program*/, GraphicsPipeline & graphicsPipeline) noexcept
	{
		const auto renderableInstance = this->getRenderableInstance();

		if ( renderableInstance == nullptr )
		{
			Tracer::error(ClassId, "The renderable instance pointer is null !");

			return false;
		}

		if ( !graphicsPipeline.configureRasterizationState(RenderPassType::SimplePass) )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline rasterization state !");

			return false;
		}

		if ( !graphicsPipeline.configureDepthStencilState(RenderPassType::SimplePass, *renderableInstance) )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline depth/stencil state !");

			return false;
		}

		return true;
	}
}
