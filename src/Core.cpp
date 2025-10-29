/*
 * src/Core.cpp
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

#include "Core.hpp"

/* STL inclusions. */
#include <sstream>
#include <iostream>
#include <ranges>
#include <thread>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"

/* Local inclusions. */
#include "Libs/Time/Elapsed/PrintScopeRealTime.hpp"
#include "Libs/Time/Time.hpp"
#include "Libs/Version.hpp"
#include "PlatformSpecific/Desktop/Dialog/Message.hpp"
#include "PlatformSpecific/Desktop/Commands.hpp"
#include "Tool/GeometryDataPrinter.hpp"
#include "Tool/ShowVulkanInformation.hpp"
#include "Input/Types.hpp"
#include "Constants.hpp"

namespace EmEn
{
	using namespace Libs;
	using namespace Vulkan;
	using namespace Graphics;
	using namespace Scenes;
	using namespace Input;
	using namespace Resources;

	Core::Core (int argc, char * * argv, const char * applicationName, const Version & applicationVersion, const char * applicationOrganization, const char * applicationDomain) noexcept
		: KeyboardListenerInterface(false, false),
		Controllable(ClassId),
		m_identification(applicationName, applicationVersion, applicationOrganization, applicationDomain),
		m_primaryServices(argc, argv, m_identification)
	{
		if ( !this->initializeBaseLevel() )
		{
#if IS_MACOS // clang 15 failure (try to use __apple_build_version__)
			std::terminate();
#else
			std::quick_exit(2);
#endif
		}
	}

#if IS_WINDOWS
	Core::Core (int argc, wchar_t * * wargv, const char * applicationName, const Version & applicationVersion, const char * applicationOrganization, const char * applicationDomain) noexcept
		: KeyboardListenerInterface(false, false),
		Controllable(ClassId),
		m_identification(applicationName, applicationVersion, applicationOrganization, applicationDomain),
		m_primaryServices(argc, wargv, m_identification)
	{
		if ( !this->initializeBaseLevel() )
		{
			std::quick_exit(2);
		}
	}
#endif

	Core::~Core ()
	{
		/* Terminate primary services. */
		for ( auto * service : std::ranges::reverse_view(m_primaryServicesEnabled) )
		{
			if ( service->terminate() )
			{
				TraceSuccess{ClassId} << service->name() << " primary service terminated gracefully!";
			}
			else
			{
				TraceError{ClassId} << service->name() << " primary service failed to terminate properly!";
			}
		}

		m_primaryServicesEnabled.clear();

		m_primaryServices.terminate();

		Tracer::success(ClassId, "*** Core level terminated ***");
	}

	void
	Core::logicsTask () noexcept
	{
		const std::chrono::duration< uint64_t, std::micro > logicsUpdateFrequency{EngineUpdateCycleDurationUS< uint64_t >};

		while ( m_isLogicsLoopRunning )
		{
			if ( m_paused )
			{
				/* NOTE: Fake an engine cycle update based on 30Hz. */
				std::this_thread::sleep_for(std::chrono::milliseconds{33});

				continue;
			}

			const auto top = std::chrono::steady_clock::now();

			{
				/* NOTE: Print a warning if this loop takes more than 16.66 ms, which means we are below 60Hz. */
				const Time::Elapsed::PrintScopeRealTimeThreshold stat{"logicsTask", EngineUpdateCycleDurationMS< double >};

				/* User-application cyclic update. */
				this->onProcessLogics(m_cycle);

				/* Core-application cyclic update on the active scene only.
				 * NOTE: It will ask for a shared-access to the active scene
				 * preventing locking the "rendering thread" for updating the scene logic. */
				m_sceneManager.withSharedActiveScene([&] (const auto & activeScene ) {
					/* This should run "freely" on one thread. */
					activeScene->processLogics(m_cycle);

					/* Tells the system one snapshot of the logic is ready to be synced. */
					activeScene->publishStateForRendering();
				}, true);

				m_lifetime += EngineUpdateCycleDurationUS< uint64_t >;
				m_cycle++;
			}

			/* NOTE: Let sleep the thread until the next update. */
			if ( const auto duration = std::chrono::steady_clock::now() - top; duration < logicsUpdateFrequency )
			{
				std::this_thread::sleep_for(logicsUpdateFrequency - duration);
			}
		}

		Tracer::success(ClassId, "[THREAD] Logics process terminated successfully !");
	}

	void
	Core::renderingTask () noexcept
	{
		uint64_t frames = 0;

		while ( m_isRenderingLoopRunning )
		{
			if ( m_paused )
			{
				/* NOTE: Fake a frame rendering time based on an ideal 30 FPS rendering. */
				std::this_thread::sleep_for(std::chrono::milliseconds{33});

				continue;
			}

			{
				/* NOTE: Print a warning if this loop takes more than 33.33 ms, which means we are below 30 FPS. */
				const Time::Elapsed::PrintScopeRealTimeThreshold stat{"renderingTask", 1000.0 / 30.0};

				/* NOTE: Ask for a shared-access to the scene content prevening to lock the "logic thread" and draw the scene. */
				m_sceneManager.withSharedActiveScene([&] (const auto & activeScene) {
					if ( activeScene != nullptr )
					{
						/* This should only sync UBO for the scene. */
						activeScene->updateVideoMemory(
							m_graphicsRenderer.isShadowMapsEnabled(),
							m_graphicsRenderer.isRenderToTexturesEnabled()
						);
					}

					/* This should only sync UBO for the overlay. */
					m_overlayManager.updateVideoMemory();

					/* Render the scene (optional) and the overlay on top. */
					m_graphicsRenderer.renderFrame(activeScene, m_overlayManager);
				}, false);

				frames++;
			}
		}

		/* NOTE: Wait until the device has finished all his pending work. */
		m_graphicsRenderer.device()->waitIdle("Core::renderingTask()");

		Tracer::success(ClassId, "[THREAD] Rendering process terminated successfully !");

		TraceInfo{ClassId} << "The rendering produced " << frames << " frames.";
	}

	void
	Core::refreshApplicationSurface () noexcept
	{
		if ( !m_sceneManager.refreshActiveScene() )
		{
			Tracer::error(ClassId, "Unable to refresh the active scene!");
		}

		if ( !m_overlayManager.updatePhysicalRepresentation() )
		{
			Tracer::error(ClassId, "Unable to refresh the overlay manager!");
		}

		this->notify(ApplicationSurfaceRefreshed);
	}

	bool
	Core::run () noexcept
	{
		switch ( m_startupMode )
		{
			case StartupMode::Error :
				Tracer::fatal(ClassId, "Engine startup has been aborted by the core constructor !");

				return false;

			case StartupMode::ToolsMode :
				return this->executeToolsMode();

			case StartupMode::Continue :
				/* NOTE: Let the application stop the run. */
				if ( this->onBeforeSecondaryServicesInitialization() )
				{
					return this->terminate() == 0;
				}

				/* NOTE: Finish the core initialization with secondary services. */
				if ( !this->initializeCoreLevel() )
				{
					return false;
				}

				Tracer::success(ClassId, "Engine at application level initialized !");

				m_isMainLoopRunning = true;

				break;
		}

		/* NOTE: Create the logic loop and the rendering loop into threads
		 * automatically joined at the end of this function. */
		std::thread logicsThread{[this] {
			this->logicsTask();
		}};

		std::thread renderingThread{[this] {
			this->renderingTask();
		}};

		Tracer::success(ClassId, "Core level execution started !");

		/* Launch the application level. */
		if ( this->onStart() )
		{
			Tracer::success(ClassId, "The application successfully started.");

			this->notify(ExecutionStarted);
		}
		else
		{
			Tracer::fatal(ClassId, "The application failed to start ! Exiting ...");

			this->stop();
		}

		auto lastTop = std::chrono::steady_clock::now();

		/* NOTE: This is the engine main loop. */
		while ( m_isMainLoopRunning )
		{
			/* EventInput: Update user events.
			 * DirectInput: Copy the state of every input device to use it in the engine cycle. */
			m_inputManager.waitSystemEvents(0.010);

			/* NOTE: If the swap-chain has been refreshed, we refresh the application according to the new framebuffer. */
			if ( m_graphicsRenderer.checkSwapChainRefresh() )
			{
				this->refreshApplicationSurface();
			}

			/* Let the child class get the call event from the main loop. */
			this->onMainLoopCycle();

			/* NOTE: Checks whether the engine is running or paused.
			 * If not, we wait for a wake-up event with a blocking function. */
			if ( m_paused )
			{
				/* Stop all timers of an active scene. */
				m_sceneManager.withExclusiveActiveScene([&] (const auto & activeScene) {
					activeScene->pauseTimers();
				}, true);

				/* Wait until an event release the pause state. */
				while ( m_paused )
				{
					/* Let the child class get the call event from the main loop. */
					this->onMainLoopCycle();

					m_inputManager.waitSystemEvents();
				}

				/* Restart all timers of the active scene. */
				m_sceneManager.withExclusiveActiveScene([&] (const auto & activeScene) {
					activeScene->resumeTimers();
				}, true);
			}

			if constexpr ( IsDebug )
			{
				auto currentTime = std::chrono::steady_clock::now();

				if ( auto elapsedTime = currentTime - lastTop; elapsedTime >= std::chrono::seconds(1) )
				{
					this->showStatistics();

					lastTop = currentTime;
				}

				if ( !m_coreMessages.empty() )
				{
					this->displayCoreMessages();
				}
			}
		}

		/* NOTE: Be sure all threads from the pool finish their work. */
		if ( const auto threadPool = m_primaryServices.threadPool(); threadPool != nullptr )
		{
			Tracer::info(ClassId, "Waiting for thread pool finished all the work ...");

			threadPool->wait();
		}

		if ( renderingThread.joinable() )
		{
			Tracer::info(ClassId, "Waiting for rendering thread to be joined ...");

			renderingThread.join();
		}

		if ( logicsThread.joinable() )
		{
			Tracer::info(ClassId, "Waiting for core logics thread to be joined ...");

			logicsThread.join();
		}

		return this->terminate() == 0;
	}

	void
	Core::displayCoreMessages () noexcept
	{
		using namespace PlatformSpecific::Desktop::Dialog;

		const auto error = m_coreMessages.front();

		Message dialog{
			"Internal engine message !",
			error,
			ButtonLayout::OK,
			MessageType::Info
		};

		m_coreMessages.pop();

		this->pause();

		dialog.execute(&this->window());

		this->resume();
	}

	void
	Core::onMainLoopCycle () noexcept
	{
		/* Makes nothing special here. */
	}

	bool
	Core::initializeBaseLevel () noexcept
	{
		std::cout << "\n"
			"Engine      : " << m_identification.engineId() << "\n"
			"Application : " << m_identification.applicationId() << "\n"
			"Reverse ID  : " << m_identification.applicationReverseId() << "\n\n";

		/* Registering core help. */
		{
			m_coreHelp.registerArgument("Show this help.", "help", 'h');
			m_coreHelp.registerArgument("Disable the window for server version.", "window-less", 'W');
			m_coreHelp.registerArgument("Disable audio layer.", "disable-audio");
			m_coreHelp.registerArgument("Display only logs which tags appears. TAG is a list of words separated by comma.", "filter-tag", 't', {"TAG"});
			m_coreHelp.registerArgument("Set a custom core settings file. FILE_PATH is where to get the settings file and should be writable.", "settings-filepath", 0, {"FILE_PATH"});
			m_coreHelp.registerArgument("Disable the generation or the saving of settings files.", "disable-settings-autosave");
			m_coreHelp.registerArgument("Enable log writing.", "enable-log", 'l', {"FILE_PATH"});
			m_coreHelp.registerArgument("Add a custom data directory.", "add-data-directory", 0, {"DIRECTORY_PATH"});
			m_coreHelp.registerArgument("Force the use of a cache directory overriding every others.", "cache-directory", 0, {"DIRECTORY_PATH"});
			m_coreHelp.registerArgument("Force the use of a config directory overriding every others.", "config-directory", 0, {"DIRECTORY_PATH"});
			m_coreHelp.registerArgument("Force the use of a data directory overriding every others.", "data-directory", 0, {"DIRECTORY_PATH"});
			m_coreHelp.registerArgument("Execute a specific tool.", "tools-mode", 't');

			m_coreHelp.registerShortcut("Quit the application.", KeyEscape, ModKeyShift);
			m_coreHelp.registerShortcut("Print the active scene content in console.", KeyF1, ModKeyShift);
			m_coreHelp.registerShortcut("Refreshes scenes (Experimental).", KeyF2, ModKeyShift);
			m_coreHelp.registerShortcut("Test dialog (Experimental).", KeyF3, ModKeyShift);
			m_coreHelp.registerShortcut("Reset the window size to defaults.", KeyF4, ModKeyShift);
			m_coreHelp.registerShortcut("Open settings file in text editor.", KeyF5, ModKeyShift);
			m_coreHelp.registerShortcut("Open file explorer to application configuration directory.", KeyF6, ModKeyShift);
			m_coreHelp.registerShortcut("Open file explorer to application cache directory.", KeyF7, ModKeyShift);
			m_coreHelp.registerShortcut("Open file explorer to application user data directory.", KeyF8, ModKeyShift);
			m_coreHelp.registerShortcut("Clean up unused resources from managers.", KeyF9, ModKeyShift);
			m_coreHelp.registerShortcut("Suspend core thread execution for 3 seconds.", KeyF10, ModKeyShift);
			m_coreHelp.registerShortcut("Toggle the window fullscreen mode.", KeyF11, ModKeyShift);
			m_coreHelp.registerShortcut("Unused yet. Reserved by the core.", KeyF12, ModKeyShift);
		}

		if ( m_primaryServices.initialize() )
		{
			Tracer::success(ClassId, "Primary services up !");

			/* We want to know at core level, when something is downloading! */
			this->observe(&m_primaryServices.netManager());
		}
		else
		{
			Tracer::fatal(ClassId, "Unable to initialize primary services !");

			m_startupMode = StartupMode::Error;

			return false;
		}

		if ( m_primaryServices.arguments().isSwitchPresent("-h", "--help") )
		{
			m_showHelp = true;

			return false;
		}

		/* Initialize the console. */
		if ( m_consoleController.initialize(m_primaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_consoleController.name() << " service up !";

			this->registerToConsole();

			this->observe(&m_consoleController);
		}
		else
		{
			TraceFatal{ClassId} << m_consoleController.name() << " service failed to execute !";

			m_startupMode = StartupMode::Error;

			return false;
		}

		/* Initialize resource manager services. */
		if ( m_resourceManager.initialize(m_primaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_resourceManager.name() << " service up !";
		}
		else
		{
			TraceError{ClassId} << m_resourceManager.name() << " service failed to execute !";
		}

		/* Initialize user service. */
		if ( m_user.initialize(m_primaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_user.name() << " service up !";
		}
		else
		{
			TraceError{ClassId} << m_user.name() << " service failed to execute !";
		}

		/* Print startup general information. */
		if ( m_primaryServices.settings().getOrSetDefault< bool >(CoreShowInformationKey, DefaultCoreShowInformation) )
		{
			Tracer::info(ClassId, m_primaryServices.information());
		}

		/* Checks if we need to execute the engine in tool mode. */
		if ( m_primaryServices.arguments().isSwitchPresent(ToolsArg, ToolsLongArg) )
		{
			m_startupMode = StartupMode::ToolsMode;
		}

		Tracer::success(ClassId, "*** Core level created ***");

		return true;
	}

	bool
	Core::initializeCoreLevel () noexcept
	{
		/* Initialize platform service (GLFW). */
		if ( m_platformManager.initialize(m_secondaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_platformManager.name() << " service up !";
		}
		else
		{
			TraceFatal{ClassId} << m_platformManager.name() << " service failed to execute !";

			return false;
		}

		/* Initialize the vulkan API. */
		if ( m_vulkanInstance.initialize(m_secondaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_vulkanInstance.name() << " service up !";
		}
		else
		{
			TraceFatal{ClassId} << m_vulkanInstance.name() << " service failed to execute !";

			return false;
		}

		/* Initialize the handle. */
		if ( m_window.initialize(m_secondaryServicesEnabled) )
		{
			this->observe(&m_window);

			TraceSuccess{ClassId} << m_window.name() << " service up !";
		}
		else
		{
			TraceFatal{ClassId} << m_window.name() << " service failed to execute !";

			return false;
		}

		/* Initialization of the input manager. */
		if ( m_inputManager.initialize(m_secondaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_inputManager.name() << " service up !";

			/* Configure the input manager. */
			m_inputManager.enableKeyboardListening(true);
			m_inputManager.enablePointerListening(true);
			m_inputManager.enableCopyKeyboardState(true);
			m_inputManager.enableCopyPointerState(true);

			/* Adds Core keyboard listener to the input manager. */
			m_inputManager.addKeyboardListener(this);

#if IS_MACOS
			m_inputManager.enablePointerScaling(
				m_window.state().contentXScale,
				m_window.state().contentYScale
			);
#endif

			this->observe(&m_inputManager);
		}
		else
		{
			TraceFatal{ClassId} << m_inputManager.name() << " service failed to execute !";

			return false;
		}

		/* Initialize graphics renderer. */
		if ( m_graphicsRenderer.initialize(m_secondaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_graphicsRenderer.name() << " service up !";

			m_graphicsRenderer.registerToObject(*this);

			this->observe(&m_graphicsRenderer.shaderManager());

			/* FIXME: Check a better way to give the access ... */
			Geometry::Interface::s_graphicsRenderer = &m_graphicsRenderer;
			TextureResource::Abstract::s_graphicsRenderer = &m_graphicsRenderer;
			Material::Interface::s_graphicsRenderer = &m_graphicsRenderer;
			Component::Abstract::s_graphicsRenderer = &m_graphicsRenderer;
		}
		else
		{
			TraceFatal{ClassId} << m_graphicsRenderer.name() << " service failed to execute !";

			return false;
		}

		/* Initialize physics manager. */
		if ( m_physicsManager.initialize(m_secondaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_physicsManager.name() << " service up !";
		}
		else
		{
			TraceWarning{ClassId} <<
				m_physicsManager.name() << " service failed to execute !" "\n"
				"No physics acceleration available !";
		}

		/* Initialize audio manager. */
		if ( m_audioManager.initialize(m_secondaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_audioManager.name() << " service up !";

			m_audioManager.registerToObject(*this);

			this->observe(&m_audioManager.trackMixer());

			/* FIXME: Check a better way to give the access ... */
			Component::Abstract::s_audioManager = &m_audioManager;
		}
		else
		{
			TraceWarning{ClassId} << m_audioManager.name() << " service failed to execute, no audio available !";
		}

		/* Initialization of the overlay manager. */
		if ( m_overlayManager.initialize(m_secondaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_overlayManager.name() << " service up !";

			m_overlayManager.enable(m_inputManager, true);

			/* Initialization of the notifier. */
			if ( m_notifier.initialize(m_secondaryServicesEnabled) )
			{
				TraceSuccess{ClassId} << m_notifier.name() << " service up !";
			}
			else
			{
				TraceError{ClassId} << m_notifier.name() << " service failed to execute !";
			}

			/* Initialization of the core screen. */
			if ( !this->initializeCoreScreen() )
			{
				Tracer::warning(ClassId, "Unable to create Core screens for information's and basic functions !");
			}
		}
		else
		{
			TraceFatal{ClassId} << m_overlayManager.name() << " service failed to execute !";

			return false;
		}

		/* Initialize scene manager. */
		if ( m_sceneManager.initialize(m_secondaryServicesEnabled) )
		{
			this->observe(&m_sceneManager);

			TraceSuccess{ClassId} << m_sceneManager.name() << " service up !";
		}
		else
		{
			TraceError{ClassId} << m_sceneManager.name() << " service failed to execute !";

			return false;
		}

		Tracer::success(ClassId, "Engine at core level initialized ! Now initialization at the application level ...");

		return true;
	}

	bool
	Core::enableUserService (ServiceInterface * userService) noexcept
	{
		if ( !userService->initialize(m_userServiceEnabled) )
		{
			TraceError{ClassId} << userService->name() << " user service failed to execute !";

			return false;
		}

		TraceSuccess{ClassId} << userService->name() << " user service up !";

		return true;
	}

	void
	Core::onRegisterToConsole () noexcept
	{
		this->bindCommand("exit,quit,shutdown", [this] (const Console::Arguments & /*arguments*/, Console::Outputs & outputs) {
			outputs.emplace_back(Severity::Info, "Shutdown procedure called from console ...");

			this->stop();

			return 0;
		}, "Quit the application.");
	}

	bool
	Core::initializeCoreScreen () noexcept
	{
#ifdef IMGUI_ENABLED
		const auto screen = m_overlayManager.createImGUIScreen("CoreScreen", [] () {
			bool show = true;
			ImGui::ShowDemoWindow(&show);
		});

		screen->setVisibility(false);
#else
		const auto screen = m_overlayManager.createScreen("CoreScreen", false, false);
#endif

		return screen != nullptr;
	}

	void
	Core::pause () noexcept
	{
		if ( m_paused  )
		{
			return;
		}

		/* Dispatch the pause event,
		 * first by sending the event,
		 * then directly to the sub application. */
		this->notify(ExecutionPaused);

		this->onPause();

		/* Pause the core engine last. */
		m_paused = true;
	}

	void
	Core::resume () noexcept
	{
		if ( !m_paused )
		{
			return;
		}

		/* Pause the core engine first. */
		m_paused = false;

		/* Dispatch the resume event,
		 * first by sending the event,
		 * then directly to the sub application. */
		this->notify(ExecutionResumed);

		this->onResume();
	}

	void
	Core::stop () noexcept
	{
		std::cout << "\n"
			"*******************************" "\n"
			"   Engine is about to stop !   " "\n"
			"*******************************" "\n\n";

		/* Dispatch the stop event,
		 * first by sending the event,
		 * then directly to the sub application. */
		this->notify(ExecutionStopped);

		this->onStop();

		Tracer::debug(ClassId, "User-application stopped !");

		/* Stopping the logics and rendering threads. */
		m_isRenderingLoopRunning = false;
		m_isLogicsLoopRunning = false;

		/* Stopping the main loop. */
		m_isMainLoopRunning = false;
	}

	unsigned int
	Core::terminate () noexcept
	{
		unsigned int error = 0;

		/* Terminate user services. */
		for ( auto * userService : std::ranges::reverse_view(m_userServiceEnabled) )
		{
			if ( userService->terminate() )
			{
				TraceSuccess{ClassId} << userService->name() << " user service terminated gracefully!";
			}
			else
			{
				error++;

				TraceError{ClassId} << userService->name() << " user service failed to terminate properly!";
			}
		}

		m_userServiceEnabled.clear();

		/* NOTE: Release the possible resources held by the user services. */
		m_resourceManager.unloadUnusedResources();

		/* Terminate secondary services. */
		for ( auto * service : std::ranges::reverse_view(m_secondaryServicesEnabled) )
		{
			if ( service->terminate() )
			{
				TraceSuccess{ClassId} << service->name() << " secondary service terminated gracefully!";
			}
			else
			{
				error++;

				TraceError{ClassId} << service->name() << " secondary service failed to terminate properly!";
			}
		}

		m_secondaryServicesEnabled.clear();

		/* NOTE: Release the possible resources held by the secondary services. */
		m_resourceManager.unloadUnusedResources();

		if constexpr ( VulkanTrackingDebugEnabled )
		{
			const auto vkAliveObjectCount = AbstractObject::s_tracking.size();

			if ( vkAliveObjectCount > 0 )
			{
				std::cerr << "[DEBUG:VK_TRACKING] There is " << vkAliveObjectCount << " Vulkan objects not properly destructed !" "\n";

				for ( auto & alloc : AbstractObject::s_tracking )
				{
					std::cerr << "[DEBUG:VK_TRACKING] The Vulkan object @" << alloc.first << " '" << alloc.second << "' still alive !" << "\n";
				}
			}
			else
			{
				std::cout << "[DEBUG:VK_TRACKING] All Vulkan objects has been released !" << "\n";
			}
		}

		return error;
	}

	void
	Core::openFiles (const std::vector< std::filesystem::path > & filepaths) noexcept
	{
		std::vector< std::filesystem::path > verifiedFilepaths;
		verifiedFilepaths.reserve(filepaths.size());

		for ( const auto & filepath : filepaths )
		{
			TraceInfo{ClassId} << "Check file : " << filepath;

			// TODO : Filter files for the application.

			// TODO : If the file is a resource file, read it to complete the resource store.

			verifiedFilepaths.emplace_back(filepath);
		}

		if ( verifiedFilepaths.empty() )
		{
			return;
		}

		this->onOpenFiles(verifiedFilepaths);
	}

	void
	Core::hangExecution (const std::string & command) noexcept
	{
		std::cout << "\n\n\n"
			"******************************************************************\n"
			"	 Execution has been hung ! Press SPACE-BAR to continue ...	\n"
			"******************************************************************\n"
			"\n\n\n";

		/* Execute a command (Only in debug) */
		if constexpr ( IsDebug )
		{
			system(command.c_str());
		}

		while ( true )
		{
			glfwWaitEvents();

			if ( glfwGetKey(m_window.handle(), GLFW_KEY_SPACE) == GLFW_PRESS )
			{
				break;
			}
		}
	}

	bool
	Core::onKeyPress (int32_t key, int32_t scancode, int32_t modifiers, bool repeat) noexcept
	{
		/* NOTE: Pause the engine. */
		if ( !repeat && key == KeyPause )
		{
			if ( m_paused )
			{
				this->resume();
			}
			else
			{
				this->pause();
			}

			return true;
		}

		/* NOTE: Let the user application consume the event. */
		return this->onAppKeyPress(key, scancode, modifiers, repeat);
	}

	bool
	Core::onKeyRelease (int32_t key, int32_t scancode, int32_t modifiers) noexcept
	{
		/* NOTE: When the shift modifiers are pressed,
		 * the core level has the priority to check the key released. */
		if ( isKeyboardModifierPressed(ModKeyShift, modifiers) )
		{
			switch ( key )
			{
				case KeyEscape :
				{
					Tracer::info(ClassId, "Force quit !");

					this->stop();
				}
					return true;

				/* Direct keys reserved by core. */
				case KeyF1 :
					m_sceneManager.withSharedActiveScene([] (const auto & activeScene) {
						if ( activeScene != nullptr )
						{
							TraceInfo{ClassId} <<
								"Scene '" << activeScene->name() << "' content:" "\n" <<
								"Octree System : " "\n" << activeScene->getSectorSystemStatistics(true) <<
								"Static entities : " "\n" << activeScene->getStaticEntitySystemStatistics(true) <<
								"Node entities : " "\n" << activeScene->getNodeSystemStatistics(true) <<
								"Light set : " "\n" << activeScene->lightSet();
						}
						else
						{
							Tracer::info(ClassId, "No active scene !");
						}
					}, false);

					m_audioManager.play("switch_on");
					return true;

				case KeyF2 :
					m_audioManager.play("switch_on");

					this->refreshApplicationSurface();

					return true;

				case KeyF3 :
				{
					const auto bytes = m_resourceManager.memoryOccupied();
					const auto MiB = static_cast< float >(bytes) / 1024.0F / 1024.0F;

					this->notifyUser(BlobTrait{} << "Resource memory loaded : " << bytes << " bytes (" << MiB << " Mib)");
				}

				{
					const auto bytes = m_resourceManager.unusedMemoryOccupied();
					const auto MiB = static_cast< float >(bytes) / 1024.0F / 1024.0F;

					this->notifyUser(BlobTrait{} << "Resource memory wasted : " << bytes << " bytes (" << MiB << " Mib)");
				}
					return true;

				case KeyF4 :
					if ( !m_window.isFullscreenMode() )
					{
						TraceInfo{ClassId} << "Reset window size to default " << DefaultVideoWindowWidth << "X" << DefaultVideoWindowHeight;

						if ( m_window.resize(DefaultVideoWindowWidth, DefaultVideoWindowHeight) )
						{
							m_window.centerPosition();
						}
					}
					return true;

				case KeyF5 :
				{
					auto & settings = this->primaryServices().settings();

					this->notifyUser("Opening settings file ...");

					/* NOTE: First, disable the settings auto-save to prevent erasing possible changes made by the user. */
					settings.saveAtExit(false);

					PlatformSpecific::Desktop::openTextFile(settings, settings.filepath());
				}
					return true;

				case KeyF6 :
				{
					this->notifyUser("Showing application configuration directory ...");

					PlatformSpecific::Desktop::openFolder(this->primaryServices().fileSystem().configDirectory());
				}
					return true;

				case KeyF7 :
				{
					this->notifyUser("Showing application cache directory ...");

					PlatformSpecific::Desktop::openFolder(this->primaryServices().fileSystem().cacheDirectory());
				}
					return true;

				case KeyF8 :
				{
					this->notifyUser("Showing application user data directory ...");

					PlatformSpecific::Desktop::openFolder(this->primaryServices().fileSystem().userDataDirectory());
				}
					return true;

				case KeyF9 :
				{
					this->notifyUser("Cleaning unused resources ...");

					m_audioManager.play("switch_on");

					const auto count = m_resourceManager.unloadUnusedResources();

					TraceInfo{ClassId} << count << " resources cleaned !";
				}
					return true;

				case KeyF10 :
				{
					this->notifyUser("Core engine paused for 3 seconds ...");

					std::this_thread::sleep_for(std::chrono::seconds(3));

					this->notifyUser("Core engine wake-up.");
				}
					return true;

				case KeyF11 :
				{
					this->notifyUser("Toggling fullscreen mode ...");

					if ( m_window.isFullscreenMode() )
					{
						m_window.switchToWindowedMode();
					}
					else
					{
						m_window.switchToFullscreenMode();
					}
				}

					return true;

				case KeyF12 :
					this->notifyUser("Core reserved SHIFT+F12 shortcut hit.");

					return true;

				case KeyR :
					Tracer::info(ClassId, "Toggling offscreen rendering ...");

					m_graphicsRenderer.toggleOffscreenRendering();

					return true;

				case KeyPad1 :
					m_window.setGamma(0.8F);
					return true;

				case KeyPad2 :
					m_window.setGamma(1.0F);
					return true;

				case KeyPad3 :
					m_window.setGamma(1.2F);
					return true;

				default:
					break;
			}
		}

		/* NOTE: Let the child application looks for the key pressed. */
		if ( this->onAppKeyRelease(key, scancode, modifiers) )
		{
			return true;
		}

		/* NOTE: If the application does not catch any key, we let the core having a default behavior. */
		if ( m_preventDefaultKeyBehaviors )
		{
			return false;
		}

		switch ( key )
		{
			case KeyGraveAccent :
				// TODO: Re-enable this method
				//m_console.enable();

				return true;

			case KeyEscape :
				this->stop();

				return true;

			default:
				return false;
		}
	}

	bool
	Core::onCharacterType (uint32_t unicode) noexcept
	{
		/* NOTE: Let the user application consume the event. */
		return this->onAppCharacterType(unicode);
	}

	bool
	Core::screenshot (bool /*depth*/, bool /*stencil*/) noexcept
	{
		/* Gets the capture directory. */
		const auto captures = m_primaryServices.fileSystem().userDataDirectory("Captures");

		if ( !IO::writable(captures) )
		{
			TraceError{ClassId} << "Unable to write in captures directory '" << captures << "' !";

			return false;
		}

		/* FIXME: TODO with vulkan */

		return true;
	}

	bool
	Core::dumpFramebuffer () const noexcept
	{
		/* Framebuffer dumping directory. */
		const auto directory = m_primaryServices.fileSystem().userDataDirectory("framebuffer-snapshots");

		if ( !IO::directoryExists(directory) )
		{
			if ( !IO::createDirectory(directory) )
			{
				TraceError{ClassId} << "Unable to create the directory '" << directory << "' !";

				return false;
			}
		}
		else
		{
			if ( !IO::writable(directory) )
			{
				TraceError{ClassId} << "Unable to write into directory '" << directory << "' !";

				return false;
			}
		}

		return true;
	}

	bool
	Core::onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept
	{
		if ( observable == &m_consoleController )
		{
			switch ( notificationCode )
			{
				case Console::Controller::Exit :

					this->stop();

					break;

				case Console::Controller::HardExit :
					/* NOTE: Hard cord termination of the program! */
					std::terminate();

					break;

				default:
					if constexpr ( ObserverDebugEnabled )
					{
						TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << Console::Controller::ClassId << "' ignored.";
					}
					break;
			}

			return true;
		}

		if ( observable == &m_primaryServices.netManager() )
		{
			TraceDebug{ClassId} << "Receiving an event from '" << Net::Manager::ClassId << "' (code:" << notificationCode << ") ...";

			return true;
		}

		if ( observable == &m_window )
		{
			switch ( notificationCode )
			{
				case Window::OSNotifiesWindowGetFocus :
				case Window::OSNotifiesWindowVisible :
					this->resume();
					break;

				case Window::OSNotifiesWindowLostFocus :
				case Window::OSNotifiesWindowHidden :
					if ( m_pausable )
					{
						this->pause();
					}
					break;

				case Window::OSRequestsToTerminate :
					this->stop();
					break;

				default :
					if constexpr ( ObserverDebugEnabled )
					{
						TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << Window::ClassId << "' ignored.";
					}
					break;
			}

			return true;
		}

		if ( observable == &m_inputManager )
		{
			TraceDebug{ClassId} << "Receiving an event from '" << Input::Manager::ClassId << "' (code:" << notificationCode << ") ...";

			return true;
		}

		if ( observable == &m_graphicsRenderer.shaderManager() )
		{
			switch ( notificationCode )
			{
				case Saphir::ShaderManager::ShaderCompilationSucceed :
					this->notifyUser(BlobTrait{} << "Shader '" << std::any_cast< std::string >(data) << "' compilation succeeded!");
					break;

				case Saphir::ShaderManager::ShaderCompilationFailed :
				{
					const auto [identifier, sourceCode] = std::any_cast< std::pair< std::string, std::string > >(data);

					this->onShaderCompilationFailed(identifier, sourceCode);
				}
					break;

				default:
					break;
			}

			return true;
		}

		if ( observable == &m_audioManager.trackMixer() )
		{
			TraceDebug{ClassId} << "Receiving an event from '" << Audio::TrackMixer::ClassId << "' (code:" << notificationCode << ") ...";

			switch ( notificationCode )
			{
				case Audio::TrackMixer::MusicPlaying :
				case Audio::TrackMixer::MusicSwitching :
					this->notifyUser(std::any_cast< std::string >(data));
					break;

				case Audio::TrackMixer::MusicPaused :
					this->notifyUser("Music paused !");
					break;

				case Audio::TrackMixer::MusicResumed :
					this->notifyUser("Music resumed !");
					break;

				case Audio::TrackMixer::MusicStopped :
					this->notifyUser("Music stopped !");
					break;

				default:
					break;
			}

			return true;
		}

		if ( observable == &m_sceneManager )
		{
			switch ( notificationCode )
			{
				case Scenes::Manager::SceneDestroyed :
					m_resourceManager.unloadUnusedResources();
					break;

				case Scenes::Manager::SceneCreated :
				case Scenes::Manager::SceneLoaded :
				case Scenes::Manager::SceneEnabled :
				case Scenes::Manager::SceneDisabled :
				default :
					TraceDebug{ClassId} << "Receiving an event from '" << Scenes::Manager::ClassId << "' (code:" << notificationCode << ") ...";
					break;
			}

			return true;
		}

		return this->onAppNotification(observable, notificationCode, data);
	}

	bool
	Core::executeToolsMode () noexcept
	{
		Tracer::info(ClassId, "Executing in tools mode ...");

		const auto tools = m_primaryServices.arguments().get(ToolsArg, ToolsLongArg).value();

		if ( tools == VulkanInformationToolName )
		{
			Tool::ShowVulkanInformation tool{m_primaryServices.arguments(), m_vulkanInstance};

			return tool.execute();
		}

		if ( tools == PrintGeometryToolName )
		{
			Tool::GeometryDataPrinter tool{m_primaryServices.arguments()};

			return tool.execute();
		}

		if ( tools == ConvertGeometryToolName )
		{
			TraceDebug{ClassId} << "FIXME: ...";

			return true;
		}

		TraceWarning{ClassId} << "Unrecognized tools '" << tools << "' !";

		return false;
	}

	void
	Core::showStatistics () const noexcept
	{
		const auto & stats = m_graphicsRenderer.statistics();

		std::cout <<
			"[TIME:" << std::setw(5) << std::setfill(' ') << std::right << Time::elapsedSeconds() << " s]"
			"[FPS:" << std::setw(4) << std::setfill(' ') << std::right << stats.executionsPerSecond() << ", " << std::setw(3) << std::setfill(' ') << std::right << stats.duration() << " ms]"
			"[FPS-AVG: " << std::setw(10) << std::setfill(' ') << std::right << stats.averageExecutionsPerSecond() << ", " << std::setw(10) << std::setfill(' ') << std::right << stats.averageDuration() << " ms]" << '\n';
	}
}
