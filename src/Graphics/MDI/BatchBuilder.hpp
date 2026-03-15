/*
 * src/Graphics/MDI/BatchBuilder.hpp
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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "PerDrawData.hpp"
#include "Scenes/RenderBatch.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/IndirectBuffer.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class CommandBuffer;
	class Device;
}

namespace EmEn::Graphics
{
	namespace RenderTarget
	{
		class Abstract;
	}

	class BindlessTextureManager;
}

namespace EmEn::Graphics::MDI
{
	/**
	 * @brief Describes a group of draws that share the same pipeline, material, and geometry.
	 *
	 * Objects within a batch can be drawn with a single vkCmdDrawIndexedIndirect call
	 * instead of N separate vkCmdDrawIndexed calls.
	 */
	struct MDIBatch final
	{
		/** @brief Offset into the indirect command buffer (in bytes). */
		VkDeviceSize indirectOffset{0};
		/** @brief Number of draw commands in this batch. */
		uint32_t drawCount{0};
		/** @brief Index of the first draw in the per-draw SSBO for this batch. */
		uint32_t firstDrawIndex{0};
		/** @brief All render batches in this group (for fallback individual rendering). */
		std::vector< const Scenes::RenderBatch * > renderBatches;
	};

	/**
	 * @brief Builds and dispatches Multi-Draw Indirect batches for opaque rendering.
	 *
	 * Analyzes a state-sorted render list, identifies groups of objects that share
	 * the same pipeline/material/geometry, fills an SSBO with per-draw model matrices
	 * (accessed via BDA + gl_DrawID), fills an indirect command buffer, and dispatches
	 * via vkCmdDrawIndexedIndirect.
	 *
	 * Resources are double-buffered (one set per frame-in-flight) to avoid GPU/CPU sync.
	 * Falls back to individual draw calls for groups smaller than 2 or when MDI is not
	 * supported by the device.
	 */
	class BatchBuilder final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"MDIBatchBuilder"};

			/**
			 * @brief Constructs an MDI batch builder.
			 * @param device A reference to the Vulkan device smart pointer.
			 * @param framesInFlight Number of concurrent frames (for double/triple buffering).
			 */
			BatchBuilder (const std::shared_ptr< Vulkan::Device > & device, uint32_t framesInFlight) noexcept;

			/**
			 * @brief Copy constructor (deleted).
			 */
			BatchBuilder (const BatchBuilder &) = delete;

			/**
			 * @brief Move constructor (deleted).
			 */
			BatchBuilder (BatchBuilder &&) = delete;

			/**
			 * @brief Copy assignment (deleted).
			 */
			BatchBuilder & operator= (const BatchBuilder &) = delete;

			/**
			 * @brief Move assignment (deleted).
			 */
			BatchBuilder & operator= (BatchBuilder &&) = delete;

			/**
			 * @brief Destructor.
			 */
			~BatchBuilder () = default;

			/**
			 * @brief Creates GPU resources (SSBO + indirect buffers for each frame-in-flight).
			 * @return True if all buffers were created successfully.
			 */
			[[nodiscard]]
			bool createResources () noexcept;

			/**
			 * @brief Analyzes a state-sorted render list and builds MDI batches.
			 *
			 * Groups consecutive render batches that share the same pipeline, material,
			 * and geometry. Fills the per-draw SSBO with model matrices and the indirect
			 * buffer with VkDrawIndexedIndirectCommand structs.
			 *
			 * @param renderList The state-sorted render list (opaque objects).
			 * @param frameIndex The current frame-in-flight index.
			 * @param readStateIndex The double-buffer read state index for matrix retrieval.
			 * @return The number of MDI batches created.
			 */
			uint32_t buildBatches (const Scenes::RenderBatch::List & renderList, uint32_t frameIndex, uint32_t readStateIndex) noexcept;

			/**
			 * @brief Dispatches the prepared MDI batches as Vulkan draw commands.
			 *
			 * For each batch, binds the shared pipeline/material/geometry state once,
			 * then issues a single vkCmdDrawIndexedIndirect call. Falls back to
			 * individual draws for batches with only 1 draw.
			 *
			 * @param renderTarget The render target for viewport/descriptor setup.
			 * @param commandBuffer The Vulkan command buffer for recording.
			 * @param frameIndex The current frame-in-flight index.
			 * @param readStateIndex The double-buffer read state index.
			 * @param bindlessTexturesManager Optional bindless texture manager.
			 */
			void dispatch (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, const Vulkan::CommandBuffer & commandBuffer, uint32_t frameIndex, uint32_t readStateIndex, const BindlessTextureManager * bindlessTexturesManager) const noexcept;

			/**
			 * @brief Returns the BDA (buffer device address) of the per-draw SSBO for a given frame.
			 * @param frameIndex The frame-in-flight index.
			 * @return VkDeviceAddress
			 */
			[[nodiscard]]
			VkDeviceAddress perDrawSSBOAddress (uint32_t frameIndex) const noexcept;

			/**
			 * @brief Returns the total number of individual draws that were batched in the last buildBatches() call.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			totalDrawsBatched () const noexcept
			{
				return m_totalDrawsBatched;
			}

			/**
			 * @brief Returns the number of individual fallback draws in the last buildBatches() call.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			totalFallbackDraws () const noexcept
			{
				return m_totalFallbackDraws;
			}

			/**
			 * @brief Returns whether MDI resources are ready.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isReady () const noexcept
			{
				return m_isReady;
			}

			/**
			 * @brief Returns the number of objects skipped by MDI (sprites, InfinityView, adaptive LOD).
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			skippedCount () const noexcept
			{
				return m_skippedCount;
			}

		private:

			std::shared_ptr< Vulkan::Device > m_device;
			uint32_t m_framesInFlight;
			/** @brief Per-draw SSBO (model matrices) with BDA support, one per frame-in-flight. */
			std::vector< std::unique_ptr< Vulkan::Buffer > > m_perDrawSSBOs;
			/** @brief Indirect command buffers, one per frame-in-flight. */
			std::vector< std::unique_ptr< Vulkan::IndirectBuffer > > m_indirectBuffers;
			/** @brief Batches computed during buildBatches(). */
			std::vector< MDIBatch > m_batches;
			/** @brief Total draws that were grouped into MDI batches. */
			uint32_t m_totalDrawsBatched{0};
			/** @brief Total draws that fell back to individual vkCmdDrawIndexed. */
			uint32_t m_totalFallbackDraws{0};
			uint32_t m_skippedCount{0};
			bool m_isReady{false};
	};
}