/*
 * src/Resources/ResourceTrait.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <filesystem>
#include <typeindex>
#include <type_traits>

/* Third-party inclusions. */
#ifndef JSON_USE_EXCEPTION
#define JSON_USE_EXCEPTION 0
#endif
#include "json/json.h"

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"
#include "Libs/FlagTrait.hpp"
#include "Libs/ObservableTrait.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Resources
	{
		class ContainerInterface;

		template< typename resource_t >
		class Container;
	}

	namespace Graphics
	{
		class Renderer;
	}

	class FileSystem;
	class Settings;
}

namespace EmEn::Resources
{
	/* Forward declaration for requires clause. */
	class ResourceTrait;

	/**
	 * @class AbstractServiceProvider
	 * @brief Abstract base class providing essential services for resource loading and management.
	 *
	 * This interface serves as a bridge between the resource management system and the engine's
	 * core services. It provides access to the FileSystem for loading resource data and the
	 * Graphics::Renderer for GPU resource creation. The class also offers type-safe access to
	 * resource containers through template methods.
	 *
	 * The class is non-copyable and non-movable due to reference members that must remain
	 * stable throughout the lifetime of the service provider.
	 *
	 * @note This is an abstract base class. Concrete implementations must provide the
	 *	   getContainerInternal() and update() methods.
	 *
	 * @see Manager
	 * @see Container
	 * @see ContainerInterface
	 * @see FileSystem
	 * @see Graphics::Renderer
	 *
	 * @version 0.8.45
	 */
	class AbstractServiceProvider
	{
		public:

			/**
			 * @brief Deleted copy constructor (reference members).
			 */
			AbstractServiceProvider (const AbstractServiceProvider &) = delete;

			/**
			 * @brief Deleted copy assignment operator (reference members).
			 */
			AbstractServiceProvider & operator= (const AbstractServiceProvider &) = delete;

			/**
			 * @brief Deleted move constructor (reference members).
			 */
			AbstractServiceProvider (AbstractServiceProvider &&) = delete;

			/**
			 * @brief Deleted move assignment operator (reference members).
			 */
			AbstractServiceProvider & operator= (AbstractServiceProvider &&) = delete;

			/**
			 * @brief Destructs the service provider.
			 */
			virtual ~AbstractServiceProvider() = default;

			/**
			 * @brief Returns access to the file system for resource loading operations.
			 *
			 * Provides read-only access to the FileSystem service used to load resource
			 * data from disk, archives, or other storage media.
			 *
			 * @return Const reference to the FileSystem instance.
			 *
			 * @see FileSystem
			 */
			[[nodiscard]]
			const FileSystem &
			fileSystem () const noexcept
			{
				return m_fileSystem;
			}

			/**
			 * @brief Returns access to the settings service for configuration retrieval.
			 *
			 * Provides access to the Settings service used to retrieve and modify
			 * engine configuration values during resource loading.
			 *
			 * @return Non-const reference to the Settings instance.
			 *
			 * @see Settings
			 */
			[[nodiscard]]
			Settings &
			settings () const noexcept
			{
				return m_settings;
			}

			/**
			 * @brief Returns access to the graphics renderer for GPU resource creation.
			 *
			 * Provides access to the Graphics::Renderer service used to create and manage
			 * GPU-side resources such as textures, buffers, and shaders. Despite the const
			 * method qualifier, the returned reference is non-const to allow resource
			 * modifications.
			 *
			 * @return Non-const reference to the Graphics::Renderer instance.
			 *
			 * @note The non-const reference allows GPU resource creation even from const
			 *	   methods of derived classes.
			 *
			 * @see Graphics::Renderer
			 */
			[[nodiscard]]
			Graphics::Renderer &
			graphicsRenderer () const noexcept
			{
				return m_graphicsRenderer;
			}

			/**
			 * @brief Returns a reference to the container for a specific resource type.
			 *
			 * Provides type-safe access to the resource container that manages instances
			 * of the specified resource type. The method uses RTTI to identify the correct
			 * container and performs a static cast to the typed container interface.
			 *
			 * @tparam resource_t The resource type to retrieve the container for. Must derive
			 *					from ResourceTrait (e.g., SoundResource, ImageResource, MeshResource).
			 *
			 * @return Pointer to the typed Container<resource_t>, or nullptr if the
			 *		 container for this type is not registered.
			 *
			 * @note The returned pointer may be nullptr if the resource type is not
			 *	   registered with the service provider.
			 *
			 * @see Container
			 * @see getContainerInternal()
			 */
			template< typename resource_t >
				requires std::is_base_of_v< ResourceTrait, resource_t >
			[[nodiscard]]
			Container< resource_t > *
			container () noexcept
			{
				const std::type_index typeId = typeid(resource_t);

				ContainerInterface * nonTypedContainer = this->getContainerInternal(typeId);

				return static_cast< Container< resource_t > * >(nonTypedContainer);
			}

			/**
			 * @brief Returns a const reference to the container for a specific resource type.
			 *
			 * Provides const type-safe access to the resource container that manages instances
			 * of the specified resource type. This const overload ensures read-only access to
			 * the container and its resources.
			 *
			 * @tparam resource_t The resource type to retrieve the container for. Must derive
			 *					from ResourceTrait (e.g., SoundResource, ImageResource, MeshResource).
			 *
			 * @return Const pointer to the typed Container<resource_t>, or nullptr if the
			 *		 container for this type is not registered.
			 *
			 * @note The returned pointer may be nullptr if the resource type is not
			 *	   registered with the service provider.
			 *
			 * @see Container
			 * @see getContainerInternal()
			 */
			template< typename resource_t >
				requires std::is_base_of_v< ResourceTrait, resource_t >
			[[nodiscard]]
			const Container< resource_t > *
			container () const noexcept
			{
				const std::type_index typeId = typeid(resource_t);

				const ContainerInterface * nonTypedContainer = this->getContainerInternal(typeId);

				return static_cast< const Container< resource_t > * >(nonTypedContainer);
			}

			/**
			 * @brief Updates resource store from another resource JSON definition.
			 *
			 * Pure virtual method that must be implemented by derived classes to handle
			 * loading and updating resources from a JSON definition. This method typically
			 * parses the JSON structure and creates or updates resource containers accordingly.
			 *
			 * @param root The root JSON value object containing resource definitions.
			 *
			 * @return true if the update was successful, false otherwise.
			 *
			 * @note This is a pure virtual method that must be implemented by concrete
			 *	   service provider classes.
			 */
			virtual bool update (const Json::Value & root) noexcept = 0;

		protected:

			/**
			 * @brief Constructs a service provider with required service references.
			 *
			 * Protected constructor ensures this class can only be instantiated through
			 * derived classes. The constructor binds references to the FileSystem, Settings,
			 * and Graphics::Renderer services that will be used throughout the lifetime of
			 * the service provider.
			 *
			 * @param fileSystem Const reference to the FileSystem service for loading
			 *				   resource data from storage.
			 * @param settings Non-const reference to the Settings service for configuration
			 *				 retrieval and modification.
			 * @param graphicsRenderer Non-const reference to the Graphics::Renderer
			 *						 service for creating GPU resources.
			 *
			 * @note The reference members make this class non-copyable and non-movable.
			 */
			AbstractServiceProvider (const FileSystem & fileSystem, Settings & settings, Graphics::Renderer & graphicsRenderer) noexcept
				: m_fileSystem{fileSystem},
				m_settings{settings},
				m_graphicsRenderer{graphicsRenderer}
			{

			}

			/**
			 * @brief Returns the container for a specific resource type (internal implementation).
			 *
			 * Pure virtual method that must be implemented by derived classes to provide
			 * the internal mapping between type indices and container instances. This method
			 * is called by the public template container<T>() methods.
			 *
			 * @param typeIndex The std::type_index identifying the resource type.
			 *
			 * @return Pointer to the ContainerInterface for the specified type, or nullptr
			 *		 if no container is registered for this type.
			 *
			 * @note This is a pure virtual method that must be implemented by concrete
			 *	   service provider classes.
			 *
			 * @see container()
			 * @see ContainerInterface
			 */
			[[nodiscard]]
			virtual ContainerInterface * getContainerInternal (const std::type_index & typeIndex) noexcept = 0;

			/**
			 * @brief Returns the container for a specific resource type (internal implementation, const).
			 *
			 * Const pure virtual method that must be implemented by derived classes to provide
			 * read-only access to the internal mapping between type indices and container instances.
			 * This method is called by the public const template container<T>() methods.
			 *
			 * @param typeIndex The std::type_index identifying the resource type.
			 *
			 * @return Const pointer to the ContainerInterface for the specified type, or nullptr
			 *		 if no container is registered for this type.
			 *
			 * @note This is a pure virtual method that must be implemented by concrete
			 *	   service provider classes.
			 *
			 * @see container()
			 * @see ContainerInterface
			 */
			[[nodiscard]]
			virtual const ContainerInterface * getContainerInternal (const std::type_index & typeIndex) const noexcept = 0;

		private:

			const FileSystem & m_fileSystem;			  ///< Reference to the FileSystem service for loading resource data.
			Settings & m_settings;						///< Reference to the Settings service for configuration retrieval.
			Graphics::Renderer & m_graphicsRenderer;	  ///< Reference to the Graphics::Renderer service for GPU resource creation.
	};

	/**
	 * @class ResourceTrait
	 * @brief Base trait for all loadable resources in the Emeraude Engine with dependency management.
	 *
	 * This abstract class provides the foundation for all engine resources (textures, meshes, audio, etc.)
	 * with automatic dependency tracking, asynchronous loading support, and state management.
	 *
	 * ## Resource Loading State Machine
	 * Resources transition through the following states:
	 * - `Unloaded` → Initial state when resource is created
	 * - `Enqueuing` → Resource is gathering dependencies (automatic loading mode)
	 * - `ManualEnqueuing` → Resource is gathering dependencies (manual loading mode)
	 * - `Loading` → All dependencies identified, waiting for them to complete
	 * - `Loaded` → Resource and all dependencies successfully loaded
	 * - `Failed` → Loading failed at any stage (terminal state)
	 *
	 * ## Loading Modes
	 * 1. **Automatic Loading**: Call beginLoading() internally, then setLoadSuccess()
	 * 2. **Manual Loading**: Call enableManualLoading(), add dependencies, then setManualLoadSuccess()
	 * 3. **Direct Loading**: Set hint via setDirectLoadingHint() for synchronous loading
	 *
	 * ## Dependency System
	 * Resources can depend on other resources, forming a directed acyclic graph (DAG):
	 * - Parent resources wait for all child dependencies to load
	 * - Dependencies automatically notify parents when loaded
	 * - Thread-safe dependency tracking via m_dependenciesAccess mutex
	 * - Circular dependencies are not detected and will cause deadlock
	 *
	 * ## Thread Safety
	 * - Dependency vectors (m_parentsToNotify, m_dependenciesToWaitFor) are protected by m_dependenciesAccess
	 * - Multiple threads can safely add dependencies and check loading status
	 * - Status transitions are atomic within mutex-protected sections
	 * - Observers are notified on LoadFinished/LoadFailed events
	 *
	 * ## Observable Notifications
	 * The resource emits notifications via ObservableTrait:
	 * - `LoadFinished` - Resource and all dependencies successfully loaded
	 * - `LoadFailed` - Loading failed (check logs for details)
	 *
	 * @note [OBS][SHARED-OBSERVABLE] - This class uses the observer pattern and requires shared_ptr.
	 * @extends std::enable_shared_from_this A resource must be managed by shared_ptr for dependency tracking.
	 * @extends EmEn::Libs::NameableTrait A resource is always named for identification and logging.
	 * @extends EmEn::Libs::FlagTrait<uint32_t> Construction flags control loading behavior.
	 * @extends EmEn::Libs::ObservableTrait Observable pattern for loading state tracking.
	 * @see AbstractServiceProvider, Container, Manager.
	 * @version 0.8.45
	 */
	class ResourceTrait : public std::enable_shared_from_this< ResourceTrait >, public Libs::NameableTrait, public Libs::FlagTrait< uint32_t >, public Libs::ObservableTrait
	{
		public:

			/**
			 * @enum NotificationCode
			 * @brief Observable notification codes emitted during resource loading lifecycle.
			 *
			 * These codes are sent via the ObservableTrait::notify() mechanism to observers
			 * that have subscribed to this resource's loading events.
			 *
			 * @see ObservableTrait::notify(), ObservableTrait::subscribe()
			 */
			enum NotificationCode
			{
				LoadFinished, ///< Resource and all dependencies successfully loaded and ready for use
				LoadFailed,   ///< Loading failed at any stage (check logs for detailed error messages)
				/* Enumeration boundary. */
				MaxEnum	   ///< Internal boundary marker, not a valid notification code
			};

			/**
			 * @brief Global flag to enable verbose resource loading information in the terminal.
			 *
			 * When enabled, detailed loading progress messages are printed for all resources,
			 * including dependency tracking, status transitions, and completion notifications.
			 *
			 * @note This affects all ResourceTrait instances globally
			 * @warning Enabling this can significantly increase console output volume
			 */
			inline static bool s_showInformation{false};

			/**
			 * @brief Global flag to suppress resource conversion warning messages.
			 *
			 * When true (default), conversion warnings (e.g., format conversions, data loss warnings)
			 * are suppressed. Set to "false" to see all conversion-related warnings.
			 *
			 * @note This affects all ResourceTrait instances globally
			 */
			inline static bool s_quietConversion{true};

			/**
			 * @brief Copy constructor (deleted).
			 *
			 * Resources cannot be copied due to complex dependency relationships and shared_ptr requirements.
			 *
			 * @param copy A reference to the copied instance
			 */
			ResourceTrait (const ResourceTrait & copy) noexcept = delete;

			/**
			 * @brief Move constructor (deleted).
			 *
			 * Resources cannot be moved due to complex dependency relationships and shared_ptr requirements.
			 *
			 * @param copy A reference to the moved instance
			 */
			ResourceTrait (ResourceTrait && copy) noexcept = delete;

			/**
			 * @brief Copy assignment operator (deleted).
			 *
			 * Resources cannot be copy-assigned due to complex dependency relationships and shared_ptr requirements.
			 *
			 * @param copy A reference to the copied instance
			 * @return ResourceTrait& (never returns, deleted)
			 */
			ResourceTrait & operator= (const ResourceTrait & copy) noexcept = delete;

			/**
			 * @brief Move assignment operator (deleted).
			 *
			 * Resources cannot be move-assigned due to complex dependency relationships and shared_ptr requirements.
			 *
			 * @param copy A reference to the moved instance
			 * @return ResourceTrait& (never returns, deleted)
			 */
			ResourceTrait & operator= (ResourceTrait && copy) noexcept = delete;

			/**
			 * @brief Destructs the resource and validates its final state.
			 *
			 * The destructor checks the resource status and logs warnings/errors for abnormal states:
			 * - Warns if destroyed while in Enqueuing or ManualEnqueuing state
			 * - Errors if destroyed while Loading
			 * - Errors if Loaded but still has parent or dependency pointers (memory leak indicator)
			 *
			 * @post Parent and dependency lists should be empty for properly loaded resources
			 * @see isLoaded(), status()
			 */
			~ResourceTrait () override;

			/**
			 * @brief Returns whether the resource is the top of a loadable object chain.
			 *
			 * A top resource has no parent resources waiting for it. Top resources are typically
			 * the root of a dependency graph and trigger the final LoadFinished notification.
			 *
			 * @return true if this resource has no parents waiting for its loading, false otherwise
			 * @note Thread-safe to call at any time
			 * @see m_parentsToNotify
			 */
			[[nodiscard]]
			bool
			isTopResource () const noexcept
			{
				return m_parentsToNotify.empty();
			}

			/**
			 * @brief Returns the number of dependencies still waiting to be loaded.
			 *
			 * This count represents pending dependencies that haven't completed loading yet.
			 * Once all dependencies are loaded, this returns 0 and the resource can complete loading.
			 *
			 * @return The number of unloaded dependencies this resource is waiting for
			 * @note Thread-safe to call at any time
			 * @see m_dependenciesToWaitFor, addDependency()
			 */
			[[nodiscard]]
			size_t
			dependencyCount () const noexcept
			{
				return m_dependenciesToWaitFor.size();
			}

			/**
			 * @brief Returns whether this resource is in the initial Unloaded state.
			 *
			 * A resource is Unloaded when freshly created and no loading process has been initiated.
			 * This differs from "not yet loaded" which includes Enqueuing and Loading states.
			 *
			 * @return true if status is Status::Unloaded, false otherwise
			 * @note Useful when using Container::getOrCreate() to detect newly created resources
			 * @warning To check if resource is not ready, use !isLoaded() instead
			 * @see isLoaded(), isLoading(), status()
			 */
			[[nodiscard]]
			bool
			isUnloaded () const noexcept
			{
				return m_status == Status::Unloaded;
			}

			/**
			 * @brief Returns whether this resource is currently in the Loading state.
			 *
			 * Loading state means all dependencies have been identified and the resource is
			 * waiting for them to complete before finalizing.
			 *
			 * @return true if status is Status::Loading, false otherwise
			 * @note Thread-safe to call at any time
			 * @see isLoaded(), isUnloaded(), status()
			 */
			[[nodiscard]]
			bool
			isLoading () const noexcept
			{
				return m_status == Status::Loading;
			}

			/**
			 * @brief Returns whether this resource is fully loaded and ready for use.
			 *
			 * A loaded resource has completed all loading stages, all dependencies are satisfied,
			 * and the resource is ready to be used by the application.
			 *
			 * @return true if status is Status::Loaded, false otherwise
			 * @note Thread-safe to call at any time
			 * @see isLoading(), isUnloaded(), status(), onDependenciesLoaded()
			 */
			[[nodiscard]]
			bool
			isLoaded () const noexcept
			{
				return m_status == Status::Loaded;
			}

			/**
			 * @brief Enables manual loading mode for this resource.
			 *
			 * Manual loading allows external code to control when dependencies are added and when
			 * loading completes. Call this before adding dependencies, then call setManualLoadSuccess()
			 * when ready to finalize.
			 *
			 * @return true if successfully transitioned to ManualEnqueuing state, false if already loading/loaded
			 * @pre Resource status must be Unloaded
			 * @post Resource status becomes ManualEnqueuing if successful
			 * @note Fails if resource is already in Loading, Loaded, or Failed state
			 * @see setManualLoadSuccess(), beginLoading(), Status
			 */
			[[nodiscard]]
			bool
			enableManualLoading () noexcept
			{
				return this->initializeEnqueuing(true);
			}

			/**
			 * @brief Completes manual loading by setting the final status.
			 *
			 * This method transitions the resource from ManualEnqueuing to either Loaded or Failed.
			 * It triggers dependency checking and notification propagation to parent resources.
			 *
			 * @param status true to mark loading as successful, false to mark as failed
			 * @return true if status was successfully set, false if not in ManualEnqueuing state
			 * @pre Resource must be in ManualEnqueuing state (call enableManualLoading() first)
			 * @post Resource transitions to Loading, then eventually Loaded or Failed
			 * @note Only works in manual loading mode; fails in automatic loading mode
			 * @see enableManualLoading(), setLoadSuccess()
			 */
			bool setManualLoadSuccess (bool status) noexcept;

			/**
			 * @brief Returns the current loading status of the resource.
			 *
			 * @return The current Status enum value (Unloaded, Enqueuing, ManualEnqueuing, Loading, Loaded, or Failed)
			 * @note Thread-safe to call at any time
			 * @see Status, isLoaded(), isLoading(), isUnloaded()
			 */
			[[nodiscard]]
			Status
			status () const noexcept
			{
				return m_status;
			}

			/**
			 * @brief Sets a hint that this resource should be loaded synchronously.
			 *
			 * Direct loading mode indicates the resource should be loaded synchronously rather than
			 * asynchronously. This is a hint to the resource manager and doesn't change loading behavior directly.
			 *
			 * @note This sets the DirectLoading flag in FlagTrait
			 * @see isDirectLoading()
			 */
			void
			setDirectLoadingHint () noexcept
			{
				this->enableFlag(DirectLoading);
			}

			/**
			 * @brief Returns the human-readable class label for this resource type.
			 *
			 * This pure virtual method must be implemented by derived classes to return
			 * a string identifying the resource type (e.g., "Texture2D", "MeshResource", "SoundResource").
			 * Used extensively in logging and debugging output.
			 *
			 * @return A C-string constant identifying the resource class type
			 * @note Must be implemented by all concrete resource classes
			 */
			[[nodiscard]]
			virtual const char * classLabel () const noexcept = 0;

			/**
			 * @brief Loads a fully functional default resource with no external data.
			 *
			 * This method creates a default/fallback version of the resource (e.g., a white texture,
			 * default mesh, or silence audio clip). Used when no file or data is provided.
			 *
			 * @param serviceProvider A reference to the resource manager service provider for accessing other resources
			 * @return true if default resource was successfully created, false on failure
			 * @pre Resource should be in Unloaded state
			 * @post Resource will be in Loaded or Failed state
			 * @note Pure virtual - must be implemented by derived classes
			 * @see load(AbstractServiceProvider&, const std::filesystem::path&)
			 */
			virtual bool load (AbstractServiceProvider & serviceProvider) noexcept = 0;

			/**
			 * @brief Loads a resource from a disk file.
			 *
			 * The default implementation attempts to parse the file as JSON and delegates to
			 * load(AbstractServiceProvider&, const Json::Value&). Derived classes can override
			 * this for binary formats or custom file handling.
			 *
			 * @param serviceProvider A reference to the resource manager service provider for accessing dependencies
			 * @param filepath Path to the resource file on disk
			 * @return true if resource was successfully loaded from file, false on failure
			 * @pre Resource should be in Unloaded state
			 * @pre File must exist and be readable
			 * @post Resource will be in Loaded or Failed state
			 * @note Default implementation parses JSON; override for binary formats
			 * @see load(AbstractServiceProvider&, const Json::Value&)
			 */
			virtual bool load (AbstractServiceProvider & serviceProvider, const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Loads a resource from a JsonCPP object.
			 *
			 * This is the primary loading method for JSON-based resource definitions. The JSON object
			 * typically contains resource parameters and references to dependencies.
			 *
			 * @param serviceProvider A reference to the resource manager service provider for loading dependencies
			 * @param data A reference to a parsed JsonCPP object containing resource data
			 * @return true if resource was successfully loaded from JSON data, false on failure
			 * @pre Resource should be in Unloaded state
			 * @pre JSON data must be valid and contain required fields
			 * @post Resource will be in Loaded or Failed state
			 * @note Pure virtual - must be implemented by derived classes
			 * @see load(AbstractServiceProvider&, const std::filesystem::path&)
			 */
			virtual bool load (AbstractServiceProvider & serviceProvider, const Json::Value & data) noexcept = 0;

			/**
			 * @brief Returns the amount of memory occupied by this resource in bytes.
			 *
			 * Calculates the total RAM footprint of the resource, including the object itself and
			 * any dynamically allocated data (vertex buffers, pixel data, audio samples, etc.).
			 * The default implementation returns sizeof(*this), which only includes the object size.
			 *
			 * @return The total memory footprint in bytes
			 * @note Override in derived classes to account for dynamically allocated data
			 * @note Default implementation only returns the size of the resource object itself
			 */
			[[nodiscard]]
			virtual
			size_t
			memoryOccupied () const noexcept
			{
				return sizeof(*this);
			}

		protected:

			/**
			 * @brief Constructs an abstract resource with a name and initial flags.
			 *
			 * Protected constructor ensures resources can only be instantiated through derived classes.
			 * Initializes the resource in Unloaded state with no dependencies.
			 *
			 * @param resourceName Unique identifier for the resource (moved into NameableTrait)
			 * @param initialResourceFlags Initial FlagTrait flags controlling loading behavior
			 * @post Resource is in Unloaded state with empty dependency lists
			 * @note Resource names should be unique within their container
			 * @see NameableTrait, FlagTrait
			 */
			ResourceTrait (std::string resourceName, uint32_t initialResourceFlags) noexcept
				: NameableTrait{std::move(resourceName)},
				FlagTrait{initialResourceFlags}
			{

			}

			/**
			 * @brief Returns whether the resource is marked for direct (synchronous) loading.
			 *
			 * Direct loading is a hint to the resource manager that this resource should be loaded
			 * synchronously rather than asynchronously. This flag is checked by the manager during loading.
			 *
			 * @return true if DirectLoading flag is enabled, false otherwise
			 * @note This is a hint flag and doesn't directly affect ResourceTrait behavior
			 * @see setDirectLoadingHint()
			 */
			[[nodiscard]]
			bool
			isDirectLoading () const noexcept
			{
				return this->isFlagEnabled(DirectLoading);
			}

			/**
			 * @brief Initiates the automatic loading process for this resource.
			 *
			 * This method transitions the resource from Unloaded to Enqueuing state, allowing
			 * dependencies to be added. Call this at the start of load() implementations before
			 * adding dependencies with addDependency().
			 *
			 * @return true if loading was successfully initiated, false if already loading/loaded
			 * @pre Resource must be in Unloaded state
			 * @post Resource transitions to Enqueuing state if successful
			 * @warning If this returns false, abort the loading process immediately
			 * @note For manual loading, use enableManualLoading() instead
			 * @see setLoadSuccess(), addDependency(), enableManualLoading()
			 */
			[[nodiscard]]
			bool
			beginLoading () noexcept
			{
				/* NOTE: The manual enqueuing is disabled. */
				return this->initializeEnqueuing(false);
			}

			/**
			 * @brief Adds a dependency that must be loaded before this resource completes.
			 *
			 * Creates a parent-child relationship where this resource (parent) waits for the
			 * dependency (child) to complete loading. Dependencies are tracked in both directions
			 * for bidirectional notification. Thread-safe and can be called from multiple threads.
			 *
			 * @param dependency Shared pointer to the dependent resource (must not be null)
			 * @return true if dependency was added or already satisfied, false on error
			 * @pre Resource must be in Enqueuing or ManualEnqueuing state
			 * @pre dependency pointer must not be null
			 * @post Dependency added to m_dependenciesToWaitFor if not already loaded
			 * @post This resource added to dependency's m_parentsToNotify list
			 * @note Already-loaded dependencies are skipped (returns true immediately)
			 * @note Duplicate dependencies are detected and skipped
			 * @note Thread-safe: uses m_dependenciesAccess mutex
			 * @warning Circular dependencies are not detected and will cause deadlock
			 * @see beginLoading(), setLoadSuccess(), m_dependenciesToWaitFor
			 */
			[[nodiscard]]
			bool addDependency (const std::shared_ptr< ResourceTrait > & dependency) noexcept;

			/**
			 * @brief Completes the loading process by setting the final status.
			 *
			 * Call this at the end of load() implementations after all dependencies have been added.
			 * Transitions the resource from Enqueuing to Loading state, then checks dependencies.
			 * When all dependencies complete, triggers onDependenciesLoaded() and notifies observers.
			 *
			 * @param status true to indicate loading succeeded, false to indicate failure
			 * @return The same value as the status parameter
			 * @pre Resource must be in Enqueuing or ManualEnqueuing state
			 * @post On success: Resource transitions to Loading, then eventually Loaded
			 * @post On failure: Resource transitions to Failed and emits LoadFailed notification
			 * @note Thread-safe: uses m_dependenciesAccess mutex
			 * @see beginLoading(), addDependency(), checkDependencies(), onDependenciesLoaded()
			 */
			[[nodiscard]]
			bool setLoadSuccess (bool status) noexcept;

			/**
			 * @brief Callback invoked when all dependencies have completed loading.
			 *
			 * This virtual method is called automatically by checkDependencies() when all
			 * dependencies transition to the Loaded state. Override this to perform finalization
			 * tasks that require all dependencies to be available (e.g., combining textures,
			 * building GPU resources, validating complete resource graphs).
			 *
			 * @return true if finalization succeeded, false to mark resource as Failed
			 * @post Resource will transition to Loaded if this returns true, Failed otherwise
			 * @note Default implementation returns true (no finalization needed)
			 * @note This is called from within a mutex-protected section
			 * @note All dependencies are guaranteed to be in Loaded state when this is called
			 * @see checkDependencies(), setLoadSuccess()
			 */
			[[nodiscard]]
			virtual
			bool
			onDependenciesLoaded () noexcept
			{
				return true;
			}

			/**
			 * @brief Extracts a relative resource name from a full filesystem path.
			 *
			 * Removes the store directory prefix and file extension from a path to generate
			 * a canonical resource name. On Windows, converts backslashes to forward slashes
			 * to maintain cross-platform compatibility.
			 *
			 * Example: "APP-DATA/data-stores/Movies/sub_directory/res_name.ext" → "sub_directory/res_name"
			 *
			 * @param filepath Full filesystem path to the resource file
			 * @param storeName Name of the store directory to strip from the path
			 * @return Relative resource name with extension removed and UNIX-style separators
			 * @note On Windows, path separators are converted to forward slashes
			 * @see NameableTrait
			 */
			[[nodiscard]]
			static std::string getResourceNameFromFilepath (const std::filesystem::path & filepath, const std::string & storeName) noexcept;

		private:

			/**
			 * @brief Notification callback invoked by child dependencies when they complete loading.
			 *
			 * This method is called by child resources through their m_parentsToNotify list when they
			 * transition to the Loaded state. It removes the dependency from m_dependenciesToWaitFor
			 * and triggers a dependency check to see if all dependencies are now satisfied.
			 *
			 * @param dependency Shared pointer to the dependency that just finished loading
			 * @post dependency is removed from m_dependenciesToWaitFor
			 * @post checkDependencies() is called to evaluate overall loading status
			 * @note Thread-safe: uses m_dependenciesAccess mutex
			 * @see addDependency(), checkDependencies(), m_dependenciesToWaitFor
			 */
			void dependencyLoaded (const std::shared_ptr< ResourceTrait > & dependency) noexcept;

			/**
			 * @brief Checks if all dependencies are loaded and propagates notifications.
			 *
			 * This method evaluates the loading state of all dependencies. If all are loaded,
			 * it calls onDependenciesLoaded(), transitions to Loaded state, emits LoadFinished,
			 * and recursively notifies parent resources. This implements the core dependency
			 * resolution algorithm.
			 *
			 * @post If all dependencies loaded: Status becomes Loaded, parents notified, observers notified
			 * @post If any dependency pending: No state change, waits for more dependencyLoaded() calls
			 * @post If onDependenciesLoaded() fails: Status becomes Failed, LoadFailed notification emitted
			 * @note Thread-safe: uses m_dependenciesAccess mutex
			 * @note Only acts when status is Loading; ignores calls in other states
			 * @see dependencyLoaded(), onDependenciesLoaded(), setLoadSuccess()
			 */
			void checkDependencies () noexcept;

			/**
			 * @brief Internal method that initiates the enqueuing phase of resource loading.
			 *
			 * This method transitions the resource from Unloaded to either Enqueuing (automatic)
			 * or ManualEnqueuing (manual) state. Called by beginLoading() and enableManualLoading().
			 *
			 * @param manual true for ManualEnqueuing mode, false for Enqueuing mode
			 * @return true if state transition succeeded, false if already loading/loaded/failed
			 * @pre Resource must be in Unloaded state
			 * @post Resource transitions to Enqueuing or ManualEnqueuing state if successful
			 * @see beginLoading(), enableManualLoading()
			 */
			[[nodiscard]]
			bool initializeEnqueuing (bool manual) noexcept;

			/**
			 * @brief Checks if adding a dependency would create a circular dependency.
			 *
			 * This method performs a depth-first search through the dependency's existing
			 * dependencies to detect if 'this' resource appears anywhere in the chain.
			 * If found, adding the dependency would create a cycle that would cause a deadlock
			 * during loading.
			 *
			 * @param dependency The dependency to check for cycles.
			 * @return true if adding this dependency would create a cycle, false otherwise.
			 * @note This method must be called while holding m_dependenciesAccess mutex.
			 * @note The check is recursive and may be expensive for deep dependency trees.
			 * @see addDependency()
			 */
			[[nodiscard]]
			bool wouldCreateCycle (const std::shared_ptr< ResourceTrait > & dependency) const noexcept;

			static constexpr uint32_t DirectLoading = 1U << 31;

			std::vector< std::shared_ptr< ResourceTrait > > m_parentsToNotify;
			std::vector< std::shared_ptr< ResourceTrait > > m_dependenciesToWaitFor;
			std::atomic< Status > m_status{Status::Unloaded};
			mutable std::mutex m_dependenciesAccess;
	};
}
