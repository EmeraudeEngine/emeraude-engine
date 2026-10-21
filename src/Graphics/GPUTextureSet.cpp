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
#include <span>
#include <string>

/* Local inclusions. */
#include "Graphics/CompressedImageResource.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
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

			for ( const auto & channels : name.purposes() )
			{
				auto & slot = std::span{set.m_slots}[static_cast< size_t >(channels.purpose)];

				if ( slot.has_value() )
				{
					TraceError{ClassId} << "The texture set '" << setName << "' is ambiguous: '" << slot->entry << "' and '" << entry << "' both serve " << to_cstring(channels.purpose) << ". It is refused: the material uses its sources.";

					return {};
				}

				slot = Slot{.entry = entry, .channels = channels, .colour = name.isColour(), .normalMap = name.isNormalMap()};
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
