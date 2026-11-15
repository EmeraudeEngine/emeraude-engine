/*
 * src/Graphics/AbstractRayTracedEffect.cpp
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

#include "AbstractRayTracedEffect.hpp"

/* STL inclusions. */
#include <cassert>

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/BindlessTextureManager.hpp"
#include "Graphics/IrradianceProbeVolume.hpp"
#include "Graphics/RayTracing.hpp"
#include "Graphics/RayTracingFrame.hpp"
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/LayoutManager.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	bool
	AbstractRayTracedEffect::createTraceLayout (const char * classId, const std::shared_ptr< DescriptorSetLayout > & inputLayout, RadianceCache radianceCache, const Base::StaticVector< VkPushConstantRange, 4 > & pushConstantRanges) noexcept
	{
		auto & renderer = this->renderer();

		/* Set 0: what a frame traces. Its layout exists exactly when the session can trace. */
		const auto & rayTracingSetLayout = renderer.rayTracing().setLayout();

		if ( rayTracingSetLayout == nullptr )
		{
			TraceError{classId} << "The ray-tracing descriptor set layout is not available: this session cannot trace !";

			return false;
		}

		if ( inputLayout == nullptr )
		{
			TraceError{classId} << "The layout of the trace inputs (set 1) is missing !";

			return false;
		}

		/* Set 2: the bindless textures. */
		const auto bindlessLayout = renderer.bindlessTextureManager().descriptorSetLayout();

		if ( bindlessLayout == nullptr )
		{
			TraceError{classId} << "The bindless texture descriptor set layout is not available !";

			return false;
		}

		Base::StaticVector< std::shared_ptr< DescriptorSetLayout >, 6 > sets;
		sets.emplace_back(rayTracingSetLayout);
		sets.emplace_back(inputLayout);
		sets.emplace_back(bindlessLayout);

		/* Set 3: the irradiance probe volume. The ray-tracing service creates it with the acceleration structure
		 * builder; an effect that reads the radiance cache at its hits is not created without it (its slot falls back
		 * to the screen-space occupant). */
		if ( radianceCache == RadianceCache::Read )
		{
			const auto * probeVolume = renderer.rayTracing().irradianceProbeVolume();

			if ( probeVolume == nullptr || !probeVolume->usable() )
			{
				TraceError{classId} << "The irradiance probe volume is not available: this effect reads the radiance cache at its hits !";

				return false;
			}

			sets.emplace_back(probeVolume->descriptorSetLayout());
		}

		m_traceLayout = renderer.layoutManager().getPipelineLayout(sets, pushConstantRanges);
		m_readsRadianceCache = m_traceLayout != nullptr && radianceCache == RadianceCache::Read;

		return m_traceLayout != nullptr;
	}

	void
	AbstractRayTracedEffect::bindTraceSets (const CommandBuffer & commandBuffer, const DescriptorSet & inputSet, const FrameContext & context, uint32_t frameIndex) const noexcept
	{
		/* NOTE: The family's contract: a member is recorded only in a frame that traces (requiresRayTracing(), the
		 * executor's gate), with its trace layout created (an effect that is not created is not recorded). */
		assert(context.rayTracing != nullptr);
		assert(m_traceLayout != nullptr);

		const auto & layout = *m_traceLayout;

		/* Set 0: what THIS frame traces (the acceleration structure it built + the scene's buffers). */
		commandBuffer.bind(context.rayTracing->descriptorSet, layout, VK_PIPELINE_BIND_POINT_GRAPHICS, 0);

		/* Set 1: the effect's own inputs for the frame. */
		commandBuffer.bind(inputSet, layout, VK_PIPELINE_BIND_POINT_GRAPHICS, 1);

		const auto & renderer = this->renderer();

		/* Set 2: the bindless textures. */
		if ( const auto * bindlessSet = renderer.bindlessTextureManager().descriptorSet(); bindlessSet != nullptr )
		{
			commandBuffer.bind(*bindlessSet, layout, VK_PIPELINE_BIND_POINT_GRAPHICS, 2);
		}

		/* Set 3: the irradiance probe volume of the frame (refreshed by the renderer before the chain runs). */
		if ( m_readsRadianceCache )
		{
			if ( const auto * probeVolume = renderer.rayTracing().irradianceProbeVolume(); probeVolume != nullptr )
			{
				if ( const auto * probeSet = probeVolume->descriptorSet(frameIndex); probeSet != nullptr )
				{
					commandBuffer.bind(*probeSet, layout, VK_PIPELINE_BIND_POINT_GRAPHICS, 3);
				}
			}
		}
	}
}
