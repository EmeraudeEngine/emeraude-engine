/*
 * src/Scenes/BindlessTextureSet.cpp
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

#include "BindlessTextureSet.hpp"

/* Local inclusions. */
#include "Vulkan/TextureInterface.hpp"

namespace EmEn::Scenes
{
	using namespace EmEn::Graphics;

	uint32_t
	BindlessTextureSet::registerInBucket (std::vector< Entry > & entries, std::unordered_map< const Vulkan::TextureInterface *, uint32_t > & lookup, std::queue< uint32_t > & freeIndices, uint32_t & nextIndex, uint32_t maxIndex, const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept
	{
		if ( texture == nullptr )
		{
			return UINT32_MAX;
		}

		const auto * texturePtr = texture.get();

		/* Deduplicate: the same texture instance keeps its slot. */
		if ( const auto it = lookup.find(texturePtr); it != lookup.end() )
		{
			return it->second;
		}

		uint32_t globalIndex;

		if ( !freeIndices.empty() )
		{
			globalIndex = freeIndices.front();
			freeIndices.pop();
		}
		else
		{
			if ( nextIndex >= maxIndex )
			{
				return UINT32_MAX;
			}

			globalIndex = nextIndex++;
		}

		entries.emplace_back(Entry{texture, globalIndex});
		lookup.emplace(texturePtr, globalIndex);

		return globalIndex;
	}

	void
	BindlessTextureSet::unregisterFromBucket (std::vector< Entry > & entries, std::unordered_map< const Vulkan::TextureInterface *, uint32_t > & lookup, std::queue< uint32_t > & freeIndices, const Vulkan::TextureInterface * texture) noexcept
	{
		const auto it = lookup.find(texture);

		if ( it == lookup.end() )
		{
			return;
		}

		const auto globalIndex = it->second;

		lookup.erase(it);
		freeIndices.push(globalIndex);

		for ( auto entryIt = entries.begin(); entryIt != entries.end(); ++entryIt )
		{
			if ( entryIt->globalIndex == globalIndex )
			{
				entries.erase(entryIt);

				break;
			}
		}
	}

	uint32_t
	BindlessTextureSet::registerTexture2D (const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

		return this->registerInBucket(m_textures2D, m_lookup2D, m_free2D, m_next2D, BindlessTextureManager::MaxTextures2D, texture);
	}

	uint32_t
	BindlessTextureSet::registerTextureCube (const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

		return this->registerInBucket(m_texturesCube, m_lookupCube, m_freeCube, m_nextCube, BindlessTextureManager::MaxTexturesCube, texture);
	}

	uint32_t
	BindlessTextureSet::registerTextureCubeArray (const std::shared_ptr< Vulkan::TextureInterface > & texture) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

		return this->registerInBucket(m_texturesCubeArray, m_lookupCubeArray, m_freeCubeArray, m_nextCubeArray, BindlessTextureManager::MaxTexturesCubeArray, texture);
	}

	void
	BindlessTextureSet::unregisterTexture2D (const Vulkan::TextureInterface * texture) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

		BindlessTextureSet::unregisterFromBucket(m_textures2D, m_lookup2D, m_free2D, texture);
	}

	void
	BindlessTextureSet::unregisterTextureCube (const Vulkan::TextureInterface * texture) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

		BindlessTextureSet::unregisterFromBucket(m_texturesCube, m_lookupCube, m_freeCube, texture);
	}

	void
	BindlessTextureSet::unregisterTextureCubeArray (const Vulkan::TextureInterface * texture) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

		BindlessTextureSet::unregisterFromBucket(m_texturesCubeArray, m_lookupCubeArray, m_freeCubeArray, texture);
	}

	void
	BindlessTextureSet::setEnvironmentCubemap (const std::shared_ptr< Vulkan::TextureInterface > & cubemap) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

		m_environmentCubemap = cubemap;
	}

	BindlessTextureSet::Snapshot
	BindlessTextureSet::snapshot () const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

		Snapshot snap;
		snap.textures2D = m_textures2D;
		snap.texturesCube = m_texturesCube;
		snap.texturesCubeArray = m_texturesCubeArray;
		snap.environmentCubemap = m_environmentCubemap;

		return snap;
	}

	void
	BindlessTextureSet::clear () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_access};

		m_textures2D.clear();
		m_texturesCube.clear();
		m_texturesCubeArray.clear();

		m_lookup2D.clear();
		m_lookupCube.clear();
		m_lookupCubeArray.clear();

		m_free2D = {};
		m_freeCube = {};
		m_freeCubeArray = {};

		m_next2D = BindlessTextureManager::FirstDynamicSlot;
		m_nextCube = BindlessTextureManager::FirstDynamicSlot;
		m_nextCubeArray = BindlessTextureManager::FirstDynamicSlot;

		m_environmentCubemap.reset();
	}
}