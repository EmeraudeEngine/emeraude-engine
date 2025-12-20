/*
 * src/Resources/Container.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <any>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <type_traits>

/* Local inclusions for inheritances. */
#include "Libs/NameableTrait.hpp"
#include "Libs/ObservableTrait.hpp"
#include "Libs/ObserverTrait.hpp"

/* Local inclusions for usages. */
#include "Libs/IO/IO.hpp"
#include "Libs/Network/URL.hpp"
#include "Libs/String.hpp"
#include "Net/Manager.hpp"
#include "PrimaryServices.hpp"
#include "BaseInformation.hpp"
#include "ResourceTrait.hpp"
#include "Types.hpp"

namespace EmEn::Resources
{
	/**
	 * @class ContainerInterface
	 * @brief Abstract base interface for all resource containers in the Emeraude Engine.
	 *
	 * This interface defines the common contract for resource management containers across
	 * different resource types. It provides methods for initialization, termination, memory
	 * tracking, and resource cleanup. All resource containers must inherit from this interface
	 * and implement its pure virtual methods.
	 *
	 * The interface combines NameableTrait for human-readable identification and ObservableTrait
	 * for event-based notification patterns, allowing observers to monitor resource lifecycle events.
	 *
	 * @see Container Template implementation of this interface
	 * @see EmEn::Libs::NameableTrait Base class providing naming functionality
	 * @see EmEn::Libs::ObservableTrait Base class providing observer pattern functionality
	 * @version 0.8.35
	 */
	class ContainerInterface : public Libs::NameableTrait, public Libs::ObservableTrait
	{
		public:

			/**
			 * @brief Destructs the container interface.
			 *
			 * Virtual destructor ensures proper cleanup of derived container implementations.
			 *
			 * @version 0.8.35
			 */
			~ContainerInterface () override = default;

			/**
			 * @brief Sets the verbosity state for the container.
			 *
			 * When enabled, the container will output detailed trace information about resource
			 * loading, unloading, and lifecycle events. Useful for debugging resource management issues.
			 *
			 * @param state True to enable verbose logging, false to disable.
			 * @version 0.8.35
			 */
			virtual void setVerbosity (bool state) noexcept = 0;

			/**
			 * @brief Initializes the container and prepares it for resource management.
			 *
			 * This method is called during the engine startup sequence to set up the container's
			 * internal state, load the resource store, and prepare for resource loading operations.
			 * Must be called before any resource operations.
			 *
			 * @return True if initialization succeeded, false otherwise.
			 * @version 0.8.35
			 */
			virtual bool initialize () noexcept = 0;

			/**
			 * @brief Terminates the container and releases all managed resources.
			 *
			 * This method is called during engine shutdown to cleanly release all loaded resources,
			 * free memory, and reset the container state. After termination, the container should not
			 * be used until re-initialized.
			 *
			 * @return True if termination succeeded, false otherwise.
			 * @version 0.8.35
			 */
			virtual bool terminate () noexcept = 0;

			/**
			 * @brief Returns the total memory consumed by all loaded resources.
			 *
			 * Calculates the sum of memory occupied by all resources currently loaded in the container,
			 * regardless of whether they are actively being used. This includes GPU memory for graphics
			 * resources, audio buffers, and other resource-specific allocations.
			 *
			 * @return Total memory occupied in bytes.
			 * @see unusedMemoryOccupied() For memory used by unused resources only
			 * @version 0.8.35
			 */
			[[nodiscard]]
			virtual size_t memoryOccupied () const noexcept = 0;

			/**
			 * @brief Returns the total memory consumed by loaded but unused resources.
			 *
			 * Calculates memory occupied by resources that are loaded but not currently referenced
			 * by any external code (use_count == 1, only held by the container). These resources
			 * are candidates for unloading to free memory.
			 *
			 * @return Total unused memory in bytes.
			 * @see unloadUnusedResources() To free this memory
			 * @see memoryOccupied() For total memory used by all resources
			 * @version 0.8.35
			 */
			[[nodiscard]]
			virtual size_t unusedMemoryOccupied () const noexcept = 0;

			/**
			 * @brief Unloads all unused resources to free memory.
			 *
			 * Iterates through all loaded resources and removes those that are no longer referenced
			 * by external code (use_count == 1). This is typically called during memory pressure
			 * situations or at strategic points in the application lifecycle.
			 *
			 * @return Number of resources that were unloaded.
			 * @see unusedMemoryOccupied() To check memory that would be freed
			 * @version 0.8.35
			 */
			virtual size_t unloadUnusedResources () noexcept = 0;

			/**
			 * @brief Returns the dependency complexity level of the resource type.
			 *
			 * The complexity level indicates how complex the resource's dependency graph is,
			 * which affects loading order and priority in the resource management system.
			 * Resources with higher complexity are typically loaded after their dependencies.
			 *
			 * @return The DepComplexity level for this resource type.
			 * @see DepComplexity Enum defining complexity levels
			 * @version 0.8.35
			 */
			[[nodiscard]]
			virtual DepComplexity complexity () const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a container interface with the given name.
			 *
			 * Protected constructor ensures only derived classes can instantiate the interface.
			 * The name is typically the resource type identifier (e.g., "Texture2D", "MeshResource").
			 *
			 * @param name Human-readable identifier for this container type.
			 * @version 0.8.35
			 */
			explicit
			ContainerInterface (const std::string & name) noexcept
				: NameableTrait{name}
			{

			}
	};

	/**
	 * @class LoadingRequest
	 * @brief Encapsulates a resource loading request with download state management.
	 *
	 * LoadingRequest handles the complete lifecycle of a resource loading operation, including
	 * local file access, external URL downloads, and direct data loading. It manages download
	 * tickets for asynchronous network operations and tracks the loading state through a
	 * finite state machine.
	 *
	 * **Download Ticket States:**
	 * - DownloadNotRequested (-4): No download needed (local or direct data)
	 * - DownloadError (-3): Download failed
	 * - DownloadSuccess (-2): Download completed successfully
	 * - DownloadPending (-1): Waiting to be submitted to download manager
	 * - Positive values: Active download ticket from the network manager
	 *
	 * **Source Types:**
	 * - LocalData: Load from filesystem path
	 * - ExternalData: Download from URL, cache locally, then load
	 * - DirectData: Load from in-memory JSON data
	 *
	 * The request automatically determines the cache filepath for external resources and handles
	 * the conversion from external URLs to cached local files after successful downloads.
	 *
	 * @tparam resource_t Resource type, must derive from ResourceTrait.
	 * @see Container For the resource container that uses this request type
	 * @see BaseInformation For resource metadata
	 * @see SourceType Enum defining resource data sources
	 * @version 0.8.35
	 */
	template< typename resource_t >
	requires (std::is_base_of_v< ResourceTrait, resource_t >)
	class LoadingRequest final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"LoadingRequest"};

			/**
			 * @brief Constructs a loading request with resource metadata.
			 *
			 * Initializes the loading request and sets the appropriate download ticket state based
			 * on the source type. For external data sources, validates the URL and sets the ticket
			 * to DownloadPending if valid, or DownloadError if invalid.
			 *
			 * @param baseInformation Resource metadata including source type and data location (moved).
			 * @param resource Shared pointer to the target resource object that will be populated.
			 * @version 0.8.35
			 */
			LoadingRequest (BaseInformation baseInformation, const std::shared_ptr< resource_t > & resource) noexcept
				: m_baseInformation{std::move(baseInformation)},
				m_resource{resource}
			{
				using namespace Libs;

				switch ( m_baseInformation.sourceType() )
				{
					case SourceType::Undefined :
						Tracer::error(ClassId, "Undefined type for resource request !");
						break;

					case SourceType::LocalData :
						break;

					case SourceType::ExternalData :
					{
						Network::URL resourceUrl{m_baseInformation.data().asString()};

						if ( resourceUrl.isValid() )
						{
							m_downloadTicket = DownloadPending;
						}
						else
						{
							TraceError{ClassId} << "'" << resourceUrl << "' is not a valid URL ! Download cancelled ...";

							m_downloadTicket = DownloadError;
						}
					}
						break;

					case SourceType::DirectData :
						break;
				}
			}

			/**
			 * @brief Returns the cache file path for downloaded external resources.
			 *
			 * Constructs the filesystem path where downloaded external resources are cached locally.
			 * The path structure is: `[cache_dir]/data/[resource_type]/[filename]`
			 *
			 * Example: `~/.cache/emeraude/data/Texture2D/albedo.png`
			 *
			 * @param fileSystem Reference to the filesystem service for cache directory location.
			 * @return Full filesystem path to the cached resource file.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::filesystem::path
			cacheFilepath (const FileSystem & fileSystem) const noexcept
			{
				std::filesystem::path filepath{fileSystem.cacheDirectory()};
				filepath.append("data");
				filepath.append(resource_t::ClassId);
				filepath.append(Libs::String::extractFilename(m_baseInformation.data().asString()));

				return filepath;
			}

			/**
			 * @brief Returns the base information metadata for this request.
			 *
			 * @return Const reference to the resource's base information (name, source type, data location).
			 * @version 0.8.35
			 */
			[[nodiscard]]
			const BaseInformation &
			baseInformation () const noexcept
			{
				return m_baseInformation;
			}

			/**
			 * @brief Returns the target resource object for this loading request.
			 *
			 * @return Shared pointer to the resource that will be populated when loading completes.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			resource () const noexcept
			{
				return m_resource;
			}

			/**
			 * @brief Returns the download manager ticket number.
			 *
			 * Returns the ticket assigned by the network download manager for tracking this download.
			 * A return value of 0 indicates no active download (either not needed or already completed).
			 *
			 * @return Download ticket number, or 0 if no active download.
			 * @see isDownloadable() To check if download is pending
			 * @version 0.8.35
			 */
			[[nodiscard]]
			int
			downloadTicket () const noexcept
			{
				if ( m_downloadTicket < 0 )
				{
					return 0;
				}

				return m_downloadTicket;
			}

			/**
			 * @brief Checks if the request is ready to be submitted for download.
			 *
			 * Returns true only if this is an external data request currently in the DownloadPending
			 * state. Requests in this state are waiting to be submitted to the network download manager.
			 *
			 * @return True if the request can be submitted for download, false otherwise.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isDownloadable () const noexcept
			{
				if ( m_baseInformation.sourceType() != SourceType::ExternalData ) [[unlikely]]
				{
					Tracer::error(ClassId, "This request is not external !");

					return false;
				}

				return m_downloadTicket == DownloadPending;
			}

			/**
			 * @brief Returns the download URL for external data requests.
			 *
			 * Extracts and returns the URL from the base information data field. Returns an
			 * empty URL if this is not an external data request.
			 *
			 * @return URL object for the resource download, or empty URL for non-external requests.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			Libs::Network::URL
			url () const noexcept
			{
				if ( m_baseInformation.sourceType() != SourceType::ExternalData )
				{
					return {};
				}

				return Libs::Network::URL{m_baseInformation.data().asString()};
			}

			/**
			 * @brief Checks if the resource download is currently in progress.
			 *
			 * Returns true if this is an external data request with a positive download ticket,
			 * indicating the download has been submitted to the network manager but not yet completed.
			 *
			 * @return True if download is active, false otherwise.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isDownloading () const noexcept
			{
				if ( m_baseInformation.sourceType() != SourceType::ExternalData ) [[unlikely]]
				{
					Tracer::error(ClassId, "This request is not external !");

					return false;
				}

				/* NOTE: Check the networkManager ticket.
				 * If it's still present, the download
				 * is not yet finished. */
				if ( m_downloadTicket >= 0 )
				{
					return false;
				}

				return true;
			}

			/**
			 * @brief Assigns a download manager ticket to this request.
			 *
			 * Updates the request's download ticket after successfully submitting it to the network
			 * download manager. This transitions the state from DownloadPending to actively downloading.
			 *
			 * @param ticket Positive ticket number assigned by the network download manager.
			 * @pre Request must be in DownloadPending state (ticket == -1).
			 * @pre Request must be of SourceType::ExternalData.
			 * @warning Calling with invalid preconditions generates error traces.
			 * @version 0.8.35
			 */
			void
			setDownloadTicket (int ticket) noexcept
			{
				if ( m_baseInformation.sourceType() != SourceType::ExternalData ) [[unlikely]]
				{
					Tracer::error(ClassId, "This request is not external !");

					return;
				}

				if ( m_downloadTicket != DownloadPending ) [[unlikely]]
				{
					Tracer::error(ClassId, "Cannot set a ticket to a request which is not in 'DownloadPending' status !");

					return;
				}

				m_downloadTicket = ticket;
			}

			/**
			 * @brief Marks the download as completed (successfully or with error).
			 *
			 * Updates the request state after download completion. On success, updates the base
			 * information to point to the cached local file instead of the original URL. On failure,
			 * sets the ticket to DownloadError state.
			 *
			 * @param fileSystem Reference to filesystem service for cache path resolution.
			 * @param success True if download succeeded, false if it failed.
			 * @post On success: ticket becomes DownloadSuccess, baseInformation updated to cache path.
			 * @post On failure: ticket becomes DownloadError.
			 * @version 0.8.35
			 */
			void
			setDownloadProcessed (const FileSystem & fileSystem, bool success) noexcept
			{
				if ( m_baseInformation.sourceType() != SourceType::ExternalData ) [[unlikely]]
				{
					Tracer::error(ClassId, "This request is not external !");

					return;
				}

				/* Invalidate the networkManager ticket. */
				if ( success ) [[likely]]
				{
					m_downloadTicket = DownloadSuccess;

					m_baseInformation.updateFromDownload(this->cacheFilepath(fileSystem));
				}
				else
				{
					m_downloadTicket = DownloadError;
				}
			}

		private:

			/* Special ticket flags. */
			static constexpr auto DownloadNotRequested{-4};
			static constexpr auto DownloadError{-3};
			static constexpr auto DownloadSuccess{-2};
			static constexpr auto DownloadPending{-1};

			BaseInformation m_baseInformation;
			std::shared_ptr< resource_t > m_resource;
			int m_downloadTicket{DownloadNotRequested};
	};

	/**
	 * @class Container
	 * @brief Thread-safe template container for managing resource lifecycle with async/sync loading.
	 *
	 * Container is the core resource management system in Emeraude Engine, providing:
	 *
	 * **Thread Safety:**
	 * - All public methods are thread-safe via internal mutex (m_resourcesAccess)
	 * - Supports concurrent access from multiple threads
	 * - Uses RAII lock guards for exception safety
	 *
	 * **Loading Modes:**
	 * - Asynchronous loading via thread pool (default)
	 * - Synchronous loading on calling thread (asyncLoad=false)
	 * - Manual loading with custom creation functions
	 * - Automatic download of external resources with local caching
	 *
	 * **Resource Lifecycle:**
	 * 1. **Creation**: Empty resource allocated in memory
	 * 2. **Enqueuing**: Request submitted to loading queue
	 * 3. **Loading**: Data loaded from source (file/network/memory)
	 * 4. **Ready**: Resource fully loaded and usable
	 * 5. **Unloading**: Resource freed when no longer referenced
	 *
	 * **Observable Pattern:**
	 * Emits NotificationCode events for monitoring:
	 * - LoadingProcessStarted: Before a resource begins loading
	 * - ResourceLoaded: When a resource successfully loads
	 * - LoadingProcessFinished: After loading completes (success or failure)
	 * - Progress: Loading progress updates (if supported by resource type)
	 *
	 * **Default Resource:**
	 * Each container maintains a "Default" resource as a fallback when requested resources
	 * cannot be found or loaded. This ensures robust error handling without null pointers.
	 *
	 * **Manual Resources ('+' prefix):**
	 * Resources with names starting with '+' are "manual" and won't be overridden by store
	 * entries. Use this for runtime-generated or procedural resources.
	 *
	 * **Two-Phase Erasure:**
	 * The unloadUnusedResources() method uses a two-phase pattern to avoid iterator invalidation
	 * issues during erase operations, ensuring stable behavior even with complex resource dependencies.
	 *
	 * @tparam resource_t Resource type must derive from ResourceTrait.
	 * @see ContainerInterface Base interface this template implements
	 * @see LoadingRequest Request object for tracking loading operations
	 * @see ResourceTrait Required base class for all manageable resources
	 * @note [OBS] This class is observable via ObservableTrait
	 * @note [OBSERVER] This class observes the network manager for download notifications
	 * @version 0.8.35
	 */
	template< typename resource_t >
	class Container final : public ContainerInterface, public Libs::ObserverTrait
	{
		public:

			/**
			 * @enum NotificationCode
			 * @brief Observable event codes for resource lifecycle notifications.
			 *
			 * These codes are emitted through the ObservableTrait notify() mechanism, allowing
			 * observers to monitor resource loading progress and state changes.
			 *
			 * @version 0.8.35
			 */
			enum NotificationCode
			{
				Unknown,                   ///< Unknown or unspecified notification.
				LoadingProcessStarted,     ///< Emitted when a resource begins loading.
				ResourceLoaded,            ///< Emitted when a resource successfully loads (data: resource_t*).
				LoadingProcessFinished,    ///< Emitted when loading completes (success or failure).
				Progress,                  ///< Emitted for loading progress updates (if supported).
				/* Enumeration boundary. */
				MaxEnum                    ///< Enumeration boundary marker.
			};

			/**
			 * @brief Constructs a resource container for the specified resource type.
			 *
			 * Initializes the container with necessary engine services and the resource store.
			 * Automatically registers as an observer of the network manager to handle download
			 * notifications for external resources.
			 *
			 * @param serviceName Human-readable identifier for this container (e.g., "Texture2D").
			 * @param primaryServices Reference to core engine services (threading, networking, filesystem).
			 * @param serviceProvider Reference to the resource service provider for loading operations.
			 * @param store Shared pointer to the resource metadata store (name -> BaseInformation map).
			 *              Can be nullptr for containers that don't use a predefined store.
			 * @note [OBS] Container becomes observable and can emit NotificationCode events.
			 * @note [OBSERVER] Container observes the network manager for download completion.
			 * @version 0.8.35
			 */
			Container (const char * serviceName, PrimaryServices & primaryServices, AbstractServiceProvider & serviceProvider, const std::shared_ptr< std::unordered_map< std::string, BaseInformation > > & store) noexcept
				: ContainerInterface{serviceName},
				m_primaryServices{primaryServices},
				m_serviceProvider{serviceProvider},
				m_localStore{store}
			{
				this->observe(&m_primaryServices.netManager());
			}

			/**
			 * @brief Returns the unique class identifier for this container type.
			 *
			 * Computes a compile-time hash of the resource type's ClassId string using the FNV-1a
			 * algorithm. This provides a fast, type-safe way to identify container types at runtime.
			 *
			 * @return Unique identifier hash for this container specialization.
			 * @note Thread-safe: uses static local variable initialization.
			 * @version 0.8.35
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Libs::Hash::FNV1a(resource_t::ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Resources::ContainerInterface::setVerbosity(bool) noexcept */
			void
			setVerbosity (bool state) noexcept override
			{
				m_verboseEnabled = state;
			}

			/** @copydoc EmEn::Resources::ContainerInterface::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				size_t bytes = 0;

				for ( const auto & resource : m_resources | std::views::values )
				{
					bytes += resource->memoryOccupied();
				}

				return bytes;
			}

			/** @copydoc EmEn::Resources::ContainerInterface::unusedMemoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			unusedMemoryOccupied () const noexcept override
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				size_t bytes = 0;

				for ( const auto & resource : m_resources | std::views::values )
				{
					if ( resource.use_count() == 1 )
					{
						bytes += resource->memoryOccupied();
					}
				}

				return bytes;
			}

			/** @copydoc EmEn::Resources::ContainerInterface::unloadUnusedResources() noexcept */
			size_t
			unloadUnusedResources () noexcept override
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				if ( m_resources.empty() )
				{
					return 0;
				}

				/* NOTE: Two-phase erasure pattern to avoid iterator invalidation issues.
				 * This fixes problems with animated 2D textures where use_count() could change
				 * between the check and the erase operation. */

				/* Phase 1: Log debug info for resources still in use. */
				for ( const auto & [name, resource] : m_resources )
				{
					if ( const auto links = resource.use_count(); links > 1 )
					{
						TraceDebug{resource_t::ClassId} << resource->name() << " is still used " << links << " times !";
					}
				}

				/* Phase 2: Erase unused resources using std::erase_if. */
				const auto unloadedResources = std::erase_if(m_resources, [](const auto & pair) {
					return pair.second.use_count() == 1;
				});

				if ( unloadedResources > 0 )
				{
					TraceInfo{resource_t::ClassId} << unloadedResources << " resource(s) unloaded !";
				}

				return unloadedResources;
			}

			/** @copydoc EmEn::Resources::ContainerInterface::unloadUnusedResources() noexcept */
			DepComplexity
			complexity () const noexcept override
			{
				return resource_t::Complexity;
			}

			/**
			 * @brief Checks if a resource is currently loaded in memory.
			 *
			 * Queries only the loaded resource map, not the store. A resource may exist in the
			 * store but not be loaded yet.
			 *
			 * @param resourceName Name of the resource to check.
			 * @return True if the resource is loaded and ready, false otherwise.
			 * @see isResourceExists() To check both loaded and unloaded resources
			 * @note Thread-safe: locks m_resourcesAccess internally.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isResourceLoaded (const std::string & resourceName) const noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				return m_resources.contains(resourceName);
			}

			/**
			 * @brief Checks if a resource exists either loaded or in the store.
			 *
			 * Performs a two-stage check:
			 * 1. Checks loaded resources (fast lookup)
			 * 2. Checks unloaded resources in the store
			 *
			 * @param resourceName Name of the resource to check.
			 * @return True if resource exists (loaded or available for loading), false otherwise.
			 * @see isResourceLoaded() To check only loaded resources
			 * @note Thread-safe: locks m_resourcesAccess internally.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			bool
			isResourceExists (const std::string & resourceName) const noexcept
			{
				if ( m_localStore == nullptr )
				{
					return false;
				}

				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				/* First, check in live resources.
				 * NOTE: Do not use Container::isResourceLoaded() to prevent from mutex deadlock. */
				if ( m_resources.contains(resourceName) )
				{
					return true;
				}

				return m_localStore->contains(resourceName);
			}

			/**
			 * @brief Returns all available resource names from the store.
			 *
			 * Extracts all resource names from the metadata store. This includes both loaded
			 * and unloaded resources. Returns an empty vector if no store is configured.
			 *
			 * @return Vector of resource names available in the store.
			 * @note Does not include runtime-created resources not in the store.
			 * @version 0.8.35
			 */
			std::vector< std::string >
			getResourceNames () const noexcept
			{
				if ( m_localStore == nullptr )
				{
					return {};
				}

				std::vector< std::string > names;
				names.reserve(m_localStore->size());

				std::ranges::copy(*m_localStore | std::views::keys, std::back_inserter(names));

				return names;
			}

			/**
			 * @brief Returns resource names from the store that start with a given prefix.
			 *
			 * Filters all resource names from the metadata store, returning only those that
			 * begin with the specified prefix string. Useful for retrieving categorized resources
			 * (e.g., "UI/", "Sound/Music/", "Texture/Terrain/").
			 *
			 * @param prefix The prefix string to filter by (e.g., "UI/" or "Enemy_").
			 * @return Vector of resource names that start with the given prefix.
			 * @note Returns empty vector if store is null or no matches found.
			 * @note The comparison is case-sensitive.
			 * @see getResourceNames() To get all resource names without filtering
			 * @version 0.8.40
			 */
			[[nodiscard]]
			std::vector< std::string >
			getResourceNames (const std::string & prefix) const noexcept
			{
				if ( m_localStore == nullptr || prefix.empty() )
				{
					return {};
				}

				std::vector< std::string > names;

				for ( const auto & key : *m_localStore | std::views::keys )
				{
					if ( key.starts_with(prefix) )
					{
						names.push_back(key);
					}
				}

				return names;
			}

			/**
			 * @brief Creates a new empty resource for manual population.
			 *
			 * Allocates a new resource object in the Unloaded state. The caller is responsible
			 * for populating it via the resource's API and calling appropriate loading methods.
			 *
			 * **Manual Resource Convention:**
			 * Prefix the resource name with '+' to mark it as manual and prevent conflicts with
			 * store resources. Example: "+ProceduralTexture"
			 *
			 * @param resourceName Unique name for the new resource. Use '+' prefix for manual resources.
			 * @param resourceFlags Resource-specific construction flags (default: 0 = no flags).
			 * @return Shared pointer to the newly created resource, or nullptr on failure.
			 * @warning Returns nullptr if a resource with this name already exists.
			 * @see addResource() To add an already-constructed resource
			 * @see getOrCreateUnloadedResource() To get existing or create new unloaded
			 * @note Thread-safe: locks m_resourcesAccess internally.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			createResource (const std::string & resourceName, uint32_t resourceFlags = 0) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				return this->createResourceUnlocked(resourceName, resourceFlags);
			}

			/**
			 * @brief Adds an externally-constructed resource to the container.
			 *
			 * Registers a pre-constructed resource object with the container. Useful for resources
			 * created outside the normal loading pipeline (procedural generation, runtime compilation, etc.).
			 *
			 * **Manual Resource Convention:**
			 * Use '+' prefix in resource names to avoid conflicts with store resources.
			 *
			 * @param resource Shared pointer to the fully-constructed resource to add.
			 * @return True if resource was successfully added, false if name already exists.
			 * @warning Returns false and logs error if resource name conflicts with existing resource.
			 * @see createResource() To create an empty resource
			 * @note Thread-safe: locks m_resourcesAccess internally.
			 * @version 0.8.35
			 */
			bool
			addResource (const std::shared_ptr< resource_t > & resource) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				if ( m_resources.contains(resource->name()) ) [[unlikely]]
				{
					TraceError{resource_t::ClassId} << "A resource name '" << resource->name() << "' is already present in the store !";

					return false;
				}

				m_resources.emplace(resource->name(), resource);

				return true;
			}

			/**
			 * @brief Preloads a resource without returning it immediately.
			 *
			 * Triggers resource loading without blocking or returning a shared pointer. Useful for
			 * preloading resources during loading screens or initialization phases. The resource
			 * will be available in cache when actually requested later.
			 *
			 * @param resourceName Name of the resource to preload from the store.
			 * @param asyncLoad True to load asynchronously in thread pool (default), false for synchronous.
			 * @return True if preload was successfully initiated, false if resource not found in store.
			 * @see preloadResources() To preload multiple resources
			 * @see getResource() To retrieve the preloaded resource later
			 * @note Returns true immediately for already-loaded resources (no-op).
			 * @note Thread-safe: locks m_resourcesAccess internally.
			 * @version 0.8.35
			 */
			bool
			preloadResource (const std::string & resourceName, bool asyncLoad = true)
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				/* NOTE: Do not use Container::isResourceLoaded() to prevent from mutex deadlock. */
				if ( m_resources.contains(resourceName) )
				{
					return true;
				}

				/* If not already loaded, check in store for loading. */
				if ( m_localStore == nullptr )
				{
					return false;
				}

				const auto & resourceIt = m_localStore->find(resourceName);

				if ( resourceIt == m_localStore->cend() )
				{
					return false;
				}

				return this->pushInLoadingQueue(resourceIt->second, asyncLoad) != nullptr;
			}

			/**
			 * @brief Preloads multiple resources in batch.
			 *
			 * Convenience method for preloading a list of resources, typically used during
			 * level loading or scene initialization.
			 *
			 * @param resourceNames Vector of resource names to preload.
			 * @return Number of resources that failed to preload (0 = all succeeded).
			 * @see preloadResource() For single resource preloading
			 * @version 0.8.35
			 */
			uint32_t
			preloadResources (const std::vector< std::string > & resourceNames) noexcept
			{
				uint32_t errors = 0;

				for ( const auto & resourceName : resourceNames )
				{
					if ( !this->preloadResource(resourceName) )
					{
						++errors;
					}
				}

				return errors;
			}

			/**
			 * @brief Returns the default fallback resource for this type.
			 *
			 * The default resource is automatically created if it doesn't exist and serves as a
			 * safe fallback when requested resources cannot be found or loaded. This ensures the
			 * engine can continue running without null pointer crashes.
			 *
			 * For example, Texture2D's default is typically a 1x1 magenta texture.
			 *
			 * @return Shared pointer to the default resource (never null).
			 * @note Thread-safe: locks m_resourcesAccess internally.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getDefaultResource () noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				return this->getDefaultResourceUnlocked();
			}

			/**
			 * @brief Returns a resource by name, loading it if necessary.
			 *
			 * This is the primary method for accessing resources. It handles the complete resource
			 * lifecycle transparently:
			 *
			 * **Loading Behavior:**
			 * - If already loaded: Returns immediately
			 * - If in store: Loads asynchronously (asyncLoad=true) or synchronously (asyncLoad=false)
			 * - If not found: Returns default resource and logs warning
			 *
			 * **Special Names:**
			 * - "Default": Returns the default resource directly
			 *
			 * **External Resources:**
			 * For external URL resources, automatically:
			 * 1. Checks local cache first
			 * 2. Downloads if not cached
			 * 3. Loads from cache after download
			 *
			 * @param resourceName Name of the resource to retrieve.
			 * @param asyncLoad True for asynchronous loading (default), false for synchronous.
			 * @return Shared pointer to the requested resource, or default resource if not found.
			 * @warning Returns default resource (not nullptr) if resource doesn't exist.
			 * @see getDefaultResource() For accessing the default resource
			 * @see createResource() To create new resources not in store
			 * @note Thread-safe: locks m_resourcesAccess internally.
			 * @note Asynchronous loading returns immediately; check resource status() to monitor progress.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getResource (const std::string & resourceName, bool asyncLoad = true) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				if ( resourceName == Default )
				{
					return this->getDefaultResourceUnlocked();
				}

				/* Checks in loaded resources. */
				{
					const auto & loadedIt = m_resources.find(resourceName);

					if ( loadedIt != m_resources.cend() )
					{
						return loadedIt->second;
					}
				}

				/* If not already loaded, check in store for loading. */
				if ( m_localStore == nullptr )
				{
					TraceWarning{resource_t::ClassId} <<
						"The store is empty, unable to get '" << resourceName << "' ! "
						"Use Resource::create() function instead.";

					return this->getDefaultResourceUnlocked();
				}

				const auto & resourceIt = m_localStore->find(resourceName);

				if ( resourceIt == m_localStore->cend() )
				{
					/* The resource is definitively not present. */
					TraceWarning{resource_t::ClassId} <<
						"The resource named '" << resourceName << "' doesn't exist ! "
						"Use Resource::create() function instead.";

					return this->getDefaultResourceUnlocked();
				}

				/* Returns the smart pointer to the future loaded resource. */
				return this->pushInLoadingQueue(resourceIt->second, asyncLoad);
			}

			/**
			 * @brief Returns an existing resource or creates a new unloaded one.
			 *
			 * Combines get and create operations: tries to load from store first, creates new if not found.
			 * Useful when you want a resource whether it exists or not.
			 *
			 * @param resourceName Name of the resource.
			 * @param resourceFlags Construction flags if creating new resource (default: 0).
			 * @param asyncLoad True for async loading if resource is in store (default: true).
			 * @return Shared pointer to existing or newly created unloaded resource.
			 * @see getResource() To only retrieve existing resources
			 * @see createResource() To only create new resources
			 * @see getOrCreateResource() To get existing or create AND initialize via custom function
			 * @note Thread-safe: locks m_resourcesAccess internally.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getOrCreateUnloadedResource (const std::string & resourceName, uint32_t resourceFlags = 0, bool asyncLoad = true) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				const auto alreadyLoadedResource = this->checkLoadedResource(resourceName, asyncLoad);

				if ( alreadyLoadedResource != nullptr )
				{
					return alreadyLoadedResource;
				}

				return this->createResourceUnlocked(resourceName, resourceFlags);
			}

			/**
			 * @brief Returns an existing resource or creates one via custom function (synchronous).
			 *
			 * If the resource doesn't exist, creates it and invokes the provided function to populate
			 * it. The function executes synchronously on the calling thread and must fully initialize
			 * the resource before returning.
			 *
			 * **Creation Function Requirements:**
			 * - Must call resource.enableManualLoading() before custom loading logic
			 * - Must end with resource.setManualLoadSuccess() or resource.load()
			 * - Must return true on success, false on failure
			 *
			 * @param resourceName Name of the resource.
			 * @param createFunction Lambda/function to populate the new resource: `bool(resource_t&)`
			 * @param resourceFlags Construction flags for new resource (default: 0).
			 * @return Shared pointer to resource on success, default resource on failure.
			 * @warning Returns default resource if creation function fails or is improperly implemented.
			 * @see getOrCreateResourceAsync() For asynchronous creation
			 * @note Thread-safe: locks m_resourcesAccess internally.
			 * @note Function executes synchronously and blocks until complete.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getOrCreateResource (const std::string & resourceName, const std::function< bool (resource_t & resource) > & createFunction, uint32_t resourceFlags = 0) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				const auto alreadyLoadedResource = this->checkLoadedResource(resourceName, false);

				if ( alreadyLoadedResource != nullptr )
				{
					return alreadyLoadedResource;
				}

				/* Creates a new resource. */
				const auto newResource = this->createResourceUnlocked(resourceName, resourceFlags);

				if ( newResource == nullptr )
				{
					return this->getDefaultResourceUnlocked();
				}

				if ( !createFunction(*newResource) )
				{
					TraceError{resource_t::ClassId} << "The manual loading function for resource '" << resourceName << "' has returned an error !";

					return this->getDefaultResourceUnlocked();
				}

				switch ( newResource->status() )
				{
					case Status::Unloaded :
					case Status::Enqueuing :
					case Status::ManualEnqueuing :
						TraceError{resource_t::ClassId} <<
							"The manual resource '" << resourceName << "' is still in creation mode!"
							"A manual loading should ends with a call to ResourceTrait::setManualLoadSuccess() or ResourceTrait::load().";

						return this->getDefaultResourceUnlocked();

					case Status::Failed :
						TraceError{resource_t::ClassId} << "The manual resource '" << resourceName << "' has failed to load!";

						return this->getDefaultResourceUnlocked();

					default :
						return newResource;
				}
			}

			/**
			 * @brief Returns an existing resource or creates one via custom function (asynchronous).
			 *
			 * Like getOrCreateResource(), but executes the creation function asynchronously in the
			 * thread pool. Returns immediately with a resource in Enqueuing/ManualEnqueuing state.
			 * Check resource.status() to monitor completion.
			 *
			 * **Creation Function Requirements:**
			 * - Same as getOrCreateResource()
			 * - Must be thread-safe (executes on worker thread)
			 *
			 * @param resourceName Name of the resource.
			 * @param createFunction Lambda/function to populate the new resource: `bool(resource_t&)`
			 * @param resourceFlags Construction flags for new resource (default: 0).
			 * @return Shared pointer to resource (may still be loading), or nullptr on failure.
			 * @warning Returns nullptr if manual loading mode cannot be enabled (name conflict).
			 * @see getOrCreateResource() For synchronous creation
			 * @note Thread-safe: locks m_resourcesAccess internally.
			 * @note Returns immediately; loading occurs asynchronously in thread pool.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getOrCreateResourceAsync (const std::string & resourceName, const std::function< bool (resource_t & resource) > & createFunction, uint32_t resourceFlags = 0) noexcept
			{
				const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

				const auto alreadyLoadedResource = this->checkLoadedResource(resourceName, true);

				if ( alreadyLoadedResource != nullptr )
				{
					return alreadyLoadedResource;
				}

				/* Creates a new resource. */
				const auto newResource = this->createResourceUnlocked(resourceName, resourceFlags);

				if ( newResource == nullptr )
				{
					return this->getDefaultResourceUnlocked();
				}

				m_primaryServices.threadPool()->enqueue([createFunction, newResource] {
					if ( createFunction(*newResource) ) {
						switch ( newResource->status() )
						{
							case Status::Unloaded :
							case Status::Enqueuing :
							case Status::ManualEnqueuing :
								TraceError{resource_t::ClassId} <<
									"The manual resource '" << newResource->name() << " is still in creation mode !"
									"A manual loading should ends with a call to ResourceTrait::setManualLoadSuccess() or ResourceTrait::load().";
								break;

							default :
								break;
						}
					}
					else
					{
						TraceError{resource_t::ClassId} << "The manual loading function has return an error !";
					}
				});

				return newResource;
			}

			/**
			 * @brief Returns a randomly selected resource from the store.
			 *
			 * Selects a random resource from the available store entries using fast random number
			 * generation. Useful for randomized content systems, testing, or procedural generation.
			 *
			 * **Performance Note:**
			 * Uses O(n) iteration through the unordered_map to access random element. For frequent
			 * random access, consider caching the name list with getResourceNames().
			 *
			 * @param asyncLoad True for asynchronous loading (default), false for synchronous.
			 * @return Shared pointer to randomly selected resource, or nullptr if store is empty.
			 * @see getResourceNames() For getting all available resource names
			 * @note Thread-safe: locks m_resourcesAccess internally.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getRandomResource (bool asyncLoad = true) noexcept
			{
				if ( m_localStore == nullptr || m_localStore->empty() )
				{
					return nullptr;
				}

				/* NOTE: O(n) iteration through unordered_map is unavoidable without maintaining a separate key vector.
				 * The random index is in range [0, size-1] to avoid off-by-one errors. */
				const auto randomIndex = Libs::Utility::quickRandom< size_t >(0, m_localStore->size() - 1);

				auto it = m_localStore->begin();
				std::advance(it, static_cast< std::ptrdiff_t >(randomIndex));

				return this->getResource(it->first, asyncLoad);
			}

		private:

			/** @copydoc EmEn::Resources::ContainerInterface::initialize() noexcept */
			bool
			initialize () noexcept override
			{
				if ( m_localStore == nullptr )
				{
					return true;
				}

				if ( m_verboseEnabled )
				{
					TraceInfo{resource_t::ClassId} << "The resource type '" << resource_t::ClassId << "' has " << m_localStore->size() <<" entries in the local store available.";
				}

				return true;
			}

			/** @copydoc EmEn::Resources::ContainerInterface::terminate() noexcept */
			bool
			terminate () noexcept override
			{
				if ( m_localStore == nullptr )
				{
					return true;
				}

				if ( m_verboseEnabled )
				{
					TraceInfo{resource_t::ClassId} << "The resource type '" << resource_t::ClassId << "' has " << m_localStore->size() << " entries in the local store to check for unload.";
				}

				{
					const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

					m_externalResources.clear();
					m_resources.clear();
				}

				return true;
			}

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			bool
			onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override
			{
				if ( observable->is(Net::Manager::getClassUID()) )
				{
					if ( notificationCode == Net::Manager::FileDownloaded )
					{
						const std::lock_guard< std::mutex > scopeLock{m_resourcesAccess};

						const auto downloadTicket = std::any_cast< int >(data);

						auto requestIt = m_externalResources.find(downloadTicket);

						if ( requestIt != m_externalResources.end() )
						{
							switch ( m_primaryServices.netManager().downloadStatus(downloadTicket) )
							{
								case Net::DownloadStatus::Pending :
								case Net::DownloadStatus::Transferring :
								case Net::DownloadStatus::OnHold :
									return true;

								case Net::DownloadStatus::Done :
								{
									Tracer::success(resource_t::ClassId, "Resource downloaded.");

									requestIt->second.setDownloadProcessed(m_primaryServices.fileSystem(), true);

									/* Enqueue the resource loading in the thread pool. */
									auto request = requestIt->second;

									m_primaryServices.threadPool()->enqueue([this, request] {
										this->loadingTask(request);
									});
								}
									break;

								case Net::DownloadStatus::Error :
									Tracer::error(resource_t::ClassId, "Resource failed to download.");

									requestIt->second.setDownloadProcessed(m_primaryServices.fileSystem(), true);
									break;
							}

							/* Removes the loading request. */
							m_externalResources.erase(requestIt);
						}
					}

					return true;
				}

				/* We don't know who is sending this message,
				 * so we stop listening. */
				Tracer::warning(resource_t::ClassId, "Unknown notification, stop listening to this sender.");

				return false;
			}

			/**
			 * @brief Internal: Creates a new resource without locking the mutex.
			 *
			 * Private version of createResource() for use in contexts where the mutex is already
			 * locked. Prevents deadlock in methods that need to create resources while holding
			 * m_resourcesAccess.
			 *
			 * @param resourceName Unique name for the new resource.
			 * @param resourceFlags Resource-specific construction flags.
			 * @return Shared pointer to the newly created resource, or nullptr on failure.
			 * @pre Caller must hold m_resourcesAccess lock.
			 * @warning Not thread-safe: caller must ensure proper locking.
			 * @see createResource() Thread-safe public version
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			createResourceUnlocked (const std::string & resourceName, uint32_t resourceFlags) noexcept
			{
				if ( resourceName == Default )
				{
					TraceError{resource_t::ClassId} << Default << "' as resource name is a reserved key !";

					return nullptr;
				}

				/* First, check in store if the resource exists. */
				if ( m_localStore != nullptr && m_localStore->contains(resourceName) )
				{
					TraceWarning{resource_t::ClassId} <<
						resource_t::ClassId << " resource named '" << resourceName << "' already exists in local store ! "
						"Use get() function instead.";

					return nullptr;
				}

				/* Checks in loaded resources. */
				{
					auto loadedIt = m_resources.find(resourceName);

					if ( loadedIt != m_resources.cend() )
					{
						TraceWarning{resource_t::ClassId} <<
							resource_t::ClassId << " resource named '" << resourceName << "' already exists in loaded resources ! "
							"Use getResource() function instead.";

						return loadedIt->second;
					}
				}

				auto result = m_resources.emplace(resourceName, std::make_shared< resource_t >(resourceName, resourceFlags));

				if ( !result.second )
				{
					TraceFatal{resource_t::ClassId} <<
						"Unable to get " << resource_t::ClassId << " resource named '" << resourceName << "' into the map. "
						"This should never happens !";

					return nullptr;
				}

				if ( !result.first->second->enableManualLoading() )
				{
					return nullptr;
				}

				return result.first->second;
			}

			/**
			 * @brief Internal: Returns the default resource without locking the mutex.
			 *
			 * Private version of getDefaultResource() for use in contexts where the mutex is already
			 * locked. Creates the default resource on first access if it doesn't exist.
			 *
			 * @return Shared pointer to the default resource (never null).
			 * @pre Caller must hold m_resourcesAccess lock.
			 * @warning Not thread-safe: caller must ensure proper locking.
			 * @see getDefaultResource() Thread-safe public version
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			getDefaultResourceUnlocked () noexcept
			{
				/* Checks in loaded resources. */
				{
					const auto & loadedIt = m_resources.find(Default);

					if ( loadedIt != m_resources.cend() )
					{
						return loadedIt->second;
					}
				}

				/* Creates and load the resource. */
				auto defaultResource = std::make_shared< resource_t >(Default);

				if ( !defaultResource->load(m_serviceProvider) )
				{
					TraceFatal{resource_t::ClassId} << "The default resource '" << resource_t::ClassId << "' can't be loaded !";

					return nullptr;
				}

				/* Save the resource in a smart pointer and return it. */
				auto result = m_resources.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(Default),
					std::forward_as_tuple(defaultResource)
				);

				if ( !result.second )
				{
					return nullptr;
				}

				return defaultResource;
			}

			/**
			 * @brief Internal: Checks for existing resource or queues it for loading.
			 *
			 * Helper method that checks loaded resources first, then queries the store and
			 * initiates loading if found. Used by methods like getOrCreateUnloadedResource() that need
			 * to check existence before creating.
			 *
			 * @param resourceName Name of the resource to check/load.
			 * @param asyncLoad True for asynchronous loading, false for synchronous.
			 * @return Shared pointer to resource if found/loading, nullptr if not in store.
			 * @pre Caller must hold m_resourcesAccess lock.
			 * @warning Not thread-safe: caller must ensure proper locking.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			checkLoadedResource (const std::string & resourceName, bool asyncLoad) noexcept
			{
				if ( resourceName == Default )
				{
					return this->getDefaultResourceUnlocked();
				}

				/* Checks in loaded resources. */
				const auto & loadedIt = m_resources.find(resourceName);

				if ( loadedIt != m_resources.cend() )
				{
					return loadedIt->second;
				}

				/* If not already loaded, check in store for loading. */
				if ( m_localStore != nullptr )
				{
					if ( const auto resourceIt = m_localStore->find(resourceName); resourceIt != m_localStore->cend() )
					{
						return this->pushInLoadingQueue(resourceIt->second, !asyncLoad);
					}
				}

				return nullptr;
			}

			/**
			 * @brief Internal: Enqueues a resource for loading.
			 *
			 * Creates a LoadingRequest and either:
			 * - For external resources: submits download request or uses cache
			 * - For local/direct resources: enqueues loading task in thread pool (async) or loads directly (sync)
			 *
			 * Handles the complete resource loading pipeline including cache checks for external resources.
			 *
			 * @param baseInformation Resource metadata from the store.
			 * @param asyncLoad True for asynchronous loading, false for synchronous.
			 * @return Shared pointer to the resource being loaded, or nullptr on failure.
			 * @pre Caller must hold m_resourcesAccess lock.
			 * @warning Not thread-safe: caller must ensure proper locking.
			 * @version 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< resource_t >
			pushInLoadingQueue (const BaseInformation & baseInformation, bool asyncLoad) noexcept
			{
				const auto & name = baseInformation.name();

				/* 1. Check if a resource is not already in the loading queue ... */
				const auto & resourceIt = m_resources.find(name);

				if ( resourceIt != m_resources.cend() )
				{
					if ( m_verboseEnabled )
					{
						TraceInfo{resource_t::ClassId} << "Resource (" << resource_t::ClassId << ") '" << name << "' is already in the loading queue.";
					}

					return resourceIt->second;
				}

				/* Creates a new resource in the loading queue. */
				auto result = m_resources.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(name),
					std::forward_as_tuple(new resource_t(name, 0))
				);

				if ( !result.second )
				{
					TraceError{resource_t::ClassId} << "Unable to create the resource (" << resource_t::ClassId << ") '" << name << "' !";

					return nullptr;
				}

				/* Gets a reference to the smart pointer of the new resource. */
				auto & newResource = result.first->second;

				LoadingRequest< resource_t > request{baseInformation, newResource};

				if ( asyncLoad )
				{
					/* NOTE: Check if we need to download the resource first. */
					if ( baseInformation.sourceType() == SourceType::ExternalData )
					{
						if ( !request.isDownloadable() )
						{
							return nullptr;
						}

						/* NOTE: Check the cache system before downloading. */
						const auto cacheFile = request.cacheFilepath(m_primaryServices.fileSystem());

						if ( Libs::IO::fileExists(cacheFile) )
						{
							request.setDownloadProcessed(m_primaryServices.fileSystem(), true);
						}
						else
						{
							const auto ticket = m_primaryServices.netManager().download(request.url(), cacheFile, false);

							request.setDownloadTicket(ticket);

							m_externalResources.emplace(ticket, request);
						}
					}
					else
					{
						/* Enqueue the resource loading into the thread pool. */
						m_primaryServices.threadPool()->enqueue([this, request] {
							this->loadingTask(request);
						});
					}
				}
				else
				{
					newResource->setDirectLoadingHint();

					/* Call directly the loading function on the manager thread. */
					this->loadingTask(request);
				}

				return newResource;
			}

			/**
			 * @brief Internal: Worker task that performs the actual resource loading.
			 *
			 * This method runs either on a thread pool worker (async) or the calling thread (sync).
			 * It dispatches to the appropriate resource loading method based on source type and
			 * emits observable notifications for monitoring.
			 *
			 * **Observable Notifications:**
			 * - Emits LoadingProcessStarted before loading
			 * - Emits ResourceLoaded on success (with resource pointer as data)
			 * - Emits LoadingProcessFinished after completion (regardless of result)
			 *
			 * @param request LoadingRequest object (passed by value for thread safety).
			 * @note Handles LocalData (file paths), DirectData (JSON), but NOT ExternalData (must be downloaded first).
			 * @warning ExternalData sources trigger an error if called before download completes.
			 * @version 0.8.35
			 */
			void
			loadingTask (LoadingRequest< resource_t > request) noexcept
			{
				/* Notify the beginning of a loading process. */
				this->notify(LoadingProcessStarted);

				const auto & infos = request.baseInformation();

				auto success = false;

				/* Load the local resource and send a notification when finished. */
				switch ( infos.sourceType() )
				{
					/* This is a local file, so we load it by using a filepath. */
					case SourceType::LocalData :
						if ( m_verboseEnabled )
						{
							TraceInfo{resource_t::ClassId} << "Loading the resource (" << resource_t::ClassId << ") '" << infos.name() << "'... [CONTAINER]";
						}

						success = request.resource()->load(m_serviceProvider, std::filesystem::path{infos.data().asString()});
						break;

					/* This is direct data with a JsonCPP way of representing the data. */
					case SourceType::DirectData :
						if ( m_verboseEnabled )
						{
							TraceInfo{resource_t::ClassId} << "Loading the resource (" << resource_t::ClassId << ") '" << infos.name() << "'... [CONTAINER]";
						}

						success = request.resource()->load(m_serviceProvider, infos.data());
						break;

					/* This should never happen! ExternalData must be processed before. */
					case SourceType::ExternalData :
						TraceError{resource_t::ClassId} << "The resource (" << resource_t::ClassId << ") '" << infos.name() << "' should be downloaded first. Unable to load it ! [CONTAINER]";
						break;

					/* This should never happen! Undefined is a bug. */
					case SourceType::Undefined :
					default:
						TraceError{resource_t::ClassId} << "The resource (" << resource_t::ClassId << ") '" << infos.name() << "' information are invalid. Unable to load it ! [CONTAINER]";
						break;
				}

				if ( success )
				{
					if ( m_verboseEnabled )
					{
						TraceSuccess{resource_t::ClassId} << "The resource (" << resource_t::ClassId << ") '" << infos.name() << "' is loaded. [CONTAINER]";
					}

					this->notify(ResourceLoaded, request.resource().get());
				}
				else
				{
					TraceError{resource_t::ClassId} << "The resource (" << resource_t::ClassId << ") '" << infos.name() << "' failed to load ! [CONTAINER]";
				}

				/* Notify the end of the loading process. */
				this->notify(LoadingProcessFinished);
			}

			PrimaryServices & m_primaryServices;                                                     ///< Core engine services (threading, networking, filesystem).
			AbstractServiceProvider & m_serviceProvider;                                          ///< Service provider for resource loading operations.
			std::shared_ptr< std::unordered_map< std::string, BaseInformation > > m_localStore;  ///< Shared store of available resource metadata (name -> BaseInformation).
			std::unordered_map< std::string, std::shared_ptr< resource_t > > m_resources;        ///< Map of loaded resources (name -> resource).
			std::unordered_map< int, LoadingRequest< resource_t > > m_externalResources;         ///< Active download requests (ticket -> request).
			mutable std::mutex m_resourcesAccess;                                                 ///< Mutex protecting m_resources and m_externalResources.
			bool m_verboseEnabled{false};                                                         ///< Verbose logging flag for detailed trace output.
	};
}
