/*
 * src/Graphics/RenderableInstance/Abstract.cpp
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

#include "Abstract.hpp"

/* Local inclusions. */
#include "Graphics/RenderTarget/Abstract.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesInterface.hpp"
#include "PrimaryServices.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/Generator/ShadowCasting.hpp"
#include "Saphir/Generator/TBNSpaceRendering.hpp"
#include "Saphir/Program.hpp"
#include "Scenes/Component/AbstractLightEmitter.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Vulkan;
	using namespace Saphir;
	using namespace Saphir::Keys;

	constexpr auto TracerTag{"RenderableInstance"};

	Renderable::ProgramCacheKey
	Abstract::buildProgramCacheKey (Renderable::ProgramType programType, RenderPassType renderPassType, uint32_t layerIndex) const noexcept
	{
		return Renderable::ProgramCacheKey{
			.programType = programType,
			.renderPassType = renderPassType,
			.layerIndex = layerIndex,
			.isInstancing = this->useModelVertexBufferObject(),
			.isLightingEnabled = this->isLightingEnabled(),
			.isDepthTestDisabled = this->isDepthTestDisabled(),
			.isDepthWriteDisabled = this->isDepthWriteDisabled()
		};
	}

	bool
	Abstract::isReadyToCastShadows (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
	{
		if ( m_renderable == nullptr || !m_renderable->isReadyForInstantiation() )
		{
			return false;
		}

		/* Check if all shadow casting programs exist for all layers. */
		const auto layerCount = m_renderable->layerCount();

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex )
		{
			const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::ShadowCasting, RenderPassType::SimplePass, layerIndex);

			if ( m_renderable->findCachedProgram(renderTarget, cacheKey) == nullptr )
			{
				return false;
			}
		}

		return true;
	}

	bool
	Abstract::isReadyToRender (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
	{
		if ( m_renderable == nullptr || !m_renderable->isReadyForInstantiation() )
		{
			return false;
		}

		/* Check if at least one rendering program exists (we can't know all pass types here). */
		return m_renderable->hasAnyCachedPrograms(renderTarget);
	}

	bool
	Abstract::getReadyForShadowCasting (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, Renderer & renderer) noexcept
	{
		if ( m_renderable == nullptr )
		{
			return false;
		}

		/* NOTE: Check whether the renderable interface is ready for instantiation.
		 * If not, this is no big deal; a loading event exists to relaunch the whole process. */
		if ( !m_renderable->isReadyForInstantiation() )
		{
			return true;
		}

		const auto layerCount = m_renderable->layerCount();

		if constexpr ( IsDebug )
		{
			if ( layerCount == 0 )
			{
				std::stringstream errorMessage;
				errorMessage <<
					"The renderable interface has no layer ! It must have at least one. "
					"Unable to setup the renderable instance '" << m_renderable->name() << "' for shadow casting.";

				return false;
			}
		}

		for ( uint32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex )
		{
			const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::ShadowCasting, RenderPassType::SimplePass, layerIndex);

			/* Try to find a cached program from the Renderable. */
			if ( m_renderable->findCachedProgram(renderTarget, cacheKey) != nullptr )
			{
				continue;
			}

			/* Generate a new program. */
			Generator::ShadowCasting generator{renderTarget, this->shared_from_this(), layerIndex};

			if ( !generator.generateShaderProgram(renderer) )
			{
				return false;
			}

			/* Cache the program on the Renderable for future instances. */
			m_renderable->cacheProgram(renderTarget, cacheKey, generator.shaderProgram());
		}

		return true;
	}

	bool
	Abstract::getReadyForRender (const Scenes::Scene & scene, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const StaticVector< RenderPassType, MaxPassCount > & renderPassTypes, Renderer & renderer) noexcept
	{
		if ( m_renderable == nullptr )
		{
			this->setBroken("The renderable instance has no renderable associated !");

			return false;
		}

		/* NOTE: Check whether the renderable interface is ready for instantiation.
		 * If not, this is no big deal; a loading event exists to relaunch the whole process. */
		if ( !m_renderable->isReadyForInstantiation() )
		{
			return true;
		}

		const auto layerCount = m_renderable->layerCount();

		/* NOTE: These tests only exist in debug mode because they are already performed beyond
		 * isReadyForInstantiation(). */
		if constexpr ( IsDebug )
		{
			if ( layerCount == 0 )
			{
				std::stringstream errorMessage;

				errorMessage <<
					"The renderable interface has no layer ! It must have at least one. "
					"Unable to setup the renderable instance '" << m_renderable->name() << "' for rendering.";

				this->setBroken(errorMessage.str());

				return false;
			}

			/* NOTE: The geometry interface is the same for every layer of the renderable interface. */
			if ( const auto * geometry = m_renderable->geometry(); geometry == nullptr )
			{
				std::stringstream errorMessage;

				errorMessage <<
					"The renderable interface has no geometry interface ! "
					"Unable to setup the renderable instance '" << m_renderable->name() << "' for rendering.";

				this->setBroken(errorMessage.str());

				return false;
			}
		}

		for ( const auto renderPassType : renderPassTypes )
		{
			for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
			{
				const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::Rendering, renderPassType, layerIndex);

				/* Try to find a cached program from the Renderable. */
				if ( m_renderable->findCachedProgram(renderTarget, cacheKey) != nullptr )
				{
					continue;
				}

				/* Generate a new program. */
				std::stringstream shaderProgramName;
				shaderProgramName << "RenderableInstance" << to_string(renderPassType);

				Generator::SceneRendering generator{shaderProgramName.str(), renderTarget, this->shared_from_this(), layerIndex, scene, renderPassType, renderer.primaryServices().settings()};

				if ( !generator.generateShaderProgram(renderer) )
				{
					std::stringstream errorMessage;
					errorMessage <<
						"Unable to generate the shader program for the renderable instance '" << m_renderable->name() << "'!"
						"(RenderPass:'" << to_string(renderPassType) << "', layer:" << layerIndex << ")";

					this->setBroken(errorMessage.str());

					return false;
				}

				/* Cache the program on the Renderable for future instances. */
				m_renderable->cacheProgram(renderTarget, cacheKey, generator.shaderProgram());
			}
		}

		if ( this->isDisplayTBNSpaceEnabled() )
		{
			for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
			{
				const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::TBNSpace, RenderPassType::SimplePass, layerIndex);

				/* Try to find a cached program from the Renderable. */
				if ( m_renderable->findCachedProgram(renderTarget, cacheKey) != nullptr )
				{
					continue;
				}

				/* Generate a new program. */
				Generator::TBNSpaceRendering generator{renderTarget, this->shared_from_this(), layerIndex};

				if ( !generator.generateShaderProgram(renderer) )
				{
					Tracer::error(TracerTag, "Unable to generate the TBN space program !");

					continue;
				}

				/* Cache the program on the Renderable for future instances. */
				m_renderable->cacheProgram(renderTarget, cacheKey, generator.shaderProgram());
			}
		}

		return true;
	}

	void
	Abstract::setBroken (const std::string & errorMessage, const std::source_location & location) noexcept
	{
		this->enableFlag(BrokenState);

		Tracer::error(TracerTag, errorMessage, location);
	}

	void
	Abstract::castShadows (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const CartesianFrame< float > * worldCoordinates, const CommandBuffer & commandBuffer) const noexcept
	{
		const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::ShadowCasting, RenderPassType::SimplePass, layerIndex);
		const auto program = m_renderable->findCachedProgram(renderTarget, cacheKey);

		if ( program == nullptr )
		{
			TraceError{TracerTag} << "There is no suitable shadow program for the renderable instance (Renderable:" << m_renderable->name() << ") !";

			return;
		}

		const auto pipelineLayout = program->pipelineLayout();

		commandBuffer.bind(*program->graphicsPipeline());

		/* NOTE: Set the dynamic viewport and scissor. */
		renderTarget->setViewport(commandBuffer);

		/* NOTE: Bind the view UBO if renderable instance uses GPU instancing. */
		if ( this->useModelVertexBufferObject() )
		{
			commandBuffer.bind(*renderTarget->viewMatrices().descriptorSet(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
		}

		this->bindInstanceModelLayer(commandBuffer, layerIndex);

		/* Build render pass context (created once per pass, reused for all objects). */
		const RenderPassContext passContext{
			.commandBuffer = &commandBuffer,
			.viewMatrices = &renderTarget->viewMatrices(),
			.readStateIndex = readStateIndex,
			.isCubemap = renderTarget->isCubemap()
		};

		/* Build push constant context (pre-computed values for this program). */
		const PushConstantContext pushContext{
			.pipelineLayout = pipelineLayout.get(),
			.stageFlags = static_cast< VkShaderStageFlags >(program->hasGeometryShader() ? VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT : VK_SHADER_STAGE_VERTEX_BIT),
			.useAdvancedMatrices = program->wasAdvancedMatricesEnabled(),
			.useBillboarding = program->wasBillBoardingEnabled()
		};

		this->pushMatricesForShadowCasting(passContext, pushContext, worldCoordinates);

		commandBuffer.draw(*m_renderable->geometry(), layerIndex, this->instanceCount());
	}

	void
	Abstract::render (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Scenes::Component::AbstractLightEmitter * lightEmitter, RenderPassType renderPassType, uint32_t layerIndex, const CartesianFrame< float > * worldCoordinates, const CommandBuffer & commandBuffer) const noexcept
	{
		const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::Rendering, renderPassType, layerIndex);
		const auto program = m_renderable->findCachedProgram(renderTarget, cacheKey);

		if ( program == nullptr )
		{
			TraceError{TracerTag} << "There is no suitable render program for the renderable instance (Renderable:" << m_renderable->name() << ") !";

			return;
		}

		const auto * geometry = m_renderable->geometry();
		const auto pipelineLayout = program->pipelineLayout();

		/* Bind the graphics pipeline. */
		commandBuffer.bind(*program->graphicsPipeline());

		/* NOTE: Set the dynamic viewport and scissor. */
		renderTarget->setViewport(commandBuffer);

		/* Bind a renderable instance VBO / IBO. */
		this->bindInstanceModelLayer(commandBuffer, layerIndex);

		/* Build render pass context (created once per pass, reused for all objects). */
		const RenderPassContext passContext{
			.commandBuffer = &commandBuffer,
			.viewMatrices = &renderTarget->viewMatrices(),
			.readStateIndex = readStateIndex,
			.isCubemap = renderTarget->isCubemap()
		};

		/* Build push constant context (pre-computed values for this program). */
		const PushConstantContext pushContext{
			.pipelineLayout = pipelineLayout.get(),
			.stageFlags = static_cast< VkShaderStageFlags >(program->hasGeometryShader() ? VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT : VK_SHADER_STAGE_VERTEX_BIT),
			.useAdvancedMatrices = program->wasAdvancedMatricesEnabled(),
			.useBillboarding = program->wasBillBoardingEnabled()
		};

		/* Configure the push constants. */
		this->pushMatricesForRendering(passContext, pushContext, worldCoordinates);

		uint32_t setOffset = 0;

		/* Bind view UBO. */
		commandBuffer.bind(*renderTarget->viewMatrices().descriptorSet(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setOffset++);

		/* Bind light UBO. */
		if ( lightEmitter != nullptr && lightEmitter->isCreated() )
		{
			commandBuffer.bind(*lightEmitter->descriptorSet(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setOffset++, lightEmitter->UBOOffset());
		}

		/* Bind material UBO and samplers. */
		const auto * material = m_renderable->material(layerIndex);

		commandBuffer.bind(*material->descriptorSet(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, setOffset/*++*/);

		/* Check for adaptive LOD rendering. */
		if ( geometry->isAdaptiveLOD() )
		{
			const auto & viewPosition = renderTarget->viewMatrices().position();

			/* Prepare LODs and stitching for this frame. */
			geometry->prepareAdaptiveRendering(viewPosition);

			/* Draw all sectors at their computed LOD level. */
			const auto drawCallCount = geometry->getAdaptiveDrawCallCount(viewPosition);

			for ( uint32_t drawCallIndex = 0; drawCallIndex < drawCallCount; ++drawCallIndex )
			{
				const auto range = geometry->getAdaptiveDrawCallRange(drawCallIndex, viewPosition);

				commandBuffer.drawIndexed(range[0], range[1], this->instanceCount());
			}

			/* Draw stitching geometry between LOD zones. */
			const auto stitchingCount = geometry->getStitchingDrawCallCount();

			for ( uint32_t stitchIndex = 0; stitchIndex < stitchingCount; ++stitchIndex )
			{
				const auto range = geometry->getStitchingDrawCallRange(stitchIndex);

				commandBuffer.drawIndexed(range[0], range[1], this->instanceCount());
			}
		}
		else if ( material->isAnimated() )
		{
			commandBuffer.draw(*geometry, m_frameIndex, this->instanceCount());
		}
		else
		{
			commandBuffer.draw(*geometry, layerIndex, this->instanceCount());
		}
	}

	void
	Abstract::renderTBNSpace (uint32_t readStateIndex, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const CartesianFrame< float > * worldCoordinates, const CommandBuffer & commandBuffer) const noexcept
	{
		const auto cacheKey = this->buildProgramCacheKey(Renderable::ProgramType::TBNSpace, RenderPassType::SimplePass, layerIndex);
		const auto program = m_renderable->findCachedProgram(renderTarget, cacheKey);

		if ( program == nullptr )
		{
			TraceError{TracerTag} << "There is no suitable TBN space program for the renderable instance (Renderable:" << m_renderable->name() << ") !";

			return;
		}

		const auto pipelineLayout = program->pipelineLayout();

		commandBuffer.bind(*program->graphicsPipeline());

		/* NOTE: Set the dynamic viewport and scissor. */
		renderTarget->setViewport(commandBuffer);

		/* NOTE: Bind the view UBO if renderable instance uses GPU instancing. */
		if ( this->useModelVertexBufferObject() )
		{
			commandBuffer.bind(*renderTarget->viewMatrices().descriptorSet(), *pipelineLayout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);
		}

		this->bindInstanceModelLayer(commandBuffer, layerIndex);

		/* Build render pass context (created once per pass, reused for all objects). */
		const RenderPassContext passContext{
			.commandBuffer = &commandBuffer,
			.viewMatrices = &renderTarget->viewMatrices(),
			.readStateIndex = readStateIndex,
			.isCubemap = renderTarget->isCubemap()
		};

		/* Build push constant context (pre-computed values for this program). */
		const PushConstantContext pushContext{
			.pipelineLayout = pipelineLayout.get(),
			.stageFlags = static_cast< VkShaderStageFlags >(program->hasGeometryShader() ? VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT : VK_SHADER_STAGE_VERTEX_BIT),
			.useAdvancedMatrices = program->wasAdvancedMatricesEnabled(),
			.useBillboarding = program->wasBillBoardingEnabled()
		};

		this->pushMatricesForRendering(passContext, pushContext, worldCoordinates);

		commandBuffer.draw(*m_renderable->geometry(), layerIndex, this->instanceCount());
	}
}
