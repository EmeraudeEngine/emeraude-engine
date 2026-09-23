/*
 * src/Graphics/ImposterAtlas.hpp
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
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions. */
#include "emeraude_export.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class CommandBuffer;
		class Image;
		class TextureInterface;
		class TransferManager;
	}

	namespace Graphics
	{
		class Renderer;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief The two textures of an octahedral imposter: the ALBEDO (+ coverage) and the NORMAL of an object, seen from
	 * gridSize² directions of the upper hemisphere (emeraude-base `Math/OctahedralMapping.hpp`, hemi variant).
	 * @note Owner decisions 2026-09-23 (engine `docs/todo/vegetation-octahedral-imposter-atlas.md`): the atlas carries
	 * albedo and normal, lit AT RUNTIME, never a baked lit colour; 8 × 8 views of 128 px.
	 * @note It lives on the GPU only. RenderTarget::ImposterBake renders the views into its G-buffer and recordBake()
	 * copies them here and builds the mip chain in the same submission: no read-back, no CPU pass.
	 *  - ALBEDO: sRGB, rgb = the albedo PREMULTIPLIED by the coverage (the bake clears to 0, so every mip keeps
	 *    sum(albedo × coverage) / n in rgb and the coverage in alpha) — the shader divides rgb by alpha;
	 *  - NORMAL: RGBA16F, xyz = the normal in the view space of the cell's own camera (Math::imposterCellFrame()),
	 *    unnormalized in the mips — the shader normalizes; w is the G-buffer's roughness/metalness packing, unused.
	 * @note Created CLEARED (every mip, coverage 0): an imposter drawn before its bake discards every pixel.
	 */
	class EMEN_API ImposterAtlas final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ImposterAtlas"};

			/** @brief The albedo texture format, and the bake's albedo attachment format. */
			static constexpr VkFormat AlbedoFormat{VK_FORMAT_R8G8B8A8_SRGB};

			/** @brief The normal texture format, and the bake's normals attachment format. */
			static constexpr VkFormat NormalFormat{VK_FORMAT_R16G16B16A16_SFLOAT};

			/** @brief Views per side of the atlas (owner decision: 8 × 8). */
			static constexpr uint32_t DefaultGridSize{8};

			/** @brief Pixels per side of one view (owner decision: 128). */
			static constexpr uint32_t DefaultCellSize{128};

			/**
			 * @brief Mip levels of the atlas.
			 * @note Stops at 8 px per cell (128 → 64 → 32 → 16 → 8): below that the box filter blends NEIGHBOURING views
			 * into each other, a tree smaller than that on screen is a handful of pixels anyway.
			 */
			static constexpr uint32_t DefaultMipLevels{5};

			/**
			 * @brief Constructs an imposter atlas.
			 * @param name The name, for the GPU object identifiers [std::move].
			 * @param gridSize Views per side.
			 * @param cellSize Pixels per side of one view.
			 * @param mipLevels Mip levels (clamped to what the cell size allows).
			 */
			explicit ImposterAtlas (std::string name, uint32_t gridSize = DefaultGridSize, uint32_t cellSize = DefaultCellSize, uint32_t mipLevels = DefaultMipLevels) noexcept;

			/**
			 * @brief Destructs the imposter atlas.
			 * @note Out of line: the members hold shared pointers to forward-declared types.
			 */
			~ImposterAtlas ();

			ImposterAtlas (const ImposterAtlas & copy) noexcept = delete;
			ImposterAtlas (ImposterAtlas && copy) noexcept = delete;
			ImposterAtlas & operator= (const ImposterAtlas & copy) noexcept = delete;
			ImposterAtlas & operator= (ImposterAtlas && copy) noexcept = delete;

			/**
			 * @brief Creates both textures on the GPU, cleared.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer) noexcept;

			/**
			 * @brief Returns whether both textures exist on the GPU.
			 * @return bool
			 */
			[[nodiscard]]
			bool isCreated () const noexcept;

			/**
			 * @brief Returns the albedo (+ coverage) texture, for a material component.
			 * @return const std::shared_ptr< Vulkan::TextureInterface > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::TextureInterface > &
			albedoTexture () const noexcept
			{
				return m_albedoTexture;
			}

			/**
			 * @brief Returns the normal texture, for a material component.
			 * @return const std::shared_ptr< Vulkan::TextureInterface > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::TextureInterface > &
			normalTexture () const noexcept
			{
				return m_normalTexture;
			}

			/**
			 * @brief Returns the name.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			name () const noexcept
			{
				return m_name;
			}

			/**
			 * @brief Returns the number of views per side.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			gridSize () const noexcept
			{
				return m_gridSize;
			}

			/**
			 * @brief Returns the pixel size of one view.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			cellSize () const noexcept
			{
				return m_cellSize;
			}

			/**
			 * @brief Returns the pixel size of the whole atlas (gridSize × cellSize).
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			size () const noexcept
			{
				return m_gridSize * m_cellSize;
			}

			/**
			 * @brief Returns the mip level count.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			mipLevels () const noexcept
			{
				return m_mipLevels;
			}

			/**
			 * @brief Returns whether a bake has been recorded into the atlas. Thread-safe.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isBaked () const noexcept
			{
				return m_baked.load(std::memory_order_acquire);
			}

			/**
			 * @brief RENDER THREAD. Records the copy of a bake into the atlas and the build of its mip chain.
			 * @note Both sources must be in TRANSFER_SRC_OPTIMAL, with their attachment writes made visible to the
			 * transfer stage (the ImposterBake render pass's outgoing dependency); they are left there. The atlas
			 * ends in SHADER_READ_ONLY_OPTIMAL, visible to the vertex and fragment stages of every later command of
			 * the queue — the frame's own main pass included.
			 * @param commandBuffer The bake target's command buffer, outside any render pass.
			 * @param albedoSource The bake's albedo attachment (AlbedoFormat, size() × size()).
			 * @param normalSource The bake's normals attachment (NormalFormat, size() × size()).
			 * @return void
			 */
			void recordBake (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::Image & albedoSource, const Vulkan::Image & normalSource) noexcept;

			/**
			 * @brief Writes the albedo atlas (mip 0) to a PNG file. Debug; blocks on the GPU.
			 * @param transferManager A reference to the transfer manager.
			 * @param filepath The file to write.
			 * @return bool
			 */
			[[nodiscard]]
			bool writeAlbedo (Vulkan::TransferManager & transferManager, const std::filesystem::path & filepath) const noexcept;

		private:

			/**
			 * @brief Records the copy into one texture and its mip chain.
			 * @param commandBuffer The command buffer.
			 * @param source The bake attachment.
			 * @param destination The atlas image.
			 * @return void
			 */
			void recordCopyAndMips (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::Image & source, Vulkan::Image & destination) const noexcept;

			std::string m_name;
			std::shared_ptr< Vulkan::Image > m_albedoImage;
			std::shared_ptr< Vulkan::Image > m_normalImage;
			std::shared_ptr< Vulkan::TextureInterface > m_albedoTexture;
			std::shared_ptr< Vulkan::TextureInterface > m_normalTexture;
			uint32_t m_gridSize;
			uint32_t m_cellSize;
			uint32_t m_mipLevels;
			std::atomic_bool m_baked{false};
	};
}
