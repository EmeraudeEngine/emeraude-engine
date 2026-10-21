/*
 * src/Graphics/TexturePacker.hpp
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
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>

/* Local inclusions for usages. */
#include "Graphics/Types.hpp"
#include "PixelFactory/Types.hpp"
#include "StaticVector.hpp"

/* Forward declarations. */
namespace EmEn::Resources
{
	class AbstractServiceProvider;

	template< typename resource_t >
	class Container;
}

namespace EmEn::Graphics
{
	class ImageResource;

	namespace TextureResource
	{
		class Texture2D;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief A canonical map a material is built from (graphics § 39): its image, before any texture is requested.
	 */
	struct PackSource
	{
		std::shared_ptr< ImageResource > image;
		/** @brief Where the map is read: two sources with the same key read the same UVs (D11). */
		std::string samplingKey;
		ComponentType role{ComponentType::Invalid};
		Base::PixelFactory::Channel channel{Base::PixelFactory::Channel::Red};
	};

	/**
	 * @brief A canonical map as the grouping sees it: its role and where it is read (graphics § 39, D11).
	 */
	struct PackCandidate
	{
		ComponentType role{ComponentType::Invalid};
		/** @brief Where the map is read: two candidates with the same key read the same UVs (D11). */
		std::string_view samplingKey;
	};

	/**
	 * @brief One canonical texture the grouping decided: its layout, the roles it takes in the layout's member order
	 * (Albedo: albedo, opacity; Data: ambient occlusion, roughness, metalness, height), and its colour space.
	 */
	struct PackPlan
	{
		std::string_view layout;
		Base::StaticVector< ComponentType, 4 > roles;
		bool sRGB{false};
	};

	/**
	 * @brief What a role reads once packed: the packed texture and the channel it lands in (the albedo reads RGB).
	 */
	struct PackedRole
	{
		std::shared_ptr< TextureResource::Texture2D > texture;
		ComponentType role{ComponentType::Invalid};
		Base::PixelFactory::Channel channel{Base::PixelFactory::Channel::Red};
	};

	/**
	 * @brief Packs the canonical maps of a material into at most two textures (graphics § 39, D14): Albedo (albedo RGB +
	 * opacity A, sRGB) and Data (ambient occlusion R, roughness G, metalness B, height A); the normal stays alone (D6).
	 * @note Used BEFORE any texture of the material is requested (a requested texture is loaded and uploaded at once),
	 * by the material JSON planner and by the scene loaders. A group needs two of its maps or more, read at the same
	 * UVs (D11); the packed image is as large as its largest source (D12).
	 * @note Thread-safe, like the containers it creates the resources in.
	 */
	class EMEN_API TexturePacker final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"TexturePacker"};

			/** @brief The roles a packed texture may take, at most. */
			static constexpr size_t MaxRoles{6};

			/** @brief The roles packed by one call, with their texture. */
			using PackedRoles = Base::StaticVector< PackedRole, MaxRoles >;

			/**
			 * @brief Constructs a packer on the resource containers of a service provider; disabled when
			 * `Core/Graphics/Texture/Packing` is false (D13).
			 * @param serviceProvider A reference to the service provider.
			 */
			explicit TexturePacker (Resources::AbstractServiceProvider & serviceProvider) noexcept;

			/**
			 * @brief Returns whether the packer packs anything (the setting and the containers).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return m_images != nullptr && m_textures != nullptr && m_enabled;
			}

			/** @brief The canonical textures a grouping may decide, at most. */
			static constexpr size_t MaxPlans{2};

			/** @brief The canonical textures one grouping decided. */
			using PackPlans = Base::StaticVector< PackPlan, MaxPlans >;

			/**
			 * @brief Decides which maps share a canonical texture — the ONE rule, used by pack() at run time and by the
			 * texture bake tool (graphics § 40, L3): a canonical texture takes the maps of its layout present among the
			 * candidates, two at least, read at the same UVs (D11); any other map stays alone.
			 * @param candidates The material's maps (a role at most once).
			 * @param owner The material's name, for the logs.
			 * @return PackPlans
			 */
			[[nodiscard]]
			static PackPlans plan (std::span< const PackCandidate > candidates, std::string_view owner) noexcept;

			/**
			 * @brief Packs the sources that group, and answers what each packed role reads; a role absent from the
			 * answer keeps its own texture.
			 * @param sources The material's canonical maps (a role at most once; a source without image is ignored).
			 * @param owner The material's name, for the logs.
			 * @return PackedRoles
			 */
			[[nodiscard]]
			PackedRoles pack (std::span< const PackSource > sources, std::string_view owner) const noexcept;

		private:

			Resources::Container< ImageResource > * m_images{nullptr};
			Resources::Container< TextureResource::Texture2D > * m_textures{nullptr};
			bool m_enabled{false};
	};
}
