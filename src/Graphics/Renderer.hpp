/*
 * src/Graphics/Renderer.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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
#include <map>
#include <memory>
#include <vector>
#include <string>

/* Local inclusions for inheritances. */
#include "Console/Controllable.hpp"
#include "Libs/ObservableTrait.hpp"
#include "Libs/ObserverTrait.hpp"
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "ExternalInput.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/Time/Statistics/RealTime.hpp"
#include "RenderTarget/Abstract.hpp"
#include "Saphir/ShaderManager.hpp"
#include "SharedUBOManager.hpp"
#include "VertexBufferFormatManager.hpp"
#include "Vulkan/Sync/Fence.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/TransferManager.hpp"
#include "Window.hpp"

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
		class Sampler;
		class SwapChain;
	}

	namespace Graphics
	{
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
			 * @brief Declares semaphore to wait.
			 * @param semaphore A reference to a semaphore smart pointer.
			 * @param primary Is a primary resource to wait?
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
	 * @extends EmEn::Console::Controllable The console can control the renderer.
	 */
	class Renderer final : public ServiceInterface, public Libs::ObserverTrait, public Console::Controllable
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RendererService"};

			/**
			 * @brief Constructs the graphics renderer.
			 * @param primaryServices A reference to primary services.
			 * @param instance A reference to the Vulkan instance.
			 * @param window A reference to a handle.
			 */
			Renderer (PrimaryServices & primaryServices, Vulkan::Instance & instance, Window & window) noexcept
				: ServiceInterface{ClassId},
				Controllable{ClassId},
				m_primaryServices{primaryServices},
				m_vulkanInstance{instance},
				m_window{window},
				m_shaderManager{primaryServices},
				m_sharedUBOManager{*this},
				m_debugMode{m_vulkanInstance.isDebugModeEnabled()}
			{
				/* Framebuffer clear color value. */
				this->setClearColor(Libs::PixelFactory::Black);

				/* Framebuffer clear depth/stencil values. */
				this->setClearDepthStencilValues(1.0F, 0);

				this->observe(&m_window);
			}

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
				static const size_t classUID = EmEn::Libs::Hash::FNV1a(ClassId);

				return classUID;
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
				m_clearColors[1].depthStencil.depth = depth;
				m_clearColors[1].depthStencil.stencil = stencil;
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
				return m_clearColors[1].depthStencil.depth;
			}

			/**
			 * @brief Returns the stencil clear value.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			getClearStencilValue () const noexcept
			{
				return m_clearColors[1].depthStencil.stencil;
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
			 * @brief Finalizes the graphics pipeline creation by replacing it with a similar or create and cache this new one.
			 * @param renderTarget A reference to a render target.
			 * @param program A reference to the program.
			 * @param graphicsPipeline A reference to the graphics pipeline smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool finalizeGraphicsPipeline (const RenderTarget::Abstract & renderTarget, const Saphir::Program & program, std::shared_ptr< Vulkan::GraphicsPipeline > & graphicsPipeline) noexcept;

			/**
			 * @brief Returns or creates a render pass.
			 * @param identifier A reference to a string.
			 * @param createFlags The createInfo flags. Default none.
			 * @return std::shared_ptr< Vulkan::RenderPass >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::RenderPass > getRenderPass (const std::string & identifier, VkRenderPassCreateFlags createFlags = 0) noexcept;

			/**
			 * @brief Returns or creates a sampler.
			 * @param identifier A string.
			 * @param setupCreateInfo A function to configure the creation info structure.
			 * @return std::shared_ptr< Vulkan::Sampler >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler > getSampler (const char * identifier, const std::function< void (Settings & settings, VkSamplerCreateInfo &) > & setupCreateInfo) noexcept;

			/**
			 * @brief Checks if the swap-chain has been refreshed and reset the marker.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			checkSwapChainRefresh () noexcept
			{
				if ( m_swapChainRefreshed )
				{
					m_swapChainRefreshed = false;

					return true;
				}

				return false;
			}

			/**
			 * @brief Render a new frame for the active scene.
			 * @param scene A reference to the scene smart pointer.
			 * @param overlayManager A reference to the overlay manager.
			 * @return void
			 */
			void renderFrame (const std::shared_ptr< Scenes::Scene > & scene, const Overlay::Manager & overlayManager) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const Libs::ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/** @copydoc EmEn::Console::Controllable::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

			/**
			 * @brief Refresh the graphics renderer framebuffer.
			 * @return bool
			 */
			[[nodiscard]]
			bool refreshFramebuffer () noexcept;

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
			 * @return void
			 */
			void renderShadowMaps (RendererFrameScope & currentFrameScope, Scenes::Scene & scene) const noexcept;

			/**
			 * @brief Updates every dynamic texture2Ds from the scene.
			 * @param currentFrameScope A writable reference to the current frame scope, the one being rendered.
			 * @param scene A reference to the scene.
			 * @return void
			 */
			void renderRenderToTextures (RendererFrameScope & currentFrameScope, Scenes::Scene & scene) const noexcept;

			/**
			 * @brief Updates every off-screen view from the scene.
			 * @param currentFrameScope A writable reference to the current frame scope, the one being rendered.
			 * @param scene A reference to the scene.
			 * @return void
			 */
			void renderViews (RendererFrameScope & currentFrameScope, Scenes::Scene & scene) const noexcept;

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

			PrimaryServices & m_primaryServices;
			Vulkan::Instance & m_vulkanInstance;
			Window & m_window;
			std::shared_ptr< Vulkan::Device > m_device;
			Vulkan::TransferManager m_transferManager;
			Vulkan::LayoutManager m_layoutManager;
			Saphir::ShaderManager m_shaderManager;
			SharedUBOManager m_sharedUBOManager;
			VertexBufferFormatManager m_vertexBufferFormatManager;
			ExternalInput m_externalInput;
			std::vector< ServiceInterface * > m_subServicesEnabled;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::shared_ptr< Vulkan::SwapChain > m_swapChain;
			std::shared_ptr< RenderTarget::Abstract > m_windowLessView;
			Libs::StaticVector< RendererFrameScope, 5 > m_rendererFrameScope;
			std::map< size_t, std::shared_ptr< Saphir::Program > > m_programs;
			std::map< size_t, std::shared_ptr< Vulkan::GraphicsPipeline > > m_pipelines;
			std::map< std::string, std::shared_ptr< Vulkan::RenderPass > > m_renderPasses;
			std::map< const char *, std::shared_ptr< Vulkan::Sampler > > m_samplers;
			Libs::Time::Statistics::RealTime< std::chrono::high_resolution_clock > m_statistics{30};
			std::array< VkClearValue, 2 > m_clearColors{};
			uint32_t m_currentFrameIndex{0};
			const uint64_t m_timeout{std::chrono::duration_cast< std::chrono::nanoseconds >(std::chrono::milliseconds(1000)).count()};
			bool m_debugMode{false};
			bool m_windowLess{false};
			bool m_shadowMapsEnabled{true};
			bool m_renderToTexturesEnabled{true};
			bool m_swapChainRefreshed{false};
	};
}
