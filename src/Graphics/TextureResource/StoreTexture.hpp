/*
 * src/Graphics/TextureResource/StoreTexture.hpp
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

#pragma once

/* STL inclusions. */
#include <cstddef>
#include <memory>
#include <string>

/* Local inclusions for usages. */
#include "Resources/Container.hpp"
#include "Resources/Types.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics::TextureResource
{
	/**
	 * @brief How a texture is sampled: the properties that decide its GPU image, hence part of its identity.
	 * @note The default (linear, unflipped) is the store texture itself, under its store name.
	 */
	struct Usage final
	{
		/** @brief True for a perceptual colour (a material variable ending with "Color"), decoded sRGB → linear by
		 * the GPU; false for data (normals, roughness, masks...). */
		bool sRGB{false};
		/** @brief True for a Direct3D-style normal map (+Y down), converted to the Khronos convention at upload. */
		bool flipNormalMapY{false};
		/** @brief True for a tangent-space normal map, block-compressed as BC5 (X, Y; the shader reconstructs Z —
		 * graphics § 39, D8). Linear only. */
		bool normalMap{false};
	};

	/**
	 * @brief The number of usages a scene loader caches per source image: linear data, sRGB colour, normal map.
	 * @note A loader's textures never flip a normal map (their formats state the convention).
	 */
	constexpr size_t LoaderUsageCount{3};

	/**
	 * @brief Returns the cache slot of a usage in a scene loader (< LoaderUsageCount).
	 * @param usage The sampling properties.
	 * @return size_t
	 */
	[[nodiscard]]
	constexpr
	size_t
	loaderUsageSlot (const Usage & usage) noexcept
	{
		if ( usage.normalMap )
		{
			return 2;
		}

		return usage.sRGB ? 1 : 0;
	}

	/**
	 * @brief Returns the resource name suffix of a usage in a scene loader.
	 * @param usage The sampling properties.
	 * @return const char *
	 */
	[[nodiscard]]
	constexpr
	const char *
	loaderUsageSuffix (const Usage & usage) noexcept
	{
		if ( usage.normalMap )
		{
			return "-normal";
		}

		return usage.sRGB ? "-srgb" : "-data";
	}

	/**
	 * @brief Returns the resource name of a store texture's variant.
	 * @note The suffixes use '#', which store names (paths under the data directory) never carry.
	 * @param storeName The texture's name in the store.
	 * @param usage The sampling properties.
	 * @return std::string The store name itself for the default usage.
	 */
	[[nodiscard]]
	inline
	std::string
	variantName (const std::string & storeName, const Usage & usage) noexcept
	{
		std::string name{storeName};

		if ( usage.sRGB )
		{
			name += "#sRGB";
		}

		if ( usage.flipNormalMapY )
		{
			name += "#flipY";
		}

		if ( usage.normalMap )
		{
			name += "#normal";
		}

		return name;
	}

	/**
	 * @brief Returns a store texture as a given usage samples it.
	 * @note The colour space, the normal map flip and the normal map kind (D8, 2026-10-20) are part of a texture's
	 * identity (owner decision 2026-10-14):
	 * each usage is a distinct resource, named by variantName(), whose properties are set by its creation function
	 * BEFORE the loading starts, then never changed. Two materials sampling one image differently get two textures;
	 * the BC7 cache is keyed by the pixels, so the sRGB and linear variants share one cache entry.
	 * @note The default usage is a plain Container::getResource(). A variant is CREATED ON THE CALLING THREAD
	 * (Container::getOrCreateResourceSync()): its properties are therefore in place when this returns, which the
	 * material setters check. Its loading is the type's `load(filepath)`: the image-based types only request their
	 * source there (decoded on the thread pool, as a store load does); the movie-based ones load their movie
	 * synchronously, as their store load does on its worker.
	 * @note A variant is made from the store entry's file: an entry given as direct data or as a URL has none, and
	 * answers the default resource with an error. A name absent from the store answers as getResource() does (a
	 * warning, the default resource).
	 * @tparam texture_t A texture resource type with a `load(const std::filesystem::path &)` method.
	 * @param container A reference to the texture container.
	 * @param storeName The texture's name in the store.
	 * @param usage The sampling properties.
	 * @param asyncLoad For the default usage: true to load on the thread pool (default), false on the calling thread.
	 * @return std::shared_ptr< texture_t > The texture (possibly still loading), or the default resource.
	 * @note Thread-safe, like the container.
	 */
	template< typename texture_t >
	[[nodiscard]]
	std::shared_ptr< texture_t >
	getStoreTexture (Resources::Container< texture_t > & container, const std::string & storeName, const Usage & usage, bool asyncLoad = true) noexcept
	{
		/* NOTE: An absent name goes through getResource() too: its usual warning and the default resource. */
		if ( (!usage.sRGB && !usage.flipNormalMapY && !usage.normalMap) || storeName == Resources::Default || !container.isResourceExists(storeName) )
		{
			return container.getResource(storeName, asyncLoad);
		}

		const auto filepath = container.localFilepath(storeName);

		if ( !filepath.has_value() )
		{
			TraceError{texture_t::ClassId} <<
				"The texture '" << storeName << "' is not a file of the store, its variant '" << variantName(storeName, usage) << "' cannot be made !";

			return container.getDefaultResource();
		}

		return container.getOrCreateResourceSync(variantName(storeName, usage), [&filepath, &usage] (texture_t & texture) {
			if ( !texture.enableSRGB(usage.sRGB) || !texture.enableFlipNormalMapY(usage.flipNormalMapY) || !texture.enableNormalMap(usage.normalMap) )
			{
				return false;
			}

			return texture.load(*filepath);
		});
	}
}
