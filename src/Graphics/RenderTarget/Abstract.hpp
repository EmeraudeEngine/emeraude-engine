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
#include <memory>
#include <string>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for inheritances. */
#include "Scenes/AVConsole/AbstractVirtualDevice.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Vulkan/Sync/Semaphore.hpp"
#include "Graphics/FramebufferPrecisions.hpp"
#include "Graphics/Types.hpp"

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
		class CommandBuffer;
		class TransferManager;
	}
}

namespace EmEn::Graphics::RenderTarget
{
	/** @brief Cubemap render strategy enumeration. */
	enum class CubemapRenderStrategy : uint8_t
	{
		/** @brief Render each face of the cubemap in a separate render pass. (6 passes total). */
		Sequential,
		/** @brief Renders all 6 faces in a single pass using a Geometry Shader. (1 pass only). */
		GeometryShader
	};

	/**
	 * @brief The base class for all render targets.
	 * @extends EmEn::AVConsole::AbstractVirtualDevice This is a virtual video device.
	 */
	class Abstract : public AVConsole::AbstractVirtualDevice
	{
		public:

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
			 * @brief Creates the render target objects in the video memory.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createRenderTarget (Renderer & renderer) noexcept;

			/**
			 * @brief Destroys the render target objects from the video memory.
			 * @return bool
			 */
			bool destroyRenderTarget () noexcept;

			/**
			 * @brief Returns whether the render target is out of date.
			 * @note Always return true with automatic rendering ON.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRenderOutOfDate () const noexcept
			{
				return m_renderOutOfDate;
			}

			/**
			 * @brief Returns whether the render target is made every frame.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAutomaticRendering () const noexcept
			{
				return m_automaticRendering;
			}

			/**
			 * @brief Sets the automatic rendering state.
			 * @param state The state.
			 * @return void
			 */
			void
			setAutomaticRenderingState (bool state) noexcept
			{
				m_automaticRendering = state;

				if ( this->isAutomaticRendering() )
				{
					m_renderOutOfDate = true;
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

				m_renderOutOfDate = true;
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

				m_renderOutOfDate = false;
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
			 * @brief Returns the semaphore associated with this render target for GPU/GPU synchronization.
			 * @return std::shared_ptr< Vulkan::Sync::Semaphore >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sync::Semaphore >
			semaphore () const noexcept
			{
				return m_semaphore;
			}

			/**
			 * @brief Sets the render target maximum viewable distance in meters.
			 * @return void
			 */
			void
			setViewDistance (float meters) noexcept
			{
				m_viewDistance = meters;
			}

			/**
			 * @brief Returns the render target maximum viewable distance in meters.
			 * @return float
			 */
			[[nodiscard]]
			float
			viewDistance () const noexcept
			{
				return m_viewDistance;
			}

			/**
			 * @brief Changes the projection type.
			 * @param state The state.
			 * @return void
			 */
			void
			setOrthographicProjection (bool state) noexcept
			{
				m_isOrthographicProjection = state;
			}

			/**
			 * @brief Returns whether the render target uses an orthographic projection.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isOrthographicProjection () const noexcept
			{
				return m_isOrthographicProjection;
			}

			/**
			 * @brief Sets the viewport to a command buffer.
			 * @note This is used when the dynamic viewport is used with graphics pipelines.
			 * @param commandBuffer A reference to the command buffer.
			 * @return void
			 */
			void setViewport (const Vulkan::CommandBuffer & commandBuffer) const noexcept;

			/**
			 * @brief Updates the render target view range properties.
			 * @note This version doesn't change the projection type.
			 * @param fovOrNear The field of view if the render target uses a perspective projection or near value for orthographic projection.
			 * @param distanceOrFar The distance if the render target uses a perspective projection or far value for orthographic projection.
			 * @return void
			 */
			virtual void updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept = 0;

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
			 * @brief Gives access to the framebuffer for the rendering process.
			 * @return const Vulkan::Framebuffer *
			 */
			[[nodiscard]]
			virtual const Vulkan::Framebuffer * framebuffer () const noexcept = 0;

			/**
			 * @brief Returns the const access to the view matrices interface.
			 * @return const ViewMatricesInterface &
			 */
			[[nodiscard]]
			virtual const ViewMatricesInterface & viewMatrices () const noexcept = 0;

			/**
			 * @brief Returns the access to the view matrices interface.
			 * @return ViewMatricesInterface &
			 */
			[[nodiscard]]
			virtual ViewMatricesInterface & viewMatrices () noexcept = 0;

			/**
			 * @brief Returns whether the render target is ready to render into.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isReadyForRendering () const noexcept = 0;

			/**
			 * @brief Returns whether the shadow map is in debug mode.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isDebug () const noexcept = 0;

			/**
			 * @brief Captures the GPU buffer to save into a pixmap.
			 * @note For single-layer textures, if layer > 0, a warning is printed and layer 0 is captured instead.
			 * @note For cubemaps: layer 0=+X, 1=-X, 2=+Y, 3=-Y, 4=+Z, 5=-Z
			 * @param transferManager A reference to the transfer manager.
			 * @param layerIndex The layer index to capture (for cubemaps and texture arrays).
			 * @param keepAlpha Keep the alpha channel from the GPU memory.
			 * @param withDepthBuffer Enable the depth buffer to be captured, if exists.
			 * @param withStencilBuffer Enable the stencil buffer to be captured, if exists.
			 * @return std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 > Array containing [0] = color, [1] = depth (optional), [2] = stencil (optional)
			 */
			[[nodiscard]]
			virtual std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 > capture (Vulkan::TransferManager & transferManager, uint32_t layerIndex, bool keepAlpha, bool withDepthBuffer, bool withStencilBuffer) const noexcept = 0;

		protected:

			/**
			 * @brief Constructs an abstract render target.
			 * @param deviceName A reference to a string for the name of the video device.
			 * @param precisions The framebuffer precisions.
			 * @param extent The framebuffer dimensions.
			 * @param viewDistance The max viewable distance in meters.
			 * @param renderType The type of render.
			 * @param allowedConnexionType The type of connexion this virtual device allows.
			 * @param isOrthographicProjection Set orthographic projection instead of perspective.
			 * @param enableSyncPrimitives Enable the creation of global sync primitive for this render target.
			 */
			Abstract (const std::string & deviceName, const FramebufferPrecisions & precisions, const VkExtent3D & extent, float viewDistance, RenderTargetType renderType, AVConsole::ConnexionType allowedConnexionType, bool isOrthographicProjection, bool enableSyncPrimitives) noexcept
				: AbstractVirtualDevice{deviceName, AVConsole::DeviceType::Video, allowedConnexionType},
				m_precisions{precisions},
				m_extent{extent},
				m_renderArea{
					.offset = {.x = 0, .y = 0},
					.extent = {.width = extent.width, .height = extent.height}
				},
				m_viewDistance{viewDistance},
				m_renderType{renderType},
				m_isOrthographicProjection{isOrthographicProjection},
				m_enableSyncPrimitive{enableSyncPrimitives}
			{

			}

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
			 * @param identifier A reference to a string to identify buffers.
			 * @return bool
			 */
			[[nodiscard]]
			bool createColorBuffer (const std::shared_ptr< Vulkan::Device > & device, std::shared_ptr< Vulkan::Image > & image, std::shared_ptr< Vulkan::ImageView > & imageView, const std::string & identifier) const noexcept;

			/**
			 * @brief Creates a depth buffer.
			 * @param device A reference to a graphics device smart pointer.
			 * @param image A reference to an image smart pointer.
			 * @param imageView A reference to an image view smart pointer.
			 * @param identifier A reference to a string to identify buffers.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDepthBuffer (const std::shared_ptr< Vulkan::Device > & device, std::shared_ptr< Vulkan::Image > & image, std::shared_ptr< Vulkan::ImageView > & imageView, const std::string & identifier) const noexcept;

			/**
			 * @brief Creates a depth+stencil buffer.
			 * @param device A reference to a graphics device smart pointer.
			 * @param image A reference to an image smart pointer.
			 * @param depthImageView A reference to an image view smart pointer.
			 * @param stencilImageView A reference to an image view smart pointer.
			 * @param identifier A reference to a string to identify buffers.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool createDepthStencilBuffer (const std::shared_ptr< Vulkan::Device > & device, std::shared_ptr< Vulkan::Image > & image, std::shared_ptr< Vulkan::ImageView > & depthImageView, std::shared_ptr< Vulkan::ImageView > & stencilImageView, const std::string & identifier) noexcept;

			/**
			 * @brief Creates or returns a render pass.
			 * @param renderer A reference to the renderer.
			 * @return std::shared_ptr< Vulkan::RenderPass >
			 */
			[[nodiscard]]
			virtual std::shared_ptr< Vulkan::RenderPass > createRenderPass (Renderer & renderer) const noexcept = 0;

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

		private:

			FramebufferPrecisions m_precisions;
			VkExtent3D m_extent{};
			VkRect2D m_renderArea{};
			float m_viewDistance{DefaultGraphicsViewDistance};
			RenderTargetType m_renderType;
			std::shared_ptr< Vulkan::Sync::Semaphore > m_semaphore;
			bool m_isOrthographicProjection{false};
			bool m_enableSyncPrimitive{false};
			bool m_renderOutOfDate{false};
			bool m_automaticRendering{false};
	};
}
