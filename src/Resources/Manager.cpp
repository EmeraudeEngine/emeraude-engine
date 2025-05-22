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
#include <iostream>

/* Local inclusions. */
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"

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

namespace EmEn::Resources
{
	using namespace EmEn::Libs;

	Manager * Manager::s_instance{nullptr};

	Manager::Manager (PrimaryServices & primaryServices) noexcept
		: ServiceInterface{ClassId},
		m_primaryServices{primaryServices}
	{
		if ( s_instance != nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

			std::terminate();
		}

		s_instance = this;

		m_containers.emplace(typeid(Audio::SoundResource), std::make_unique< Sounds >(m_primaryServices, m_stores, "Sound manager", "Sounds"));
		m_containers.emplace(typeid(Audio::MusicResource), std::make_unique< Musics >(m_primaryServices, m_stores, "Music manager", "Musics"));
		m_containers.emplace(typeid(Graphics::FontResource), std::make_unique< Fonts >(m_primaryServices, m_stores, "Font manager", "Fonts"));
		m_containers.emplace(typeid(Graphics::ImageResource), std::make_unique< Images >(m_primaryServices, m_stores, "Image manager", "Images"));
		m_containers.emplace(typeid(Graphics::CubemapResource), std::make_unique< Cubemaps >(m_primaryServices, m_stores, "Cubemap manager", "Cubemaps"));
		m_containers.emplace(typeid(Graphics::MovieResource), std::make_unique< Movies >(m_primaryServices, m_stores, "Movie manager", "Movies"));
		m_containers.emplace(typeid(Graphics::TextureResource::Texture1D), std::make_unique< Texture1Ds >(m_primaryServices, m_stores, "Texture 1D manager", "Images"));
		m_containers.emplace(typeid(Graphics::TextureResource::Texture2D), std::make_unique< Texture2Ds >(m_primaryServices, m_stores, "Texture 2D manager", "Images"));
		m_containers.emplace(typeid(Graphics::TextureResource::Texture3D), std::make_unique< Texture3Ds >(m_primaryServices, m_stores, "Texture 3D manager", "Images"));
		m_containers.emplace(typeid(Graphics::TextureResource::TextureCubemap), std::make_unique< TextureCubemaps >(m_primaryServices, m_stores, "Texture cubemap manager", "Cubemaps"));
		m_containers.emplace(typeid(Graphics::TextureResource::AnimatedTexture2D), std::make_unique< AnimatedTexture2Ds >(m_primaryServices, m_stores, "Animated texture 2D manager", "Movies"));
		m_containers.emplace(typeid(Graphics::Geometry::VertexResource), std::make_unique< VertexGeometries >(m_primaryServices, m_stores, "Geometry manager", "Geometries"));
		m_containers.emplace(typeid(Graphics::Geometry::IndexedVertexResource), std::make_unique< IndexedVertexGeometries >(m_primaryServices, m_stores, "Indexed geometry manager", "Geometries"));
		m_containers.emplace(typeid(Graphics::Geometry::VertexGridResource), std::make_unique< VertexGridGeometries >(m_primaryServices, m_stores, "Grid geometry manager", "Geometries"));
		m_containers.emplace(typeid(Graphics::Geometry::AdaptiveVertexGridResource), std::make_unique< AdaptiveVertexGridGeometries >(m_primaryServices, m_stores, "Adaptive grid geometry manager", "Geometries"));
		m_containers.emplace(typeid(Graphics::Material::BasicResource), std::make_unique< BasicMaterials >(m_primaryServices, m_stores, "Basic material manager", "Materials"));
		m_containers.emplace(typeid(Graphics::Material::StandardResource), std::make_unique< StandardMaterials >(m_primaryServices, m_stores, "Standard material manager", "Materials"));
		m_containers.emplace(typeid(Graphics::Renderable::SimpleMeshResource), std::make_unique< SimpleMeshes >(m_primaryServices, m_stores, "Simple mesh manager", "Meshes"));
		m_containers.emplace(typeid(Graphics::Renderable::MeshResource), std::make_unique< Meshes >(m_primaryServices, m_stores, "Mesh manager", "Meshes"));
		m_containers.emplace(typeid(Graphics::Renderable::SpriteResource), std::make_unique< Sprites >(m_primaryServices, m_stores, "Sprite manager", "Sprites"));
		m_containers.emplace(typeid(Graphics::Renderable::SkyBoxResource), std::make_unique< SkyBoxes >(m_primaryServices, m_stores, "Skybox manager", "Backgrounds"));
		m_containers.emplace(typeid(Graphics::Renderable::DynamicSkyResource), std::make_unique< DynamicSkies >(m_primaryServices, m_stores, "Dynamic sky manager", "Backgrounds"));
		m_containers.emplace(typeid(Graphics::Renderable::BasicFloorResource), std::make_unique< BasicFloors >(m_primaryServices, m_stores, "BasicFloor manager", "SceneAreas"));
		m_containers.emplace(typeid(Graphics::Renderable::TerrainResource), std::make_unique< Terrains >(m_primaryServices, m_stores, "Terrain manager", "SceneAreas"));
		m_containers.emplace(typeid(Graphics::Renderable::WaterLevelResource), std::make_unique< WaterLevels >(m_primaryServices, m_stores, "Water level manager", "SeaLevels"));
		m_containers.emplace(typeid(Scenes::DefinitionResource), std::make_unique< SceneDefinitions >(m_primaryServices, m_stores, "Scene definition manager", "Scenes"));
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

	void
	Manager::setVerbosity (bool state) noexcept
	{
		m_flags[VerbosityEnabled] = state;

		ResourceTrait::s_verboseEnabled = state;

		for ( const auto & resourceContainer : m_containers | std::views::values )
		{
			resourceContainer->setVerbosity(state);
		}
	}

	bool
	Manager::onInitialize () noexcept
	{
		m_flags[VerbosityEnabled] = m_primaryServices.settings().get< bool >(ResourcesShowInformationKey, DefaultResourcesShowInformation);
		m_flags[DownloadingAllowed] = m_primaryServices.settings().get< bool >(ResourcesDownloadEnabledKey, DefaultResourcesDownloadEnabled);
		m_flags[QuietConversion] = m_primaryServices.settings().get< bool >(ResourcesQuietConversionKey, DefaultResourcesQuietConversion);

		/* NOTE: Initialize the store service. */
		if ( m_stores.initialize(m_primaryServices.fileSystem(), m_flags[VerbosityEnabled]) )
		{
			if ( m_flags[VerbosityEnabled] )
			{
				TraceInfo{ClassId} << m_stores;
			}
		}
		else
		{
			TraceInfo{ClassId} << "There is no resource store available.";
		}

		/* NOTE: Transfers flags. */
		ResourceTrait::s_verboseEnabled = m_flags[VerbosityEnabled];
		ResourceTrait::s_quietConversion = m_flags[QuietConversion];

		/* NOTE: Initialize every resource manager. */
		for ( const auto & resourceContainer : m_containers | std::views::values )
		{
			resourceContainer->setVerbosity(m_flags[VerbosityEnabled]);

			if ( resourceContainer->initialize() )
			{
				TraceSuccess{ClassId} << resourceContainer->name() << " service up !";
			}
			else
			{
				TraceError{ClassId} << resourceContainer->name() << " service failed to execute !";
			}
		}

		m_flags[Initialized] = true;

		return true;
	}

	bool
	Manager::onTerminate () noexcept
	{
		m_flags[Initialized] = false;

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
}
