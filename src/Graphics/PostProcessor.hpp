/*
 * src/Graphics/PostProcessor.hpp
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
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "RenderTarget/Abstract.hpp"
#include "DirectPostProcessEffect.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Saphir
	{
		class Program;
	}

	namespace Vulkan
	{
		class CommandBuffer;
		class DescriptorSet;
		class DescriptorSetLayout;
		class LayoutManager;
	}

	namespace Graphics
	{
		class GrabPass;
		class IndirectPostProcessEffect;
		class PostProcessStack;
		class Renderer;

		namespace Geometry
		{
			class IndexedVertexResource;
		}
	}
}

namespace EmEn::Graphics
{
	/**
	 * @brief The post-processor service — a pure GPU executor for fullscreen effects.
	 * @note Effects are owned by Scene (PostProcessStack for multi-pass) and Camera
	 * (lensEffects for single-pass). The PostProcessor only executes them.
	 * @extends EmEn::ServiceInterface The post-processor is a renderer sub-service.
	 */
	class PostProcessor final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PostProcessorService"};

			/* GLSL variables. */
			static constexpr auto Fragment{"em_Fragment"};

			/**
			 * @brief Push constants matching the GLSL pcPostProcessing layout.
			 */
			struct PushConstants
			{
				float frameWidth;
				float frameHeight;
				float time;
				float nearPlane;
				float farPlane;
				float tanHalfFovY;
			};

			/* Construction & configuration. */

			/**
			 * @brief Constructs the post-processor service.
			 * @param renderer A reference to the graphics renderer.
			 */
			explicit PostProcessor (Renderer & renderer) noexcept;

			/**
			 * @brief Configures the post-processor over a render-target with explicit requirements.
			 * @param renderTarget A reference to a render-target.
			 * @param requiresHDR Whether the scene effects require HDR.
			 * @param requiresDepth Whether the scene effects require depth.
			 * @param requiresNormals Whether the scene effects require normals.
			 * @return bool
			 */
			[[nodiscard]]
			bool configure (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, bool requiresHDR, bool requiresDepth, bool requiresNormals) noexcept;

			/* Shared state. */

			/**
			 * @brief Enables or disables the post-processor.
			 * @param state The desired enabled state.
			 * @return void
			 */
			void
			enable (bool state) noexcept
			{
				m_enabled = state;
			}

			/**
			 * @brief Returns whether the post-processor is enabled and ready to render.
			 * @note All cancellation conditions are centralized here for simplicity.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isEnabled () const noexcept
			{
				return m_enabled && this->usable();
			}

			/* Cached requirements — stored from configure(). */

			/**
			 * @brief Returns the cached HDR requirement.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			cachedRequiresHDR () const noexcept
			{
				return m_cachedRequiresHDR;
			}

			/**
			 * @brief Returns the cached depth requirement.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			cachedRequiresDepth () const noexcept
			{
				return m_cachedRequiresDepth;
			}

			/**
			 * @brief Returns the cached normals requirement.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			cachedRequiresNormals () const noexcept
			{
				return m_cachedRequiresNormals;
			}

			/**
			 * @brief Updates the cached requirements without reconfiguring GPU resources.
			 * @note Call this before recreateSceneTarget() so it picks up correct formats.
			 * @param requiresHDR Whether the scene effects require HDR.
			 * @param requiresDepth Whether the scene effects require depth.
			 * @param requiresNormals Whether the scene effects require normals.
			 * @return void
			 */
			void
			updateCachedRequirements (bool requiresHDR, bool requiresDepth, bool requiresNormals) noexcept
			{
				m_cachedRequiresHDR = requiresHDR;
				m_cachedRequiresDepth = requiresDepth;
				m_cachedRequiresNormals = requiresNormals;
			}

			/**
			 * @brief Updates the near and far plane values for depth-based effects.
			 * @param nearPlane The camera near plane distance.
			 * @param farPlane The camera far plane distance.
			 * @return void
			 */
			void
			setClipPlanes (float nearPlane, float farPlane) noexcept
			{
				m_nearPlane = nearPlane;
				m_farPlane = farPlane;
			}

			/* GPU execution. */

			/**
			 * @brief Records the blit from the swap chain color image into the post-processor's own grab pass.
			 * @note Must be called between render pass 1 and render pass 2, outside any active render pass.
			 * @param commandBuffer A reference to the active command buffer.
			 * @return void
			 */
			void recordBlit (const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Executes multi-pass scene effects outside any active render pass.
			 * @note Must be called after recordBlit() and before the RP2 restart.
			 * Each effect in the chain receives the output of the previous one.
			 * After execution, the descriptor set is updated to point to the chain output.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param stack The scene's post-process stack.
			 * @return void
			 */
			void executeIndirectPostProcessEffects (const Vulkan::CommandBuffer & commandBuffer, const PostProcessStack & stack) noexcept;

			/**
			 * @brief Executes single-pass camera lens effects as a fullscreen quad.
			 * @note Must be called inside an active render pass.
			 * Generates or retrieves a cached shader program from the effects list,
			 * then renders a fullscreen quad with that program.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param lensEffects The camera's lens effects list (may be empty for passthrough).
			 * @return void
			 */
			void executeDirectPostProcessEffects (const Vulkan::CommandBuffer & commandBuffer, const std::vector< std::shared_ptr< DirectPostProcessEffect > > & lensEffects) noexcept;

			/* Static. */

			/**
			 * @brief Returns or creates the descriptor set layout for post-processing.
			 * @param layoutManager A reference to the Vulkan layout manager.
			 * @return std::shared_ptr< Vulkan::DescriptorSetLayout >
			 */
			[[nodiscard]]
			static std::shared_ptr< Vulkan::DescriptorSetLayout > getDescriptorSetLayout (Vulkan::LayoutManager & layoutManager) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/* Shared state. */
			Renderer & m_renderer;
			std::unique_ptr< GrabPass > m_grabPass;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			std::shared_ptr< Geometry::IndexedVertexResource > m_quadGeometry;
			float m_nearPlane{0.1F};
			float m_farPlane{1000.0F};
			bool m_enabled{false};

			/* Cached requirements from configure(). */
			bool m_cachedRequiresHDR{false};
			bool m_cachedRequiresDepth{false};
			bool m_cachedRequiresNormals{false};
	};
}
