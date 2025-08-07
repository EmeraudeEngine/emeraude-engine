/*
 * src/Graphics/Renderer.cpp
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

#include "Renderer.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <ranges>

/* Local inclusions. */
#include "Libs/Time/Elapsed/PrintScopeRealTime.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/Types.hpp"
#include "Scenes/Scene.hpp"
#include "PrimaryServices.hpp"
#include "Overlay/Manager.hpp"

namespace EmEn::Graphics
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace EmEn::Libs::PixelFactory;
	using namespace EmEn::Vulkan;
	using namespace EmEn::Saphir;

	const size_t Renderer::ClassUID{getClassUID(ClassId)};
	Renderer * Renderer::s_instance{nullptr};

	bool
	Renderer::initializeSubServices () noexcept
	{
		/* Initialize the graphics shader manager. */
		if ( m_shaderManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_shaderManager.name() << " service up !";
		}
		else
		{
			TraceFatal{ClassId} <<
				m_shaderManager.name() << " service failed to execute !" "\n"
				"The engine is unable to produce GLSL shaders !";

			return false;
		}

		/* Initialize a transfer manager for graphics. */
		m_transferManager.setDevice(m_device);

		if ( m_transferManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_transferManager.name() << " service up !";
		}
		else
		{
			TraceFatal{ClassId} << m_transferManager.name() << " service failed to execute !";

			return false;
		}

		/* Initialize the layout manager for graphics. */
		m_layoutManager.setDevice(m_device);

		if ( m_layoutManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_layoutManager.name() << " service up !";
		}
		else
		{
			TraceFatal{ClassId} << m_layoutManager.name() << " service failed to execute !";

			return false;
		}

		/* Initialize a shared UBO manager for graphics. */
		m_sharedUBOManager.setDevice(m_device);

		if ( m_sharedUBOManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_sharedUBOManager.name() << " service up !";
		}
		else
		{
			TraceFatal{ClassId} << m_sharedUBOManager.name() << " service failed to execute !";

			return false;
		}

		/* Initialize vertex buffer format manager. */
		if ( m_vertexBufferFormatManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_vertexBufferFormatManager.name() << " service up !";
		}
		else
		{
			TraceFatal{ClassId} << m_vertexBufferFormatManager.name() << " service failed to execute !";

			return false;
		}

		/* Initialize video input. */
		if ( m_externalInput.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_externalInput.name() << " service up !";
		}
		else
		{
			TraceWarning{ClassId} <<
				m_externalInput.name() << " service failed to execute !" "\n"
				"No video input available !";
		}

		return true;
	}

	bool
	Renderer::onInitialize () noexcept
	{
		/* NOTE: Graphics device selection from the vulkan instance.
		 * The Vulkan instance doesn't directly create a device on its initialization. */
		if ( m_vulkanInstance.usable() )
		{
			m_device = m_vulkanInstance.getGraphicsDevice(&m_window);

			if ( m_device == nullptr )
			{
				Tracer::fatal(ClassId, "Unable to find a suitable graphics device !");

				return false;
			}
		}
		else
		{
			Tracer::fatal(ClassId, "The Vulkan instance is not usable to select a graphics device !");

			return false;
		}

		/*
		 * NOTE: Initialize all sub-services:
		 *  - The shader manager (for shader code generation to binary in the GPU)
		 *  - The transfer manager (for memory move from CPU to GPU)
		 *  - The layout manager (for a graphics pipeline)
		 *  - The shared uniform buffer object manager (to re-use the same large UBO between objects)
		 *  - The vertex buffer format manager (to describe each vertex buffer once)
		 *  - The external input manager
		 */
		if ( !this->initializeSubServices() )
		{
			Tracer::fatal(ClassId, "Unable to initialize renderer sub-services properly !");

			return false;
		}

		/* NOTE: Create the swap-chain for presenting images to the screen. */
		{
			m_swapChain = std::make_shared< SwapChain >(m_device, *this, m_primaryServices.settings());
			m_swapChain->setIdentifier(ClassId, "Main", "SwapChain");

			if ( !m_swapChain->createOnHardware() )
			{
				Tracer::fatal(ClassId, "Unable to create the swap-chain !");

				return false;
			}

			/* Create a command pools and command buffers following the swap-chain images. */
			if ( !this->createCommandSystem() )
			{
				m_swapChain.reset();

				Tracer::fatal(ClassId, "Unable to create the swap-chain command pools and buffers !");

				return false;
			}

			this->notify(SwapChainCreated, m_swapChain);
		}

		/* NOTE: Create the main descriptor pool. */
		{
			// TODO: Sizes management is maybe in the wrong place !
			const auto sizes = std::vector< VkDescriptorPoolSize >{
				/* NOTE: Texture filtering alone. */
				{VK_DESCRIPTOR_TYPE_SAMPLER, 16},
				/* NOTE: Texture (that can be sampled). */
				{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 64},
				/* NOTE: Texture associated with a filter (VK_DESCRIPTOR_TYPE_SAMPLER+VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE). */
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64},
				/* NOTE:  */
				//{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0},
				/* NOTE:  */
				//{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 0},
				/* NOTE:  */
				//{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 0},
				/* NOTE: UBO (Uniform Buffer Object) */
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 512},
				/* NOTE: SSBO (Shader Storage Buffer Object) */
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 64},
				/* NOTE: Dynamic UBO (Uniform Buffer Object) */
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 512},
				/* NOTE: Dynamic SSBO (Shader Storage Buffer Object) */
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 64},
				/* NOTE:  */
				{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 32},
				/* NOTE: Special UBO (Uniform Buffer Object). */
				// TODO: Check to enable this for re-usable UBO between render calls.
				//{VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK, 512},
				/* NOTE:  */
				//{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0},
				/* NOTE:  */
				//{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 0},
				/* NOTE:  */
				//{VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM, 0},
				/* NOTE:  */
				//{VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM, 0},
				/* NOTE:  */
				//{VK_DESCRIPTOR_TYPE_MUTABLE_EXT, 0}
			};

			m_descriptorPool = std::make_shared< DescriptorPool >(m_device, sizes, 4096, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
			m_descriptorPool->setIdentifier(ClassId, "Main", "DescriptorPool");

			if ( !m_descriptorPool->createOnHardware() )
			{
				Tracer::fatal(ClassId, "Unable to create the descriptor pool !");

				return false;
			}
		}

		this->registerToConsole();

		/* Reading some parameters. */
		{
			auto & settings = m_primaryServices.settings();

			if ( settings.get< bool >(NormalMappingEnabledKey, DefaultNormalMappingEnabled) )
			{
				Tracer::info(ClassId, "Normal mapping enabled !");
			}

			if ( settings.get< bool >(HighQualityLightEnabledKey, DefaultHighQualityLightEnabled) )
			{
				Tracer::info(ClassId, "High quality light shader code enabled !");
			}

			if ( settings.get< bool >(HighQualityReflectionEnabledKey, DefaultHighQualityReflectionEnabled) )
			{
				Tracer::info(ClassId, "High quality reflection shader code enabled !");
			}
		}

		m_serviceInitialized = true;

		return true;
	}

	bool
	Renderer::onTerminate () noexcept
	{
		size_t error = 0;

		m_serviceInitialized = false;

		m_device->waitIdle("Renderer::onTerminate()");

		/* NOTE: Stacked resources on the runtime. */
		{
			for ( const auto & sampler: m_samplers | std::views::values )
			{
				sampler->destroyFromHardware();
			}

			m_samplers.clear();

			for ( const auto & renderPass: m_renderPasses | std::views::values )
			{
				renderPass->destroyFromHardware();
			}

			m_renderPasses.clear();

			for ( const auto & pipeline: m_pipelines | std::views::values )
			{
				pipeline->destroyFromHardware();
			}

			m_pipelines.clear();
		}

		this->destroyCommandSystem();

		if ( m_swapChain != nullptr )
		{
			m_swapChain->destroyFromHardware();
			m_swapChain.reset();
		}

		if ( m_descriptorPool != nullptr )
		{
			m_descriptorPool->destroyFromHardware();
			m_descriptorPool.reset();
		}

		/* Terminate sub-services. */
		for ( auto * service : std::ranges::reverse_view(m_subServicesEnabled) )
		{
			if ( service->terminate() )
			{
				TraceSuccess{ClassId} << service->name() << " sub-service terminated gracefully !";
			}
			else
			{
				error++;

				TraceError{ClassId} << service->name() << " sub-service failed to terminate properly !";
			}
		}

		m_subServicesEnabled.clear();

		/* Release the pointer on the device. */
		m_device.reset();

		return error == 0;
	}

	void
	Renderer::onRegisterToConsole () noexcept
	{

	}

	std::shared_ptr< RenderPass >
	Renderer::getRenderPass (const std::string & identifier, VkRenderPassCreateFlags createFlags) noexcept
	{
		const auto uniqueIdentifier = (std::stringstream{} << identifier << '+' << createFlags).str();

		const auto renderPassIt = m_renderPasses.find(uniqueIdentifier);

		if ( renderPassIt == m_renderPasses.cend() )
		{
			auto renderPass = std::make_shared< RenderPass >(this->device(), createFlags);
			renderPass->setIdentifier(ClassId, uniqueIdentifier, "RenderPass");

			m_renderPasses.emplace(uniqueIdentifier, renderPass);

			return renderPass;
		}

		return renderPassIt->second;
	}

	std::shared_ptr< Sampler >
	Renderer::getSampler (size_t type, VkSamplerCreateFlags createFlags) noexcept
	{
		if ( const auto samplerIt = m_samplers.find(type); samplerIt != m_samplers.cend() )
		{
			return samplerIt->second;
		}

		auto sampler = std::make_shared< Sampler >(m_device, m_primaryServices.settings(), createFlags);
		sampler->setIdentifier(ClassId, "Main", "Sampler");

		if ( !sampler->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create a sampler !");

			return nullptr;
		}

		const auto [samplerPair, success] = m_samplers.emplace(type, sampler);

		if constexpr ( IsDebug )
		{
			if ( !success )
			{
				Tracer::fatal(ClassId, "Unable to insert the sampler into map !");

				return {};
			}
		}

		return samplerPair->second;
	}

	[[nodiscard]]
	bool
	Renderer::finalizeGraphicsPipeline (const RenderTarget::Abstract & renderTarget, const Program & program, std::shared_ptr< GraphicsPipeline > & graphicsPipeline) noexcept
	{
		/* FIXME: Fake hash ! */
		const auto hash = GraphicsPipeline::getHash();

		if ( const auto pipelineIt = m_pipelines.find(hash); pipelineIt != m_pipelines.cend() )
		{
			graphicsPipeline = pipelineIt->second;

			return true;
		}

		if ( !graphicsPipeline->finalize(renderTarget.framebuffer()->renderPass(), program.pipelineLayout(), program.useTesselation(), m_vulkanInstance.isDynamicStateExtensionEnabled()) )
		{
			return false;
		}

		return m_pipelines.emplace(hash, graphicsPipeline).second;
	}

	void
	Renderer::renderFrame (const std::shared_ptr< Scenes::Scene > & scene, const Overlay::Manager & overlayManager) noexcept
	{
		if ( !this->usable() )
		{
			return;
		}

		if ( m_swapChain->status() == SwapChain::Status::Degraded )
		{
			TraceInfo{ClassId} << "The swap-chain is degraded !";

			if ( !this->recreateSwapChain() )
			{
				return;
			}
		}

		m_statistics.start();

		const auto imageIndex = m_swapChain->acquireNextImage();

		if ( !imageIndex )
		{
			return;
		}

		/* NOTE: Clear all semaphores for the new frame. */
		m_rendererFrameScope[imageIndex.value()].clearSemaphores();

		/* NOTE: Offscreen rendering */
		if ( scene != nullptr )
		{
			if ( this->isShadowMapsEnabled() )
			{
				/* [VULKAN-SHADOW] */
				this->renderShadowMaps(imageIndex.value(), *scene);
			}

			if ( this->isRenderToTexturesEnabled() )
			{
				this->renderRenderToTextures(imageIndex.value(), *scene);
			}

			this->renderViews(imageIndex.value(), *scene);
		}

		/* Then we need the command buffer linked to this image by its index. */
		const auto commandBuffer = m_rendererFrameScope[imageIndex.value()].commandBuffer();

		if ( !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return;
		}

		commandBuffer->beginRenderPass(*m_swapChain->framebuffer(), m_swapChain->renderArea(), m_clearColors, VK_SUBPASS_CONTENTS_INLINE);

		/* First, render the scene. */
		if ( scene != nullptr )
		{
			scene->render(m_swapChain, *commandBuffer);
		}

		/* Then render the overlay system over the 3D-rendered scene. */
		overlayManager.render(m_swapChain, *commandBuffer);

		commandBuffer->endRenderPass();

		if ( !commandBuffer->end() )
		{
			return;
		}

		if ( !m_swapChain->submitCommandBuffer(commandBuffer, imageIndex.value(), m_rendererFrameScope[imageIndex.value()].secondarySemaphores()) )
		{
			return;
		}

		m_statistics.stop();
	}

	std::shared_ptr< CommandBuffer >
	Renderer::getCommandBuffer (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) noexcept
	{
		const auto commandBufferIt = m_offScreenCommandBuffers.find(renderTarget);

		if ( commandBufferIt != m_offScreenCommandBuffers.cend() )
		{
			return commandBufferIt->second;
		}

		auto commandBuffer = std::make_shared< CommandBuffer >(m_offScreenCommandPool);
		commandBuffer->setIdentifier(ClassId, renderTarget->id(), "CommandBuffer");

		if ( !commandBuffer->isCreated() )
		{
			TraceError{ClassId} << "Unable to create the off-screen command buffer for render target '" << renderTarget->id() << "' !";

			return {};
		}

		m_offScreenCommandBuffers.emplace(renderTarget, commandBuffer);

		return commandBuffer;
	}

	void
	Renderer::renderShadowMaps (uint32_t frameIndex, Scenes::Scene & scene) noexcept
	{
		const auto * queue = this->device()->getQueue(QueueJob::Graphics, QueuePriority::High);

		scene.forEachRenderToShadowMap([&] (const std::shared_ptr< RenderTarget::Abstract > & shadowMap) {
			if constexpr ( IsDebug )
			{
				if ( !shadowMap->isValid() )
				{
					TraceError{ClassId} << "Unable to render shadow map " << shadowMap->id() << " !";

					return;
				}
			}

			const auto fence = shadowMap->fence();

			if ( !fence->waitAndReset(250000000) )
			{
				TraceDebug{ClassId} << "Unable to wait on shadow map " << shadowMap->id() << " !";

				return;
			}

			const auto commandBuffer = this->getCommandBuffer(shadowMap);

			if ( !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
			{
				TraceError{ClassId} << "Unable to begin with render target '" << shadowMap->id() << "' command buffer !";

				return;
			}

			commandBuffer->beginRenderPass(*shadowMap->framebuffer(), shadowMap->renderArea(), m_clearColors, VK_SUBPASS_CONTENTS_INLINE);

			scene.castShadows(shadowMap, *commandBuffer);

			commandBuffer->endRenderPass();

			if ( !commandBuffer->end() )
			{
				TraceError{ClassId} << "Unable to finish the command buffer for render target '" << shadowMap->id() << " !";

				return;
			}

			const auto semaphoreHandle = shadowMap->semaphore()->handle();

			const auto submitted = queue->submit(
				commandBuffer,
				SynchInfo{}
					.signals({&semaphoreHandle, 1})
					.withFence(fence->handle())
			);

			if ( !submitted )
			{
				TraceError{ClassId} << "Unable to submit command buffer for render target '" << shadowMap->id() << "' !";

				return;
			}

			m_rendererFrameScope[frameIndex].declareSemaphore(shadowMap->semaphore(), true);
		});
	}

	void
	Renderer::renderRenderToTextures (uint32_t frameIndex, Scenes::Scene & scene) noexcept
	{
		const auto * queue = this->device()->getQueue(QueueJob::Graphics, QueuePriority::High);

		scene.forEachRenderToTexture([&] (const std::shared_ptr< RenderTarget::Abstract > & renderToTexture)
		{
			if constexpr ( IsDebug )
			{
				if ( !renderToTexture->isValid() )
				{
					TraceError{ClassId} << "Unable to render to texture " << renderToTexture->id() << " !";

					return;
				}
			}

			const auto fence = renderToTexture->fence();

			if ( !fence->waitAndReset(250000000) )
			{
				TraceDebug{ClassId} << "Unable to wait on render to texture " << renderToTexture->id() << " !";

				return;
			}

			const auto commandBuffer = this->getCommandBuffer(renderToTexture);

			if ( !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
			{
				TraceError{ClassId} << "Unable to begin with render target '" << renderToTexture->id() << "' command buffer !";

				return;
			}

			commandBuffer->beginRenderPass(*renderToTexture->framebuffer(), renderToTexture->renderArea(), m_clearColors, VK_SUBPASS_CONTENTS_INLINE);

			scene.render(renderToTexture, *commandBuffer);

			commandBuffer->endRenderPass();

			if ( !commandBuffer->end() )
			{
				TraceError{ClassId} << "Unable to finish the command buffer for render target '" << renderToTexture->id() << " !";

				return;
			}

			const auto signalSemaphoreHandle = renderToTexture->semaphore()->handle();

			const auto & waitSemaphores = m_rendererFrameScope[frameIndex].primarySemaphores();
			const StaticVector< VkPipelineStageFlags, 16 > waitStages(waitSemaphores.size(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

			const auto submitted = queue->submit(
				commandBuffer,
				SynchInfo{}
					.waits(waitSemaphores, waitStages)
					.signals({&signalSemaphoreHandle, 1})
					.withFence(fence->handle())
			);

			if ( !submitted )
			{
				TraceError{ClassId} << "Unable to submit command buffer for render target '" << renderToTexture->id() << "' !";

				return;
			}

			m_rendererFrameScope[frameIndex].declareSemaphore(renderToTexture->semaphore(), false);
		});
	}

	void
	Renderer::renderViews (uint32_t /*frameIndex*/, Scenes::Scene & /*scene*/) noexcept
	{

	}

	bool
	Renderer::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable->is(Window::ClassUID) )
		{
			switch ( notificationCode )
			{
				/* Updates the handle surface information. */
				case Window::OSNotifiesFramebufferResized :
				case Window::OSRequestsToRescaleContentBy :
				{
					const auto device = this->device();

					if ( device == nullptr )
					{
						Tracer::info(ClassId, "Vulkan instance is not fully initialized ! The device is not selected yet ...");

						break;
					}

					if ( !m_window.surface()->update(device->physicalDevice()) )
					{
						Tracer::error(ClassId, "Unable to update the handle surface from a framebuffer resized !");
					}
				}
					break;

				default :
					if constexpr ( ObserverDebugEnabled )
					{
						TraceDebug{ClassId} << "Event #" << notificationCode << " from the window ignored.";
					}
					break;
			}

			return true;
		}

		/* NOTE: Don't know what it is, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable '" << whoIs(observable->classUID()) << "' (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
	}

	bool
	Renderer::createCommandSystem () noexcept
	{
		const auto imageCount = m_swapChain->imageCount();

		m_rendererFrameScope.resize(imageCount);

		for ( uint32_t imageIndex = 0; imageIndex < imageCount; imageIndex++ )
		{
			if ( !m_rendererFrameScope[imageIndex].initialize(m_device, imageIndex) )
			{
				TraceError{ClassId} << "Unable to create the render frame scope #" << imageIndex << " !";

				return false;
			}
		}

		m_offScreenCommandPool = std::make_shared< CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, true, false);
		m_offScreenCommandPool->setIdentifier(ClassId, "offScreen", "CommandPool");

		if ( !m_offScreenCommandPool->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the off-screen command pool !";

			return false;
		}

		return true;
	}

	void
	Renderer::destroyCommandSystem () noexcept
	{
		m_device->waitIdle("Destroying the renderer command pool");

		m_offScreenCommandPool.reset();

		m_rendererFrameScope.clear();
	}

	bool
	Renderer::recreateSwapChain () noexcept
	{
		//this->destroyCommandSystem();

		/* NOTE: Wait for a valid framebuffer dimension in case of handle minimization. */
		//m_window.waitValidWindowSize();

		if ( !m_swapChain->recreateOnHardware() )
		{
			Tracer::fatal(ClassId, "Unable to rebuild the swap chain !");

			return false;
		}

		/*if ( !this->createCommandSystem() )
		{
			Tracer::fatal(ClassId, "Unable to rebuild the command system !");

			return false;
		}*/

		this->notify(SwapChainRecreated, m_swapChain);

		return true;
	}
}
