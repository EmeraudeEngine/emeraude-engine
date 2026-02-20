/*
 * src/Saphir/Generator/PostProcessing.cpp
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

#include "PostProcessing.hpp"

/* Local inclusions. */
#include "Graphics/PostProcessor.hpp"
#include "Saphir/FramebufferEffectInterface.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Libs/Hash/FNV1a.hpp"
#include "Libs/SourceCodeParser.hpp"
#include "Saphir/Code.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/RenderPass.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace Libs;
	using namespace Graphics;
	using namespace Vulkan;
	using namespace Saphir::Keys;

	bool
	PostProcessing::onGenerateShadersCode (Program & program) noexcept
	{
		/* Generate the vertex shader stage. */
		if ( !this->generateVertexShader(program) )
		{
			return false;
		}

		if ( this->debuggingEnabled() )
		{
			program.vertexShader()->traceSuccessfulGeneration();
		}

		/* Generate the fragment shader stage. */
		if ( !this->generateFragmentShader(program) )
		{
			return false;
		}

		if ( this->debuggingEnabled() )
		{
			program.fragmentShader()->traceSuccessfulGeneration();
		}

		return true;
	}

	bool
	PostProcessing::onCreateDataLayouts (Renderer & renderer, const SetIndexes & /*setIndexes*/, StaticVector< std::shared_ptr< DescriptorSetLayout >, 4 > & descriptorSetLayouts, StaticVector< VkPushConstantRange, 4 > & pushConstantRanges) noexcept
	{
		auto descriptorSetLayout = PostProcessor::getDescriptorSetLayout(renderer.layoutManager());

		if ( descriptorSetLayout == nullptr )
		{
			Tracer::error(ClassId, "Unable to get the post-processing descriptor set layout !");

			return false;
		}

		descriptorSetLayouts.emplace_back(descriptorSetLayout);

		/* NOTE: Push constants are visible to both vertex and fragment stages. */
		Abstract::generatePushConstantRanges(this->shaderProgram()->vertexShader()->pushConstantBlockDeclarations(), pushConstantRanges, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

		return true;
	}

	bool
	PostProcessing::generateVertexShader (Program & program) noexcept
	{
		auto * vertexShader = program.initVertexShader(this->name() + "VertexShader", false, false, false);
		vertexShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		/* Push constant block for post-processing parameters.
		 * NOTE: Must match PostProcessor::PushConstants struct layout exactly. */
		{
			Declaration::PushConstantBlock pushConstantBlock{PushConstant::Type::PostProcessing, PushConstant::PostProcessing};
			pushConstantBlock.addMember(Declaration::VariableType::FloatVector2, PushConstant::Component::FrameSize);
			pushConstantBlock.addMember(Declaration::VariableType::Float, PushConstant::Component::Time);
			pushConstantBlock.addMember(Declaration::VariableType::Float, "nearPlane");
			pushConstantBlock.addMember(Declaration::VariableType::Float, "farPlane");
			pushConstantBlock.addMember(Declaration::VariableType::Float, "tanHalfFovY");

			if ( !vertexShader->declare(pushConstantBlock) )
			{
				return false;
			}
		}

		/* Input: vertex position (NDC quad [-1,1]). */
		if ( !vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::Position}, true) )
		{
			return false;
		}

		Code{*vertexShader, Location::Output} << "gl_Position = vec4(" << Attribute::Position << ", 1.0);";

		/* Input: texture coordinates. */
		{
			if ( !vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::Primary2DTextureCoordinates}) )
			{
				return false;
			}

			if ( !vertexShader->declare(Declaration::StageOutput{this->getNextShaderVariableLocation(), GLSL::FloatVector2, ShaderVariable::Primary2DTextureCoordinates, GLSL::Smooth}) )
			{
				return false;
			}

			Code{*vertexShader, Location::Output} << ShaderVariable::Primary2DTextureCoordinates << " = " << Attribute::Primary2DTextureCoordinates << ';';
		}

		return vertexShader->generateSourceCode(*this);
	}

	bool
	PostProcessing::generateFragmentShader (Program & program) noexcept
	{
		auto * fragmentShader = program.initFragmentShader(this->name() + "FragmentShader");
		fragmentShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		/* Automatic input declarations from the vertex shader. */
		if ( !fragmentShader->connectFromPreviousShader(*program.vertexShader()) )
		{
			return false;
		}

		/* Push constant block for post-processing parameters (must match vertex shader layout). */
		{
			Declaration::PushConstantBlock pushConstantBlock{PushConstant::Type::PostProcessing, PushConstant::PostProcessing};
			pushConstantBlock.addMember(Declaration::VariableType::FloatVector2, PushConstant::Component::FrameSize);
			pushConstantBlock.addMember(Declaration::VariableType::Float, PushConstant::Component::Time);
			pushConstantBlock.addMember(Declaration::VariableType::Float, "nearPlane");
			pushConstantBlock.addMember(Declaration::VariableType::Float, "farPlane");
			pushConstantBlock.addMember(Declaration::VariableType::Float, "tanHalfFovY");

			if ( !fragmentShader->declare(pushConstantBlock) )
			{
				return false;
			}
		}

		/* GrabPass color sampler at set=PerModel, binding=0. */
		if ( !fragmentShader->declare(Declaration::Sampler{program.setIndex(SetType::PerModel), 0, GLSL::Sampler2D, Uniform::PrimarySampler}) )
		{
			return false;
		}

		if ( !fragmentShader->declareDefaultOutputFragment() )
		{
			return false;
		}

		if ( m_effectsList.empty() )
		{
			/* Passthrough: sample GrabPass and output unchanged. */
			Code{*fragmentShader, Location::Output} <<
				ShaderVariable::OutputFragment << " = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ");";
		}
		else
		{
			/* Declare the working fragment variable. */
			Code{*fragmentShader, Location::Top} << "vec4 " << PostProcessor::Fragment << ";";

			/* Fetch the fragment from the GrabPass texture (unless an effect overrides fetching). */
			{
				bool fetchOverridden = false;

				for ( const auto & effect : m_effectsList )
				{
					if ( effect->overrideFragmentFetching() )
					{
						fetchOverridden = true;
						break;
					}
				}

				if ( !fetchOverridden )
				{
					Code{*fragmentShader} <<
						PostProcessor::Fragment << " = texture(" << Uniform::PrimarySampler << ", " << ShaderVariable::Primary2DTextureCoordinates << ");";
				}
			}

			/* Apply each effect in order. */
			for ( const auto & effect : m_effectsList )
			{
				if ( !effect->generateFragmentShaderCode(*this, *fragmentShader) )
				{
					Tracer::error(ClassId, "Unable to generate fragment shader code for a post-processing effect !");

					return false;
				}
			}

			/* Output the processed fragment. */
			Code{*fragmentShader, Location::Output} <<
				ShaderVariable::OutputFragment << " = " << PostProcessor::Fragment << ";";
		}

		return fragmentShader->generateSourceCode(*this);
	}

	bool
	PostProcessing::onGraphicsPipelineConfiguration (const Program & /*program*/, GraphicsPipeline & graphicsPipeline) noexcept
	{
		/* Dynamic viewport and scissor. */
		{
			const StaticVector< VkDynamicState, 16 > dynamicStates{
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			if ( !graphicsPipeline.configureDynamicStates(dynamicStates) )
			{
				Tracer::error(ClassId, "Unable to configure the graphics pipeline dynamic states !");

				return false;
			}
		}

		/* Rasterization: fill, no cull, CCW. */
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

		/* Depth: test OFF, write OFF. */
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

		/* Color blend: opaque (no blending, full framebuffer replacement). */
		{
			const StaticVector< VkPipelineColorBlendAttachmentState, 8 > attachments{
				VkPipelineColorBlendAttachmentState{
					.blendEnable = VK_FALSE,
					.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
					.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
					.colorBlendOp = VK_BLEND_OP_ADD,
					.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
					.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
					.alphaBlendOp = VK_BLEND_OP_ADD,
					.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
				}
			};

			VkPipelineColorBlendStateCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.logicOpEnable = VK_FALSE;
			createInfo.logicOp = VK_LOGIC_OP_COPY;
			createInfo.attachmentCount = static_cast< uint32_t >(attachments.size());
			createInfo.pAttachments = attachments.data();
			createInfo.blendConstants[0] = 0.0F;
			createInfo.blendConstants[1] = 0.0F;
			createInfo.blendConstants[2] = 0.0F;
			createInfo.blendConstants[3] = 0.0F;

			if ( !graphicsPipeline.configureColorBlendState(attachments, createInfo) )
			{
				Tracer::error(ClassId, "Unable to configure the graphics pipeline color blend state !");

				return false;
			}
		}

		return true;
	}

	size_t
	PostProcessing::computeProgramCacheKey () const noexcept
	{
		const auto hashCombine = [] (size_t & seed, size_t value) noexcept {
			seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		};

		size_t hash = Hash::FNV1a(ClassId);

		/* Render pass handle (critical for pipeline compatibility). */
		{
			const auto * fb = this->pipelineFramebuffer();

			if ( fb == nullptr )
			{
				fb = this->renderTarget()->framebuffer();
			}

			if ( fb != nullptr )
			{
				hashCombine(hash, reinterpret_cast< size_t >(fb->renderPass()->handle()));
			}
		}

		/* Include effects count so the cache key changes when effects are added/removed. */
		hashCombine(hash, m_effectsList.size());

		return hash;
	}
}
