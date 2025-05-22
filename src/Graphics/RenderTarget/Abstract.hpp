/*
 * src/Graphics/RenderTarget/Abstract.hpp
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
#include <cstdint>
#include <array>
#include <memory>
#include <string>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for inheritances. */
#include "AVConsole/AbstractVirtualDevice.hpp"

/* Local inclusions for usages. */
#include "Graphics/FramebufferPrecisions.hpp"
#include "Graphics/Types.hpp"
#include "Vulkan/Sync/Fence.hpp"
#include "Vulkan/Sync/Semaphore.hpp"

/* Forward declarations. */
namespace EmEn
{
	namespace Graphics
	{
		class ViewMatricesInterface;
		class Renderer;
	}

	namespace Vulkan
	{
		class Device;
		class Framebuffer;
		class RenderPass;
		class Image;
		class ImageView;
	}
}

namespace EmEn::Graphics::RenderTarget
{
	/**
	 * @brief The base class for all render targets.
	 * @extends EmEn::AVConsole::AbstractVirtualDevice This is a virtual video device.
	 */
	class Abstract : public AVConsole::AbstractVirtualDevice
	{
		public:

			static constexpr auto ViewRender{"ViewRender"};
			static constexpr auto TextureRender{"TextureRender"};
			static constexpr auto ShadowRender{"ShadowRender"};

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Abstract (Abstract && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (const Abstract & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Abstract &
			 */
			Abstract & operator= (Abstract && copy) noexcept = delete;

			/**
			 * @brief Destructs an abstract render target.
			 */
			~Abstract () override = default;

			/**
			 * @brief Returns whether the render target is out of date.
			 * @note Always return true with automatic rendering ON.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRenderOutOfDate () const noexcept
			{
				return m_flags[RenderOutOfDate];
			}

			/**
			 * @brief Returns whether the render target is made every frame.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAutomaticRendering () const noexcept
			{
				return m_flags[AutomaticRendering];
			}

			/**
			 * @brief Sets the automatic rendering state.
			 * @param state The state.
			 * @return void
			 */
			void
			setAutomaticRenderingState (bool state) noexcept
			{
				m_flags[AutomaticRendering] = state;

				if ( this->isAutomaticRendering() )
				{
					m_flags[RenderOutOfDate] = true;
				}
			}

			/**
			 * @brief Discard the render.
			 * @note Ineffective with automatic rendering ON.
			 * @return void
			 */
			void
			setRenderOutOfDate () noexcept
			{
				if ( this->isAutomaticRendering() )
				{
					return;
				}

				m_flags[RenderOutOfDate] = true;
			}

			/**
			 * @brief Sets the render is finished.
			 * @note Ineffective with automatic rendering ON.
			 * @return void
			 */
			void
			setRenderFinished () noexcept
			{
				if ( this->isAutomaticRendering() )
				{
					return;
				}

				m_flags[RenderOutOfDate] = false;
			}

			/**
			 * @brief Returns the precisions of the framebuffer.
			 * @return const FramebufferPrecisions &
			 */
			[[nodiscard]]
			const FramebufferPrecisions &
			precisions () const noexcept
			{
				return m_precisions;
			}

			/**
			 * @brief Returns the dimensions of the framebuffer.
			 * @return const VkExtent3D &
			 */
			[[nodiscard]]
			const VkExtent3D &
			extent () const noexcept
			{
				return m_extent;
			}

			/**
			 * @brief Returns the render area.
			 * @return const VkRect2D &
			 */
			[[nodiscard]]
			const VkRect2D &
			renderArea () const noexcept
			{
				return m_renderArea;
			}

			/**
			 * @brief Returns the render type.
			 * @return RenderTargetType
			 */
			[[nodiscard]]
			RenderTargetType
			renderType () const noexcept
			{
				return m_renderType;
			}

			/**
			 * @brief Returns the render to target semaphore for CPU/GPU synchronization.
			 * @return std::shared_ptr< Vulkan::Sync::Fence >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sync::Fence >
			fence () const noexcept
			{
				return m_fence;
			}

			/**
			 * @brief Returns the render to target semaphore for GPU/GPU synchronization.
			 * @return std::shared_ptr< Vulkan::Sync::Semaphore >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sync::Semaphore >
			semaphore () const noexcept
			{
				return m_semaphore;
			}

			/**
			 * @brief Creates the render target in the video memory.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer) noexcept;

			/**
			 * @brief Destroys the render target from the video memory.
			 * @return bool
			 */
			bool destroy () noexcept;

			/**
			 * @brief Returns the aspect ratio of the render target.
			 * @return float
			 */
			[[nodiscard]]
			virtual float aspectRatio () const noexcept = 0;

			/**
			 * @brief Returns whether the render target is a cubemap.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isCubemap () const noexcept = 0;

			/**
			 * @brief Returns the framebuffer.
			 * @return const Vulkan::Framebuffer *
			 */
			[[nodiscard]]
			virtual const Vulkan::Framebuffer * framebuffer () const noexcept = 0;

			/**
			 * @brief Gives access to the main hardware image object of the render target.
			 * @deprecated
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Vulkan::Image > image () const noexcept = 0;

			/**
			 * @brief Gives access to the main hardware image view object of the render target.
			 * @deprecated
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Vulkan::ImageView > imageView () const noexcept = 0;

			/**
			 * @brief Returns the const access to view matrices object.
			 * @return const ViewMatricesInterface &
			 */
			[[nodiscard]]
			virtual const ViewMatricesInterface & viewMatrices () const noexcept = 0;

			/**
			 * @brief Returns the access to view matrices object.
			 * @return ViewMatricesInterface &
			 */
			[[nodiscard]]
			virtual ViewMatricesInterface & viewMatrices () noexcept = 0;

			/**
			 * @brief Returns whether the render target is valid.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isValid () const noexcept = 0;

		protected:

			/**
			 * @brief Methods to create on child class.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool onCreate (Renderer & renderer) noexcept = 0;

			/**
			 * @brief Methods to destroy on child class.
			 * @return void
			 */
			virtual void onDestroy () noexcept = 0;

			/**
			 * @brief Constructs an abstract render target.
			 * @param name A reference to a string for the name of the video device.
			 * @param precisions The framebuffer precisions.
			 * @param extent The framebuffer dimensions.
			 * @param renderType The type of render.
			 * @param enableSyncPrimitives Enable the creation of global sync primitive for this render target.
			 * @param allowedConnexionType The type of connexion this virtual device allows.
			 */
			Abstract (const std::string & name, const FramebufferPrecisions & precisions, const VkExtent3D & extent, RenderTargetType renderType, AVConsole::ConnexionType allowedConnexionType, bool enableSyncPrimitives) noexcept;

			/**
			 * @brief Sets extents of the render target.
			 * @param width The width
			 * @param height The height
			 * @return void
			 */
			void
			setExtent (uint32_t width, uint32_t height) noexcept
			{
				m_extent.width = width;
				m_extent.height = height;
				m_extent.depth = 1;

				this->resetRenderArea();
			}

			/**
			 * @brief Sets extents of the render target.
			 * @param extent A reference to the extent.
			 * @return void
			 */
			void
			setExtent (const VkExtent3D & extent) noexcept
			{
				m_extent = extent;

				this->resetRenderArea();
			}

			/**
			 * @brief Resets the render area on the whole render target.
			 * @return void
			 */
			void
			resetRenderArea () noexcept
			{
				m_renderArea.offset.x = 0;
				m_renderArea.offset.y = 0;
				m_renderArea.extent.width = m_extent.width;
				m_renderArea.extent.height = m_extent.height;
			}

			/**
			 * @brief Creates a color buffer.
			 * @param device A reference to a graphics device smart pointer.
			 * @param image A reference to an image smart pointer.
			 * @param imageView A reference to an image view smart pointer.
			 * @param purposeId A reference to a string to identify buffers.
			 * @return bool
			 */
			[[nodiscard]]
			bool createColorBuffer (const std::shared_ptr< Vulkan::Device > & device, std::shared_ptr< Vulkan::Image > & image, std::shared_ptr< Vulkan::ImageView > & imageView, const std::string & purposeId) const noexcept;

			/**
			 * @brief Creates a depth buffer.
			 * @param device A reference to a graphics device smart pointer.
			 * @param image A reference to an image smart pointer.
			 * @param imageView A reference to an image view smart pointer.
			 * @param purposeId A reference to a string to identify buffers.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDepthBuffer (const std::shared_ptr< Vulkan::Device > & device, std::shared_ptr< Vulkan::Image > & image, std::shared_ptr< Vulkan::ImageView > & imageView, const std::string & purposeId) const noexcept;

			/**
			 * @brief Creates a depth+stencil buffer.
			 * @param device A reference to a graphics device smart pointer.
			 * @param image A reference to an image smart pointer.
			 * @param depthImageView A reference to an image view smart pointer.
			 * @param stencilImageView A reference to an image view smart pointer.
			 * @param purposeId A reference to a string to identify buffers.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool createDepthStencilBuffer (const std::shared_ptr< Vulkan::Device > & device, std::shared_ptr< Vulkan::Image > & image, std::shared_ptr< Vulkan::ImageView > & depthImageView, std::shared_ptr< Vulkan::ImageView > & stencilImageView, const std::string & purposeId) noexcept;

			/**
			 * @brief Creates or returns a render pass.
			 * @param renderer A reference to the renderer.
			 * @return std::shared_ptr< Vulkan::RenderPass >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Vulkan::RenderPass > createRenderPass (Renderer & renderer) const noexcept = 0;

		private:

			/* Flag names. */
			static constexpr auto EnableSyncPrimitive{0UL};
			static constexpr auto RenderOutOfDate{1UL};
			static constexpr auto AutomaticRendering{2UL};

			FramebufferPrecisions m_precisions;
			VkExtent3D m_extent{};
			VkRect2D m_renderArea{};
			RenderTargetType m_renderType;
			std::shared_ptr< Vulkan::Sync::Fence > m_fence;
			std::shared_ptr< Vulkan::Sync::Semaphore > m_semaphore;
			std::array< bool, 8 > m_flags{
				false/*EnableSyncPrimitive*/,
				false/*RenderOutOfDate*/,
				false/*AutomaticRendering*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
