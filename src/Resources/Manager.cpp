/*
 * src/Resources/Manager.cpp
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
#include "emeraude_platform.hpp"

/* Third-party inclusions. */
#if IS_LINUX && defined(__GLIBC__)
#include <malloc.h>
#endif

/* STL inclusions. */
#include <chrono>
#include <cmath>
#include <ranges>
#include <regex>
#include <thread>

/* Local inclusions. */
#include "Animations/AnimationClipResource.hpp"
#include "Animations/SkeletonResource.hpp"
#include "Audio/MusicResource.hpp"
#include "Audio/PlaylistResource.hpp"
#include "Audio/SoundfontResource.hpp"
#include "Audio/SoundResource.hpp"
#include "Graphics/CompressedImageResource.hpp"
#include "Graphics/CubemapMovieResource.hpp"
#include "Graphics/CubemapResource.hpp"
#include "Graphics/FontResource.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Geometry/RawIndexedVertexResource.hpp"
#include "Graphics/Geometry/PulledVertexResource.hpp"
#include "Graphics/Geometry/RawVertexResource.hpp"
#include "Graphics/Geometry/VertexGridResource.hpp"
#include "Graphics/Geometry/VertexResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/BeamResource.hpp"
#include "Graphics/Material/PathResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/MovieResource.hpp"
#include "Graphics/Renderable/BasicGroundResource.hpp"
#include "Graphics/Renderable/BasicSeaResource.hpp"
#include "Graphics/Renderable/DynamicSkyResource.hpp"
#include "Graphics/Renderable/MultiLayerMeshResource.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/SkyBoxResource.hpp"
#include "Graphics/Renderable/SpriteResource.hpp"
#include "Graphics/Renderable/TerrainResource.hpp"
#include "Graphics/CloudShapeResource.hpp"
#include "Graphics/TextureResource/AnimatedTexture2D.hpp"
#include "Graphics/TextureResource/AnimatedTextureCubemap.hpp"
#include "Graphics/TextureResource/Texture1D.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Graphics/TextureResource/Texture3D.hpp"
#include "Graphics/TextureResource/TextureCubemap.hpp"
#include "Arguments.hpp"
#include "FastJSON.hpp"
#include "FileSystem.hpp"
#include "IO/IO.hpp"
#include "Net/Manager.hpp"
#include "Network/URI.hpp"
#include "PeerStore.hpp"
#include "PrimaryServices.hpp"
#include "Scenes/DefinitionResource.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "SharingServer.hpp"

namespace EmEn::Resources
{
	using namespace Base;

	ContainerInterface *
	Manager::getContainerInternal (const std::type_index & typeIndex) noexcept
	{
		const auto containerIt = m_containers.find(typeIndex);

		if ( containerIt == m_containers.end() )
		{
			Tracer::fatal(ClassId, "Container does not exist !");

			return nullptr;
		}

		return containerIt->second.get();
	}

	const ContainerInterface *
	Manager::getContainerInternal (const std::type_index & typeIndex) const noexcept
	{
		const auto containerIt = m_containers.find(typeIndex);

		if ( containerIt == m_containers.end() )
		{
			Tracer::fatal(ClassId, "Container does not exist !");

			return nullptr;
		}

		return containerIt->second.get();
	}

	Manager::Manager (PrimaryServices & primaryServices, Graphics::Renderer & graphicsRenderer, Audio::Manager & audioManager) noexcept
		: ServiceInterface{ClassId},
		ControllableTrait{ClassId},
		AbstractServiceProvider{primaryServices, graphicsRenderer, audioManager},
		m_primaryServices{primaryServices}
	{

	}

	Manager::~Manager ()
	{
		/* NOTE: normally already done by onTerminate(); the server first, its index reads the stores. */
		m_sharingServer.reset();
		m_peerStore.reset();
	}

	std::shared_ptr< std::unordered_map< std::string, BaseInformation > >
	Manager::getLocalStore (const std::string & storeName) noexcept
	{
		/* NOTE: Create the store when it does not exist yet, NEVER return null.
		 * Containers capture this shared_ptr once, at registration, and keep it
		 * for their whole life. Returning null for a store the boot-time scan
		 * happened not to produce left the container permanently sterile: a
		 * later update() - which is what Core::openFiles() on a resource index
		 * calls - would then create a BRAND NEW map under that name, visible to
		 * the manager and to nothing else. The resources registered fine and
		 * were unreachable, with no error anywhere. An empty store shared from
		 * the start costs one allocation and keeps the runtime path working. */
		const auto it = m_localStores.find(storeName);

		if ( it != m_localStores.cend() )
		{
			return it->second;
		}

		return m_localStores[storeName] = std::make_shared< std::unordered_map< std::string, BaseInformation > >();
	}

	void
	Manager::setVerbosity (bool state) noexcept
	{
		m_showInformation = state;

		ResourceTrait::s_showInformation = state;

		for ( const auto & resourceContainer : m_containers | std::views::values )
		{
			resourceContainer->setVerbosity(state);
		}
	}

	size_t
	Manager::memoryOccupied () const noexcept
	{
		size_t bytes = 0;

		for ( const auto & container : m_containers | std::views::values )
		{
			bytes += container->memoryOccupied();
		}

		return bytes;
	}

	void
	Manager::releaseIdleLocalData () noexcept
	{
		if ( !m_releaseLocalData )
		{
			return;
		}

		constexpr std::chrono::seconds PassInterval{1};

		const auto now = std::chrono::steady_clock::now();

		if ( now - m_lastLocalDataReleasePass < PassInterval )
		{
			return;
		}

		/* NOTE: One pass at a time, on a pool worker: a pass extracts metadata (full scans of large images) and frees
		 * memory, up to ~165 ms measured on terrain (2026-10-04) — never on the logic loop (owner decision). */
		if ( m_localDataReleasePassRunning.exchange(true) )
		{
			return;
		}

		m_lastLocalDataReleasePass = now;

		const auto threadPool = m_primaryServices.threadPool();

		if ( threadPool == nullptr || !threadPool->enqueue([this, now] {
			this->runLocalDataReleasePass(now);

			m_localDataReleasePassRunning = false;
		}) )
		{
			m_localDataReleasePassRunning = false;
		}
	}

	void
	Manager::runLocalDataReleasePass (std::chrono::steady_clock::time_point now) noexcept
	{
		size_t released = 0;

		for ( const auto & container : m_containers | std::views::values )
		{
			released += container->releaseIdleLocalData(now, m_localDataReleaseDelay);
		}

		if ( released > 0 )
		{
			/* NOTE: A load releases its copies over several passes: the memory is given back once, when the burst
			 * ends (owner decision 2026-10-04). */
			m_releaseBurstInProgress = true;

			if ( m_showInformation )
			{
				TraceInfo{ClassId} << released << " CPU cop" << (released > 1 ? "ies" : "y") << " of uploaded resources released.";
			}
		}
		else if ( m_releaseBurstInProgress )
		{
			m_releaseBurstInProgress = false;

			Manager::returnFreedMemoryToSystem();
		}
	}

	size_t
	Manager::releaseLocalData (bool ignoreGraceDelay) noexcept
	{
		const auto now = std::chrono::steady_clock::now();
		const auto graceDelay = ignoreGraceDelay ? std::chrono::steady_clock::duration::zero() : m_localDataReleaseDelay;

		size_t released = 0;

		for ( const auto & container : m_containers | std::views::values )
		{
			released += container->releaseIdleLocalData(now, graceDelay);
		}

		if ( released > 0 )
		{
			Manager::returnFreedMemoryToSystem();
		}

		return released;
	}

	void
	Manager::returnFreedMemoryToSystem () noexcept
	{
		/* NOTE: glibc keeps freed blocks in its arenas: without a trim the released copies barely lower the RSS (citadel
		 * with the release on: 5143 MiB untrimmed, ~2.5 GiB trimmed). A trim costs 1-60 ms and holds the arena locks
		 * (measured 2026-10-04): it runs in the release pass's pool worker or the console's call, never on the logic
		 * loop, once per release burst (owner decisions 2026-10-04; Linux / glibc only, nothing elsewhere). */
#if IS_LINUX && defined(__GLIBC__)
		static_cast< void >(malloc_trim(0));
#endif
	}

	size_t
	Manager::unusedMemoryOccupied () const noexcept
	{
		size_t bytes = 0;

		for ( const auto & container : m_containers | std::views::values )
		{
			bytes += container->unusedMemoryOccupied();
		}

		return bytes;
	}

	size_t
	Manager::unloadUnusedResources () noexcept
	{
		std::vector< ContainerInterface * > sortedContainers;
		sortedContainers.reserve(m_containers.size());

		for ( const auto & container : m_containers | std::views::values )
		{
			sortedContainers.push_back(container.get());
		}

		/* NOTE: Sort container by depth dependency complexity. */
		std::ranges::sort(sortedContainers, [] (const ContainerInterface * a, const ContainerInterface * b) {
			return static_cast< int >(a->complexity()) > static_cast< int >(b->complexity());
		});

		size_t totalUnloaded = 0;
		size_t passUnloaded = 0;

		do
		{
			passUnloaded = 0;

			for ( auto * container : sortedContainers )
			{
				passUnloaded += container->unloadUnusedResources();
			}

			totalUnloaded += passUnloaded;
		} while ( passUnloaded > 0 );

		return totalUnloaded;
	}

	bool
	Manager::update (const Json::Value & root) noexcept
	{
		if ( !root.isObject() )
		{
			Tracer::warning(ClassId, "It must be a JSON object to check for additional stores !");

			return false;
		}

		if ( !root.isMember(StoresKey) )
		{
			return false;
		}

		const auto & stores = root[StoresKey];

		if ( !stores.isObject() )
		{
			TraceError{ClassId} << "'" << StoresKey << "' key must be a JSON object !";

			return false;
		}

		const std::scoped_lock lock{m_localStoresAccess};

		return this->parseStores(m_primaryServices.fileSystem(), stores, m_showInformation);
	}

	bool
	Manager::readResourceIndexes () noexcept
	{
		const auto & fileSystem = m_primaryServices.fileSystem();
		const auto indexes = Manager::getResourcesIndexFiles(fileSystem);

		if ( indexes.empty() )
		{
			std::stringstream message;

			message <<
				"No resources index available !" "\n"
				"Checked directories :" "\n";

			for ( auto directory : fileSystem.dataDirectories() )
			{
				message << directory.append(DataStores).string() << "\n";
			}

			TraceWarning{ClassId} << message;

			return false;
		}

		for ( const auto & filepath : indexes )
		{
			TraceInfo{ClassId} << "Loading resource index from file '" << filepath << "' ...";

			/* 1. Get raw JSON data from a file. */
			const auto rootCheck = FastJSON::getRootFromFile(filepath);

			if ( !rootCheck )
			{
				TraceError{ClassId} << "Unable to parse the index file " << filepath << " !" "\n";

				continue;
			}

			const auto & root = *rootCheck;

			/* 3. Register every stores (jsoncpp's member access aborts on anything but an object). */
			if ( !root.isObject() || !root.isMember(StoresKey) )
			{
				TraceError{ClassId} << "'" << StoresKey << "' key doesn't exist !";

				continue;
			}

			const auto & storesObject = root[StoresKey];

			if ( !storesObject.isObject() )
			{
				TraceError{ClassId} << "'" << StoresKey << "' key must be a JSON object !";

				continue;
			}

			if ( this->parseStores(fileSystem, storesObject, m_showInformation) )
			{
				TraceSuccess{ClassId} << "Resource index '" << filepath << "' loaded !";
			}
		}

		return true;
	}

	std::string
	Manager::determineStoreForFile (const std::filesystem::path & filepath, const std::filesystem::path & dataStoreDirectory) noexcept
	{
		/* Extract file extension for type detection. */
		const auto extension = filepath.extension().string();

		/* For JSON files, we need to inspect the parent directory name
		 * as they usually contain configuration data for different resource types. */
		if ( extension == ".json" )
		{
			/* Get the parent directory name relative to data-stores/
			 * This typically corresponds to a store name (e.g., "Backgrounds", "Materials"). */
			std::error_code relativeError;
			const auto relativePath = std::filesystem::relative(filepath.parent_path(), dataStoreDirectory, relativeError);

			if ( !relativeError && !relativePath.empty() )
			{
				/* Return the first component of the relative path as the store name. */
				return relativePath.begin()->string();
			}

			return {};
		}

		/* Map common file extensions to their corresponding stores. */
		static const std::unordered_map< std::string, std::string > extensionToStore = {
			/* Image formats → Images store */
			{".png", "Images"}, {".jpg", "Images"}, {".jpeg", "Images"},
			{".bmp", "Images"}, {".tga", "Images"}, {".dds", "Images"},
			{".webp", "Images"}, {".gif", "Images"},

			/* Audio formats → Sounds or Musics store (we'll default to Sounds) */
			{".wav", "Sounds"}, {".ogg", "Sounds"}, {".mp3", "Sounds"},
			{".flac", "Musics"}, {".opus", "Musics"}, {".aiff", "Musics"},

			/* SoundFont formats → SoundBanks store */
			{".sf2", "SoundBanks"},

			/* 3D Model formats → Meshes store */
			{".gltf", "Meshes"}, {".glb", "Meshes"}, {".obj", "Meshes"},
			{".fbx", "Meshes"}, {".dae", "Meshes"},

			/* Font formats → Fonts store */
			{".ttf", "Fonts"}, {".otf", "Fonts"}, {".woff", "Fonts"}, {".woff2", "Fonts"},

			/* Video formats → Movies store */
			{".mp4", "Movies"}, {".webm", "Movies"}, {".mkv", "Movies"},
			{".avi", "Movies"}, {".mov", "Movies"},
		};

		const auto it = extensionToStore.find(extension);

		if ( it != extensionToStore.end() )
		{
			return it->second;
		}

		/* If we can't determine the store from extension,
		 * try using the parent directory name. */
		std::error_code relativeError;
		const auto relativePath = std::filesystem::relative(filepath.parent_path(), dataStoreDirectory, relativeError);

		if ( !relativeError && !relativePath.empty() )
		{
			return relativePath.begin()->string();
		}

		return {};
	}

	bool
	Manager::scanResourceDirectories () noexcept
	{
		const auto & fileSystem = m_primaryServices.fileSystem();
		size_t resourcesFound = 0;

		/* For each data directory pointed by the file system. */
		for ( auto dataStoreDirectory : fileSystem.dataDirectories() )
		{
			dataStoreDirectory.append(DataStores);

			if ( !IO::directoryExists(dataStoreDirectory) )
			{
				/* No "data-stores/" in this data directory. */
				continue;
			}

			if ( m_showInformation )
			{
				TraceInfo{ClassId} << "Scanning directory: " << dataStoreDirectory;
			}

			/* First, iterate over first-level directories which represent stores.
			 * NOTE: never a range-for over directory_iterator: its operator++ throws (terminate under -fno-exceptions). */
			const auto storesWalked = IO::forEachDirectoryEntry(dataStoreDirectory, false, [&] (const std::filesystem::directory_entry & storeEntry) {
				std::error_code entryError;

				if ( !storeEntry.is_directory(entryError) )
				{
					/* This entry is not a directory, skip it (e.g., ResourcesIndex.json files). */
					return true;
				}

				const auto storeName = storeEntry.path().filename().string();

				/* Skip hidden directories (starting with dot). */
				if ( storeName.starts_with('.') )
				{
					return true;
				}

				if ( m_showInformation )
				{
					TraceInfo{ClassId} << "Scanning store directory: " << storeName;
				}

				/* Create or retrieve the store. */
				if ( !m_localStores.contains(storeName) )
				{
					m_localStores[storeName] = std::make_shared< std::unordered_map< std::string, BaseInformation > >();

					if ( m_showInformation )
					{
						TraceInfo{ClassId} << "Creating store '" << storeName << "' from dynamic scan";
					}
				}

				const auto & store = m_localStores[storeName];
				const auto & storeDirectory = storeEntry.path();

				/* Now, recursively scan files within this store directory. */
				const auto filesWalked = IO::forEachDirectoryEntry(storeDirectory, true, [&] (const std::filesystem::directory_entry & fileEntry) {
					std::error_code fileError;

					if ( !fileEntry.is_regular_file(fileError) )
					{
						/* This entry is not a file (could be a directory or symlink). */
						return true;
					}

					const auto & filepath = fileEntry.path();
					const auto filename = filepath.filename().string();

					/* Skip hidden files (starting with dot). */
					if ( filename.starts_with('.') )
					{
						return true;
					}

					/* Calculate relative path from the store directory (not data-stores).
					 * This will be used as the resource name and preserve subdirectory structure.
					 * Examples:
					 *   - data-stores/Images/texture.png -> resourceName = "texture"
					 *   - data-stores/Images/Murs/briques001.png -> resourceName = "Murs/briques001"
					 * NOTE: lexically_relative(), not std::filesystem::relative(): the walk yields storeDirectory/…
					 * paths, so the result is the same without canonicalizing both paths (system calls) per file. */
					const auto relativePathFromStore = filepath.lexically_relative(storeDirectory);
					const auto resourceName = relativePathFromStore.parent_path() / relativePathFromStore.stem();
					const auto resourceNameStr = resourceName.generic_string();

					/* Calculate relative path from data-stores directory for the "Data" field. */
					const auto relativePath = filepath.lexically_relative(dataStoreDirectory);

					/* Check if resource already exists (might have been loaded from JSON index). */
					if ( store->contains(resourceNameStr) )
					{
						/* Resource already registered, skip it. */
						return true;
					}

					/* Create a JSON-like structure for BaseInformation parsing. */
					Json::Value resourceDefinition;
					resourceDefinition["Name"] = resourceNameStr;
					resourceDefinition["Source"] = "LocalData";
					resourceDefinition["Data"] = relativePath.string();

					/* Parse and add to store. */
					BaseInformation baseInformation;

					if ( baseInformation.parse(fileSystem, resourceDefinition) )
					{
						store->emplace(resourceNameStr, baseInformation);
						resourcesFound++;

						if ( m_showInformation )
						{
							TraceInfo{ClassId} << "Registered '" << resourceNameStr << "' in '" << storeName << "' store";
						}
					}
					else
					{
						TraceWarning{ClassId} << "Failed to parse resource information for: " << filepath;
					}

					return true;
				});

				if ( !filesWalked )
				{
					TraceWarning{ClassId} << "The store directory " << storeDirectory << " could not be fully scanned (see the IO error above).";
				}

				return true;
			});

			if ( !storesWalked )
			{
				TraceWarning{ClassId} << "The data-stores directory " << dataStoreDirectory << " could not be fully scanned (see the IO error above).";
			}
		}

		if ( m_showInformation || resourcesFound > 0 )
		{
			TraceSuccess{ClassId} << "Dynamic scan found " << resourcesFound << " resources across all stores";
		}

		return resourcesFound > 0;
	}

	bool
	Manager::onInitialize () noexcept
	{
		const auto & arguments = m_primaryServices.arguments();
		auto & settings = m_primaryServices.settings();

		m_showInformation = settings.getOrSetDefault< bool >(ResourcesShowInformationKey, DefaultResourcesShowInformation) ||
			arguments.isSwitchPresent("--show-all-infos") ||
			arguments.isSwitchPresent("--show-resources-infos");
		m_quietConversion = settings.getOrSetDefault< bool >(ResourcesQuietConversionKey, DefaultResourcesQuietConversion);
		m_useDynamicScan = settings.getOrSetDefault< bool >(ResourcesUseDynamicScanKey, DefaultResourcesUseDynamicScan);
		m_releaseLocalData = settings.getOrSetDefault< bool >(ResourcesReleaseLocalDataKey, DefaultResourcesReleaseLocalData);

		/* NOTE: A settings value (trust boundary): a non-finite or out-of-range delay falls back to the default. */
		{
			constexpr auto MaximumDelay{3600.0F};

			auto delay = settings.getOrSetDefault< float >(ResourcesLocalDataReleaseDelayKey, DefaultResourcesLocalDataReleaseDelay);

			if ( !std::isfinite(delay) || delay < 0.0F || delay > MaximumDelay )
			{
				TraceWarning{ClassId} << "The setting '" << ResourcesLocalDataReleaseDelayKey << "' must be a number of seconds in [0, " << MaximumDelay << "], " << DefaultResourcesLocalDataReleaseDelay << " is used.";

				delay = DefaultResourcesLocalDataReleaseDelay;
			}

			m_localDataReleaseDelay = std::chrono::duration_cast< std::chrono::steady_clock::duration >(std::chrono::duration< float >{delay});
		}

		/* NOTE: Initialize the store service. */
		{
			const std::scoped_lock lock{m_localStoresAccess};

			if ( m_useDynamicScan )
			{
				/* Dynamic scan mode: Scan directories to discover resources automatically. */
				TraceDebug{ClassId} << "Using dynamic resource scanning ...";

				if ( !this->scanResourceDirectories() )
				{
					TraceWarning{ClassId} << "No local resources available from dynamic scan !";
				}
			}
			else
			{
				/* Index mode: Use pre-generated JSON indexes for faster loading. */
				TraceDebug{ClassId} << "Using JSON-based resource indexing ...";

				if ( !this->readResourceIndexes() )
				{
					TraceWarning{ClassId} << "No local resources available from indexes !";
				}
			}

			/* A peer's resources this engine lacks join the stores BEFORE the containers capture them. */
			this->connectPeer();

			m_containers.emplace(typeid(Animations::SkeletonResource), std::make_unique< Skeletons >("Skeleton manager", m_primaryServices, *this, this->getLocalStore("Animations")));
			m_containers.emplace(typeid(Animations::AnimationClipResource), std::make_unique< AnimationClips >("Animation clip manager", m_primaryServices, *this, this->getLocalStore("Animations")));
			m_containers.emplace(typeid(Audio::SoundResource), std::make_unique< Sounds >("Sound manager", m_primaryServices, *this, this->getLocalStore("Sounds")));
			m_containers.emplace(typeid(Audio::MusicResource), std::make_unique< Musics >("Music manager", m_primaryServices, *this, this->getLocalStore("Musics")));
			m_containers.emplace(typeid(Audio::PlaylistResource), std::make_unique< Playlists >("Playlist manager", m_primaryServices, *this, this->getLocalStore("MusicPlaylists")));
			m_containers.emplace(typeid(Audio::SoundfontResource), std::make_unique< Soundfonts >("Soundfont manager", m_primaryServices, *this, this->getLocalStore("SoundBanks")));
			m_containers.emplace(typeid(Graphics::FontResource), std::make_unique< Fonts >("Font manager", m_primaryServices, *this, this->getLocalStore("Fonts")));
			m_containers.emplace(typeid(Graphics::ImageResource), std::make_unique< Images >("Image manager", m_primaryServices, *this, this->getLocalStore("Images")));
			m_containers.emplace(typeid(Graphics::CompressedImageResource), std::make_unique< CompressedImages >("Compressed image manager", m_primaryServices, *this, this->getLocalStore("Images")));
			m_containers.emplace(typeid(Graphics::CubemapResource), std::make_unique< Cubemaps >("Cubemap manager", m_primaryServices, *this, this->getLocalStore("Cubemaps")));
			m_containers.emplace(typeid(Graphics::MovieResource), std::make_unique< Movies >("Movie manager", m_primaryServices, *this, this->getLocalStore("Movies")));
			m_containers.emplace(typeid(Graphics::TextureResource::Texture1D), std::make_unique< Texture1Ds >("Texture 1D manager", m_primaryServices, *this, this->getLocalStore("Images")));
			m_containers.emplace(typeid(Graphics::TextureResource::Texture2D), std::make_unique< Texture2Ds >("Texture 2D manager", m_primaryServices, *this, this->getLocalStore("Images")));
			m_containers.emplace(typeid(Graphics::TextureResource::Texture3D), std::make_unique< Texture3Ds >("Texture 3D manager", m_primaryServices, *this, this->getLocalStore("Images")));
			m_containers.emplace(typeid(Graphics::TextureResource::TextureCubemap), std::make_unique< TextureCubemaps >("Texture cubemap manager", m_primaryServices, *this, this->getLocalStore("Cubemaps")));
			m_containers.emplace(typeid(Graphics::TextureResource::AnimatedTexture2D), std::make_unique< AnimatedTexture2Ds >("Animated texture 2D manager", m_primaryServices, *this, this->getLocalStore("Movies")));
			m_containers.emplace(typeid(Graphics::CubemapMovieResource), std::make_unique< CubemapMovies >("Cubemap movie manager", m_primaryServices, *this, this->getLocalStore("CubemapMovies")));
			m_containers.emplace(typeid(Graphics::TextureResource::AnimatedTextureCubemap), std::make_unique< AnimatedTextureCubemaps >("Animated texture cubemap manager", m_primaryServices, *this, this->getLocalStore("CubemapMovies")));
			m_containers.emplace(typeid(Graphics::Geometry::VertexResource), std::make_unique< VertexGeometries >("Geometry manager", m_primaryServices, *this, this->getLocalStore("Geometries")));
			m_containers.emplace(typeid(Graphics::Geometry::IndexedVertexResource), std::make_unique< IndexedVertexGeometries >("Indexed geometry manager", m_primaryServices, *this, this->getLocalStore("Geometries")));
			m_containers.emplace(typeid(Graphics::Geometry::RawVertexResource), std::make_unique< RawVertexGeometries >("Raw geometry manager", m_primaryServices, *this, this->getLocalStore("Geometries")));
			m_containers.emplace(typeid(Graphics::Geometry::RawIndexedVertexResource), std::make_unique< RawIndexedVertexGeometries >("Raw indexed geometry manager", m_primaryServices, *this, this->getLocalStore("Geometries")));
			m_containers.emplace(typeid(Graphics::Geometry::PulledVertexResource), std::make_unique< PulledVertexGeometries >("Pulled vertex geometry manager", m_primaryServices, *this, this->getLocalStore("Geometries")));
			m_containers.emplace(typeid(Graphics::Geometry::VertexGridResource), std::make_unique< VertexGridGeometries >("Grid geometry manager", m_primaryServices, *this, this->getLocalStore("Geometries")));
			m_containers.emplace(typeid(Graphics::Material::StandardResource), std::make_unique< StandardMaterials >("Standard material manager", m_primaryServices, *this, this->getLocalStore("Materials")));
			m_containers.emplace(typeid(Graphics::Material::BeamResource), std::make_unique< BeamMaterials >("Beam material manager", m_primaryServices, *this, this->getLocalStore("Materials")));
			m_containers.emplace(typeid(Graphics::Material::PathResource), std::make_unique< PathMaterials >("Path material manager", m_primaryServices, *this, this->getLocalStore("Materials")));
			m_containers.emplace(typeid(Graphics::Renderable::MeshResource), std::make_unique< SimpleMeshes >("Simple mesh manager", m_primaryServices, *this, this->getLocalStore("Meshes")));
			m_containers.emplace(typeid(Graphics::Renderable::MultiLayerMeshResource), std::make_unique< Meshes >("Mesh manager", m_primaryServices, *this, this->getLocalStore("Meshes")));
			m_containers.emplace(typeid(Graphics::Renderable::SpriteResource), std::make_unique< Sprites >("Sprite manager", m_primaryServices, *this, this->getLocalStore("Sprites")));
			m_containers.emplace(typeid(Graphics::Renderable::SkyBoxResource), std::make_unique< SkyBoxes >("Skybox manager", m_primaryServices, *this, this->getLocalStore("Backgrounds")));
			m_containers.emplace(typeid(Graphics::Renderable::DynamicSkyResource), std::make_unique< DynamicSkies >("Dynamic sky manager", m_primaryServices, *this, this->getLocalStore("Backgrounds")));
			m_containers.emplace(typeid(Graphics::Renderable::BasicGroundResource), std::make_unique< BasicGrounds >("Basic ground manager", m_primaryServices, *this, this->getLocalStore("Grounds")));
			m_containers.emplace(typeid(Graphics::Renderable::TerrainResource), std::make_unique< Terrains >("Terrain manager", m_primaryServices, *this, this->getLocalStore("Grounds")));
			m_containers.emplace(typeid(Graphics::Renderable::BasicSeaResource), std::make_unique< BasicSeas >("Basic sea manager", m_primaryServices, *this, this->getLocalStore("Seas")));
			/* Procedural only today (the shapes are grown, never read from disk), filed under the store a
			 * future imported cloud (VDB) would live in. */
			m_containers.emplace(typeid(Graphics::CloudShapeResource), std::make_unique< CloudShapes >("Cloud shape manager", m_primaryServices, *this, this->getLocalStore("CloudShapes")));
			m_containers.emplace(typeid(Scenes::DefinitionResource), std::make_unique< SceneDefinitions >("Scene definition manager", m_primaryServices, *this, this->getLocalStore("Scenes")));
		}

		/* NOTE: Transfers flags. */
		ResourceTrait::s_showInformation = m_showInformation;
		ResourceTrait::s_quietConversion = m_quietConversion;

		/* NOTE: Initialize every resource manager. */
		for ( const auto & resourceContainer : m_containers | std::views::values )
		{
			resourceContainer->setVerbosity(m_showInformation);

			if ( resourceContainer->initialize() )
			{
				TraceSuccess{ClassId} << resourceContainer->name() << " service up !";
			}
			else
			{
				TraceError{ClassId} << resourceContainer->name() << " service failed to execute !";
			}
		}

		this->startSharingServer();

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
		/* The sharing server reads the stores, a peer fetch writes into the data stores: both stop first. */
		m_sharingServer.reset();
		m_peerStore.reset();

		/* A release pass running on a pool worker uses the containers: wait for it. */
		m_releaseLocalData = false;

		while ( m_localDataReleasePassRunning )
		{
			std::this_thread::sleep_for(std::chrono::milliseconds{1});
		}

		/* Terminate primary services. */
		for ( const auto & resourceContainer : m_containers | std::views::values )
		{
			if ( resourceContainer->terminate() )
			{
				TraceSuccess{ClassId} << resourceContainer->name() << " primary service terminated gracefully!";
			}
			else
			{
				TraceError{ClassId} << resourceContainer->name() << " primary service failed to terminate properly!";
			}
		}

		m_containers.clear();

		return true;
	}

	bool
	Manager::parseStores (const FileSystem & fileSystem, const Json::Value & storesObject, bool verbose) noexcept
	{
		size_t resourcesRegistered = 0;

		for ( auto storeIt = storesObject.begin(); storeIt != storesObject.end(); ++storeIt )
		{
			const auto storeName = storeIt.name();

			/* Checks if the store is a JSON array, ie : "Meshes":[{},{},...] */
			if ( !storeIt->isArray() )
			{
				TraceError{ClassId} << "Store '" << storeName << "' isn't a JSON array !";

				continue;
			}

			/* Checks if we have to create the store or to complete it. */
			if ( !m_localStores.contains(storeName) )
			{
				m_localStores[storeName] = std::make_shared< std::unordered_map< std::string, BaseInformation > >();

				if ( verbose )
				{
					TraceInfo{ClassId} << "Initializing '" << storeName << "' store...";
				}
			}

			const auto & store = m_localStores[storeName];

			/* Crawling in resource definition. */
			for ( const auto & resourceDefinition : *storeIt )
			{
				/* Checks the data source to load it. */
				BaseInformation baseInformation;

				if ( !baseInformation.parse(fileSystem, resourceDefinition) )
				{
					TraceError{ClassId} <<
						"Invalid resource in '" << storeName << "' store ! "
						"Skipping ...";

					continue;
				}

				/* Resource name starting with '+' is reserved by the engine. */
				if ( baseInformation.name().starts_with('+') )
				{
					TraceError{ClassId} <<
						"Resource name starting with '+' is reserved by the engine ! "
						"Skipping '" << baseInformation.name() << "' resource ...";

					continue;
				}

				/* Warns user if we erase an old resource named the same way. */
				if ( store->contains(baseInformation.name()) )
				{
					TraceWarning{ClassId} << "'" << baseInformation.name() << "' already exists in '" << storeName << "' store. Skipping ...";

					continue;
				}

				/* Adds resource to the store. */
				store->emplace(baseInformation.name(), baseInformation);

				resourcesRegistered++;

				if ( verbose )
				{
					TraceInfo{ClassId} << "Resource '" << baseInformation.name() << "' added to store '" << storeName << "'.";
				}
			}
		}

		return resourcesRegistered > 0;
	}

	std::vector< std::string >
	Manager::getResourcesIndexFiles (const FileSystem & fileSystem) noexcept
	{
		std::vector< std::string > indexes{};

		const std::regex indexMatchRule("ResourcesIndex.([0-9]{3}).json",std::regex_constants::ECMAScript);

		/* NOTE: For each data directory pointed by the file system, we will look for resource index files. */
		for ( auto dataStoreDirectory : fileSystem.dataDirectories() )
		{
			dataStoreDirectory.append(DataStores);

			if ( !IO::directoryExists(dataStoreDirectory) )
			{
				/* No "data-stores/" in this data directory. */
				continue;
			}

			/* NOTE: never a range-for over directory_iterator: its operator++ throws (terminate under -fno-exceptions). */
			const auto walked = IO::forEachDirectoryEntry(dataStoreDirectory, false, [&indexes, &indexMatchRule] (const std::filesystem::directory_entry & entry) {
				std::error_code entryError;

				if ( !entry.is_regular_file(entryError) )
				{
					/* This entry is not a file. */
					return true;
				}

				const auto filepath = entry.path().string();

				if ( !std::regex_search(filepath, indexMatchRule) )
				{
					/* No resource index file in this "data-stores/" directory. */
					TraceWarning{ClassId} << "Directory '" << entry.path() << "' do not contains any resource index file !";

					return true;
				}

				indexes.emplace_back(filepath);

				return true;
			});

			if ( !walked )
			{
				TraceWarning{ClassId} << "The data-stores directory " << dataStoreDirectory << " could not be fully scanned (see the IO error above).";
			}
		}

		return indexes;
	}

	bool
	Manager::isJSONData (const std::string & buffer) noexcept
	{
		return buffer.find('{') != std::string::npos;
	}

	std::ostream &
	operator<< (std::ostream & out, const Manager & obj)
	{
		out << "Resources stores :" "\n";

		for ( const auto & [name, store] : obj.m_localStores )
		{
			out << " - " << name << " (" << store->size() << " resources)" << '\n';
		}

		return out;
	}

	std::string
	to_string (const Manager & obj) noexcept
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}

	std::string
	Manager::buildSharingIndex () const noexcept
	{
		const auto & dataDirectories = m_primaryServices.fileSystem().dataDirectories();

		/* A local file is published by its path under some data directory's data-stores/ (the peer asks for it by
		 * that path), never by its absolute path on this machine. */
		const auto storeRelative = [&dataDirectories] (const std::filesystem::path & filepath) -> std::string {
			for ( const auto & dataDirectory : dataDirectories )
			{
				const auto relative = filepath.lexically_relative(dataDirectory / DataStores);

				if ( relative.empty() || *relative.begin() == std::filesystem::path{".."} )
				{
					continue;
				}

				auto text = IO::toU8String(relative);

				if constexpr ( IsWindows )
				{
					text = String::replace('\\', '/', text);
				}

				return text;
			}

			return {};
		};

		Json::Value stores{Json::objectValue};

		{
			const std::scoped_lock lock{m_localStoresAccess};

			std::vector< std::string > storeNames;
			storeNames.reserve(m_localStores.size());

			for ( const auto & storeName : m_localStores | std::views::keys )
			{
				storeNames.emplace_back(storeName);
			}

			std::ranges::sort(storeNames);

			for ( const auto & storeName : storeNames )
			{
				const auto & store = *m_localStores.at(storeName);

				std::vector< const BaseInformation * > entries;
				entries.reserve(store.size());

				for ( const auto & information : store | std::views::values )
				{
					entries.emplace_back(&information);
				}

				std::ranges::sort(entries, {}, [] (const BaseInformation * information) -> const std::string & {
					return information->name();
				});

				Json::Value storeArray{Json::arrayValue};

				for ( const auto * information : entries )
				{
					Json::Value entry{Json::objectValue};
					entry[BaseInformation::NameKey] = information->name();

					switch ( information->sourceType() )
					{
						case SourceType::LocalData :
						{
							const auto filepath = information->dataString();

							if ( !filepath )
							{
								continue;
							}

							const auto path = IO::u8path(*filepath);
							const auto relative = storeRelative(path);

							if ( relative.empty() )
							{
								continue;
							}

							std::error_code errorCode;
							const auto size = std::filesystem::file_size(path, errorCode);

							entry["Path"] = relative;
							entry["Size"] = static_cast< Json::UInt64 >(errorCode ? 0 : size);
						}
							break;

						case SourceType::ExternalData :
							entry[BaseInformation::SourceKey] = ExternalDataString;
							entry[BaseInformation::DataKey] = information->data();
							break;

						case SourceType::DirectData :
							entry[BaseInformation::SourceKey] = DirectDataString;
							entry[BaseInformation::DataKey] = information->data();
							break;

						case SourceType::Undefined :
							continue;
					}

					storeArray.append(std::move(entry));
				}

				stores[storeName] = std::move(storeArray);
			}
		}

		Json::Value root{Json::objectValue};
		root["FormatVersion"] = "1.0.0";
		root[StoresKey] = std::move(stores);

		return FastJSON::stringify(root);
	}

	size_t
	Manager::mergePeerIndex (const Json::Value & index) noexcept
	{
		if ( m_peerStore == nullptr || !index.isObject() || !index.isMember(StoresKey) || !index[StoresKey].isObject() )
		{
			return 0;
		}

		const auto & remoteStores = index[StoresKey];
		Json::Value missing{Json::objectValue};

		for ( const auto & storeName : remoteStores.getMemberNames() )
		{
			const auto & remoteStore = remoteStores[storeName];

			if ( !remoteStore.isArray() )
			{
				continue;
			}

			const auto localStoreIt = m_localStores.find(storeName);
			Json::Value additions{Json::arrayValue};

			for ( const auto & remoteEntry : remoteStore )
			{
				if ( !remoteEntry.isObject() )
				{
					continue;
				}

				const auto name = FastJSON::getValue< std::string >(remoteEntry, BaseInformation::NameKey);

				/* A local resource always wins: the peer only fills the gaps. */
				if ( !name || ( localStoreIt != m_localStores.end() && localStoreIt->second->contains(*name) ) )
				{
					continue;
				}

				if ( const auto path = FastJSON::getValue< std::string >(remoteEntry, "Path"); path.has_value() )
				{
					Json::Value entry{Json::objectValue};
					entry[BaseInformation::NameKey] = *name;
					entry[BaseInformation::SourceKey] = ExternalDataString;
					entry[BaseInformation::DataKey] = m_peerStore->fileURL(*path);

					additions.append(std::move(entry));
				}
				else
				{
					/* An external or direct entry of the peer: as it is (BaseInformation::parse() checks it). */
					additions.append(remoteEntry);
				}
			}

			if ( !additions.empty() )
			{
				missing[storeName] = std::move(additions);
			}
		}

		if ( missing.empty() )
		{
			return 0;
		}

		size_t count = 0;

		for ( const auto & storeName : missing.getMemberNames() )
		{
			count += missing[storeName].size();
		}

		static_cast< void >(this->parseStores(m_primaryServices.fileSystem(), missing, m_showInformation));

		return count;
	}

	void
	Manager::connectPeer () noexcept
	{
		auto & settings = m_primaryServices.settings();

		const auto peerURL = settings.getOrSetDefault< std::string >(ResourcesPeerURLKey, DefaultResourcesPeerURL);
		auto peerBearerToken = settings.getOrSetDefault< std::string >(ResourcesPeerBearerTokenKey, DefaultResourcesPeerBearerToken);

		if ( peerURL.empty() )
		{
			return;
		}

		const Network::URI baseURL{peerURL};

		/* A settings value (trust boundary): only an http(s) URL with a host. */
		if ( const auto scheme = String::toLower(baseURL.scheme()); ( scheme != "http" && scheme != "https" ) || baseURL.uriDomain().hostname().name().empty() )
		{
			TraceError{ClassId} << "The setting '" << ResourcesPeerURLKey << "' must be an http(s)://host:port URL, '" << peerURL << "' is ignored.";

			return;
		}

		if ( !m_primaryServices.netManager().registerPeer(baseURL, peerBearerToken) )
		{
			return;
		}

		m_peerStore = std::make_unique< PeerStore >(m_primaryServices.fileSystem(), m_primaryServices.threadPool(), baseURL, std::move(peerBearerToken));

		const auto start = std::chrono::steady_clock::now();
		const auto index = m_peerStore->fetchIndex();

		if ( !index )
		{
			TraceWarning{ClassId} << "The peer " << m_peerStore->baseURL() << " is not reachable: no resource from it this run (its files can still be fetched later).";

			return;
		}

		const auto added = this->mergePeerIndex(*index);
		const auto milliseconds = std::chrono::duration_cast< std::chrono::milliseconds >(std::chrono::steady_clock::now() - start).count();

		TraceSuccess{ClassId} << added << " resource(s) of the peer " << m_peerStore->baseURL() << " added to the stores (downloaded on first use), index read in " << milliseconds << " ms.";
	}

	void
	Manager::startSharingServer () noexcept
	{
		auto & settings = m_primaryServices.settings();

		/* NOTE: like the console and MCP keys, written on first run even when the server stays off. */
		const auto enabled = settings.getOrSetDefault< bool >(ResourcesSharingEnabledKey, DefaultResourcesSharingEnabled);
		const auto address = settings.getOrSetDefault< std::string >(ResourcesSharingAddressKey, DefaultResourcesSharingAddress);
		const auto port = settings.getOrSetDefault< uint16_t >(ResourcesSharingPortKey, DefaultResourcesSharingPort);
		auto bearerToken = settings.getOrSetDefault< std::string >(ResourcesSharingBearerTokenKey, DefaultResourcesSharingBearerToken);

		if ( !enabled )
		{
			return;
		}

		auto server = std::make_unique< SharingServer >(m_primaryServices.fileSystem(), m_primaryServices.threadPool(), [this] () {
			return this->buildSharingIndex();
		});

		if ( server->start(address, port, std::move(bearerToken)) )
		{
			m_sharingServer = std::move(server);
		}
	}
}
