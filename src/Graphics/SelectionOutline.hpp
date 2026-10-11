/*
 * src/Graphics/SelectionOutline.hpp
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
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions for usages. */
#include "Math/Space3D/AACuboid.hpp"
#include "PixelFactory/Color.hpp"

namespace EmEn
{
	namespace Vulkan
	{
		class CommandBuffer;
		class DescriptorSet;
		class DescriptorSetLayout;
		class Framebuffer;
		class GraphicsPipeline;
		class PipelineLayout;
		class Sampler;
	}

	namespace Scenes
	{
		class Scene;
	}

	namespace Graphics
	{
		class GrabPass;
		class Renderer;
		class SelectionDepthTarget;
		class ViewMatricesInterface;
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief Draws the OUTLINE of the highlighted entities (Scenes::Scene::setHighlightedEntities()): a thin line of constant
	 * pixel width around it, FULL where the entity is visible, DIMMED where other geometry hides it.
	 * @note Screen-space, the "custom depth" scheme (Unreal's CustomDepth; owner decisions 2026-09-28):
	 * 1. recordDepth() — after the scene pass, the highlighted entities alone are drawn into a SelectionDepthTarget with
	 *    the main camera, unjittered, through the depth-only shadow-casting programs (skinning, alpha test and wind
	 *    included: the outline follows what the GPU drew);
	 * 2. recordComposite() — in the final composite pass, AFTER the tone mapping (neither exposed nor blurred by the
	 *    TAA): each pixel outside the entity looks for the nearest entity pixel within the width; found, it is outline,
	 *    and its opacity says whether that entity pixel is the scene's front-most surface (the grab pass depth).
	 *    The disk search costs ~width² taps per pixel (RTX 3070 Ti, 2880x1620, full screen: 1 px 0.10 ms, 2 px
	 *    0.20 ms, 4 px 0.56 ms, 8 px 1.81 ms), so the pass is SCISSORED to the entities' projected world render boxes
	 *    (Scenes::Scene::highlightedWorldBoundingBoxes(), their union) grown by the width: only the pixels that may be outline pay.
	 * @note Owned by the Renderer; internal-target frames only (no scene depth copy in the direct swap-chain path).
	 */
	class EMEN_API SelectionOutline final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SelectionOutline"};

			/** @brief The widest outline the composite kernel is built for, in pixels. */
			static constexpr auto MaxWidth{8.0F};

			/**
			 * @brief Constructs the selection outline.
			 * @param renderer A reference to the renderer.
			 */
			explicit SelectionOutline (Renderer & renderer) noexcept;

			SelectionOutline (const SelectionOutline & copy) noexcept = delete;
			SelectionOutline (SelectionOutline && copy) noexcept = delete;
			SelectionOutline & operator= (const SelectionOutline & copy) noexcept = delete;
			SelectionOutline & operator= (SelectionOutline && copy) noexcept = delete;

			/**
			 * @brief Destructs the selection outline.
			 */
			~SelectionOutline ();

			/**
			 * @brief Sets the outline colour, as DISPLAYED (sRGB).
			 * @param color A reference to a colour.
			 */
			void
			setColor (const Base::PixelFactory::Color< float > & color) noexcept
			{
				m_color = color;
			}

			/**
			 * @brief Sets the outline width, in pixels.
			 * @param pixels The width, clamped to [1, MaxWidth].
			 */
			void setWidth (float pixels) noexcept;

			/**
			 * @brief Sets the opacity of the outline where the entity is HIDDEN behind other geometry.
			 * @param opacity In [0, 1]: 0 shows the visible parts only, 1 draws the hidden ones full (x-ray).
			 */
			void setHiddenOpacity (float opacity) noexcept;

			/**
			 * @brief Draws the highlighted entities' depth into the selection depth target.
			 * @note Render thread, outside any render pass, after the scene's prepareRender(). Creates the target on
			 * first use and recreates it when the scene extent changes.
			 * @param commandBuffer A reference to the frame's command buffer.
			 * @param scene A reference to the scene.
			 * @param width The scene width, in pixels.
			 * @param height The scene height, in pixels.
			 * @param mainViewMatrices A reference to the main camera's view matrices.
			 * @return bool True when an outline source was drawn: recordComposite() has something to show.
			 */
			[[nodiscard]]
			bool recordDepth (const Vulkan::CommandBuffer & commandBuffer, Scenes::Scene & scene, uint32_t width, uint32_t height, ViewMatricesInterface & mainViewMatrices) noexcept;

			/**
			 * @brief Composites the outline over the final image.
			 * @note Inside the final composite render pass, after the direct (tone mapping) effects, before the overlays.
			 * @param commandBuffer A reference to the frame's command buffer.
			 * @param framebuffer A reference to the composite pass framebuffer (its render pass seals the pipeline).
			 * @param width The composite width, in pixels.
			 * @param height The composite height, in pixels.
			 * @param grabPass A pointer to the post-process grab pass (the scene depth), nullptr for none.
			 * @param mainViewMatrices A reference to the main camera's view matrices (the clip planes).
			 * @param sRGBTarget Whether the composite target encodes sRGB on write (the colour is then written linear).
			 */
			void recordComposite (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::Framebuffer & framebuffer, uint32_t width, uint32_t height, const GrabPass * grabPass, const ViewMatricesInterface & mainViewMatrices, bool sRGBTarget) noexcept;

			/**
			 * @brief Releases every GPU resource (while the device still exists).
			 */
			void destroy () noexcept;

		private:

			/**
			 * @brief Creates the resources shared by every frame: sampler, layouts, descriptor sets.
			 * @return bool
			 */
			[[nodiscard]]
			bool createSharedResources () noexcept;

			/**
			 * @brief Creates the composite pipeline against a render pass.
			 * @param framebuffer A reference to the composite framebuffer.
			 * @param width The composite width (the viewport is dynamic: the initial value only).
			 * @param height The composite height.
			 * @return bool
			 */
			[[nodiscard]]
			bool createPipeline (const Vulkan::Framebuffer & framebuffer, uint32_t width, uint32_t height) noexcept;

			/**
			 * @brief Computes the normalised screen area the composite is scissored to: the union of the highlighted
			 * entities' projected world render boxes. The whole screen when one box is unknown; empty when every entity is
			 * behind the eye.
			 * @param worldBoundingBoxes The published world render boxes (an invalid one = unknown).
			 * @param mainViewMatrices A reference to the main camera's view matrices.
			 * @param readStateIndex The render state slot the frame draws.
			 */
			void updateScreenArea (const std::vector< Base::Math::Space3D::AACuboid< float > > & worldBoundingBoxes, const ViewMatricesInterface & mainViewMatrices, uint32_t readStateIndex) noexcept;

			/**
			 * @brief Projects one world box with the main camera, clipped against the eye plane.
			 * @param worldBoundingBox A reference to a valid world box.
			 * @param mainViewMatrices A reference to the main camera's view matrices.
			 * @param readStateIndex The render state slot the frame draws.
			 * @return std::array< float, 4 > Minimum U, minimum V, maximum U, maximum V; empty (minimum above maximum) when
			 * the whole box is behind the eye.
			 */
			[[nodiscard]]
			static std::array< float, 4 > projectedArea (const Base::Math::Space3D::AACuboid< float > & worldBoundingBox, const ViewMatricesInterface & mainViewMatrices, uint32_t readStateIndex) noexcept;

			Renderer & m_renderer;
			std::shared_ptr< SelectionDepthTarget > m_depthTarget;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::shared_ptr< Vulkan::GraphicsPipeline > m_pipeline;
			/** @brief The render pass the pipeline was sealed against: a recreated swap chain brings a new one. */
			VkRenderPass m_pipelineRenderPass{VK_NULL_HANDLE};
			Base::PixelFactory::Color< float > m_color{1.0F, 0.6F, 0.1F, 1.0F};
			/** @brief The composite's screen area, normalised [0, 1]: minimum U, minimum V, maximum U, maximum V; a minimum
			 * above its maximum = empty (nothing to draw). */
			std::array< float, 4 > m_screenArea{0.0F, 0.0F, 1.0F, 1.0F};
			float m_width{2.0F};
			float m_hiddenOpacity{0.35F};
			bool m_sharedResourcesCreated{false};
	};
}
