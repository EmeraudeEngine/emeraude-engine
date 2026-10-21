/*
 * src/Graphics/GPUTextureSet.hpp
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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/* Local inclusions for usages. */
#include "PixelFactory/TextureName.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace Json
{
	class Value;
}

namespace EmEn::Graphics
{
	class CompressedImageResource;
	class ImageResource;

	namespace TextureResource
	{
		class Texture2D;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief The entries of one texture set in the GPUTextures store, indexed by the purpose they serve (graphics § 40).
	 * @note A set is the files `<setName>.<Purpose>.<extension>` of the store. Each purpose is served by one file at most:
	 * a purpose two files claim makes the whole set AMBIGUOUS — refused, logged, and the material falls back on its
	 * sources (owner decision, 2026-10-21). A file that names the set but breaks the convention is reported and ignored.
	 */
	class EMEN_API GPUTextureSet final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GPUTextureSet"};

			/** @brief The name prefix of the Texture2D created for an entry: never a store file (see texture()). */
			static constexpr std::string_view NamePrefix{"GPUTextures/"};

			/** @brief One entry of the set, as it serves a purpose. */
			struct Slot final
			{
				/** @brief The entry's resource name in the GPUTextures store. */
				std::string entry;
				/** @brief The purpose's channels in that entry. */
				Base::PixelFactory::TexturePurposeChannels channels{};
				/** @brief Whether the entry is a colour (sRGB) file. */
				bool colour{false};
				/** @brief The bake's fingerprint (KTX2Writer::BakeFingerprintKey), when the entry has one. */
				std::optional< std::string > fingerprint;
				/** @brief Whether the entry is a normal map. */
				bool normalMap{false};
			};

			/** @brief What a fingerprint check concludes (graphics § 40, L4). */
			enum class Freshness : uint8_t
			{
				/** @brief The sources, the recipe and the pipeline are those of the bake. */
				Fresh,
				/** @brief A source is absent (a product without its sources): nothing to compare, the entry is served. */
				Unverifiable,
				/** @brief The entry has no fingerprint (baked before L4, or by another tool): served, reported. */
				NoFingerprint,
				/** @brief A source, the recipe or the pipeline changed since the bake: the sources replace the entry. */
				Stale
			};

			/**
			 * @brief Finds the entries of a set in the GPUTextures store.
			 * @param store The CompressedImageResource container (its store is GPUTextures).
			 * @param setName The set's name: a material's TextureSet, or its name.
			 * @return GPUTextureSet Empty when the set has no entry, or is ambiguous (logged).
			 */
			[[nodiscard]]
			static GPUTextureSet find (const Resources::Container< CompressedImageResource > & store, std::string_view setName) noexcept;

			/**
			 * @brief Returns whether the set serves no purpose.
			 * @return bool
			 */
			[[nodiscard]]
			bool empty () const noexcept;

			/**
			 * @brief Returns the entry serving a purpose, if any.
			 * @param purpose The purpose.
			 * @return const Slot * Null when the set does not serve it.
			 */
			[[nodiscard]]
			const Slot * slot (Base::PixelFactory::TexturePurpose purpose) const noexcept;

			/**
			 * @brief Returns the Texture2D of an entry, created once (named NamePrefix + entry) with the identity its purpose
			 * gives — sRGB for a colour file, a normal map for a NormalGL one — and loaded from the compressed image.
			 * @param textures The Texture2D container.
			 * @param store The CompressedImageResource container.
			 * @param slot The entry.
			 * @return std::shared_ptr< TextureResource::Texture2D > Null on a failure (logged).
			 */
			[[nodiscard]]
			static std::shared_ptr< TextureResource::Texture2D > texture (Resources::Container< TextureResource::Texture2D > & textures, Resources::Container< CompressedImageResource > & store, const Slot & slot) noexcept;

			/** @brief The content hashes of the source files read during one planning: each file hashed once. */
			using SourceHashes = std::vector< std::pair< std::string, std::string > >;

			/**
			 * @brief Checks an entry against its fingerprint and the material's current declaration of a component it
			 * serves: the pipeline version (TextureCache::version()), the component's source (its image, its channel, a
			 * normal's flip), and the content of every source of the entry (sourceHash()).
			 * @param slot The entry.
			 * @param declared The component's declaration (its "Data" object in the material definition).
			 * @param images The ImageResource container (the sources' files).
			 * @param hashes The hashes already computed by this planning (completed by this call).
			 * @param reason Why the entry is stale, written for Freshness::Stale.
			 * @return Freshness
			 */
			[[nodiscard]]
			static Freshness check (const Slot & slot, const Json::Value & declared, const Resources::Container< ImageResource > & images, SourceHashes & hashes, std::string & reason) noexcept;

			/**
			 * @brief Returns the content hash of a source file, as a bake fingerprint records it: FNV-1a 64 of its bytes,
			 * 16 hexadecimal digits (the same function for the bake tool and the engine).
			 * @param filepath The file.
			 * @return std::optional< std::string > Nothing when the file cannot be read.
			 */
			[[nodiscard]]
			static std::optional< std::string > sourceHash (const std::filesystem::path & filepath) noexcept;

		private:

			std::array< std::optional< Slot >, Base::PixelFactory::TexturePurposeCount > m_slots{};
	};
}
