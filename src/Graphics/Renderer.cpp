/*
 * src/Graphics/Renderer.cpp
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

#include "Renderer.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <ranges>
#include <thread>

/* Local inclusions. */
#include "Libs/Time/Elapsed/PrintScopeRealTime.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/DescriptorSet.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Scenes/SceneMetaData.hpp"
#include "Scenes/LightSet.hpp"
#include "PostProcessStack.hpp"
#include "Scenes/Scene.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Overlay/Manager.hpp"
#include "PrimaryServices.hpp"
#include "DummyColorProjectionTexture.hpp"
#include "DummyShadowTexture.hpp"
#include "GrabPass.hpp"
#include "SceneRenderTarget.hpp"
#include "Geometry/Interface.hpp"
#include "TextureResource/Abstract.hpp"
#include "Material/Interface.hpp"

namespace
{
	/** @brief Empty lens effects list for the passthrough shader case. */
	constexpr std::vector< std::shared_ptr< EmEn::Graphics::DirectPostProcessEffect > > EmptyLensEffects{};
}

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
		else
		{
			m_secondarySemaphores.emplace_back(handle);
		}
	}

	Renderer::Renderer (PrimaryServices & primaryServices, Resources::Manager & resourcesManager, Instance & instance, Window & window) noexcept
		: ServiceInterface{ClassId},
		ControllableTrait{ClassId},
		m_primaryServices{primaryServices},
		m_resourcesManager{resourcesManager},
		m_renderer{resourcesManager.graphicsRenderer()},
		m_vulkanInstance{instance},
		m_window{window},
		m_shaderManager{primaryServices},
		m_sharedUBOManager{*this},
		m_bindlessTextureManager{*this},
		m_debugMode{m_vulkanInstance.isDebugModeEnabled()}
	{
		/* Framebuffer clear color value. */
		this->setClearColor(Black);

		/* Framebuffer clear depth/stencil values. */
		this->setClearDepthStencilValues(1.0F, 0);

		this->observe(&m_window);
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

		/* Initialize the bindless textures manager. */
		m_bindlessTextureManager.setDevice(m_device);

		if ( m_bindlessTextureManager.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_bindlessTextureManager.name() << " service up!";
		}
		else
		{
			TraceFatal{ClassId} << m_bindlessTextureManager.name() << " service failed to execute!";

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

		/* Initialize post-processor. */
		if ( m_postProcessor.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_postProcessor.name() << " service up!";
		}
		else
		{
			TraceWarning{ClassId} << m_postProcessor.name() << " service failed or disabled at startup!";
		}

		/* Initialize video input. */
		if ( m_externalInput.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_externalInput.name() << " service up!";
		}
		else
		{
			TraceWarning{ClassId} << m_recorder.name() << " service failed or disabled at startup!";
		}

		/* Initialize video recorder. */
		if ( m_recorder.initialize(m_subServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_recorder.name() << " service up!";
		}
		else
		{
			TraceWarning{ClassId} << m_recorder.name() << " service failed or disabled at startup!";
		}

		return true;
	}

	void
	Renderer::createDefaultResources (Resources::Manager & resources) noexcept
	{
		/* NOTE: As a default resource, we use a synchronous loading here. */
		m_defaultTextureCubemap = resources.container< TextureResource::TextureCubemap >()
			->getOrCreateResourceSync(DefaultTextureCubemapName, [&resources] (TextureResource::TextureCubemap & texture) {
				/* Create a small 16x16 black cubemap. */
				const auto blackCubemap = resources.container< CubemapResource >()
					->getOrCreateResourceSync(DefaultTextureCubemapName, [] (CubemapResource & cubemap) {
						return cubemap.load(Black, 16);
					});

				return texture.load(blackCubemap);
			});

		/* Initialize the bindless environment cubemap slot with the default cubemap. */
		if ( m_bindlessTextureManager.usable() && m_defaultTextureCubemap != nullptr )
		{
			if ( m_bindlessTextureManager.updateTextureCube(BindlessTextureManager::EnvironmentCubemapSlot, *m_defaultTextureCubemap) )
			{
				TraceSuccess{ClassId} << "Bindless environment cubemap slot initialized with default cubemap.";
			}
		}

		/* Create dummy shadow textures for unified descriptor set layouts. */
		m_dummyShadowTexture2D = std::make_shared< DummyShadowTexture >(false);

		if ( !m_dummyShadowTexture2D->create(*this) )
		{
			TraceError{ClassId} << "Unable to create the dummy 2D shadow texture !";

			m_dummyShadowTexture2D.reset();
		}

		m_dummyShadowTextureCube = std::make_shared< DummyShadowTexture >(true);

		if ( !m_dummyShadowTextureCube->create(*this) )
		{
			TraceError{ClassId} << "Unable to create the dummy cubemap shadow texture !";

			m_dummyShadowTextureCube.reset();
		}

		/* Create dummy color projection textures for unified descriptor set layouts. */
		m_dummyColorProjectionTexture2D = std::make_shared< DummyColorProjectionTexture >(false);

		if ( !m_dummyColorProjectionTexture2D->create(*this) )
		{
			TraceError{ClassId} << "Unable to create the dummy 2D color projection texture !";

			m_dummyColorProjectionTexture2D.reset();
		}

		m_dummyColorProjectionTextureCube = std::make_shared< DummyColorProjectionTexture >(true);

		if ( !m_dummyColorProjectionTextureCube->create(*this) )
		{
			TraceError{ClassId} << "Unable to create the dummy cubemap color projection texture !";

			m_dummyColorProjectionTextureCube.reset();
		}
	}

	void
	Renderer::clearDefaultResources () noexcept
	{
		if ( m_grabPass != nullptr )
		{
			m_grabPass->destroy();
			m_grabPass.reset();
		}

		if ( m_dummyColorProjectionTextureCube != nullptr )
		{
			m_dummyColorProjectionTextureCube->destroy();
			m_dummyColorProjectionTextureCube.reset();
		}

		if ( m_dummyColorProjectionTexture2D != nullptr )
		{
			m_dummyColorProjectionTexture2D->destroy();
			m_dummyColorProjectionTexture2D.reset();
		}

		if ( m_dummyShadowTextureCube != nullptr )
		{
			m_dummyShadowTextureCube->destroy();
			m_dummyShadowTextureCube.reset();
		}

		if ( m_dummyShadowTexture2D != nullptr )
		{
			m_dummyShadowTexture2D->destroy();
			m_dummyShadowTexture2D.reset();
		}

		m_defaultTextureCubemap.reset();
	}

	bool
	Renderer::onInitialize () noexcept
	{
		m_windowLess = m_primaryServices.arguments().isSwitchPresent("-W", "--window-less");

		/* NOTE: Reserve capacity for cache maps to avoid rehashing during initialization.
		 * Typical usage patterns: ~30-50 samplers, ~50-100 pipelines/programs. */
		m_samplers.reserve(50);
		m_graphicsPipelines.reserve(100);
		m_programs.reserve(100);

		/* NOTE: Initialize the optional frame rate limiter.
		 * 0 = disabled, otherwise the target FPS (e.g., 60, 120, 144). */
		m_frameRateLimit = m_primaryServices.settings().getOrSetDefault< uint32_t >(VideoFrameRateLimitKey, DefaultVideoFrameRateLimit);

		if ( m_frameRateLimit > 0 )
		{
			m_frameDuration = std::chrono::nanoseconds(1'000'000'000 / m_frameRateLimit);

			TraceInfo{ClassId} << "Frame rate limiter enabled: " << m_frameRateLimit << " FPS (frame duration: " << m_frameDuration.count() / 1'000'000.0 << " ms)";
		}

		m_rayTracingSettingEnabled = m_primaryServices.settings().getOrSetDefault< bool >(GraphicsRayTracingEnabledKey, DefaultGraphicsRayTracingEnabled);
		m_shadowMapsEnabled = m_primaryServices.settings().getOrSetDefault< bool >(GraphicsShadowMappingEnabledKey, DefaultGraphicsShadowMappingEnabled);
		m_MDIEnabled = m_primaryServices.settings().getOrSetDefault< bool >(GraphicsMDIEnabledKey, DefaultGraphicsMDIEnabled);

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

		/* NOTE: Acquire a fixed graphics queue for rendering.
		 * Using the same queue for ALL frames ensures that binary semaphores
		 * (shared across frame scopes, e.g. shadow maps) are properly serialized.
		 * With round-robin queue selection, concurrent frame scopes could use
		 * different queues, causing undefined behavior on shared binary semaphores. */
		m_graphicsQueue = m_device->getGraphicsQueue(QueuePriority::High);

		if ( m_graphicsQueue == nullptr )
		{
			Tracer::fatal(ClassId, "Unable to acquire a graphics queue for rendering!");

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
			const auto viewDistance = m_primaryServices.settings().getOrSetDefault< float >(GraphicsViewDistanceKey, DefaultGraphicsViewDistance);

			/* NOTE: Windowless mode uses single-sample only. renderOffscreenFrame() has a single
			 * render pass, so the overlay pipeline (created for single-sample) must match.
			 * TODO: Add a two-pass structure (scene MSAA -> resolve -> overlay single-sample)
			 * to renderOffscreenFrame() to support MSAA for quality screenshots. */
			constexpr uint32_t sampleCount = 1;
			/*auto sampleCount = m_primaryServices.settings().getOrSetDefault< uint32_t >(VideoFramebufferSamplesKey, DefaultVideoFramebufferSamples);

			if ( sampleCount > 1 )
			{
				sampleCount = m_device->checkMultisampleCount(sampleCount);
			}*/

			m_windowLessView = std::make_shared< RenderTarget::View< ViewMatrices2DUBO > >(
				"WindowLessView",
				m_window.state().windowWidth,
				m_window.state().windowHeight,
				FramebufferPrecisions{8, 8, 8, 8, 24, 0, sampleCount},
				viewDistance,
				false
			);

			if ( !m_windowLessView->createRenderTarget(*this) )
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
			m_swapChain = std::make_shared< SwapChain >(*this, m_primaryServices.settings(), m_vulkanInstance.showInformation());
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
			auto sizes = std::vector< VkDescriptorPoolSize >{
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
				//{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV, 0},
				/* NOTE:  */
				//{VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM, 0},
				/* NOTE:  */
				//{VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM, 0},
				/* NOTE:  */
				//{VK_DESCRIPTOR_TYPE_MUTABLE_EXT, 0}
			};

			if ( m_device->rayTracingEnabled() )
			{
				sizes.emplace_back(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 16);
			}

			m_descriptorPool = std::make_shared< DescriptorPool >(m_device, sizes, 4096, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
			m_descriptorPool->setIdentifier(ClassId, "Main", "DescriptorPool");

			if ( !m_descriptorPool->createOnHardware() )
			{
				Tracer::fatal(ClassId, "Unable to create the descriptor pool!");

				return false;
			}
		}

		/* Create the RT descriptor set for ray query shaders. */
		if ( m_device->rayTracingEnabled() && !this->createRTDescriptorSet() )
		{
			Tracer::warning(ClassId, "Unable to create RT descriptor set. Ray tracing reflections will be unavailable.");
		}

		this->registerToConsole();

		/* Create the grab pass texture (initially disabled, but pre-allocated). */
		if ( m_swapChain != nullptr )
		{
			const auto & swapChainCreateInfo = m_swapChain->createInfo();

			m_grabPass = std::make_unique< GrabPass >();

			if ( m_grabPass->create(*this, swapChainCreateInfo.imageExtent.width, swapChainCreateInfo.imageExtent.height, swapChainCreateInfo.imageFormat, m_swapChain->depthStencilFormat()) )
			{
				if ( m_bindlessTextureManager.usable() )
				{
					static_cast< void >(m_bindlessTextureManager.updateTexture2D(BindlessTextureManager::GrabPassSlot, *m_grabPass));

					if ( m_grabPass->hasDepth() )
					{
						static_cast< void >(m_bindlessTextureManager.updateTexture2DFromDescriptorInfo(
							BindlessTextureManager::GrabPassDepthSlot,
							m_grabPass->depthDescriptorInfo()));
					}
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to create the grab pass texture !";

				m_grabPass.reset();
			}
		}

		/* Configure the post-processor now that the descriptor pool and swap chain are ready.
		 * At this point no scene is loaded yet, so requirements are all false (passthrough). */
		if ( m_swapChain != nullptr && m_postProcessor.usable() )
		{
			if ( !m_postProcessor.configure(m_swapChain, false, false, false, false) )
			{
				TraceError{ClassId} << "Unable to configure the post-processor !";
			}
		}

		/* Initialize Multi-Draw Indirect batch builder if enabled and device supports it. */
		if ( m_MDIEnabled && m_device != nullptr )
		{
			m_MDIBatchBuilder = std::make_unique< MDI::BatchBuilder >(m_device, this->framesInFlight());

			if ( !m_MDIBatchBuilder->createResources() )
			{
				TraceWarning{ClassId} << "MDI batch builder initialization failed. Falling back to individual draws.";

				m_MDIBatchBuilder.reset();
				m_MDIEnabled = false;
			}
		}

		return true;
	}

	void
	Renderer::requestShutdown () noexcept
	{
		TraceInfo{ClassId} << "Graceful shutdown requested. Waiting for frames in-flight to complete...";

		m_shutdownRequested = true;

		/* Wait for all frames in-flight to complete their GPU work. */
		for ( auto & frameScope : m_rendererFrameScope )
		{
			if ( frameScope.inFlightFence() != nullptr )
			{
				[[maybe_unused]] const auto result = frameScope.inFlightFence()->wait(m_timeout);
			}
		}

		TraceInfo{ClassId} << "All frames in-flight have completed.";
	}

	bool
	Renderer::onTerminate () noexcept
	{
		/* NOTE: Request shutdown if not already done to ensure graceful termination. */
		if ( !m_shutdownRequested )
		{
			this->requestShutdown();
		}

		/* NOTE: Final device idle to ensure all GPU work is complete. */
		m_device->waitIdle("Renderer::onTerminate()");

		/* Release MDI resources before other Vulkan objects are destroyed. */
		m_MDIBatchBuilder.reset();

		size_t error = 0;

		/* NOTE: Stacked resources on the runtime. */
		{
			for ( const auto & sampler: m_samplers | std::views::values )
			{
				sampler->destroyFromHardware();
			}

			m_samplers.clear();

			for ( const auto & pipeline: m_graphicsPipelines | std::views::values )
			{
				pipeline->destroyFromHardware();
			}

			m_graphicsPipelines.clear();

			/* NOTE: Clear the program cache to release shared pointers before Vulkan resources are destroyed. */
			m_programs.clear();
		}

		/* NOTE: Release default resources before Vulkan resources are destroyed.
		 * These textures have VMA allocations that must be freed before the device is destroyed. */
		this->clearDefaultResources();

		m_rtDescriptorSets.clear();
		m_rtDescriptorSetLayout.reset();
		m_descriptorPool.reset();

		this->destroyRenderingSystem();

		m_swapChain.reset();
		m_sceneTarget.reset();
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

	std::shared_ptr< RenderTarget::Abstract >
	Renderer::mainRenderTarget () const noexcept
	{
		if ( m_windowLess )
		{
			return m_windowLessView;
		}

		return m_swapChain;
	}

	std::shared_ptr< Image >
	Renderer::currentSwapChainColorImage () const noexcept
	{
		if ( m_swapChain != nullptr )
		{
			return m_swapChain->currentColorImage();
		}

		return nullptr;
	}

	VkFormat
	Renderer::swapChainColorFormat () const noexcept
	{
		if ( m_swapChain == nullptr )
		{
			return VK_FORMAT_UNDEFINED;
		}

		return m_swapChain->createInfo().imageFormat;
	}

	VkFormat
	Renderer::swapChainDepthStencilFormat () const noexcept
	{
		if ( m_swapChain == nullptr )
		{
			return VK_FORMAT_UNDEFINED;
		}

		return m_swapChain->depthStencilFormat();
	}

	std::shared_ptr< Vulkan::Image >
	Renderer::currentSwapChainDepthStencilImage () const noexcept
	{
		if ( m_swapChain != nullptr )
		{
			return m_swapChain->currentDepthStencilImage();
		}

		return nullptr;
	}

	std::shared_ptr< Vulkan::Image >
	Renderer::currentSceneColorImage () const noexcept
	{
		if ( m_sceneTarget != nullptr )
		{
			return m_sceneTarget->colorImage();
		}

		return this->currentSwapChainColorImage();
	}

	std::shared_ptr< Vulkan::Image >
	Renderer::currentSceneDepthImage () const noexcept
	{
		if ( m_sceneTarget != nullptr )
		{
			return m_sceneTarget->depthStencilImage();
		}

		return this->currentSwapChainDepthStencilImage();
	}

	std::shared_ptr< Vulkan::Image >
	Renderer::currentSceneNormalsImage () const noexcept
	{
		if ( m_sceneTarget != nullptr )
		{
			return m_sceneTarget->normalsImage();
		}

		return nullptr;
	}

	std::shared_ptr< Vulkan::Image >
	Renderer::currentSceneMaterialPropertiesImage () const noexcept
	{
		if ( m_sceneTarget != nullptr )
		{
			return m_sceneTarget->materialPropertiesImage();
		}

		return nullptr;
	}

	bool
	Renderer::recreateSceneTarget () noexcept
	{
		if ( m_sceneTarget != nullptr )
		{
			m_sceneTarget->destroyRenderTarget();
			m_sceneTarget.reset();
		}

		if ( !m_postProcessor.isEnabled() && !m_windowLess )
		{
			return true;
		}

		/* Determine color format: float16 for HDR, swapchain format otherwise. */
		const auto colorFormat = m_postProcessor.cachedRequiresHDR()
			? VK_FORMAT_R16G16B16A16_SFLOAT
			: this->swapChainColorFormat();

		const auto depthFormat = this->swapChainDepthStencilFormat();

		uint32_t width;
		uint32_t height;

		if ( m_swapChain != nullptr )
		{
			const auto & ext = m_swapChain->extent();
			width = ext.width;
			height = ext.height;
		}
		else
		{
			width = m_window.state().windowWidth;
			height = m_window.state().windowHeight;
		}

		/* Material properties require normals (same effects need both, and the shader generator
		 * uses colorAttachmentCount to detect MRT layout: normals is always before materialProperties). */
		const auto needsNormals = m_postProcessor.cachedRequiresNormals() || m_postProcessor.cachedRequiresMaterialProperties();

		/* Normals buffer format for MRT: view-space normals for SSAO and SSR.
		 * Only allocated when the effect chain actually needs normals. */
		const auto normalsFormat = needsNormals
			? VK_FORMAT_R16G16B16A16_SFLOAT
			: VK_FORMAT_UNDEFINED;

		/* Material properties buffer format for MRT: per-pixel surface properties for post-process modulation.
		 * Only allocated when the effect chain actually needs material properties. */
		const auto materialPropertiesFormat = m_postProcessor.cachedRequiresMaterialProperties()
			? VK_FORMAT_R8G8B8A8_UNORM
			: VK_FORMAT_UNDEFINED;

		m_sceneTarget = std::make_shared< SceneRenderTarget >(
			"SceneRenderTarget",
			width, height,
			colorFormat, normalsFormat, materialPropertiesFormat, depthFormat,
			m_primaryServices.settings().getOrSetDefault< float >(GraphicsViewDistanceKey, DefaultGraphicsViewDistance)
		);

		if ( !m_sceneTarget->createRenderTarget(*this) )
		{
			TraceError{ClassId} << "Unable to create the scene render target!";

			m_sceneTarget.reset();

			return false;
		}

		/* Share the main render target's view matrices (camera UBO) with the scene target.
		 * The scene target renders the same camera view, avoiding a separate UBO. */
		const auto mainTarget = this->mainRenderTarget();

		if ( mainTarget != nullptr )
		{
			m_sceneTarget->setSourceViewMatrices(mainTarget->viewMatrices());
		}

		TraceSuccess{ClassId} << "Scene render target created (" << width << "x" << height << ", format: " << (m_postProcessor.cachedRequiresHDR() ? "R16G16B16A16_SFLOAT" : "swapchain") << ").";

		return true;
	}

	std::shared_ptr< Sampler >
	Renderer::getSampler (std::string_view identifier, const std::function< void (Settings & settings, VkSamplerCreateInfo &) > & setupCreateInfo) noexcept
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

		std::string id{identifier};

		auto sampler = std::make_shared< Sampler >(m_device, createInfo);
		sampler->setIdentifier(ClassId, id.c_str(), "Sampler");

		if ( !sampler->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create a sampler!");

			return nullptr;
		}

		const auto [samplerPair, success] = m_samplers.emplace(std::move(id), sampler);

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
	Renderer::finalizeGraphicsPipeline (const Framebuffer & framebuffer, const Program & program, std::shared_ptr< GraphicsPipeline > & graphicsPipeline) noexcept
	{
		const auto & renderPass = framebuffer.renderPass();
		auto hash = graphicsPipeline->getHash(*renderPass);

		/* Include the pipeline layout handle in the hash to prevent sharing
		 * pipelines across programs with different descriptor set layouts. */
		if ( const auto & pipelineLayout = program.pipelineLayout(); pipelineLayout != nullptr )
		{
			hash ^= reinterpret_cast< uintptr_t >(pipelineLayout->handle()) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}

		if ( const auto pipelineIt = m_graphicsPipelines.find(hash); pipelineIt != m_graphicsPipelines.cend() )
		{
			graphicsPipeline = pipelineIt->second;

			m_pipelineReusedCount++;

			return true;
		}

		if ( !graphicsPipeline->finalize(renderPass, program.pipelineLayout(), program.useTesselation(), m_vulkanInstance.isDynamicStateExtensionEnabled()) )
		{
			return false;
		}

		m_pipelineBuiltCount++;

		return m_graphicsPipelines.emplace(hash, graphicsPipeline).second;
	}

	[[nodiscard]]
	bool
	Renderer::finalizeGraphicsPipeline (const RenderTarget::Abstract & renderTarget, const Program & program, std::shared_ptr< GraphicsPipeline > & graphicsPipeline) noexcept
	{
		return this->finalizeGraphicsPipeline(*renderTarget.framebuffer(), program, graphicsPipeline);
	}

	const Framebuffer *
	Renderer::overlayFramebuffer () const noexcept
	{
		if ( m_windowLess )
		{
			return m_windowLessView != nullptr ? m_windowLessView->framebuffer() : nullptr;
		}

		return m_swapChain != nullptr ? m_swapChain->postProcessFramebuffer() : nullptr;
	}

	std::shared_ptr< Saphir::Program >
	Renderer::findCachedProgram (size_t programKey) const noexcept
	{
		if ( const auto programIt = m_programs.find(programKey); programIt != m_programs.cend() )
		{
			return programIt->second;
		}

		return nullptr;
	}

	bool
	Renderer::cacheProgram (size_t programKey, const std::shared_ptr< Program > & program) noexcept
	{
		if ( program == nullptr )
		{
			return false;
		}

		const auto [it, inserted] = m_programs.emplace(programKey, program);

		if ( inserted )
		{
			m_programBuiltCount++;
		}

		return inserted;
	}

	void
	Renderer::applyFrameRateLimit () const noexcept
	{
		if ( !this->isSoftwareFrameLimiterEnabled() )
		{
			return;
		}

		const auto elapsed = std::chrono::high_resolution_clock::now() - m_frameStartTime;

		if ( elapsed >= m_frameDuration )
		{
			return;
		}

		/* Sleep for most of the remaining time (leave 1ms for busy-wait precision). */
		if ( const auto remainingTime = m_frameDuration - elapsed; remainingTime > std::chrono::milliseconds(1) )
		{
			std::this_thread::sleep_for(remainingTime - std::chrono::milliseconds(1));
		}

		/* Busy-wait for the final portion to achieve precise timing.
		 * NOTE: yield() hints the OS to schedule other threads, reducing CPU waste.
		 * If frame timing becomes inconsistent, comment out the yield(). */
		while ( std::chrono::high_resolution_clock::now() - m_frameStartTime < m_frameDuration )
		{
			std::this_thread::yield();
		}
	}

	void
	Renderer::renderOffscreenFrame (const std::shared_ptr< Scenes::Scene > & scene, const Overlay::Manager & overlayManager) noexcept
	{
		/* NOTE: Record frame start time for the optional frame limiter. */
		if ( this->isSoftwareFrameLimiterEnabled() )
		{
			m_frameStartTime = std::chrono::high_resolution_clock::now();
		}

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
				this->renderShadowMaps(currentFrameScope, *scene, m_graphicsQueue);
			}

			if ( this->isRenderToTexturesEnabled() )
			{
				this->renderRenderToTextures(currentFrameScope, *scene, m_graphicsQueue);
			}

			//this->renderViews(currentFrameScope, *scene, m_graphicsQueue);

			/* Forward any unconsumed primary semaphores (shadow maps) to secondary,
			 * so the final submit correctly waits on them. */
			currentFrameScope.promotePrimaryToSecondary();
		}

		/* Then we need the command buffer linked to this image by its index. */
		const auto commandBuffer = currentFrameScope.getCommandBuffer(m_windowLessView.get());

		if ( !commandBuffer->begin() )
		{
			return;
		}

		commandBuffer->beginRenderPass(*m_windowLessView->framebuffer(), m_windowLessView->renderArea(), m_swapChainClearColors, VK_SUBPASS_CONTENTS_INLINE);

		/* First, render the scene. */
		if ( scene != nullptr )
		{
			if ( scene->prepareRender(m_windowLessView) )
			{
				scene->renderOpaque(m_windowLessView, *commandBuffer);
				scene->renderTranslucent(m_windowLessView, *commandBuffer);
				scene->renderTranslucentGB(m_windowLessView, *commandBuffer);

				if ( m_TBNSpaceRenderingEnabled )
				{
					scene->renderTBNSpace(m_windowLessView, *commandBuffer);
				}
			}
		}

		/* Then render the overlay system over the 3D-rendered scene. */
		overlayManager.render(m_windowLessView, *commandBuffer);

		commandBuffer->endRenderPass();

		if ( !commandBuffer->end() )
		{
			return;
		}

		const StaticVector< VkPipelineStageFlags, 16 > waitStages(
			currentFrameScope.secondarySemaphores().size(),
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		);

		const auto submitResult = m_graphicsQueue->submit(
			*commandBuffer,
			SynchInfo{}
				.waits(currentFrameScope.secondarySemaphores(), waitStages)
				.withFence(currentFrameScope.inFlightFence()->handle())
		);

		if ( !submitResult )
		{
			TraceError{ClassId} << "Unable to submit command buffer for render target '" << m_windowLessView->id() << "' !";

			return;
		}

		this->applyFrameRateLimit();
	}

	void
	Renderer::renderFrame (const std::shared_ptr< Scenes::Scene > & scene, const Overlay::Manager & overlayManager) noexcept
	{
		/* NOTE: Record frame start time for the optional frame limiter. */
		if ( this->isSoftwareFrameLimiterEnabled() )
		{
			m_frameStartTime = std::chrono::high_resolution_clock::now();
		}

		/* 1. If the swap-chain was marked degraded, we discard the next frame until we get back a valid swap-chain. */
		if ( this->isSwapChainDegraded() )
		{
			/* NOTE: Let's try to recreate every new frame and Core decide what to do with the renderer. */
			if ( !this->recreateRenderingSubSystem(false, false) )
			{
				Tracer::fatal(ClassId, "Unable to refresh the swap-chain!");
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

		/* Clean up deferred resources once all in-flight frames have completed.
		 * After framesInFlight() fence waits, every command buffer that could
		 * reference the retired resources has finished execution. */
		if ( !m_retiredSceneTargets.empty() )
		{
			if ( m_retiredFrameCountdown > 0 )
			{
				--m_retiredFrameCountdown;
			}

			if ( m_retiredFrameCountdown == 0 )
			{
				for ( const auto & target : m_retiredSceneTargets )
				{
					target->destroyRenderTarget();
				}

				m_retiredSceneTargets.clear();
			}
		}

		/* 3. Get the new frame to render to. */
		const auto frameIndexOpt = m_swapChain->acquireNextImage(currentFrameScope.imageAvailableSemaphore(), m_timeout);

		if ( !frameIndexOpt )
		{
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
				this->renderShadowMaps(currentFrameScope, *scene, m_graphicsQueue);
			}

			if ( this->isRenderToTexturesEnabled() )
			{
				this->renderRenderToTextures(currentFrameScope, *scene, m_graphicsQueue);
			}

			//this->renderViews(currentFrameScope, *scene, m_graphicsQueue);

			/* Forward any unconsumed primary semaphores (shadow maps) to secondary,
			 * so the final submit correctly waits on them. */
			currentFrameScope.promotePrimaryToSecondary();
		}

		/* Then we need the command buffer linked to this image by its index. */
		const auto commandBuffer = currentFrameScope.getCommandBuffer(m_swapChain.get());

		if ( !commandBuffer->begin() )
		{
			return;
		}

		/* Lazy creation/destruction of the internal scene target based on post-processor state.
		 * When PP is enabled: create the HDR scene target and reconfigure PP with it.
		 * When PP is disabled: destroy the scene target and return to direct swapchain rendering.
		 * NOTE: Defer creation until the scene's PostProcessStack is ready. The logic thread
		 * may still be building the scene when the render thread first reaches this point.
		 * Creating the scene target without the stack leads to wrong formats (no HDR/depth/normals). */
		const auto * stack = scene != nullptr ? scene->postProcessStack() : nullptr;

		if ( m_postProcessor.isEnabled() && m_sceneTarget == nullptr && stack != nullptr )
		{
			/* Pre-update cached requirements so recreateSceneTarget() picks up
			 * correct formats (HDR color, normals MRT) before creating the target. */
			m_postProcessor.updateCachedRequirements(
				stack->requiresHDR(),
				stack->requiresDepth(),
				stack->requiresNormals(),
				stack->requiresMaterialProperties());

			if ( this->recreateSceneTarget() )
			{
				if ( !m_postProcessor.configure(
					std::static_pointer_cast< RenderTarget::Abstract >(m_sceneTarget),
					stack->requiresHDR(),
					stack->requiresDepth(),
					stack->requiresNormals(),
					stack->requiresMaterialProperties()) )
				{
					TraceError{ClassId} << "Unable to reconfigure the post-processor with the scene target!";
				}
			}
		}
		else if ( !m_postProcessor.isEnabled() && m_sceneTarget != nullptr )
		{
			/* Defer destruction: move the scene target to a retirement queue so that
			 * in-flight command buffers finish referencing its resources (framebuffer,
			 * render pass, images) before they are destroyed. */
			m_retiredSceneTargets.emplace_back(std::move(m_sceneTarget));
			m_retiredFrameCountdown = static_cast< uint32_t >(m_rendererFrameScope.size());
		}

		/* Dispatch to the appropriate rendering strategy. */
		if ( m_sceneTarget != nullptr && m_postProcessor.isEnabled() )
		{
			this->renderFrameWithInternal(scene, overlayManager, currentFrameScope, commandBuffer);
		}
		else
		{
			this->renderFrameDirect(scene, overlayManager, currentFrameScope, commandBuffer);
		}

		if ( !commandBuffer->end() )
		{
			return;
		}

		/* 6. Submit the work on the GPU and present. */
		{
			/* Build wait stages: FRAGMENT_SHADER for render-to-textures, COLOR_ATTACHMENT_OUTPUT for swap-chain */
			StaticVector< VkPipelineStageFlags, 16 > waitStages(
				currentFrameScope.secondarySemaphores().size(),
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			);

			/* Add swap-chain image available semaphore and wait at COLOR_ATTACHMENT_OUTPUT stage */
			currentFrameScope.secondarySemaphores().emplace_back(currentFrameScope.imageAvailableSemaphore()->handle());
			waitStages.emplace_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

			auto renderFinishedSemaphoreHandle = currentFrameScope.renderFinishedSemaphore()->handle();

			if ( !m_graphicsQueue->submit(*commandBuffer, SynchInfo{}
					.waits(currentFrameScope.secondarySemaphores(), waitStages)
					.signals({&renderFinishedSemaphoreHandle, 1})
					.withFence(currentFrameScope.inFlightFence()->handle())
				) )
			{
				return;
			}

			m_swapChain->present(frameIndex, m_graphicsQueue, renderFinishedSemaphoreHandle);
		}

		m_currentFrameIndex = (m_currentFrameIndex + 1) % m_rendererFrameScope.size();

		this->applyFrameRateLimit();
	}

	void
	Renderer::renderFrameDirect (const std::shared_ptr< Scenes::Scene > & scene, const Overlay::Manager & overlayManager, RendererFrameScope & /*currentFrameScope*/, const std::shared_ptr< CommandBuffer > & commandBuffer) noexcept
	{
		auto * const scenePtr = scene.get();

		/* Prepare scene render lists once (frustum culling, Z-sorting). */
		const bool sceneHasContent = scenePtr != nullptr && scenePtr->prepareRender(m_swapChain);

		/* Record deferred TLAS build into the render command buffer (no CPU stall). */
		if ( scenePtr != nullptr )
		{
			scenePtr->recordTLASBuild(commandBuffer->handle());
		}

		/* Render pass 1: Scene rendering (clears buffers). */
		commandBuffer->beginRenderPass(*m_swapChain->framebuffer(), m_swapChain->renderArea(), m_swapChainClearColors, VK_SUBPASS_CONTENTS_INLINE);

		/* Render opaque and translucent objects in the MSAA render pass. */
		if ( sceneHasContent )
		{
			scenePtr->renderOpaque(m_swapChain, *commandBuffer);
			scenePtr->renderTranslucent(m_swapChain, *commandBuffer);

			if ( m_TBNSpaceRenderingEnabled )
			{
				scenePtr->renderTBNSpace(m_swapChain, *commandBuffer);
			}
		}

		commandBuffer->endRenderPass();

		/* Grab pass: capture the resolved scene into a sample-able texture (same frame). */
		if ( m_grabPassEnabled && m_grabPass != nullptr && m_grabPass->isCreated() )
		{
			const auto * srcDepth = m_grabPass->hasDepth() ? m_swapChain->currentDepthStencilImage().get() : nullptr;

			m_grabPass->recordBlit(*commandBuffer, *m_swapChain->currentColorImage(), srcDepth);
		}

		/* Render pass 2: Post-processing and overlay (preserves scene content).
		 * TranslucentGB objects render here, after the grab pass blit, so they
		 * can sample the captured scene through the grab pass texture. */
		commandBuffer->beginRenderPass(*m_swapChain->postProcessFramebuffer(), m_swapChain->renderArea(), m_swapChainClearColors, VK_SUBPASS_CONTENTS_INLINE);

		/* Render translucent grab-pass objects (refraction, etc.). */
		if ( sceneHasContent && scenePtr->hasTranslucentGBObjects() )
		{
			scenePtr->renderTranslucentGB(m_swapChain, *commandBuffer);
		}

		/* Post-processing: end RP2, blit the complete scene, restart RP2, draw fullscreen quad. */
		if ( m_postProcessor.isEnabled() )
		{
			commandBuffer->endRenderPass();

			m_postProcessor.recordBlit(*commandBuffer);

			/* Set the current TLAS and update RT descriptor set for post-process effects.
			 * Capture the read state index so post-process effects (RTR) use view matrices
			 * consistent with the depth buffer rendered this frame. */
			if ( scenePtr != nullptr )
			{
				m_currentTLAS = scenePtr->TLAS();
				m_currentReadStateIndex = scenePtr->preparedReadStateIndex();

				if ( m_currentTLAS != nullptr )
				{
					this->updateRTDescriptorSet(scenePtr->sceneMetaData(), scenePtr->lightSet());
				}
			}
			else
			{
				m_currentTLAS = nullptr;
			}

			/* Process scene effects (multi-pass). */
			if ( scenePtr != nullptr && scenePtr->hasPostProcessStack() )
			{
				m_postProcessor.executeIndirectPostProcessEffects(*commandBuffer, *scenePtr->postProcessStack(), &scenePtr->lightSet());
			}

			commandBuffer->beginRenderPass(*m_swapChain->postProcessFramebuffer(), m_swapChain->renderArea(), m_swapChainClearColors, VK_SUBPASS_CONTENTS_INLINE);

			/* Process camera lens effects (single-pass). */
			const auto * camera = (scenePtr != nullptr) ? scenePtr->activeCamera() : nullptr;
			const auto & lensEffects = (camera != nullptr) ? camera->lensEffects() : EmptyLensEffects;

			m_postProcessor.executeDirectPostProcessEffects(*commandBuffer, lensEffects);
		}

		/* Render the overlay system over the scene. */
		overlayManager.render(m_swapChain, *commandBuffer);

		commandBuffer->endRenderPass();
	}

	void
	Renderer::renderFrameWithInternal (const std::shared_ptr< Scenes::Scene > & scene, const Overlay::Manager & overlayManager, RendererFrameScope & /*currentFrameScope*/, const std::shared_ptr< CommandBuffer > & commandBuffer) noexcept
	{
		auto * const scenePtr = scene.get();

		/* Prepare scene render lists once (frustum culling, Z-sorting)
		 * against the internal scene target (HDR float16 framebuffer). */
		const bool sceneHasContent = scenePtr != nullptr && scenePtr->prepareRender(m_sceneTarget);

		/* Record deferred TLAS build into the render command buffer (no CPU stall). */
		if ( scenePtr != nullptr )
		{
			scenePtr->recordTLASBuild(commandBuffer->handle());
		}

		/* RP-scene (internal target, CLEAR): Render opaque and translucent objects.
		 * Clear values must match the actual attachment layout which depends on which
		 * MRT attachments are present. m_clearColors layout: [0]=color, [1]=normals,
		 * [2]=materialProperties, [3]=depth. Build the matching subset for beginRenderPass. */
		const bool sceneTargetHasNormals = m_sceneTarget->normalsFormat() != VK_FORMAT_UNDEFINED;
		const bool sceneTargetHasMaterialProperties = m_sceneTarget->materialPropertiesFormat() != VK_FORMAT_UNDEFINED;

		if ( sceneTargetHasNormals && sceneTargetHasMaterialProperties )
		{
			commandBuffer->beginRenderPass(*m_sceneTarget->framebuffer(), m_sceneTarget->renderArea(), m_clearColors, VK_SUBPASS_CONTENTS_INLINE);
		}
		else if ( sceneTargetHasNormals )
		{
			const std::array< VkClearValue, 3 > cv{m_clearColors[0], m_clearColors[1], m_clearColors[3]};
			commandBuffer->beginRenderPass(*m_sceneTarget->framebuffer(), m_sceneTarget->renderArea(), cv, VK_SUBPASS_CONTENTS_INLINE);
		}
		else if ( sceneTargetHasMaterialProperties )
		{
			const std::array< VkClearValue, 3 > cv{m_clearColors[0], m_clearColors[2], m_clearColors[3]};
			commandBuffer->beginRenderPass(*m_sceneTarget->framebuffer(), m_sceneTarget->renderArea(), cv, VK_SUBPASS_CONTENTS_INLINE);
		}
		else
		{
			commandBuffer->beginRenderPass(*m_sceneTarget->framebuffer(), m_sceneTarget->renderArea(), m_swapChainClearColors, VK_SUBPASS_CONTENTS_INLINE);
		}

		if ( sceneHasContent )
		{
			scenePtr->renderOpaque(m_sceneTarget, *commandBuffer);
			scenePtr->renderTranslucent(m_sceneTarget, *commandBuffer);

			if ( m_TBNSpaceRenderingEnabled )
			{
				scenePtr->renderTBNSpace(m_sceneTarget, *commandBuffer);
			}
		}

		commandBuffer->endRenderPass();

		/* Grab pass: capture the scene from the internal target for TranslucentGB refraction.
		 * The internal target's color image is in COLOR_ATTACHMENT_OPTIMAL (matching GrabPass expectations). */
		if ( m_grabPassEnabled && m_grabPass != nullptr && m_grabPass->isCreated() )
		{
			const auto * srcDepth = m_grabPass->hasDepth() ? m_sceneTarget->depthStencilImage().get() : nullptr;

			m_grabPass->recordBlit(*commandBuffer, *m_sceneTarget->colorImage(), srcDepth);
		}

		/* RP-scene-load (internal target, LOAD): TranslucentGB objects render after the grab pass
		 * so they can sample the captured scene for refraction effects. */
		if ( sceneHasContent && scenePtr->hasTranslucentGBObjects() )
		{
			if ( sceneTargetHasNormals && sceneTargetHasMaterialProperties )
			{
				commandBuffer->beginRenderPass(*m_sceneTarget->postProcessFramebuffer(), m_sceneTarget->renderArea(), m_clearColors, VK_SUBPASS_CONTENTS_INLINE);
			}
			else if ( sceneTargetHasNormals )
			{
				const std::array< VkClearValue, 3 > cv{m_clearColors[0], m_clearColors[1], m_clearColors[3]};
				commandBuffer->beginRenderPass(*m_sceneTarget->postProcessFramebuffer(), m_sceneTarget->renderArea(), cv, VK_SUBPASS_CONTENTS_INLINE);
			}
			else if ( sceneTargetHasMaterialProperties )
			{
				const std::array< VkClearValue, 3 > cv{m_clearColors[0], m_clearColors[2], m_clearColors[3]};
				commandBuffer->beginRenderPass(*m_sceneTarget->postProcessFramebuffer(), m_sceneTarget->renderArea(), cv, VK_SUBPASS_CONTENTS_INLINE);
			}
			else
			{
				commandBuffer->beginRenderPass(*m_sceneTarget->postProcessFramebuffer(), m_sceneTarget->renderArea(), m_swapChainClearColors, VK_SUBPASS_CONTENTS_INLINE);
			}

			scenePtr->renderTranslucentGB(m_sceneTarget, *commandBuffer);

			commandBuffer->endRenderPass();
		}

		/* Post-processor: blit the complete scene (with TranslucentGB) into PP's grab pass.
		 * The internal target's color is in COLOR_ATTACHMENT_OPTIMAL. */
		m_postProcessor.recordBlit(*commandBuffer);

		/* Set the current TLAS and update RT descriptor set for post-process effects.
		 * Capture the read state index so post-process effects (RTR) use view matrices
		 * consistent with the depth buffer rendered this frame. */
		if ( scenePtr != nullptr )
		{
			m_currentTLAS = scenePtr->TLAS();
			m_currentReadStateIndex = scenePtr->preparedReadStateIndex();

			if ( m_currentTLAS != nullptr )
			{
				this->updateRTDescriptorSet(scenePtr->sceneMetaData(), scenePtr->lightSet());
			}
		}
		else
		{
			m_currentTLAS = nullptr;
		}

		/* Process scene effects (multi-pass). */
		if ( scenePtr != nullptr && scenePtr->hasPostProcessStack() )
		{
			m_postProcessor.executeIndirectPostProcessEffects(*commandBuffer, *scenePtr->postProcessStack(), &scenePtr->lightSet());
		}

		/* Establish swapchain image layouts by running RP1 (CLEAR) with no draw calls.
		 * This transitions swapchain color from UNDEFINED to COLOR_ATTACHMENT_OPTIMAL
		 * and depth from UNDEFINED to DEPTH_STENCIL_ATTACHMENT_OPTIMAL. */
		commandBuffer->beginRenderPass(*m_swapChain->framebuffer(), m_swapChain->renderArea(), m_swapChainClearColors, VK_SUBPASS_CONTENTS_INLINE);
		commandBuffer->endRenderPass();

		/* RP-final (swapchain postProcess, LOAD): Draw the PP fullscreen quad + overlay.
		 * This transitions the swapchain color to PRESENT_SRC_KHR for presentation. */
		commandBuffer->beginRenderPass(*m_swapChain->postProcessFramebuffer(), m_swapChain->renderArea(), m_swapChainClearColors, VK_SUBPASS_CONTENTS_INLINE);

		/* Process camera lens effects (single-pass). */
		const auto * camera = (scenePtr != nullptr) ? scenePtr->activeCamera() : nullptr;
		const auto & lensEffects = (camera != nullptr) ? camera->lensEffects() : EmptyLensEffects;

		m_postProcessor.executeDirectPostProcessEffects(*commandBuffer, lensEffects);

		/* Render the overlay system over the post-processed scene. */
		overlayManager.render(m_swapChain, *commandBuffer);

		commandBuffer->endRenderPass();
	}

	void
	Renderer::renderShadowMaps (RendererFrameScope & currentFrameScope, Scenes::Scene & scene, const Queue * queue) const noexcept
	{
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

			commandBuffer->beginRenderPass(*shadowMap->framebuffer(), shadowMap->renderArea(), m_shadowMapClearValues, VK_SUBPASS_CONTENTS_INLINE);

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
	Renderer::renderRenderToTextures (RendererFrameScope & currentFrameScope, Scenes::Scene & scene, const Queue * queue) const noexcept
	{
		bool primaryConsumed = false;

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

			commandBuffer->beginRenderPass(*renderToTexture->framebuffer(), renderToTexture->renderArea(), m_swapChainClearColors, VK_SUBPASS_CONTENTS_INLINE);

			if ( scene.prepareRender(renderToTexture) )
			{
				scene.renderOpaque(renderToTexture, *commandBuffer);
				scene.renderTranslucent(renderToTexture, *commandBuffer);
				scene.renderTranslucentGB(renderToTexture, *commandBuffer);
			}

			commandBuffer->endRenderPass();

			if ( !commandBuffer->end() )
			{
				TraceError{ClassId} << "Unable to finish the command buffer for render target '" << renderToTexture->id() << "' !";

				return;
			}

			const auto signalSemaphoreHandle = renderToTexture->semaphore()->handle();

			bool submitted;

			/* Only the first render-to-texture waits on primary (shadow map) semaphores.
			 * Binary semaphores can only be waited on once per signal. */
			if ( !primaryConsumed && !currentFrameScope.primarySemaphores().empty() )
			{
				const auto & waitSemaphores = currentFrameScope.primarySemaphores();
				const StaticVector< VkPipelineStageFlags, 16 > waitStages(waitSemaphores.size(), VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

				submitted = queue->submit(
					*commandBuffer,
					SynchInfo{}
						.waits(waitSemaphores, waitStages)
						.signals({&signalSemaphoreHandle, 1})
				);

				primaryConsumed = true;
			}
			else
			{
				submitted = queue->submit(
					*commandBuffer,
					SynchInfo{}
						.signals({&signalSemaphoreHandle, 1})
				);
			}

			if ( !submitted )
			{
				TraceError{ClassId} << "Unable to submit command buffer for render target '" << renderToTexture->id() << "' !";

				return;
			}

			currentFrameScope.declareSemaphore(renderToTexture->semaphore(), false);
		});

		/* Clear primary after RTTs consumed them. */
		if ( primaryConsumed )
		{
			currentFrameScope.primarySemaphores().clear();
		}
	}

	void
	Renderer::renderViews (RendererFrameScope & /*currentFrameScope*/, Scenes::Scene & /*scene*/, const Queue * /*queue*/) const noexcept
	{

	}

	bool
	Renderer::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable->is(Window::getClassUID()) )
		{
			switch ( notificationCode )
			{
				/* NOTE: These two notifications invalidate the framebuffer content. */
				case Window::OSNotifiesFramebufferResized :
				case Window::OSRequestsToRescaleContentBy :
					if ( m_windowLess )
					{
						// TODO: Resize the windowless framebuffer to the new size!
					}
					else
					{
						/* NOTE: We declare the swap-chain degraded. */
						if ( m_swapChain != nullptr )
						{
							m_swapChain->setDegraded();
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
	Renderer::recreateRenderingSubSystem (bool withSurface, bool useNativeCode) noexcept
	{
		/* NOTE: Wait the device to finish all his work before destroying/recreating the swap-chain. */
		this->device()->waitIdle("Renderer::recreateSystem()");

		/* NOTE: Lock operation to wait a valid size from the OS. */
		m_window.waitValidWindowSize();

		/* NOTE: Query the surface properties again. */
		if ( !m_window.surface()->update(this->device()->physicalDevice()) )
		{
			Tracer::error(ClassId, "Unable to update the handle surface from a framebuffer resized!");

			return false;
		}

		/* NOTE: Recreate the swap-chain. */
		if ( withSurface )
		{
			if ( !m_swapChain->fullRecreate(useNativeCode) )
			{
				return false;
			}
		}
		else
		{
			if ( !m_swapChain->recreate() )
			{
				return false;
			}
		}

		/* Recreate the internal scene render target with new dimensions. */
		if ( m_sceneTarget != nullptr )
		{
			if ( !this->recreateSceneTarget() )
			{
				TraceError{ClassId} << "Unable to recreate the scene render target on resize!";
			}
		}

		/* Recreate the post-processor with the new dimensions.
		 * Reuses cached requirements — the PostProcessStack is resized by Scene's observer. */
		if ( m_postProcessor.usable() && m_postProcessor.isEnabled() )
		{
			const auto renderTarget = m_sceneTarget != nullptr
				? std::static_pointer_cast< RenderTarget::Abstract >(m_sceneTarget)
				: std::static_pointer_cast< RenderTarget::Abstract >(m_swapChain);

			if ( !m_postProcessor.configure(
				renderTarget,
				m_postProcessor.cachedRequiresHDR(),
				m_postProcessor.cachedRequiresDepth(),
				m_postProcessor.cachedRequiresNormals(),
				m_postProcessor.cachedRequiresMaterialProperties()) )
			{
				TraceError{ClassId} << "Unable to reconfigure the post-processor on resize!";
			}
		}

		/* Recreate the grab pass texture with the new swap-chain dimensions. */
		if ( m_grabPass != nullptr )
		{
			const auto & swapChainCreateInfo = m_swapChain->createInfo();

			if ( m_grabPass->recreate(*this, swapChainCreateInfo.imageExtent.width, swapChainCreateInfo.imageExtent.height, swapChainCreateInfo.imageFormat, m_swapChain->depthStencilFormat()) )
			{
				if ( m_bindlessTextureManager.usable() )
				{
					static_cast< void >(m_bindlessTextureManager.updateTexture2D(BindlessTextureManager::GrabPassSlot, *m_grabPass));

					if ( m_grabPass->hasDepth() )
					{
						static_cast< void >(m_bindlessTextureManager.updateTexture2DFromDescriptorInfo(
							BindlessTextureManager::GrabPassDepthSlot,
							m_grabPass->depthDescriptorInfo()));
					}
				}
			}
			else
			{
				TraceError{ClassId} << "Unable to recreate the grab pass texture !";
			}
		}

		this->notify(WindowContentRefreshed);

		return true;
	}

	bool
	Renderer::captureFramebuffer (std::array< Pixmap< uint8_t >, 3 > & result, bool keepAlpha, bool withDepthBuffer, bool withStencilBuffer, bool postProcess) noexcept
	{
		if ( m_swapChain == nullptr || !m_transferManager.usable() )
		{
			return false;
		}

		if ( !m_swapChain->capture(m_transferManager, 0, keepAlpha, withDepthBuffer, withStencilBuffer, result) )
		{
			return false;
		}

		if ( postProcess && result[0].isValid() )
		{
			result[0] = Processor< uint8_t >::swapChannels(result[0], false);
		}

		return true;
	}

	bool
	Renderer::createRTDescriptorSet () noexcept
	{
		/* Layout: binding 0 = TLAS, binding 1 = mesh metadata SSBO,
		 *         binding 2 = material data SSBO, binding 3 = light array SSBO. */
		m_rtDescriptorSetLayout = std::make_shared< DescriptorSetLayout >(m_device, "RTDescriptorSet");

		if ( !m_rtDescriptorSetLayout->declareAccelerationStructureKHR(0, VK_SHADER_STAGE_FRAGMENT_BIT) ||
			 !m_rtDescriptorSetLayout->declareStorageBuffer(1, VK_SHADER_STAGE_FRAGMENT_BIT) ||
			 !m_rtDescriptorSetLayout->declareStorageBuffer(2, VK_SHADER_STAGE_FRAGMENT_BIT) ||
			 !m_rtDescriptorSetLayout->declareStorageBuffer(3, VK_SHADER_STAGE_FRAGMENT_BIT) )
		{
			TraceError{ClassId} << "Unable to declare RT descriptor set layout bindings!";

			m_rtDescriptorSetLayout.reset();

			return false;
		}

		if ( !m_rtDescriptorSetLayout->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create RT descriptor set layout!";

			m_rtDescriptorSetLayout.reset();

			return false;
		}

		const auto frameCount = this->framesInFlight();

		m_rtDescriptorSets.resize(frameCount);

		for ( uint32_t i = 0; i < frameCount; ++i )
		{
			m_rtDescriptorSets[i] = std::make_unique< DescriptorSet >(m_descriptorPool, m_rtDescriptorSetLayout);

			if ( !m_rtDescriptorSets[i]->create() )
			{
				TraceError{ClassId} << "Unable to create RT descriptor set for frame " << i << "!";

				m_rtDescriptorSets.clear();
				m_rtDescriptorSetLayout.reset();

				return false;
			}
		}

		return true;
	}

	void
	Renderer::updateRTDescriptorSet (const Scenes::SceneMetaData & sceneMetaData, const Scenes::LightSet & lightSet) noexcept
	{
		if ( m_rtDescriptorSets.empty() )
		{
			return;
		}

		const auto * tlas = sceneMetaData.TLAS();

		if ( tlas == nullptr )
		{
			return;
		}

		/* Update only the current frame's descriptor set to avoid write-while-pending. */
		auto & descriptorSet = m_rtDescriptorSets[m_currentFrameIndex];

		/* Binding 0: TLAS. */
		static_cast< void >(descriptorSet->writeAccelerationStructure(0, tlas->handle()));

		/* Binding 1: Mesh metadata SSBO (per-frame to avoid CPU/GPU race). */
		const auto * meshSSBO = sceneMetaData.meshMetaDataSSBO(m_currentFrameIndex);

		if ( meshSSBO != nullptr )
		{
			const VkDescriptorBufferInfo meshInfo{meshSSBO->handle(), 0, meshSSBO->bytes()};
			static_cast< void >(descriptorSet->writeStorageBuffer(1, meshInfo));
		}

		/* Binding 2: Material data SSBO (per-frame to avoid CPU/GPU race). */
		const auto * materialSSBO = sceneMetaData.materialDataSSBO(m_currentFrameIndex);

		if ( materialSSBO != nullptr )
		{
			const VkDescriptorBufferInfo materialInfo{materialSSBO->handle(), 0, materialSSBO->bytes()};
			static_cast< void >(descriptorSet->writeStorageBuffer(2, materialInfo));
		}

		/* Binding 3: Light array SSBO. */
		const auto * lightSSBO = lightSet.rtLightBuffer();

		if ( lightSSBO != nullptr )
		{
			const VkDescriptorBufferInfo lightInfo{lightSSBO->handle(), 0, lightSSBO->bytes()};
			static_cast< void >(descriptorSet->writeStorageBuffer(3, lightInfo));
		}

		m_rtLightCount = lightSet.rtLightCount();
	}
}
