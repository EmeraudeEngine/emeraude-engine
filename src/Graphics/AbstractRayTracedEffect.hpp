/*
 * src/Graphics/AbstractRayTracedEffect.hpp
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
#include "volk.h"

/* Local inclusions for inheritances. */
#include "IndirectPostProcessEffect.hpp"

/* Local inclusions for usages. */
#include "StaticVector.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics
	{
		class Renderer;
	}

	namespace Vulkan
	{
		class CommandBuffer;
		class DescriptorSet;
		class DescriptorSetLayout;
		class PipelineLayout;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief The family level of the ray-traced effects: what every traced pass shares, between the effect contract
	 * (IndirectPostProcessEffect) and the concrete effects (RTGI, RTR, RTAO, RTContactShadows).
	 * @details The trace pass of every member of the family follows ONE descriptor set convention:
	 * - set 0: what the frame traces — the acceleration structure it built and the scene's buffers
	 *   (FrameContext::rayTracing, made by RayTracing::bindFrame());
	 * - set 1: the effect's own inputs for the frame (its G-buffer textures, its parameters block);
	 * - set 2: the bindless textures (the hit shading and the alpha-test rule sample them);
	 * - set 3: the irradiance probes, for an effect that reads the radiance cache at its hits.
	 *
	 * This level builds the pipeline layout of that convention (createTraceLayout()) and binds the sets of a frame
	 * (bindTraceSets()); a concrete effect brings its shaders, its targets and its set 1. It also answers
	 * requiresRayTracing() for the whole family: the executor and the selection record a member only in a frame that
	 * traces (PostProcessStack::canOccupantRun()), so nothing below tests it again.
	 * @extends EmEn::Graphics::IndirectPostProcessEffect This is a multi-pass post-process effect.
	 */
	class EMEN_API AbstractRayTracedEffect : public IndirectPostProcessEffect
	{
		public:

			/** @copydoc EmEn::Graphics::IndirectPostProcessEffect::requiresRayTracing()
			 * @note The family's answer: every member traces. */
			[[nodiscard]]
			bool
			requiresRayTracing () const noexcept final
			{
				return true;
			}

		protected:

			/** @brief Whether the trace of an effect reads the irradiance probes — the radiance cache — at its hits (set 3). */
			enum class RadianceCache : uint8_t
			{
				Unused,
				Read
			};

			/**
			 * @brief Constructs the family level of a ray-traced effect.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit
			AbstractRayTracedEffect (Renderer & renderer) noexcept
				: IndirectPostProcessEffect{renderer}
			{

			}

			/**
			 * @brief Builds the pipeline layout of the trace pass, on the family's set convention.
			 * @note Called by the concrete effect's create(). Fails — the effect is then not created, and its slot falls
			 * back — when the session cannot trace (no ray-tracing set layout), when the bindless layout is missing, or
			 * when the radiance cache is asked for and the irradiance probe volume is not usable.
			 * @param classId The concrete effect's identifier, for the traces.
			 * @param inputLayout The layout of the effect's own inputs (set 1).
			 * @param radianceCache Whether the trace reads the irradiance probes (set 3).
			 * @param pushConstantRanges The push constant ranges of the trace pass, empty when its parameters live in set 1.
			 * @return bool
			 */
			[[nodiscard]]
			bool createTraceLayout (const char * classId, const std::shared_ptr< Vulkan::DescriptorSetLayout > & inputLayout, RadianceCache radianceCache, const Base::StaticVector< VkPushConstantRange, 4 > & pushConstantRanges = {}) noexcept;

			/**
			 * @brief Releases the pipeline layout of the trace pass (the concrete effect's destroy()).
			 */
			void
			destroyTraceLayout () noexcept
			{
				m_traceLayout.reset();
				m_readsRadianceCache = false;
			}

			/**
			 * @brief Returns the pipeline layout of the trace pass: what the concrete effect builds its pipelines with.
			 * @note Null before createTraceLayout() succeeded.
			 * @return const std::shared_ptr< Vulkan::PipelineLayout > &
			 */
			[[nodiscard]]
			const std::shared_ptr< Vulkan::PipelineLayout > &
			traceLayout () const noexcept
			{
				return m_traceLayout;
			}

			/**
			 * @brief Binds the sets of the trace pass for the frame being recorded: what the frame traces (0), the
			 * effect's inputs (1), the bindless textures (2) and, for an effect that reads it, the radiance cache (3).
			 * @pre The trace layout is created, and the frame traces: context.rayTracing is set — the executor records
			 * a member of the family only then (checked in Debug).
			 * @param commandBuffer A reference to the frame's command buffer.
			 * @param inputSet The effect's own descriptor set for this frame (set 1).
			 * @param context The per-frame chain context.
			 * @param frameIndex The index of the frame in flight being recorded.
			 */
			void bindTraceSets (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::DescriptorSet & inputSet, const FrameContext & context, uint32_t frameIndex) const noexcept;

		private:

			std::shared_ptr< Vulkan::PipelineLayout > m_traceLayout;
			bool m_readsRadianceCache{false};
	};
}
