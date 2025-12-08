/*
 * src/Resources/Manager.hpp
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
#include <unordered_map>
#include <typeindex>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"
#include "ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "Container.hpp"

/* Forward declarations. */
namespace EmEn
{
	class PrimaryServices;
	class NetworkManager;
}

namespace EmEn::Resources
{
	/**
	 * @class Manager
	 * @brief The central resource management service for the Emeraude Engine.
	 *
	 * The Manager class is responsible for managing all resource types in the engine,
	 * including loading, caching, memory management, and lifecycle control. It maintains
	 * type-indexed containers for different resource types (textures, meshes, sounds, etc.)
	 * and coordinates resource discovery through either JSON-based indexing or dynamic
	 * directory scanning.
	 *
	 * Resource Discovery Modes:
	 * - JSON Indexing: Uses pre-generated ResourcesIndex.XXX.json files for fast loading
	 * - Dynamic Scanning: Automatically discovers resources by scanning data-stores directories
	 *
	 * The manager organizes resources into logical stores (e.g., "Images", "Sounds", "Meshes")
	 * and provides thread-safe access to resource containers. It supports runtime resource
	 * updates and memory optimization through unused resource cleanup.
	 *
	 * @extends EmEn::ServiceInterface The resource manager is a service.
	 * @extends EmEn::Resources::AbstractServiceProvider Provides resource loading services.
	 *
	 * @note This class is thread-safe for store access operations.
	 * @note Resources are loaded on-demand and cached until explicitly unloaded.
	 *
	 * @see AbstractServiceProvider For container access methods.
	 * @see ContainerInterface For resource container interface.
	 * @see BaseInformation For resource metadata structure.
	 *
	 * @since 0.8.35
	 */
	class Manager final : public ServiceInterface, public AbstractServiceProvider
	{
		public:

			/** @brief Class identifier for logging and debugging. */
			static constexpr auto ClassId{"ResourcesManagerService"};

			/**
			 * @brief Constructs the resource manager service.
			 *
			 * Initializes the resource manager with references to required engine services.
			 * The manager does not take ownership of these services and expects them to
			 * remain valid throughout its lifetime.
			 *
			 * @param primaryServices Reference to the engine's primary services provider.
			 * @param graphicsRenderer Reference to the graphics rendering subsystem.
			 *
			 * @note The constructor does not initialize resource containers. Call initialize()
			 *       after construction to set up resource management.
			 * @note This class follows the service initialization pattern defined by ServiceInterface.
			 *
			 * @since 0.8.35
			 */
			explicit
			Manager (PrimaryServices & primaryServices, Graphics::Renderer & graphicsRenderer) noexcept
				: ServiceInterface{ClassId},
				AbstractServiceProvider{primaryServices.fileSystem(), graphicsRenderer},
				m_primaryServices{primaryServices}
			{

			}

			/**
			 * @brief Updates resource stores from a JSON definition.
			 *
			 * Allows dynamic addition of resource stores at runtime by parsing a JSON object
			 * containing store definitions. The JSON must contain a "Stores" key with store
			 * arrays defining individual resources.
			 *
			 * @param root The JSON object containing store definitions.
			 * @return true if stores were successfully parsed and added, false otherwise.
			 *
			 * @note This operation is thread-safe.
			 * @note Existing resources with duplicate names will not be overwritten.
			 *
			 * @see parseStores() For the actual parsing implementation.
			 *
			 * @since 0.8.35
			 */
			bool update (const Json::Value & root) noexcept override;

			/**
			 * @brief Returns a reference to the primary services provider.
			 *
			 * Provides access to core engine services including file system, settings,
			 * and other fundamental subsystems required for resource management.
			 *
			 * @return Reference to the primary services provider.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			PrimaryServices &
			primaryServices () const noexcept
			{
				return m_primaryServices;
			}

			/**
			 * @brief Enables or disables verbose logging for all resource operations.
			 *
			 * When enabled, detailed information about resource loading, registration,
			 * and management operations will be logged to the console. This affects both
			 * the manager and all resource containers.
			 *
			 * @param state true to enable verbose logging, false to disable.
			 *
			 * @note Changes affect all existing containers and the global ResourceTrait verbosity flag.
			 *
			 * @since 0.8.35
			 */
			void setVerbosity (bool state) noexcept;

			/**
			 * @brief Checks whether verbose logging is currently enabled.
			 *
			 * @return true if verbose logging is enabled, false otherwise.
			 *
			 * @see setVerbosity() To change the verbosity state.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			bool
			verbosityEnabled () const noexcept
			{
				return m_showInformation;
			}

			/**
			 * @brief Checks whether dynamic directory scanning mode is enabled.
			 *
			 * Returns the current resource discovery mode. When dynamic scanning is enabled,
			 * the manager automatically discovers resources by scanning data-stores directories.
			 * When disabled, the manager uses pre-generated JSON index files.
			 *
			 * @return true if dynamic scanning is enabled, false if using JSON indexing.
			 *
			 * @note The discovery mode is set during initialization based on engine settings.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			bool
			isUsingDynamicScan () const noexcept
			{
				return m_useDynamicScan;
			}

			/**
			 * @brief Calculates the total memory consumed by all loaded resources.
			 *
			 * Iterates through all resource containers and sums the memory occupied by
			 * loaded resources, including both actively used and cached resources.
			 *
			 * @return Total memory occupied in bytes across all resource containers.
			 *
			 * @note This includes GPU and CPU memory for graphics resources.
			 * @note Thread-safe operation.
			 *
			 * @see unusedMemoryOccupied() For memory consumed only by unused resources.
			 * @see unloadUnusedResources() To free unused resource memory.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			size_t memoryOccupied () const noexcept;

			/**
			 * @brief Calculates the memory consumed by unused but loaded resources.
			 *
			 * Iterates through all resource containers and sums the memory occupied by
			 * resources that are loaded but no longer referenced by active objects.
			 * These resources are candidates for unloading to free memory.
			 *
			 * @return Total memory occupied by unused resources in bytes.
			 *
			 * @note This includes GPU and CPU memory for graphics resources.
			 * @note Thread-safe operation.
			 *
			 * @see memoryOccupied() For total memory consumption.
			 * @see unloadUnusedResources() To free this memory.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			size_t unusedMemoryOccupied () const noexcept;

			/**
			 * @brief Unloads all unused resources from memory across all containers.
			 *
			 * Performs a multi-pass cleanup of resources that are no longer referenced
			 * by active objects. Containers are sorted by dependency complexity to ensure
			 * proper unloading order (complex resources that depend on simpler ones are
			 * unloaded first). Multiple passes continue until no more resources can be freed.
			 *
			 * @return Total number of resources successfully unloaded.
			 *
			 * @note This operation may take multiple passes to resolve dependency chains.
			 * @note Unloading frees both CPU and GPU memory.
			 * @note Thread-safe operation.
			 *
			 * @see memoryOccupied() To check memory before cleanup.
			 * @see unusedMemoryOccupied() To check potential memory savings.
			 *
			 * @since 0.8.35
			 */
			size_t unloadUnusedResources () noexcept;

		private:

			/**
			 * @brief Initializes the resource manager service.
			 *
			 * Loads engine settings for resource management (verbosity, download permissions,
			 * dynamic scanning mode), then populates resource stores either by reading JSON
			 * index files or scanning directories. Creates all resource type containers
			 * (sounds, textures, meshes, etc.) and associates them with their respective stores.
			 *
			 * @return true if initialization succeeded, false otherwise.
			 *
			 * @note This method is called automatically by ServiceInterface::initialize().
			 * @note Creates 25+ different resource containers for all supported resource types.
			 *
			 * @see readResourceIndexes() For JSON-based resource discovery.
			 * @see scanResourceDirectories() For dynamic resource scanning.
			 *
			 * @since 0.8.35
			 */
			bool onInitialize () noexcept override;

			/**
			 * @brief Terminates the resource manager service.
			 *
			 * Gracefully shuts down all resource containers and clears the container map.
			 * Each container is given the opportunity to properly release its resources
			 * before destruction.
			 *
			 * @return true if termination succeeded, false otherwise.
			 *
			 * @note This method is called automatically by ServiceInterface::terminate().
			 * @note All resources should be unloaded before termination for clean shutdown.
			 *
			 * @since 0.8.35
			 */
			bool onTerminate () noexcept override;

			/**
			 * @brief Retrieves a resource container by its type index.
			 *
			 * Internal method used by the template container() method to access
			 * type-specific resource containers. Logs a fatal error if the requested
			 * container type does not exist.
			 *
			 * @param typeIndex The std::type_index of the resource type.
			 * @return Pointer to the container, or nullptr if not found.
			 *
			 * @note This method is used internally by AbstractServiceProvider::container<T>().
			 * @warning Returns nullptr and logs fatal error if container doesn't exist.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			ContainerInterface * getContainerInternal (const std::type_index & typeIndex) noexcept override;

			/**
			 * @brief Retrieves a resource container by its type index (const version).
			 *
			 * Internal method used by the template container() method to access
			 * type-specific resource containers. Logs a fatal error if the requested
			 * container type does not exist.
			 *
			 * @param typeIndex The std::type_index of the resource type.
			 * @return Const pointer to the container, or nullptr if not found.
			 *
			 * @note This method is used internally by AbstractServiceProvider::container<T>().
			 * @warning Returns nullptr and logs fatal error if container doesn't exist.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			const ContainerInterface * getContainerInternal (const std::type_index & typeIndex) const noexcept override;

			/**
			 * @brief Reads JSON resource index files to populate resource stores.
			 *
			 * Searches for ResourcesIndex.XXX.json files in all data-stores directories
			 * and parses them to build the internal resource catalog. Index files contain
			 * pre-generated lists of available resources organized by store type.
			 *
			 * @return true if at least one index file was successfully loaded, false otherwise.
			 *
			 * @note This is the faster resource discovery method compared to dynamic scanning.
			 * @note Multiple index files can be loaded and merged.
			 * @warning If no index files are found, a warning is logged.
			 *
			 * @see getResourcesIndexFiles() To locate index files.
			 * @see parseStores() For the actual JSON parsing logic.
			 * @see scanResourceDirectories() For the alternative discovery method.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			bool readResourceIndexes () noexcept;

			/**
			 * @brief Scans data-stores directories to automatically discover resources.
			 *
			 * Performs a recursive scan of all data-stores directories to discover resource
			 * files. Resources are organized by their parent directory names (e.g., Images,
			 * Sounds, Meshes) and registered in the appropriate stores. This method allows
			 * adding new resources without regenerating index files.
			 *
			 * @return true if at least one resource was found, false otherwise.
			 *
			 * @note This is slower than JSON indexing but more flexible for development.
			 * @note Hidden files and directories (starting with '.') are skipped.
			 * @note Subdirectory structure is preserved in resource names.
			 *
			 * @see determineStoreForFile() For file-to-store mapping logic.
			 * @see readResourceIndexes() For the alternative discovery method.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			bool scanResourceDirectories () noexcept;

			/**
			 * @brief Determines the appropriate store name for a file during dynamic scanning.
			 *
			 * Analyzes a file's extension and directory structure to determine which resource
			 * store it belongs to. Uses a mapping of file extensions to store names (e.g., .png
			 * files go to "Images", .wav files to "Sounds"). For JSON files or unknown types,
			 * falls back to using the parent directory name.
			 *
			 * @param filepath The absolute path to the resource file.
			 * @param dataStoreDirectory The base data-stores directory path.
			 * @return The determined store name (e.g., "Images", "Sounds"), or empty string if undetermined.
			 *
			 * @note This is a static utility method used during directory scanning.
			 * @note Extension mapping covers common formats: images, audio, video, fonts, and 3D models.
			 *
			 * @see scanResourceDirectories() Where this method is utilized.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			static std::string determineStoreForFile (const std::filesystem::path & filepath, const std::filesystem::path & dataStoreDirectory) noexcept;

			/**
			 * @brief Parses JSON store definitions and populates resource stores.
			 *
			 * Processes a JSON object containing resource store definitions. Each store is
			 * an array of resource objects with metadata (name, source, data path). Resources
			 * are validated and added to their respective stores, with duplicate checking to
			 * prevent overwriting existing resources.
			 *
			 * @param fileSystem Reference to the file system for path resolution.
			 * @param storesObject The JSON object containing store arrays (e.g., {"Images": [...], "Sounds": [...]}).
			 * @param verbose If true, logs detailed information about each registered resource.
			 * @return true if at least one resource was successfully registered, false otherwise.
			 *
			 * @note Resources with names starting with '+' are reserved by the engine and rejected.
			 * @note Duplicate resource names within a store are skipped with a warning.
			 * @note Thread-safe when called with m_localStoresAccess lock held.
			 *
			 * @see update() Which calls this method for runtime store updates.
			 * @see readResourceIndexes() Which calls this method during initialization.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			bool parseStores (const FileSystem & fileSystem, const Json::Value & storesObject, bool verbose) noexcept;

			/**
			 * @brief Retrieves a resource store by its name.
			 *
			 * Returns a shared pointer to the internal resource information map for the
			 * specified store. Used during container initialization to associate containers
			 * with their data sources.
			 *
			 * @param storeName The name of the store to retrieve (e.g., "Images", "Sounds", "Meshes").
			 * @return Shared pointer to the store's resource map, or nullptr if the store doesn't exist.
			 *
			 * @warning This method must be called while holding the m_localStoresAccess mutex lock.
			 * @note Returns nullptr for non-existent stores without logging an error.
			 *
			 * @see onInitialize() Where this method is used to connect containers to stores.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			std::shared_ptr< std::unordered_map< std::string, BaseInformation > > getLocalStore (const std::string & storeName) noexcept;

			/**
			 * @brief Allows stream output operator to access private members.
			 *
			 * Declares the stream insertion operator as a friend to enable printing
			 * of Manager state including store information.
			 *
			 * @see operator<<(std::ostream&, const Manager&) For the implementation.
			 *
			 * @since 0.8.35
			 */
			friend std::ostream & operator<< (std::ostream & out, const Manager & obj);

			/**
			 * @brief Checks if a string contains JSON data.
			 *
			 * Simple heuristic check that looks for the presence of curly braces
			 * to determine if a string likely contains JSON data.
			 *
			 * @param buffer The string to check.
			 * @return true if the buffer appears to contain JSON data (contains '{'), false otherwise.
			 *
			 * @note This is a simple heuristic, not a full JSON validator.
			 * @note Currently unused in the implementation.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			static bool isJSONData (const std::string & buffer) noexcept;

			/**
			 * @brief Locates all resource index files in data-stores directories.
			 *
			 * Searches all registered data-stores directories for files matching the pattern
			 * ResourcesIndex.NNN.json (where NNN is a three-digit number). These index files
			 * contain pre-generated resource catalogs for faster loading.
			 *
			 * @param fileSystem Reference to the file system service for directory access.
			 * @return Vector of absolute file paths to found index files.
			 *
			 * @note Searches all data directories registered with the file system.
			 * @note Logs a warning for each data-stores directory that lacks index files.
			 * @note The three-digit numbering allows for ordered loading of multiple indexes.
			 *
			 * @see readResourceIndexes() Which uses this method to find index files.
			 *
			 * @since 0.8.35
			 */
			[[nodiscard]]
			static std::vector< std::string > getResourcesIndexFiles (const FileSystem & fileSystem) noexcept;

			/** @brief JSON key for accessing store definitions in resource index files. */
			static constexpr auto StoresKey{"Stores"};

			/** @brief Reference to the engine's primary services provider. */
			PrimaryServices & m_primaryServices;

			/** @brief Map of resource stores, indexed by store name, containing resource metadata. */
			std::unordered_map< std::string, std::shared_ptr< std::unordered_map< std::string, BaseInformation > > > m_localStores;

			/** @brief Map of resource containers, indexed by resource type, managing loaded resources. */
			std::unordered_map< std::type_index, std::unique_ptr< ContainerInterface > > m_containers;

			/** @brief Mutex protecting concurrent access to m_localStores. */
			mutable std::mutex m_localStoresAccess;

			/** @brief Flag indicating whether verbose logging is enabled for resource operations. */
			bool m_showInformation{false};

			/** @brief Flag indicating whether downloading resources from remote sources is allowed. */
			bool m_downloadingAllowed{false};

			/** @brief Flag indicating whether resource conversion should suppress output messages. */
			bool m_quietConversion{false};

			/** @brief Flag indicating whether dynamic directory scanning is used instead of JSON indexing. */
			bool m_useDynamicScan{false};
	};

	/**
	 * @brief Outputs Manager state to a stream.
	 *
	 * Formats and writes the Manager's resource store information to an output stream.
	 * Lists all stores with their resource counts.
	 *
	 * @param out The output stream to write to.
	 * @param obj The Manager instance to output.
	 * @return Reference to the output stream for chaining.
	 *
	 * @note Output format: Lists each store name with its resource count.
	 *
	 * @since 0.8.35
	 */
	std::ostream & operator<< (std::ostream & out, const Manager & obj);

	/**
	 * @brief Converts Manager state to a string representation.
	 *
	 * Creates a string representation of the Manager by invoking the stream
	 * insertion operator. Useful for logging and debugging.
	 *
	 * @param obj The Manager instance to stringify.
	 * @return String containing the Manager's state information.
	 *
	 * @see operator<<(std::ostream&, const Manager&) For the formatting logic.
	 *
	 * @since 0.8.35
	 */
	std::string to_string (const Manager & obj) noexcept;
}
