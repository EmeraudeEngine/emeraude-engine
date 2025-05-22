/*
 * src/Saphir/Generator/OverlayRendering.cpp
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

#include "OverlayRendering.hpp"

/* Local inclusions. */
#include "Libs/SourceCodeParser.hpp"
#include "Saphir/Code.hpp"
#include "Overlay/Manager.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace EmEn::Libs;
	using namespace Graphics;
	using namespace Vulkan;
	using namespace Keys;

	OverlayRendering::OverlayRendering (const std::shared_ptr< const RenderTarget::Abstract > & renderTarget, const std::shared_ptr< const Geometry::Interface > & geometry, ColorConversion conversion) noexcept
		: Abstract(ClassId, renderTarget, geometry->topology(), geometry->flags()),
		m_colorConversion(conversion)
	{

	}

	bool
	OverlayRendering::onGenerateShadersCode (Program & program) noexcept
	{
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
			for ( const auto & shader: program.getShaderList() )
			{
				TraceDebug{ClassId} <<
					to_string(shader->type()) << " '" << shader->name() << "' generated into "
					"a source code of " << shader->sourceCode().size() << " bytes !";
			}
		}

		return true;
	}

	bool
	OverlayRendering::onCreateDataLayouts (const SetIndexes & /*setIndexes*/, std::vector< std::shared_ptr< DescriptorSetLayout > > & descriptorSetLayouts, std::vector< VkPushConstantRange > & pushConstantRanges) noexcept
	{
		auto descriptorSetLayout = Overlay::Manager::getDescriptorSetLayout(Renderer::instance()->layoutManager());

		if ( descriptorSetLayout == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the overlay descriptor set layout !");

			return false;
		}

		descriptorSetLayouts.emplace_back(descriptorSetLayout);

		Abstract::generatePushConstantRanges(this->shaderProgram()->vertexShader()->pushConstantBlockDeclarations(), pushConstantRanges, VK_SHADER_STAGE_VERTEX_BIT);

		return true;
	}

	bool
	OverlayRendering::generateVertexShader (Program & program) noexcept
	{
		auto * vertexShader = program.initVertexShader(this->name( ) + "VertexShader", false, false, false);
		vertexShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		/* Generation surface position. */
		{
			Declaration::PushConstantBlock pushConstantBlock{PushConstant::Type::Overlay, PushConstant::Overlay};
			pushConstantBlock.addMember(Declaration::VariableType::Matrix4, PushConstant::Component::TransformationMatrix);

			if ( !vertexShader->declare(pushConstantBlock) )
			{
				return false;
			}

			if ( !vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::Position}, true) )
			{
				return false;
			}

			Code{*vertexShader, Location::Output} <<
				"gl_Position = " << OverlayPC(PushConstant::Component::TransformationMatrix) << " * vec4(" << Attribute::Position << ", 1.0);";
		}

		/* Generation surface texture coordinates. */
		{
			if ( !vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::Primary2DTextureCoordinates}) )
			{
				return false;
			}

			if ( !vertexShader->declare(Declaration::StageOutput{this->getNextShaderVariableLocation(), GLSL::FloatVector2, ShaderVariable::Primary2DTextureCoordinates, GLSL::Smooth}) )
			{
				return false;
			}

			Code{*vertexShader, Location::Output} <<
				ShaderVariable::Primary2DTextureCoordinates << " = " << Attribute::Primary2DTextureCoordinates << ';';
		}

		return vertexShader->generateSourceCode(*this);
	}

	bool
	OverlayRendering::generateFragmentShader (Program & program) noexcept
	{
		auto * fragmentShader = program.initFragmentShader(this->name( ) + "FragmentShader");
		fragmentShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		/* Automatic input declarations from the vertex shader. */
		if ( !fragmentShader->connectFromPreviousShader(*program.vertexShader()) )
		{
			return false;
		}

		if ( !fragmentShader->declare(Declaration::Sampler{program.setIndex(SetType::PerModel), 0, GLSL::Sampler2D, Uniform::PrimarySampler}) )
		{
			return false;
		}

		if ( !fragmentShader->declareDefaultOutputFragment() )
		{
			return false;
		}

		switch ( m_colorConversion )
		{
			case ColorConversion::ToSRGB :
				if ( !fragmentShader->declare(FragmentShader::generateToSRGBColorFunction()) )
				{
					return false;
				}

				Code{*fragmentShader, Location::Output} <<
					"const vec4 color = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ").bgra;" "\n" <<
					ShaderVariable::OutputFragment << " = toSRGBColor(color);";
				break;

			case ColorConversion::ToLinear :
				if ( !fragmentShader->declare(FragmentShader::generateToLinearColorFunction()) )
				{
					return false;
				}

				Code{*fragmentShader, Location::Output} <<
					"const vec4 color = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ").bgra;" "\n" <<
					ShaderVariable::OutputFragment << " = toLinearColor(color);";
				break;

			case ColorConversion::None :
			default:
				Code{*fragmentShader, Location::Output} <<
					ShaderVariable::OutputFragment << " = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ").bgra;";
				break;
		}

		return fragmentShader->generateSourceCode(*this);
	}

	bool
	OverlayRendering::onGraphicsPipelineConfiguration (const Program & /*program*/, GraphicsPipeline & graphicsPipeline) noexcept
	{
		{
			VkPipelineRasterizationStateCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.depthClampEnable = VK_FALSE;
			createInfo.rasterizerDiscardEnable = VK_FALSE;
			createInfo.polygonMode = VK_POLYGON_MODE_FILL;
			createInfo.cullMode = VK_CULL_MODE_NONE;
			createInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			createInfo.depthBiasEnable = VK_FALSE;
			createInfo.depthBiasConstantFactor = 0.0F;
			createInfo.depthBiasClamp = 0.0F;
			createInfo.depthBiasSlopeFactor = 0.0F;
			createInfo.lineWidth = 1.0F;

			if ( !graphicsPipeline.configureRasterizationState(createInfo) )
			{
				Tracer::error(ClassId, "Unable to configure the graphics pipeline rasterization state !");

				return false;
			}
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

		if ( !graphicsPipeline.configureColorBlendStateForAlphaBlending() )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline color blend state !");

			return false;
		}

		return true;
	}
}
