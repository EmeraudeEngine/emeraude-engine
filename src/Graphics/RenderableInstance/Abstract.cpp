/*
 * src/Graphics/RenderableInstance/Abstract.cpp
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

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <any>
#include <memory>
#include <mutex>
#include <ranges>
#include <string>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "PrimaryServices.hpp"
#include "Resources/ResourceTrait.hpp"
#include "Saphir/Generator/SceneRendering.hpp"
#include "Saphir/Generator/ShadowCasting.hpp"
#include "Saphir/Generator/TBNSpaceRendering.hpp"
#include "Saphir/Program.hpp"
#include "Scenes/Component/AbstractLightEmitter.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"

#include "RenderTargetProgramsSingleLayer.hpp"
#include "RenderTargetProgramsMultipleLayers.hpp"

namespace EmEn::Graphics::RenderableInstance
{
	using namespace EmEn::Libs;
	using namespace EmEn::Vulkan;
	using namespace Saphir;
	using namespace Keys;

	constexpr auto TracerTag{"RenderableInstance"};

	const size_t Abstract::ClassUID{getClassUID("AbstractRenderableInstance")};

	Abstract::Abstract (const std::shared_ptr< Renderable::Interface > & renderable, uint32_t flagBits) noexcept
		: FlagTrait(flagBits),
		m_renderable(renderable)
	{

	}

	bool
	Abstract::isReadyToCastShadows (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
	{
		const auto renderTargetIt = m_renderTargetPrograms.find(renderTarget);

		if ( renderTargetIt == m_renderTargetPrograms.cend() )
		{
			return false;
		}

		return renderTargetIt->second->isReadyToCastShadows();
	}

	bool
	Abstract::isReadyToRender (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) const noexcept
	{
		const auto renderTargetIt = m_renderTargetPrograms.find(renderTarget);

		if ( renderTargetIt == m_renderTargetPrograms.cend() )
		{
			return false;
		}

		return renderTargetIt->second->isReadyToRender();
	}

	RenderTargetProgramsInterface *
	Abstract::getOrCreateRenderTargetProgramInterface (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerCount)
	{
		if ( layerCount > 1 )
		{
			const auto renderTargetIt = m_renderTargetPrograms.try_emplace(renderTarget, std::make_unique< RenderTargetProgramsMultipleLayers >(layerCount)).first;

			return renderTargetIt->second.get();
		}

		const auto renderTargetIt = m_renderTargetPrograms.try_emplace(renderTarget, std::make_unique< RenderTargetProgramsSingleLayer >()).first;

		return renderTargetIt->second.get();
	}

	bool
	Abstract::getReadyForShadowCasting (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, Renderer & renderer) noexcept
	{
		/* NOTE: Checking the renderable interface.
		 * This is the shared part between all renderable instances. */
		/* TODO: Check for renderable interface already in video memory to reduce renderable instance preparation time. */
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

		auto * renderTargetProgram = this->getOrCreateRenderTargetProgramInterface(renderTarget, layerCount);

		if ( renderTargetProgram == nullptr )
		{
			return false;
		}

		if constexpr ( IsDebug )
		{
			/* NOTE: This test only exists in debug mode because it is already performed beyond isReadyForInstantiation(). */
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
			Generator::ShadowCasting generator{renderTarget, this->shared_from_this(), layerIndex};

			if ( !generator.generateShaderProgram(renderer) )
			{
				return false;
			}

			renderTargetProgram->setShadowCastingProgram(layerIndex, generator.shaderProgram());
		}

		renderTargetProgram->setReadyToCastShadows();

		return true;
	}

	bool
	Abstract::getReadyForRender (const Scenes::Scene & scene, const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const std::vector< RenderPassType > & renderPassTypes, Renderer & renderer) noexcept
	{
		/* NOTE: Checking the renderable interface.
		 * This is the shared part between all renderable instances. */
		/* TODO: Check for renderable interface already in video memory to reduce renderable instance preparation time. */
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

		auto * renderTargetProgram = this->getOrCreateRenderTargetProgramInterface(renderTarget, layerCount);

		if ( renderTargetProgram == nullptr )
		{
			return false;
		}

		if constexpr ( IsDebug )
		{
			/* NOTE: This test only exists in debug mode because it is already performed beyond isReadyForInstantiation(). */
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
			const auto * geometry = m_renderable->geometry();

			/* NOTE: This test only exists in debug mode because it is already performed beyond isReadyForInstantiation(). */
			if ( geometry == nullptr )
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
				std::stringstream shaderProgramName;
				shaderProgramName << "RenderableInstance" << to_string(renderPassType);

				/* The first step is to generate the shader source code from every resource involved. */
				Generator::SceneRendering generator{
					shaderProgramName.str(),
					renderTarget,
					this->shared_from_this(),
					layerIndex,
					scene,
					renderPassType,
					renderer.primaryServices().settings()
				};

				if ( !generator.generateShaderProgram(renderer) )
				{
					std::stringstream errorMessage;
					errorMessage <<
						"Unable to generate the shader program for the renderable instance '" << m_renderable->name() << "'!"
						"(RenderPass:'" << to_string(renderPassType) << "', layer:" << layerIndex << ")";

					this->setBroken(errorMessage.str());

					return false;
				}

				renderTargetProgram->setRenderProgram(renderPassType, layerIndex, generator.shaderProgram());
			}
		}

		renderTargetProgram->setReadyToRender();

		if ( this->isDisplayTBNSpaceEnabled() )
		{
			for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
			{
				Generator::TBNSpaceRendering generator{renderTarget, this->shared_from_this(), layerIndex};

				if ( !generator.generateShaderProgram(renderer) )
				{
					Tracer::error(TracerTag, "Unable to generate the TBN space program !");

					continue;

				}

				renderTargetProgram->setTBNSpaceProgram(layerIndex, generator.shaderProgram());
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

	bool
	Abstract::refreshGraphicsPipelines (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) noexcept
	{
		/*const std::lock_guard< std::mutex > lock{m_GPUMemoryAccess};

		const auto renderTargetIt = m_renderTargetPrograms.find(renderTarget);

		if ( renderTargetIt == m_renderTargetPrograms.end() )
		{
			return false;
		}

		uint32_t error = 0;

		for ( const auto & programs : std::ranges::views::values(renderTargetIt->second.renderPasses) )
		{
			for ( const auto & program : programs )
			{
				if ( !program->graphicsPipeline()->recreateOnHardware(*renderTarget, *this) )
				{
					error++;
				}
			}
		}

		return error == 0;*/

		return false;
	}

	void
	Abstract::destroyGraphicsPipelines (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_GPUMemoryAccess};

		m_renderTargetPrograms.erase(renderTarget);
	}

	void
	Abstract::castShadows (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const CommandBuffer & commandBuffer) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_GPUMemoryAccess};

		const auto renderTargetProgramsIt = m_renderTargetPrograms.find(renderTarget);

		if ( renderTargetProgramsIt == m_renderTargetPrograms.end() )
		{
			TraceError{TracerTag} << "There is no render target programs named '" << renderTarget->id() << "' in the renderable instance (Renderable:" << m_renderable->name() << ") !";

			return;
		}

		const auto program = renderTargetProgramsIt->second->shadowCastingProgram(layerIndex);

		if ( program == nullptr )
		{
			TraceError{TracerTag} << "There is no suitable shadow program for the renderable instance (Renderable:" << m_renderable->name() << ") !";

			return;
		}

		const auto pipelineLayout = program->pipelineLayout();

		commandBuffer.bind(*program->graphicsPipeline());

		/* Bind view UBO. */
		if ( this->isFlagEnabled(EnableInstancing) )
		{
			commandBuffer.bind(
				*renderTarget->viewMatrices().descriptorSet(),
				*pipelineLayout,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				0
			);
		}

		this->bindInstanceModelLayer(commandBuffer, layerIndex);

		this->pushMatrices(commandBuffer, *pipelineLayout, renderTarget->viewMatrices(), *program);

		commandBuffer.draw(*m_renderable->geometry(), layerIndex, this->instanceCount());
	}

	void
	Abstract::render (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Scenes::Component::AbstractLightEmitter * lightEmitter, RenderPassType renderPassType, uint32_t layerIndex, const CommandBuffer & commandBuffer) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_GPUMemoryAccess};

		const auto renderTargetProgramsIt = m_renderTargetPrograms.find(renderTarget);

		if ( renderTargetProgramsIt == m_renderTargetPrograms.end() )
		{
			TraceError{TracerTag} << "There is no render target programs named '" << renderTarget->id() << "' in the renderable instance (Renderable:" << m_renderable->name() << ") !";

			return;
		}

		const auto program = renderTargetProgramsIt->second->renderProgram(renderPassType, layerIndex);

		if ( program == nullptr )
		{
			TraceError{TracerTag} << "There is no suitable render program for the renderable instance (Renderable:" << m_renderable->name() << ") !";

			return;
		}

		const auto * geometry = m_renderable->geometry();
		const auto pipelineLayout = program->pipelineLayout();

		/* Bind the graphics pipeline. */
		commandBuffer.bind(*program->graphicsPipeline());

		/* Bind renderable instance VBO/IBO. */
		this->bindInstanceModelLayer(commandBuffer, layerIndex);

		/* Configure the push constants. */
		this->pushMatrices(commandBuffer, *pipelineLayout, renderTarget->viewMatrices(), *program);

		uint32_t setOffset = 0;

		/* Bind view UBO. */
		commandBuffer.bind(
			*renderTarget->viewMatrices().descriptorSet(),
			*pipelineLayout,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			setOffset++
		);

		/* Bind light UBO. */
		if ( lightEmitter != nullptr && lightEmitter->isCreated() )
		{
			commandBuffer.bind(
				*lightEmitter->descriptorSet(),
				*pipelineLayout,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				setOffset++,
				lightEmitter->UBOOffset()
			);
		}

		/* Bind material UBO and samplers. */
		const auto * material = m_renderable->material(layerIndex);

		commandBuffer.bind(
			*material->descriptorSet(),
			*pipelineLayout,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			setOffset++
		);

		if ( material->isAnimated() )
		{
			commandBuffer.draw(*geometry, m_frameIndex, this->instanceCount());
		}
		else
		{
			commandBuffer.draw(*geometry, layerIndex, this->instanceCount());
		}
	}

	void
	Abstract::renderTBNSpace (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, uint32_t layerIndex, const CommandBuffer & commandBuffer) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_GPUMemoryAccess};

		const auto renderTargetProgramsIt = m_renderTargetPrograms.find(renderTarget);

		if ( renderTargetProgramsIt == m_renderTargetPrograms.end() )
		{
			TraceError{TracerTag} << "There is no render target programs named '" << renderTarget->id() << "' in the renderable instance (Renderable:" << m_renderable->name() << ") !";

			return;
		}

		const auto program = renderTargetProgramsIt->second->TBNSpaceProgram(layerIndex);

		if ( program == nullptr )
		{
			TraceError{TracerTag} << "There is no suitable TBN space program for the renderable instance (Renderable:" << m_renderable->name() << ") !";

			return;
		}

		const auto pipelineLayout = program->pipelineLayout();

		commandBuffer.bind(*program->graphicsPipeline());

		/* Bind view UBO. */
		if ( this->isFlagEnabled(EnableInstancing) )
		{
			commandBuffer.bind(
				*renderTarget->viewMatrices().descriptorSet(),
				*pipelineLayout,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				0
			);
		}

		this->bindInstanceModelLayer(commandBuffer, layerIndex);

		this->pushMatrices(commandBuffer, *pipelineLayout, renderTarget->viewMatrices(), *program);

		commandBuffer.draw(*m_renderable->geometry(), layerIndex, this->instanceCount());
	}

	bool
	Abstract::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable == m_renderable.get() )
		{
			switch ( notificationCode )
			{
				case Resources::ResourceTrait::LoadFinished :
					this->notify(ReadyToSetupOnGPU, this->shared_from_this());
					break;

				case Resources::ResourceTrait::LoadFailed :
				{
					std::stringstream errorMessage;
					errorMessage <<
						"Unable to load the renderable '" << m_renderable->name() << "' ! "
						"Set the renderable instance as broken.";

					this->setBroken(errorMessage.str());
				}
					break;

				default:
					break;
			}

			return true;
		}

		return false;
	}
}
