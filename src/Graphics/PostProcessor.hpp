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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <chrono>
#include <memory>
#include <vector>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "DirectPostProcessEffect.hpp"
#include "PrimaryServices.hpp"
#include "RenderTarget/Abstract.hpp"

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

	namespace Scenes
	{
		class LightSet;

		namespace Component
		{
			class Camera;
		}
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

	namespace Resources
	{
		class Manager;
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
	class EMEN_API PostProcessor final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"PostProcessorService"};

			/* GLSL variables. */
			static constexpr auto Fragment{"em_Fragment"};

			/**
			 * @brief Push constants matching the GLSL pcPostProcessing layout.
			 */
			struct EMEN_API PushConstants
			{
				float frameWidth;
				float frameHeight;
				float time;
				float nearPlane;
				float farPlane;
				float tanHalfFovY;
				/** @brief Duration of the previous RENDERED frame, in seconds. Single source of
				 * truth for the whole chain: the velocity G-buffer holds a PER-FRAME delta, so any
				 * effect converting it to a physical duration (the motion blur shutter angle) needs
				 * the same number. Clamped to a sane range, so a hitch or the first frame cannot
				 * produce an absurd exposure. Zero on the direct (lens) chain, which has no
				 * temporal consumer. */
				float deltaTime;
			};

			/* Construction & configuration. */

			/**
			 * @brief Constructs the post-processor service.
			 * @param primaryServices A reference to the primary services.
			 * @param resourcesManager A reference to the resource manager.
			 */
			PostProcessor (PrimaryServices & primaryServices, Resources::Manager & resourcesManager) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			PostProcessor (const PostProcessor & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			PostProcessor (PostProcessor && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return PostProcessor &
			 */
			PostProcessor & operator= (const PostProcessor & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return PostProcessor &
			 */
			PostProcessor & operator= (PostProcessor && copy) noexcept = delete;

			/**
			 * @brief Destructs the post-processor service.
			 * @note Declared here and defined out-of-line (not implicit): the class is exported
			 * (EMEN_API), which forces MSVC to instantiate the destructor at the class definition
			 * point in every TU. It destroys a std::unique_ptr< GrabPass > held by pointer to a
			 * forward-declared (incomplete) type, so the deleter needs the complete type — only
			 * visible in PostProcessor.cpp. See docs/windows-export-api.md § "exported pimpl".
			 */
			~PostProcessor () override;

			/**
			 * @brief Configures the post-processor over a render-target with explicit requirements.
			 * @param renderTarget A reference to a render-target.
			 * @param requiresHDR Whether the scene effects require HDR.
			 * @param requiresDepth Whether the scene effects require depth.
			 * @param requiresNormals Whether the scene effects require normals.
			 * @param requiresMaterialProperties Whether the scene effects require material properties.
			 * @param requiresAlbedo Whether the scene effects require albedo.
			 * @return bool
			 */
			[[nodiscard]]
			bool configure (const std::shared_ptr< RenderTarget::Abstract > & renderTarget, bool requiresHDR, bool requiresDepth, bool requiresNormals, bool requiresMaterialProperties, bool requiresAlbedo, bool requiresVelocity) noexcept;

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
			 * @brief Returns the cached material properties requirement.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			cachedRequiresMaterialProperties () const noexcept
			{
				return m_cachedRequiresMaterialProperties;
			}

			/**
			 * @brief Returns the cached albedo requirement.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			cachedRequiresAlbedo () const noexcept
			{
				return m_cachedRequiresAlbedo;
			}

			/**
			 * @brief Returns the cached velocity requirement (motion vectors).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			cachedRequiresVelocity () const noexcept
			{
				return m_cachedRequiresVelocity;
			}

			/**
			 * @brief Updates the cached requirements without reconfiguring GPU resources.
			 * @note Call this before recreateSceneTarget() so it picks up correct formats.
			 * @param requiresHDR Whether the scene effects require HDR.
			 * @param requiresDepth Whether the scene effects require depth.
			 * @param requiresNormals Whether the scene effects require normals.
			 * @param requiresMaterialProperties Whether the scene effects require material properties.
			 * @param requiresAlbedo Whether the scene effects require albedo.
			 * @return void
			 */
			void
			updateCachedRequirements (bool requiresHDR, bool requiresDepth, bool requiresNormals, bool requiresMaterialProperties, bool requiresAlbedo, bool requiresVelocity) noexcept
			{
				m_cachedRequiresHDR = requiresHDR;
				m_cachedRequiresDepth = requiresDepth;
				m_cachedRequiresNormals = requiresNormals;
				m_cachedRequiresMaterialProperties = requiresMaterialProperties;
				m_cachedRequiresAlbedo = requiresAlbedo;
				m_cachedRequiresVelocity = requiresVelocity;
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
			 * @param lightSet A reference to the scene light set.
			 * @return bool
			 */
			bool executeIndirectPostProcessEffects (const Vulkan::CommandBuffer & commandBuffer, const PostProcessStack & stack, const Scenes::LightSet * lightSet, const Scenes::Component::Camera * activeCamera) const noexcept;

			/**
			 * @brief Executes single-pass camera lens effects as a fullscreen quad.
			 * @note Must be called inside an active render pass.
			 * Generates or retrieves a cached shader program from the effects list,
			 * then renders a fullscreen quad with that program.
			 * @param commandBuffer A reference to the active command buffer.
			 * @param lensEffects The camera's lens effects list (maybe empty for passthrough).
			 * @return bool
			 */
			bool executeDirectPostProcessEffects (const Vulkan::CommandBuffer & commandBuffer, const std::vector< std::shared_ptr< DirectPostProcessEffect > > & lensEffects) const noexcept;

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

			PrimaryServices & m_primaryServices;
			Resources::Manager & m_resourcesManager;
			Renderer & m_renderer;
			std::unique_ptr< GrabPass > m_grabPass;
			std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_descriptorSets;
			std::shared_ptr< Geometry::IndexedVertexResource > m_quadGeometry;
			float m_nearPlane{0.1F};
			float m_farPlane{1000.0F};
			/** @brief Timestamp of the previous indirect-chain execution, for PushConstants::deltaTime.
			 * Mutable because the chain executes from a const method (same idiom as
			 * ViewMatrices2DUBO's cached jittered projection). */
			mutable std::chrono::steady_clock::time_point m_lastChainFrameTime{};
			bool m_enabled{false};
			/* Cached requirements from configure(). */
			bool m_cachedRequiresHDR{false};
			bool m_cachedRequiresDepth{false};
			bool m_cachedRequiresNormals{false};
			bool m_cachedRequiresMaterialProperties{false};
			bool m_cachedRequiresAlbedo{false};
			bool m_cachedRequiresVelocity{false};
	};
}
