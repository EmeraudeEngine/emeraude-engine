/*
 * src/Graphics/GPUTextureSet.cpp
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

#include "GPUTextureSet.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/* Local inclusions. */
#include "FastJSON.hpp"
#include "Graphics/CompressedImageResource.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/KTX2Decoder.hpp"
#include "Graphics/KTX2Writer.hpp"
#include "Graphics/Material/Component/Texture.hpp"
#include "Graphics/TextureCache.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Hash/FNV1a.hpp"
#include "IO/IO.hpp"
#include "Resources/Container.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Base::PixelFactory;

	GPUTextureSet
	GPUTextureSet::find (const Resources::Container< CompressedImageResource > & store, std::string_view setName) noexcept
	{
		GPUTextureSet set;

		if ( setName.empty() )
		{
			return set;
		}

		for ( const auto & entry : store.availableResourceNames() )
		{
			/* NOTE: An entry of this set is "<setName>.<Purpose>": the purpose follows the last dot. */
			if ( entry.size() <= setName.size() + 1 || !entry.starts_with(setName) || entry[setName.size()] != '.' || entry.find('.', setName.size() + 1) != std::string::npos || entry.find('/', setName.size()) != std::string::npos )
			{
				continue;
			}

			const auto name = TextureName::parse(entry, TextureStore::GPU);

			if ( !name.isValid() )
			{
				TraceError{ClassId} << "The GPU texture '" << entry << "' of the set '" << setName << "' is ignored: " << to_cstring(name.error()) << ".";

				continue;
			}

			/* NOTE: The fingerprint is read from the header and the key/value block only, not the images (L4). */
			std::optional< std::string > fingerprint;

			if ( const auto filepath = store.localFilepath(entry); filepath.has_value() )
			{
				fingerprint = KTX2Decoder::readKeyValue(*filepath, KTX2Writer::BakeFingerprintKey);
			}

			for ( const auto & channels : name.purposes() )
			{
				auto & slot = std::span{set.m_slots}[static_cast< size_t >(channels.purpose)];

				if ( slot.has_value() )
				{
					TraceError{ClassId} << "The texture set '" << setName << "' is ambiguous: '" << slot->entry << "' and '" << entry << "' both serve " << to_cstring(channels.purpose) << ". It is refused: the material uses its sources.";

					return {};
				}

				slot = Slot{.entry = entry, .channels = channels, .colour = name.isColour(), .fingerprint = fingerprint, .normalMap = name.isNormalMap()};
			}
		}

		return set;
	}

	bool
	GPUTextureSet::empty () const noexcept
	{
		return std::ranges::all_of(m_slots, [] (const auto & slot) { return !slot.has_value(); });
	}

	const GPUTextureSet::Slot *
	GPUTextureSet::slot (TexturePurpose purpose) const noexcept
	{
		const auto & slot = std::span{m_slots}[static_cast< size_t >(purpose)];

		return slot.has_value() ? &*slot : nullptr;
	}

	GPUTextureSet::Freshness
	GPUTextureSet::check (const Slot & slot, const Json::Value & declared, const Resources::Container< ImageResource > & images, SourceHashes & hashes, std::string & reason) noexcept
	{
		using Material::Component::Texture;

		if ( !slot.fingerprint.has_value() )
		{
			return Freshness::NoFingerprint;
		}

		const auto root = Base::FastJSON::getRootFromString(*slot.fingerprint, 16, true);

		if ( !root.has_value() || !root->isObject() || !root->isMember("sources") || !(*root)["sources"].isObject() || !root->isMember("purposes") || !(*root)["purposes"].isObject() )
		{
			return Freshness::NoFingerprint;
		}

		const auto & sources = (*root)["sources"];

		/* A source absent (a product shipped without its sources): nothing to compare (owner decision). */
		std::vector< std::pair< std::string, std::filesystem::path > > files;

		for ( const auto & name : sources.getMemberNames() )
		{
			const auto filepath = images.localFilepath(name);

			if ( !filepath.has_value() || !Base::IO::fileExists(*filepath) )
			{
				return Freshness::Unverifiable;
			}

			files.emplace_back(name, *filepath);
		}

		if ( const auto version = Base::FastJSON::getValue< uint32_t >(*root, "version"); version.value_or(0) != TextureCache::version() )
		{
			reason = "it was baked by the texture pipeline version " + std::to_string(version.value_or(0)) + ", the engine's is " + std::to_string(TextureCache::version());

			return Freshness::Stale;
		}

		/* The recipe of the component: its source image, its channel, a normal's flip. */
		const auto token = std::string{to_cstring(slot.channels.purpose)};
		const auto & purposes = (*root)["purposes"];
		const auto declaredName = Base::FastJSON::getValue< std::string >(declared, Texture::JKResourceName).value_or(std::string{});

		if ( !purposes.isMember(token) || !purposes[token].isObject() )
		{
			reason = "its fingerprint does not record " + token;

			return Freshness::Stale;
		}

		const auto & recorded = purposes[token];

		if ( const auto source = Base::FastJSON::getValue< std::string >(recorded, "source").value_or(std::string{}); source != declaredName )
		{
			reason = "its " + token + " was baked from '" + source + "', the material now reads '" + declaredName + "'";

			return Freshness::Stale;
		}

		if ( slot.channels.channelCount == 1 )
		{
			const auto declaredChannel = declared.isMember(Texture::JKSourceChannel) ? Base::FastJSON::asValue< uint32_t >(declared[Texture::JKSourceChannel]).value_or(0) : 0U;

			if ( Base::FastJSON::getValue< uint32_t >(recorded, "channel").value_or(0) != declaredChannel )
			{
				reason = "its " + token + " was baked from another channel of its image";

				return Freshness::Stale;
			}
		}

		if ( slot.normalMap && Base::FastJSON::getValue< bool >(recorded, "flip").value_or(false) != Base::FastJSON::getValue< bool >(declared, Texture::JKFlipNormalMapY).value_or(false) )
		{
			reason = "its normal was baked with another Y flip";

			return Freshness::Stale;
		}

		/* The content of every source of the entry, each file hashed once per planning (the cache). */
		for ( const auto & [name, filepath] : files )
		{
			const auto cached = std::ranges::find(hashes, name, &std::pair< std::string, std::string >::first);
			std::optional< std::string > hash;

			if ( cached != hashes.end() )
			{
				hash = cached->second;
			}
			else if ( hash = sourceHash(filepath); hash.has_value() )
			{
				hashes.emplace_back(name, *hash);
			}

			const auto recordedHash = Base::FastJSON::getValue< std::string >(sources, name.c_str());

			if ( !hash.has_value() || !recordedHash.has_value() || *hash != *recordedHash )
			{
				reason = "its source '" + name + "' changed since the bake";

				return Freshness::Stale;
			}
		}

		return Freshness::Fresh;
	}

	std::optional< std::string >
	GPUTextureSet::sourceHash (const std::filesystem::path & filepath) noexcept
	{
		std::string content;

		if ( !Base::IO::fileGetContents(filepath, content) )
		{
			return std::nullopt;
		}

		auto hash = static_cast< uint64_t >(Base::Hash::FNV1a(content));
		std::string digits(16, '0');

		for ( auto & digit : std::ranges::reverse_view(digits) )
		{
			digit = std::string_view{"0123456789abcdef"}[hash & 0xFU];
			hash >>= 4U;
		}

		return digits;
	}

	std::shared_ptr< TextureResource::Texture2D >
	GPUTextureSet::texture (Resources::Container< TextureResource::Texture2D > & textures, Resources::Container< CompressedImageResource > & store, const Slot & slot) noexcept
	{
		auto image = store.getResource(slot.entry);

		if ( image == nullptr )
		{
			TraceError{ClassId} << "The GPU texture '" << slot.entry << "' is not in the GPUTextures store !";

			return nullptr;
		}

		auto texture = textures.getOrCreateResourceSync(std::string{NamePrefix} + slot.entry, [&image, &slot] (TextureResource::Texture2D & created) {
			return created.enableSRGB(slot.colour) && created.enableNormalMap(slot.normalMap) && created.load(image);
		});

		if ( texture == nullptr )
		{
			TraceError{ClassId} << "Unable to create the texture of the GPU texture '" << slot.entry << "' !";
		}

		return texture;
	}
}
