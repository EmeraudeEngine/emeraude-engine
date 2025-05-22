/*
 * src/Saphir/Generator/Abstract.cpp
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

#include "Abstract.hpp"

/* Local inclusions. */
#include "Abstract.hpp"
#include "Graphics/RenderableInstance/Abstract.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Types.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "Saphir/Code.hpp"
#include "Tracer.hpp"

namespace EmEn::Saphir::Generator
{
	using namespace EmEn::Libs;
	using namespace Graphics;
	using namespace Vulkan;
	using namespace Keys;

	constexpr auto TracerTag{"ShaderGenerator"};

	Abstract::Abstract (const std::string & shaderProgramName, const std::shared_ptr< const RenderTarget::Abstract > & renderTarget) noexcept
		: NameableTrait(shaderProgramName),
		m_renderTarget(renderTarget)
	{

	}
	
	Abstract::Abstract (const std::string & shaderProgramName, const std::shared_ptr< const RenderTarget::Abstract > & renderTarget, const std::shared_ptr< const RenderableInstance::Abstract > & renderableInstance, uint32_t layerIndex) noexcept
		: NameableTrait(shaderProgramName),
		m_renderTarget(renderTarget),
		m_renderableInstance(renderableInstance),
		m_layerIndex(layerIndex)
	{
		if ( renderableInstance->instancingEnabled() )
		{
			this->enableFlag(IsInstancingEnabled);
		}

		if ( renderableInstance->isFacingCamera() )
		{
			this->enableFlag(IsRenderableFacingCamera);
		}

		if ( renderableInstance->isLightingEnabled() )
		{
			this->enableFlag(IsLightingEnabled);
		}
	}

	Abstract::Abstract (const std::string & shaderProgramName, const std::shared_ptr< const RenderTarget::Abstract > & renderTarget, Topology topology, uint32_t geometryFlags) noexcept
		: NameableTrait(shaderProgramName),
		m_renderTarget(renderTarget),
		m_topology(topology),
		m_geometryFlags(geometryFlags)
	{

	}

	bool
	Abstract::generateFallBackVertexShader (Program & program) noexcept
	{
		if ( program.setIndexes().isSetEnabled(SetType::PerView) )
		{
			auto * vertexShader = program.initVertexShader(
				this->name( ) + "FallbackMVPVertexShader",
				program.wasInstancingEnabled(),
				program.wasAdvancedMatricesEnabled(),
				program.wasBillBoardingEnabled()
			);
			vertexShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

			if ( !vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::Position}, true) )
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

			if ( !vertexShader->generateSourceCode(*this) )
			{
				return false;
			}
		}
		else
		{
			auto * vertexShader = program.initVertexShader(
				this->name( ) + "FallbackVertexShader",
				program.wasInstancingEnabled(),
				program.wasAdvancedMatricesEnabled(),
				program.wasBillBoardingEnabled()
			);
			vertexShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

			if ( !vertexShader->declare(Declaration::InputAttribute{VertexAttributeType::Position}, true) )
			{
				return false;
			}

			/* TODO: Try to use at least the model view if exists */
			Code{*vertexShader, Location::Output} << "gl_Position = vec4(" << Attribute::Position << ", 1.0);";

			if ( !vertexShader->generateSourceCode(*this) )
			{
				return false;
			}
		}

		return true;
	}

	bool
	Abstract::generateFallBackFragmentShader (Program & program) noexcept
	{
		auto * fragmentShader = program.initFragmentShader(this->name( ) + "FallbackFragmentShader");
		fragmentShader->setExtensionBehavior("GL_ARB_separate_shader_objects", "enable");

		/* Automatic input declarations from the vertex shader. */
		if ( !fragmentShader->connectFromPreviousShader(*program.vertexShader()) )
		{
			return false;
		}

		if ( !fragmentShader->declareDefaultOutputFragment() )
		{
			return false;
		}

		Code{*fragmentShader, Location::Output} << ShaderVariable::OutputFragment << " = vec4(1.0, 0.0, 1.0, 1.0);";

		return fragmentShader->generateSourceCode(*this);
	}

	bool
	Abstract::createDataLayout (Renderer & renderer) noexcept
	{
		std::vector< std::shared_ptr< DescriptorSetLayout > > descriptorSetLayouts{};
		std::vector< VkPushConstantRange > pushConstantRanges{};

		const auto & setIndexes = m_shaderProgram->setIndexes();

		/* Prepare the descriptor set layout for the view. */
		if ( setIndexes.isSetEnabled(SetType::PerView) )
		{
			/* FIXME: This could be general for render target type. */
			const auto descriptorSetLayout = m_renderTarget->viewMatrices().getDescriptorSetLayout();

			if ( descriptorSetLayout == nullptr )
			{
				Tracer::error(TracerTag, "Unable to get the view descriptor set layout !");

				return false;
			}

			descriptorSetLayouts.emplace_back(descriptorSetLayout);
		}

		/* Let child class generate all other descriptor set layouts. */
		if ( !this->onCreateDataLayouts(setIndexes, descriptorSetLayouts, pushConstantRanges) )
		{
			Tracer::error(TracerTag, "Unable to create the data layouts at child shader generator level !");

			return false;
		}

		/* Get the proper pipeline layout according to all descriptor set layouts to build the graphics pipeline.
		 * NOTE: This step will check if the pipeline layout already exists. */
		const auto pipelineLayout = renderer.layoutManager().getPipelineLayout(descriptorSetLayouts, pushConstantRanges);

		if ( pipelineLayout == nullptr )
		{
			Tracer::error(TracerTag, "Unable to get the pipeline layout !");

			return false;
		}

		m_shaderProgram->setPipelineLayout(pipelineLayout);

		return true;
	}

	bool
	Abstract::createGraphicsPipeline (Renderer & renderer) noexcept
	{
		/* Create a graphics pipeline base and configure it before letting the renderer handle the final invocation on GPU.  */
		auto graphicsPipeline = std::make_shared< GraphicsPipeline >(renderer.device());
		graphicsPipeline->setIdentifier(TracerTag, this->name(), "Pipeline");

		{
			auto shaderModules = renderer.shaderManager().getShaderModules(renderer.device(), m_shaderProgram);

			if ( shaderModules.empty() || !graphicsPipeline->configureShaderStages(shaderModules) )
			{
				Tracer::error(TracerTag, "Unable to configure the graphics pipeline with shader stages !");

				return false;
			}
		}

		const auto & vertexBufferFormat = *m_shaderProgram->vertexBufferFormat();

		if ( !graphicsPipeline->configureVertexInputState(vertexBufferFormat) )
		{
			Tracer::error(TracerTag, "Unable to configure the graphics pipeline vertex input state !");

			return false;
		}

		if ( !graphicsPipeline->configureInputAssemblyState(vertexBufferFormat) )
		{
			Tracer::error(TracerTag, "Unable to configure the graphics pipeline input assembly state !");

			return false;
		}

		/* NOTE: If tesselation wasn't enabled, there is no point to configure it. */
		if ( m_shaderProgram->useTesselation() )
		{
			if ( !graphicsPipeline->configureTessellationState(0) )
			{
				Tracer::error(TracerTag, "Unable to configure the graphics pipeline tesselation state !");

				return false;
			}
		}

		/* FIXME: Only to get the width/height. This can become a dynamic state ! */
		const auto & extent = m_renderTarget->extent();

		if ( !graphicsPipeline->configureViewportState(extent.width, extent.height) )
		{
			Tracer::error(TracerTag, "Unable to configure the graphics pipeline viewport state !");

			return false;
		}

		/* FIXME: Only to get the framebuffer sampler. This can become a dynamic state or a global settings. ! */
		if ( !graphicsPipeline->configureMultisampleState(*m_renderTarget) )
		{
			Tracer::error(TracerTag, "Unable to configure the graphics pipeline multisample state !");

			return false;
		}

		if ( !this->onGraphicsPipelineConfiguration(*m_shaderProgram, *graphicsPipeline) )
		{
			Tracer::error(TracerTag, "Unable to configure the graphics pipeline at child shader generator level !");

			return false;
		}

		/* FIXME: Only to get the render pass handle. This can become general for the render target type. ! */
		if ( !renderer.finalizeGraphicsPipeline(*m_renderTarget, *m_shaderProgram, graphicsPipeline) )
		{
			TraceError{TracerTag} << "Unable to finalize the graphics pipeline of the program '" << m_shaderProgram->name() << "' !";

			return false;
		}

		m_shaderProgram->setGraphicsPipeline(graphicsPipeline);

		return true;
	}

	bool
	Abstract::generateShaderProgram (Renderer & renderer, const std::string & GLSLVersion, const std::string & GLSLProfile) noexcept
	{
		this->enableDebugging(renderer.shaderManager().showSourceCode());

		/* NOTE: Declare a new program. */
		m_shaderProgram = std::make_shared< Program >(this->name(), GLSLVersion, GLSLProfile);

		/* Prepare the number of binding sets and identifiers. */
		this->prepareUniformSets(m_shaderProgram->setIndexes());

		/* NOTE: First, we generate all shader source code. */
		if ( !this->onGenerateShadersCode(*m_shaderProgram) )
		{
			Tracer::error(TracerTag, "Unable to generate shaders source code at child shader generator level !");

			return false;
		}

		if ( this->isRenderableInstanceAvailable() )
		{
			const auto * geometry = this->getGeometryInterface();

			if ( !m_shaderProgram->createVertexBufferFormat(renderer.vertexBufferFormatManager(), geometry) )
			{
				Tracer::error(TracerTag, "Unable to create the vertex buffer format !");

				return false;
			}
		}
		else
		{
			if ( !m_shaderProgram->createVertexBufferFormat(renderer.vertexBufferFormatManager(), m_topology, m_geometryFlags) )
			{
				Tracer::error(TracerTag, "Unable to create the vertex buffer format !");

				return false;
			}
		}

		if ( !m_shaderProgram->isComplete() )
		{
			Tracer::error(TracerTag, "The shader program is not complete !");

			return false;
		}

		/* The second step is to check every resource needed by shaders (UBO, Samples, etc.).
		 * NOTE: VBO is an exception done before. */
		if ( !this->createDataLayout(renderer) )
		{
			return false;
		}

		/* NOTE: The third step is to check if separate shaders already exist to avoid an extra compilation.
		 * Retrieve the graphics pipeline for the combination of the current renderable instance layer and the render pass. */
		if ( !this->createGraphicsPipeline(renderer) )
		{
			return false;
		}

		return true;
	}

	uint32_t
	Abstract::getNextShaderVariableLocation (uint32_t increment) noexcept
	{
		const auto location = m_nextShaderVariableLocation;

		m_nextShaderVariableLocation += increment;

		return location;
	}

	bool
	Abstract::declareMatrixPushConstantBlock (AbstractShader & shader) const noexcept
	{
		/* [VULKAN-PUSH-CONSTANT:1] Declare push constant usage by analyzing needs. */
		Declaration::PushConstantBlock pushConstantBlock{PushConstant::Type::Matrices, PushConstant::Matrices};

		if ( m_shaderProgram->wasInstancingEnabled() )
		{
			if ( m_shaderProgram->wasAdvancedMatricesEnabled() || m_shaderProgram->wasBillBoardingEnabled() )
			{
				/* NOTE: Push the view matrix (V) and the view projection matrix (VP). */
				pushConstantBlock.addMember(Declaration::VariableType::Matrix4, PushConstant::Component::ViewMatrix);
				pushConstantBlock.addMember(Declaration::VariableType::Matrix4, PushConstant::Component::ViewProjectionMatrix);
			}
			else
			{
				/* NOTE: Push the view projection matrix (VP). */
				pushConstantBlock.addMember(Declaration::VariableType::Matrix4, PushConstant::Component::ViewProjectionMatrix);
			}
		}
		else
		{
			if ( m_shaderProgram->wasAdvancedMatricesEnabled() )
			{
				/* NOTE: Push the view matrix (V) and the model matrix (M). */
				pushConstantBlock.addMember(Declaration::VariableType::Matrix4, PushConstant::Component::ViewMatrix);
				pushConstantBlock.addMember(Declaration::VariableType::Matrix4, PushConstant::Component::ModelMatrix);
			}
			else
			{
				/* NOTE: Push the model view projection matrix (MVP). */
				pushConstantBlock.addMember(Declaration::VariableType::Matrix4, PushConstant::Component::ModelViewProjectionMatrix);
			}
		}

		return shader.declare(pushConstantBlock);
	}

	void
	Abstract::generatePushConstantRanges (const std::vector< Declaration::PushConstantBlock > & pushConstantBlocks, std::vector< VkPushConstantRange > & pushConstantRanges, VkShaderStageFlags stageFlags) noexcept
	{
		/* [VULKAN-PUSH-CONSTANT:2] Prepare the push constants declaration for the pipeline. */
		uint32_t offset = 0;

		for ( const auto & pushConstantBlock : pushConstantBlocks )
		{
			const auto bytes = pushConstantBlock.bytes();

			pushConstantRanges.push_back({
				.stageFlags = stageFlags,
				.offset = offset,
				.size = bytes
			});

			offset += bytes;
		}

		/*for ( const auto & pushConstantBlock : pushConstantBlocks )
		{
			uint32_t offset = 0;

			for ( const auto & range : pushConstantBlock.members() )
			{
				const uint32_t bytes = range.second.bytes();

				pushConstantRanges.push_back({
					.stageFlags = stageFlags,
					.offset = offset,
					.size = bytes
				});

				offset += bytes;
			}
		}*/
	}

	bool
	Abstract::declareViewUniformBlock (AbstractShader & shader, uint32_t binding) const noexcept
	{
		const auto setIndex = m_shaderProgram->setIndex(SetType::PerView);

		if ( m_renderTarget->isCubemap() )
		{
			/* FIXME: Rework this to split common data between each cubemap faces ! */
			Declaration::Structure structure{Struct::CubemapFace, UniformBlock::Component::Instance};
			structure.addMember(Declaration::VariableType::Matrix4, UniformBlock::Component::ProjectionMatrix);
			structure.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::PositionWorldSpace);
			structure.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::Velocity);
			structure.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::ViewProperties);
			structure.addMember(Declaration::VariableType::FloatVector4, "_padding");

			Declaration::UniformBlock uniformBlock{setIndex, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::CubemapView, UniformBlock::View};
			uniformBlock.addArrayMember(structure, 6);

			return shader.declare(uniformBlock);
		}

		Declaration::UniformBlock uniformBlock{setIndex, binding, Declaration::MemoryLayout::Std140, UniformBlock::Type::View, UniformBlock::View};
		uniformBlock.addMember(Declaration::VariableType::Matrix4, UniformBlock::Component::ProjectionMatrix);
		uniformBlock.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::PositionWorldSpace);
		uniformBlock.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::Velocity);
		uniformBlock.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::ViewProperties);
		uniformBlock.addMember(Declaration::VariableType::FloatVector4, UniformBlock::Component::AmbientLightColor);
		uniformBlock.addMember(Declaration::VariableType::Float, UniformBlock::Component::AmbientLightIntensity);

		return shader.declare(uniformBlock);
	}

	bool
	Abstract::declareMaterialUniformBlock (const Material::Interface & material, AbstractShader & shader, uint32_t binding) const noexcept
	{
		const auto setIndex = m_shaderProgram->setIndex(SetType::PerModelLayer);
		const auto uniformBlock = material.getUniformBlock(setIndex, binding);

		return shader.declare(uniformBlock);
	}
}
