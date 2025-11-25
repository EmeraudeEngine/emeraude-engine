/*
 * src/Core.hpp
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

/**
 * @file Core.hpp
 * @brief Defines the Core class, the central orchestrator of Emeraude Engine.
 * @details The Core class is the heart of Emeraude Engine. It manages the complete
 * lifecycle of the engine, from initialization to termination, and coordinates all
 * engine subsystems including graphics, physics, audio, input, resources, and scenes.
 *
 * ## Architecture Overview
 *
 * The Core class follows a service-oriented architecture where each major subsystem
 * is represented as a service. Services are categorized into three levels:
 *
 * ### Primary Services (Always Available)
 * - **PrimaryServices**: System info, user info, arguments, tracer, filesystem, settings, network
 * - **Console::Controller**: Console command processing
 * - **Resources::Manager**: Resource loading and caching
 * - **User**: User preferences and settings
 *
 * ### Secondary Services (Graphics Context Required)
 * - **PlatformManager**: Platform-specific operations
 * - **Vulkan::Instance**: Vulkan API abstraction
 * - **Window**: Display window management
 * - **Input::Manager**: Keyboard, mouse, and gamepad input
 * - **Graphics::Renderer**: Vulkan-based rendering pipeline
 * - **Physics::Manager**: Physics simulation
 * - **Audio::Manager**: OpenAL-based 3D audio
 * - **Overlay::Manager**: ImGui-based UI system
 * - **Notifier**: On-screen notifications
 * - **Scenes::Manager**: Scene graph management
 *
 * ## Usage Pattern
 *
 * Applications extend the Core class and implement the pure virtual methods:
 * @code
 * class MyApplication : public EmEn::Core
 * {
 * public:
 *     MyApplication(int argc, char** argv)
 *         : Core(argc, argv, "MyApp", {1, 0, 0}, "MyOrg", "myorg.com")
 *     {}
 *
 * protected:
 *     // Required: Setup your scene here
 *     bool onCoreStarted() noexcept override { return true; }
 *
 *     // Required: Update game logic here (runs on logic thread)
 *     void onCoreProcessLogics(size_t cycle) noexcept override {}
 *
 *     // Optional overrides available:
 *     // - onBeforeCoreSecondaryServicesInitialization() : Pre-init checks
 *     // - onCorePaused() / onCoreResumed() : Pause handling
 *     // - onBeforeCoreStop() : Cleanup before shutdown
 *     // - onCoreKeyPress() / onCoreKeyRelease() : Keyboard input
 *     // - onCoreCharacterType() : Text input
 *     // - onCoreNotification() : Observer pattern events
 *     // - onCoreOpenFiles() : Drag & drop files
 *     // - onCoreSurfaceRefreshed() : Window resize handling
 * };
 *
 * int main(int argc, char** argv)
 * {
 *     MyApplication app(argc, argv);
 *     return app.run() ? EXIT_SUCCESS : EXIT_FAILURE;
 * }
 * @endcode
 *
 * ## Main Loop Architecture
 *
 * The engine uses a multi-threaded main loop:
 * - **Main Thread**: Window events, input processing, Vulkan presentation
 * - **Logic Thread**: Physics, scene updates, game logic (via onCoreProcessLogics)
 * - **Rendering Thread**: Frame preparation and GPU command submission
 *
 * ## Observer Pattern
 *
 * Core implements both ObserverTrait and ObservableTrait for event-driven communication:
 * - **As Observer**: Receives notifications from subsystems (window resize, etc.)
 * - **As Observable**: Broadcasts execution state changes to listeners
 *
 * @see PrimaryServices
 * @see Graphics::Renderer
 * @see Scenes::Manager
 * @see Resources::Manager
 * @version 0.8.35
 */

#pragma once

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <any>

/* Local inclusions for inheritances. */
#include "Input/KeyboardListenerInterface.hpp"
#include "Console/Controllable.hpp"
#include "Libs/ObserverTrait.hpp"
#include "Libs/ObservableTrait.hpp"

/* Local inclusions for usages. */
#include "Audio/Manager.hpp"
#include "Console/Controller.hpp"
#include "CursorAtlas.hpp"
#include "Graphics/Renderer.hpp"
#include "Help.hpp"
#include "Identification.hpp"
#include "Input/Manager.hpp"
#include "Notifier.hpp"
#include "Overlay/Manager.hpp"
#include "Physics/Manager.hpp"
#include "PlatformManager.hpp"
#include "PrimaryServices.hpp"
#include "Resources/Manager.hpp"
#include "Scenes/Manager.hpp"
#include "User.hpp"
#include "Vulkan/Instance.hpp"
#include "Window.hpp"

namespace EmEn
{
	/**
	 * @brief Core object of Emeraude-Engine. One of his main roles is to hold all services.
	 * @note [OBS][STATIC-OBSERVER][STATIC-OBSERVABLE]
	 * @extends EmEn::Input::KeyboardListenerInterface The core needs to get events from the keyboard for low-level interaction.
	 * @extends EmEn::Console::Controllable The core can be controlled by the console.
	 * @extends EmEn::Libs::ObserverTrait The core is an observer.
	 * @extends EmEn::Libs::ObservableTrait The core is observable.
	 */
	class Core : private Input::KeyboardListenerInterface, private Console::Controllable, public Libs::ObserverTrait, public Libs::ObservableTrait
	{
		public:

			/** @brief Class identifier for logging and debugging. */
			static constexpr auto ClassId{"Core"};

			/** @name Command-Line Argument Keys
			 * @brief Constants for parsing command-line arguments.
			 * @{
			 */
			static constexpr auto ToolsArg{"-t"};               ///< Short argument for tools mode.
			static constexpr auto ToolsLongArg{"--tools-mode"}; ///< Long argument for tools mode.
			/** @} */

			/** @name Available Tool Names
			 * @brief Names of built-in tools accessible via tools mode.
			 * @details Use with `-t <tool_name>` or `--tools-mode <tool_name>`.
			 * @{
			 */
			static constexpr auto VulkanInformationToolName{"vulkanInfo"};   ///< Displays Vulkan instance/device info.
			static constexpr auto PrintGeometryToolName{"printGeometry"};    ///< Prints geometry file contents.
			static constexpr auto ConvertGeometryToolName{"convertGeometry"};///< Converts between geometry formats.
			/** @} */

			/**
			 * @brief Observable notification codes broadcast by the Core.
			 * @details These codes are sent to observers when the engine execution state changes.
			 * Observers can subscribe to Core notifications to react to lifecycle events.
			 *
			 * Example usage:
			 * @code
			 * void MyObserver::onNotification(const ObservableTrait* obs, int code, const std::any& data)
			 * {
			 *     if (code == Core::ExecutionPaused) {
			 *         // Pause audio, physics, etc.
			 *     }
			 * }
			 * @endcode
			 * @version 0.8.35
			 */
			enum NotificationCode
			{
				EnteringMainLoop,             ///< Engine main loop has started.
				ExitingMainLoop,              ///< Engine main loop is stopped.
				ExecutionPaused,              ///< Engine execution is paused (game pause).
				ExecutionResumed,             ///< Engine execution has resumed after pause.
				ExecutionStopping,            ///< Engine is shutting down.
				ExecutionStopped,             ///< Engine is stopped.
				SurfaceRefreshed,			  ///< Render surface was recreated (resize, etc.).
				/* Enumeration boundary. */
				MaxEnum                       ///< Sentinel value for iteration.
			};

			/**
			 * @brief Defines the engine startup mode.
			 * @details Determines how the engine should behave after argument parsing
			 * and initial setup. This allows running auxiliary tools without full
			 * engine initialization.
			 * @version 0.8.35
			 */
			enum class StartupMode : uint8_t
			{
				Error,     ///< An error occurred during initialization; engine cannot start.
				ToolsMode, ///< Engine runs in tools mode (vulkanInfo, geometry tools, etc.).
				Continue   ///< Normal startup; proceed with full engine initialization.
			};

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Core (const Core & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Core (Core && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Core &
			 */
			Core & operator= (const Core & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Core &
			 */
			Core & operator= (Core && copy) noexcept = delete;

			/**
			 * @brief Destructs the engine core.
			 */
			~Core () override;

			/**
			 * @brief Runs the engine main loop.
			 * @details This is the main entry point for engine execution. The method performs
			 * the following sequence:
			 * 1. Calls initializeCoreLevel() to set up all secondary services
			 * 2. Calls onCoreStarted() to allow application-specific initialization
			 * 3. Enters the main loop (processing events, logic, and rendering)
			 * 4. On exit, calls terminate() which invokes onBeforeCoreStop()
			 *
			 * The main loop continues until stop() is called or a fatal error occurs.
			 *
			 * @return true if the engine executed and shut down successfully, false on error.
			 * @see stop()
			 * @see onCoreStarted()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool run () noexcept;

			/**
			 * @brief Pauses the engine main loop.
			 * @details Suspends logic processing and rendering while keeping the window
			 * responsive. Useful for game pause menus. Broadcasts ExecutionPaused
			 * notification to all observers.
			 * @note The engine must be pausable (controlled by internal state).
			 * @see resume()
			 * @see NotificationCode::ExecutionPaused
			 * @version 0.8.35
			 */
			void pause () noexcept;

			/**
			 * @brief Resumes the engine main loop after a pause.
			 * @details Restores normal engine operation. Broadcasts ExecutionResumed
			 * notification to all observers.
			 * @see pause()
			 * @see NotificationCode::ExecutionResumed
			 * @version 0.8.35
			 */
			void resume () noexcept;

			/**
			 * @brief Stops the engine and initiates shutdown.
			 * @details This method signals the main loop to exit. The shutdown sequence:
			 * 1. Broadcasts ExecutionStopped notification
			 * 2. Calls onBeforeCoreStop() for application cleanup
			 * 3. Terminates all services in reverse initialization order
			 *
			 * @warning After calling stop(), the engine cannot be restarted.
			 * @see run()
			 * @see onBeforeCoreStop()
			 * @see NotificationCode::ExecutionStopped
			 * @version 0.8.35
			 */
			void stop () noexcept;

			/**
			 * @brief Handles files dropped onto the application window.
			 * @details This method is called when users drag and drop files onto the window.
			 * Typical use cases include loading scene files, importing assets, or opening
			 * project files. The method delegates to onCoreOpenFiles() for application-specific
			 * handling.
			 * @param filepaths A reference to a vector of filesystem paths representing the dropped files.
			 * @see onCoreOpenFiles()
			 * @version 0.8.35
			 */
			void openFiles (const std::vector< std::filesystem::path > & filepaths) noexcept;

			/**
			 * @brief Suspends engine execution to run an external system command.
			 * @details Pauses all engine processing, executes the specified command through
			 * the system shell, then resumes engine operation. Useful for launching external
			 * editors, file managers, or other tools.
			 * @warning This blocks the entire engine. Use sparingly and only for user-initiated actions.
			 * @param command The shell command to execute.
			 * @version 0.8.35
			 */
			void hangExecution (const std::string & command) noexcept;

			/**
			 * @brief Displays an on-screen notification message.
			 * @details The notification appears in the overlay and fades out after the
			 * specified duration. Useful for non-intrusive user feedback.
			 * @param message The message text to display.
			 * @param duration Display duration in milliseconds. Default is Notifier::DefaultDuration (3000ms).
			 * @see Notifier
			 * @version 0.8.35
			 */
			void
			notifyUser (const std::string & message, uint32_t duration = Notifier::DefaultDuration) noexcept
			{
				// TODO: On no duration expressed, use the length of the string to compute an optimal duration for the user to read.
				this->notifier().push(message, duration);
			}

			/**
			 * @brief Displays an on-screen notification message from a BlobTrait.
			 * @details Overload accepting a BlobTrait for building messages with stream syntax.
			 * @code
			 * notifyUser(Libs::BlobTrait{} << "Score: " << score);
			 * @endcode
			 * @param message A BlobTrait containing the message text.
			 * @param duration Display duration in milliseconds. Default is Notifier::DefaultDuration (3000ms).
			 * @see Notifier
			 * @see Libs::BlobTrait
			 * @version 0.8.35
			 */
			void
			notifyUser (const Libs::BlobTrait & message, uint32_t duration = Notifier::DefaultDuration) noexcept
			{
				// TODO: On no duration expressed, use the length of the string to compute an optimal duration for the user to read.
				this->notifier().push(message.get(), duration);
			}

			/**
			 * @brief Checks if the application was launched with --help.
			 * @details When true, the application should display help text and exit
			 * without initializing the graphics subsystem.
			 * @return true if help was requested, false otherwise.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			showHelp () const noexcept
			{
				return m_showHelp;
			}

			/**
			 * @brief Returns the application identification structure.
			 * @details Contains application name, version, organization, and domain
			 * as specified in the constructor.
			 * @return Const reference to the Identification structure.
			 * @see Identification
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Identification &
			identification () const noexcept
			{
				return m_identification;
			}

			/**
			 * @brief Returns the core help service.
			 * @details Provides access to command-line help text and argument documentation.
			 * @return Const reference to the Help service.
			 * @see Help
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Help &
			coreHelp () const noexcept
			{
				return m_coreHelp;
			}

			/** @name Service Accessors
			 * @brief Accessors for engine subsystem services.
			 * @details All services are initialized during Core construction and remain
			 * valid for the lifetime of the Core object. Services are categorized as:
			 * - **Primary**: Available immediately after construction
			 * - **Secondary**: Available after graphics context initialization
			 * @{
			 */

			/**
			 * @brief Returns the primary services container (const).
			 * @details PrimaryServices bundles system info, user info, arguments,
			 * tracer, filesystem, settings, and network manager.
			 * @return Const reference to PrimaryServices.
			 * @see PrimaryServices
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const PrimaryServices &
			primaryServices () const noexcept
			{
				return m_primaryServices;
			}

			/**
			 * @brief Returns the primary services container.
			 * @return Reference to PrimaryServices.
			 * @see PrimaryServices
			 * @version 0.8.35
			 */
			[[nodiscard]]
			PrimaryServices &
			primaryServices () noexcept
			{
				return m_primaryServices;
			}

			/**
			 * @brief Returns the console controller service.
			 * @details Manages console commands and variables. Allows registering
			 * custom commands and variables accessible from the in-game console.
			 * @return Reference to Console::Controller.
			 * @see Console::Controller
			 * @see Console::Controllable
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Console::Controller &
			consoleController () noexcept
			{
				return m_consoleController;
			}

			/**
			 * @brief Returns the console controller service (const).
			 * @return Const reference to Console::Controller.
			 * @see Console::Controller
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Console::Controller &
			consoleController () const noexcept
			{
				return m_consoleController;
			}

			/**
			 * @brief Returns the resource manager service.
			 * @details Handles loading, caching, and lifecycle of all engine resources
			 * (textures, meshes, sounds, etc.). Implements fail-safe resource loading.
			 * @return Reference to Resources::Manager.
			 * @see Resources::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Resources::Manager &
			resourceManager () noexcept
			{
				return m_resourceManager;
			}

			/**
			 * @brief Returns the resource manager service (const).
			 * @return Const reference to Resources::Manager.
			 * @see Resources::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Resources::Manager &
			resourceManager () const noexcept
			{
				return m_resourceManager;
			}

			/**
			 * @brief Returns the user service.
			 * @details Manages user preferences, settings persistence, and
			 * user-specific configuration.
			 * @return Reference to User.
			 * @see User
			 * @version 0.8.35
			 */
			[[nodiscard]]
			User &
			user () noexcept
			{
				return m_user;
			}

			/**
			 * @brief Returns the user service (const).
			 * @return Const reference to User.
			 * @see User
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const User &
			user () const noexcept
			{
				return m_user;
			}

			/**
			 * @brief Returns the platform manager service.
			 * @details Provides platform-specific operations and abstractions
			 * for OS-level functionality.
			 * @return Reference to PlatformManager.
			 * @see PlatformManager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			PlatformManager &
			platformManager () noexcept
			{
				return m_platformManager;
			}

			/**
			 * @brief Returns the platform manager service (const).
			 * @return Const reference to PlatformManager.
			 * @see PlatformManager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const PlatformManager &
			platformManager () const noexcept
			{
				return m_platformManager;
			}

			/**
			 * @brief Returns the Vulkan instance service.
			 * @details Provides access to the Vulkan API abstraction layer,
			 * including physical device selection and instance management.
			 * @return Reference to Vulkan::Instance.
			 * @see Vulkan::Instance
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Vulkan::Instance &
			vulkanInstance () noexcept
			{
				return m_vulkanInstance;
			}

			/**
			 * @brief Returns the Vulkan instance service (const).
			 * @return Const reference to Vulkan::Instance.
			 * @see Vulkan::Instance
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Vulkan::Instance &
			vulkanInstance () const noexcept
			{
				return m_vulkanInstance;
			}

			/**
			 * @brief Returns the window service.
			 * @details Manages the application window, including creation,
			 * resizing, fullscreen toggle, and input event routing.
			 * @return Reference to Window.
			 * @see Window
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Window &
			window () noexcept
			{
				return m_window;
			}

			/**
			 * @brief Returns the window service (const).
			 * @return Const reference to Window.
			 * @see Window
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Window &
			window () const noexcept
			{
				return m_window;
			}

			/**
			 * @brief Returns the input manager service.
			 * @details Handles keyboard, mouse, and gamepad input. Provides
			 * input state queries and event listener registration.
			 * @return Reference to Input::Manager.
			 * @see Input::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Input::Manager &
			inputManager () noexcept
			{
				return m_inputManager;
			}

			/**
			 * @brief Returns the input manager service (const).
			 * @return Const reference to Input::Manager.
			 * @see Input::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Input::Manager &
			inputManager () const noexcept
			{
				return m_inputManager;
			}

			/**
			 * @brief Returns the graphics renderer service.
			 * @details Main rendering pipeline built on Vulkan. Handles frame
			 * submission, render passes, and GPU resource management.
			 * @return Reference to Graphics::Renderer.
			 * @see Graphics::Renderer
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Graphics::Renderer &
			graphicsRenderer () noexcept
			{
				return m_graphicsRenderer;
			}

			/**
			 * @brief Returns the graphics renderer service (const).
			 * @return Const reference to Graphics::Renderer.
			 * @see Graphics::Renderer
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Graphics::Renderer &
			graphicsRenderer () const noexcept
			{
				return m_graphicsRenderer;
			}

			/**
			 * @brief Returns the physics manager service.
			 * @details Handles physics simulation, collision detection, and rigid body
			 * dynamics. Uses a 4-entity type system (Boundaries, Ground, StaticEntity, Nodes).
			 * @return Reference to Physics::Manager.
			 * @see Physics::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Physics::Manager &
			physicsManager () noexcept
			{
				return m_physicsManager;
			}

			/**
			 * @brief Returns the physics manager service (const).
			 * @return Const reference to Physics::Manager.
			 * @see Physics::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Physics::Manager &
			physicsManager () const noexcept
			{
				return m_physicsManager;
			}

			/**
			 * @brief Returns the audio manager service.
			 * @details OpenAL-based 3D spatial audio system. Manages sound sources,
			 * listeners, and audio resource playback.
			 * @return Reference to Audio::Manager.
			 * @see Audio::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Audio::Manager &
			audioManager () noexcept
			{
				return m_audioManager;
			}

			/**
			 * @brief Returns the audio manager service (const).
			 * @return Const reference to Audio::Manager.
			 * @see Audio::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Audio::Manager &
			audioManager () const noexcept
			{
				return m_audioManager;
			}

			/**
			 * @brief Returns the overlay manager service.
			 * @details ImGui-based 2D overlay system for UI, debug panels,
			 * and heads-up display elements.
			 * @return Reference to Overlay::Manager.
			 * @see Overlay::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Overlay::Manager &
			overlayManager () noexcept
			{
				return m_overlayManager;
			}

			/**
			 * @brief Returns the overlay manager service (const).
			 * @return Const reference to Overlay::Manager.
			 * @see Overlay::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Overlay::Manager &
			overlayManager () const noexcept
			{
				return m_overlayManager;
			}

			/**
			 * @brief Returns the notifier service.
			 * @details Manages temporary on-screen notification messages.
			 * @return Reference to Notifier.
			 * @see Notifier
			 * @see notifyUser()
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Notifier &
			notifier () noexcept
			{
				return m_notifier;
			}

			/**
			 * @brief Returns the notifier service (const).
			 * @return Const reference to Notifier.
			 * @see Notifier
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Notifier &
			notifier () const noexcept
			{
				return m_notifier;
			}

			/**
			 * @brief Returns the scene manager service.
			 * @details Manages the hierarchical scene graph, entity creation,
			 * and scene lifecycle (loading, activation, transitions).
			 * @return Reference to Scenes::Manager.
			 * @see Scenes::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Scenes::Manager &
			sceneManager () noexcept
			{
				return m_sceneManager;
			}

			/**
			 * @brief Returns the scene manager service (const).
			 * @return Const reference to Scenes::Manager.
			 * @see Scenes::Manager
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const Scenes::Manager &
			sceneManager () const noexcept
			{
				return m_sceneManager;
			}

			/**
			 * @brief Returns the total engine execution time in microseconds.
			 * @details Accumulated time since run() was called. Useful for timing,
			 * animations, and performance monitoring.
			 * @return Engine lifetime in microseconds.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			uint64_t
			lifetime () const noexcept
			{
				return m_lifetime;
			}

			/**
			 * @brief Returns the current main loop cycle count.
			 * @details Incremented each frame. Useful for frame-based timing
			 * and periodic operations.
			 * @return Number of completed engine cycles.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			size_t
			cycle () const noexcept
			{
				return m_cycle;
			}

			/** @} */ // End of Service Accessors group

		protected:

			/**
			 * @brief Constructs the engine core.
			 * @details Initializes all primary services and prepares the engine for execution.
			 * The constructor is protected because Core is designed to be subclassed.
			 *
			 * Example:
			 * @code
			 * class MyGame : public EmEn::Core
			 * {
			 * public:
			 *     MyGame(int argc, char** argv)
			 *         : Core(argc, argv, "MyGame", {1, 0, 0}, "MyCompany", "mycompany.com")
			 *     {}
			 * };
			 * @endcode
			 *
			 * @param argc The argument count from main().
			 * @param argv The argument values from main().
			 * @param applicationName Name displayed in window title and logs. Default "UnknownApplication".
			 * @param applicationVersion Semantic version for the application. Default 0.0.0.
			 * @param applicationOrganization Organization name for settings paths. Default "UnknownOrganization".
			 * @param applicationDomain Domain for network identification. Default "localhost".
			 * @see Identification
			 * @version 0.8.35
			 */
			Core (int argc, char * * argv, const char * applicationName = "UnknownApplication", const Libs::Version & applicationVersion = {0, 0, 0}, const char * applicationOrganization = "UnknownOrganization", const char * applicationDomain = "localhost") noexcept;

#if IS_WINDOWS
			/**
			 * @brief Constructs the engine core (Windows wide-character variant).
			 * @details Windows-specific overload accepting wide-character arguments
			 * for proper Unicode path handling.
			 * @param argc The argument count from wmain().
			 * @param wargv The wide-character argument values from wmain().
			 * @param applicationName Name displayed in window title and logs. Default "UnknownApplication".
			 * @param applicationVersion Semantic version for the application. Default 0.0.0.
			 * @param applicationOrganization Organization name for settings paths. Default "UnknownOrganization".
			 * @param applicationDomain Domain for network identification. Default "unknown.org".
			 * @version 0.8.35
			 */
			Core (int argc, wchar_t * * wargv, const char * applicationName = "UnknownApplication", const Libs::Version & applicationVersion = {0, 0, 0}, const char * applicationOrganization = "UnknownOrganization", const char * applicationDomain = "unknown.org") noexcept;
#endif

			/**
			 * @brief Disables default keyboard handling by the Core.
			 * @details When enabled, the Core will not process unhandled keyboard events
			 * (like F11 for fullscreen). Use this when your application handles all
			 * keyboard input directly.
			 * @warning Only enable this if your application provides complete keyboard handling.
			 * @version 0.8.35
			 */
			void
			preventDefaultKeyBehaviors () noexcept
			{
				m_preventDefaultKeyBehaviors = true;
			}

			/**
			 * @brief Captures a screenshot and saves it to the user's images folder.
			 * @details The image is saved in PNG format with a timestamped filename.
			 * The exact path depends on the operating system.
			 * @return true if the screenshot was saved successfully, false otherwise.
			 * @version 0.8.35
			 */
			bool screenshot () noexcept;

			/**
			 * @brief Dumps all framebuffers to files for debugging.
			 * @details Saves each render target's content to separate image files.
			 * Useful for debugging rendering pipeline issues.
			 * @return true if all framebuffers were dumped successfully.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool dumpFramebuffer () const noexcept;

			/** @name Cursor Management
			 * @brief Methods for changing the mouse cursor appearance.
			 * @{
			 */

			/**
			 * @brief Sets the cursor to a standard system cursor type.
			 * @param cursorType The standard cursor type (arrow, hand, resize, etc.).
			 * @see CursorType
			 * @version 0.8.35
			 */
			void
			setCursor (CursorType cursorType) noexcept
			{
				m_cursorAtlas.setCursor(m_window, cursorType);
			}

			/**
			 * @brief Sets a custom cursor from a pixmap.
			 * @param label Unique identifier for caching this cursor.
			 * @param pixmap The cursor image data.
			 * @param hotSpot Click point offset from top-left corner. Default {0, 0}.
			 * @see Libs::PixelFactory::Pixmap
			 * @version 0.8.35
			 */
			void
			setCursor (const std::string & label, const Libs::PixelFactory::Pixmap< uint8_t > & pixmap, const std::array< int, 2 > & hotSpot = {0, 0}) noexcept
			{
				m_cursorAtlas.setCursor(m_window, label, pixmap, hotSpot);
			}

			/**
			 * @brief Sets a custom cursor from raw RGBA data.
			 * @warning Low-level interface for GLFW compatibility. Prefer other overloads.
			 * @param label Unique identifier for caching this cursor.
			 * @param size Cursor dimensions as {width, height}.
			 * @param data Raw RGBA pixel data (width * height * 4 bytes).
			 * @param hotSpot Click point offset from top-left corner. Default {0, 0}.
			 * @version 0.8.35
			 */
			void
			setCursor (const std::string & label, const std::array< int, 2 > & size, unsigned char * data, const std::array< int, 2 > & hotSpot = {0, 0}) noexcept
			{
				m_cursorAtlas.setCursor(m_window, label, size, data, hotSpot);
			}

			/**
			 * @brief Sets a custom cursor from an image resource.
			 * @details Recommended method for custom cursors as it integrates
			 * with the resource management system.
			 * @param imageResource The image resource to use as cursor.
			 * @param hotSpot Click point offset from top-left corner. Default {0, 0}.
			 * @see Graphics::ImageResource
			 * @version 0.8.35
			 */
			void
			setCursor (const std::shared_ptr< Graphics::ImageResource > & imageResource, const std::array< int, 2 > & hotSpot = {0, 0}) noexcept
			{
				m_cursorAtlas.setCursor(m_window, imageResource, hotSpot);
			}

			/**
			 * @brief Resets the cursor to the default arrow.
			 * @version 0.8.35
			 */
			void
			resetCursor () noexcept
			{
				m_cursorAtlas.resetCursor(m_window);
			}

			/** @} */ // End of Cursor Management group

			/**
			 * @brief Registers a user-defined service with the engine.
			 * @details User services are initialized after secondary services and
			 * terminated before them. Use this to integrate custom subsystems
			 * into the engine lifecycle.
			 * @param userService Pointer to a service implementing ServiceInterface.
			 * @return true if the service was registered successfully.
			 * @see ServiceInterface
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool enableUserService (ServiceInterface * userService) noexcept;

		private:

			/** @name Interface Implementations
			 * @brief Implementations of inherited interface methods.
			 * @{
			 */

			/** @copydoc EmEn::Input::KeyboardListenerInterface::onKeyPress() */
			bool onKeyPress (int32_t key, int32_t scancode, int32_t modifiers, bool repeat) noexcept final;

			/** @copydoc EmEn::Input::KeyboardListenerInterface::onKeyRelease() */
			bool onKeyRelease (int32_t key, int32_t scancode, int32_t modifiers) noexcept final;

			/** @copydoc EmEn::Input::KeyboardListenerInterface::onCharacterType() */
			bool onCharacterType (uint32_t unicode) noexcept final;

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept final;

			/** @copydoc EmEn::Console::Controllable::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

			/** @} */ // End of Interface Implementations group

			/** @name Internal Engine Methods
			 * @brief Core engine lifecycle and processing methods.
			 * @{
			 */

			/**
			 * @brief Initializes basic engine functionality (no graphics).
			 * @details Sets up primary services, parses arguments, and prepares
			 * for either tools mode or full engine initialization.
			 * @return true if initialization succeeded.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool initializeBaseLevel () noexcept;

			/**
			 * @brief Initializes all secondary services requiring graphics context.
			 * @details After this method, Graphics::Renderer, Audio::Manager,
			 * Input::Manager, and all other secondary services are available.
			 * @return true if initialization succeeded; false aborts engine startup.
			 * @version 0.8.35
			 */
			bool initializeCoreLevel () noexcept;

			/**
			 * @brief Initializes the rendering surface and swap-chain.
			 * @return true if screen initialization succeeded.
			 * @version 0.8.35
			 */
			bool initializeCoreScreen () noexcept;

			/**
			 * @brief Logic thread entry point.
			 * @details Runs physics simulation, scene updates, and calls onCoreProcessLogics().
			 * Executes on a dedicated thread separate from rendering.
			 * @version 0.8.35
			 */
			void logicsTask () noexcept;

			/**
			 * @brief Rendering thread entry point.
			 * @details Handles frame preparation, command buffer recording,
			 * and GPU submission. Executes on a dedicated thread.
			 * @version 0.8.35
			 */
			void renderingTask () noexcept;

			/**
			 * @brief Recreates the render surface after resize or mode change.
			 * @details Handles swap-chain recreation and notifies observers via
			 * ApplicationSurfaceRefreshed notification.
			 * @version 0.8.35
			 */
			void onWindowChanged () noexcept;

			/**
			 * @brief Performs engine shutdown sequence.
			 * @details Terminates all services in reverse initialization order.
			 * @return Exit code (0 for success).
			 * @version 0.8.35
			 */
			unsigned int terminate () noexcept;

			/**
			 * @brief Executes command-line tools without full engine initialization.
			 * @details Handles tools like vulkanInfo and geometry conversion.
			 * @return true if tools mode executed successfully.
			 * @see StartupMode::ToolsMode
			 * @version 0.8.35
			 */
			bool executeToolsMode () noexcept;

			/**
			 * @brief Processes and displays queued core messages.
			 * @details Shows error dialogs and shader compilation failures.
			 * @version 0.8.35
			 */
			void displayCoreMessages () noexcept;

			/** @} */ // End of Internal Engine Methods group

			/** @name Application Callbacks
			 * @brief Virtual methods to be implemented by derived application classes.
			 * @details These methods define the application lifecycle hooks. Pure virtual
			 * methods must be implemented; others have default implementations.
			 * @{
			 */

			/**
			 * @brief Called when a shader fails to compile.
			 * @details Default implementation notifies the user and queues the source
			 * code for display. Override to customize error handling.
			 * @param shaderIdentifier Name/path of the failed shader.
			 * @param sourceCode The shader source that failed to compile.
			 * @version 0.8.35
			 */
			virtual
			void
			onCoreShaderCompilationFailed (const std::string & shaderIdentifier, const std::string & sourceCode) noexcept
			{
				this->notifyUser(Libs::BlobTrait{} << "Shader '" << shaderIdentifier << "' compilation failed!");

				m_coreMessages.emplace(sourceCode);
			}

			/**
			 * @brief Called before secondary services are initialized.
			 * @details Use this hook to display help, validate configuration,
			 * or perform early checks that don't require graphics.
			 * @return true to abort startup (e.g., after showing help), false to continue.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			virtual
			bool
			onBeforeCoreSecondaryServicesInitialization () noexcept
			{
				return false;
			}

			/**
			 * @brief Called after all services are initialized, before the main loop.
			 * @details This is the primary initialization hook for applications.
			 * Load scenes, create entities, and set up game state here.
			 * @return true to enter main loop, false to abort with error.
			 * @version 0.8.35
			 */
			virtual bool onCoreStarted () noexcept = 0;

			/**
			 * @brief Hook called every main loop iteration.
			 * @details Override in derived classes for per-frame custom processing.
			 * Called after input processing and before rendering.
			 * @version 0.8.35
			 */
			virtual
			void
			onCoreMainLoopCycle () noexcept
			{
				/* Makes nothing special here. */
			}

			/**
			 * @brief Called every logic frame for game logic processing.
			 * @details Runs on the logic thread, separate from rendering.
			 * Update game state, AI, and non-physics logic here.
			 * @warning Thread safety: Do not access rendering resources directly.
			 * @param engineCycle The current frame number since engine start.
			 * @version 0.8.35
			 */
			virtual void onCoreProcessLogics (size_t engineCycle) noexcept = 0;

			/**
			 * @brief Called when the engine is paused.
			 * @details Pause game-specific systems (timers, AI, etc.) here.
			 * Physics and audio are paused automatically by the engine.
			 * @see pause()
			 * @version 0.8.35
			 */
			virtual
			void
			onCorePaused () noexcept
			{
				/* Nothing by default. */
			}

			/**
			 * @brief Called when the engine resumes from pause.
			 * @details Resume game-specific systems paused in onCorePaused().
			 * @see resume()
			 * @version 0.8.35
			 */
			virtual
			void
			onCoreResumed () noexcept
			{
				/* Nothing by default. */
			}

			/**
			 * @brief Called when the engine is stopping.
			 * @details Clean up game state, save progress, and release
			 * application-specific resources here. Services are still available.
			 * @see stop()
			 * @version 0.8.35
			 */
			virtual
			void
			onBeforeCoreStop () noexcept
			{
				/* Nothing by default. */
			}

			/**
			 * @brief Application-level key press handler.
			 * @details Called after Core processes its reserved keys (F11 fullscreen, etc.).
			 * Implement to handle application-specific keyboard shortcuts.
			 * @param key Platform-independent key code (GLFW key codes).
			 * @param scancode Platform-specific scancode.
			 * @param modifiers Modifier key bitmask (Shift, Ctrl, Alt, Super).
			 * @param repeat true if this is a key repeat event.
			 * @return true if the event was handled, false to pass to next listener.
			 * @version 0.8.35
			 */
			virtual
			bool
			onCoreKeyPress ([[maybe_unused]] int32_t key, [[maybe_unused]] int32_t scancode, [[maybe_unused]] int32_t modifiers, [[maybe_unused]] bool repeat) noexcept
			{
				return false;
			}

			/**
			 * @brief Application-level key release handler.
			 * @details Called after Core processes reserved keys.
			 * @param key Platform-independent key code (GLFW key codes).
			 * @param scancode Platform-specific scancode.
			 * @param modifiers Modifier key bitmask (Shift, Ctrl, Alt, Super).
			 * @return true if the event was handled, false to pass to next listener.
			 * @version 0.8.35
			 */
			virtual
			bool
			onCoreKeyRelease ([[maybe_unused]] int32_t key, [[maybe_unused]] int32_t scancode, [[maybe_unused]] int32_t modifiers) noexcept
			{
				return false;
			}

			/**
			 * @brief Application-level character input handler.
			 * @details Called for text input (after key translation). Use for text fields
			 * and Unicode character input rather than key press events.
			 * @param unicode The Unicode code point of the typed character.
			 * @return true if the event was handled, false to pass to next listener.
			 * @version 0.8.35
			 */
			virtual
			bool
			onCoreCharacterType ([[maybe_unused]] uint32_t unicode) noexcept
			{
				return false;
			}

			/**
			 * @brief Application-level notification handler.
			 * @details Called for notifications not consumed by Core. Implement to
			 * react to subsystem events (window resize, resource loading, etc.).
			 * @param observable The source of the notification.
			 * @param notificationCode Notification type identifier.
			 * @param data Optional notification payload.
			 * @return true to remain attached as observer, false to detach.
			 * @version 0.8.35
			 */
			virtual
			bool
			onCoreNotification ([[maybe_unused]] const ObservableTrait * observable, [[maybe_unused]] int notificationCode, [[maybe_unused]] const std::any & data) noexcept
			{
				return true;
			}

			/**
			 * @brief Called when files are dropped onto the application window.
			 * @details Implement to handle drag-and-drop file loading.
			 * @param filepaths Paths to the dropped files.
			 * @see openFiles()
			 * @version 0.8.35
			 */
			virtual
			void
			onCoreOpenFiles ([[maybe_unused]] const std::vector< std::filesystem::path > & filepaths) noexcept
			{
				/* Nothing by default. */
			}

			/**
			 * @brief Called when the core engine refresh the visible surface of the application.
			 * @note The overlay manager and the scene manager is already refreshed at this point.
			 * @details Implement to handle a resize into the user-application.
			 * @see refreshSurface()
			 * @version 0.8.35
			 */
			virtual
			void
			onCoreSurfaceRefreshed () noexcept
			{
				/* Nothing by default. */
			}

			/** @} */ // End of Application Callbacks group

			/** @name Member Variables
			 * @brief Internal state and service instances.
			 * @details Services are initialized in declaration order and destroyed
			 * in reverse order. Primary services are listed first, then secondary.
			 * @{
			 */

			Identification m_identification;                                           ///< Application identity (name, version, org).
			Help m_coreHelp{"Core engine"};                                            ///< Command-line help system.

			/* Primary Services - Available immediately after construction. */
			PrimaryServices m_primaryServices;                                         ///< Bundled primary service container.
			Console::Controller m_consoleController{m_primaryServices};                ///< Console command processor.
			Resources::Manager m_resourceManager{m_primaryServices, m_graphicsRenderer}; ///< Resource loading and caching.
			User m_user{m_primaryServices};                                            ///< User preferences and settings.

			/* Secondary Services - Require graphics context. */
			PlatformManager m_platformManager{m_primaryServices};                      ///< Platform abstraction layer.
			Vulkan::Instance m_vulkanInstance{m_identification, m_primaryServices};    ///< Vulkan instance wrapper.
			Window m_window{m_primaryServices, m_vulkanInstance, m_identification};    ///< Application window.
			Input::Manager m_inputManager{m_primaryServices, m_window};                ///< Input device management.
			Graphics::Renderer m_graphicsRenderer{m_primaryServices, m_vulkanInstance, m_window}; ///< Vulkan rendering pipeline.
			Physics::Manager m_physicsManager{m_primaryServices, m_vulkanInstance};    ///< Physics simulation.
			Audio::Manager m_audioManager{m_primaryServices, m_resourceManager};       ///< OpenAL audio system.
			Overlay::Manager m_overlayManager{m_primaryServices, m_window, m_graphicsRenderer}; ///< ImGui overlay system.
			Notifier m_notifier{m_resourceManager, m_overlayManager};                  ///< On-screen notifications.
			Scenes::Manager m_sceneManager{m_primaryServices, m_resourceManager, m_inputManager, m_graphicsRenderer, m_audioManager}; ///< Scene graph management.

			/* Service tracking. */
			std::vector< ServiceInterface * > m_primaryServicesEnabled;   ///< Enabled primary service pointers.
			std::vector< ServiceInterface * > m_secondaryServicesEnabled; ///< Enabled secondary service pointers.
			std::vector< ServiceInterface * > m_userServiceEnabled;       ///< User-registered service pointers.

			/* Runtime state. */
			CursorAtlas m_cursorAtlas;                          ///< Custom cursor cache.
			std::thread m_logicsThread;                         ///< Logic processing thread.
			std::thread m_renderingThread;                      ///< Rendering thread.
			uint64_t m_lifetime{0};                             ///< Total runtime in microseconds.
			size_t m_cycle{0};                                  ///< Main loop iteration count.
			StartupMode m_startupMode{StartupMode::Continue};   ///< Startup behavior mode.
			std::queue< std::string > m_coreMessages;           ///< Pending messages for display. @todo Display in ImGui.

			/* Control flags. */
			bool m_isMainLoopRunning{true};          ///< Main loop active flag.
			bool m_isLogicsLoopRunning{true};        ///< Logic thread active flag.
			bool m_isRenderingLoopRunning{true};     ///< Render thread active flag.
			bool m_pausable{false};                  ///< Whether pause is currently allowed.
			bool m_paused{false};                    ///< Current pause state.
			bool m_showHelp{false};                  ///< Help display requested via --help.
			bool m_preventDefaultKeyBehaviors{false}; ///< Disable Core's default key handling.
			bool m_enableStatistics{false};			 ///< Enable statistics display in the terminal.
			bool m_windowChanged{false};			 ///< Tells to Core the window has changed.

			/** @} */ // End of Member Variables group
	};
}
