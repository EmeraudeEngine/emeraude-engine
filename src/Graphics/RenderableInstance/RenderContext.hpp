/*
 * src/Graphics/RenderableInstance/RenderContext.hpp
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

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class CommandBuffer;
	class PipelineLayout;
}

namespace EmEn::Graphics
{
	class ViewMatricesInterface;
}

namespace EmEn::Graphics::RenderableInstance
{
	/**
	 * @brief Lightweight POD structure holding render pass context.
	 *
	 * This structure encapsulates all render pass-related information needed by
	 * RenderableInstance implementations to configure push constants correctly.
	 *
	 * @note Created once per render pass, shared by all rendered objects.
	 * @note Designed to be cache-friendly (32 bytes) and cheap to copy.
	 *
	 * @par Usage
	 * The `isCubemap` flag is the key discriminator for push constant strategy:
	 * - When false: Classic 2D rendering, matrices via push constants
	 * - When true: Cubemap multiview, View/Projection in UBO indexed by gl_ViewIndex
	 *
	 * @see PushConstantContext For shader-specific push constant configuration.
	 */
	struct RenderPassContext final
	{
		/** @brief The command buffer recording commands for this render pass. */
		const Vulkan::CommandBuffer * commandBuffer{nullptr};

		/** @brief The view matrices interface providing View/Projection data. */
		const ViewMatricesInterface * viewMatrices{nullptr};

		/** @brief The render state index for double/triple buffering synchronization. */
		uint32_t readStateIndex{0};

		/**
		 * @brief Whether the render target is a cubemap using multiview rendering.
		 *
		 * When true, the rendering uses Vulkan multiview extension where:
		 * - All 6 cubemap faces are rendered in a single pass
		 * - View/Projection matrices are stored in a UBO array indexed by gl_ViewIndex
		 * - Push constants only need to provide the Model matrix (Unique) or nothing (Multiple)
		 */
		bool isCubemap{false};

		/**
		 * @brief Whether the render target is a Cascaded Shadow Map using multiview rendering.
		 *
		 * When true, the rendering uses Vulkan multiview extension where:
		 * - All N cascades are rendered in a single pass
		 * - View/Projection matrices are stored in a UBO array indexed by gl_ViewIndex
		 * - Push constants only need to provide the Model matrix (Unique) or nothing (Multiple)
		 */
		bool isCSM{false};
	};

	/**
	 * @brief Lightweight POD structure holding push constant configuration.
	 *
	 * This structure encapsulates shader program-specific information needed to
	 * configure push constants. Values are pre-computed once per program to avoid
	 * redundant calculations during high-frequency draw calls.
	 *
	 * @note Created once per shader program, reused for multiple draw calls.
	 * @note Pre-computes values that would otherwise be calculated per draw call.
	 *
	 * @par Performance
	 * The `stageFlags` field is pre-computed based on whether the program has a
	 * geometry shader, avoiding a conditional check on every vkCmdPushConstants call.
	 *
	 * @see RenderPassContext For render pass-level context.
	 */
	struct PushConstantContext final
	{
		/** @brief The pipeline layout defining push constant ranges. */
		const Vulkan::PipelineLayout * pipelineLayout{nullptr};

		/**
		 * @brief Pre-computed shader stage flags for vkCmdPushConstants.
		 *
		 * Typically VK_SHADER_STAGE_VERTEX_BIT, or combined with
		 * VK_SHADER_STAGE_GEOMETRY_BIT if the program uses a geometry shader.
		 */
		VkShaderStageFlags stageFlags{VK_SHADER_STAGE_VERTEX_BIT};

		/**
		 * @brief Whether the shader needs separate View and Model matrices.
		 *
		 * When true, pushes V and M separately instead of combined MVP.
		 * Required for lighting calculations that need world-space positions.
		 */
		bool useAdvancedMatrices{false};

		/**
		 * @brief Whether the shader uses billboarding (sprites facing camera).
		 *
		 * When true, the View matrix is pushed separately so the shader can
		 * construct a billboard orientation from it.
		 */
		bool useBillboarding{false};
	};
}
