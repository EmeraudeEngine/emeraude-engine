/*
 * src/Graphics/TexturePacker.cpp
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

#include "TexturePacker.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <functional>
#include <initializer_list>
#include <optional>
#include <ranges>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/ImageResource.hpp"
#include "Graphics/TextureResource/StoreTexture.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Logging/Severity.hpp"
#include "PrimaryServices.hpp"
#include "Resources/Container.hpp"
#include "Resources/ResourceTrait.hpp"
#include "Resources/Types.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Base;

	namespace
	{
		/** @brief One map of a canonical texture: its role, and the output channel it fills (none: R, G and B). */
		struct PackMember
		{
			ComponentType role{ComponentType::Invalid};
			std::optional< PixelFactory::Channel > output;
		};

		/** @brief A canonical texture of the layout (graphics § 39). */
		struct PackGroup
		{
			std::string_view layout;
			std::array< PackMember, 4 > members{};
			size_t minimumMembers{0};
			bool sRGB{false};
		};

		/* NOTE: The normal map stays alone (owner decision D6, revised 2026-10-20 on a measurement): packed with the
		 * height in its A, the shared BC7 blocks cost the normal its fine grain (relief, per pixel: mean |d| 2.63, 5.6 %
		 * of the pixels off by more than 8); the height goes into the free A of the data texture instead (1.15, the
		 * noise) — the same footprint. */
		constexpr std::array< PackGroup, 2 > PackGroups{{
			{.layout = "Albedo", .members = {{{.role = ComponentType::Albedo, .output = std::nullopt}, {.role = ComponentType::Opacity, .output = PixelFactory::Channel::Alpha}, {}, {}}}, .minimumMembers = 2, .sRGB = true},
			{.layout = "Data", .members = {{{.role = ComponentType::AmbientOcclusion, .output = PixelFactory::Channel::Red}, {.role = ComponentType::Roughness, .output = PixelFactory::Channel::Green}, {.role = ComponentType::Metalness, .output = PixelFactory::Channel::Blue}, {.role = ComponentType::Displacement, .output = PixelFactory::Channel::Alpha}}}, .minimumMembers = 2, .sRGB = false}
		}};

		/** @brief A source taken by a group, with the output channel it fills. */
		struct GroupedSource
		{
			const PackSource * source{nullptr};
			std::optional< PixelFactory::Channel > output;
		};

		using GroupedSources = StaticVector< GroupedSource, 4 >;

		/**
		 * @brief Gathers the sources a canonical texture takes (one per member role, with an image).
		 * @param sources The material's maps.
		 * @param group The canonical texture.
		 * @return GroupedSources
		 */
		[[nodiscard]]
		GroupedSources
		gather (std::span< const PackSource > sources, const PackGroup & group) noexcept
		{
			GroupedSources grouped;

			for ( const auto & member : group.members )
			{
				if ( member.role == ComponentType::Invalid )
				{
					continue;
				}

				/* NOTE: A missing image's stand-in (the default resource) is never packed. */
				const auto source = std::ranges::find_if(sources, [&member] (const PackSource & candidate) {
					return candidate.role == member.role && candidate.image != nullptr && candidate.image->name() != Resources::Default;
				});

				/* NOTE: In capacity: one entry per member, 4 members. */
				if ( source != sources.end() )
				{
					grouped.push_back({.source = &*source, .output = member.output});
				}
			}

			return grouped;
		}

		/**
		 * @brief Builds the recipe of a canonical texture: its sources' channels, 1.0 elsewhere (D4).
		 * @param grouped The sources.
		 * @return PackRecipe
		 */
		[[nodiscard]]
		PackRecipe
		buildRecipe (const GroupedSources & grouped) noexcept
		{
			PackRecipe recipe;

			for ( const auto output : {PixelFactory::Channel::Red, PixelFactory::Channel::Green, PixelFactory::Channel::Blue, PixelFactory::Channel::Alpha} )
			{
				recipe.setConstant(output, 255);
			}

			for ( const auto & [source, output] : grouped )
			{
				if ( output.has_value() )
				{
					recipe.setChannel(*output, source->image, source->channel);

					continue;
				}

				/* NOTE: A colour map (the albedo) fills R, G and B with its own. */
				recipe.setChannel(PixelFactory::Channel::Red, source->image, PixelFactory::Channel::Red);
				recipe.setChannel(PixelFactory::Channel::Green, source->image, PixelFactory::Channel::Green);
				recipe.setChannel(PixelFactory::Channel::Blue, source->image, PixelFactory::Channel::Blue);
			}

			return recipe;
		}
	}

	TexturePacker::TexturePacker (Resources::AbstractServiceProvider & serviceProvider) noexcept
		: m_images{serviceProvider.container< ImageResource >()},
		m_textures{serviceProvider.container< TextureResource::Texture2D >()},
		m_enabled{serviceProvider.primaryServices().settings().getOrSetDefault< bool >(GraphicsTexturePackingKey, DefaultGraphicsTexturePacking)}
	{

	}

	TexturePacker::PackedRoles
	TexturePacker::pack (std::span< const PackSource > sources, std::string_view owner) const noexcept
	{
		PackedRoles packed;

		if ( !this->isEnabled() )
		{
			return packed;
		}

		for ( const auto & group : PackGroups )
		{
			const auto grouped = gather(sources, group);

			if ( grouped.size() < group.minimumMembers )
			{
				continue;
			}

			/* D11: only maps read at the same UVs share a texture. */
			if ( !std::ranges::all_of(grouped, [&grouped] (const GroupedSource & entry) { return entry.source->samplingKey == grouped.front().source->samplingKey; }) )
			{
				TraceInfo{ClassId} << "Material '" << owner << "': its " << group.layout << " maps are read at different UVs, they stay separate textures.";

				continue;
			}

			const auto recipe = buildRecipe(grouped);
			const auto imageName = recipe.name(group.layout);
			const auto textureName = TextureResource::variantName(imageName, {.sRGB = group.sRGB});

			const auto image = m_images->getOrCreateResourceSync(imageName, [recipe] (ImageResource & packedImage) {
				return packedImage.load(recipe);
			});

			auto texture = m_textures->getOrCreateResourceSync(textureName, [image, sRGB = group.sRGB] (TextureResource::Texture2D & packedTexture) {
				return packedTexture.enableSRGB(sRGB) && packedTexture.load(image);
			});

			/* NOTE: A failed creation answers the default resource: the maps then keep their own textures. */
			if ( image == nullptr || image->name() != imageName || texture == nullptr || texture->name() != textureName )
			{
				TraceError{ClassId} << "Material '" << owner << "': unable to pack its " << group.layout << " maps, they stay separate textures !";

				continue;
			}

			for ( const auto & [source, output] : grouped )
			{
				/* NOTE: In capacity: one entry per role, the roles of the two groups being distinct (6). */
				packed.push_back({.texture = texture, .role = source->role, .channel = output.value_or(PixelFactory::Channel::Red)});
			}

			TraceInfo{ClassId} << "Material '" << owner << "': " << grouped.size() << " maps packed into '" << textureName << "'.";
		}

		return packed;
	}
}
