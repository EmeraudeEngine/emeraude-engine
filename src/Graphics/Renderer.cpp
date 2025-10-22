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
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Scenes/Scene.hpp"
#include "PrimaryServices.hpp"
#include "Overlay/Manager.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::PixelFactory;
	using namespace Vulkan;
	using namespace Saphir;

	bool
	RendererFrameScope::initialize (const std::shared_ptr< Device > & device, uint32_t frameIndex) noexcept
	{
		const auto frameName = RendererFrameScope::getFrameName(frameIndex);

		m_frameIndex = frameIndex;

		/* NOTE: We create a rendering command pool, no individual reset for command buffer. */
		m_commandPool = std::make_shared< CommandPool >(device, device->getGraphicsFamilyIndex(), true, false, false);
		m_commandPool->setIdentifier(ClassId, frameName, "CommandPool");

		if ( !m_commandPool->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the command pool #" << m_frameIndex << '!';

			return false;
		}

		m_inFlightFence = std::make_unique< Sync::Fence >(device, VK_FENCE_CREATE_SIGNALED_BIT);
#if IS_MACOS
		m_inFlightFence->setIdentifier(ClassId, (std::stringstream{} << "Frame" << frameIndex << "ImageInFlight").str(), "Fence");
#else
		m_inFlightFence->setIdentifier(ClassId, std::format("Frame{}ImageInFlight", frameIndex), "Fence");
#endif

		if ( !m_inFlightFence->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create a fence #" << frameIndex << " for in-flight!";

			return false;
		}

		m_imageAvailableSemaphore = std::make_unique< Sync::Semaphore >(device);
#if IS_MACOS
		m_imageAvailableSemaphore->setIdentifier(ClassId, (std::stringstream{} << "Frame" << frameIndex << "ImageAvailable").str(), "Semaphore");
#else
		m_imageAvailableSemaphore->setIdentifier(ClassId, std::format("Frame{}ImageAvailable", frameIndex), "Semaphore");
#endif

		if ( !m_imageAvailableSemaphore->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create a semaphore #" << frameIndex << " for image available!";

			return false;
		}

		m_renderFinishedSemaphore = std::make_unique< Sync::Semaphore >(device);
#if IS_MACOS
		m_renderFinishedSemaphore->setIdentifier(ClassId, (std::stringstream{} << "Frame" << frameIndex << "RenderFinished").str(), "Semaphore");
#else
		m_renderFinishedSemaphore->setIdentifier(ClassId, std::format("Frame{}RenderFinished", frameIndex), "Semaphore");
#endif

		if ( !m_renderFinishedSemaphore->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create a semaphore #" << frameIndex << " for image finished!";

			return false;
		}

		return true;
	}

	std::shared_ptr< CommandBuffer >
	RendererFrameScope::getCommandBuffer (const RenderTarget::Abstract * renderTarget) noexcept
	{
		if ( const auto commandBufferIt = m_commandBuffers.find(renderTarget); commandBufferIt != m_commandBuffers.cend() )
		{
			return commandBufferIt->second;
		}

		auto commandBuffer = std::make_shared< CommandBuffer >(m_commandPool, true);
		commandBuffer->setIdentifier(ClassId, renderTarget->id(), "CommandBuffer");

		if ( !commandBuffer->isCreated() )
		{
			TraceError{ClassId} << "Unable to create a command buffer for render target '" << renderTarget->id() << "' !";

			return {};
		}

		m_commandBuffers.emplace(renderTarget, commandBuffer);

		return commandBuffer;
	}

	void
	RendererFrameScope::declareSemaphore (const std::shared_ptr< Sync::Semaphore > & semaphore, bool primary) noexcept
	{
		const auto handle = semaphore->handle();

		if ( primary )
		{
			m_primarySemaphores.emplace_back(handle);
		}

		m_secondarySemaphores.emplace_back(handle);
	}

	bool
	Renderer::initializeSubServices () noexcept
	{
		/* Initialize the graphics shader manager. */
		if ( m_shaderManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_shaderManager.name() << " service up!";
		}
		else
		{
			TraceFatal{ClassId} <<
				m_shaderManager.name() << " service failed to execute!" "\n"
				"The engine is unable to produce GLSL shaders!";

			return false;
		}

		/* Initialize a transfer manager for graphics. */
		m_transferManager.setDevice(m_device);

		if ( m_transferManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_transferManager.name() << " service up!";
		}
		else
		{
			TraceFatal{ClassId} << m_transferManager.name() << " service failed to execute!";

			return false;
		}

		/* Initialize the layout manager for graphics. */
		m_layoutManager.setDevice(m_device);

		if ( m_layoutManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_layoutManager.name() << " service up!";
		}
		else
		{
			TraceFatal{ClassId} << m_layoutManager.name() << " service failed to execute!";

			return false;
		}

		/* Initialize a shared UBO manager for graphics. */
		m_sharedUBOManager.setDevice(m_device);

		if ( m_sharedUBOManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_sharedUBOManager.name() << " service up!";
		}
		else
		{
			TraceFatal{ClassId} << m_sharedUBOManager.name() << " service failed to execute!";

			return false;
		}

		/* Initialize vertex buffer format manager. */
		if ( m_vertexBufferFormatManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_vertexBufferFormatManager.name() << " service up!";
		}
		else
		{
			TraceFatal{ClassId} << m_vertexBufferFormatManager.name() << " service failed to execute!";

			return false;
		}

		/* Initialize video input. */
		if ( m_externalInput.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_externalInput.name() << " service up!";
		}
		else
		{
			TraceWarning{ClassId} <<
				m_externalInput.name() << " service failed to execute!" "\n"
				"No video input available!";
		}

		return true;
	}

	bool
	Renderer::onInitialize () noexcept
	{
		m_windowLess = m_primaryServices.arguments().isSwitchPresent("-W", "--window-less");

		/* NOTE: Graphics device selection from the vulkan instance.
		 * The Vulkan instance doesn't directly create a device on its initialization. */
		if ( m_vulkanInstance.usable() )
		{
			m_device = m_vulkanInstance.getGraphicsDevice(m_windowLess ? nullptr : &m_window);

			if ( m_device == nullptr )
			{
				Tracer::fatal(ClassId, "Unable to find a suitable graphics device!");

				return false;
			}
		}
		else
		{
			Tracer::fatal(ClassId, "The Vulkan instance is not usable to select a graphics device!");

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
			Tracer::fatal(ClassId, "Unable to initialize renderer sub-services properly!");

			return false;
		}

		/* NOTE: Create the swap-chain for presenting images to the screen. */
		if ( m_windowLess )
		{
			/* NOTE: Check for multisampling. */
			auto sampleCount = m_primaryServices.settings().getOrSetDefault< uint32_t >(VideoFramebufferSamplesKey, DefaultVideoFramebufferSamples);

			if ( sampleCount > 1 )
			{
				sampleCount = m_device->findSampleCount(sampleCount);
			}

			m_windowLessView = std::make_shared< RenderTarget::View< ViewMatrices2DUBO > >(
				"WindowLessView",
				m_window.state().windowWidth,
				m_window.state().windowHeight,
				FramebufferPrecisions{8, 8, 8, 8, 24, 0, sampleCount},
				false
			);

			if ( !m_windowLessView->create(*this) )
			{
				Tracer::fatal(ClassId, "Unable to create the window less view!");

				return false;
			}

			/* Create a command pools and command buffers following the offscreen view image. */
			if ( !this->createRenderingSystem(1) )
			{
				m_windowLessView.reset();

				Tracer::fatal(ClassId, "Unable to create the offscreen view command pools and buffers!");

				return false;
			}
		}
		else
		{
			m_swapChain = std::make_shared< SwapChain >(m_device, *this, m_primaryServices.settings());
			m_swapChain->setIdentifier(ClassId, "Main", "SwapChain");

			if ( !m_swapChain->createOnHardware() )
			{
				Tracer::fatal(ClassId, "Unable to create the swap-chain!");

				return false;
			}

			/* Create a command pools and command buffers following the swap-chain images. */
			if ( !this->createRenderingSystem(m_swapChain->imageCount()) )
			{
				m_swapChain.reset();

				Tracer::fatal(ClassId, "Unable to create the swap-chain command pools and buffers!");

				return false;
			}
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
				Tracer::fatal(ClassId, "Unable to create the descriptor pool!");

				return false;
			}
		}

		this->registerToConsole();

		/* Reading some parameters. */
		{
			auto & settings = m_primaryServices.settings();

			if ( settings.getOrSetDefault< bool >(NormalMappingEnabledKey, DefaultNormalMappingEnabled) )
			{
				Tracer::info(ClassId, "Normal mapping enabled.");
			}

			if ( settings.getOrSetDefault< bool >(HighQualityLightEnabledKey, DefaultHighQualityLightEnabled) )
			{
				Tracer::info(ClassId, "High quality light shader code enabled.");
			}

			if ( settings.getOrSetDefault< bool >(HighQualityReflectionEnabledKey, DefaultHighQualityReflectionEnabled) )
			{
				Tracer::info(ClassId, "High quality reflection shader code enabled.");
			}
		}

		return true;
	}

	bool
	Renderer::onTerminate () noexcept
	{
		m_device->waitIdle("Renderer::onTerminate()");

		size_t error = 0;

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

		m_descriptorPool.reset();

		this->destroyRenderingSystem();

		m_swapChain.reset();
		m_windowLessView.reset();

		/* Terminate sub-services. */
		{
			for ( auto * service : std::ranges::reverse_view(m_subServicesEnabled) )
			{
				if ( service->terminate() )
				{
					TraceSuccess{ClassId} << service->name() << " sub-service terminated gracefully!";
				}
				else
				{
					error++;

					TraceError{ClassId} << service->name() << " sub-service failed to terminate properly!";
				}
			}

			m_subServicesEnabled.clear();
		}

		/* Release the pointer on the device. */
		m_device.reset();

		return error == 0;
	}

	void
	Renderer::onRegisterToConsole () noexcept
	{

	}

	std::shared_ptr< RenderTarget::Abstract >
	Renderer::mainRenderTarget () const noexcept
	{
		if ( m_windowLess )
		{
			return m_windowLessView;
		}

		return m_swapChain;
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
	Renderer::getSampler (const char * identifier, const std::function< void (Settings & settings, VkSamplerCreateInfo &) > & setupCreateInfo) noexcept
	{
		if ( const auto samplerIt = m_samplers.find(identifier); samplerIt != m_samplers.cend() )
		{
			return samplerIt->second;
		}

		VkSamplerCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.magFilter = VK_FILTER_NEAREST;
		createInfo.minFilter = VK_FILTER_NEAREST;
		createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		createInfo.mipLodBias = 0.0F;
		createInfo.anisotropyEnable = VK_FALSE;
		createInfo.maxAnisotropy = 1.0F;
		createInfo.compareEnable = VK_FALSE;
		createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		createInfo.minLod = 0.0F;
		createInfo.maxLod = VK_LOD_CLAMP_NONE;
		createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		createInfo.unnormalizedCoordinates = VK_FALSE;

		setupCreateInfo(this->primaryServices().settings(), createInfo);

		auto sampler = std::make_shared< Sampler >(m_device, createInfo);
		sampler->setIdentifier(ClassId, identifier, "Sampler");

		if ( !sampler->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create a sampler!");

			return nullptr;
		}

		const auto [samplerPair, success] = m_samplers.emplace(identifier, sampler);

		if constexpr ( IsDebug )
		{
			if ( !success )
			{
				Tracer::fatal(ClassId, "Unable to insert the sampler into map!");

				return {};
			}
		}

		return samplerPair->second;
	}

	[[nodiscard]]
	bool
	Renderer::finalizeGraphicsPipeline (const RenderTarget::Abstract & renderTarget, const Program & program, std::shared_ptr< GraphicsPipeline > & graphicsPipeline) noexcept
	{
		/* FIXME: This is a fake hash! */
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
		if ( m_windowLess )
		{
			auto & currentFrameScope = m_rendererFrameScope[0];

			/* NOTE: Wait for the current frame to complete. */
			if ( currentFrameScope.inFlightFence()->waitAndReset(m_timeout) )
			{
				m_statistics.stop();

				currentFrameScope.prepareForNewFrame();
			}
			else
			{
				TraceError{ClassId} << "Something wrong happens while waiting the fence for image!";

				std::abort();

				return;
			}

			m_statistics.start();

			/* NOTE: Offscreen rendering */
			if ( scene != nullptr )
			{
				if ( this->isShadowMapsEnabled() )
				{
					/* [VULKAN-SHADOW] */
					this->renderShadowMaps(currentFrameScope, *scene);
				}

				if ( this->isRenderToTexturesEnabled() )
				{
					this->renderRenderToTextures(currentFrameScope, *scene);
				}

				//this->renderViews(currentFrameScope, *scene);
			}

			const auto * queue = this->device()->getGraphicsQueue(QueuePriority::High);

			/* Then we need the command buffer linked to this image by its index. */
			const auto commandBuffer = currentFrameScope.getCommandBuffer(m_windowLessView.get());

			if ( !commandBuffer->begin() )
			{
				return;
			}

			commandBuffer->beginRenderPass(*m_windowLessView->framebuffer(), m_windowLessView->renderArea(), m_clearColors, VK_SUBPASS_CONTENTS_INLINE);

			/* First, render the scene. */
			if ( scene != nullptr )
			{
				scene->render(m_windowLessView, *commandBuffer);
			}

			/* Then render the overlay system over the 3D-rendered scene. */
			overlayManager.render(m_windowLessView, *commandBuffer);

			commandBuffer->endRenderPass();

			if ( !commandBuffer->end() )
			{
				return;
			}

			const auto submitResult = queue->submit(
				*commandBuffer,
				SynchInfo{}
					.withFence(currentFrameScope.inFlightFence()->handle())
			);

			if ( !submitResult )
			{
				TraceError{ClassId} << "Unable to submit command buffer for render target '" << m_windowLessView->id() << "' !";

				return;
			}
		}
		else
		{
			/* 1. If the swap-chain was marked degraded, we rebuild it and skip this frame. */
			if ( m_swapChain->status() == Status::Degraded )
			{
				Tracer::info(ClassId, "The swap-chain is degraded, refreshing it...");

				if ( !this->refreshFramebuffer() )
				{
					Tracer::fatal(ClassId, "Unable to refresh the swap-chain!");

					std::abort();
				}

				/* Let this image drop. */
				return;
			}

			auto & currentFrameScope = m_rendererFrameScope[m_currentFrameIndex];

			/* 2. Wait for the previous use of this frame's resources to complete. */
			if ( currentFrameScope.inFlightFence()->wait(m_timeout) )
			{
				m_statistics.stop();

				currentFrameScope.prepareForNewFrame();
			}
			else
			{
				TraceError{ClassId} << "Something wrong happens while waiting the fence for image #" << m_currentFrameIndex << '!';

				std::abort();

				return;
			}

			/* 3. Get the new frame to render to. */
			const auto frameIndexOpt = m_swapChain->acquireNextImage(currentFrameScope.imageAvailableSemaphore(), m_timeout);

			if ( !frameIndexOpt )
			{
				Tracer::error(ClassId, "Unable to acquire swap-chain image (likely out of date)!");

				return;
			}

			/* 4. Reset the fence. */
			if ( !currentFrameScope.inFlightFence()->reset() )
			{
				TraceError{ClassId} << "Something wrong happens while reset the fence for image #" << m_currentFrameIndex << '!';

				return;
			}

			/* 5. The new frame rendering is starting now. */
			m_statistics.start();

			const uint32_t frameIndex = frameIndexOpt.value();

			/* NOTE: Offscreen rendering */
			if ( scene != nullptr )
			{
				if ( this->isShadowMapsEnabled() )
				{
					/* [VULKAN-SHADOW] */
					this->renderShadowMaps(currentFrameScope, *scene);
				}

				if ( this->isRenderToTexturesEnabled() )
				{
					this->renderRenderToTextures(currentFrameScope, *scene);
				}

				//this->renderViews(currentFrameScope, *scene);
			}

			/* Then we need the command buffer linked to this image by its index. */
			const auto commandBuffer = currentFrameScope.getCommandBuffer(m_swapChain.get());

			if ( !commandBuffer->begin() )
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

			/* 6. Submit the work on the GPU and present. */
			{
				const auto * queue = this->device()->getGraphicsQueue(QueuePriority::High);

				currentFrameScope.secondarySemaphores().emplace_back(currentFrameScope.imageAvailableSemaphore()->handle());

				const StaticVector< VkPipelineStageFlags, 16 > waitStages(
					currentFrameScope.secondarySemaphores().size(),
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
				);

				auto renderFinishedSemaphoreHandle = currentFrameScope.renderFinishedSemaphore()->handle();

				if ( !queue->submit(*commandBuffer, SynchInfo{}
						.waits(currentFrameScope.secondarySemaphores(), waitStages)
						.signals({&renderFinishedSemaphoreHandle, 1})
						.withFence(currentFrameScope.inFlightFence()->handle())
					) )
				{
					return;
				}

				m_swapChain->present(frameIndex, queue, renderFinishedSemaphoreHandle);
			}

			m_currentFrameIndex = (m_currentFrameIndex + 1) % m_rendererFrameScope.size();
		}
	}

	void
	Renderer::renderShadowMaps (RendererFrameScope & currentFrameScope, Scenes::Scene & scene) const noexcept
	{
		const auto * queue = this->device()->getGraphicsQueue(QueuePriority::High);

		scene.forEachRenderToShadowMap([&] (const std::shared_ptr< RenderTarget::Abstract > & shadowMap) {
			if ( !shadowMap->isReadyForRendering() )
			{
				TraceDebug{ClassId} << "The shadow map " << shadowMap->id() << " is not yet ready for rendering!";

				return;
			}

			const auto commandBuffer = currentFrameScope.getCommandBuffer(shadowMap.get());

			if ( !commandBuffer->begin() )
			{
				TraceError{ClassId} << "Unable to begin with render target '" << shadowMap->id() << "' command buffer!";

				return;
			}

			commandBuffer->beginRenderPass(*shadowMap->framebuffer(), shadowMap->renderArea(), m_clearColors, VK_SUBPASS_CONTENTS_INLINE);

			scene.castShadows(shadowMap, *commandBuffer);

			commandBuffer->endRenderPass();

			if ( !commandBuffer->end() )
			{
				TraceError{ClassId} << "Unable to finish the command buffer for render target '" << shadowMap->id() << '!';

				return;
			}

			const auto semaphoreHandle = shadowMap->semaphore()->handle();

			const auto submitted = queue->submit(
				*commandBuffer,
				SynchInfo{}
					.signals({&semaphoreHandle, 1})
			);

			if ( !submitted )
			{
				TraceError{ClassId} << "Unable to submit command buffer for render target '" << shadowMap->id() << "' !";

				return;
			}

			currentFrameScope.declareSemaphore(shadowMap->semaphore(), true);
		});
	}

	void
	Renderer::renderRenderToTextures (RendererFrameScope & currentFrameScope, Scenes::Scene & scene) const noexcept
	{
		const auto * queue = this->device()->getGraphicsQueue(QueuePriority::High);

		scene.forEachRenderToTexture([&] (const std::shared_ptr< RenderTarget::Abstract > & renderToTexture)
		{
			if ( !renderToTexture->isReadyForRendering() )
			{
				TraceDebug{ClassId} << "The render-to-texture " << renderToTexture->id() << " is not yet ready for rendering!";

				return;
			}

			const auto commandBuffer = currentFrameScope.getCommandBuffer(renderToTexture.get());

			if ( !commandBuffer->begin() )
			{
				TraceError{ClassId} << "Unable to begin with render target '" << renderToTexture->id() << "' command buffer!";

				return;
			}

			commandBuffer->beginRenderPass(*renderToTexture->framebuffer(), renderToTexture->renderArea(), m_clearColors, VK_SUBPASS_CONTENTS_INLINE);

			scene.render(renderToTexture, *commandBuffer);

			commandBuffer->endRenderPass();

			if ( !commandBuffer->end() )
			{
				TraceError{ClassId} << "Unable to finish the command buffer for render target '" << renderToTexture->id() << "' !";

				return;
			}

			const auto signalSemaphoreHandle = renderToTexture->semaphore()->handle();

			const auto & waitSemaphores = currentFrameScope.primarySemaphores();
			const StaticVector< VkPipelineStageFlags, 16 > waitStages(waitSemaphores.size(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

			const auto submitted = queue->submit(
				*commandBuffer,
				SynchInfo{}
					.waits(waitSemaphores, waitStages)
					.signals({&signalSemaphoreHandle, 1})
			);

			if ( !submitted )
			{
				TraceError{ClassId} << "Unable to submit command buffer for render target '" << renderToTexture->id() << "' !";

				return;
			}

			currentFrameScope.declareSemaphore(renderToTexture->semaphore(), false);
		});
	}

	void
	Renderer::renderViews (RendererFrameScope & /*currentFrameScope*/, Scenes::Scene & /*scene*/) const noexcept
	{

	}

	bool
	Renderer::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable->is(Window::getClassUID()) )
		{
			switch ( notificationCode )
			{
				case Window::OSNotifiesFramebufferResized :
				case Window::OSRequestsToRescaleContentBy :
					if ( m_windowLess )
					{
						// TODO: Resize the framebuffer to the right size!
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
			"Received an unhandled notification (Code:" << notificationCode << ") from observable (UID:" << observable->classUID() << ")! "
			"Forgetting it ...";

		return false;
	}

	bool
	Renderer::createRenderingSystem (uint32_t imageCount) noexcept
	{
		m_rendererFrameScope.resize(imageCount);

		for ( uint32_t imageIndex = 0; imageIndex < imageCount; imageIndex++ )
		{
			if ( !m_rendererFrameScope[imageIndex].initialize(m_device, imageIndex) )
			{
				TraceError{ClassId} << "Unable to create the render frame scope #" << imageIndex << '!';

				return false;
			}
		}

		return true;
	}

	void
	Renderer::destroyRenderingSystem () noexcept
	{
		m_rendererFrameScope.clear();
	}

	bool
	Renderer::refreshFramebuffer () noexcept
	{
		this->device()->waitIdle("Refreshing the framebuffer.");

		if ( !m_window.surface()->update(this->device()->physicalDevice()) )
		{
			Tracer::error(ClassId, "Unable to update the handle surface from a framebuffer resized!");

			return false;
		}

		if ( !m_swapChain->refresh() )
		{
			return false;
		}

		m_swapChainRefreshed = true;

		return true;
	}
}
