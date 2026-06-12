/*
 * src/Overlay/Manager.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Manager.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <iterator>
#include <ranges>

/* Third-party inclusions. */
#ifdef IMGUI_ENABLED
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#include "backends/imgui_impl_glfw.h"

#include "Vulkan/Instance.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/Queue.hpp"
#include "Vulkan/DescriptorPool.hpp"
#include "Vulkan/Framebuffer.hpp"
#endif

/* Local inclusions. */
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Renderer.hpp"
#include "VertexFactory/ShapeGenerator.hpp"
#include "PrimaryServices.hpp"
#include "Resources/Manager.hpp"
#include "Saphir/Generator/OverlayRendering.hpp"
#include "UIScreen.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Window.hpp"

namespace EmEn::Overlay
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Base::VertexFactory;
	using namespace Saphir;
	using namespace Graphics;
	using namespace Vulkan;

	std::shared_ptr< Program >
	Manager::generateShaderProgram (bool premultipliedAlpha, bool isBGRASurface) const noexcept
	{
		auto & renderer = m_resourceManager.graphicsRenderer();

		Generator::OverlayRendering generator{renderer.mainRenderTarget(), m_surfaceGeometry, premultipliedAlpha, isBGRASurface};

		/* Use the post-process framebuffer for pipeline creation (single-sample overlay). */
		if ( const auto * overlayFB = renderer.overlayFramebuffer(); overlayFB != nullptr )
		{
			generator.setPipelineFramebuffer(overlayFB);
		}

		if ( !generator.generateShaderProgram(renderer) )
		{
			Tracer::error(ClassId, "Unable to generate the overlay manager shader program !");

			return nullptr;
		}

		return generator.shaderProgram();
	}

	void
	Manager::enable (Input::Manager & inputManager, bool state) noexcept
	{
		if ( !this->usable() )
		{
			Tracer::warning(ClassId, "The overlay manager is not available !");

			return;
		}

		if ( state )
		{
			inputManager.addKeyboardListener(this);
			inputManager.addPointerListener(this);
		}
		else
		{
			inputManager.removeKeyboardListener(this);
			inputManager.removePointerListener(this);
		}

		m_enabled = state;
	}

	bool
	Manager::onInitialize () noexcept
	{
		m_surfaceGeometry = std::make_shared< Geometry::IndexedVertexResource >(m_resourceManager, "OverlayQuad", Geometry::EnablePrimaryTextureCoordinates);

		if ( !m_surfaceGeometry->load(ShapeGenerator::generateQuad(2.0F, 2.0F)) )
		{
			TraceError{ClassId} << "Unable to generate a geometry for UI surfaces !";

			m_surfaceGeometry.reset();

			return false;
		}

		/* Generate program variants for all combinations of alpha modes and pixel formats.
		 * Index layout:
		 *   0 = RGBA + standard alpha
		 *   1 = RGBA + premultiplied alpha
		 *   2 = BGRA + standard alpha
		 *   3 = BGRA + premultiplied alpha */
		for ( size_t index = 0; index < ProgramCount; ++index )
		{
			const bool premultipliedAlpha = (index & 0x1) != 0;
			const bool isBGRASurface = (index & 0x2) != 0;

			m_programs[index] = this->generateShaderProgram(premultipliedAlpha, isBGRASurface);

			if ( m_programs[index] == nullptr )
			{
				Tracer::error(ClassId, "Unable to generate an overlay manager shader program !");

				return false;
			}
		}

#ifdef IMGUI_ENABLED
		if ( this->initImGUI() )
		{
			TraceSuccess{ClassId} << "ImGUI library initialized !";
		}
		else
		{
			TraceError{ClassId} << "Unable to initialize ImGUI library !";

			return false;
		}
#endif

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
#ifdef IMGUI_ENABLED
		TraceInfo{ClassId} << "Releasing ImGUI library ...";

		this->releaseImGUI();
#endif

		this->forget(&m_resourceManager.graphicsRenderer().window());

		m_screens.clear();

		for ( auto & program : m_programs )
		{
			program.reset();
		}

		m_surfaceGeometry.reset();

		return true;
	}

	std::shared_ptr< UIScreen >
	Manager::createScreen (const std::string & name, bool enableKeyboardListener, bool enablePointerListener) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_screensAccess};

		if constexpr ( IsDebug )
		{
			if ( !m_framebufferProperties.isValid() )
			{
				TraceError{ClassId} << "The screen size are not initialized !";

				return nullptr;
			}
		}

		if ( m_screens.contains(name) )
		{
			TraceError{ClassId} << "An UI screen named '" << name << "' already exists !";

			return nullptr;
		}

		auto screen = std::make_shared< UIScreen >(
			name,
			m_framebufferProperties,
			m_resourceManager.graphicsRenderer(),
			enableKeyboardListener,
			enablePointerListener
		);

		m_screens[name] = screen;

		this->notify(UIScreenCreated, screen);

		return screen;
	}

#ifdef IMGUI_ENABLED
	std::shared_ptr< ImGUIScreen >
	Manager::createImGUIScreen (const std::string & name, const std::function< void () > & drawFunction) noexcept
	{
		if ( m_ImGUIScreens.contains(name) )
		{
			TraceError{ClassId} << "An ImGUI screen named '" << name << "' already exists !";

			return nullptr;
		}

		auto screen = std::make_shared< ImGUIScreen >(name, drawFunction);

		m_ImGUIScreens[name] = screen;

		return screen;
	}
#endif

	bool
	Manager::destroyScreen (const std::string & name) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_screensAccess};

		const auto screenIt = m_screens.find(name);

		if ( screenIt == m_screens.end() )
		{
			TraceError{ClassId} << "Unable to find '" << name << "' UI screen to erase it !";

			return false;
		}

		this->notify(UIScreenDestroying, screenIt->second);

		m_screens.erase(screenIt);

		this->notify(UIScreenDestroyed, name);

		return true;
	}

	void
	Manager::clearScreens () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_screensAccess};

		/* [ERASE IN LOOP] */
		auto screenIt = m_screens.begin();

		while ( screenIt != m_screens.end() )
		{
			/* Get a copy */
			const auto name = screenIt->first;

			this->notify(UIScreenDestroying, screenIt->second);

			screenIt = m_screens.erase(screenIt);

			this->notify(UIScreenDestroyed, name);
		}
	}

	bool
	Manager::enableScreen (const std::string & name) noexcept
	{
		const auto screenIt = m_screens.find(name);

		if ( screenIt == m_screens.cend() )
		{
			TraceWarning{ClassId} << "Unable to find the UI screen '" << name << "' to activate it !";

			return false;
		}

		screenIt->second->setVisibility(true);

		return true;
	}

	bool
	Manager::toggleScreen (const std::string & name) noexcept
	{
		const auto screenIt = m_screens.find(name);

		if ( screenIt == m_screens.cend() )
		{
			TraceWarning{ClassId} << "Unable to find the UI screen '" << name << "' to toggle it !";

			return false;
		}

		screenIt->second->setVisibility(!screenIt->second->isVisible());

		return true;
	}

	bool
	Manager::disableScreen (const std::string & name) noexcept
	{
		const auto screenIt = m_screens.find(name);

		if ( screenIt == m_screens.cend() )
		{
			TraceWarning{ClassId} << "Unable to find the UI screen '" << name << "' to disable it !";

			return false;
		}

		screenIt->second->setVisibility(false);

		return true;
	}

	void
	Manager::disableAllScreens () noexcept
	{
		for ( const auto & screen : m_screens | std::views::values )
		{
			screen->setVisibility(false);
		}
	}

	bool
	Manager::bringScreenOnTop (const std::string & screenName) noexcept
	{
		if ( const auto screenIt = m_screens.find(screenName); screenIt == m_screens.cend() )
		{
			TraceWarning{ClassId} << "Unable to find the UI screen '" << screenName << "' to bring it on top !";

			return false;
		}

		// TODO : Re-order the map.

		return true;
	}

	std::vector< std::string >
	Manager::screensNameList () const noexcept
	{
		std::vector< std::string > screenNames;
		screenNames.reserve(m_screens.size());

		std::ranges::transform(m_screens, std::back_inserter(screenNames), [] (const auto & screenIt) {
			return screenIt.first;
		});

		return screenNames;
	}

	std::vector< std::string >
	Manager::activeScreensNameList () const noexcept
	{
		std::vector< std::string > screenNames;
		screenNames.reserve(m_screens.size());

		for ( const auto & [screenName, screen] : m_screens )
		{
			if ( screen->isVisible() )
			{
				screenNames.emplace_back(screenName);
			}
		}

		return screenNames;
	}

	std::shared_ptr< const UIScreen >
	Manager::screen (const std::string & screenName) const noexcept
	{
		const auto screenIt = m_screens.find(screenName);

		if ( screenIt == m_screens.cend() )
		{
			TraceWarning{ClassId} << "There is no screen named '" << screenName << "' !";

			return nullptr;
		}

		return screenIt->second;
	}

	std::shared_ptr< UIScreen >
	Manager::screen (const std::string & screenName) noexcept
	{
		const auto screenIt = m_screens.find(screenName);

		if ( screenIt == m_screens.cend() )
		{
			TraceWarning{ClassId} << "There is no screen named '" << screenName << "' !";

			return nullptr;
		}

		return screenIt->second;
	}

	void
	Manager::updateFramebufferProperties () noexcept
	{
		const auto & windowState = m_resourceManager.graphicsRenderer().window().state();

		if ( auto & settings = m_primaryServices.settings(); settings.getOrSetDefault< bool >(OverlayForceScaleKey, DefaultOverlayForceScale) )
		{
			m_framebufferProperties.updateProperties(
				windowState.framebufferWidth,
				windowState.framebufferHeight,
				settings.getOrSetDefault< float >(OverlayScaleXKey, DefaultOverlayScale),
				settings.getOrSetDefault< float >(OverlayScaleYKey, DefaultOverlayScale)
			);
		}
		else
		{
			m_framebufferProperties.updateProperties(
				windowState.framebufferWidth,
				windowState.framebufferHeight,
				windowState.contentXScale,
				windowState.contentYScale
			);
		}
	}

	void
	Manager::updateVideoMemory () noexcept
	{
		/* NOTE: This can collide with the window resize event from Manager::onWindowResized() in another thread. */
		const std::lock_guard< std::mutex > lock{m_physicalRepresentationUpdateMutex};

		if ( !this->isEnabled() || m_screens.empty() )
		{
			return;
		}

		/* NOTE: Only process VISIBLE screens for performance.
		 * Hidden screens will be processed when they become visible again. */
		for ( const auto & screen : m_screens | std::views::values )
		{
			if ( screen->empty() || !screen->isVisible() )
			{
				continue;
			}

			if ( !screen->processSurfaceUpdates(false) )
			{
				TraceError{ClassId} << "Failed to process screen '" << screen->name() << "' updates!";
			}
		}
	}

	bool
	Manager::onWindowResized () noexcept
	{
		/* NOTE: This can collide with the successive call to Manager::processFrameUpdates() in the rendering loop. */
		const std::lock_guard< std::mutex > lock{m_physicalRepresentationUpdateMutex};

		/* Step 1: Update shared framebuffer properties with new window dimensions. */
		this->updateFramebufferProperties();

		/* Step 2: Force ALL screens (visible or not) to recalculate their pixel dimensions.
		 * The 'true' parameter triggers invalidate() on all surfaces, causing them to
		 * recreate their transition buffers at the new size.
		 * NOTE: Surfaces are NOT automatically committed. The active buffer continues to render
		 * with the old content/size until the application explicitly calls commitTransitionBuffer().
		 * This allows asynchronous renderers (e.g., CEF) to prepare new content before committing. */
		for ( const auto & screen : m_screens | std::views::values )
		{
			if ( screen->empty() )
			{
				continue;
			}

			if ( !screen->processSurfaceUpdates(true) )
			{
				TraceError{ClassId} << "Failed to process screen '" << screen->name() << "' updates!";
			}

			TraceDebug{ClassId} << "The screen '" << screen->name() << "' resized.";
		}

		/* Step 3: Notify observers of the resize completion. */
		const auto & windowState = m_resourceManager.graphicsRenderer().window().state();

		this->notify(OverlayResized, std::array< uint32_t, 2 >{
		   windowState.framebufferWidth,
		   windowState.framebufferHeight
		});

		TraceDebug{ClassId} <<
			"The overlay manager received last windows properties. " "\n"
			"Width: " << windowState.framebufferWidth << "\n"
			"Height: " << windowState.framebufferHeight << "\n"
			"ScaleX: " << windowState.contentXScale << "\n"
			"ScaleY: " << windowState.contentYScale << "\n";

		return true;
	}

	void
	Manager::render (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const CommandBuffer & commandBuffer) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_screensAccess};

		/* Check if the overlay is enabled and there is something to render.
		 * NOTE: ImGUI screens live in a separate container, so an ImGUI-only
		 * overlay (no UIScreen) must still be rendered. */
#ifdef IMGUI_ENABLED
		const bool nothingToRender = m_screens.empty() && m_ImGUIScreens.empty();
#else
		const bool nothingToRender = m_screens.empty();
#endif

		if ( !this->isEnabled() || nothingToRender )
		{
			return;
		}

		if constexpr ( IsDebug )
		{
			if ( m_surfaceGeometry == nullptr || !m_surfaceGeometry->isCreated() )
			{
				TraceError{ClassId} << "The surface geometry is no ready !";

				return;
			}
		}

		/* Lock for overlay resizing ! */
		const std::lock_guard< std::mutex > overlayLock{m_physicalRepresentationUpdateMutex};

		for ( const auto & screen : m_screens | std::views::values )
		{
			if ( screen->empty() || !screen->isVisible() )
			{
				continue;
			}

			/* NOTE: Select the right program based on alpha blending mode and pixel format.
			 * Index layout: bit 0 = premultiplied alpha, bit 1 = BGRA source. */
			const size_t programIndex = (screen->premultipliedAlpha() ? 1 : 0) | (screen->isUsingBGRAFormat() ? 2 : 0);
			const auto & program = m_programs[programIndex];

			/* Bind the graphics pipeline. */
			commandBuffer.bind(*program->graphicsPipeline());

			/* NOTE: Set dynamic viewport and scissor based on current render target extent. */
			{
				const auto & extent3D = renderTarget->extent();

				const VkViewport viewport{
					.x = 0.0F,
					.y = 0.0F,
					.width = static_cast< float >(extent3D.width),
					.height = static_cast< float >(extent3D.height),
					.minDepth = 0.0F,
					.maxDepth = 1.0F
				};
				vkCmdSetViewport(commandBuffer.handle(), 0, 1, &viewport);

				const VkRect2D scissor{
					.offset = {0, 0},
					.extent = {extent3D.width, extent3D.height}
				};
				vkCmdSetScissor(commandBuffer.handle(), 0, 1, &scissor);
			}

			/* Bind the geometry VBO and the optional IBO. */
			commandBuffer.bind(*m_surfaceGeometry, 0);

			const auto pipelineLayout = program->pipelineLayout();

			screen->render(renderTarget, commandBuffer, *pipelineLayout, *m_surfaceGeometry);
		}

#ifdef IMGUI_ENABLED
		/* NOTE: ImGUI relies on a single global context, so exactly one
		 * NewFrame()/Render() cycle is allowed per frame. We open the frame
		 * once, let every visible ImGUI screen emit its widgets, then submit
		 * all the draw data in a single pass. */
		const bool hasVisibleImGUIScreen = std::ranges::any_of(m_ImGUIScreens | std::views::values, [] (const auto & screen) {
			return screen->isVisible();
		});

		if ( hasVisibleImGUIScreen )
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			for ( const auto & screen : m_ImGUIScreens | std::views::values )
			{
				if ( screen->isVisible() )
				{
					screen->draw();
				}
			}

			ImGui::Render();

			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer.handle());
		}
#endif
	}

	bool
	Manager::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable->is(Window::getClassUID()) )
		{
			/* NOTE: On the window creation, we use the initial size to set the overlay physical size. */
			if ( notificationCode == Window::Created )
			{
				this->updateFramebufferProperties();

				return true;
			}

			if constexpr ( ObserverDebugEnabled )
			{
				TraceDebug{ClassId} << "Event #" << notificationCode << " from the window ignored.";
			}

			return true;
		}

		/* NOTE: Don't know what it is, goodbye! */
		TraceDebug{ClassId} <<
			"Received an unhandled notification (Code:" << notificationCode << ") from observable (UID:" << observable->classUID() << ")  ! "
			"Forgetting it ...";

		return false;
	}

	std::shared_ptr< DescriptorSetLayout >
	Manager::getDescriptorSetLayout (LayoutManager & layoutManager) noexcept
	{
		auto descriptorSetLayout = layoutManager.getDescriptorSetLayout(ClassId);

		if ( descriptorSetLayout == nullptr )
		{
			descriptorSetLayout = layoutManager.prepareNewDescriptorSetLayout(ClassId);
			descriptorSetLayout->setIdentifier(ClassId, ClassId, "DescriptorSetLayout");

			descriptorSetLayout->declareCombinedImageSampler(0, VK_SHADER_STAGE_FRAGMENT_BIT);

			if ( !layoutManager.createDescriptorSetLayout(descriptorSetLayout) )
			{
				return nullptr;
			}
		}

		return descriptorSetLayout;
	}

	bool
	Manager::setInputExclusiveScreen (const std::string & name) noexcept
	{
		const auto screen = this->screen(name);

		if ( screen == nullptr )
		{
			return false;
		}

		m_inputExclusiveScreen = screen;

		return true;
	}

#ifdef IMGUI_ENABLED

	bool
	Manager::initImGUI () noexcept
	{
		auto & graphicsRenderer = m_resourceManager.graphicsRenderer();

		if ( !graphicsRenderer.usable() )
		{
			TraceError{ClassId} << "No Vulkan graphics layer !";

			return false;
		}

		const auto & filesystem = m_primaryServices.fileSystem();

		m_iniFilepath = filesystem.configDirectory("imgui.ini");
		m_logFilepath = filesystem.cacheDirectory("imgui_log.txt");

		/* NOTE: Initialize ImGUI library. */
		{
			IMGUI_CHECKVERSION();

			ImGui::CreateContext();

			ImGuiIO & io = ImGui::GetIO(); (void)io;
			io.IniFilename = m_iniFilepath.c_str();
			io.LogFilename = m_logFilepath.c_str();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

			// Setup Dear ImGui style
			ImGui::StyleColorsDark();
			//ImGui::StyleColorsClassic();
		}

		/* NOTE: Initialize ImGUI with GLFW. */
		if ( !ImGui_ImplGlfw_InitForVulkan(graphicsRenderer.window().handle(), true) )
		{
			Tracer::error(ClassId, "Unable to initialize ImGUI with GLFW !");

			return false;
		}

		/* NOTE: Initialize ImGUI with Vulkan. */
		{
			const auto & device = graphicsRenderer.device();

			/* NOTE: ImGUI draws inside the engine overlay render pass (post-process
			 * framebuffer). Its main pipeline must be created against that render pass,
			 * otherwise the Vulkan backend silently skips pipeline creation (see
			 * imgui_impl_vulkan.cpp) and nothing is ever drawn. */
			const auto * overlayFramebuffer = graphicsRenderer.overlayFramebuffer();

			if ( overlayFramebuffer == nullptr )
			{
				Tracer::error(ClassId, "No overlay framebuffer available to create the ImGUI pipeline !");

				return false;
			}

			/* Create the descriptor pool.
			 * FIXME: These are fancy numbers ! */
			const auto sizes = std::vector< VkDescriptorPoolSize >{
				{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
				{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
				{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
				{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
				{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
				{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
				{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
			};

			m_ImGUIDescriptorPool = std::make_shared< DescriptorPool >(device, sizes, 1000, VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);
			m_ImGUIDescriptorPool->setIdentifier(ClassId, "ImGUI", "DescriptorPool");

			if ( !m_ImGUIDescriptorPool->createOnHardware() )
			{
				Tracer::fatal(ClassId, "Unable to create the ImGUI descriptor pool !");

				return false;
			}

			const auto frameCount = std::max< uint32_t >(graphicsRenderer.framesInFlight(), 2);

			// Setup Platform/Renderer backends
			ImGui_ImplVulkan_InitInfo info{};
			info.ApiVersion = VK_API_VERSION_1_3;
			info.Instance = graphicsRenderer.vulkanInstance().handle();
			info.PhysicalDevice = device->physicalDevice()->handle();
			info.Device = device->handle();
			info.QueueFamily = device->getGraphicsFamilyIndex();
			info.Queue = device->getGraphicsQueue(QueuePriority::High)->handle();
			info.DescriptorPool = m_ImGUIDescriptorPool->handle();
			//info.DescriptorPoolSize = 1;
			info.MinImageCount = frameCount;
			info.ImageCount = frameCount;
			info.PipelineCache = VK_NULL_HANDLE;
			info.PipelineInfoMain.RenderPass = overlayFramebuffer->renderPass()->handle();
			info.PipelineInfoMain.Subpass = 0;
			info.UseDynamicRendering = false;
			info.Allocator = VK_NULL_HANDLE;
			info.CheckVkResultFn = nullptr;
			info.MinAllocationSize = 1024 * 1024; // Minimum allocation size. Set to 1024*1024 to satisfy zealous best practices validation layer and waste a little memory.
			//info.CustomShaderVertCreateInfo = ;
			//info.CustomShaderFragCreateInfo = ;

			if ( !ImGui_ImplVulkan_Init(&info) )
			{
				Tracer::error(ClassId, "Unable to initialize ImGUI with Vulkan !");

				return false;
			}
		}

		return true;
	}

	void
	Manager::releaseImGUI () noexcept
	{
		ImGui_ImplVulkan_Shutdown();

		ImGui_ImplGlfw_Shutdown();

		ImGui::DestroyContext();

		m_ImGUIDescriptorPool->destroyFromHardware();
		m_ImGUIDescriptorPool.reset();
	}

#endif
}
