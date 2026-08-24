/*
 * src/Graphics/CompressedImageResource.hpp
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
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for inheritances. */
#include "Resources/ResourceTrait.hpp"

/* Local inclusions for usages. */
#include "KTX2Decoder.hpp"

/* Forward declarations. */
namespace EmEn
{
	class Settings;
}

namespace EmEn::Resources
{
	template< typename resource_t >
	class Container;
}

namespace EmEn::Graphics
{
	/**
	 * @class CompressedImageResource
	 * @brief Provides an already block-compressed image as a loadable resource.
	 *
	 * This is the compressed counterpart of @ref ImageResource. Where ImageResource carries
	 * a decoded RGBA8 @ref Base::PixelFactory::Pixmap that the engine has to compress itself
	 * (@ref TextureCompressor, BC7, expensive), this resource carries the block-compressed
	 * mip chain **as it came off the disk**, ready for a straight
	 * @ref Vulkan::Image::createFromCompressed() upload.
	 *
	 * The payload comes from a KTX2 container, the transport format of the glTF
	 * `KHR_texture_basisu` extension, transcoded by @ref KTX2Decoder. Nothing here is ever
	 * expanded to uncompressed pixels: a 4096×4096 texture costs ~22 MiB with its full mip
	 * chain instead of the ~89 MiB the RGBA8 level 0 alone would need.
	 *
	 * @note **This resource is an opaque GPU payload, and that is deliberate.** It exposes no
	 * `averageColor()`, no `isGrayScale()`, no per-pixel access and no `flipNormalMapY()` —
	 * none of those can be answered without decoding the blocks, which would defeat the whole
	 * point. Code that needs to *inspect* pixels wants @ref ImageResource, not this.
	 *
	 * @note The stored format is always the **linear** variant of the block layout. The sRGB
	 * decision belongs to the consuming texture, which knows the usage (albedo and emissive
	 * are sRGB, normal and ORM maps are not) — the blocks are identical either way.
	 *
	 * @extends EmEn::Resources::ResourceTrait This is a loadable resource.
	 * @see EmEn::Graphics::ImageResource
	 * @see EmEn::Graphics::KTX2Decoder
	 * @see EmEn::Graphics::TextureResource::Texture2D
	 * @version 0.8.36
	 */
	class EMEN_API CompressedImageResource final : public Resources::ResourceTrait
	{
		friend class Resources::Container< CompressedImageResource >;

		using ResourceTrait::load;

		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"CompressedImageResource"};

			/** @brief Defines the resource dependency complexity. */
			static constexpr auto Complexity{Resources::DepComplexity::None};

			/**
			 * @brief Constructs a compressed image resource with the specified name.
			 * @param serviceProvider A reference to the service provider.
			 * @param name The name of the resource [std::move].
			 * @param resourceFlags The resource flag bits. Default none. (Unused yet)
			 */
			CompressedImageResource (Resources::AbstractServiceProvider & serviceProvider, std::string name, uint32_t resourceFlags = 0) noexcept
				: ResourceTrait{serviceProvider, std::move(name), resourceFlags}
			{

			}

			/**
			 * @brief Returns the unique class identifier for CompressedImageResource.
			 * @return Unique identifier as a size_t hash value.
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Base::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Base::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Base::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/** @copydoc EmEn::Resources::ResourceTrait::classLabel() const */
			[[nodiscard]]
			const char *
			classLabel () const noexcept override
			{
				return ClassId;
			}

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::load()
			 *
			 * Builds the fail-safe default : a 64×64 chequer compressed on the spot, so a
			 * missing compressed texture still shows something instead of crashing.
			 */
			bool load () noexcept override;

			/**
			 * @copydoc EmEn::Resources::ResourceTrait::load(const std::filesystem::path &)
			 *
			 * Reads a `.ktx2` file from disk and decodes it.
			 *
			 * @note The `.ktx2` extension is not registered in the resource store yet, so this
			 * path is only reachable through an explicit call. Wiring it into the store is the
			 * remaining step to make KTX2 a first-class engine image format.
			 */
			bool load (const std::filesystem::path & filepath) noexcept override;

			/** @copydoc EmEn::Resources::ResourceTrait::load(const Json::Value &) */
			bool load (const Json::Value & data) noexcept override;

			/**
			 * @brief Loads the resource from an in-memory KTX2 container.
			 * @param bytes The raw KTX2 container bytes. Only read during the call, nothing is retained.
			 * @return bool
			 * @note This is the entry point used by the glTF loader for `KHR_texture_basisu`
			 * images embedded in a `.glb` buffer view.
			 */
			bool load (std::span< const std::byte > bytes) noexcept;

			/** @copydoc EmEn::Resources::ResourceTrait::memoryOccupied() const noexcept */
			[[nodiscard]]
			size_t
			memoryOccupied () const noexcept override
			{
				size_t bytes = sizeof(*this);

				for ( const auto & mip : m_payload.mips )
				{
					bytes += mip.data.size();
				}

				return bytes;
			}

			/**
			 * @brief Returns the block-compressed mip levels, level 0 (largest) first.
			 * @return const std::vector< CompressedMipLevel > &
			 */
			[[nodiscard]]
			const std::vector< CompressedMipLevel > &
			mips () const noexcept
			{
				return m_payload.mips;
			}

			/**
			 * @brief Returns the linear Vulkan format of the blocks.
			 * @return VkFormat
			 * @note Use KTX2Decoder::sRGBFormat() to get the sRGB variant when the texture usage calls for it.
			 */
			[[nodiscard]]
			VkFormat
			format () const noexcept
			{
				return m_payload.format;
			}

			/**
			 * @brief Returns the width of the largest kept mip level, in pixels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			width () const noexcept
			{
				return m_payload.mips.empty() ? 0 : m_payload.mips.front().width;
			}

			/**
			 * @brief Returns the height of the largest kept mip level, in pixels.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			height () const noexcept
			{
				return m_payload.mips.empty() ? 0 : m_payload.mips.front().height;
			}

			/**
			 * @brief Returns the number of mip levels held.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			levelCount () const noexcept
			{
				return static_cast< uint32_t >(m_payload.mips.size());
			}

			/**
			 * @brief Returns the texture dimension clamp currently configured, in pixels.
			 * @param settings A reference to the engine settings.
			 * @return uint32_t
			 * @note 0 means no clamping.
			 */
			[[nodiscard]]
			static uint32_t maxDimension (Settings & settings) noexcept;

		private:

			KTX2Decoder::Result m_payload;
	};
}

/* Expose the resource container. */
namespace EmEn::Resources
{
	using CompressedImages = Container< Graphics::CompressedImageResource >;
}
