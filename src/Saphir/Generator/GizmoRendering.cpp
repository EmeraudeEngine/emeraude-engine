/*
 * src/Saphir/Generator/GizmoRendering.cpp
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

#include "GizmoRendering.hpp"

/* Local inclusions. */
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Libs/Hash/FNV1a.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Saphir/Code.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace Libs;
	using namespace Graphics;
	using namespace Vulkan;
	using namespace Saphir::Keys;

	void
	GizmoRendering::prepareUniformSets (SetIndexes & /*setIndexes*/) noexcept
	{
		/* NOTE: The gizmo has no descriptor sets (no textures, no UBOs).
		 * Everything is passed via push constants. */
	}

	bool
	GizmoRendering::onGenerateShadersCode (Program & program) noexcept
	{
		if ( !this->generateVertexShader(program) )
		{
			return false;
		}

		if ( this->debuggingEnabled() )
		{
			program.vertexShader()->traceSuccessfulGeneration();
		}

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
	GizmoRendering::onCreateDataLayouts (Renderer & /*renderer*/, const SetIndexes & /*setIndexes*/, StaticVector< std::shared_ptr< DescriptorSetLayout >, 5 > & /*descriptorSetLayouts*/, StaticVector< VkPushConstantRange, 4 > & pushConstantRanges) noexcept
	{
		/* NOTE: No descriptor sets needed. Only push constants for the MVP matrix. */
		Abstract::generatePushConstantRanges(this->shaderProgram()->vertexShader()->pushConstantBlockDeclarations(), pushConstantRanges, VK_SHADER_STAGE_VERTEX_BIT);

		return true;
	}

	bool
	GizmoRendering::generateVertexShader (Program & program) noexcept
	{
		auto * vertexShader = program.initVertexShader(this->name() + "VertexShader", false, false, false);
		vertexShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		/* NOTE: Declare the MVP push constant block (mat4 + float frameIndex). */
		if ( !this->declareMatrixPushConstantBlock(*vertexShader) )
		{
			return false;
		}

		/* NOTE: Declare position input attribute. */
		if ( !vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::Position}, true) )
		{
			return false;
		}

		/* NOTE: Declare vertex color input attribute and stage output. */
		if ( !vertexShader->declare(Declaration::StageOutput{this->getNextShaderVariableLocation(), GLSL::FloatVector4, ShaderVariable::PrimaryVertexColor, GLSL::Smooth}) )
		{
			return false;
		}

		if ( !vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::VertexColor}) )
		{
			return false;
		}

		/* NOTE: Transform position by MVP and passthrough vertex color. */
		Code{*vertexShader, Location::Output} <<
			"gl_Position = " << MatrixPC(PushConstant::Component::ModelViewProjectionMatrix) << " * vec4(" << Attribute::Position << ", 1.0);" << '\n' <<
			'\t' << ShaderVariable::PrimaryVertexColor << " = " << Attribute::Color << ';';

		return vertexShader->generateSourceCode(*this);
	}

	bool
	GizmoRendering::generateFragmentShader (Program & program) noexcept
	{
		auto * fragmentShader = program.initFragmentShader(this->name() + "FragmentShader");
		fragmentShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		/* NOTE: Receive vertex color from vertex shader. */
		if ( !fragmentShader->connectFromPreviousShader(*program.vertexShader()) )
		{
			return false;
		}

		/* NOTE: Declare default output fragment (location 0, vec4). */
		if ( !fragmentShader->declareDefaultOutputFragment() )
		{
			return false;
		}

		/* NOTE: Output the vertex color directly without any lighting. */
		Code{*fragmentShader, Location::Output} <<
			ShaderVariable::OutputFragment << " = " << ShaderVariable::PrimaryVertexColor << ';';

		return fragmentShader->generateSourceCode(*this);
	}

	bool
	GizmoRendering::onGraphicsPipelineConfiguration (const Program & /*program*/, GraphicsPipeline & graphicsPipeline) noexcept
	{
		/* NOTE: Dynamic viewport and scissor. */
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

		/* NOTE: Rasterization: fill polygons, no face culling (gizmo viewed from all angles). */
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

		/* NOTE: Depth test and write disabled — gizmo always renders on top. */
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

		/* NOTE: Standard alpha blending for potential transparency. */
		if ( !graphicsPipeline.configureColorBlendStateForAlphaBlending(false) )
		{
			Tracer::error(ClassId, "Unable to configure the graphics pipeline color blend state !");

			return false;
		}

		return true;
	}

	size_t
	GizmoRendering::computeProgramCacheKey () const noexcept
	{
		const auto hashCombine = [] (size_t & seed, size_t value) noexcept {
			seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		};

		size_t hash = Hash::FNV1a(ClassId);

		/* 1. Render pass handle (critical for pipeline compatibility). */
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

		/* 2. Render target type. */
		hashCombine(hash, static_cast< size_t >(this->renderTarget()->isCubemap()));

		return hash;
	}
}
