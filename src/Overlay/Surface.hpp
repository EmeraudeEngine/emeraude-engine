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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_config.hpp"
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <string>
#include <concepts>
#include <functional>
#include <chrono>

/* Local inclusions for usages. */
#include "Math/Matrix.hpp"
#include "Math/Space2D/AARectangle.hpp"
#include "NameableTrait.hpp"
#include "Tracer.hpp"
#include "Framebuffer.hpp"
#include "FramebufferProperties.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Sampler;
	class CommandPool;
	class CommandBuffer;
	struct ExternalImageDescriptor;

	namespace Sync
	{
		class Fence;
	}
}

namespace EmEn::Overlay
{
	/**
	 * @brief Defines the transition buffer synchronization status for async content providers.
	 * @details Used to coordinate resize operations with asynchronous content providers
	 * like CEF browsers, video decoders, or streaming sources.
	 */
	enum class EMEN_API TransitionBufferStatus : uint8_t
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
	enum class EMEN_API TargetBuffer : uint8_t
	{
		/** Frame dimensions don't match any buffer - skip the frame. */
		None,
		/** Frame matches active buffer dimensions - normal operation. */
		Active,
		/** Frame matches transition buffer dimensions - completing a resize. */
		Transition
	};

	/**
	 * @brief The base class for overlay UIScreen surfaces.
	 * @extends Base::NameableTrait A surface has a name.
	 */
	class EMEN_LEAN_API Surface : public Base::NameableTrait
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"OverlaySurface"};

			/**
			 * @brief GPU upload accounting for one measurement window (diagnostic only).
			 * @details Every entry is accumulated by processUpdates() on the render thread. The
			 * three byte counters answer the same question three ways, which is the whole point:
			 *  - uploadedBytes: what the upload costs TODAY (the whole pixmap, every time),
			 *  - regionBytes: what a tight bounding-box upload of the touched area would cost,
			 *  - bandBytes: what a full-width row-band upload (the simplest sub-region copy that
			 *    keeps the staging buffer layout linear) would cost.
			 * uploadedBytes / bandBytes is therefore the achievable gain factor, and
			 * uploadedBytes / regionBytes the theoretical ceiling above it.
			 * @note Not thread-safe by design: written and read on the render thread only.
			 */
			struct UploadStatistics
			{
				/** @brief Number of GPU uploads performed. */
				uint64_t uploadCount{0};
				/** @brief Bytes ACTUALLY moved to the GPU. */
				uint64_t uploadedBytes{0};
				/** @brief Bytes a full-image upload would have moved - the baseline to compare against. */
				uint64_t fullBytes{0};
				/** @brief Bytes a tight bounding-box upload would have moved - the ceiling. */
				uint64_t regionBytes{0};
				/** @brief Bytes a full-width row-band upload would have moved - what the current strategy targets. */
				uint64_t bandBytes{0};
				/** @brief Cumulated upload duration, in microseconds. */
				uint64_t writeDurationUS{0};
				/** @brief Uploads that took the partial (row-band) path. */
				uint64_t partialCount{0};
				/** @brief Uploads whose touched region already covered the whole pixmap (nothing to win). */
				uint64_t saturatedCount{0};
				/** @brief Uploads carrying no valid touched region (upload forced without a blit). */
				uint64_t unknownRegionCount{0};
			};

			/**
			 * @brief Constructs a surface.
			 * @note The stack ordering (rendering order, input dispatch priority) is owned by
			 * the parent UIScreen — surfaces no longer carry an explicit depth in their public
			 * API. The UIScreen assigns an internal depth value automatically based on the
			 * surface's index in its stack. Use UIScreen's stack mutation methods
			 * (bringToFront, sendToBack, moveAbove, moveBelow, ...) to reorder.
			 * @param framebufferProperties A reference to the overlay framebuffer properties.
			 * @param name A string [std::move].
			 * @param rectangle A reference to a rectangle for the surface geometry on screen.
			 * @param visible Set visibility state on startup.
			 */
			Surface (const FramebufferProperties & framebufferProperties, std::string name, const Base::Math::Space2D::AARectangle< float > & rectangle, bool visible) noexcept
				: NameableTrait{std::move(name)},
				m_framebufferProperties{framebufferProperties},
				m_latchedProperties{framebufferProperties},
				m_rectangle{rectangle},
				m_isVisible{visible}
			{
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
			 * @brief Enables the accelerated (zero-copy GPU) content source mode.
			 * @details The surface image becomes a pure GPU→GPU copy target (DEVICE_LOCAL, OPTIMAL,
			 * TRANSFER_DST|SAMPLED) with NO CPU pixmap and NO staging upload. Content arrives through
			 * importAcceleratedFrame() — an external GPU texture (e.g. a CEF shared texture) imported
			 * and copied on the GPU. Mutually exclusive with the memory-mapping mode (forced to staging
			 * layout, mapping disabled).
			 * @warning Must be called BEFORE createOnHardware(). The CPU-side alpha-test event blocking
			 * (isEventBlocked) degrades: without a pixmap, per-pixel alpha reads return transparent.
			 * @return void
			 */
			void
			enableAcceleratedSource () noexcept
			{
				m_acceleratedSourceEnabled = true;
			}

			/**
			 * @brief Returns whether the accelerated (zero-copy GPU) content source mode is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAcceleratedSourceEnabled () const noexcept
			{
				return m_acceleratedSourceEnabled;
			}

			/**
			 * @brief Imports an external GPU frame (zero-copy shared texture) and copies it into this surface.
			 * @details The whole operation is performed synchronously inside the call, per the producer's
			 * handle-validity contract (for CEF the handle dies when OnAcceleratedPaint returns):
			 * import the external texture as a VkImage, record acquire barriers, vkCmdCopyImage toward the
			 * active (or transition) buffer image, restore SHADER_READ_ONLY, submit on the renderer's frame
			 * queue and WAIT the fence, then destroy the imported image. The frame is routed by size through
			 * the transition-buffer machinery (same rules as the CPU paths): a mismatching frame is clamped
			 * into the active buffer and a transition resize is requested for convergence.
			 * @note Callable from the content provider's thread once the surface exists on the GPU
			 * (createOnHardware() caches the renderer).
			 * @param descriptor A reference to the external image descriptor filled by the producer.
			 * @return bool True when the frame was copied (or safely clamped), false on failure.
			 */
			[[nodiscard]]
			bool importAcceleratedFrame (const Vulkan::ExternalImageDescriptor & descriptor) noexcept;

			/**
			 * @brief Imports an external GPU popup frame (e.g. a CEF <select> dropdown) into the popup cache.
			 * @details The popup content is cached in a persistent surface-owned GPU image (recreated when
			 * the popup size changes), then composited onto the active buffer at the popup position. The
			 * cache exists because every accelerated view frame is a FULL copy that erases the popup — the
			 * view path re-composites the cache after each frame while the popup is visible (mirror of the
			 * CPU compositePopup() semantics). Synchronous, same handle contract as importAcceleratedFrame().
			 * @param descriptor A reference to the external image descriptor filled by the producer.
			 * @return bool True when the popup frame was cached and composited.
			 */
			[[nodiscard]]
			bool importAcceleratedPopupFrame (const Vulkan::ExternalImageDescriptor & descriptor) noexcept;

			/**
			 * @brief Shows or hides the accelerated popup overlay.
			 * @details On hide, the popup cache is released — the next view frame (a full copy)
			 * naturally erases the popup from the surface.
			 * @param visible The popup visibility state.
			 * @return void
			 */
			void setAcceleratedPopupVisible (bool visible) noexcept;

			/**
			 * @brief Sets the accelerated popup position on the surface (view coordinates, pixels).
			 * @param positionX The popup X position (may be negative — clamped at composite time).
			 * @param positionY The popup Y position (may be negative — clamped at composite time).
			 * @return void
			 */
			void
			setAcceleratedPopupPosition (int32_t positionX, int32_t positionY) noexcept
			{
				m_acceleratedPopupX = positionX;
				m_acceleratedPopupY = positionY;
			}

			/**
			 * @brief CPU-to-GPU memory-mapping policy for the surface image. Resolved against the device
			 * at creation: Auto maps only on unified-memory devices (integrated GPUs, software, full-ReBAR
			 * discrete GPUs) where the image is not sampled across PCIe.
			 */
			enum class MemoryMappingMode : uint8_t
			{
				/** Always use a staging upload (DEVICE_LOCAL OPTIMAL image). */
				Staging,
				/** Force direct CPU mapping (LINEAR host-visible image) when the format allows it. */
				Direct,
				/** Decide from the device memory architecture (unified -> map, discrete -> staging). */
				Auto
			};

			/**
			 * @brief Enable the GPU image to be mappable from the CPU for direct writing.
			 * @note Equivalent to setMemoryMappingMode(MemoryMappingMode::Direct).
			 */
			void
			enableMapping () noexcept
			{
				m_memoryMappingMode = MemoryMappingMode::Direct;
			}

			/**
			 * @brief Sets the CPU-to-GPU memory-mapping mode. Resolved against the device in createOnHardware().
			 * @param mode Off (staging), On (force mapping), Auto (decide from the device).
			 */
			void
			setMemoryMappingMode (MemoryMappingMode mode) noexcept
			{
				m_memoryMappingMode = mode;
			}

			/**
			 * @brief Parses a memory-mapping mode from a setting value ("on"/"off"/"auto", tolerant; unknown -> Auto).
			 * @param value The setting string.
			 * @return MemoryMappingMode
			 */
			[[nodiscard]]
			static MemoryMappingMode parseMemoryMappingMode (const std::string & value) noexcept;

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
			 * @brief Returns the latched framebuffer-properties snapshot for asynchronous-provider surfaces.
			 * @details Unlike framebufferProperties() (the live, shared properties that mutate the instant
			 * the OS scale/size flips), this is a value copy that only advances when the provider calls
			 * syncPropertiesLatch(). A provider whose content is produced off the render thread / in another
			 * process (e.g. an OSR web-view) reads THIS so it always observes a coherent (scale, size) tuple,
			 * never a half-applied transition. Seeded to the live properties at construction.
			 * @return const FramebufferProperties &
			 */
			[[nodiscard]]
			const FramebufferProperties &
			latchedProperties () const noexcept
			{
				return m_latchedProperties;
			}

			/**
			 * @brief Returns the surface geometry.
			 * @return const Base::Math::Space2D::AARectangle< float > &
			 */
			[[nodiscard]]
			const Base::Math::Space2D::AARectangle< float > &
			geometry () const noexcept
			{
				return m_rectangle;
			}

			/**
			 * @brief Returns the model matrix to place the surface on screen.
			 * @return const Base::Math::Matrix< 4, float > &
			 */
			[[nodiscard]]
			const Base::Math::Matrix< 4, float > &
			modelMatrix () const noexcept
			{
				return m_modelMatrix;
			}

			/**
			 * @brief Returns the pixmap from the active buffer.
			 * @warning Use the active buffer mutex before writing into the pixmap with Surface::activeBufferMutex().
			 * @return Base::PixelFactory::Pixmap< uint8_t > &
			 */
			[[nodiscard]]
			Base::PixelFactory::Pixmap< uint8_t > &
			activePixmap () noexcept
			{
				return m_activeBuffer.pixmap;
			}

			/**
			 * @brief Returns the pixmap from the transition buffer.
			 * @note Only meaningful when resize transition is enabled. Used during resize
			 * to prepare content at the new size while the active buffer continues rendering.
			 * @return Base::PixelFactory::Pixmap< uint8_t > &
			 */
			[[nodiscard]]
			Base::PixelFactory::Pixmap< uint8_t > &
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
			setGeometry (const Base::Math::Space2D::AARectangle< float > & rectangle) noexcept
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

				this->notifyRedrawRequired();
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

				this->notifyRedrawRequired();
			}

			/**
			 * @brief Shows the web view.
			 * @return void
			 */
			void
			show () noexcept
			{
				m_isVisible = true;

				this->notifyRedrawRequired();
			}

			/**
			 * @brief Hides the web view.
			 * @return void
			 */
			void
			hide () noexcept
			{
				m_isVisible = false;

				this->notifyRedrawRequired();
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

				this->notifyRedrawRequired();
			}

			/**
			 * @brief Declares the video memory content outdated to re-upload it.
			 * @return void
			 */
			void
			setVideoMemoryOutdated () noexcept
			{
				m_videoMemoryUpToDate = false;

				this->notifyRedrawRequired();
			}

			/**
			 * @brief Installs the callback invoked whenever this surface changes in a way that
			 * affects the rendered image (content, geometry, visibility, stack order).
			 * @details Used by the on-demand rendering mode to wake the rendering thread. The
			 * UIScreen owning the surface installs it; in continuous rendering it stays empty.
			 * @param requester A callable invoked on every visual mutation, or an empty function to detach.
			 * @return void
			 */
			void
			setRedrawRequester (std::function< void () > requester) noexcept
			{
				m_redrawRequester = std::move(requester);
			}

			/**
			 * @brief Requests a re-composite of the overlay WITHOUT marking the video memory outdated.
			 * @details Fires the installed redraw requester only. Use when the on-screen result changed
			 * but the GPU texture is already current, so no re-upload is needed - only a re-composite.
			 * The canonical case is a memory-mapped CEF paint that writes pixels straight into device
			 * memory (directPaint): it needs the on-demand rendering thread woken, but must NOT trigger
			 * the CPU->GPU upload path that setVideoMemoryOutdated() would. No-op in continuous rendering.
			 * @return void
			 * @see setVideoMemoryOutdated(), setRedrawRequester()
			 */
			void
			requestRedraw () noexcept
			{
				this->notifyRedrawRequired();
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
			 * @tparam function_t Function type accepting (void* mappedPtr, VkDeviceSize rowPitch) and returning bool.
			 * @param writeFunction The function to call with the mapped memory.
			 * @return bool True if mapping and write succeeded, false otherwise.
			 */
			template< typename function_t >
			requires std::invocable< function_t, void *, VkDeviceSize > && std::convertible_to< std::invoke_result_t< function_t, void *, VkDeviceSize >, bool >
			[[nodiscard]]
			bool
			writeActiveBufferWithMapping (function_t && writeFunction) const noexcept
			{
				return m_activeBuffer.writeWithMapping(std::forward< function_t >(writeFunction));
			}

			/**
			 * @brief Writes to the transition buffer GPU image using memory mapping.
			 * @details Convenience method that wraps transitionBuffer().writeWithMapping().
			 * Maps the GPU memory, calls the provided function, then unmaps automatically.
			 * @tparam function_t Function type accepting (void* mappedPtr, VkDeviceSize rowPitch) and returning bool.
			 * @param writeFunction The function to call with the mapped memory.
			 * @return bool True if mapping and write succeeded, false otherwise.
			 */
			template< typename function_t >
			requires std::invocable< function_t, void *, VkDeviceSize > && std::convertible_to< std::invoke_result_t< function_t, void *, VkDeviceSize >, bool >
			[[nodiscard]]
			bool
			writeTransitionBufferWithMapping (function_t && writeFunction) const noexcept
			{
				return m_transitionBuffer.writeWithMapping(std::forward< function_t >(writeFunction));
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
			/**
			 * @brief Returns the GPU upload statistics accumulated since the last reset.
			 * @note Render thread only. @see UploadStatistics
			 * @return const UploadStatistics &
			 */
			[[nodiscard]]
			const UploadStatistics &
			uploadStatistics () const noexcept
			{
				return m_uploadStatistics;
			}

			/**
			 * @brief Clears the GPU upload statistics, starting a new measurement window.
			 * @note Render thread only.
			 * @return void
			 */
			void
			resetUploadStatistics () noexcept
			{
				m_uploadStatistics = {};
			}

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
			 * @brief Requests a recreation of the transition buffer at an explicit pixel size.
			 * @details For asynchronous content providers (e.g. CEF), the provider is the source
			 * of truth for the painted pixel size. The engine's surface-size formula and the
			 * provider's own device-scale rounding can diverge by a sub-pixel on fractional
			 * display scales (e.g. 125%), so the painted frame may match neither the active nor
			 * the transition buffer — which would otherwise stall the resize commit (black render).
			 * Call this from the provider's paint callback with the actually-painted size: the
			 * transition buffer is then recreated at that size on the render thread (next
			 * processUpdates()), so the following identical frame can commit.
			 * @note Thread-safe: only records the requested size under a dedicated mutex, performs
			 * no GPU work. The actual recreation happens on the render thread.
			 * @param width The painted frame width in pixels.
			 * @param height The painted frame height in pixels.
			 * @return void
			 */
			void requestTransitionBufferResize (uint32_t width, uint32_t height) noexcept;

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

				if constexpr ( PointerInputDebugEnabled )
				{
					TraceDebug{ClassId} << "The surface " << this->name() << " received an unused keyboard key press event!";
				}

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

				if constexpr ( PointerInputDebugEnabled )
				{
					TraceDebug{ClassId} << "The surface " << this->name() << " received an unused keyboard key release event!";
				}

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

				if constexpr ( PointerInputDebugEnabled )
				{
					TraceDebug{ClassId} << "The surface " << this->name() << " received an unused keyboard character type event!";
				}

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

				if constexpr ( PointerInputDebugEnabled )
				{
					TraceDebug{ClassId} << "The surface " << this->name() << " received an unused pointer enter event!";
				}
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

				if constexpr ( PointerInputDebugEnabled )
				{
					TraceDebug{ClassId} << "The surface " << this->name() << " received an unused pointer leave event!";
				}
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

				if constexpr ( PointerInputDebugEnabled )
				{
					TraceDebug{ClassId} << "The surface " << this->name() << " received an unused pointer move event!";
				}

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

				if constexpr ( PointerInputDebugEnabled )
				{
					TraceDebug{ClassId} << "The surface " << this->name() << " received an unused pointer button press event!";
				}

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

				if constexpr ( PointerInputDebugEnabled )
				{
					TraceDebug{ClassId} << "The surface " << this->name() << " received an unused pointer button release event!";
				}

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

				if constexpr ( PointerInputDebugEnabled )
				{
					TraceDebug{ClassId} << "The surface " << this->name() << " received an unused mouse wheel event!";
				}

				return this->isBlockingEvent();
			}

		protected:

			/**
			 * @brief Advances the latched framebuffer-properties snapshot to the current live properties.
			 * @details For asynchronous-provider surfaces (see latchedProperties()): call when it is safe to
			 * move to a new (scale, size) — typically together with the buffer transition the provider drives
			 * and its own resize/scale notification — so every provider-facing read observes a coherent
			 * snapshot rather than a scale that changed mid-flight.
			 * @return void
			 */
			void
			syncPropertiesLatch () noexcept
			{
				m_latchedProperties = m_framebufferProperties;
			}

		private:

			/* NOTE: Diagnostic accounting, render thread only. @see UploadStatistics */
			UploadStatistics m_uploadStatistics{};

			/**
			 * @brief Accumulates one GPU upload into the diagnostic statistics.
			 * @note Render thread only, called from processUpdates() under m_framebufferAccess.
			 * @param touchedRegion The pixmap region actually written since the previous upload.
			 * @param writeDuration The duration of the Image::writeData() call.
			 * @return void
			 */
			void accountUpload (const Base::Math::Space2D::AARectangle< uint32_t > & touchedRegion, uint64_t uploadedBytes, bool partial, std::chrono::steady_clock::duration writeDuration) noexcept;

			/**
			 * @brief Uploads the active pixmap to the GPU, only the touched rows when possible.
			 * @details Falls back to a full-image upload whenever a partial one is not provably safe:
			 * no valid touched region, an image that never received a complete upload, a size
			 * mismatch, more than one array layer, or a failed partial transfer. @see UploadStatistics
			 * @param renderer A reference to the graphics renderer.
			 * @param touchedRegion The pixmap region written since the previous upload.
			 * @param uploadedBytes A writable reference receiving the bytes actually moved.
			 * @param partial A writable reference telling whether the partial path was taken.
			 * @return bool
			 */
			[[nodiscard]]
			bool uploadActiveBuffer (Graphics::Renderer & renderer, const Base::Math::Space2D::AARectangle< uint32_t > & touchedRegion, uint64_t & uploadedBytes, bool & partial) noexcept;

			/* NOTE: UIScreen owns the stack ordering. It is the only entity allowed to
			 * assign the internal depth value used for the model matrix Z translation
			 * (and hence draw-call order). Surfaces never carry a user-facing depth. */
			friend class UIScreen;

			/**
			 * @brief Notifies the installed redraw requester that this surface changed visually.
			 * @details Invoked by every visual mutation (content, geometry, visibility, stack order)
			 * so the on-demand rendering mode can wake the rendering thread. No-op when no requester
			 * is installed (continuous rendering). @see setRedrawRequester()
			 * @return void
			 */
			void
			notifyRedrawRequired () const noexcept
			{
				if ( m_redrawRequester )
				{
					m_redrawRequester();
				}
			}

			/**
			 * @brief Sets the surface depth from its stack index in the parent UIScreen.
			 * @details Called by UIScreen after any stack mutation (creation, bring/send,
			 * move above/below, destruction of a peer). The depth is computed as
			 * `index * StackDepthStep` and fed to the model matrix Z translation, keeping
			 * the GPU pipeline consistent without exposing depth as a public concept.
			 * @param stackIndex The 0-based index of this surface in the UIScreen stack
			 * (0 = bottom, N-1 = top).
			 * @return void
			 */
			void
			setStackIndex (size_t stackIndex) noexcept
			{
				constexpr auto StackDepthStep = 0.001F;

				m_depth = static_cast< float >(stackIndex) * StackDepthStep;

				this->updateModelMatrix();

				this->notifyRedrawRequired();
			}

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
			 * @brief Recreates the transition buffer at the size requested by the content provider.
			 * @details Dedicated path for the content-provider-driven resize (see
			 * requestTransitionBufferResize()). Unlike updatePhysicalRepresentation(), it does NOT
			 * recompute the size from the engine's surface-size formula — it uses the exact painted
			 * size recorded by the provider, which is authoritative. A no-op (returns true) when no
			 * resize was requested or the transition buffer already matches the requested size.
			 * @warning Must be called on the render thread while holding m_framebufferAccess (i.e.
			 * from processUpdates()). It performs GPU resource destruction/creation and a waitIdle().
			 * It deliberately does NOT call onTransitionBufferReady(): the provider is already
			 * painting at this size, so notifying it again (which triggers a CEF WasResized()) could
			 * restart the convergence loop.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool True on success (including the no-op case), false on GPU failure.
			 */
			[[nodiscard]]
			bool recreateTransitionBufferToRequestedSize (Graphics::Renderer & renderer) noexcept;

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
			friend EMEN_API std::ostream & operator<< (std::ostream & out, const Surface & obj);

			/**
			 * @brief Lazily creates the one-shot GPU resources (command pool/buffer + fence) used by importAcceleratedFrame().
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareAcceleratedCopyResources (Graphics::Renderer & renderer) noexcept;

			/**
			 * @brief Records the popup-cache composite onto a target image (already in TRANSFER_DST layout).
			 * @note No-op when the popup is hidden or the cache is absent. The copy region is clamped
			 * to the target boundaries (negative positions and overflow handled).
			 * @param targetImage A reference to the target image.
			 * @return void
			 */
			void recordAcceleratedPopupComposite (Vulkan::Image & targetImage) noexcept;

			/* NOTE: Cached by createOnHardware() so content providers (importAcceleratedFrame) can
			 * reach the renderer from their own thread. The renderer outlives every overlay surface. */
			Graphics::Renderer * m_renderer{nullptr};
			const FramebufferProperties & m_framebufferProperties;
			FramebufferProperties m_latchedProperties; ///< Provider-facing latched snapshot — see latchedProperties()/syncPropertiesLatch().
			Base::Math::Space2D::AARectangle< float > m_rectangle{0.0F, 0.0F, 1.0F, 1.0F};
			Base::Math::Matrix< 4, float > m_modelMatrix;
			Framebuffer m_activeBuffer;
			Framebuffer m_transitionBuffer;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			/* NOTE: One-shot resources for the accelerated-frame GPU copy (lazily created, CEF-thread only). */
			std::shared_ptr< Vulkan::CommandPool > m_acceleratedCommandPool;
			std::shared_ptr< Vulkan::CommandBuffer > m_acceleratedCommandBuffer;
			std::shared_ptr< Vulkan::Sync::Fence > m_acceleratedFence;
			/* NOTE: Persistent GPU cache of the popup content (accelerated mode). Re-composited after
			 * every view frame while visible; released on hide. Content-provider thread only. */
			std::shared_ptr< Vulkan::Image > m_acceleratedPopupImage;
			int32_t m_acceleratedPopupX{0};
			int32_t m_acceleratedPopupY{0};
			bool m_acceleratedPopupVisible{false};
			mutable std::mutex m_framebufferAccess;
			mutable std::mutex m_requestedTransitionSizeMutex;
			uint32_t m_requestedTransitionWidth{0};
			uint32_t m_requestedTransitionHeight{0};
			float m_depth{0.0F};
			float m_alphaThreshold{0.1F};
			TransitionBufferStatus m_transitionBufferStatus{TransitionBufferStatus::Ready};
			MemoryMappingMode m_memoryMappingMode{MemoryMappingMode::Staging};
			std::function< void () > m_redrawRequester{}; ///< Invoked on any visual mutation to request a redraw (on-demand rendering). Empty in continuous rendering. @see setRedrawRequester()
			bool m_transitionResizeRequested{false};
			bool m_acceleratedSourceEnabled{false};
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
	EMEN_API std::string to_string (const Surface & obj);
}
