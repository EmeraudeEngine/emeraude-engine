/*
 * src/Core.cpp
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

#include "Core.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>
#include <string_view>
#include <thread>

/* Third-party inclusions. */
#include "GLFW/glfw3.h"
#ifdef IMGUI_ENABLED
#include "imgui.h"
#include "Overlay/ImGUIScreen.hpp"
#endif

/* Local inclusions. */
#include "Arguments.hpp"
#include "Audio/MusicResource.hpp"
#include "Constants.hpp"
#include "FastJSON.hpp"
#include "FileSystem.hpp"
#include "Graphics/Effects/Framebuffer/DepthOfField.hpp"
#include "Graphics/Effects/Framebuffer/ToneMapping.hpp"
#include "Graphics/Photometry.hpp"
#include "Graphics/PostProcessStack.hpp"
#include "Scenes/Component/Camera.hpp"
#include "Scenes/Loaders/Interface.hpp"
#include "Scenes/Viewers/ImageViewer.hpp"
#include "Scenes/Viewers/ModelViewer.hpp"
#include "Input/KeyboardListenerInterface.hpp"
#include "Input/Types.hpp"
#include "IO/IO.hpp"
#include "Locale.hpp"
#include "Net/Manager.hpp"
#include "Time/Elapsed/PrintScopeRealTime.hpp"
#include "Time/Time.hpp"
#include "Version.hpp"
#include "PlatformSpecific/Desktop/Commands.hpp"
#include "PlatformSpecific/Desktop/Dialog/CustomMessage.hpp"
#include "PlatformSpecific/Desktop/Dialog/Message.hpp"
#include "SettingKeys.hpp"
#include "Tool/GeometryDataPrinter.hpp"
#include "Tool/ShowVulkanInformation.hpp"

namespace EmEn
{
	using namespace Base;
	using namespace Vulkan;
	using namespace Graphics;
	using namespace Scenes;
	using namespace Input;
	using namespace Resources;

	Core::Core (int argc, char * * argv, const char * applicationName, const Version & applicationVersion, const char * applicationOrganization, const char * applicationDomain, bool resetSettingsOnNewVersion) noexcept
		: KeyboardListenerInterface{false, false},
		ControllableTrait{ClassId},
		m_identification{applicationName, applicationVersion, applicationOrganization, applicationDomain},
		m_primaryServices{argc, argv, m_identification},
		m_resetSettingsOnNewVersion{resetSettingsOnNewVersion}
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
	Core::Core (int argc, wchar_t * * wargv, const char * applicationName, const Version & applicationVersion, const char * applicationOrganization, const char * applicationDomain, bool resetSettingsOnNewVersion) noexcept
		: KeyboardListenerInterface{false, false},
		ControllableTrait{ClassId},
		m_identification{applicationName, applicationVersion, applicationOrganization, applicationDomain},
		m_primaryServices{argc, wargv, m_identification},
		m_resetSettingsOnNewVersion{resetSettingsOnNewVersion}
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
		constexpr std::chrono::duration< uint64_t, std::micro > logicsUpdateFrequency{WorldPhysicsUpdateCycleDurationUS< uint64_t >};

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
				const Time::Elapsed::PrintScopeRealTimeThreshold stat{"logicsTask", WorldPhysicsUpdateCycleDurationMS< double >};

				/* Core-application cyclic update on the active scene only.
				 * NOTE: It will ask for a shared-access to the active scene
				 * preventing locking the "rendering thread" for updating the scene logic. */
				m_sceneManager.withSharedActiveScene([&] (const auto & activeScene ) {
					/* This should run "freely" on one thread. */
					activeScene->processLogics(m_cycle, m_physicalSimulationEnabled);

					/* Tells the system one snapshot of the logic is ready to be synced. */
					activeScene->publishStateForRendering();
				}, true);

				/* Editor logic update (gizmo scale, hover). */
				m_sceneManager.editorManager().processLogics();

				/* User-application cyclic update. */
				this->onCoreProcessLogics(m_cycle);

				m_lifetime += WorldPhysicsUpdateCycleDurationUS< uint64_t >;
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
	Core::requestRedraw () noexcept
	{
		/* NOTE: In continuous mode the rendering thread never sleeps, so there is nothing to
		 * wake and nothing to budget. */
		if ( m_renderingMode != RenderingMode::OnDemand )
		{
			return;
		}

		/* NOTE: Budget enough frames to refresh every swap-chain image, otherwise a
		 * multi-buffered swap-chain would keep re-displaying a stale buffer for the images
		 * we did not redraw. framesInFlight() is sized from the swap-chain image count. */
		const auto budget = std::max< uint32_t >(1, m_graphicsRenderer.framesInFlight());

		{
			/* NOTE: Mutate the shared budget under the same mutex the rendering thread waits on,
			 * to close the lost-wakeup window between the predicate test and the wait. */
			const std::scoped_lock lock{m_redrawMutex};

			m_pendingFrames.store(budget, std::memory_order_relaxed);
		}

		m_redrawCondition.notify_one();
	}

	void
	Core::renderingTask () noexcept
	{
		uint64_t frames = 0;

		/* NOTE: On-demand safety re-check period (one 60 FPS frame). On timeout the thread merely
		 * re-evaluates the predicate; it does not force a frame, so the GPU stays idle at rest. */
		constexpr std::chrono::duration< double > onDemandTimeout{1.0 / OnDemandRenderingSafetyRefreshHz< double >};

		while ( m_isRenderingLoopRunning )
		{
			/* NOTE: Check the pause flag BEFORE taking the lock.
			 * This prevents deadlock: the main thread sets m_paused=true, then waits for
			 * the lock. If we check m_paused first, we release quickly without blocking
			 * on potentially long Vulkan operations while holding the lock. */
			if ( m_paused )
			{
				/* NOTE: Fake a frame rendering time based on an ideal 30 FPS rendering. */
				std::this_thread::sleep_for(std::chrono::milliseconds{33});

				continue;
			}

			/* NOTE: On-demand rendering gate. When enabled and there is no active 3D scene, the
			 * rendering thread sleeps until a redraw is requested (overlay/WebView repaint, window
			 * event, scene enable/disable, or an explicit requestRedraw()) instead of rendering every
			 * iteration. An active scene bypasses the gate entirely (v1: an active scene is treated as
			 * always-dirty and renders continuously). The wait uses a 60 FPS-period safety timeout so a
			 * missed dirty signal self-heals within one frame rather than freezing the display. */
			if ( m_renderingMode == RenderingMode::OnDemand && !m_sceneManager.hasActiveScene() )
			{
				std::unique_lock< std::mutex > lock{m_redrawMutex};

				if ( m_pendingFrames.load(std::memory_order_relaxed) == 0 )
				{
					m_redrawCondition.wait_for(lock, onDemandTimeout, [this] {
						return m_pendingFrames.load(std::memory_order_relaxed) > 0 || !m_isRenderingLoopRunning;
					});
				}

				/* NOTE: Woke on the safety timeout with nothing to draw (and still no scene): idle again. */
				if ( m_pendingFrames.load(std::memory_order_relaxed) == 0 )
				{
					continue;
				}
			}

			/* NOTE: Ask for a shared-access to the scene content preventing to lock the "logic thread" and draw the scene. */
			m_sceneManager.withSharedActiveScene([&] (const auto & activeScene) {
				if ( m_graphicsRenderer.isShutdownRequested() )
				{
					return;
				}

				/* Expose the frame's scene to the overlay screens drawn inside renderFrame():
				 * they run on THIS thread, inside THIS shared lock, and must not re-acquire it
				 * (recursive shared_mutex = UB, deadlocks on Windows once a writer queues). */
				m_frameScene = activeScene.get();

				/* ⚠️⚠️ The scene's jitter and UBO upload used to happen HERE, before renderFrame() —
				 * i.e. before the frame's in-flight fence had been waited on. Every buffer they
				 * write is single-instance, so the host was overwriting memory that up to
				 * framesInFlight() - 1 still-executing frames were reading. They now live inside
				 * Renderer::renderFrame() / renderOffscreenFrame(), after the fence and after
				 * Scene::beginRenderFrame() has latched the frame's read state index.
				 * The overlay stays here on purpose: it writes surface IMAGES through the transfer
				 * manager (a staged transfer, a different hazard class) and already sizes per frame. */

				/* This should only synchronize UBOs for the overlay. */
				m_overlayManager.updateVideoMemory();

				/* Render the scene (optional), editor gizmos, and the overlay on top. */
				const auto * editorPtr = m_sceneManager.editorManager().isActive() ? &m_sceneManager.editorManager() : nullptr;

				if ( m_graphicsRenderer.isWindowLess() )
				{
					m_graphicsRenderer.renderOffscreenFrame(activeScene, m_overlayManager, editorPtr);
				}
				else
				{
					m_graphicsRenderer.renderFrame(activeScene, m_overlayManager, editorPtr);
				}

				if ( m_graphicsRenderer.recorder().isRecording() && m_graphicsRenderer.recorder().shouldCaptureFrame() )
				{
					m_graphicsRenderer.recorder().captureAndSubmitFrame();
				}

				/* The frame scope ends with the shared lock: past this point the pointer
				 * would outlive the guarantee. */
				m_frameScene = nullptr;
			}, false);

			/* NOTE: Consume one frame from the on-demand budget. Harmless in continuous mode
			 * (the budget stays at zero) and while a scene is active (the gate is bypassed). */
			if ( m_pendingFrames.load(std::memory_order_relaxed) > 0 )
			{
				m_pendingFrames.fetch_sub(1, std::memory_order_relaxed);
			}

			frames++;
		}

		/* NOTE: Wait until the device has finished all his pending work. */
		m_graphicsRenderer.device()->waitIdle("Core::renderingTask()");

		Tracer::success(ClassId, "[THREAD] Rendering process terminated successfully !");

		TraceInfo{ClassId} <<
			"The rendering produced " << frames << " frames." "\n"
			"Pipelines statistics : " << m_graphicsRenderer.pipelineBuiltCount() << " built during, " << m_graphicsRenderer.pipelineReusedCount() << " were re-used." "\n"
			"Programs statistics : " << m_graphicsRenderer.programBuiltCount() << " built during, " << m_graphicsRenderer.programsReusedCount() << " were re-used.";
	}

	void
	Core::onWindowChanged () noexcept
	{
		if ( !m_overlayManager.onWindowResized() )
		{
			Tracer::error(ClassId, "Unable to resize the overlay manager!");
		}

		this->onCoreSurfaceRefreshed();

		m_windowChanged = false;

		if ( m_graphicsRenderer.recorder().isRecording() )
		{
			TraceInfo{ClassId} << "Stopping recording due to framebuffer resize.";

			this->stopAudioVideoRecording();
		}

		/* NOTE: The surface was recreated (resize, scale change): force a redraw so the new
		 * framebuffer is repainted in on-demand mode. No-op in continuous mode. */
		this->requestRedraw();

		this->notify(SurfaceRefreshed);
	}

	void
	Core::updatePointerScaling () noexcept
	{
		/* NOTE: Decide whether the cursor must be scaled to physical pixels. Some windowing systems
		 * report it in logical/DIP coordinates, which would mismatch the physical framebuffer
		 * dimensions used by the overlay hit-testing (Overlay::Surface::isBelowPoint / isEventBlocked).
		 *  - macOS: cursor in screen points (DIP) -> always scale.
		 *  - Linux/Wayland: GLFW reports logical surface coordinates (fractional-scale aware) -> scale.
		 *  - Linux/X11, Windows: cursor already in physical pixels -> no scaling. */
		bool requiresScaling = false;

		if constexpr ( IsMacOS )
		{
			requiresScaling = true;
		}

		if constexpr ( IsLinux )
		{
			requiresScaling = PlatformManager::isUsingWayland();
		}

		if ( requiresScaling )
		{
			/* NOTE: Use the surface content scale (glfwGetWindowContentScale), i.e. the same factor the
			 * framebuffer uses (1.5 for a 150% fractional scale) - NOT the per-monitor integer scale. */
			m_inputManager.enablePointerScaling(m_window.state().contentXScale, m_window.state().contentYScale);
		}
		else
		{
			m_inputManager.disablePointerScaling();
		}
	}

	int
	Core::run () noexcept
	{
		switch ( m_startupMode )
		{
			case StartupMode::Error :
				Tracer::fatal(ClassId, "Engine startup has been aborted by the core constructor !");

				return EXIT_FAILURE;

			case StartupMode::ToolsMode :
				return this->executeToolsMode();

			case StartupMode::Continue :
				/* NOTE: Let the application stop the run. */
				if ( this->onBeforeCoreSecondaryServicesInitialization() )
				{
					return this->terminate();
				}

				/* NOTE: Finish the core initialization with secondary services. */
				if ( !this->initializeCoreLevel() )
				{
					return EXIT_FAILURE;
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
		if ( this->onCoreStarted(m_primaryServices.arguments(), m_primaryServices.settings()) )
		{
			Tracer::success(ClassId, "The application successfully started.");

			/* Dispatch the entering in main loop event,
			 * first by sending the event,
			 * then directly to the sub application. */
			this->notify(EnteringMainLoop);
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
			 * DirectInput: Copy the state of every input device to use it in the engine cycle.
			 * NOTE: The regular timeout is bounded by a pending scheduleMainLoopCycle() deadline. */
			m_inputManager.waitSystemEvents(this->mainLoopWaitTimeout(m_mainLoopEventTimeoutSeconds));

			/* NOTE: Check if the graphics render do not have a problem. */
			if ( m_graphicsRenderer.usable() )
			{
				/* NOTE: Must be done on the main thread. */
				if ( m_windowChanged )
				{
					this->onWindowChanged();
				}
			}
			else
			{
				/* ... If so, we stop nicely here, letting the chance to the user application to save data. */
				this->stop();
			}

			this->executeMainLoopCycle();

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
					this->executeMainLoopCycle();

					/* NOTE: The indefinite wait is bounded by a pending scheduleMainLoopCycle()
					 * deadline, so external pumps (e.g., CEF UI) stay serviced while paused. */
					m_inputManager.waitSystemEvents(this->mainLoopWaitTimeout(0.0));
				}

				/* Restart all timers of the active scene. */
				m_sceneManager.withExclusiveActiveScene([&] (const auto & activeScene) {
					activeScene->resumeTimers();
				}, true);
			}

			if ( m_enableStatistics )
			{
				auto currentTime = std::chrono::steady_clock::now();

				if ( auto elapsedTime = currentTime - lastTop; elapsedTime >= std::chrono::seconds(1) )
				{
					const auto & stats = m_graphicsRenderer.statistics();

					std::cout <<
						"[TIME:" << std::setw(5) << std::setfill(' ') << std::right << Time::elapsedSeconds() << " s]"
						"[FPS:" << std::setw(4) << std::setfill(' ') << std::right << stats.executionsPerSecond() << ", " << std::setw(3) << std::setfill(' ') << std::right << stats.duration() << " ms]"
						"[FPS-AVG: " << std::setw(10) << std::setfill(' ') << std::right << stats.averageExecutionsPerSecond() << ", " << std::setw(10) << std::setfill(' ') << std::right << stats.averageDuration() << " ms]" << '\n';

					lastTop = currentTime;
				}
			}

			if constexpr ( IsDebug )
			{
				if ( !m_coreMessages.empty() )
				{
					this->displayCoreMessages();
				}
			}
		}

		/* Dispatch the exiting the main loop event,
		 * first by sending the event,
		 * then directly to the sub application. */
		this->notify(ExitingMainLoop);

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

		return this->terminate();
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

		dialog.execute(this->window());

		this->resume();
	}

	bool
	Core::initializeBaseLevel () noexcept
	{
		/* The engine serialises floats through the C numeric family (settings, scene files, cache
		 * keys, generated shader code), so it states the numeric locale it needs instead of hoping
		 * nobody changed it: in a comma-decimal locale (fr_BE, fr_FR, de_DE …) every written float
		 * changes format and strtod() stops parsing at the dot. See Base::Locale::enforceNumericC().
		 * ⚠️ This call covers the engine's own bring-up ONLY. A library initialised later can still
		 * change it — CEF is initialised AFTER the engine and Chromium does call
		 * setlocale(LC_ALL, ""), so an application embedding such a library must re-assert it at
		 * that point. */
		if ( Base::Locale::enforceNumericC() )
		{
			TraceWarning{ClassId} << "The C numeric locale was not 'C' when the engine started: something already called setlocale() in this process. Restored, but any float parsed or written before this point may have used a comma as decimal separator.";
		}

		std::cout << "\n"
			"Engine	  : " << m_identification.engineId() << "\n"
			"Application : " << m_identification.applicationId() << "\n"
			"Reverse ID  : " << m_identification.applicationReverseId() << "\n\n";

		/* Registering core help. */
		{
			m_coreHelp.registerArgument("Show this help.", "help", 'h');
			m_coreHelp.registerArgument("Show information about the Core system in terminal.", "show-core-infos");
			m_coreHelp.registerArgument("Show information about the input system in terminal.", "show-input-infos");
			m_coreHelp.registerArgument("Show information about the resource system in terminal.", "show-resources-infos");
			m_coreHelp.registerArgument("Show information about the audio system in terminal.", "show-audio-infos");
			m_coreHelp.registerArgument("Show information about the video system in terminal.", "show-video-infos");
			m_coreHelp.registerArgument("Show information about all systems in terminal.", "show-all-infos");
			m_coreHelp.registerArgument("Disable the window for server version.", "window-less", 'W');
			m_coreHelp.registerArgument("Disable audio layer.", "disable-audio");
			m_coreHelp.registerArgument("Display only logs which tags appears. TAG is a list of words separated by comma.", "filter-tag", 't', {"TAG"});
			m_coreHelp.registerArgument("Set a custom core settings file. FILE_PATH is where to get the settings file and should be writable.", "settings-filepath", 0, {"FILE_PATH"});
			m_coreHelp.registerArgument("Set a custom core settings filename. FILE_NAME is an alternate name of the 'settings.json' file.", "settings-filepath", 0, {"FILE_NAME"});
			m_coreHelp.registerArgument("Disable the generation or the saving of settings files.", "disable-settings-autosave");
			m_coreHelp.registerArgument("Enable log writing.", "enable-log", 'l', {"FILE_PATH"});
			m_coreHelp.registerArgument("Add a custom data directory.", "add-data-directory", 0, {"DIRECTORY_PATH"});
			m_coreHelp.registerArgument("Force the use of a cache directory overriding every others.", "cache-directory", 0, {"DIRECTORY_PATH"});
			m_coreHelp.registerArgument("Force the use of a config directory overriding every others.", "config-directory", 0, {"DIRECTORY_PATH"});
			m_coreHelp.registerArgument("Force the use of a data directory overriding every others.", "data-directory", 0, {"DIRECTORY_PATH"});
			m_coreHelp.registerArgument("Execute a specific tool.", "tools-mode", 't');
			m_coreHelp.registerArgument("List local data that would be wiped (dry run). Settings are preserved.", "wipe-local-data");
			m_coreHelp.registerArgument("Wipe all local data (cache and user data directories). Settings are preserved. The application exits immediately.", "wipe-local-data-confirm");
			m_coreHelp.registerArgument("Backup and reset the settings file. The application exits immediately.", "reset-settings");

			m_coreHelp.registerShortcut("Quit the application.", KeyEscape, ModKeyShift);
			m_coreHelp.registerShortcut("Print the active scene content in console.", KeyF1, ModKeyShift);
			m_coreHelp.registerShortcut("Toggle the physical camera panel (requires ImGui).", KeyF2, ModKeyShift);
			m_coreHelp.registerShortcut("Toggle scene editor mode.", KeyF3, ModKeyShift);
			m_coreHelp.registerShortcut("Reset the window size to defaults.", KeyF4, ModKeyShift);
			m_coreHelp.registerShortcut("Open settings file in text editor.", KeyF5, ModKeyShift);
			m_coreHelp.registerShortcut("Open file explorer to application configuration directory.", KeyF6, ModKeyShift);
			m_coreHelp.registerShortcut("Open file explorer to application cache directory.", KeyF7, ModKeyShift);
			m_coreHelp.registerShortcut("Open file explorer to application user data directory.", KeyF8, ModKeyShift);
			m_coreHelp.registerShortcut("Clean up unused resources from managers.", KeyF9, ModKeyShift);
			m_coreHelp.registerShortcut("Suspend core thread execution for 3 seconds.", KeyF10, ModKeyShift);
			m_coreHelp.registerShortcut("Toggle the window fullscreen mode.", KeyF11, ModKeyShift);
			m_coreHelp.registerShortcut("Take a screenshot.", KeyF12, ModKeyShift);
			m_coreHelp.registerShortcut("Toggle video recording.", KeyF12, ModKeyShift | ModKeyControl);
			m_coreHelp.registerShortcut("Trigger a RenderDoc frame capture.", KeyC, ModKeyShift);
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

		if ( m_primaryServices.arguments().isSwitchPresent(WipeLocalDataConfirmArg) )
		{
			this->executeWipeLocalData(false);

			m_willNotRun = true;

			return true;
		}

		if ( m_primaryServices.arguments().isSwitchPresent(WipeLocalDataArg) )
		{
			this->executeWipeLocalData(true);

			m_willNotRun = true;

			return true;
		}

		if ( m_primaryServices.arguments().isSwitchPresent(ResetSettingsArg) )
		{
			this->executeResetSettings();

			m_willNotRun = true;

			return true;
		}

		/* NOTE: Per-project settings version guard. The settings file has just been loaded by
		 * m_primaryServices.initialize(); if the project opted in and the file predates the current
		 * engine version, back it up and clear the in-memory store so we keep running on a clean base
		 * (no restart needed, unlike --reset-settings which exits). */
		if ( m_resetSettingsOnNewVersion )
		{
			this->resetSettingsIfOutdated();
		}

		/* Initialize the console. */
		if ( m_consoleController.initialize(m_primaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_consoleController.name() << " service up !";

			this->registerToConsole();

			m_primaryServices.arguments().registerToObject(*this);
			m_primaryServices.fileSystem().registerToObject(*this);
			m_primaryServices.settings().registerToObject(*this);

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
			m_resourceManager.registerToObject(*this);

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
		if (
			m_primaryServices.settings().getOrSetDefault< bool >(CoreShowInformationKey, DefaultCoreShowInformation) ||
			m_primaryServices.arguments().isSwitchPresent("--show-all-infos") ||
			m_primaryServices.arguments().isSwitchPresent("--show-core-infos")
		)
		{
			Tracer::info(ClassId, m_primaryServices.information());
		}

		if (
			m_primaryServices.settings().getOrSetDefault< bool >(CoreEnableStatisticsKey, DefaultCoreEnableStatistics) ||
			m_primaryServices.arguments().isSwitchPresent("--show-rendering-stats")
		)
		{
			Tracer::info(ClassId, "Graphics rendering statistics enabled.");

			m_enableStatistics = true;
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
			TraceSuccess{ClassId} << m_platformManager.name() << " service up!";
		}
		else
		{
			TraceFatal{ClassId} << m_platformManager.name() << " service failed to execute!";

			return false;
		}

		/* Initialize the vulkan API. */
		if ( m_vulkanInstance.initialize(m_secondaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_vulkanInstance.name() << " service up!";
		}
		else
		{
			TraceFatal{ClassId} << m_vulkanInstance.name() << " service failed to execute!";

			return false;
		}

		/* Initialize the handle. */
		if ( m_window.initialize(m_secondaryServicesEnabled) )
		{
			m_window.registerToObject(*this);

			this->observe(&m_window);

			TraceSuccess{ClassId} << m_window.name() << " service up!";
		}
		else
		{
			TraceFatal{ClassId} << m_window.name() << " service failed to execute!";

			return false;
		}

		/* Initialization of the input manager. */
		if ( m_inputManager.initialize(m_secondaryServicesEnabled) )
		{
			m_inputManager.registerToObject(*this);

			/* Configure the input manager. */
			m_inputManager.enableKeyboardListening(true);
			m_inputManager.enablePointerListening(true);
			m_inputManager.enableCopyKeyboardState(true);
			m_inputManager.enableCopyPointerState(true);

			/* Adds Core keyboard listener to the input manager. */
			m_inputManager.addKeyboardListener(this);

			/* NOTE: Scale the cursor to physical pixels on windowing systems that report it in
			 * logical/DIP coordinates (macOS, Linux/Wayland), so it stays consistent with the
			 * framebuffer dimensions used by the overlay hit-testing. Refreshed on scale changes
			 * via Window::OSRequestsToRescaleContentBy (see onNotification). */
			this->updatePointerScaling();

			this->observe(&m_inputManager);

			TraceSuccess{ClassId} << m_inputManager.name() << " service up!";
		}
		else
		{
			TraceFatal{ClassId} << m_inputManager.name() << " service failed to execute!";

			return false;
		}

		/* Initialize graphics renderer. */
		if ( m_graphicsRenderer.initialize(m_secondaryServicesEnabled) )
		{
			m_graphicsRenderer.createDefaultResources(m_resourceManager);
			m_graphicsRenderer.registerToObject(*this);

			this->observe(&m_graphicsRenderer);
			this->observe(&m_graphicsRenderer.shaderManager());

			TraceSuccess{ClassId} << m_graphicsRenderer.name() << " service up!";
		}
		else
		{
			TraceFatal{ClassId} << m_graphicsRenderer.name() << " service failed to execute!";

			return false;
		}

		/* Initialize physics manager. */
		if ( m_physicsManager.initialize(m_secondaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_physicsManager.name() << " service up!";
		}
		else
		{
			TraceWarning{ClassId} <<
				m_physicsManager.name() << " service failed to execute!" "\n"
				"No physics acceleration available!";
		}

		/* Initialize audio manager. */
		if ( m_audioManager.initialize(m_secondaryServicesEnabled) )
		{
			m_audioManager.registerToObject(*this);

			this->observe(&m_audioManager.trackMixer());

			TraceSuccess{ClassId} << m_audioManager.name() << " service up!";
		}
		else
		{
			TraceWarning{ClassId} << m_audioManager.name() << " service failed to execute, no audio available!";
		}

		/* Initialize system notification service (OS-level notifications). */
		if ( m_systemNotification.initialize(m_secondaryServicesEnabled) )
		{
			TraceSuccess{ClassId} << m_systemNotification.name() << " service up!";
		}
		else
		{
			TraceWarning{ClassId} << m_systemNotification.name() << " service failed to execute, no system notifications available!";
		}

		/* Initialization of the overlay manager. */
		if ( m_overlayManager.initialize(m_secondaryServicesEnabled) )
		{
			/* NOTE: Observe the overlay manager so its RedrawRequested notifications (any visual
			 * mutation of a screen or surface) drive the on-demand rendering thread. */
			this->observe(&m_overlayManager);

			m_overlayManager.enable(m_inputManager, true);

			TraceSuccess{ClassId} << m_overlayManager.name() << " service up!";

			if ( !m_disableNotifier )
			{
				/* Initialization of the notifier. */
				if ( m_notifier.initialize(m_secondaryServicesEnabled) )
				{
					TraceSuccess{ClassId} << m_notifier.name() << " service up!";
				}
				else
				{
					TraceError{ClassId} << m_notifier.name() << " service failed to execute!";
				}
			}

			/* Initialization of the core screen. */
			if ( !this->initializeCoreScreen() )
			{
				Tracer::warning(ClassId, "Unable to create Core screens for information's and basic functions!");
			}
		}
		else
		{
			TraceFatal{ClassId} << m_overlayManager.name() << " service failed to execute!";

			return false;
		}

		/* Initialize scene manager. */
		if ( m_sceneManager.initialize(m_secondaryServicesEnabled) )
		{
			m_sceneManager.registerToObject(*this);

			this->observe(&m_sceneManager);

			/* Register JSON scene handler on the console controller. */
			m_consoleController.setJsonHandler([this] (const std::string & json, Console::Outputs & outputs) {
				return m_sceneManager.loadSceneFromJson(json, outputs);
			});

			TraceSuccess{ClassId} << m_sceneManager.name() << " service up!";
		}
		else
		{
			TraceError{ClassId} << m_sceneManager.name() << " service failed to execute!";

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
			TraceError{ClassId} << userService->name() << " user service failed to execute!";

			return false;
		}

		TraceSuccess{ClassId} << userService->name() << " user service up!";

		return true;
	}

	bool
	Core::initializeCoreScreen () noexcept
	{
#ifdef IMGUI_ENABLED
		const auto screen = m_overlayManager.createImGUIScreen("CoreScreen", []  {
			bool show = true;
			ImGui::ShowDemoWindow(&show);
		});

		screen->setVisibility(false);

		/* PHYSICAL CAMERA panel, toggled with Shift+F2. The camera is the single source of truth
		 * for the photographic behaviour of the image — optics, exposure triad, and the effects it
		 * materializes — so a panel bound to it replaces a rebuild for every tweak. The EV100
		 * readout at the bottom is the point: it makes the physics legible while you drag. */
		m_cameraScreen = m_overlayManager.createImGUIScreen("PhysicalCameraScreen", [this] {
			if ( !ImGui::Begin("Physical camera") )
			{
				ImGui::End();

				return;
			}

			/* This callback runs on the RENDER THREAD, inside the frame's shared-scene scope
			 * (Overlay::Manager::render() is called from Renderer::renderFrame(), itself inside
			 * withSharedActiveScene). Re-locking the scene manager here would re-acquire a
			 * shared_mutex this thread already share-owns — UB, and a real deadlock on Windows
			 * SRWLOCK once a writer queues. The frame's scene is read from m_frameScene, the
			 * pointer Core publishes for exactly this scope. */
			[] (Scenes::Scene * const activeScene) {
				if ( activeScene == nullptr )
				{
					ImGui::TextUnformatted("No active scene.");

					return;
				}

				const auto camera = activeScene->activeCamera();

				if ( camera == nullptr )
				{
					ImGui::TextUnformatted("No active camera.");

					return;
				}

				if ( ImGui::CollapsingHeader("Optics", ImGuiTreeNodeFlags_DefaultOpen) )
				{
					auto focalLength = camera->focalLength();

					if ( ImGui::SliderFloat("Focal length (mm)", &focalLength, 8.0F, 300.0F, "%.0f") )
					{
						camera->setFocalLength(focalLength);
					}

					auto aperture = camera->aperture();

					if ( ImGui::SliderFloat("Aperture (f/N)", &aperture, 1.0F, 32.0F, "f/%.1f") )
					{
						camera->setAperture(aperture);
					}

					auto sensorWidth = camera->sensorWidth();

					if ( ImGui::SliderFloat("Sensor width (mm)", &sensorWidth, 5.5F, 70.0F, "%.1f") )
					{
						camera->setSensorWidth(sensorWidth);
					}

					auto autoFocus = camera->isAutoFocusEnabled();

					if ( ImGui::Checkbox("Auto-focus", &autoFocus) )
					{
						camera->setAutoFocus(autoFocus);
					}

					if ( autoFocus )
					{
						/* The rack-focus position lives on the GPU (1x1 focus history); the
						 * camera-materialized DoF reads it back with framesInFlight frames of
						 * latency. Same frame-scope contract as the metered ISO below. */
						const auto depthOfField = activeScene->postProcessStack() != nullptr ? activeScene->postProcessStack()->cameraDepthOfField() : nullptr;

						if ( depthOfField != nullptr && depthOfField->meteredFocusDistance() > 0.0F )
						{
							ImGui::Text("measured: focus at %.2f m", depthOfField->meteredFocusDistance());
						}
						else
						{
							ImGui::TextDisabled("measuring...");
						}
					}
					else
					{
						auto focusDistance = camera->focusDistance();

						if ( ImGui::SliderFloat("Focus distance (m)", &focusDistance, 0.1F, 200.0F, "%.2f", ImGuiSliderFlags_Logarithmic) )
						{
							camera->setFocusDistance(focusDistance);
						}
					}
				}

				if ( ImGui::CollapsingHeader("Exposure", ImGuiTreeNodeFlags_DefaultOpen) )
				{
					/* Shown as the denominator, the way a shutter speed is actually read. */
					auto shutterDenominator = 1.0F / std::max(camera->shutterSpeed(), 1.0e-6F);

					if ( ImGui::SliderFloat("Shutter (1/x s)", &shutterDenominator, 4.0F, 4000.0F, "1/%.0f", ImGuiSliderFlags_Logarithmic) )
					{
						camera->setShutterSpeed(1.0F / std::max(shutterDenominator, 1.0F));
					}

					auto autoExposure = camera->isAutoExposureEnabled();

					if ( ImGui::Checkbox("Auto-ISO", &autoExposure) )
					{
						camera->setAutoExposure(autoExposure);
					}

					auto sensitivity = camera->sensitivity();

					/* ⚠️ With auto-ISO on, the metering picks the sensitivity on the GPU, so this
					 * slider does NOT show what is in effect — it shows the manual value the panel
					 * would fall back to. The METERED value is displayed below instead, read back
					 * from the adaptation history with framesInFlight frames of latency. */
					ImGui::BeginDisabled(autoExposure);

					if ( ImGui::SliderFloat("ISO", &sensitivity, camera->minSensitivity(), camera->maxSensitivity(), "%.0f", ImGuiSliderFlags_Logarithmic) && !autoExposure )
					{
						camera->setSensitivity(sensitivity);
					}

					ImGui::EndDisabled();

					if ( autoExposure )
					{
						/* The camera-materialized tone mapper owns the metering; the panel draws
						 * on the render thread inside the frame scope, so reading it here is the
						 * documented contract (PostProcessStack::cameraToneMapping()). */
						const auto toneMapping = activeScene->postProcessStack() != nullptr ? activeScene->postProcessStack()->cameraToneMapping() : nullptr;

						if ( toneMapping != nullptr && toneMapping->meteredSensitivity() > 0.0F )
						{
							ImGui::Text("metered: ISO %.0f | scene avg %.1f nits", toneMapping->meteredSensitivity(), toneMapping->meteredLuminance());

							if ( toneMapping->meteredSensitivity() <= camera->minSensitivity() || toneMapping->meteredSensitivity() >= camera->maxSensitivity() )
							{
								ImGui::TextDisabled("(saturated at the sensor bound — range %.0f-%.0f ISO)", camera->minSensitivity(), camera->maxSensitivity());
							}

							/* A GROWING count means the luminance chain is sampling implausible
							 * data every frame — the metering is being held, not measured. This is
							 * a corruption indicator, which is why it is surfaced and not hidden. */
							if ( const auto rejected = toneMapping->meteredRejectedCount(); rejected > 0 )
							{
								ImGui::TextDisabled("(%u metered frame(s) rejected as implausible - held)", rejected);
							}
						}
						else
						{
							ImGui::TextDisabled("metering... — range %.0f-%.0f ISO", camera->minSensitivity(), camera->maxSensitivity());
						}
					}

					auto exposureCompensation = camera->exposureCompensation();

					if ( ImGui::SliderFloat("Compensation (EV)", &exposureCompensation, -5.0F, 5.0F, "%+.2f") )
					{
						camera->setExposureCompensation(exposureCompensation);
					}
				}

				if ( ImGui::CollapsingHeader("Photographic effects", ImGuiTreeNodeFlags_DefaultOpen) )
				{
					auto depthOfField = camera->isDepthOfFieldEnabled();

					if ( ImGui::Checkbox("Depth of field", &depthOfField) )
					{
						camera->enableDepthOfField(depthOfField);
					}

					auto motionBlur = camera->isMotionBlurEnabled();

					if ( ImGui::Checkbox("Motion blur", &motionBlur) )
					{
						camera->enableMotionBlur(motionBlur);
					}

					if ( motionBlur )
					{
						/* There is no strength slider by design: the shutter speed above IS the
						 * control, since the smear length is the shutter angle. */
						ImGui::SameLine();
						ImGui::TextDisabled("(length = the shutter speed above)");
					}

					auto HDR = camera->isHDREnabled();

					if ( ImGui::Checkbox("HDR (tone mapping)", &HDR) )
					{
						camera->enableHDR(HDR);
					}

					auto bloom = camera->isBloomEnabled();

					if ( ImGui::Checkbox("Lens glare (bloom)", &bloom) )
					{
						camera->enableBloom(bloom);
					}

					if ( bloom )
					{
						auto threshold = camera->bloomThreshold();

						if ( ImGui::SliderFloat("Glare threshold (nits)", &threshold, 1.0F, 20000.0F, "%.0f", ImGuiSliderFlags_Logarithmic) )
						{
							camera->setBloomThreshold(threshold);
						}

						auto intensity = camera->bloomIntensity();

						/* Physical scattered FRACTION: a clean lens sits at 2-5%, a hazy vintage
						 * one above 10% — 25% is already a heavily veiling glass. */
						if ( ImGui::SliderFloat("Glare intensity", &intensity, 0.0F, 0.25F, "%.3f") )
						{
							camera->setBloomIntensity(intensity);
						}
					}
				}

				/* The readout that makes the model legible: the same APEX equation the tone mapper
				 * uses, so a setting that reads wrong here IS wrong on screen. */
				ImGui::Separator();

				const auto exposureValue = Graphics::Photometry::exposureValue100(camera->aperture(), camera->shutterSpeed(), camera->sensitivity());

				ImGui::Text("EV100 %.2f   exposure %.3e", exposureValue, Graphics::Photometry::exposureFromValue100(exposureValue));

				if ( camera->isAutoExposureEnabled() )
				{
					ImGui::TextDisabled("(computed at the manual ISO above — the metered one differs)");
				}
				ImGui::TextDisabled("sunlit 100000 lx ~ EV15 | overcast ~ EV12 | lit interior ~ EV7");
			}(m_frameScene);

			ImGui::End();
		});

		if ( m_cameraScreen != nullptr )
		{
			m_cameraScreen->setVisibility(false);
		}
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

		this->onCorePaused();

		/* Dispatch the pause event,
		 * first by sending the event,
		 * then directly to the sub application. */
		this->notify(ExecutionPaused);

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

		this->onCoreResumed();

		/* Dispatch the resume event,
		 * first by sending the event,
		 * then directly to the sub application. */
		this->notify(ExecutionResumed);
	}

	void
	Core::scheduleMainLoopCycle (int64_t delayMS) noexcept
	{
		const auto dueTimeMS = Base::Time::elapsedMilliseconds() + std::max< int64_t >(delayMS, 0);

		/* NOTE: Contract — a new request replaces any pending one (last call wins). */
		m_scheduledCycleDueTimeMS.store(dueTimeMS, std::memory_order_release);

		if ( delayMS <= 0 )
		{
			/* NOTE: Immediate request — wake the main loop, which may be blocked in
			 * waitSystemEvents() for up to 1/mainLoopFrequencyHz() seconds, or
			 * indefinitely while paused. */
			Input::Manager::wakeUpEventsLoop();
		}
	}

	void
	Core::executeMainLoopCycle () noexcept
	{
		/* Let the console execute pending remote commands. */
		m_consoleController.poll();

		/* NOTE: External cycle-scheduling contract (scheduleMainLoopCycle()): the cycle
		 * about to run satisfies a due request. CAS — if a fresher request landed
		 * concurrently, keep its deadline. */
		auto dueTimeMS = m_scheduledCycleDueTimeMS.load(std::memory_order_acquire);

		if ( dueTimeMS != MainLoopCycleNotScheduled && Base::Time::elapsedMilliseconds() >= dueTimeMS )
		{
			m_scheduledCycleDueTimeMS.compare_exchange_strong(dueTimeMS, MainLoopCycleNotScheduled, std::memory_order_acq_rel);
		}

		/* Let the child class get the call event from the main loop. */
		this->onCoreMainLoopCycle();
	}

	double
	Core::mainLoopWaitTimeout (double fallbackSeconds) const noexcept
	{
		const auto dueTimeMS = m_scheduledCycleDueTimeMS.load(std::memory_order_acquire);

		if ( dueTimeMS == MainLoopCycleNotScheduled )
		{
			return fallbackSeconds;
		}

		/* NOTE: glfwWaitEventsTimeout() requires a strictly positive timeout; an already
		 * due deadline degenerates to a minimal wait so the loop keeps draining OS events. */
		constexpr auto MinimalWaitSeconds{0.001};

		const auto remainingSeconds = std::max(static_cast< double >(dueTimeMS - Base::Time::elapsedMilliseconds()) * 0.001, MinimalWaitSeconds);

		/* NOTE: A fallback of 0.0 means "wait indefinitely" (pause loop): the pending
		 * deadline becomes the timeout, otherwise the tightest of the two wins. */
		if ( fallbackSeconds <= 0.0 )
		{
			return remainingSeconds;
		}

		return std::min(fallbackSeconds, remainingSeconds);
	}

	void
	Core::stop (int userExitCode) noexcept
	{
		using namespace PlatformSpecific::Desktop::Dialog;

		/* Save the user exit code. */
		m_userExitCode = userExitCode;

		/* Ask the user-application if it agrees to stop. */
		if ( !this->onBeforeCoreStop() )
		{
			m_stopVetoCount++;

			/* Below the threshold: just defer. */
			if ( m_stopVetoCount < MaxStopVetoCount )
			{
				return;
			}

			/* Threshold reached: ask the user whether to force-quit. */
			CustomMessage dialog{
				"Application not responding",
				"The application is not responding to the close request." "\n\n"
				"Do you want to force quit, or keep waiting?",
				ButtonLabels{"Force Quit", "Wait More"},
				MessageType::Question
			};

			dialog.execute(m_window, false);

			/* Index 0 = "Force Quit". Anything else (1 = "Wait More", -1 = dismissed) keeps waiting. */
			if ( dialog.getClickedButtonIndex() != 0 )
			{
				return;
			}

			/* NOTE: Go for a force quit! */
			this->setAppReadyToQuit(ForceQuitExitCode);

			/* NOTE: Don't care of the return here, we proceed to full termination. */
			(void) this->onBeforeCoreStop();
		}

		std::cout << "\n"
			"**************************************************************" "\n"
			"   Engine is about to stop (User exit code: " << userExitCode << ") !   " "\n"
			"**************************************************************" "\n\n";

		/* Dispatch the stop event,
		 * first by sending the event,
		 * then directly to the sub application. */
		this->notify(ExecutionStopping);

		if ( m_graphicsRenderer.recorder().isRecording() )
		{
			this->stopAudioVideoRecording();
		}

		/* Stopping the logics and rendering threads. */
		m_isRenderingLoopRunning = false;
		m_isLogicsLoopRunning = false;

		/* NOTE: Wake the rendering thread in case it is idling on the on-demand condition,
		 * so it observes the stopped flag and exits promptly instead of waiting the timeout. */
		m_redrawCondition.notify_all();

		/* Stopping the main loop. */
		m_isMainLoopRunning = false;

		/* Dispatch the stopped event,
		 * first by sending the event,
		 * then directly to the sub application. */
		this->notify(ExecutionStopped);
	}

	int
	Core::terminate () noexcept
	{
		uint32_t errors = 0;

		/* Terminate user services. */
		for ( auto * userService : std::ranges::reverse_view(m_userServiceEnabled) )
		{
			if ( userService->terminate() )
			{
				TraceSuccess{ClassId} << userService->name() << " user service terminated gracefully!";
			}
			else
			{
				errors++;

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
				errors++;

				TraceError{ClassId} << service->name() << " secondary service failed to terminate properly!";
			}

			/* NOTE: Release resources after each service terminates.
			 * This is critical to ensure Vulkan resources (with VMA allocations)
			 * are freed before the Device is destroyed by the Instance service. */
			m_resourceManager.unloadUnusedResources();
		}

		m_secondaryServicesEnabled.clear();

		if constexpr ( VulkanTrackingDebugEnabled )
		{
			if ( const auto vkAliveObjectCount = AbstractObject::s_tracking.size(); vkAliveObjectCount > 0 )
			{
				std::cerr << "[DEBUG:VK_TRACKING] There is " << vkAliveObjectCount << " Vulkan objects not properly destructed !" "\n";

				for ( auto & [address, identifier] : AbstractObject::s_tracking )
				{
					std::cerr << "[DEBUG:VK_TRACKING] The Vulkan object @" << address << " '" << identifier << "' still alive !" << "\n";

					errors++;
				}
			}
			else
			{
				std::cout << "[DEBUG:VK_TRACKING] All Vulkan objects has been released !" << "\n";
			}
		}

		return errors > 0 ? EXIT_FAILURE : m_userExitCode;
	}

	void
	Core::openFiles (const std::vector< std::filesystem::path > & filepaths) noexcept
	{
		std::vector< std::filesystem::path > remainingFilepaths;
		remainingFilepaths.reserve(filepaths.size());

		/* NOTE: Here Core always has the first view on files to consume engine-level content. */
		for ( const auto & filepath : filepaths )
		{
			if ( this->openResourceIndex(filepath) )
			{
				continue;
			}

			remainingFilepaths.emplace_back(filepath);
		}

		/* NOTE: If all files are used by Core, we stop the operation. */
		if ( remainingFilepaths.empty() )
		{
			return;
		}

		/* NOTE: The application overrides the Core default behaviors by
		 * removing every file it consumes from the list. */
		this->onCoreOpenFiles(remainingFilepaths);

		/* NOTE: Core default behaviors on the files left over by the application. */
		for ( const auto & filepath : remainingFilepaths )
		{
			static constexpr std::array< std::string_view, 7 > imageFileExtensions{
				"jpg", "jpeg", "png", "tga", "tif", "tiff", "hdr"
			};

			static constexpr std::array< std::string_view, 12 > audioFileExtensions{
				"wav", "flac", "ogg", "oga", "opus", "mp3", "aiff", "aif", "au", "caf", "mid", "midi"
			};

			const auto extension = IO::getFileExtension(filepath, true);

			if ( extension == "json" && this->openSceneDefinition(filepath) )
			{
				continue;
			}

			if ( std::ranges::find(imageFileExtensions, extension) != imageFileExtensions.cend() && this->openImageViewer(filepath) )
			{
				continue;
			}

			if ( std::ranges::find(audioFileExtensions, extension) != audioFileExtensions.cend() && this->openAudioTrack(filepath) )
			{
				continue;
			}

			/* NOTE: The scene loaders decide the composite asset formats themselves. */
			if ( this->openModelViewer(filepath) )
			{
				continue;
			}

			TraceWarning{ClassId} << "No default behavior to open the file '" << IO::toU8String(filepath) << "' !";

			this->notifyUser(BlobTrait{} << "Unable to open the file '" << IO::toU8String(filepath.filename()) << "'.");
		}
	}

	bool
	Core::openResourceIndex (const std::filesystem::path & filepath) noexcept
	{
		if ( IO::getFileExtension(filepath, true) != "json" )
		{
			return false;
		}

		const auto rootCheck = FastJSON::getRootFromFile(filepath, 16, true);

		if ( !rootCheck || !rootCheck->isMember(Resources::Manager::StoresKey) )
		{
			return false;
		}

		if ( !m_resourceManager.update(rootCheck.value()) )
		{
			TraceError{ClassId} << "Unable to update the resource stores from the index '" << IO::toU8String(filepath) << "' !";

			this->notifyUser(BlobTrait{} << "Unable to read the resource index '" << IO::toU8String(filepath.filename()) << "'.");

			return true;
		}

		this->notifyUser(BlobTrait{} << "Resource stores updated from '" << IO::toU8String(filepath.filename()) << "'.");

		return true;
	}

	bool
	Core::openSceneDefinition (const std::filesystem::path & filepath) noexcept
	{
		const auto rootCheck = FastJSON::getRootFromFile(filepath, 16, true);

		if ( !rootCheck )
		{
			return false;
		}

		/* NOTE: Identify a scene definition by its structural top-level keys. */
		{
			const auto & root = rootCheck.value();

			if ( !root.isMember(DefinitionResource::NodesKey) && !root.isMember(DefinitionResource::StaticEntitiesKey) && !root.isMember(DefinitionResource::BoundaryKey) )
			{
				return false;
			}
		}

		const auto [scene, definition] = m_sceneManager.loadScene(filepath);

		if ( scene == nullptr )
		{
			TraceError{ClassId} << "Unable to load the scene definition '" << IO::toU8String(filepath) << "' !";

			this->notifyUser(BlobTrait{} << "Unable to load the scene definition '" << IO::toU8String(filepath.filename()) << "'.");

			return true;
		}

		/* NOTE: Never disturb an active scene, the loaded scene stays available. */
		if ( m_sceneManager.hasActiveScene() )
		{
			this->notifyUser(BlobTrait{} << "Scene '" << scene->name() << "' loaded. Disable the active scene to enable it.");

			return true;
		}

		if ( !m_sceneManager.enableScene(scene) )
		{
			TraceError{ClassId} << "Unable to enable the scene '" << scene->name() << "' !";

			this->notifyUser(BlobTrait{} << "Unable to enable the scene '" << scene->name() << "'.");

			return true;
		}

		this->notifyUser(BlobTrait{} << "Scene '" << scene->name() << "' enabled.");

		return true;
	}

	bool
	Core::openAudioTrack (const std::filesystem::path & filepath) noexcept
	{
		auto & trackMixer = m_audioManager.trackMixer();

		if ( !trackMixer.usable() )
		{
			TraceWarning{ClassId} << "The track mixer is unavailable to play the file '" << IO::toU8String(filepath) << "' !";

			this->notifyUser("The audio system is unavailable to play the file.");

			return true;
		}

		const auto track = std::make_shared< Audio::MusicResource >(m_resourceManager, "+" + IO::toU8String(filepath.stem()));

		if ( !track->load(filepath) )
		{
			TraceError{ClassId} << "Unable to read the audio file '" << IO::toU8String(filepath) << "' !";

			this->notifyUser(BlobTrait{} << "Unable to read the audio file '" << IO::toU8String(filepath.filename()) << "'.");

			return true;
		}

		/* NOTE: The playback starts immediately. A track still loading
		 * (e.g. MIDI rendering) starts by itself when the loading finishes. */
		trackMixer.addToPlaylist(track);

		if ( !trackMixer.play(track) )
		{
			TraceError{ClassId} << "Unable to play the audio file '" << IO::toU8String(filepath) << "' !";

			this->notifyUser(BlobTrait{} << "Unable to play the audio file '" << IO::toU8String(filepath.filename()) << "'.");

			return true;
		}

		this->notifyUser(BlobTrait{} << "Playing '" << track->title() << "' ...");

		return true;
	}

	bool
	Core::clearStageForViewerScene () noexcept
	{
		if ( !m_sceneManager.hasActiveScene() )
		{
			return true;
		}

		std::string activeSceneName;

		m_sceneManager.withSharedActiveScene([&activeSceneName] (const std::shared_ptr< Scenes::Scene > & scene) {
			activeSceneName = scene->name();
		}, true);

		/* NOTE: A regular active scene is never disturbed by a viewer. */
		if ( activeSceneName != Viewers::ImageViewer::SceneName && activeSceneName != Viewers::ModelViewer::SceneName )
		{
			this->notifyUser("A scene is running, the file was ignored.");

			return false;
		}

		/* NOTE: An active viewer scene is replaced by the incoming one. */
		return m_sceneManager.deleteScene(activeSceneName);
	}

	bool
	Core::openImageViewer (const std::filesystem::path & filepath) noexcept
	{
		if ( !this->clearStageForViewerScene() )
		{
			return true;
		}

		Viewers::ImageViewer viewer{m_resourceManager, m_sceneManager};

		const auto scene = viewer.createScene(filepath);

		if ( scene == nullptr )
		{
			this->notifyUser(BlobTrait{} << "Unable to display the image '" << IO::toU8String(filepath.filename()) << "'.");

			return true;
		}

		if ( !m_sceneManager.enableScene(scene) )
		{
			TraceError{ClassId} << "Unable to enable the image viewer scene for '" << IO::toU8String(filepath) << "' !";

			this->notifyUser(BlobTrait{} << "Unable to display the image '" << IO::toU8String(filepath.filename()) << "'.");

			m_sceneManager.deleteScene(Viewers::ImageViewer::SceneName);

			return true;
		}

		this->notifyUser(BlobTrait{} << "Viewing '" << IO::toU8String(filepath.filename()) << "'.");

		return true;
	}

	bool
	Core::openModelViewer (const std::filesystem::path & filepath) noexcept
	{
		/* NOTE: Neither a composite asset nor a raw geometry file, let the dispatch continue. */
		if ( !Viewers::ModelViewer::handlesFile(m_sceneManager, filepath) )
		{
			return false;
		}

		if ( !this->clearStageForViewerScene() )
		{
			return true;
		}

		Viewers::ModelViewer viewer{m_resourceManager, m_sceneManager, m_primaryServices.settings()};

		const auto scene = viewer.createScene(filepath);

		if ( scene == nullptr )
		{
			this->notifyUser(BlobTrait{} << "Unable to display the model '" << IO::toU8String(filepath.filename()) << "'.");

			return true;
		}

		if ( !m_sceneManager.enableScene(scene) )
		{
			TraceError{ClassId} << "Unable to enable the model viewer scene for '" << IO::toU8String(filepath) << "' !";

			this->notifyUser(BlobTrait{} << "Unable to display the model '" << IO::toU8String(filepath.filename()) << "'.");

			m_sceneManager.deleteScene(Viewers::ModelViewer::SceneName);

			return true;
		}

		this->notifyUser(BlobTrait{} << "Viewing '" << IO::toU8String(filepath.filename()) << "'.");

		return true;
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
			const auto subCode = system(command.c_str());

			if ( subCode > 0 )
			{
				TraceWarning{ClassId} << "Command " << command << " exitd with code: " << subCode;
			}
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
		return this->onCoreKeyPress(key, scancode, modifiers, repeat);
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
					this->stop();
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
					return true;

				case KeyF2 :
#ifdef IMGUI_ENABLED
					if ( m_cameraScreen != nullptr )
					{
						m_cameraScreen->setVisibility(!m_cameraScreen->isVisible());
					}
#else
					Tracer::info(ClassId, "The physical camera panel requires EMERAUDE_ENABLE_IMGUI.");
#endif
					return true;

				case KeyF3 :
					m_sceneManager.toggleEditorMode();

					return true;

				case KeyF4 :
					if ( !m_window.isFullscreenMode() )
					{
						TraceInfo{ClassId} << "Reset window size to default " << DefaultWindowWidth << "X" << DefaultWindowHeight;

						if ( m_window.resize(DefaultWindowWidth, DefaultWindowHeight) )
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
					if ( isKeyboardModifierPressed(ModKeyControl, modifiers) )
					{
						if ( m_graphicsRenderer.recorder().isRecording() )
						{
							this->stopAudioVideoRecording();
						}
						else
						{
							static_cast< void >(this->startAudioVideoRecording());
						}
					}
					else
					{
						this->screenshot();
					}
					return true;

				case KeyC :
					if ( auto & renderDoc = m_vulkanInstance.renderDocCapture(); renderDoc.isAvailable() )
					{
						renderDoc.triggerCapture();

						this->notifyUser("RenderDoc: triggered frame capture.");
					}
					else
					{
						this->notifyUser("RenderDoc not available (not injected).");
					}
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
		if ( this->onCoreKeyRelease(key, scancode, modifiers) )
		{
			return true;
		}

		/* NOTE: If the application does not catch any key, we let the core having a default behavior. */
		if ( !m_preventDefaultKeyBehaviors )
		{
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

		return false;
	}

	bool
	Core::onCharacterType (uint32_t unicode) noexcept
	{
		/* NOTE: Let the user application consume the event. */
		return this->onCoreCharacterType(unicode);
	}

	bool
	Core::screenshot () noexcept
	{
		/* Gets the capture directory. */
		auto captureDirectory = m_primaryServices.fileSystem().userDataDirectory("captures");

		if ( !IO::writable(captureDirectory) )
		{
			TraceError{ClassId} << "Unable to write in captures directory " << captureDirectory << " !";

			return false;
		}

		if ( !m_graphicsRenderer.captureFramebuffer(m_screenshotImages, false, false) || !m_screenshotImages[0].isValid() )
		{
			Tracer::error(ClassId, "Unable to capture the framebuffer !");

			return false;
		}

		std::stringstream filename;
		filename << std::chrono::duration_cast< std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count() << ".png";

		const auto filepath = captureDirectory.append(filename.str());

		if ( !PixelFactory::FileIO::write(m_screenshotImages[0], filepath) )
		{
			TraceError{ClassId} << "Unable to write the screenshot " << filepath << " !";

			return false;
		}

		TraceSuccess{ClassId} << "The screenshot is saved to " << filepath;

		return true;
	}

	bool
	Core::startAudioVideoRecording () noexcept
	{
		if ( !m_graphicsRenderer.recorder().usable() && !m_audioManager.recorder().usable() )
		{
			Tracer::warning(ClassId, "No recorder enabled!");

			return false;
		}

		/* Guard: refuse a new rush if the audio recorder is still active from a previous one. */
		if ( m_audioManager.recorder().isRecording() )
		{
			TraceWarning{ClassId} << "Cannot start new recording: audio recorder is still active from previous rush.";

			return false;
		}

		/* Gets the capture directory. */
		const auto captureDirectory = m_primaryServices.fileSystem().userDataDirectory("captures");

		if ( !IO::writable(captureDirectory) )
		{
			TraceError{ClassId} << "Unable to write in captures directory " << captureDirectory << " !";

			return false;
		}

		/* Generate a timestamp-based base name. */
		std::stringstream baseName;
		baseName << std::chrono::duration_cast< std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();

		const auto filename = baseName.str();

		if ( m_graphicsRenderer.recorder().usable() )
		{
			if ( m_graphicsRenderer.recorder().startRecording(captureDirectory / (filename + "." + m_graphicsRenderer.recorder().videoFileExtension())) )
			{
				this->notifyUser("Video recording started...");
			}
		}

		if ( m_audioManager.recorder().usable() )
		{
			if ( m_audioManager.recorder().startRecording(captureDirectory / (filename + ".wav")) )
			{
				this->notifyUser("Audio recording started...");
			}
		}

		/* Voice-over: start microphone capture if enabled. */
		auto & settings = m_primaryServices.settings();

		if ( settings.getOrSetDefault< bool >(RushMakerEnableVoiceOverKey, DefaultRushMakerEnableVoiceOver) )
		{
			if ( m_audioManager.externalInput().usable() )
			{
				m_rushVoiceOverPath = captureDirectory / (filename + "-voice.wav");

				if ( m_audioManager.externalInput().start(m_rushVoiceOverPath) )
				{
					TraceSuccess{ClassId} << "Voice-over recording started -> " << m_rushVoiceOverPath;
				}
				else
				{
					TraceError{ClassId} << "Failed to start voice-over recording to " << m_rushVoiceOverPath;

					m_rushVoiceOverPath.clear();
				}
			}
			else
			{
				TraceWarning{ClassId} <<
					"Voice-over is enabled but the audio capture service is not available. "
					"Ensure '" << AudioCaptureEnableKey << "' is set to true in settings to allow microphone access.";
			}
		}

		/* RushMaker: generate the assembly script when video is active. */
		if ( m_graphicsRenderer.recorder().isRecording() )
		{
			const auto hasGameAudio = m_audioManager.recorder().isRecording();
			const auto hasVoiceOver = !m_rushVoiceOverPath.empty();
			const auto audioBitrate = hasGameAudio
				? m_graphicsRenderer.recorder().recommendedAudioBitrate() * m_audioManager.recorder().channelCount() / 2
				: 128U;

			/* Hardware H.265 -> MP4/AAC (the raw elementary stream needs the input
			 * framerate); software VP9/IVF -> WebM/Opus. */
			const std::string videoExtension{m_graphicsRenderer.recorder().videoFileExtension()};
			const auto hardware = videoExtension == "h265";
			const std::string videoInputOptions = hardware ? "-framerate " + std::to_string(m_graphicsRenderer.recorder().targetFramerate()) + " " : "";
			const std::string audioCodec = hardware ? "aac" : "libopus";
			const std::string containerExtension = hardware ? ".mp4" : ".webm";

#if IS_WINDOWS
			const auto scriptPath = captureDirectory / (filename + "_assemble.ps1");
			std::ofstream script{scriptPath};

			if ( script.is_open() )
			{
				script << "# RushMaker - Auto-generated script to assemble the recording.\n";
				script << "# Run this script after stopping the recording.\n";
				script << "Set-Location \"" << captureDirectory.string() << "\"\n";
				script << "ffmpeg.exe " << videoInputOptions << "-i \"" << filename << "." << videoExtension << "\"";

				if ( hasGameAudio && hasVoiceOver )
				{
					script << " -i \"" << filename << ".wav\" -i \"" << filename << "-voice.wav\"";
					script << " -filter_complex \"[1:a][2:a]amix=inputs=2:duration=longest:normalize=0[aout]\"";
					script << " -map 0:v -map \"[aout]\" -c:v copy -c:a " << audioCodec << " -b:a " << audioBitrate << "k";
				}
				else if ( hasGameAudio )
				{
					script << " -i \"" << filename << ".wav\" -c:v copy -c:a " << audioCodec << " -b:a " << audioBitrate << "k";
				}
				else if ( hasVoiceOver )
				{
					script << " -i \"" << filename << "-voice.wav\" -c:v copy -c:a " << audioCodec << " -b:a " << audioBitrate << "k";
				}
				else
				{
					script << " -c:v copy";
				}

				/* Container-level colour tags (the VP9 bitstream itself already carries BT.709). */
			script << " -colorspace bt709 -color_primaries bt709 -color_trc bt709";

			script << " \"" << filename << containerExtension << "\"\n";

				TraceSuccess{ClassId} << "RushMaker assemble script written to " << scriptPath;
			}
#else
			const auto scriptPath = captureDirectory / (filename + "_assemble.sh");
			std::ofstream script{scriptPath};

			if ( script.is_open() )
			{
				script << "#!/bin/bash\n";
				script << "# RushMaker - Auto-generated script to assemble the recording.\n";
				script << "# Run this script after stopping the recording.\n";
				script << "cd \"" << captureDirectory.string() << "\"\n";
				script << "ffmpeg " << videoInputOptions << "-i \"" << filename << "." << videoExtension << "\"";

				if ( hasGameAudio && hasVoiceOver )
				{
					script << " -i \"" << filename << ".wav\" -i \"" << filename << "-voice.wav\" \\\n";
					script << "  -filter_complex \"[1:a][2:a]amix=inputs=2:duration=longest:normalize=0[aout]\" \\\n";
					script << "  -map 0:v -map \"[aout]\" -c:v copy -c:a " << audioCodec << " -b:a " << audioBitrate << "k";
				}
				else if ( hasGameAudio )
				{
					script << " -i \"" << filename << ".wav\" -c:v copy -c:a " << audioCodec << " -b:a " << audioBitrate << "k";
				}
				else if ( hasVoiceOver )
				{
					script << " -i \"" << filename << "-voice.wav\" -c:v copy -c:a " << audioCodec << " -b:a " << audioBitrate << "k";
				}
				else
				{
					script << " -c:v copy";
				}

				/* Container-level colour tags (the VP9 bitstream itself already carries BT.709). */
			script << " -colorspace bt709 -color_primaries bt709 -color_trc bt709";

			script << " \"" << filename << containerExtension << "\"\n";

				script.close();

				std::filesystem::permissions(scriptPath,
					std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec,
					std::filesystem::perm_options::add);

				TraceSuccess{ClassId} << "RushMaker assemble script written to " << scriptPath;
			}
#endif
		}

		return true;
	}

	void
	Core::stopAudioVideoRecording () noexcept
	{
		/* Voice-over: stop streaming capture (finalizes WAV header and closes file). */
		if ( m_audioManager.externalInput().isRecording() )
		{
			m_audioManager.externalInput().stop();

			m_rushVoiceOverPath.clear();
		}

		if ( m_audioManager.recorder().isRecording() )
		{
			m_audioManager.recorder().stopRecording();
		}

		if ( m_graphicsRenderer.recorder().isRecording() )
		{
			m_graphicsRenderer.recorder().stopRecording();
		}

		this->notifyUser("Recording stopped.");
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
			if constexpr ( ObserverDebugEnabled )
			{
				TraceInfo{ClassId} << "Receiving an event from '" << Console::Controller::ClassId << "' (code:" << notificationCode << ") ...";
			}

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
					TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << Console::Controller::ClassId << "' ignored or unknown.";
					break;
			}

			return true;
		}

		if ( observable == &m_primaryServices.netManager() )
		{
			if constexpr ( ObserverDebugEnabled )
			{
				TraceInfo{ClassId} << "Receiving an event from '" << Net::Manager::ClassId << "' (code:" << notificationCode << ") ...";
			}

			switch ( notificationCode )
			{
				case Net::Manager::Unknown :
				case Net::Manager::DownloadingStarted :
				case Net::Manager::FileDownloaded :
				case Net::Manager::DownloadingFinished :
				case Net::Manager::Progress :
					break;

				default:
					TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << Net::Manager::ClassId << "' ignored or unknown.";
					break;
			}

			return true;
		}

		if ( observable == &m_window )
		{
			if constexpr ( ObserverDebugEnabled )
			{
				TraceInfo{ClassId} << "Receiving an event from '" << Window::ClassId << "' (code:" << notificationCode << ") ...";
			}

			switch ( notificationCode )
			{
				case Window::OSNotifiesWindowGetFocus :
				case Window::OSNotifiesWindowVisible :
					this->resume();
					/* NOTE: The window became visible/focused again: repaint once so a freshly
					 * revealed surface is not left showing a stale image in on-demand mode. */
					this->requestRedraw();
					break;

				case Window::OSNotifiesWindowLostFocus :
				case Window::OSNotifiesWindowHidden :
					if ( m_pausable )
					{
						this->pause();
					}
					break;

				/* NOTE: Framebuffer size change — the swap-chain is marked degraded by Renderer::onNotification.
				 * In on-demand rendering the render thread is asleep, so it would never notice the degraded
				 * swap-chain nor recreate it (the WindowContentRefreshed -> onWindowChanged -> requestRedraw
				 * path only fires AFTER a recreation, which never happens). Wake it here so it processes the
				 * resize; onWindowChanged() then re-budgets a redraw at the new size. No-op in continuous mode. */
				case Window::OSNotifiesFramebufferResized :
					/* NOTE: Commented for excessive logs. */
					//Tracer::debug(ClassId, "The GLFW API detected a framebuffer content size change.");
					this->requestRedraw();
					break;

				/* NOTE: The surface content scale changed (window moved to a monitor with a different
				 * scale, or a fractional-scale change). Refresh the pointer scaling so cursor coordinates
				 * stay consistent with the framebuffer used by the overlay hit-testing. */
				case Window::OSRequestsToRescaleContentBy :
					this->updatePointerScaling();
					/* NOTE: A scale change also degrades the swap-chain (see Renderer::onNotification):
					 * wake the render thread for the same reason as OSNotifiesFramebufferResized above. */
					this->requestRedraw();
					break;

				case Window::OSRequestsToTerminate :
					this->stop();
					break;

				default:
					TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << Window::ClassId << "' ignored or unknown.";
					break;
			}

			return true;
		}

		if ( observable == &m_inputManager )
		{
			if constexpr ( ObserverDebugEnabled )
			{
				TraceInfo{ClassId} << "Receiving an event from '" << Input::Manager::ClassId << "' (code:" << notificationCode << ") ...";
			}

			switch ( notificationCode )
			{
				case Input::Manager::DroppedFiles :
				{
					const auto filepaths = std::any_cast< std::vector< std::filesystem::path > >(data);

					this->openFiles(filepaths);
				}
					break;

				default:
					TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << Input::Manager::ClassId << "' ignored or unknown.";
					break;
			}

			return true;
		}

		if ( observable == &m_graphicsRenderer )
		{
			if constexpr ( ObserverDebugEnabled )
			{
				TraceInfo{ClassId} << "Receiving an event from '" << Renderer::ClassId << "' (code:" << notificationCode << ") ...";
			}

			switch ( notificationCode )
			{
				case Renderer::WindowContentRefreshed :
					/* NOTE: If the swap-chain has been refreshed, we refresh
					 * the application according to the new framebuffer. */
					m_windowChanged = true;
					break;

				default:
					TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << Renderer::ClassId << "' ignored or unknown.";
					break;
			}

			return true;
		}

		if ( observable == &m_graphicsRenderer.shaderManager() )
		{
			if constexpr ( ObserverDebugEnabled )
			{
				TraceInfo{ClassId} << "Receiving an event from '" << Saphir::ShaderManager::ClassId << "' (code:" << notificationCode << ") ...";
			}

			switch ( notificationCode )
			{
				case Saphir::ShaderManager::ShaderCompilationSucceed :
					this->notifyUser(BlobTrait{} << "Shader '" << std::any_cast< std::string >(data) << "' compilation succeeded!");
					break;

				case Saphir::ShaderManager::ShaderCompilationFailed :
				{
					const auto [identifier, sourceCode] = std::any_cast< std::pair< std::string, std::string > >(data);

					this->onCoreShaderCompilationFailed(identifier, sourceCode);
				}
					break;

				default:
					TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << Saphir::ShaderManager::ClassId << "' ignored or unknown.";
					break;
			}

			return true;
		}

		if ( observable == &m_audioManager.trackMixer() )
		{
			if constexpr ( ObserverDebugEnabled )
			{
				TraceInfo{ClassId} << "Receiving an event from '" << Audio::TrackMixer::ClassId << "' (code:" << notificationCode << ") ...";
			}

			switch ( notificationCode )
			{
				case Audio::TrackMixer::MusicPlaying :
				case Audio::TrackMixer::MusicSwitching :
					this->notifyUser(std::any_cast< std::string >(data));
					break;

				case Audio::TrackMixer::MusicPaused :
					this->notifyUser("Music paused!");
					break;

				case Audio::TrackMixer::MusicResumed :
					this->notifyUser("Music resumed!");
					break;

				case Audio::TrackMixer::MusicStopped :
					this->notifyUser("Music stopped!");
					break;

				default:
					TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << Audio::TrackMixer::ClassId << "' ignored or unknown.";
					break;
			}

			/* NOTE: Also forward to derived class for additional handling (e.g., UI updates). */
			return this->onCoreNotification(observable, notificationCode, data);
		}

		if ( observable == &m_overlayManager )
		{
			if constexpr ( ObserverDebugEnabled )
			{
				TraceInfo{ClassId} << "Receiving an event from '" << Overlay::Manager::ClassId << "' (code:" << notificationCode << ") ...";
			}

			switch ( notificationCode )
			{
				case Overlay::Manager::RedrawRequested :
					/* NOTE: The overlay signalled a visual change (surface content/geometry/visibility/order,
					 * or a screen lifecycle/visibility change). Wake the on-demand rendering thread. No-op in
					 * continuous rendering. */
					this->requestRedraw();
					break;

				default:
					TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << Overlay::Manager::ClassId << "' ignored or unknown.";
					break;
			}

			return true;
		}

		if ( observable == &m_sceneManager )
		{
			if constexpr ( ObserverDebugEnabled )
			{
				TraceInfo{ClassId} << "Receiving an event from '" << Scenes::Manager::ClassId << "' (code:" << notificationCode << ") ...";
			}

			switch ( notificationCode )
			{
				case Scenes::Manager::SceneDestroyed :
					m_resourceManager.unloadUnusedResources();
					break;

				case Scenes::Manager::SceneEnabled :
				case Scenes::Manager::SceneDisabled :
					/* NOTE: On-demand rendering. Enabling a scene must wake the rendering thread so it
					 * starts drawing it without the safety-timeout latency; disabling one must trigger a
					 * final frame so the display falls back to the bare clear color instead of keeping the
					 * last rendered scene image. No-op in continuous mode. */
					this->requestRedraw();
					break;

				default:
					TraceDebug{ClassId} << "Event #" << notificationCode << " from '" << Scenes::Manager::ClassId << "' ignored or unknown.";
					break;
			}

			return true;
		}

		/* NOTE: If no event observable is identified by core, we pass it to the application level. */
		return this->onCoreNotification(observable, notificationCode, data);
	}

	int
	Core::executeToolsMode () noexcept
	{
		Tracer::info(ClassId, "Executing in tools mode ...");

		const auto tools = m_primaryServices.arguments().get(ToolsArg, ToolsLongArg).value();

		if ( tools == VulkanInformationToolName )
		{
			Tool::ShowVulkanInformation tool{m_primaryServices.arguments(), m_vulkanInstance};

			return tool.execute() ? EXIT_SUCCESS : EXIT_FAILURE;
		}

		if ( tools == PrintGeometryToolName )
		{
			Tool::GeometryDataPrinter tool{m_primaryServices.arguments()};

			return tool.execute() ? EXIT_SUCCESS : EXIT_FAILURE;
		}

		if ( tools == ConvertGeometryToolName )
		{
			TraceDebug{ClassId} << "FIXME: ...";

			return EXIT_SUCCESS;
		}

		TraceWarning{ClassId} << "Unrecognized tools '" << tools << "' !";

		return EXIT_FAILURE;
	}

	void
	Core::executeWipeLocalData (bool dryRun) noexcept
	{
		const auto & fileSystem = m_primaryServices.fileSystem();

		const auto & cacheDir = fileSystem.cacheDirectory();
		const auto & userDataDir = fileSystem.userDataDirectory();

		/* Format byte size to human-readable string. */
		const auto formatSize = [] (uintmax_t bytes) noexcept -> std::string {
			if ( bytes >= 1024ULL * 1024ULL )
			{
				return std::to_string(bytes / (1024ULL * 1024ULL)) + " MiB";
			}

			if ( bytes >= 1024ULL )
			{
				return std::to_string(bytes / 1024ULL) + " KiB";
			}

			return std::to_string(bytes) + " bytes";
		};

		/* Build the entire report in a single trace message. */
		TraceWarning trace{ClassId};

		trace <<
			"\n"
			"======================================================================" "\n"
			<< (dryRun ? "  LOCAL DATA WIPE - DRY RUN (nothing will be deleted)" : "  LOCAL DATA WIPE - CONFIRM (data will be permanently deleted)") <<
			"\n"
			"======================================================================" "\n";

		size_t totalFiles = 0;
		uintmax_t totalBytes = 0;

		/* Scan a directory: list content into trace and collect stats. */
		const auto scanDirectory = [&trace] (const std::filesystem::path & directory) noexcept -> std::pair< size_t, uintmax_t > {
			std::error_code errorCode;
			size_t fileCount = 0;
			uintmax_t totalSize = 0;

			for ( const auto & entry : std::filesystem::recursive_directory_iterator(directory, errorCode) )
			{
				if ( entry.is_regular_file(errorCode) )
				{
					const auto fileSize = entry.file_size(errorCode);

					trace << "	" << entry.path().string() << " (" << fileSize << " bytes)" "\n";

					fileCount++;
					totalSize += fileSize;
				}
				else if ( entry.is_directory(errorCode) )
				{
					trace << "	" << entry.path().string() << "/" "\n";
				}
			}

			return {fileCount, totalSize};
		};

		if ( IO::directoryExists(cacheDir) )
		{
			trace << "[WIPE TARGET] Cache: " << cacheDir.string() << "\n";

			const auto [fileCount, dirSize] = scanDirectory(cacheDir);

			trace << "  => " << fileCount << " files, " << formatSize(dirSize) << "\n";

			totalFiles += fileCount;
			totalBytes += dirSize;

			if ( !dryRun )
			{
				if ( IO::eraseDirectory(cacheDir, true) )
				{
					trace << "  * Cache directory wiped." "\n";
				}
				else
				{
					trace << "  ! Failed to wipe cache directory!" "\n";
				}
			}
		}

		if ( IO::directoryExists(userDataDir) )
		{
			trace << "[WIPE TARGET] User data: " << userDataDir.string() << "\n";

			const auto [fileCount, dirSize] = scanDirectory(userDataDir);

			trace << "  => " << fileCount << " files, " << formatSize(dirSize) << "\n";

			totalFiles += fileCount;
			totalBytes += dirSize;

			if ( !dryRun )
			{
				if ( IO::eraseDirectory(userDataDir, true) )
				{
					trace << "  * User data directory wiped." "\n";
				}
				else
				{
					trace << "  ! Failed to wipe user data directory!" "\n";
				}
			}
		}

		trace <<
			"----------------------------------------------------------------------" "\n"
			"[PRESERVED] Settings: " << fileSystem.configDirectory().string() <<
			"\n"
			"Total: " << totalFiles << " files, " << formatSize(totalBytes) <<
			"\n"
			"----------------------------------------------------------------------" "\n";

		if ( dryRun )
		{
			trace << "This was a dry run. Use --wipe-local-data-confirm to actually delete." "\n";
		}
		else
		{
			trace << "Local data wipe complete." "\n";
		}

		trace << "======================================================================";
	}

	void
	Core::executeResetSettings () noexcept
	{
		const auto & settingsPath = m_primaryServices.settings().filepath();

		if ( !std::filesystem::exists(settingsPath) )
		{
			TraceWarning trace{ClassId};
			trace <<
				"\n"
				"======================================================================" "\n"
				"  SETTINGS RESET" "\n"
				"======================================================================" "\n"
				"No settings file found. Nothing to reset." "\n"
				"----------------------------------------------------------------------" "\n"
				"Restart the application without --reset-settings." "\n"
				"======================================================================";

			return;
		}

		const auto timestamp = std::chrono::duration_cast< std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
		const auto backupPath = std::filesystem::path{settingsPath.string() + "." + std::to_string(timestamp) + "-bck"};

		std::error_code errorCode;
		std::filesystem::rename(settingsPath, backupPath, errorCode);

		if ( errorCode )
		{
			TraceError{ClassId} << "Failed to backup settings file: " << errorCode.message();

			return;
		}

		TraceWarning trace{ClassId};
		trace <<
			"\n"
			"======================================================================" "\n"
			"  SETTINGS RESET" "\n"
			"======================================================================" "\n"
			"Settings file backed up to:" "\n"
			"  " << backupPath.string() <<
			"\n"
			"----------------------------------------------------------------------" "\n"
			"Restart the application without --reset-settings to generate fresh settings." "\n"
			"======================================================================";
	}

	void
	Core::resetSettingsIfOutdated () noexcept
	{
		auto & settings = m_primaryServices.settings();
		const auto & settingsPath = settings.filepath();

		/* Fresh install (no file yet): nothing to reset. */
		if ( settingsPath.empty() || !std::filesystem::exists(settingsPath) )
		{
			return;
		}

		/* Reset when the file predates the current build — checked in order, any hit wins:
		 *  1. either version stamp is missing/unparsable (a file written before these keys existed);
		 *  2. the engine version increased (stored WrittenByEngineVersion < current engine version);
		 *  3. the application version increased (stored WrittenByApplicationVersion < Core's applicationVersion).
		 * An exact match, or a downgrade (stored newer than current), keeps the settings. */
		const auto storedEngineVersion = Version::FromString(settings.get< std::string >(Settings::EngineVersionKey, std::string{}));
		const auto storedApplicationVersion = Version::FromString(settings.get< std::string >(Settings::ApplicationVersionKey, std::string{}));

		std::string reason;

		if ( !storedEngineVersion || !storedApplicationVersion )
		{
			reason = "missing version stamp (file predates the version keys)";
		}
		else if ( storedEngineVersion.value() < Identification::EngineVersion )
		{
			reason = "engine version " + Base::to_string(storedEngineVersion.value()) + " older than " + Base::to_string(Identification::EngineVersion);
		}
		else if ( storedApplicationVersion.value() < m_identification.applicationVersion() )
		{
			reason = "application version " + Base::to_string(storedApplicationVersion.value()) + " older than " + Base::to_string(m_identification.applicationVersion());
		}

		/* Up to date (or a downgrade): keep the settings. */
		if ( reason.empty() )
		{
			return;
		}

		const auto timestamp = std::chrono::duration_cast< std::chrono::seconds >(std::chrono::system_clock::now().time_since_epoch()).count();
		const auto backupPath = std::filesystem::path{settingsPath.string() + "." + std::to_string(timestamp) + "-bck"};

		std::error_code errorCode;
		std::filesystem::rename(settingsPath, backupPath, errorCode);

		if ( errorCode )
		{
			/* Backup failed: leave the settings untouched — better an outdated file than clearing the
			 * store and overwriting the user's settings with empties at save-at-exit (data loss). */
			TraceError{ClassId} << "Settings version guard: failed to back up '" << settingsPath.string() << "': " << errorCode.message() << " - keeping the current settings.";

			return;
		}

		/* Clear the loaded store: the application keeps running on a clean base, every service
		 * re-generates its defaults lazily, and the fresh file is written at save-at-exit with the
		 * current version. */
		settings.clear();

		TraceWarning{ClassId} <<
			"\n"
			"======================================================================" "\n"
			"  SETTINGS RESET (version guard)" "\n"
			"======================================================================" "\n"
			"Reset reason: " << reason << "." "\n"
			"The settings file has been backed up to:" "\n"
			"  " << backupPath.string() << "\n"
			"The application starts on fresh settings." "\n"
			"======================================================================";
	}
}
