/*
 * src/Resources/Manager.cpp
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

/* STL inclusions. */
#include <ranges>
#include <regex>

/* Local inclusions. */
#include "Libs/FastJSON.hpp"
#include "Audio/MusicResource.hpp"
#include "Audio/SoundResource.hpp"
#include "Graphics/CubemapResource.hpp"
#include "Graphics/FontResource.hpp"
#include "Graphics/Geometry/AdaptiveVertexGridResource.hpp"
#include "Graphics/Geometry/IndexedVertexResource.hpp"
#include "Graphics/Geometry/VertexGridResource.hpp"
#include "Graphics/Geometry/VertexResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/Material/BasicResource.hpp"
#include "Graphics/Material/StandardResource.hpp"
#include "Graphics/MovieResource.hpp"
#include "Graphics/Renderable/BasicFloorResource.hpp"
#include "Graphics/Renderable/DynamicSkyResource.hpp"
#include "Graphics/Renderable/MeshResource.hpp"
#include "Graphics/Renderable/SimpleMeshResource.hpp"
#include "Graphics/Renderable/SkyBoxResource.hpp"
#include "Graphics/Renderable/SpriteResource.hpp"
#include "Graphics/Renderable/TerrainResource.hpp"
#include "Graphics/Renderable/WaterLevelResource.hpp"
#include "Graphics/TextureResource/AnimatedTexture2D.hpp"
#include "Graphics/TextureResource/Texture1D.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Graphics/TextureResource/Texture3D.hpp"
#include "Graphics/TextureResource/TextureCubemap.hpp"
#include "Scenes/DefinitionResource.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"

namespace EmEn::Resources
{
	using namespace Libs;

	void
	Manager::setVerbosity (bool state) noexcept
	{
		m_verbosityEnabled = state;

		ResourceTrait::s_verboseEnabled = state;

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

		const std::lock_guard< std::mutex > lock{m_localStoresAccess};

		return this->parseStores(m_primaryServices.fileSystem(), stores, m_verbosityEnabled);
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

			const auto & root = rootCheck.value();

			/* 3. Register every stores */
			if ( !root.isMember(StoresKey) )
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

			if ( this->parseStores(fileSystem, storesObject, m_verbosityEnabled) )
			{
				TraceSuccess{ClassId} << "Resource index '" << filepath << "' loaded !";
			}
		}

		return true;
	}

	bool
	Manager::onInitialize () noexcept
	{
		m_verbosityEnabled = m_primaryServices.settings().getOrSetDefault< bool >(ResourcesShowInformationKey, DefaultResourcesShowInformation);
		m_downloadingAllowed = m_primaryServices.settings().getOrSetDefault< bool >(ResourcesDownloadEnabledKey, DefaultResourcesDownloadEnabled);
		m_quietConversion = m_primaryServices.settings().getOrSetDefault< bool >(ResourcesQuietConversionKey, DefaultResourcesQuietConversion);

		/* NOTE: Initialize the store service. */
		{
			const std::lock_guard< std::mutex > lock{m_localStoresAccess};

			if ( !this->readResourceIndexes() )
			{
				TraceWarning{ClassId} << "No local resources available !";
			}

			m_containers.emplace(typeid(Audio::SoundResource), std::make_unique< Sounds >("Sound manager", m_primaryServices, *this, this->getLocalStore("Sounds")));
			m_containers.emplace(typeid(Audio::MusicResource), std::make_unique< Musics >("Music manager", m_primaryServices, *this, this->getLocalStore("Musics")));
			m_containers.emplace(typeid(Graphics::FontResource), std::make_unique< Fonts >("Font manager", m_primaryServices, *this, this->getLocalStore("Fonts")));
			m_containers.emplace(typeid(Graphics::ImageResource), std::make_unique< Images >("Image manager", m_primaryServices, *this, this->getLocalStore("Images")));
			m_containers.emplace(typeid(Graphics::CubemapResource), std::make_unique< Cubemaps >("Cubemap manager", m_primaryServices, *this, this->getLocalStore("Cubemaps")));
			m_containers.emplace(typeid(Graphics::MovieResource), std::make_unique< Movies >("Movie manager", m_primaryServices, *this, this->getLocalStore("Movies")));
			m_containers.emplace(typeid(Graphics::TextureResource::Texture1D), std::make_unique< Texture1Ds >("Texture 1D manager", m_primaryServices, *this, this->getLocalStore("Images")));
			m_containers.emplace(typeid(Graphics::TextureResource::Texture2D), std::make_unique< Texture2Ds >("Texture 2D manager", m_primaryServices, *this, this->getLocalStore("Images")));
			m_containers.emplace(typeid(Graphics::TextureResource::Texture3D), std::make_unique< Texture3Ds >("Texture 3D manager", m_primaryServices, *this, this->getLocalStore("Images")));
			m_containers.emplace(typeid(Graphics::TextureResource::TextureCubemap), std::make_unique< TextureCubemaps >("Texture cubemap manager", m_primaryServices, *this, this->getLocalStore("Cubemaps")));
			m_containers.emplace(typeid(Graphics::TextureResource::AnimatedTexture2D), std::make_unique< AnimatedTexture2Ds >("Animated texture 2D manager", m_primaryServices, *this, this->getLocalStore("Movies")));
			m_containers.emplace(typeid(Graphics::Geometry::VertexResource), std::make_unique< VertexGeometries >("Geometry manager", m_primaryServices, *this, this->getLocalStore("Geometries")));
			m_containers.emplace(typeid(Graphics::Geometry::IndexedVertexResource), std::make_unique< IndexedVertexGeometries >("Indexed geometry manager", m_primaryServices, *this, this->getLocalStore("Geometries")));
			m_containers.emplace(typeid(Graphics::Geometry::VertexGridResource), std::make_unique< VertexGridGeometries >("Grid geometry manager", m_primaryServices, *this, this->getLocalStore("Geometries")));
			m_containers.emplace(typeid(Graphics::Geometry::AdaptiveVertexGridResource), std::make_unique< AdaptiveVertexGridGeometries >("Adaptive grid geometry manager", m_primaryServices, *this, this->getLocalStore("Geometries")));
			m_containers.emplace(typeid(Graphics::Material::BasicResource), std::make_unique< BasicMaterials >("Basic material manager", m_primaryServices, *this, this->getLocalStore("Materials")));
			m_containers.emplace(typeid(Graphics::Material::StandardResource), std::make_unique< StandardMaterials >("Standard material manager", m_primaryServices, *this, this->getLocalStore("Materials")));
			m_containers.emplace(typeid(Graphics::Renderable::SimpleMeshResource), std::make_unique< SimpleMeshes >("Simple mesh manager", m_primaryServices, *this, this->getLocalStore("Meshes")));
			m_containers.emplace(typeid(Graphics::Renderable::MeshResource), std::make_unique< Meshes >("Mesh manager", m_primaryServices, *this, this->getLocalStore("Meshes")));
			m_containers.emplace(typeid(Graphics::Renderable::SpriteResource), std::make_unique< Sprites >("Sprite manager", m_primaryServices, *this, this->getLocalStore("Sprites")));
			m_containers.emplace(typeid(Graphics::Renderable::SkyBoxResource), std::make_unique< SkyBoxes >("Skybox manager", m_primaryServices, *this, this->getLocalStore("Backgrounds")));
			m_containers.emplace(typeid(Graphics::Renderable::DynamicSkyResource), std::make_unique< DynamicSkies >("Dynamic sky manager", m_primaryServices, *this, this->getLocalStore("Backgrounds")));
			m_containers.emplace(typeid(Graphics::Renderable::BasicFloorResource), std::make_unique< BasicFloors >("BasicFloor manager", m_primaryServices, *this, this->getLocalStore("SceneAreas")));
			m_containers.emplace(typeid(Graphics::Renderable::TerrainResource), std::make_unique< Terrains >("Terrain manager", m_primaryServices, *this, this->getLocalStore("SceneAreas")));
			m_containers.emplace(typeid(Graphics::Renderable::WaterLevelResource), std::make_unique< WaterLevels >("Water level manager", m_primaryServices, *this, this->getLocalStore("SeaLevels")));
			m_containers.emplace(typeid(Scenes::DefinitionResource), std::make_unique< SceneDefinitions >("Scene definition manager", m_primaryServices, *this, this->getLocalStore("Scenes")));
		}

		/* NOTE: Transfers flags. */
		ResourceTrait::s_verboseEnabled = m_verbosityEnabled;
		ResourceTrait::s_quietConversion = m_quietConversion;

		/* NOTE: Initialize every resource manager. */
		for ( const auto & resourceContainer : m_containers | std::views::values )
		{
			resourceContainer->setVerbosity(m_verbosityEnabled);

			if ( resourceContainer->initialize() )
			{
				TraceSuccess{ClassId} << resourceContainer->name() << " service up !";
			}
			else
			{
				TraceError{ClassId} << resourceContainer->name() << " service failed to execute !";
			}
		}

		m_serviceInitialized = true;

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
		m_serviceInitialized = false;

		/* Terminate primary services. */
		for ( const auto & resourceContainer : m_containers | std::views::values )
		{
			if ( resourceContainer->terminate() )
			{
				TraceSuccess{ClassId} << resourceContainer->name() << " primary service terminated gracefully !";
			}
			else
			{
				TraceError{ClassId} << resourceContainer->name() << " primary service failed to terminate properly !";
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
			auto storeName = storeIt.name();

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

			for ( const auto & entry : std::filesystem::directory_iterator(dataStoreDirectory) )
			{
				if ( !is_regular_file(entry.path()) )
				{
					/* This entry is not a file. */
					continue;
				}

				const auto filepath = entry.path().string();

				if ( !std::regex_search(filepath, indexMatchRule) )
				{
					/* No resource index file in this "data-stores/" directory. */
					TraceWarning{ClassId} << "Directory '" << entry << "' do not contains any resource index file !";

					continue;
				}

				indexes.emplace_back(filepath);
			}
		}

		return indexes;
	}

	bool
	Manager::isJSONData (const std::string & buffer) noexcept
	{
		return buffer.find('{') != std::string::npos;
	}
}
