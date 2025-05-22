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
#include "Libs/ObserverTrait.hpp"
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/Time/Statistics/RealTime.hpp"
#include "Vulkan/LayoutManager.hpp"
#include "Vulkan/SharedUBOManager.hpp"
#include "Vulkan/TransferManager.hpp"
#include "RenderTarget/Abstract.hpp"
#include "Saphir/ShaderManager.hpp"
#include "RendererFrameScope.hpp"
#include "VertexBufferFormatManager.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Vulkan
	{
		class Instance;
		class SwapChain;
		class Device;
		class DescriptorPool;
		class CommandPool;
		class CommandBuffer;
		class GraphicsPipeline;
		class Sampler;
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
	class Window;
}

namespace EmEn::Graphics
{
	/**
	 * @brief The graphics renderer service class.
	 * @extends EmEn::ServiceInterface The renderer is a service.
	 * @extends EmEn::Console::Controllable The console can control the renderer.
	 * @extends EmEn::Libs::ObserverTrait The renderer needs to observe handle changes, for instance.
	 */
	class Renderer final : public ServiceInterface, public Console::Controllable, public Libs::ObserverTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"RendererService"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/** @brief Observable notification codes. */
			enum NotificationCode
			{
				SwapChainCreated,
				SwapChainRecreated,
				SwapChainDestroyed,
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
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Renderer (const Renderer & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Renderer (Renderer && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Renderer &
			 */
			Renderer & operator= (const Renderer & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Renderer &
			 */
			Renderer & operator= (Renderer && copy) noexcept = delete;

			/**
			 * @brief Destructs the graphics renderer.
			 */
			~Renderer () override;

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_flags[ServiceInitialized];
			}

			/**
			 * @brief Returns whether the debug mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isDebugModeEnabled () const noexcept
			{
				return m_flags[DebugMode];
			}

			/**
			 * @brief Controls the state of shadow maps rendering.
			 * @param state The state.
			 * @return void
			 */
			void
			enableShadowMaps (bool state) noexcept
			{
				m_flags[ShadowMapsEnabled] = state;
			}

			/**
			 * @brief Returns whether the shadow maps rendering is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isShadowMapsEnabled () const noexcept
			{
				return m_flags[ShadowMapsEnabled];
			}

			/**
			 * @brief Controls the state of rendering to textures.
			 * @param state The state.
			 * @return void
			 */
			void
			enableRenderToTextures (bool state) noexcept
			{
				m_flags[RenderToTexturesEnabled] = state;
			}

			/**
			 * @brief Returns whether the rendering to textures is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRenderToTexturesEnabled () const noexcept
			{
				return m_flags[RenderToTexturesEnabled];
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
			 * @brief Returns the reference to the shared UBO manager.
			 * @return Vulkan::SharedUBOManager &
			 */
			[[nodiscard]]
			Vulkan::SharedUBOManager &
			sharedUBOManager () noexcept
			{
				return m_sharedUBOManager;
			}

			/**
			 * @brief Returns the reference to the shared UBO manager.
			 * @return const Vulkan::SharedUBOManager &
			 */
			[[nodiscard]]
			const Vulkan::SharedUBOManager &
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
			 * @brief Returns the swap chain.
			 * @return std::shared_ptr< Vulkan::SwapChain >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::SwapChain >
			swapChain () const noexcept
			{
				return m_swapChain;
			}

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
			 * @brief Finalizes the graphics pipeline creation by replacing it by a similar or create and cache this new one.
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
			 * @param type
			 * @param createFlags The createInfo flags. Default none.
			 * @return std::shared_ptr< Vulkan::Sampler >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler > getSampler (size_t type, VkSamplerCreateFlags createFlags = 0) noexcept;

			/**
			 * @brief Render a new frame for the active scene.
			 * @param scene A reference to the scene smart pointer.
			 * @param overlayManager A reference to the overlay manager.
			 * @return void
			 */
			void renderFrame (const std::shared_ptr< Scenes::Scene > & scene, const Overlay::Manager & overlayManager) noexcept;

			/**
			 * @brief Returns the instance of the graphics renderer.
			 * @return Renderer *
			 */
			[[nodiscard]]
			static
			Renderer *
			instance () noexcept
			{
				return s_instance;
			}

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

			/** @copydoc EmEn::Libs::ObserverTrait::onNotification() */
			[[nodiscard]]
			bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept override;

			/** @copydoc EmEn::Console::Controllable::onRegisterToConsole. */
			void onRegisterToConsole () noexcept override;

			/**
			 * @brief Initialize all sub services of the renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool initializeSubServices () noexcept;

			/**
			 * @brief @brief Returns a command buffer for a specific render target.
			 * @param renderTarget A reference to a render target smart pointer.
			 * @return std::shared_ptr< Vulkan::CommandBuffer >
			 */
			std::shared_ptr< Vulkan::CommandBuffer > getCommandBuffer (const std::shared_ptr< RenderTarget::Abstract > & renderTarget) noexcept;

			/**
			 * @brief Updates every shadow map from the scene.
			 * @param frameIndex The current frame index in the swap-chain.
			 * @param scene A reference to the scene.
			 * @return void
			 */
			void renderShadowMaps (uint32_t frameIndex, Scenes::Scene & scene) noexcept;

			/**
			 * @brief Updates every dynamic texture2Ds from the scene.
			 * @param frameIndex The current frame index in the swap-chain.
			 * @param scene A reference to the scene.
			 * @return void
			 */
			void renderRenderToTextures (uint32_t frameIndex, Scenes::Scene & scene) noexcept;

			/**
			 * @brief Updates every off-screen view from the scene.
			 * @param frameIndex The current frame index in the swap-chain.
			 * @param scene A reference to the scene.
			 * @return void
			 */
			void renderViews (uint32_t frameIndex, Scenes::Scene & scene) noexcept;

			/**
			 * @brief Creates command pools and buffers according to the swap chain image count.
			 * @return bool
			 */
			bool createCommandSystem () noexcept;

			/**
			 * @brief Destroys command pools and buffers.
			 * @return void
			 */
			void destroyCommandSystem () noexcept;

			/**
			 * @brief Recreates the swap-chain.
			 * @return bool
			 */
			[[nodiscard]]
			bool recreateSwapChain () noexcept;

			/* Flag names. */
			static constexpr auto ServiceInitialized{0UL};
			static constexpr auto DebugMode{1UL};
			static constexpr auto ShadowMapsEnabled{2UL};
			static constexpr auto RenderToTexturesEnabled{3UL};

			static Renderer * s_instance;

			PrimaryServices & m_primaryServices;
			Vulkan::Instance & m_vulkanInstance;
			Window & m_window;
			std::shared_ptr< Vulkan::Device > m_device;
			Saphir::ShaderManager m_shaderManager;
			Vulkan::TransferManager m_transferManager;
			Vulkan::LayoutManager m_layoutManager;
			Vulkan::SharedUBOManager m_sharedUBOManager;
			VertexBufferFormatManager m_vertexBufferFormatManager;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::vector< RendererFrameScope > m_rendererFrameScope;
			std::shared_ptr< Vulkan::CommandPool > m_offScreenCommandPool;
			std::map< std::shared_ptr< RenderTarget::Abstract >, std::shared_ptr< Vulkan::CommandBuffer > > m_offScreenCommandBuffers;
			std::shared_ptr< Vulkan::SwapChain > m_swapChain;
			std::map< size_t, std::shared_ptr< Saphir::Program > > m_programs;
			std::map< size_t, std::shared_ptr< Vulkan::GraphicsPipeline > > m_pipelines;
			std::map< std::string, std::shared_ptr< Vulkan::RenderPass > > m_renderPasses;
			std::map< size_t, std::shared_ptr< Vulkan::Sampler > > m_samplers;
			Libs::Time::Statistics::RealTime< std::chrono::high_resolution_clock > m_statistics{30};
			std::array< VkClearValue, 2 > m_clearColors{};
			std::vector< ServiceInterface * > m_subServicesEnabled;
			std::array< bool, 8 > m_flags{
				false/*ServiceInitialized*/,
				false/*DebugMode*/,
				true/*ShadowMapsEnabled*/,
				true/*RenderToTexturesEnabled*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
