/*
 * src/Overlay/Surface.hpp
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
#include <array>
#include <concepts>
#include <string>

/* Local inclusions for inheritance. */
#include "Libs/NameableTrait.hpp"

/* Local inclusions for usages. */
#include "Vulkan/DescriptorSet.hpp"
#include "Graphics/TextureResource/Abstract.hpp"
#include "FramebufferProperties.hpp"
#include "Libs/Math/Matrix.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"

namespace EmEn::Vulkan
{
	class LayoutManager;
}

namespace EmEn::Overlay
{
	/**
	 * @brief Defines the transition buffer synchronization status for async content providers.
	 * @details Used to coordinate resize operations with asynchronous content providers
	 * like CEF browsers, video decoders, or streaming sources.
	 */
	enum class TransitionBufferStatus : uint8_t
	{
		/** Transition buffer is ready. Drawing and committing are allowed. */
		Ready,

		/** Transition buffer is being recreated due to resize. Drawing is not allowed. */
		Resizing,

		/** Transition buffer has been recreated, waiting for async content.
		 *  Drawing is allowed, call contentReady() when done. */
		WaitingForContent
	};

	/**
	 * @brief Identifies which buffer should receive incoming frame data.
	 * @details Used by async content providers to determine where to write
	 * frame data based on the frame dimensions and current buffer states.
	 */
	enum class TargetBuffer : uint8_t
	{
		/** Frame dimensions don't match any buffer - skip the frame. */
		None,

		/** Frame matches active buffer dimensions - normal operation. */
		Active,

		/** Frame matches transition buffer dimensions - completing a resize. */
		Transition
	};

	/**
	 * @brief Encapsulates all resources for a single framebuffer.
	 * @details This structure groups the local pixmap data with its corresponding
	 * GPU resources (image, image view, descriptor set). Used internally by Surface
	 * for both single and double buffer modes.
	 */
	struct Framebuffer final
	{
		/**
		 * @brief Checks if all GPU resources are valid and created.
		 * @return bool True if the framebuffer is ready for rendering.
		 */
		[[nodiscard]]
		bool
		isValid () const noexcept
		{
			return image != nullptr && image->isCreated() &&
				   imageView != nullptr && imageView->isCreated() &&
				   descriptorSet != nullptr && descriptorSet->isCreated();
		}

		/**
		 * @brief Returns the framebuffer width in pixels.
		 * @return uint32_t The width, or 0 if not initialized.
		 */
		[[nodiscard]]
		uint32_t
		width () const noexcept
		{
			if ( image != nullptr )
			{
				return image->createInfo().extent.width;
			}

			return pixmap.width();
		}

		/**
		 * @brief Returns the framebuffer height in pixels.
		 * @return uint32_t The height, or 0 if not initialized.
		 */
		[[nodiscard]]
		uint32_t
		height () const noexcept
		{
			if ( image != nullptr )
			{
				return image->createInfo().extent.height;
			}

			return pixmap.height();
		}

		/**
		 * @brief Checks if the image dimensions match the given size.
		 * @param targetWidth Target width in pixels.
		 * @param targetHeight Target height in pixels.
		 * @return bool True if dimensions match.
		 */
		[[nodiscard]]
		bool
		matchesSize (uint32_t targetWidth, uint32_t targetHeight) const noexcept
		{
			return this->width() == targetWidth && this->height() == targetHeight;
		}

		/**
		 * @brief Destroys all GPU resources.
		 * @return void
		 */
		void
		destroy () noexcept
		{
			if ( descriptorSet != nullptr )
			{
				descriptorSet->destroy();
				descriptorSet.reset();
			}

			if ( imageView != nullptr )
			{
				imageView->destroyFromHardware();
				imageView.reset();
			}

			if ( image != nullptr )
			{
				image->destroyFromHardware();
				image.reset();
			}
		}

		/**
		 * @brief Writes to the GPU image using memory mapping with RAII safety.
		 * @details Maps the GPU memory, calls the provided function with the mapped pointer
		 * and row pitch, then unmaps automatically. Only works when image is host visible.
		 * @tparam write_func_t Function type accepting (void* mappedPtr, VkDeviceSize rowPitch) and returning bool.
		 * @param writeFunction The function to call with the mapped memory.
		 * @return bool True if mapping and write succeeded, false otherwise.
		 */
		template< typename write_func_t >
		requires std::invocable< write_func_t, void *, VkDeviceSize > && std::convertible_to< std::invoke_result_t< write_func_t, void *, VkDeviceSize >, bool >
		[[nodiscard]]
		bool
		writeWithMapping (write_func_t && writeFunction) const noexcept
		{
			if ( image == nullptr || !image->isHostVisible() )
			{
				return false;
			}

			void * mappedPtr = image->mapMemory();

			if ( mappedPtr == nullptr )
			{
				return false;
			}

			const auto rowPitch = image->rowPitch();

			const bool result = writeFunction(mappedPtr, rowPitch);

			image->unmapMemory();

			return result;
		}

		/** @brief Local pixmap data (CPU-side). Only used when memory mapping is disabled. */
		Libs::PixelFactory::Pixmap< uint8_t > pixmap;
		/** @brief Vulkan image on GPU. */
		std::shared_ptr< Vulkan::Image > image;
		/** @brief Vulkan image view for the image. */
		std::shared_ptr< Vulkan::ImageView > imageView;
		/** @brief Descriptor set binding the image for shader access. */
		std::unique_ptr< Vulkan::DescriptorSet > descriptorSet;
	};

	/**
	 * @brief The base class for overlay UIScreen surfaces.
	 * @extends Libs::NameableTrait A surface has a name.
	 */
	class Surface : public Libs::NameableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"OverlaySurface"};

			/**
			 * @brief Constructs a surface.
			 * @param framebufferProperties A reference to the overlay framebuffer properties.
			 * @param name A string [std::move].
			 * @param rectangle A reference to a rectangle for the surface geometry on screen. Default the whole screen.
			 * @param depth A depth value to order surface on the screen. Default 0.0.
			 * @param visible Set visibility state on startup. Default true.
			 */
			Surface (const FramebufferProperties & framebufferProperties, std::string name, const Libs::Math::Space2D::AARectangle< float > & rectangle = {}, float depth = 0.0F, bool visible = true) noexcept
				: NameableTrait{std::move(name)},
				m_framebufferProperties{framebufferProperties},
				m_rectangle{rectangle},
				m_depth{depth}
			{
				m_isVisible = visible;

				this->updateModelMatrix();
			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Surface (const Surface & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Surface (Surface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return SurfaceInterface &
			 */
			Surface & operator= (const Surface & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return SurfaceInterface &
			 */
			Surface & operator= (Surface && copy) noexcept = delete;

			/**
			 * @brief Destructs the surface.
			 */
			~Surface () noexcept override = default;

			/**
			 * @brief Enables double buffering mode for asynchronous content providers.
			 * @details When enabled, the surface uses a two-buffer system for smooth resize:
			 * - activeBuffer: always used for normal read/write operations and GPU rendering
			 * - transitionBuffer: prepared in background with new dimensions during resize
			 * - When async content at new size is ready, buffers are swapped
			 *
			 * Use this for external renderers like CEF browsers, video decoders, or any
			 * source that cannot provide content synchronously during resize.
			 *
			 * When disabled (default), resize operations block until complete.
			 *
			 * @warning Must be called BEFORE createOnHardware() for proper initialization.
			 */
			void
			enableTransitionBuffer () noexcept
			{
				m_transitionBufferEnabled = true;
			}

			/**
			 * @brief Enable the GPU image to be mappable from the CPU for direct writing
			 */
			void
			enableMapping () noexcept
			{
				m_memoryMappingEnabled = true;
			}

			/**
			 * @brief Returns the framebuffer properties from the overlay.
			 * @return const FramebufferProperties &
			 */
			[[nodiscard]]
			const FramebufferProperties &
			framebufferProperties () const noexcept
			{
				return m_framebufferProperties;
			}

			/**
			 * @brief Returns the surface geometry.
			 * @return const Libs::Math::Space2D::AARectangle< float > &
			 */
			[[nodiscard]]
			const Libs::Math::Space2D::AARectangle< float > &
			geometry () const noexcept
			{
				return m_rectangle;
			}

			/**
			 * @brief Returns the surface depth on screen.
			 * @return float
			 */
			[[nodiscard]]
			float
			depth () const noexcept
			{
				return m_depth;
			}

			/**
			 * @brief Returns the model matrix to place the surface on screen.
			 * @return const Libs::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Matrix< 4, float > &
			modelMatrix () const noexcept
			{
				return m_modelMatrix;
			}

			/**
			 * @brief Returns the pixmap from the active buffer.
			 * @warning Use the active buffer mutex before writing into the pixmap with Surface::activeBufferMutex().
			 * @return Libs::PixelFactory::Pixmap< uint8_t > &
			 */
			[[nodiscard]]
			Libs::PixelFactory::Pixmap< uint8_t > &
			activePixmap () noexcept
			{
				return m_activeBuffer.pixmap;
			}

			/**
			 * @brief Returns the pixmap from the transition buffer.
			 * @note Only meaningful when resize transition is enabled. Used during resize
			 * to prepare content at the new size while the active buffer continues rendering.
			 * @return Libs::PixelFactory::Pixmap< uint8_t > &
			 */
			[[nodiscard]]
			Libs::PixelFactory::Pixmap< uint8_t > &
			transitionPixmap () noexcept
			{
				return m_transitionBuffer.pixmap;
			}

			/**
			 * @brief Returns the mutex to access the active buffer for writing operation.
			 * @return std::mutex &
			 */
			[[nodiscard]]
			std::mutex &
			activeBufferMutex () const noexcept
			{
				return m_framebufferAccess;
			}

			/**
			 * @brief Redefines the surface position and size in the screen.
			 * @param rectangle A reference to a rectangle.
			 * @return void
			 */
			void
			setGeometry (const Libs::Math::Space2D::AARectangle< float > & rectangle) noexcept
			{
				m_rectangle = rectangle;

				/* NOTE: The texture must be resized. */
				this->invalidate();
			}

			/**
			 * @brief Sets the surface position in the screen.
			 * @param xPosition The absolute X position.
			 * @param yPosition The absolute Y position.
			 * @return void
			 */
			void
			setPosition (float xPosition, float yPosition) noexcept
			{
				m_rectangle.setLeft(xPosition);
				m_rectangle.setTop(yPosition);

				this->updateModelMatrix();
			}

			/**
			 * @brief Sets the surface size in the screen.
			 * @param width A scalar value.
			 * @param height A scalar value.
			 * @return void
			 */
			void
			setSize (float width, float height) noexcept
			{
				m_rectangle.setWidth(width);
				m_rectangle.setHeight(height);

				/* NOTE: The texture must be resized. */
				this->invalidate();
			}

			/**
			 * @brief Sets the surface depth in the screen.
			 * @param depth A scalar value.
			 * @return void
			 */
			void
			setDepth (float depth) noexcept
			{
				m_depth = depth;

				this->updateModelMatrix();
			}

			/**
			 * @brief Moves the surface from a distance in the screen.
			 * @param deltaX The distance to move in X axis.
			 * @param deltaY The distance to move in Y axis.
			 * @return void
			 */
			void
			move (float deltaX, float deltaY) noexcept
			{
				m_rectangle.move(deltaX, deltaY);

				this->updateModelMatrix();
			}

			/**
			 * @brief Shows the web view.
			 * @return void
			 */
			void
			show () noexcept
			{
				m_isVisible = true;
			}

			/**
			 * @brief Hides the web view.
			 * @return void
			 */
			void
			hide () noexcept
			{
				m_isVisible = false;
			}

			/**
			 * @brief Returns whether the surface is visible.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVisible () const noexcept
			{
				return m_isVisible;
			}

			/**
			 * @brief Returns whether the surface is valid on GPU to draw in it.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVideoMemorySizeValid () const noexcept
			{
				return m_videoMemorySizeValid;
			}

			/**
			 * @brief Returns whether the surface is visible.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isVideoMemoryUpToDate () const noexcept
			{
				return m_videoMemoryUpToDate;
			}

			/**
			 * @brief Declares the surface to be recreated on video memory.
			 * @return void
			 */
			void
			invalidate () noexcept
			{
				m_videoMemorySizeValid = false;
				m_videoMemoryUpToDate = false;
			}

			/**
			 * @brief Declares the video memory content outdated to re-upload it.
			 * @return void
			 */
			void
			setVideoMemoryOutdated () noexcept
			{
				m_videoMemoryUpToDate = false;
			}

			/**
			 * @brief Returns whether double buffering mode is enabled for async content.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUsingTransferBuffer () const noexcept
			{
				return m_transitionBufferEnabled;
			}

			/**
			 * @brief Disables the automatic pixmap copy when creating the transition buffer.
			 * @details By default, when the transition buffer is created during resize,
			 * the active buffer content is scaled and copied to the transition buffer.
			 * This provides a placeholder image while waiting for new content.
			 * Set this to true if you want the transition buffer to start empty/black.
			 * @param disabled True to disable the copy, false to enable (default).
			 */
			void
			disablePixmapCopyInTransitionBuffer (bool disabled) noexcept
			{
				m_disablePixmapCopyInTransitionBuffer = disabled;
			}

			/**
			 * @brief Returns whether pixmap copy to transition buffer is disabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPixmapCopyInTransitionBufferDisabled () const noexcept
			{
				return m_disablePixmapCopyInTransitionBuffer;
			}

			/**
			 * @brief Returns the current transition buffer status.
			 * @note Only meaningful when resize transition is enabled.
			 * @return TransitionBufferStatus
			 */
			[[nodiscard]]
			TransitionBufferStatus
			transitionBufferStatus () const noexcept
			{
				return m_transitionBufferStatus;
			}

			/**
			 * @brief Checks if the transition buffer is ready to be committed.
			 * @details Returns true when the transition buffer has valid GPU resources
			 * and is not currently being resized. Use this to check if it's safe to
			 * call commitTransitionBuffer().
			 * @warning Returns false and logs a warning if transition buffer mode is not enabled.
			 * @return bool True if the transition buffer is ready for commit.
			 */
			[[nodiscard]]
			bool isTransitionBufferReady () const noexcept;

			/**
			 * @brief Determines which buffer should receive frame data based on dimensions.
			 * @details Used by async content providers to route incoming frames to the
			 * appropriate buffer. During resize transitions, frames may arrive at either
			 * the old size (for active buffer) or new size (for transition buffer).
			 * @param frameWidth The width of the incoming frame in pixels.
			 * @param frameHeight The height of the incoming frame in pixels.
			 * @return TargetBuffer The buffer that matches the frame dimensions, or None if no match.
			 */
			[[nodiscard]]
			TargetBuffer
			determineTargetBuffer (uint32_t frameWidth, uint32_t frameHeight) const noexcept
			{
				if ( this->isTransitionBufferReady() && m_transitionBuffer.matchesSize(frameWidth, frameHeight) )
				{
					return TargetBuffer::Transition;
				}

				if ( m_activeBuffer.matchesSize(frameWidth, frameHeight) )
				{
					return TargetBuffer::Active;
				}

				return TargetBuffer::None;
			}

			/**
			 * @brief Enables the listening of keyboard events.
			 * @param state The state.
			 * @return void
			 */
			void
			enableKeyboardListening (bool state) noexcept
			{
				m_isListeningKeyboard = state;
			}

			/**
			 * @brief Returns whether the keyboard is listened.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isListeningKeyboard () const noexcept
			{
				return m_isListeningKeyboard;
			}

			/**
			 * @brief Enables the listening of pointer events.
			 * @param state The state.
			 * @return void
			 */
			void
			enablePointerListening (bool state) noexcept
			{
				m_isListeningPointer = state;
			}

			/**
			 * @brief Returns whether the pointer is listened.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isListeningPointer () const noexcept
			{
				return m_isListeningPointer;
			}

			/**
			 * @brief Lock this listener when holding a mouse button to send all move events to it.
			 * @param state The state.
			 * @return void
			 */
			void
			lockPointerMoveEvents (bool state) noexcept
			{
				m_lockPointerMoveEvents = state;
			}

			/**
			 * @brief Returns whether the move events are tracked when a button is held.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPointerMoveEventsLocked () const noexcept
			{
				return m_lockPointerMoveEvents;
			}

			/**
			 * @brief Sets the surface "pointer-over" state.
			 * @param state The state.
			 * @return void
			 */
			void
			setPointerOverState (bool state) noexcept
			{
				m_isPointerWasOver = state;
			}

			/**
			 * @brief Returns whether the pointer was on the surface in the last move event check.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPointerWasOver () const noexcept
			{
				return m_isPointerWasOver;
			}

			/**
			 * @brief Sets the surface "focus" state.
			 * @param state The state.
			 * @return void
			 */
			void
			setFocusedState (bool state) noexcept
			{
				m_isFocused = state;
			}

			/**
			 * @brief Returns whether the surface is focused.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFocused () const noexcept
			{
				return m_isFocused;
			}

			/**
			 * @brief Enables the event blocking system.
			 * @note This enables only the surface area. See enableAlphaTest().
			 * @param state The state.
			 * @return void
			 */
			void
			enableEventBlocking (bool state) noexcept
			{
				m_isOpaque = state;
			}

			/**
			 * @brief Returns whether the event blocking system is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isBlockingEvent () const noexcept
			{
				return m_isOpaque;
			}

			/**
			 * @brief Enables the event blocking system using alpha test.
			 * @note The alpha value threshold is set to 10% by default.
			 * @return void
			 */
			void
			enableEventBlockingAlphaTest (bool state) noexcept
			{
				m_isAlphaTestEnabled = state;
			}

			/**
			 * @brief Returns whether the event blocking system using alpha test is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isBlockingEventWithAlphaTest () const noexcept
			{
				return m_isAlphaTestEnabled;
			}

			/**
			 * @brief Sets a threshold below where the alpha test won't block the event.
			 * @param threshold A value between 0.0 to 1.0
			 * @return void
			 */
			void
			setAlphaThreshold (float threshold) noexcept
			{
				if ( threshold > 1.0F )
				{
					m_alphaThreshold = 1.0F;
				}
				else if ( threshold < 0.0F )
				{
					m_alphaThreshold = 0.0F;
				}
				else
				{
					m_alphaThreshold = threshold;
				}
			}

			/**
			 * @brief Returns the current alpha threshold for event blocking test.
			 * @return float
			 */
			[[nodiscard]]
			float
			alphaThreshold () const noexcept
			{
				return m_alphaThreshold;
			}

			/**
			 * @brief Checks whether the pointer is blocked by something on the surface
			 * to prevent to dispatch the related event below.
			 * @param screenX The position in X on the screen.
			 * @param screenY The position in y on the screen.
			 * @return bool
			 */
			[[nodiscard]]
			bool isEventBlocked (float screenX, float screenY) const noexcept;

			/**
			 * @brief Checks whether the pointer coordinates intersect with the surface.
			 * @param positionX The pointer coordinate on X screen axis.
			 * @param positionY The pointer coordinate on Y screen axis.
			 * @return bool
			 */
			[[nodiscard]]
			bool isBelowPoint (float positionX, float positionY) const noexcept;

			/**
			 * @brief Returns the surface descriptor set of the active buffer.
			 * @return const Vulkan::DescriptorSet *
			 */
			[[nodiscard]]
			const Vulkan::DescriptorSet *
			descriptorSet () const noexcept
			{
				return m_activeBuffer.descriptorSet.get();
			}

			/**
			 * @return Returns whether the image buffer is mappable.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isMemoryMappingEnabled () const noexcept
			{
				return m_memoryMappingEnabled;
			}

			/**
			 * @brief Returns a reference to the active framebuffer.
			 * @return const Framebuffer &
			 */
			[[nodiscard]]
			const Framebuffer &
			activeBuffer () const noexcept
			{
				return m_activeBuffer;
			}

			/**
			 * @brief Returns a reference to the transition framebuffer.
			 * @note Only meaningful when transition buffer mode is enabled.
			 * @return const Framebuffer &
			 */
			[[nodiscard]]
			const Framebuffer &
			transitionBuffer () const noexcept
			{
				return m_transitionBuffer;
			}

			/**
			 * @brief Writes to the active buffer GPU image using memory mapping.
			 * @details Convenience method that wraps activeBuffer().writeWithMapping().
			 * Maps the GPU memory, calls the provided function, then unmaps automatically.
			 * @tparam write_func_t Function type accepting (void* mappedPtr, VkDeviceSize rowPitch) and returning bool.
			 * @param writeFunction The function to call with the mapped memory.
			 * @return bool True if mapping and write succeeded, false otherwise.
			 */
			template< typename write_func_t >
			requires std::invocable< write_func_t, void *, VkDeviceSize > && std::convertible_to< std::invoke_result_t< write_func_t, void *, VkDeviceSize >, bool >
			[[nodiscard]]
			bool
			writeActiveBufferWithMapping (write_func_t && writeFunction) const noexcept
			{
				return m_activeBuffer.writeWithMapping(std::forward< write_func_t >(writeFunction));
			}

			/**
			 * @brief Writes to the transition buffer GPU image using memory mapping.
			 * @details Convenience method that wraps transitionBuffer().writeWithMapping().
			 * Maps the GPU memory, calls the provided function, then unmaps automatically.
			 * @tparam write_func_t Function type accepting (void* mappedPtr, VkDeviceSize rowPitch) and returning bool.
			 * @param writeFunction The function to call with the mapped memory.
			 * @return bool True if mapping and write succeeded, false otherwise.
			 */
			template< typename write_func_t >
			requires std::invocable< write_func_t, void *, VkDeviceSize > && std::convertible_to< std::invoke_result_t< write_func_t, void *, VkDeviceSize >, bool >
			[[nodiscard]]
			bool
			writeTransitionBufferWithMapping (write_func_t && writeFunction) const noexcept
			{
				return m_transitionBuffer.writeWithMapping(std::forward< write_func_t >(writeFunction));
			}

			/**
			 * @brief Creates the surface on the GPU.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createOnHardware (Graphics::Renderer & renderer) noexcept;

			/**
			 * @brief Destroys the surface from the GPU.
			 * @return bool
			 */
			bool destroyFromHardware () noexcept;

			/**
			 * @brief Processes pending updates for this surface.
			 * @details This method handles two types of updates:
			 * 1. Size changes: If the surface was invalidated (via invalidate() or window resize),
			 *	the back buffer is recreated at the new pixel dimensions. The front buffer
			 *	continues to be used for rendering until swapFramebuffers() is called.
			 * 2. Content changes: If setVideoMemoryOutdated() was called, the front buffer
			 *	content is uploaded to GPU memory.
			 * @note For asynchronous renderers (e.g., CEF), the back buffer preparation and
			 * front buffer swap are decoupled to allow content to be ready before switching.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool True if update succeeded, false on failure.
			 */
			[[nodiscard]]
			bool processUpdates (Graphics::Renderer & renderer) noexcept;

			/**
			 * @brief Commits the transition buffer, making it the new active buffer.
			 * @details After a resize, call this to switch from the old active buffer
			 * to the transition buffer (which has the new size and content).
			 * The old active buffer becomes the new transition buffer for the next resize.
			 * @warning Returns false and logs a warning if transition buffer mode is not enabled.
			 * @return bool True on success, false if not ready to commit.
			 */
			[[nodiscard]]
			bool commitTransitionBuffer () noexcept;

			/**
			 * @brief On key press event handler.
			 * @note Override this method to react on the input event.
			 * @param key The keyboard universal key code. I.e., QWERTY keyboard 'A' key gives the ASCII code '65' on all platforms.
			 * @param scancode The OS dependent scancode.
			 * @param modifiers The modifier keys mask.
			 * @param repeat Repeat state.
			 * @return bool
			 */
			virtual
			bool
			onKeyPress (int32_t key, int32_t scancode, int32_t modifiers, bool repeat) noexcept
			{
				/* Unused by default. */
				static_cast< void >(key);
				static_cast< void >(scancode);
				static_cast< void >(modifiers);
				static_cast< void >(repeat);

				return false;
			}

			/**
			 * @brief On key release event handler.
			 * @note Override this method to react on the input event.
			 * @param key The keyboard universal key code. I.e., QWERTY keyboard 'A' key gives the ASCII code '65' on all platforms.
			 * @param scancode The OS dependent scancode.
			 * @param modifiers The modifier keys mask.
			 * @return bool
			 */
			virtual
			bool
			onKeyRelease (int32_t key, int32_t scancode, int32_t modifiers) noexcept
			{
				/* Unused by default. */
				static_cast< void >(key);
				static_cast< void >(scancode);
				static_cast< void >(modifiers);

				return false;
			}

			/**
			 * @brief On character typing event handler.
			 * @note Override this method to react on the input event.
			 * @param unicode The character Unicode value.
			 * @return bool
			 */
			virtual
			bool
			onCharacterType (uint32_t unicode) noexcept
			{
				/* Unused by default. */
				static_cast< void >(unicode);

				return false;
			}

			/**
			 * @brief Method fired when a pointer is entering the surface.
			 * @note Override this method to react on the input event.
			 * @param positionX The pointer X position.
			 * @param positionY The pointer Y position.
			 * @return bool
			 */
			virtual
			void
			onPointerEnter (float positionX, float positionY) noexcept
			{
				/* Unused by default. */
				static_cast< void >(positionX);
				static_cast< void >(positionY);
			}

			/**
			 * @brief Method fired when a pointer is leaving the surface.
			 * @note Override this method to react on the input event.
			 * @param positionX The pointer X position.
			 * @param positionY The pointer Y position.
			 * @return bool
			 */
			virtual
			void
			onPointerLeave (float positionX, float positionY) noexcept
			{
				/* Unused by default. */
				static_cast< void >(positionX);
				static_cast< void >(positionY);
			}

			/**
			 * @brief Method fired when a pointer is moving on the surface.
			 * @note Override this method to react on the input event.
			 * @param positionX The pointer X position.
			 * @param positionY The pointer Y position.
			 * @return bool
			 */
			virtual
			bool
			onPointerMove (float positionX, float positionY) noexcept
			{
				/* Unused by default. */
				static_cast< void >(positionX);
				static_cast< void >(positionY);

				return this->isBlockingEvent();
			}

			/**
			 * @brief Method fired when a button of the pointer is pressed on the surface.
			 * @note Override this method to react on the input event.
			 * @param positionX The pointer X position.
			 * @param positionY The pointer Y position.
			 * @param buttonNumber The pointer button number pressed.
			 * @param modifiers The keyboard modifiers held when the button has been pressed.
			 * @return bool
			 */
			virtual
			bool
			onButtonPress (float positionX, float positionY, int32_t buttonNumber, int32_t modifiers) noexcept
			{
				/* Unused by default. */
				static_cast< void >(positionX);
				static_cast< void >(positionY);
				static_cast< void >(buttonNumber);
				static_cast< void >(modifiers);

				return this->isBlockingEvent();
			}

			/**
			 * @brief Method fired when a button of the pointer is released on the surface.
			 * @note Override this method to react on the input event.
			 * @param positionX The pointer X position.
			 * @param positionY The pointer Y position.
			 * @param buttonNumber The pointer button number released.
			 * @param modifiers The keyboard modifiers held when the button has been released.
			 * @return bool
			 */
			virtual
			bool
			onButtonRelease (float positionX, float positionY, int buttonNumber, int modifiers) noexcept
			{
				/* Unused by default. */
				static_cast< void >(positionX);
				static_cast< void >(positionY);
				static_cast< void >(buttonNumber);
				static_cast< void >(modifiers);

				return this->isBlockingEvent();
			}

			/**
			 * @brief Method fired when the mouse wheel is activated on the surface.
			 * @note Override this method to react on the input event.
			 * @param positionX The pointer X position when the mouse wheel occurred.
			 * @param positionY The pointer Y modifiers the mouse wheel occurred.
			 * @param xOffset The scroll distance on the X axis.
			 * @param yOffset The scroll distance on the Y axis.
			 * @param modifiers The keyboard modifiers pressed during the scroll (Ctrl, Shift, Alt, etc.).
			 * @return bool
			 */
			virtual
			bool
			onMouseWheel (float positionX, float positionY, float xOffset, float yOffset, int32_t modifiers = 0) noexcept
			{
				/* Unused by default. */
				static_cast< void >(positionX);
				static_cast< void >(positionY);
				static_cast< void >(xOffset);
				static_cast< void >(yOffset);
				static_cast< void >(modifiers);

				return this->isBlockingEvent();
			}

		private:

			/**
			 * @brief Gets a Vulkan sampler.
			 * @param renderer A reference to the renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool getSampler (Graphics::Renderer & renderer) noexcept;

			/**
			 * @brief Creates all GPU resources for a framebuffer.
			 * @details Creates the Vulkan image, image view, and descriptor set for the given
			 * framebuffer structure. When memory mapping is disabled, the pixmap must be initialized
			 * before calling this method. When memory mapping is enabled, width and height are used directly.
			 * @param buffer A reference to the framebuffer to populate.
			 * @param renderer A reference to the graphics renderer.
			 * @param width The texture width in pixels.
			 * @param height The texture height in pixels.
			 * @return bool True on success, false on failure.
			 */
			[[nodiscard]]
			bool createFramebufferResources (Framebuffer & buffer, Graphics::Renderer & renderer, uint32_t width, uint32_t height) const noexcept;

			/**
			 * @brief Updates the model matrix to place the surface on screen.
			 * @return void
			 */
			void updateModelMatrix () noexcept;

			/**
			 * @brief Updates the physical representation of the surface in video memory.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool updatePhysicalRepresentation (Graphics::Renderer & renderer) noexcept;

			/**
			 * @brief Called when the active buffer is ready for use.
			 * @details Override this method to be notified when the active buffer has been
			 * created or recreated. This is called:
			 * - After initial creation in createOnHardware()
			 * - After resize in single buffer mode (m_useTransitionBuffer = false)
			 * @param framebuffer A reference to the active framebuffer.
			 */
			virtual
			void
			onActiveBufferReady (Framebuffer & framebuffer) noexcept
			{
				static_cast< void >(framebuffer);
			}

			/**
			 * @brief Called when the transition buffer is ready for content.
			 * @details Override this method to be notified when the transition buffer
			 * has been recreated with a new size and is waiting for content.
			 *
			 * For async content providers (CEF, video decoder, etc.):
			 * - Notify your external renderer to produce content at the new size
			 * - Check isTransitionBufferReady() and call commitTransitionBuffer() when ready
			 *
			 * @note canDraw() returns true when this callback is invoked.
			 * @param framebuffer A reference to the transition framebuffer.
			 */
			virtual
			void
			onTransitionBufferReady (Framebuffer & framebuffer) noexcept
			{
				static_cast< void >(framebuffer);
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Surface & obj);

			const FramebufferProperties & m_framebufferProperties;
			Libs::Math::Space2D::AARectangle< float > m_rectangle{0.0F, 0.0F, 1.0F, 1.0F};
			Libs::Math::Matrix< 4, float > m_modelMatrix;
			Framebuffer m_activeBuffer;
			Framebuffer m_transitionBuffer;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			mutable std::mutex m_framebufferAccess;
			float m_depth{0.0F};
			float m_alphaThreshold{0.1F};
			TransitionBufferStatus m_transitionBufferStatus{TransitionBufferStatus::Ready};
			bool m_videoMemorySizeValid{false};
			bool m_videoMemoryUpToDate{false};
			bool m_transitionBufferEnabled{false};
			bool m_disablePixmapCopyInTransitionBuffer{false};
			bool m_memoryMappingEnabled{false};
			bool m_isVisible{false};
			bool m_isListeningKeyboard{false};
			bool m_isListeningPointer{false};
			bool m_isFocused{false};
			bool m_isOpaque{false};
			bool m_isAlphaTestEnabled{false};
			bool m_lockPointerMoveEvents{false};
			bool m_processUnblockedPointerEvents{false};
			bool m_isPointerWasOver{false};
	};

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	std::string to_string (const Surface & obj);
}
