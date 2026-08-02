/*
 * src/Graphics/SkinnedGeometryProcessor.hpp
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

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "Vulkan/ComputePipeline.hpp"
#include "Vulkan/PipelineLayout.hpp"

/* Forward declarations. */
namespace EmEn::Graphics
{
	class Renderer;
}

namespace EmEn::Graphics
{
	/**
	 * @brief Skins vertices on the GPU into a per-instance mirror vertex buffer for
	 * ray-traced geometry (per-frame BLAS refit).
	 * @details The rasterizer skins in the vertex shader, so a skinned mesh's VBO only
	 * holds the bind pose — a static BLAS would put a frozen mannequin in the TLAS.
	 * This processor runs the SAME skinning math (interleaved {current, previous} bone
	 * matrices, current slots) in a compute pass, writing a full-layout copy of the
	 * vertex buffer (same stride and attribute offsets) with skinned position and
	 * skinned normal/TBN. The mirror buffer then feeds the per-frame BLAS refit and
	 * the RT hit-shading vertex fetches (GPUMeshMetaData.vertexBufferAddress).
	 * @note One instance is owned by the renderer, next to the acceleration structure
	 * builder. It reuses the raster skinning descriptor sets (the layout declares the
	 * COMPUTE stage) so the exact same per-frame bone section drives both pipelines.
	 */
	class EMEN_API SkinnedGeometryProcessor final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SkinnedGeometryProcessor"};

			/**
			 * @brief Push constants for the skinning dispatch.
			 * @note Addresses are BDA (the source VBO and the mirror buffer both carry
			 * VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT). Attribute offsets are in FLOATS.
			 */
			struct EMEN_API PushConstants
			{
				uint64_t srcAddress{0};
				uint64_t dstAddress{0};
				uint32_t vertexCount{0};
				uint32_t floatsPerVertex{0};
				/** @brief 0 = no normal data, 1 = normal only (floats 3-5), 2 = full TBN (tangent 3-5, binormal 6-8, normal 9-11). */
				uint32_t tbnMode{0};
				/** @brief Float offset of the influence vec4; the weight vec4 follows it directly. */
				uint32_t influenceOffset{0};
			};

			/**
			 * @brief Constructs the processor.
			 */
			SkinnedGeometryProcessor () noexcept = default;

			/**
			 * @brief Destructs the processor.
			 * @note Out of line: members are smart pointers to forward-declared types.
			 */
			~SkinnedGeometryProcessor () noexcept;

			/**
			 * @brief Initializes the compute pipeline.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool initialize (Renderer & renderer) noexcept;

			/**
			 * @brief Returns whether the processor is ready to record dispatches.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			usable () const noexcept
			{
				return m_pipeline != nullptr;
			}

			/**
			 * @brief Records one skinning dispatch (one workgroup thread per vertex).
			 * @note The caller owns the surrounding barriers (previous-frame reads ->
			 * compute write before, compute write -> BLAS build read after).
			 * @param cmdBuf The command buffer (must be in recording state).
			 * @param pushConstants The dispatch description.
			 * @param bonesDescriptorSet The instance's per-frame skinning matrices set
			 * (from RenderableInstance::Abstract::flushSkinningMatrices()).
			 * @return void
			 */
			void recordDispatch (VkCommandBuffer cmdBuf, const PushConstants & pushConstants, VkDescriptorSet bonesDescriptorSet) const noexcept;

		private:

			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::unique_ptr< Vulkan::ComputePipeline > m_pipeline;
	};
}
