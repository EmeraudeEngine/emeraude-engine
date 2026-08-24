/*
 * src/Graphics/Compute/ProbeConvolver.hpp
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

/* Engine configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Vulkan
{
	class CommandBuffer;
	class ComputePipeline;
	class DescriptorPool;
	class DescriptorSet;
	class Image;
	class ImageView;
	class PipelineLayout;
	class Sampler;
}

namespace EmEn::Graphics::Compute
{
	/**
	 * @brief GGX-convolves a render-to-cubemap probe's mip chain, per frame, in the frame
	 * command buffer (no blocking submit).
	 * @details Mip 0 is the probe's native render (perfect mirror, untouched). The upper
	 * mips are GGX-prefiltered with roughness k/(mips-1) — the exact chain semantics of the
	 * sky IBL, so materials sample `textureLod(probe, R, roughness × (mips-1))` like the
	 * automatic environment path. The prefilter's FILTERED importance sampling needs a plain
	 * mip chain on its SOURCE (reads mips by solid-angle ratio): reading the probe's own
	 * mips while writing them is both a hazard and a bias, so the convolver first blits a
	 * standard mip cascade into a private half-size SCRATCH cubemap and importance-samples
	 * that. The pipelines are BORROWED from the renderer's IBLBaker (same GLSL, same layout).
	 */
	class EMEN_API ProbeConvolver final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"GraphicsComputeProbeConvolver"};

			/**
			 * @brief Constructs a probe convolver.
			 */
			ProbeConvolver () noexcept;

			/**
			 * @brief Copy constructor (deleted).
			 * @note Deleted EXPLICITLY, not merely left implicit: the class is exported
			 * (EMEN_API), which forces MSVC to DEFINE every implicit special member at the
			 * class definition point. The implicit copy assignment is not deleted (std::vector's
			 * copy assignment is declared for any T, move-only included — it is only ill-formed
			 * in its body), so defining it hard-errors on m_mipDescriptorSets
			 * (std::unique_ptr is not copy-assignable). Same export mechanism as the out-of-line
			 * destructor of BindlessTextureManager. See docs/windows-export-api.md.
			 */
			ProbeConvolver (const ProbeConvolver &) = delete;

			/**
			 * @brief Move constructor (deleted).
			 * @note Already suppressed by the user-declared destructor — explicit for the
			 * diagnostic and for consistency with the other exported RAII holders.
			 */
			ProbeConvolver (ProbeConvolver &&) = delete;

			/**
			 * @brief Copy assignment (deleted).
			 * @note See the copy constructor — this is the member the export actually chokes on.
			 */
			ProbeConvolver & operator= (const ProbeConvolver &) = delete;

			/**
			 * @brief Move assignment (deleted).
			 */
			ProbeConvolver & operator= (ProbeConvolver &&) = delete;

			/**
			 * @brief Destructs the probe convolver.
			 * @note Out of line: members are smart pointers to forward-declared types.
			 */
			~ProbeConvolver () noexcept;

			/**
			 * @brief Creates the scratch cubemap, the per-mip views and descriptor sets.
			 * @param renderer A reference to the graphics renderer (IBL baker, samplers).
			 * @param probeImage The probe color cubemap image (6 layers, 2+ mips, usage
			 * TRANSFER_SRC + STORAGE on top of the attachment/sampled bits).
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer, const std::shared_ptr< Vulkan::Image > & probeImage) noexcept;

			/**
			 * @brief Returns whether the convolver is ready to record.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			usable () const noexcept
			{
				return !m_mipDescriptorSets.empty();
			}

			/**
			 * @brief Records the convolution into the probe's command buffer, AFTER its
			 * render pass: scratch blit cascade, then one GGX prefilter dispatch per upper
			 * mip. Every touched subresource is left in SHADER_READ_ONLY_OPTIMAL, matching
			 * the image layout the render pass and the materials expect.
			 * @param commandBuffer The probe's frame command buffer (recording state).
			 * @return void
			 */
			void record (const Vulkan::CommandBuffer & commandBuffer) const noexcept;

		private:

			std::shared_ptr< Vulkan::Image > m_probeImage;
			std::shared_ptr< Vulkan::Image > m_scratchImage;
			std::shared_ptr< Vulkan::ImageView > m_scratchCubeView;
			std::shared_ptr< Vulkan::Sampler > m_scratchSampler;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			/** @brief One storage view + descriptor set per DEST mip (probe mips 1..N-1). */
			std::vector< std::shared_ptr< Vulkan::ImageView > > m_mipStorageViews;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_mipDescriptorSets;
			/* Borrowed from the renderer's IBL baker (renderer outlives every render target). */
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			const Vulkan::ComputePipeline * m_prefilterPipeline{nullptr};
			uint32_t m_probeSize{0};
			uint32_t m_probeMipLevels{0};
			uint32_t m_scratchSize{0};
			uint32_t m_scratchMipLevels{0};
	};
}
