/*
 * src/Graphics/RenderableInstance/RenderStateTracker.hpp
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

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

namespace EmEn::Graphics::RenderableInstance
{
	/**
	 * @brief Tracks the last-bound Vulkan state to skip redundant bind commands.
	 *
	 * Used during state-sorted opaque rendering to avoid re-binding identical
	 * pipelines, geometry buffers, and descriptor sets between consecutive draw calls.
	 *
	 * When the pipeline changes, all downstream state is invalidated because
	 * Vulkan descriptor set compatibility depends on the pipeline layout.
	 *
	 * @note POD structure, stack-allocated per render pass.
	 * @note Statistics counters are only active in debug builds.
	 */
	struct RenderStateTracker final
	{
		/** @brief Last bound graphics pipeline handle. */
		VkPipeline lastPipeline{VK_NULL_HANDLE};
		/** @brief Last bound geometry identity (C++ object address as proxy). */
		const void * lastGeometry{nullptr};
		/** @brief Last bound geometry layer index. */
		uint32_t lastLayerIndex{UINT32_MAX};
		/** @brief Last bound material descriptor set handle. */
		VkDescriptorSet lastMaterialDS{VK_NULL_HANDLE};
		/** @brief Last bound light descriptor set handle. */
		VkDescriptorSet lastLightDS{VK_NULL_HANDLE};
		/** @brief Last bound view descriptor set handle. */
		VkDescriptorSet lastViewDS{VK_NULL_HANDLE};
		/** @brief Last bound bindless descriptor set handle. */
		VkDescriptorSet lastBindlessDS{VK_NULL_HANDLE};
		/** @brief Whether the viewport/scissor has already been set for this pass. */
		bool viewportSet{false};

		/**
		 * @brief Invalidates all tracked state.
		 *
		 * Called when the pipeline changes, since Vulkan descriptor set binding
		 * validity depends on pipeline layout compatibility.
		 */
		void
		invalidateAll () noexcept
		{
			lastPipeline = VK_NULL_HANDLE;
			lastGeometry = nullptr;
			lastLayerIndex = UINT32_MAX;
			lastMaterialDS = VK_NULL_HANDLE;
			lastLightDS = VK_NULL_HANDLE;
			lastViewDS = VK_NULL_HANDLE;
			lastBindlessDS = VK_NULL_HANDLE;
			viewportSet = false;
		}

		/**
		 * @brief Invalidates descriptor set state (keeps pipeline and geometry).
		 *
		 * Called when pipeline changes but we want to keep geometry tracking
		 * (geometry binds are independent of pipeline layout).
		 */
		void
		invalidateDescriptorSets () noexcept
		{
			lastMaterialDS = VK_NULL_HANDLE;
			lastLightDS = VK_NULL_HANDLE;
			lastViewDS = VK_NULL_HANDLE;
			lastBindlessDS = VK_NULL_HANDLE;
		}

#ifdef DEBUG
		/** @brief Number of pipeline bind calls saved. */
		uint32_t savedPipelineBinds{0};
		/** @brief Number of geometry bind calls saved. */
		uint32_t savedGeometryBinds{0};
		/** @brief Number of material descriptor set bind calls saved. */
		uint32_t savedMaterialBinds{0};
		/** @brief Number of viewport/scissor set calls saved. */
		uint32_t savedViewportSets{0};
		/** @brief Total draw calls processed through this tracker. */
		uint32_t totalDrawCalls{0};
#endif
	};
}
