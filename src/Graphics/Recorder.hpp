/*
 * src/Graphics/Recorder.hpp
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
#include <cstdint>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "vpx/vpx_encoder.h"
#include "VideoFrameConverter.hpp"
#include "Vulkan/Buffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/CommandPool.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sync/Fence.hpp"
#include "Vulkan/Sync/Semaphore.hpp"
#include "Vulkan/VideoEncoderH265.hpp"
#ifdef _MSC_VER
	#pragma warning(push)
	#pragma warning(disable: 4505) /* unreferenced function with internal linkage has been removed */
#endif
#ifdef _MSC_VER
	#pragma warning(pop)
#endif

namespace EmEn
{
	namespace Graphics
	{
		class Renderer;
	}

	class PrimaryServices;
}

namespace EmEn::Graphics
{
	/**
	 * @class Recorder
	 * @brief Video recording service that captures the framebuffer and encodes it as VP9/IVF.
	 *
	 * Studio-quality video recording of the Vulkan swap-chain framebuffer using asynchronous
	 * GPU readback, a bounded grab buffer, and SIMD-accelerated VP9 software encoding. There
	 * is ONE mode: quality-first, constant frame rate (CFR) on the wall clock — the video
	 * timeline is real time, so the separately recorded audio tracks stay in sync.
	 * The recording pipeline operates in three stages:
	 * 1. The copy of the finished swap-chain image is recorded INSIDE the frame's own command
	 *    buffer (Renderer::renderFrame(), recordFrameCopy()) at the target FPS (wall-clock
	 *    pacing), and the frame's in-flight fence releases it (onFrameSlotRetired())
	 * 2. Bounded grab buffer accumulates frames for the encoding thread; above the bound,
	 *    captures are skipped (backpressure) so a slow encode cannot balloon memory
	 * 3. Dedicated encoding thread drains the buffer, converts BGRA to I420 (BT.709 limited
	 *    range, signalled in the bitstream), encodes VP9 (VBR, lookahead), and writes the
	 *    IVF container as CONSTANT frame rate: any missing capture slot (renderer slower
	 *    than the target FPS, backpressure skip) is filled by re-encoding the previous
	 *    image — a static VP9 frame costs almost nothing and the timeline never judders
	 *
	 * PTS timing uses wall-clock milliseconds with smoothing to handle variable game framerate
	 * while maintaining correct playback speed. Recording dimensions are locked at start time.
	 *
	 * @see EmEn::ServiceInterface
	 * @version 0.8.51
	 */
	class EMEN_API Recorder final : public ServiceInterface
	{
		public:

			/** @brief Service identifier for logging and registration. */
			static constexpr auto ClassId{"GraphicsRecorderService"};

			/**
			 * @brief Constructs the video recorder service.
			 *
			 * @param primaryServices Reference to primary services for settings and filesystem access.
			 * @param renderer Reference to the graphics renderer for swap-chain access.
			 */
			Recorder (PrimaryServices & primaryServices, Renderer & renderer) noexcept;

			/**
			 * @brief Starts video recording to an IVF file.
			 *
			 * Initializes the VP9 encoder, creates async GPU readback resources, starts the
			 * encoding thread, and begins capturing frames. Recording dimensions are
			 * locked to the current framebuffer size (rounded down to even values for I420).
			 *
			 * @param outputPath The filesystem path for the output IVF file.
			 * @return True if recording started successfully, false otherwise.
			 * @pre Framebuffer dimensions must be non-zero.
			 * @post Encoding thread is running and GPU resources are allocated.
			 * @note If already recording, this method returns false and logs a warning.
			 */
			bool startRecording (const std::filesystem::path & outputPath) noexcept;

			/**
			 * @brief Stops video recording and finalizes the output file.
			 *
			 * No frame copy is recorded from this call on. The session closes at once when no copy is on the GPU
			 * any more, otherwise on the rendering thread as soon as the last one lands (within framesInFlight()
			 * frames): signals the encoding thread to stop, flushes the encoder, finalizes the file, releases the
			 * GPU resources.
			 *
			 * @return True if recording was active and stopped successfully, false if not recording.
			 * @post isRecording() returns false; startRecording() refuses a new rush until the session is closed.
			 */
			bool stopRecording () noexcept;

			/**
			 * @brief Checks if recording is currently active.
			 *
			 * @return True if recording, false otherwise.
			 */
			[[nodiscard]]
			bool isRecording () const noexcept;

			/**
			 * @brief Returns whether the frame being recorded should carry a copy for the video.
			 * @note Rendering thread, inside Renderer::renderFrame(). The frame qualifies when recording and when it
			 * opens a CFR slot not served yet (shouldCaptureFrame()). One atomic load when not recording.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			wantsFrame () const noexcept
			{
				return m_isRecording.load(std::memory_order_acquire) && this->shouldCaptureFrame();
			}

			/**
			 * @brief Records the copy of the finished swap-chain image into the frame's own command buffer.
			 *
			 * Called after the last pass (overlay included) and before the command buffer ends, while the image is
			 * still ACQUIRED: the copy belongs to the very batch that rendered the frame, so it can only see that
			 * frame. The image is left in its final layout for the present.
			 * ⚠️⚠️ It used to be a separate submit made after the present, on whatever graphics queue
			 * Vulkan::Device::getGraphicsQueue() handed out: that accessor ROTATES over every queue of the family
			 * (16 on NVIDIA) while the renderer keeps one, nothing ordered the two, and a GPU running behind copied
			 * the image BEFORE the frame was drawn into it — the video jumped back 3-4 frames (the swap-chain image
			 * count) while the screen stayed perfect (measured 2026-09-25: 19 of 260 frames).
			 *
			 * @param commandBuffer The frame's command buffer, recording, outside any render pass.
			 * @param image The acquired swap-chain image.
			 * @param finalLayout The layout the frame left the image in (the render pass final layout).
			 * @param frameSlot The renderer's frame slot (m_currentFrameIndex), whose fence covers this frame.
			 * @return bool True when a copy was recorded: confirmSubmit() must then follow the frame's submit.
			 */
			bool recordFrameCopy (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::Image & image, VkImageLayout finalLayout, uint32_t frameSlot) noexcept;

			/**
			 * @brief Tells the recorder whether the frame carrying the copy recorded by recordFrameCopy() reached the GPU.
			 * @note An abandoned frame frees its slot at once: the CFR timeline fills the gap downstream.
			 * @param submitted True when the frame's batch was submitted.
			 */
			void confirmSubmit (bool submitted) noexcept;

			/**
			 * @brief The renderer waited the frame slot's in-flight fence: the copies recorded with it are complete.
			 * @note Rendering thread, every frame. The hardware path hands the snapshot to the encoding thread, the
			 * software path reads the staging buffer back (or starts its transfer-queue DMA). One atomic load when no
			 * session is open.
			 * @param frameSlot The renderer's frame slot whose fence was just waited.
			 */
			void onFrameSlotRetired (uint32_t frameSlot) noexcept;

			/**
			 * @brief The device is idle (swap-chain recreation): every copy submitted with a frame is complete.
			 * @note The frame slots may be rebuilt with another count behind this call, so a copy cannot wait for the
			 * fence of a slot that will never be waited again.
			 */
			void onDeviceIdle () noexcept;


			/**
			 * @brief Returns the video file extension of the active encoding path.
			 * @note "h265" (hardware Vulkan Video, muxed to MP4 by the assemble script)
			 * when the device supports it, "ivf" (software VP9, muxed to WebM) otherwise.
			 * @return const char *
			 */
			[[nodiscard]]
			const char * videoFileExtension () const noexcept;

			/**
			 * @brief Returns whether the hardware H.265 path will be used.
			 * @note False when the device lacks Vulkan Video H.265 encode OR when
			 * `Core/RushMaker/ForceCPUEncoding` is set (A/B lever).
			 * @return bool
			 */
			[[nodiscard]]
			bool hardwarePath () const noexcept;

			/**
			 * @brief Returns the target recording framerate.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			targetFramerate () const noexcept
			{
				return m_targetFramerate;
			}

			/**
			 * @brief Checks whether the current frame should be captured: it opens a CFR slot not served yet.
			 *
			 * The video timeline is the wall clock cut into slots of 1 / target FPS (cfrSlotAt()). The first
			 * rendered frame inside a slot is captured, so a game rendering faster than the target fills EVERY
			 * slot, and a slot with no rendered frame at all becomes a CFR duplicate downstream.
			 * ⚠️⚠️ It used to wait one frame duration after the LAST CAPTURE: the capture then lagged behind the
			 * slot grid by up to one render interval every time, drifted to a lower rate (48 FPS rendering →
			 * ~24 captures/s for a 30 FPS timeline) and every fourth slot came out empty — 21 % of duplicated
			 * frames on `forest` at 48 FPS, read as hiccups in the video (measured 2026-09-24).
			 *
			 * @return True if a frame should be captured now, false otherwise.
			 * @note Returns false immediately if not recording.
			 */
			[[nodiscard]]
			bool shouldCaptureFrame () const noexcept;

			/**
			 * @brief Returns the CFR slot (the frame index of the video timeline) a wall-clock instant falls in.
			 * @note THE single formula of the timeline: the capture gate and every PTS use it, so they cannot disagree.
			 * @param instant The instant.
			 * @return int64_t
			 */
			[[nodiscard]]
			int64_t cfrSlotAt (std::chrono::steady_clock::time_point instant) const noexcept;

			/**
			 * @brief Returns the recommended audio bitrate in kbps based on the current quality preset.
			 *
			 * @return Audio bitrate in kbps (128, 192, 256, or 320).
			 */
			[[nodiscard]]
			unsigned int recommendedAudioBitrate () const noexcept;

		private:

			/**
			 * @enum CopyState
			 * @brief Where the copy held by a capture slot stands.
			 */
			enum class CopyState : uint8_t
			{
				Free, ///< The slot can take the next copy.
				Recorded, ///< Recorded in a frame's command buffer, the frame not submitted yet.
				Submitted, ///< On the GPU with its frame, waiting for the frame slot's fence.
				Transferring, ///< Software path, transfer queue: the device-local copy is being DMAed to the host.
				Queued ///< Hardware path: complete and handed to the encoding thread, which frees it.
			};

			/** @brief No slot recorded in the current frame. */
			static constexpr size_t NoSlot{static_cast< size_t >(-1)};

			/**
			 * @brief Initializes video recording configuration from settings.
			 *
			 * Reads target FPS, bitrate, and debug stats flag from settings. Computes
			 * frame duration for pacing.
			 *
			 * @return True on successful initialization.
			 */
			bool onInitialize () noexcept override;

			/**
			 * @brief Terminates the video recorder service.
			 *
			 * Ensures recording is stopped before termination.
			 *
			 * @return True on successful termination.
			 */
			bool onTerminate () noexcept override;

			/**
			 * @brief Joins and removes finished background encoding sessions.
			 * @post m_finishingSessions contains only sessions still encoding.
			 */
			void cleanupFinishedSessions () noexcept;

			/**
			 * @brief Creates the software path's readback resources (staging buffers, transfer-queue DMA resources).
			 *
			 * Initializes AsyncBufferCount readback slots, each with a persistently mapped host-visible staging
			 * buffer sized for a full BGRA framebuffer; with a dedicated transfer queue, also a device-local
			 * intermediate buffer, a DMA command buffer and its fence.
			 *
			 * @return True if all resources created successfully, false otherwise.
			 * @post m_asyncSlots are fully initialized.
			 */
			bool createAsyncResources () noexcept;

			/**
			 * @brief Destroys the software path's readback resources.
			 *
			 * Waits on every DMA fence (safe for signaled fences), then releases command buffers, fences, buffers,
			 * and the transfer command pool.
			 *
			 * @pre No copy is recorded or submitted with a frame (copiesInFlight() is false).
			 * @post All async resources are released.
			 */
			void destroyAsyncResources () noexcept;

			/**
			 * @brief Records the software path's copy: swap-chain image to the staging buffer, or to the
			 * device-local buffer when the transfer queue does the host copy.
			 * @param commandBuffer The frame's command buffer.
			 * @param image The acquired swap-chain image, in its final layout.
			 * @param finalLayout The layout to leave it in.
			 * @return size_t The slot index, NoSlot when none is free or the grab buffer is full (backpressure).
			 */
			[[nodiscard]]
			size_t recordReadbackCopy (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::Image & image, VkImageLayout finalLayout) noexcept;

			/**
			 * @brief Records the hardware path's copy: swap-chain image to a free snapshot slot.
			 * @param commandBuffer The frame's command buffer.
			 * @param image The acquired swap-chain image, in its final layout.
			 * @param finalLayout The layout to leave it in.
			 * @return size_t The slot index, NoSlot when the encoder still holds every slot.
			 */
			[[nodiscard]]
			size_t recordHardwareCopy (const Vulkan::CommandBuffer & commandBuffer, const Vulkan::Image & image, VkImageLayout finalLayout) noexcept;

			/**
			 * @brief Completes the copies submitted with a frame slot whose fence was waited.
			 * @param frameSlot The renderer's frame slot.
			 * @param everyFrameSlot True to complete every submitted copy whatever its slot (the device is idle).
			 */
			void retireCopies (uint32_t frameSlot, bool everyFrameSlot) noexcept;

			/**
			 * @brief Harvests the software path's DMA transfers that completed (non-blocking fence checks).
			 * @param wait True to wait each transfer instead (the session is closing).
			 */
			void harvestTransfers (bool wait) noexcept;

			/**
			 * @brief Reads a completed software readback back into the grab buffer for the encoding thread.
			 * @param slotIndex Index of the async readback slot, whose staging buffer holds the frame.
			 */
			void harvestReadback (size_t slotIndex) noexcept;

			/**
			 * @brief Returns whether a copy is still recorded in a frame or on the GPU.
			 * @note A hardware snapshot handed to the encoding thread is not counted: the thread drains it at the join.
			 * @return bool
			 */
			[[nodiscard]]
			bool copiesInFlight () const noexcept;

			/**
			 * @brief Closes the session once no copy is in flight any more (finalizes the file, releases the resources).
			 * @pre m_captureAccess is held and copiesInFlight() is false.
			 */
			void closeSession () noexcept;

			/** @brief Function signature for BGRA-to-I420 conversion implementations. */
			using BGRAToI420Func = void (*)(const uint8_t * bgra, uint32_t w, uint32_t h, uint8_t * y, uint8_t * u, uint8_t * v);

			/**
			 * @brief Starts a hardware (Vulkan Video H.265) recording session.
			 * @param outputPath The .h265 elementary stream path.
			 * @return bool True on success.
			 */
			[[nodiscard]]
			bool startHardwareRecording (const std::filesystem::path & outputPath) noexcept;

			/**
			 * @brief Hardware encoding thread: converts and encodes captured slots at the
			 * silicon's pace, filling missing CFR slots by re-encoding the previous frame.
			 */
			void hardwareEncodingLoop () noexcept;

			/** @brief Stops the hardware session (drains, joins, finalizes the file). */
			void stopHardwareRecording () noexcept;

			/** @brief Number of async GPU readback slots for double-buffering. */
			static constexpr size_t AsyncBufferCount{4};

			/** @brief Number of hardware capture slots (swap-chain snapshots on the GPU). */
			static constexpr size_t HardwareSlotCount{4};

			/**
			 * @struct HardwareSlot
			 * @brief One GPU snapshot of the swap-chain for the hardware encode path.
			 */
			struct HardwareSlot
			{
				std::shared_ptr< Vulkan::Image > image; ///< BGRA snapshot (TRANSFER_DST + SAMPLED).
				std::shared_ptr< Vulkan::ImageView > view; ///< Sampled view for the converter.
				int64_t cfrSlot{0}; ///< Constant-frame-rate slot of this capture.
				uint32_t frameSlot{0}; ///< The renderer's frame slot whose fence covers the copy.
				std::atomic< CopyState > state{CopyState::Free}; ///< Written back to Free by the encoding thread.
			};

			/**
			 * @struct HardwareSession
			 * @brief State of a hardware encoding session (producer: capture, consumer: encode thread).
			 */
			struct HardwareSession
			{
				std::FILE * outputFile{nullptr};
				std::filesystem::path outputPath;
				std::deque< size_t > readySlots;
				std::mutex queueMutex;
				std::condition_variable queueCV;
				std::thread encodingThread;
				std::atomic< bool > threadRunning{false};
				uint64_t frameCount{0};
				uint64_t duplicatedFrames{0};
				std::atomic< uint64_t > skippedCaptures{0};
				int64_t lastCfrSlot{-1};
			};

			/**
			 * @struct AsyncReadbackSlot
			 * @brief Represents one async GPU readback slot with command buffer, fence, and staging buffer.
			 */
			struct AsyncReadbackSlot
			{
				std::unique_ptr< Vulkan::Buffer > stagingBuffer; ///< Host-visible staging buffer for readback.
				uint8_t * mappedPtr{nullptr}; ///< Persistently mapped pointer to staging buffer (valid from create to destroy).
				/* Transfer queue path (only used when m_useTransferQueue is true). */
				std::unique_ptr< Vulkan::CommandBuffer > transferCommandBuffer; ///< Transfer queue: buffer-to-buffer DMA copy.
				std::unique_ptr< Vulkan::Sync::Fence > fence; ///< Signaled when the transfer-queue DMA completes.
				std::unique_ptr< Vulkan::Buffer > deviceLocalBuffer; ///< Device-local intermediate buffer for two-step readback.
				int64_t cfrSlot{0}; ///< Constant-frame-rate slot of this capture.
				uint32_t frameSlot{0}; ///< The renderer's frame slot whose fence covers the copy.
				CopyState state{CopyState::Free}; ///< Rendering thread only (or the closing thread, under m_captureAccess).
			};

			/**
			 * @struct FrameSlot
			 * @brief Holds a captured frame for the encoding queue.
			 */
			struct FrameSlot
			{
				std::vector< uint8_t > data; ///< BGRA pixel data (width * height * 4 bytes).
				vpx_codec_pts_t pts{0}; ///< Wall-clock presentation timestamp in timebase units.
			};

			/**
			 * @struct EncodingSession
			 * @brief Self-contained encoding session that can be detached from the Recorder.
			 *
			 * Owns the VP9 codec, IVF output file, frame queue, and encoding thread.
			 * Once detached from the Recorder, it autonomously drains remaining frames,
			 * flushes the codec, patches the IVF header, and cleans up.
			 */
			struct EncodingSession
			{
				/* Codec state. */
				vpx_codec_ctx_t codec{};
				vpx_image_t vpxImage{};
				bool codecInitialized{false};

				/* IVF output. */
				std::FILE * outputFile{nullptr};
				std::filesystem::path outputPath;

				/* Frame queue (producer-consumer). */
				std::deque< FrameSlot > frameQueue;
				std::deque< FrameSlot > freeFrames;
				std::mutex queueMutex;
				std::condition_variable queueCV;

				/* Thread control. */
				std::thread encodingThread;
				std::atomic< bool > threadRunning{false};
				std::atomic< bool > finished{false};

				/* Encoding parameters (snapshot from Recorder at session creation). */
				uint32_t recordWidth{0};
				uint32_t recordHeight{0};
				uint32_t targetFramerate{30};
				uint32_t maxQueuedFrames{32}; ///< Grab buffer bound (for the adaptive-speed watermarks).
				int cpuUsedBase{3}; ///< Preset encoder effort (quality target when the CPU keeps pace).
				std::atomic< int > cpuUsedCurrent{3}; ///< Live encoder effort, adapted to sustain the capture rate.
				uint64_t frameCount{0};
				uint64_t duplicatedFrames{0}; ///< CFR filler frames re-encoded from the previous image.
				std::atomic< uint64_t > captureCount{0};
				std::atomic< uint64_t > skippedCaptures{0};
				vpx_codec_pts_t lastEncodedPts{-1}; ///< PTS of the last frame written; -1 = nothing encoded yet.
				bool showStatistics{false};

				/* SIMD dispatch. */
				BGRAToI420Func bgraToI420{nullptr};

				/** @brief Encoding thread entry point. Drains queue, encodes, then calls finalize(). */
				void encodingThreadFunc () noexcept;

				/**
				 * @brief Encodes the current vpxImage content at the given PTS and writes the packets.
				 * @note Also used to duplicate the previous frame into empty CFR slots: called
				 * BEFORE the next conversion overwrites the image planes, it re-encodes the
				 * previous picture at the filler PTS for almost no cost.
				 * @param pts The constant-frame-rate slot to encode into.
				 * @param encodedBytes Running byte counter for the periodic statistics.
				 * @return True on success, false when the encoder rejected the frame.
				 */
				bool encodeImageAt (vpx_codec_pts_t pts, uint64_t & encodedBytes) noexcept;

				/** @brief Writes the 32-byte IVF file header. */
				[[nodiscard]] 
				bool writeIVFFileHeader () const noexcept;

				/** @brief Writes a 12-byte IVF frame header. */
				[[nodiscard]] 
				bool writeIVFFrameHeader (uint32_t frameSize, uint64_t pts) const noexcept;

				/** @brief Patches the frame count at byte offset 24 in the IVF header. */
				[[nodiscard]] 
				bool patchIVFFrameCount () const noexcept;

				/** @brief Flushes codec, patches IVF, closes file, destroys resources. */
				void finalize () noexcept;

				/** @brief Safety-net destructor: joins thread, cleans up if finalize() was not called. */
				~EncodingSession () noexcept;

				EncodingSession () = default;
				EncodingSession (const EncodingSession &) = delete;
				EncodingSession & operator= (const EncodingSession &) = delete;
				EncodingSession (EncodingSession &&) = delete;
				EncodingSession & operator= (EncodingSession &&) = delete;
			};

			/* Service dependencies. */
			PrimaryServices & m_primaryServices; ///< Primary services for settings and filesystem.
			Renderer & m_renderer; ///< Graphics renderer for swap-chain access.

			/* Capture state shared by the rendering thread (record, submit, retire) and the thread that starts
			 * and stops the rush. */
			mutable std::mutex m_captureAccess; ///< Guards the slots, the sessions and the stop.
			size_t m_recordedSlot{NoSlot}; ///< The slot recorded in the frame being built, until confirmSubmit().

			/* Async GPU readback resources. */
			std::shared_ptr< Vulkan::CommandPool > m_transferCommandPool; ///< Command pool for transfer queue operations.
			std::array< AsyncReadbackSlot, AsyncBufferCount > m_asyncSlots{}; ///< Async readback slots.
			uint32_t m_graphicsFamilyIndex{0}; ///< Graphics queue family index for ownership transfers.
			uint32_t m_transferFamilyIndex{0}; ///< Transfer queue family index for ownership transfers.

			/* Detachable encoding sessions. */
			std::unique_ptr< EncodingSession > m_currentSession; ///< Active encoding session (null when not recording).
			std::vector< std::unique_ptr< EncodingSession > > m_finishingSessions; ///< Sessions still encoding in background.

			/* Hardware (Vulkan Video H.265) path. */
			std::unique_ptr< VideoFrameConverter > m_frameConverter; ///< GPU BGRA->NV12 converter.
			std::unique_ptr< Vulkan::VideoEncoderH265 > m_hardwareEncoder; ///< Hardware H.265 encoder.
			std::array< HardwareSlot, HardwareSlotCount > m_hardwareSlots{}; ///< GPU snapshot slots.
			std::unique_ptr< HardwareSession > m_hardwareSession; ///< Active hardware session (null on the software path).

			/* Recording state. */
			std::atomic< bool > m_isRecording{false}; ///< True while frames are captured (cleared by stopRecording()).
			std::atomic< bool > m_sessionOpen{false}; ///< True from startRecording() until the session is closed (drain included).

			/* Timing and frame pacing. */
			uint32_t m_targetFramerate{30}; ///< Target recording framerate (default 30 FPS).
			uint32_t m_maxQueuedFrames{90}; ///< Grab buffer bound; captures are skipped above this depth (RAM guard).
			int m_adaptedCpuUsed{-1}; ///< Last adapted encoder speed; warm-starts the next session (-1 = none yet).
			std::chrono::steady_clock::time_point m_recordStartTime; ///< Wall-clock time when recording started (PTS origin).
			int64_t m_lastCapturedSlot{-1}; ///< The CFR slot of the last capture, -1 before the first one (pacing).

			/* Recording parameters (locked at start). */
			uint32_t m_recordWidth{0}; ///< Recording width in pixels (even, locked at start).
			uint32_t m_recordHeight{0}; ///< Recording height in pixels (even, locked at start).
			/* State flags. */
			bool m_stopPending{false}; ///< stopRecording() came while copies were in flight: the last one closes the session.
			bool m_extentMismatchTraced{false}; ///< The swap-chain no longer matches the recording size (traced once per rush).
			bool m_useTransferQueue{false}; ///< True when dedicated transfer queue is available and in use.
			bool m_showStatistics{false}; ///< True to log periodic encoding statistics.
			bool m_forceCPUEncoding{false}; ///< Force the software VP9 path on a hardware-capable device (A/B lever).
			
			/**
			 * @enum QualityPreset
			 * @brief Presets for video quality configuration.
			 */
			enum class QualityPreset : uint8_t
			{
				Low,
				Medium,
				High,
				Ultra
			};

			QualityPreset m_qualityPreset{QualityPreset::Medium}; ///< Current quality preset.

			/**
			 * @brief Converts a QualityPreset enum value to its string representation.
			 *
			 * @param preset The quality preset to convert.
			 * @return A C-string representation of the preset ("Low", "Medium", "High", "Ultra").
			 */
			static const char * qualityPresetToString (QualityPreset preset) noexcept;

			/* SIMD dispatch. */
			BGRAToI420Func m_bgraToI420{nullptr}; ///< Selected BGRA-to-I420 conversion function (scalar, SSSE3, or AVX2).
	};
}
