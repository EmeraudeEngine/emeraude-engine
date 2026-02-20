/*
 * src/Graphics/Renderer.hpp
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
#include <cstddef>
#include <cstdint>
#include <any>
#include <array>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

/* Local inclusions for inheritances. */
#include "Console/ControllableTrait.hpp"
#include "Libs/ObserverTrait.hpp"
#include "Libs/ObservableTrait.hpp"
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "ExternalInput.hpp"
#include "Recorder.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/Time/Statistics/RealTime.hpp"
#include "RenderTarget/Abstract.hpp"
#include "Saphir/ShaderManager.hpp"
#include "BindlessTextureManager.hpp"
#include "SharedUBOManager.hpp"
#include "VertexBufferFormatManager.hpp"
#include "Vulkan/Sync/Fence.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/TransferManager.hpp"
#include "Vulkan/SwapChain.hpp"
#include "Window.hpp"
#include "Resources/Manager.hpp"
#include "TextureResource/TextureCubemap.hpp"
#include "GrabPass.hpp"
#include "PostProcessor.hpp"
#include "SceneRenderTarget.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class Device;
		class DescriptorPool;
		class CommandPool;
		class CommandBuffer;
		class GraphicsPipeline;
		class Queue;
		class Sampler;
	}

	namespace Graphics
	{
		class DummyColorProjectionTexture;
		class DummyShadowTexture;

		namespace Material
		{
			class Interface;
		}

		namespace Geometry
		{
			class Interface;
		}

		namespace RenderableInstance
		{
			class Abstract;
		}

		namespace RenderTarget
		{
			class Abstract;
		}
	}

	namespace Saphir
	{
		class Program;
	}

	namespace Scenes
	{
		class Scene;
	}

	namespace Overlay
	{
		class Manager;
	}

	class PrimaryServices;
}

namespace EmEn::Graphics
{
	/**
	 * @brief Declares the scope of one renderer frame.
	 */
	class RendererFrameScope final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RendererFrameScope"};

			/**
			 * @brief Constructs a render frame scope.
			 */
			RendererFrameScope () noexcept = default;

			/**
			 * @brief Initializes the command pool and the command buffer.
			 * @param device A reference to the graphics device smart pointer.
			 * @param frameIndex The frame index.
			 * @return bool
			 */
			[[nodiscard]]
			bool initialize (const std::shared_ptr< Vulkan::Device > & device, uint32_t frameIndex) noexcept;

			/**
			 * @brief Declares a semaphore to be waited on by a later submission.
			 * @note The semaphore is added to exactly ONE list (primary or secondary), never both.
			 * Primary semaphores (shadow maps) are consumed by render-to-textures, then forwarded
			 * to secondary via promotePrimaryToSecondary(). Secondary semaphores are waited on
			 * by the final frame submission. Binary semaphores can only be waited on once per signal.
			 * @param semaphore A reference to a semaphore smart pointer.
			 * @param primary True for shadow map semaphores (consumed by RTTs), false for RTT semaphores.
			 * @return void
			 */
			void declareSemaphore (const std::shared_ptr< Vulkan::Sync::Semaphore > & semaphore, bool primary) noexcept;

			/**
			 * @brief Returns the command pool smart pointer.
			 * @return std::shared_ptr< Vulkan::CommandPool >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::CommandPool >
			commandPool () const noexcept
			{
				return m_commandPool;
			}

			/**
			 * @brief Returns the command buffer smart pointer for a render-target.
			 * @param renderTarget A pointer to a render target.
			 * @return std::shared_ptr< Vulkan::CommandPool >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::CommandBuffer > getCommandBuffer (const RenderTarget::Abstract * renderTarget) noexcept;

			/**
			 * @brief Returns the frame index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			frameIndex () const noexcept
			{
				return m_frameIndex;
			}

			/**
			 * @brief Returns primary semaphores ready to use with vkQueueSubmit().
			 * @return Libs::Storage< VkSemaphore, 16 > &
			 */
			[[nodiscard]]
			Libs::StaticVector< VkSemaphore, 16 > &
			primarySemaphores () noexcept
			{
				return m_primarySemaphores;
			}

			/**
			 * @brief Returns secondary semaphores ready to use with vkQueueSubmit().
			 * @return Libs::Storage< VkSemaphore, 16 > &
			 */
			[[nodiscard]]
			Libs::StaticVector< VkSemaphore, 16 > &
			secondarySemaphores () noexcept
			{
				return m_secondarySemaphores;
			}

			/**
			 * @brief Forwards unconsumed primary semaphores to the secondary list.
			 * @note Call this after render-to-textures to ensure shadow map semaphores
			 * are waited on by the final submit when no RTT consumed them.
			 * @return void
			 */
			void
			promotePrimaryToSecondary () noexcept
			{
				for ( const auto sem : m_primarySemaphores )
				{
					m_secondarySemaphores.emplace_back(sem);
				}

				m_primarySemaphores.clear();
			}

			/**
			 * @brief Returns the in-flight fence.
			 * @return Vulkan::Sync::Fence *
			 */
			[[nodiscard]]
			Vulkan::Sync::Fence *
			inFlightFence () const noexcept
			{
				return m_inFlightFence.get();
			}

			/**
			 * @brief Returns the image available semaphore.
			 * @return Vulkan::Sync::Semaphore *
			 */
			[[nodiscard]]
			Vulkan::Sync::Semaphore *
			imageAvailableSemaphore () const noexcept
			{
				return m_imageAvailableSemaphore.get();
			}

			/**
			 * @brief Returns the image finished semaphore.
			 * @return Vulkan::Sync::Semaphore *
			 */
			[[nodiscard]]
			Vulkan::Sync::Semaphore *
			renderFinishedSemaphore () const noexcept
			{
				return m_renderFinishedSemaphore.get();
			}

			/**
			 * @brief Clears all command buffers and semaphores for a next frame usage.
			 * @return bool
			 */
			bool
			prepareForNewFrame () noexcept
			{
				m_primarySemaphores.clear();
				m_secondarySemaphores.clear();

				return m_commandPool->resetCommandBuffers(false);
			}

		private:

			/**
			 * @brief Returns the frame name.
			 * @param frameIndex The number of the frame.
			 * @return std::string
			 */
			[[nodiscard]]
			static
			std::string
			getFrameName (uint32_t frameIndex) noexcept
			{
				std::stringstream frameName;

				frameName << "Frame" << frameIndex;

				return frameName.str();
			}

			std::shared_ptr< Vulkan::CommandPool > m_commandPool;
			std::unordered_map< const RenderTarget::Abstract *, std::shared_ptr< Vulkan::CommandBuffer > > m_commandBuffers;
			Libs::StaticVector< VkSemaphore, 16 > m_primarySemaphores;
			Libs::StaticVector< VkSemaphore, 16 > m_secondarySemaphores;
			/* Synchronization. */
			std::unique_ptr< Vulkan::Sync::Fence > m_inFlightFence;
			std::unique_ptr< Vulkan::Sync::Semaphore > m_imageAvailableSemaphore;
			std::unique_ptr< Vulkan::Sync::Semaphore > m_renderFinishedSemaphore;
			uint32_t m_frameIndex{0};
	};

	/**
	 * @brief The graphics renderer service class.
	 * @note [OBS][STATIC-OBSERVER][STATIC-OBSERVABLE]
	 * @extends EmEn::ServiceInterface The renderer is a service.
	 * @extends EmEn::Libs::ObserverTrait The renderer needs to observe handle changes, for instance.
	 * @extends EmEn::Libs::ObservableTrait The renderer can notify surface changes.
	 * @extends EmEn::Console::ControllableTrait The console can control the renderer.
	 * 
	 * # Performance Optimizations (Cache Lookups)
	 *
	 * The Renderer uses std::unordered_map for cache structures to achieve O(1) average
	 * lookup performance instead of O(log n) with std::map.
	 *
	 * **Optimized caches:**
	 * - `m_samplers`: Sampler cache (O(log S) → O(1))
	 * - `m_pipelines`: GraphicsPipeline cache (O(log G) → O(1))
	 * - `m_programs`: Saphir Program cache (O(log P) → O(1))
	 *
	 * **Expected performance impact:** 10-20% improvement on Graphics hot paths.
	 *
	 * **Validation:** Run `ctest -R RendererPerformance` to benchmark improvements.
	 *
	 * # Thread Safety
	 *
	 * All cache maps (m_programs, m_pipelines, m_samplers) are written during initialization
	 * phase only. Once rendering begins, maps are read-only from the render thread
	 * (no mutex needed for O(1) lookup performance).
	 *
	 * **Future consideration:** If dynamic pipeline creation is needed during runtime,
	 * protect cache modifications with std::shared_mutex (read-write lock).
	 */
	class Renderer final : public ServiceInterface, public Libs::ObserverTrait, public Libs::ObservableTrait, public Console::ControllableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RendererService"};

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				WindowContentRefreshed,
				/* Enumeration boundary. */
				MaxEnum
			};

			/**
			 * @brief Constructs the graphics renderer.
			 * @param primaryServices A reference to primary services.
			 * @param instance A reference to the Vulkan instance.
			 * @param window A reference to a handle.
			 */
			Renderer (PrimaryServices & primaryServices, Vulkan::Instance & instance, Window & window) noexcept;

			/**
			 * @brief Destructs the graphics renderer.
			 */
			~Renderer () override = default;

			/**
			 * @brief Returns the unique identifier for this class [Thread-safe].
			 * @return size_t
			 */
			static
			size_t
			getClassUID () noexcept
			{
				return Libs::Hash::FNV1a(ClassId);
			}

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return getClassUID();
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == getClassUID();
			}

			/**
			 * @brief Returns the windows.
			 * @return const Window &
			 */
			const Window &
			window () const noexcept
			{
				return m_window;
			}

			/**
			 * @brief Returns the windows.
			 * @return Window &
			 */
			Window &
			window () noexcept
			{
				return m_window;
			}

			/**
			 * @brief Returns the reference to the primary services.
			 * @return PrimaryServices &
			 */
			[[nodiscard]]
			PrimaryServices &
			primaryServices () const noexcept
			{
				return m_primaryServices;
			}

			/**
			 * @brief Return the vulkan instance reference.
			 * @return const Vulkan::Instance &
			 */
			[[nodiscard]]
			const Vulkan::Instance &
			vulkanInstance () const noexcept
			{
				return m_vulkanInstance;
			}

			/**
			 * @brief Returns the reference to the transfer manager.
			 * @return Vulkan::TransferManager &
			 */
			[[nodiscard]]
			Vulkan::TransferManager &
			transferManager () noexcept
			{
				return m_transferManager;
			}

			/**
			 * @brief Returns the reference to the transfer manager.
			 * @return const Vulkan::TransferManager &
			 */
			[[nodiscard]]
			const Vulkan::TransferManager &
			transferManager () const noexcept
			{
				return m_transferManager;
			}

			/**
			 * @brief Returns the reference to the layout manager.
			 * @return Vulkan::LayoutManager &
			 */
			[[nodiscard]]
			Vulkan::LayoutManager &
			layoutManager () noexcept
			{
				return m_layoutManager;
			}

			/**
			 * @brief Returns the reference to the layout manager.
			 * @return const Vulkan::LayoutManager &
			 */
			[[nodiscard]]
			const Vulkan::LayoutManager &
			layoutManager () const noexcept
			{
				return m_layoutManager;
			}

			/**
			 * @brief Returns the reference to the shader manager service.
			 * @return Saphir::ShaderManager &
			 */
			[[nodiscard]]
			Saphir::ShaderManager &
			shaderManager () noexcept
			{
				return m_shaderManager;
			}

			/**
			 * @brief Returns the reference to the shader manager service.
			 * @return const Saphir::ShaderManager &
			 */
			[[nodiscard]]
			const Saphir::ShaderManager &
			shaderManager () const noexcept
			{
				return m_shaderManager;
			}

			/**
			 * @brief Returns the reference to the shared UBO manager.
			 * @return SharedUBOManager &
			 */
			[[nodiscard]]
			SharedUBOManager &
			sharedUBOManager () noexcept
			{
				return m_sharedUBOManager;
			}

			/**
			 * @brief Returns the reference to the shared UBO manager.
			 * @return const SharedUBOManager &
			 */
			[[nodiscard]]
			const SharedUBOManager &
			sharedUBOManager () const noexcept
			{
				return m_sharedUBOManager;
			}

			/**
			 * @brief Returns the reference to the bindless texture manager.
			 * @return BindlessTextureManager &
			 */
			[[nodiscard]]
			BindlessTextureManager &
			bindlessTextureManager () noexcept
			{
				return m_bindlessTextureManager;
			}

			/**
			 * @brief Returns the reference to the bindless texture manager.
			 * @return const BindlessTextureManager &
			 */
			[[nodiscard]]
			const BindlessTextureManager &
			bindlessTextureManager () const noexcept
			{
				return m_bindlessTextureManager;
			}

			/**
			 * @brief Returns the reference to the vertex buffer format manager.
			 * @return VertexBufferFormatManager &
			 */
			[[nodiscard]]
			VertexBufferFormatManager &
			vertexBufferFormatManager () noexcept
			{
				return m_vertexBufferFormatManager;
			}

			/**
			 * @brief Returns the reference to the vertex buffer format manager.
			 * @return const VertexBufferFormatManager &
			 */
			[[nodiscard]]
			const VertexBufferFormatManager &
			vertexBufferFormatManager () const noexcept
			{
				return m_vertexBufferFormatManager;
			}

			/**
			 * @brief Returns the reference to the video external input service.
			 * @return ExternalInput &
			 */
			[[nodiscard]]
			ExternalInput &
			externalInput () noexcept
			{
				return m_externalInput;
			}

			/**
			 * @brief Returns the reference to the video external input service.
			 * @return const ExternalInput &
			 */
			[[nodiscard]]
			const ExternalInput &
			externalInput () const noexcept
			{
				return m_externalInput;
			}

			/**
			 * @brief Returns the reference to the video recorder.
			 * @return Recorder &
			 */
			[[nodiscard]]
			Recorder &
			recorder () noexcept
			{
				return m_recorder;
			}

			/**
			 * @brief Returns the reference to the video recorder.
			 * @return const Recorder &
			 */
			[[nodiscard]]
			const Recorder &
			recorder () const noexcept
			{
				return m_recorder;
			}

			/**
			 * @brief Returns the reference to the post-processor service.
			 * @return PostProcessor &
			 */
			[[nodiscard]]
			PostProcessor &
			postProcessor () noexcept
			{
				return m_postProcessor;
			}

			/**
			 * @brief Returns the reference to the post-processor service.
			 * @return const PostProcessor &
			 */
			[[nodiscard]]
			const PostProcessor &
			postProcessor () const noexcept
			{
				return m_postProcessor;
			}

			/**
			 * @brief Returns whether the debug mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDebugModeEnabled () const noexcept
			{
				return m_debugMode;
			}

			/**
			 * @brief Returns whether the window-less mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isWindowLess () const noexcept
			{
				return m_windowLess;
			}

			/**
			 * @brief Controls the state of shadow maps rendering.
			 * @param state The state.
			 * @return void
			 */
			void
			enableShadowMaps (bool state) noexcept
			{
				m_shadowMapsEnabled = state;
			}

			/**
			 * @brief Returns whether the shadow maps rendering is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShadowMapsEnabled () const noexcept
			{
				return m_shadowMapsEnabled;
			}

			/**
			 * @brief Controls the state of rendering to textures.
			 * @param state The state.
			 * @return void
			 */
			void
			enableRenderToTextures (bool state) noexcept
			{
				m_renderToTexturesEnabled = state;
			}

			/**
			 * @brief Returns whether the rendering to textures is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRenderToTexturesEnabled () const noexcept
			{
				return m_renderToTexturesEnabled;
			}

			/**
			 * @brief Toggles offscreen-rendering.
			 * @return void
			 */
			void
			toggleOffscreenRendering () noexcept
			{
				m_renderToTexturesEnabled = !m_renderToTexturesEnabled;
				m_shadowMapsEnabled = m_renderToTexturesEnabled;
			}

			/**
			 * @brief Controls the state of the TBN space rendering.
			 * @note Visual debug functionality, this needs a geometry shader stage support from the GPU.
			 * @param state The state.
			 * @return void
			 */
			void
			enableTBNSpaceRendering (bool state) noexcept
			{
				m_TBNSpaceRenderingEnabled = state;
			}

			/**
			 * @brief Returns whether the TBN space rendering is enabled.
			 * @note Visual debug functionality, this needs a geometry shader stage support from the GPU.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTBNSpaceRenderingEnabled () const noexcept
			{
				return m_TBNSpaceRenderingEnabled;
			}

			/**
			 * @brief Controls the state of the grab pass.
			 * @param state The state.
			 * @return void
			 */
			void
			enableGrabPass (bool state) noexcept
			{
				m_grabPassEnabled = state;
			}

			/**
			 * @brief Returns whether the grab pass is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isGrabPassEnabled () const noexcept
			{
				return m_grabPassEnabled;
			}

			/**
			 * @brief Returns the grab pass texture.
			 * @return GrabPass *
			 */
			[[nodiscard]]
			GrabPass *
			grabPass () const noexcept
			{
				return m_grabPass.get();
			}

			/**
			 * @brief Sets the clear value for the color buffer for the next rendering.
			 * @param red A scalar value.
			 * @param green A scalar value.
			 * @param blue A scalar value.
			 * @param alpha A scalar value. Default 1.
			 * @return void
			 */
			template< typename data_t = float >
			void
			setClearColor (data_t red, data_t green, data_t blue, data_t alpha = 1) noexcept requires (std::is_floating_point_v< data_t >)
			{
				m_clearColors[0].color.float32[0] = Libs::Math::clampToUnit(static_cast< float >(red));
				m_clearColors[0].color.float32[1] = Libs::Math::clampToUnit(static_cast< float >(green));
				m_clearColors[0].color.float32[2] = Libs::Math::clampToUnit(static_cast< float >(blue));
				m_clearColors[0].color.float32[3] = Libs::Math::clampToUnit(static_cast< float >(alpha));
				m_swapChainClearColors[0] = m_clearColors[0];
			}

			/**
			 * @brief Sets the clear value for the color buffer for the next rendering.
			 * @param clearColor A reference to a color.
			 * @return void
			 */
			void
			setClearColor (const Libs::PixelFactory::Color< float > & clearColor) noexcept
			{
				m_clearColors[0].color.float32[0] = clearColor.red();
				m_clearColors[0].color.float32[1] = clearColor.green();
				m_clearColors[0].color.float32[2] = clearColor.blue();
				m_clearColors[0].color.float32[3] = clearColor.alpha();
				m_swapChainClearColors[0] = m_clearColors[0];
			}

			/**
			 * @brief Sets the clear values for the depth/stencil buffers for the next rendering.
			 * @param depth The depth value.
			 * @param stencil The stencil value.
			 * @return void
			 */
			void
			setClearDepthStencilValues (float depth, uint32_t stencil) noexcept
			{
				m_clearColors[2].depthStencil.depth = depth;
				m_clearColors[2].depthStencil.stencil = stencil;
				m_swapChainClearColors[1].depthStencil.depth = depth;
				m_swapChainClearColors[1].depthStencil.stencil = stencil;
			}

			/**
			 * @brief Returns the clear color.
			 * @return Libraries::PixelFactory::Color< float >
			 */
			[[nodiscard]]
			Libs::PixelFactory::Color< float >
			getClearColor () const noexcept
			{
				return {
					m_clearColors[0].color.float32[0],
					m_clearColors[0].color.float32[1],
					m_clearColors[0].color.float32[2],
					m_clearColors[0].color.float32[3]
				};
			}

			/**
			 * @brief Returns the depth clear value.
			 * @return float
			 */
			[[nodiscard]]
			float
			getClearDepthValue () const noexcept
			{
				return m_clearColors[2].depthStencil.depth;
			}

			/**
			 * @brief Returns the stencil clear value.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getClearStencilValue () const noexcept
			{
				return m_clearColors[2].depthStencil.stencil;
			}

			/**
			 * @brief Returns the selected logical device used for graphics.
			 * @return std::shared_ptr< Vulkan::Device >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Device >
			device () const noexcept
			{
				return m_device;
			}

			/**
			 * @brief Returns the current swap chain color image (for async GPU readback).
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image > currentSwapChainColorImage () const noexcept;

			/**
			 * @brief Returns the swap chain color format.
			 * @return VkFormat
			 */
			[[nodiscard]]
			VkFormat swapChainColorFormat () const noexcept;

			/**
			 * @brief Returns the swap chain depth/stencil format.
			 * @return VkFormat
			 */
			[[nodiscard]]
			VkFormat swapChainDepthStencilFormat () const noexcept;

			/**
			 * @brief Returns the current swap chain depth/stencil image.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image > currentSwapChainDepthStencilImage () const noexcept;

			/**
			 * @brief Returns whether the internal scene render target is needed.
			 * @note The internal target is required when the post-processor is active,
			 * windowless mode is active, or MSAA is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool needsInternalTarget () const noexcept;

			/**
			 * @brief Returns the current scene color image for post-processing blit.
			 * @note Returns the internal target's color image when active, otherwise the swap chain color.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image > currentSceneColorImage () const noexcept;

			/**
			 * @brief Returns the current scene depth image for post-processing blit.
			 * @note Returns the internal target's depth image when active, otherwise the swap chain depth.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image > currentSceneDepthImage () const noexcept;

			/**
			 * @brief Returns the current scene normals image for post-processing.
			 * @note Returns the internal target's normals image when active, otherwise nullptr.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image > currentSceneNormalsImage () const noexcept;

			/**
			 * @brief Returns the current frame-in-flight index.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			currentFrameIndex () const noexcept
			{
				return m_currentFrameIndex;
			}

			/**
			 * @brief Returns the number of frames in flight.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			framesInFlight () const noexcept
			{
				return static_cast< uint32_t >(m_rendererFrameScope.size());
			}

			/**
			 * @brief Returns the internal scene render target.
			 * @return std::shared_ptr< SceneRenderTarget >
			 */
			[[nodiscard]]
			std::shared_ptr< SceneRenderTarget >
			sceneTarget () const noexcept
			{
				return m_sceneTarget;
			}

			/**
			 * @brief Returns the descriptor pool.
			 * @return std::shared_ptr< Vulkan::DescriptorPool >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::DescriptorPool >
			descriptorPool () const noexcept
			{
				return m_descriptorPool;
			}

			/**
			 * @brief Returns the main render target. The swap chain or an offscreen view.
			 * @return std::shared_ptr< RenderTarget::Abstract >
			 */
			[[nodiscard]]
			std::shared_ptr< RenderTarget::Abstract > mainRenderTarget () const noexcept;

			/**
			 * @brief Returns rendering statistics.
			 * @return const Libraries::Time::Statistics::RealTime< std::chrono::high_resolution_clock > &
			 */
			[[nodiscard]]
			const Libs::Time::Statistics::RealTime< std::chrono::high_resolution_clock > &
			statistics () const noexcept
			{
				return m_statistics;
			}

			/**
			 * @brief Creates graphics resources for default or fall-back behavior.
			 * @param resources A reference to the resource manager.
			 * @return void
			 */
			void createDefaultResources (Resources::Manager & resources) noexcept;

			/**
			 * @brief Clears default graphics resources before shutdown.
			 * @note This must be called before Vulkan resources are destroyed
			 * because these textures have VMA allocations.
			 * @return void
			 */
			void clearDefaultResources () noexcept;

			/**
			 * @brief Returns the default texture cubemap.
			 * @return std::shared_ptr< TextureResource::TextureCubemap >
			 */
			[[nodiscard]]
			std::shared_ptr< TextureResource::TextureCubemap >
			getDefaultTextureCubemap () const noexcept
			{
				return m_defaultTextureCubemap;
			}

			/**
			 * @brief Returns the dummy 2D shadow texture.
			 * @note This texture is used for lights without shadow mapping to maintain unified descriptor set layouts.
			 * @return std::shared_ptr< DummyShadowTexture >
			 */
			[[nodiscard]]
			std::shared_ptr< DummyShadowTexture >
			getDummyShadowTexture2D () const noexcept
			{
				return m_dummyShadowTexture2D;
			}

			/**
			 * @brief Returns the dummy cubemap shadow texture.
			 * @note This texture is used for point lights without shadow mapping to maintain unified descriptor set layouts.
			 * @return std::shared_ptr< DummyShadowTexture >
			 */
			[[nodiscard]]
			std::shared_ptr< DummyShadowTexture >
			getDummyShadowTextureCube () const noexcept
			{
				return m_dummyShadowTextureCube;
			}

			/**
			 * @brief Returns the dummy 2D color projection texture.
			 * @note This texture is used for lights without color projection to maintain unified descriptor set layouts.
			 * @return std::shared_ptr< DummyColorProjectionTexture >
			 */
			[[nodiscard]]
			std::shared_ptr< DummyColorProjectionTexture >
			getDummyColorProjectionTexture2D () const noexcept
			{
				return m_dummyColorProjectionTexture2D;
			}

			/**
			 * @brief Returns the dummy cubemap color projection texture.
			 * @note This texture is used for point lights without color projection to maintain unified descriptor set layouts.
			 * @return std::shared_ptr< DummyColorProjectionTexture >
			 */
			[[nodiscard]]
			std::shared_ptr< DummyColorProjectionTexture >
			getDummyColorProjectionTextureCube () const noexcept
			{
				return m_dummyColorProjectionTextureCube;
			}

			/**
			 * @brief Finalizes the graphics pipeline creation by replacing it with a similar or create and cache this new one.
			 * @param renderTarget A reference to a render target.
			 * @param program A reference to the program.
			 * @param graphicsPipeline A reference to the graphics pipeline smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool finalizeGraphicsPipeline (const RenderTarget::Abstract & renderTarget, const Saphir::Program & program, std::shared_ptr< Vulkan::GraphicsPipeline > & graphicsPipeline) noexcept;

			/**
			 * @brief Finalizes the graphics pipeline creation using a specific framebuffer (and its render pass).
			 * @param framebuffer A reference to the framebuffer to use for pipeline finalization.
			 * @param program A reference to the program.
			 * @param graphicsPipeline A reference to the graphics pipeline smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool finalizeGraphicsPipeline (const Vulkan::Framebuffer & framebuffer, const Saphir::Program & program, std::shared_ptr< Vulkan::GraphicsPipeline > & graphicsPipeline) noexcept;

			/**
			 * @brief Returns the post-process framebuffer used for overlay rendering.
			 * @note In windowed mode, returns the swap-chain's post-process framebuffer (single-sample).
			 * In windowless mode, returns the main framebuffer.
			 * @return const Vulkan::Framebuffer *
			 */
			[[nodiscard]]
			const Vulkan::Framebuffer * overlayFramebuffer () const noexcept;

			/**
			 * @brief Finds a cached shader program by its unique key.
			 * @note This allows early lookup before shader generation to avoid redundant work.
			 * @param programKey The unique key identifying the program configuration.
			 * @return std::shared_ptr< Saphir::Program > The cached program or nullptr if not found.
			 */
			[[nodiscard]]
			std::shared_ptr< Saphir::Program > findCachedProgram (size_t programKey) const noexcept;

			/**
			 * @brief Caches a shader program for future reuse.
			 * @param programKey The unique key identifying the program configuration.
			 * @param program The program to cache.
			 * @return bool True if the program was successfully cached.
			 */
			bool cacheProgram (size_t programKey, const std::shared_ptr< Saphir::Program > & program) noexcept;

			/**
			 * @brief Notifies that a cached program was reused.
			 * @note This increments the reuse counter for statistics.
			 * @return void
			 */
			void
			notifyProgramReused () noexcept
			{
				m_programReusedCount++;
			}

			/**
			 * @brief Returns or creates a sampler.
			 * @param identifier A string.
			 * @param setupCreateInfo A function to configure the creation info structure.
			 * @return std::shared_ptr< Vulkan::Sampler >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler > getSampler (const std::string & identifier, const std::function< void (Settings & settings, VkSamplerCreateInfo &) > & setupCreateInfo) noexcept;

			/**
			 * @brief Render a new offscreen frame for the active scene.
			 * @param scene A reference to the scene smart pointer.
			 * @param overlayManager A reference to the overlay manager.
			 * @return void
			 */
			void renderOffscreenFrame (const std::shared_ptr< Scenes::Scene > & scene, const Overlay::Manager & overlayManager) noexcept;

			/**
			 * @brief Render a new frame for the active scene.
			 * @param scene A reference to the scene smart pointer.
			 * @param overlayManager A reference to the overlay manager.
			 * @return void
			 */
			void renderFrame (const std::shared_ptr< Scenes::Scene > & scene, const Overlay::Manager & overlayManager) noexcept;

			/**
			 * @brief Captures the framebuffer.
			 * @param result Array to fill: [0] = color, [1] = depth (optional), [2] = stencil (optional).
			 * @param keepAlpha Keep the alpha channel from the GPU memory. Default false.
			 * @param withDepthBuffer Enable to capture the depth buffer. Default false.
			 * @param withStencilBuffer Enable to capture the stencil buffer. Default false.
			 * @param postProcess Enable to swap channels from BGRA to RGBA. Default true.
			 * @return bool
			 */
			bool captureFramebuffer (std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 > & result, bool keepAlpha = false, bool withDepthBuffer = false, bool withStencilBuffer = false, bool postProcess = true) noexcept;

			/**
			 * @brief Sets the swap-chain to status degraded in order to force a refresh.
			 * @return void
			 */
			void
			setSwapChainDegraded () const noexcept
			{
				m_swapChain->setDegraded();
			}

			/**
			 * @brief Tells the Core that the swap-chain must be recreated.
			 * @note This allows the main thread to be in charge of recreating the swap-chain.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSwapChainDegraded () const noexcept
			{
				return m_swapChain->status() == Vulkan::Status::Degraded;
			}

			/**
			 * @brief Requests a graceful shutdown of the renderer.
			 * @note This will signal the renderer to stop producing new frames
			 * and allow any frames in-flight to complete before destruction.
			 * @return void
			 */
			void requestShutdown () noexcept;

			/**
			 * @brief Returns whether a shutdown has been requested.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShutdownRequested () const noexcept
			{
				return m_shutdownRequested;
			}

			/**
			 * @brief Returns whether the software frame rate limiter is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSoftwareFrameLimiterEnabled () const noexcept
			{
				return m_frameDuration.count() > 0;
			}

			[[nodiscard]]
			uint32_t
			pipelineBuiltCount () const noexcept
			{
				return m_pipelineBuiltCount;
			}

			[[nodiscard]]
			uint32_t
			pipelineReusedCount () const noexcept
			{
				return m_pipelineReusedCount;
			}

			[[nodiscard]]
			uint32_t
			programBuiltCount () const noexcept
			{
				return m_programBuiltCount;
			}

			[[nodiscard]]
			uint32_t
			programsReusedCount () const noexcept
			{
				return m_programReusedCount;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const Libs::ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/** @copydoc EmEn::Console::ControllableTrait::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

			/**
			 * @brief Initialize all sub services of the renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool initializeSubServices () noexcept;

			/**
			 * @brief Updates every shadow map from the scene.
			 * @param currentFrameScope A writable reference to the current frame scope, the one being rendered.
			 * @param scene A reference to the scene.
			 * @param queue A pointer to the graphics queue to use for submissions.
			 * @return void
			 */
			void renderShadowMaps (RendererFrameScope & currentFrameScope, Scenes::Scene & scene, const Vulkan::Queue * queue) const noexcept;

			/**
			 * @brief Updates every dynamic texture2Ds from the scene.
			 * @param currentFrameScope A writable reference to the current frame scope, the one being rendered.
			 * @param scene A reference to the scene.
			 * @param queue A pointer to the graphics queue to use for submissions.
			 * @return void
			 */
			void renderRenderToTextures (RendererFrameScope & currentFrameScope, Scenes::Scene & scene, const Vulkan::Queue * queue) const noexcept;

			/**
			 * @brief Updates every off-screen view from the scene.
			 * @param currentFrameScope A writable reference to the current frame scope, the one being rendered.
			 * @param scene A reference to the scene.
			 * @param queue A pointer to the graphics queue to use for submissions.
			 * @return void
			 */
			void renderViews (RendererFrameScope & currentFrameScope, Scenes::Scene & scene, const Vulkan::Queue * queue) const noexcept;

			/**
			 * @brief Renders a frame directly to the swapchain (no internal target).
			 * @note Used when no post-processing and no MSAA is active.
			 * @param scene A reference to the scene smart pointer.
			 * @param overlayManager A reference to the overlay manager.
			 * @param currentFrameScope A reference to the current frame scope.
			 * @param commandBuffer A reference to the command buffer smart pointer.
			 * @return void
			 */
			void renderFrameDirect (const std::shared_ptr< Scenes::Scene > & scene, const Overlay::Manager & overlayManager, RendererFrameScope & currentFrameScope, const std::shared_ptr< Vulkan::CommandBuffer > & commandBuffer) noexcept;

			/**
			 * @brief Renders a frame through the internal scene render target.
			 * @note Used when post-processing is active (HDR pipeline).
			 * Renders scene to internal target, blits to grab pass, runs effects,
			 * then draws final quad + overlay to swapchain.
			 * @param scene A reference to the scene smart pointer.
			 * @param overlayManager A reference to the overlay manager.
			 * @param currentFrameScope A reference to the current frame scope.
			 * @param commandBuffer A reference to the command buffer smart pointer.
			 * @return void
			 */
			void renderFrameWithInternal (const std::shared_ptr< Scenes::Scene > & scene, const Overlay::Manager & overlayManager, RendererFrameScope & currentFrameScope, const std::shared_ptr< Vulkan::CommandBuffer > & commandBuffer) noexcept;

			/**
			 * @brief Recreates the internal scene render target on resize.
			 * @return bool
			 */
			[[nodiscard]]
			bool recreateSceneTarget () noexcept;

			/**
			 * @brief Creates command pools and buffers according to the swap chain image count.
			 * @param imageCount The number of frames for the rendering system.
			 * @return bool
			 */
			bool createRenderingSystem (uint32_t imageCount) noexcept;

			/**
			 * @brief Destroys command pools and buffers.
			 * @return void
			 */
			void destroyRenderingSystem () noexcept;

			/**
			 * @brief Recreates the base of the rendering system.
			 * @param withSurface Set this parameter to true to recreate the swap-chain and the surface.
			 * @param useNativeCode Use the native code to build surface instead the GLFW. Default false.
			 * @return bool
			 */
			[[nodiscard]]
			bool recreateRenderingSubSystem (bool withSurface, bool useNativeCode = false) noexcept;

			PrimaryServices & m_primaryServices;
			Vulkan::Instance & m_vulkanInstance;
			Window & m_window;
			std::shared_ptr< Vulkan::Device > m_device;
			/* NOTE: Fixed graphics queue used for ALL frame rendering.
			 * Shadow map semaphores are binary and shared across frame scopes (they belong
			 * to render targets, not frames). With triple buffering, up to 3 frames can be
			 * in-flight simultaneously. If each frame used a different queue (round-robin),
			 * concurrent signal/wait operations on the same binary semaphore across independent
			 * queues would cause undefined behavior (double-signal on an already-signaled semaphore).
			 * A single queue serializes all submissions in FIFO order, ensuring binary semaphore
			 * state transitions are always correct. This has no meaningful performance impact since
			 * queues from the same family share the same GPU execution units. */
			Vulkan::Queue * m_graphicsQueue{nullptr};
			Vulkan::TransferManager m_transferManager;
			Vulkan::LayoutManager m_layoutManager;
			Saphir::ShaderManager m_shaderManager;
			SharedUBOManager m_sharedUBOManager;
			BindlessTextureManager m_bindlessTextureManager;
			VertexBufferFormatManager m_vertexBufferFormatManager;
			PostProcessor m_postProcessor{*this};
			ExternalInput m_externalInput{m_primaryServices};
			Recorder m_recorder{m_primaryServices, *this};
			std::vector< ServiceInterface * > m_subServicesEnabled;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::shared_ptr< Vulkan::SwapChain > m_swapChain;
			std::shared_ptr< SceneRenderTarget > m_sceneTarget;
			std::vector< std::shared_ptr< SceneRenderTarget > > m_retiredSceneTargets;
			uint32_t m_retiredFrameCountdown{0};
			std::shared_ptr< RenderTarget::Abstract > m_windowLessView;
			Libs::StaticVector< RendererFrameScope, 5 > m_rendererFrameScope;
			std::unordered_map< size_t, std::shared_ptr< Saphir::Program > > m_programs;
			std::unordered_map< size_t, std::shared_ptr< Vulkan::GraphicsPipeline > > m_graphicsPipelines;
			std::unordered_map< std::string, std::shared_ptr< Vulkan::Sampler > > m_samplers;
			Libs::Time::Statistics::RealTime< std::chrono::high_resolution_clock > m_statistics{30};
			std::array< VkClearValue, 3 > m_clearColors{};
			/* Clear values for render targets without the MRT normals attachment (swap chain).
			 * Layout: [0]=color, [1]=depth/stencil (vs m_clearColors: [0]=color, [1]=normals, [2]=depth). */
			std::array< VkClearValue, 2 > m_swapChainClearColors{};
			/* NOTE: Shadow maps only have a depth attachment at index 0, so they need
			 * separate clear values with depth=1.0 at index 0 (not index 1 like main render). */
			std::array< VkClearValue, 1 > m_shadowMapClearValues{VkClearValue{.depthStencil = {1.0F, 0}}};
			uint32_t m_currentFrameIndex{0};
			const uint64_t m_timeout{std::chrono::duration_cast< std::chrono::nanoseconds >(std::chrono::milliseconds(60'000)).count()};
			std::chrono::high_resolution_clock::time_point m_frameStartTime{};
			std::chrono::nanoseconds m_frameDuration{0}; // 0 = frame limiter disabled
			uint32_t m_frameRateLimit{0}; // 0 = disabled, otherwise FPS target
			uint32_t m_pipelineBuiltCount{0};
			uint32_t m_pipelineReusedCount{0};
			uint32_t m_programBuiltCount{0};
			uint32_t m_programReusedCount{0};
			std::shared_ptr< TextureResource::TextureCubemap > m_defaultTextureCubemap;
			std::shared_ptr< DummyShadowTexture > m_dummyShadowTexture2D;
			std::shared_ptr< DummyShadowTexture > m_dummyShadowTextureCube;
			std::shared_ptr< DummyColorProjectionTexture > m_dummyColorProjectionTexture2D;
			std::shared_ptr< DummyColorProjectionTexture > m_dummyColorProjectionTextureCube;
			std::unique_ptr< GrabPass > m_grabPass;
			bool m_debugMode{false};
			bool m_windowLess{false};
			bool m_shadowMapsEnabled{true};
			bool m_renderToTexturesEnabled{true};
			bool m_TBNSpaceRenderingEnabled{false};
			bool m_grabPassEnabled{false};
			bool m_swapChainMustBeRecreated{false};
			bool m_shutdownRequested{false};
	};
}
