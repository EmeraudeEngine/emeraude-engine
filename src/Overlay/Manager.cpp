/*
 * src/Overlay/Manager.cpp
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

#include "Manager.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <iterator>
#include <ranges>

/* Third-party inclusions */
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
#include "Libs/VertexFactory/ShapeGenerator.hpp"
#include "Saphir/Generator/OverlayRendering.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Vulkan/GraphicsPipeline.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/DescriptorSetLayout.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Resources/Manager.hpp"
#include "UIScreen.hpp"
#include "PrimaryServices.hpp"
#include "Window.hpp"

namespace EmEn::Overlay
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Libs::VertexFactory;
	using namespace Saphir;
	using namespace Graphics;
	using namespace Vulkan;

	bool
	Manager::onKeyPress (int32_t key, int32_t scancode, int32_t modifiers, bool repeat) noexcept
	{
		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningKeyboard() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onKeyPress(key, scancode, modifiers, repeat);
		}

		return std::ranges::any_of(m_screens, [key, scancode, modifiers, repeat] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningKeyboard() )
			{
				return false;
			}

			return screen->onKeyPress(key, scancode, modifiers, repeat);
		});
	}

	bool
	Manager::onKeyRelease (int32_t key, int32_t scancode, int32_t modifiers) noexcept
	{
		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningKeyboard() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onKeyRelease(key, scancode, modifiers);
		}

		return std::ranges::any_of(m_screens, [key, scancode, modifiers] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningKeyboard() )
			{
				return false;
			}

			return screen->onKeyRelease(key, scancode, modifiers);
		});
	}

	bool
	Manager::onCharacterType (uint32_t unicode) noexcept
	{
		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningKeyboard() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onCharacterType(unicode);
		}

		return std::ranges::any_of(m_screens, [unicode] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningKeyboard() )
			{
				return false;
			}

			return screen->onCharacterType(unicode);
		});
	}

	bool
	Manager::onPointerMove (float positionX, float positionY) noexcept
	{
		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningPointer() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onPointerMove(positionX, positionY);
		}

		return std::ranges::any_of(m_screens, [&] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningPointer() )
			{
				return false;
			}

			return screen->onPointerMove(positionX, positionY);
		});
	}

	bool
	Manager::onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept
	{
		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningPointer() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onButtonPress(positionX, positionY, buttonNumber, modifiers);
		}

		return std::ranges::any_of(m_screens, [&] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningPointer() )
			{
				return false;
			}

			return screen->onButtonPress(positionX, positionY, buttonNumber, modifiers);
		});
	}

	bool
	Manager::onButtonRelease (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept
	{
		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningPointer() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onButtonRelease(positionX, positionY, buttonNumber, modifiers);
		}

		return std::ranges::any_of(m_screens, [&] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningPointer() )
			{
				return false;
			}

			return screen->onButtonRelease(positionX, positionY, buttonNumber, modifiers);
		});
	}

	bool
	Manager::onMouseWheel (float positionX, float positionY, float xOffset, float yOffset, int32_t modifiers) noexcept
	{
		if ( m_screens.empty() )
		{
			return false;
		}

		if ( m_inputExclusiveScreen )
		{
			if ( m_inputExclusiveScreen->empty() || !m_inputExclusiveScreen->isVisible() || !m_inputExclusiveScreen->isListeningPointer() )
			{
				return false;
			}

			return m_inputExclusiveScreen->onMouseWheel(positionX, positionY, xOffset, yOffset, modifiers);
		}

		return std::ranges::any_of(m_screens, [&] (const auto & pair) {
			const auto & screen = pair.second;

			if ( screen->empty() || !screen->isVisible() || !screen->isListeningPointer() )
			{
				return false;
			}

			return screen->onMouseWheel(positionX, positionY, xOffset, yOffset, modifiers);
		});
	}

	bool
	Manager::generateShaderProgram () noexcept
	{
		Generator::OverlayRendering generator{m_graphicsRenderer.mainRenderTarget(), m_surfaceGeometry, Generator::OverlayRendering::ColorConversion::ToLinear};

		if ( !generator.generateShaderProgram(m_graphicsRenderer) )
		{
			Tracer::error(ClassId, "Unable to generate the overlay manager shader program !");

			return false;
		}

		m_program = generator.shaderProgram();

		return true;
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
		m_surfaceGeometry = std::make_shared< Geometry::IndexedVertexResource >("OverlayQuad", Geometry::EnablePrimaryTextureCoordinates);

		if ( !m_surfaceGeometry->load(ShapeGenerator::generateQuad(2.0F, 2.0F)) )
		{
			TraceError{ClassId} << "Unable to generate a geometry for UI surfaces !";

			m_surfaceGeometry.reset();

			return false;
		}

		if ( !this->generateShaderProgram() )
		{
			Tracer::error(ClassId, "Unable to generate the overlay manager shader program !");

			return false;
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

		this->forget(&m_window);

		m_screens.clear();

		m_program.reset();
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

		auto screen = std::make_shared< UIScreen >(name, m_framebufferProperties, m_graphicsRenderer, enableKeyboardListener, enablePointerListener);

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
		const auto screenIt = m_screens.find(screenName);

		if ( screenIt == m_screens.cend() )
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
		std::vector< std::string > screenNames{};
		screenNames.reserve(m_screens.size());

		std::ranges::transform(m_screens, std::back_inserter(screenNames), [] (const auto & screenIt) {
			return screenIt.first;
		});

		return screenNames;
	}

	std::vector< std::string >
	Manager::activeScreensNameList () const noexcept
	{
		std::vector< std::string > screenNames{};
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
		const auto & windowState = m_window.state();

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
	Manager::processFrameUpdates () noexcept
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
			if ( !screen->isVisible() )
			{
				continue;
			}

			screen->processSurfaceUpdates(false);
		}
	}

	bool
	Manager::onWindowResized () noexcept
	{
		/* NOTE: This can collide with the successive call to Manager::processFrameUpdates() in the rendering loop. */
		const std::lock_guard< std::mutex > lock{m_physicalRepresentationUpdateMutex};

		/* Step 1: Update shared framebuffer properties with new window dimensions. */
		this->updateFramebufferProperties();

		if ( m_program == nullptr )
		{
			TraceError{ClassId} << "The program wasn't generated !";

			return false;
		}

		/* NOTE: Pipeline recreation is no longer needed because viewport and scissor are now dynamic states.
		 * The viewport/scissor are set dynamically in render() using the current render target extent. */

		/* Step 2: Force ALL screens (visible or not) to recalculate their pixel dimensions.
		 * The 'true' parameter triggers invalidate() on all surfaces, causing them to
		 * recreate their transition buffers at the new size.
		 * NOTE: Surfaces are NOT automatically committed. The active buffer continues to render
		 * with the old content/size until the application explicitly calls commitTransitionBuffer().
		 * This allows asynchronous renderers (e.g., CEF) to prepare new content before committing. */
		for ( const auto & screen : m_screens | std::views::values )
		{
			screen->processSurfaceUpdates(true);
		}

		/* Step 3: Notify observers of the resize completion. */
		const auto & windowState = m_window.state();

		this->notify(OverlayResized, std::array< uint32_t, 2 >{
		   windowState.framebufferWidth,
		   windowState.framebufferHeight
		});

		return true;
	}

	void
	Manager::render (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const CommandBuffer & commandBuffer) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_screensAccess};

		/* Check if the overlay is enabled or there is something to render. */
		if ( !this->isEnabled() || m_screens.empty() )
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

		/* Bind the graphics pipeline. */
		commandBuffer.bind(*m_program->graphicsPipeline());

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

		const auto pipelineLayout = m_program->pipelineLayout();

		for ( const auto & screen : m_screens | std::views::values )
		{
			if ( screen->empty() || !screen->isVisible() )
			{
				continue;
			}

			screen->render(renderTarget, commandBuffer, *pipelineLayout, *m_surfaceGeometry);
		}

#ifdef IMGUI_ENABLED
		for ( const auto & screen : m_ImGUIScreens | std::views::values )
		{
			if ( !screen->isVisible() )
			{
				continue;
			}

			screen->render(commandBuffer);
		}
#endif
	}

	bool
	Manager::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & /*data*/) noexcept
	{
		if ( observable->is(Window::getClassUID()) )
		{
			/* NOTE: On the window creation, we use the initial size to set the overlay phyical size. */
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
		if ( !m_graphicsRenderer.usable() )
		{
			TraceError{ClassId} << "No Vulkan graphics layer !";

			return false;
		}

		const auto & filesystem = m_primaryServices.fileSystem();

		m_iniFilepath = filesystem.configDirectory("imgui.ini");
		m_logFilepath = filesystem.cacheDirectory("imgui_log.txt");

		/* NOTE: Initialize ImGUI library. */
		{
			// this initializes the core structures of imgui
			// Setup Dear ImGui context
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
		if ( !ImGui_ImplGlfw_InitForVulkan(m_window.handle(), true) )
		{
			Tracer::error(ClassId, "Unable to initialize ImGUI with GLFW !");

			return false;
		}

		/* NOTE: Initialize ImGUI with Vulkan. */
		{
			const auto swapChain = m_graphicsRenderer.swapChain();
			const auto device = swapChain->device();

			/* Create a descriptor pool */
			/* FIXME: These are fancy numbers ! */
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

			// Setup Platform/Renderer backends
			ImGui_ImplVulkan_InitInfo info{};
			info.Instance = m_graphicsRenderer.vulkanInstance().handle();
			info.PhysicalDevice = device->physicalDevice()->handle();
			info.Device = device->handle();
			info.QueueFamily = device->getGraphicsFamilyIndex();
			info.Queue = device->getQueue(QueueJob::Graphics, QueuePriority::High)->handle();
			info.PipelineCache = VK_NULL_HANDLE;
			info.DescriptorPool = m_ImGUIDescriptorPool->handle();
			info.RenderPass = swapChain->framebuffer()->renderPass()->handle();
			info.Subpass = 0;
			info.MinImageCount = swapChain->createInfo().minImageCount;
			info.ImageCount = swapChain->imageCount();
			info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			info.Allocator = VK_NULL_HANDLE;
			info.CheckVkResultFn = nullptr;

			if ( !ImGui_ImplVulkan_Init(&info) )
			{
				Tracer::error(ClassId, "Unable to initialize ImGUI with Vulkan !");

				return false;
			}

			if ( !ImGui_ImplVulkan_CreateFontsTexture() )
			{
				Tracer::error(ClassId, "Unable to create ImGUI fonts texture !");

				return false;
			}
		}

		return true;
	}

	void
	Manager::releaseImGUI () noexcept
	{
		ImGui_ImplVulkan_DestroyFontsTexture();

		ImGui_ImplVulkan_Shutdown();

		ImGui_ImplGlfw_Shutdown();

		ImGui::DestroyContext();

		m_ImGUIDescriptorPool->destroyFromHardware();
		m_ImGUIDescriptorPool.reset();
	}

#endif
}
