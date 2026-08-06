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
#include "vpx/vp8cx.h"
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
	 * 1. GPU async readback (double-buffered) captures BGRA frames from the swap-chain at
	 *    the target FPS (wall-clock pacing)
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
			 * Waits for pending GPU readbacks, signals the encoding thread to stop, flushes
			 * the VP9 encoder, patches the IVF frame count, and releases all resources.
			 *
			 * @return True if recording was active and stopped successfully, false if not recording.
			 * @post All GPU resources released, encoding thread joined, output file closed.
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
			 * @brief Returns the video file extension of the active encoding path.
			 * @note "h265" (hardware Vulkan Video, muxed to MP4 by the assemble script)
			 * when the device supports it, "ivf" (software VP9, muxed to WebM) otherwise.
			 * @return const char *
			 */
			[[nodiscard]]
			const char * videoFileExtension () const noexcept;

			/**
			 * @brief Returns whether the hardware H.265 path will be used.
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
			 * @brief Checks if enough time has elapsed to capture the next frame.
			 *
			 * Uses frame duration based on target FPS to determine if a new frame should
			 * be captured. This pacing mechanism ensures the recording matches the target
			 * framerate regardless of game render rate.
			 *
			 * @return True if a frame should be captured now, false otherwise.
			 * @note Returns false immediately if not recording.
			 */
			[[nodiscard]]
			bool shouldCaptureFrame () const noexcept;

			/**
			 * @brief Returns the recommended audio bitrate in kbps based on the current quality preset.
			 *
			 * @return Audio bitrate in kbps (128, 192, 256, or 320).
			 */
			[[nodiscard]]
			unsigned int recommendedAudioBitrate () const noexcept;

			/**
			 * @brief Captures the current swap-chain framebuffer and submits GPU copy operation.
			 *
			 * Harvests any completed async readbacks, finds a free async slot (or drops the frame
			 * if both slots are busy), tags the frame with wall-clock PTS, and submits a GPU
			 * image-to-buffer copy command. The copy executes asynchronously on the GPU.
			 *
			 * @pre shouldCaptureFrame() returned true.
			 * @note If no async slots are available, the frame is silently dropped. If the
			 * encoder queue has reached its bound, the capture is skipped and counted
			 * (backpressure) — wall-clock PTS keeps the video timeline correct.
			 */
			void captureAndSubmitFrame () noexcept;

		private:

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
			 * @brief Creates async GPU readback resources (command pool, buffers, fences).
			 *
			 * Allocates a transient command pool and initializes AsyncBufferCount readback slots,
			 * each with a command buffer, fence (signaled), and host-visible staging buffer sized
			 * for a full BGRA framebuffer.
			 *
			 * @return True if all resources created successfully, false otherwise.
			 * @post m_asyncCommandPool and m_asyncSlots are fully initialized.
			 */
			bool createAsyncResources () noexcept;

			/**
			 * @brief Destroys async GPU readback resources and waits for pending operations.
			 *
			 * Unconditionally waits on all fences (safe for signaled fences), then releases
			 * command buffers, fences, staging buffers, and the command pool.
			 *
			 * @post All async resources are released.
			 */
			void destroyAsyncResources () noexcept;

			/**
			 * @brief Submits a GPU copy command to transfer swap-chain image to staging buffer.
			 *
			 * Records a command buffer that transitions the swap-chain image to TRANSFER_SRC_OPTIMAL,
			 * copies it to the staging buffer, then transitions back to PRESENT_SRC_KHR. Submits
			 * the command buffer with a fence and marks the slot as pending.
			 *
			 * @param slotIndex Index of the async readback slot to use (0 or 1).
			 * @return True if GPU copy submitted successfully, false otherwise.
			 * @pre Slot is not pending.
			 * @post Slot fence is reset and marked pending.
			 */
			bool submitGPUCopy (size_t slotIndex) noexcept;

			/**
			 * @brief Submits a two-step GPU copy using the dedicated transfer queue.
			 *
			 * Step 1 (graphics queue): copies swap-chain image to a device-local intermediate
			 * buffer with layout transitions, signals a semaphore.
			 * Step 2 (transfer queue): DMA copies device-local buffer to host-visible staging
			 * buffer on dedicated transfer hardware, signals fence.
			 *
			 * @param slotIndex Index of the async readback slot to use (0 or 1).
			 * @return True if both submissions succeeded, false otherwise.
			 * @pre m_useTransferQueue is true and slot is not pending.
			 */
			bool submitTransferQueueCopy (size_t slotIndex) noexcept;

			/**
			 * @brief Harvests a completed async readback and pushes data to the frame queue.
			 *
			 * Checks the fence status non-blockingly. If ready, maps the staging buffer, copies
			 * the BGRA pixel data into a new frame queue entry, and signals the encoding thread
			 * via condition variable.
			 *
			 * @param slotIndex Index of the async readback slot to harvest (0 or 1).
			 * @return True if readback harvested successfully, false if not pending or not ready.
			 * @post If successful, frame is in the queue and slot is no longer pending.
			 */
			bool harvestReadback (size_t slotIndex) noexcept;

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
			 * @brief Captures the current swap-chain image into a free hardware slot
			 * (GPU image copy — the frame never reaches system memory).
			 */
			void captureHardwareFrame () noexcept;

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
				std::unique_ptr< Vulkan::CommandBuffer > commandBuffer; ///< Copy command buffer.
				std::unique_ptr< Vulkan::Sync::Fence > fence; ///< Signaled when the snapshot copy completes.
				int64_t cfrSlot{0}; ///< Constant-frame-rate slot of this capture.
				bool pending{false}; ///< True while queued for encoding.
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
				std::unique_ptr< Vulkan::CommandBuffer > commandBuffer; ///< Command buffer for image-to-buffer copy.
				std::unique_ptr< Vulkan::Sync::Fence > fence; ///< Fence signaled when GPU copy completes.
				std::unique_ptr< Vulkan::Buffer > stagingBuffer; ///< Host-visible staging buffer for readback.
				uint8_t * mappedPtr{nullptr}; ///< Persistently mapped pointer to staging buffer (valid from create to destroy).
				/* Transfer queue path (only used when m_useTransferQueue is true). */
				std::unique_ptr< Vulkan::CommandBuffer > transferCommandBuffer; ///< Transfer queue: buffer-to-buffer DMA copy.
				std::unique_ptr< Vulkan::Sync::Semaphore > transferSemaphore; ///< Signaled after graphics copy, waited by transfer DMA.
				std::unique_ptr< Vulkan::Buffer > deviceLocalBuffer; ///< Device-local intermediate buffer for two-step readback.
				std::chrono::steady_clock::time_point captureTime; ///< Wall-clock time when GPU copy was submitted.
				bool pending{false}; ///< True if GPU copy is in flight.
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

			/* Async GPU readback resources. */
			std::shared_ptr< Vulkan::CommandPool > m_asyncCommandPool; ///< Transient command pool for async readback.
			std::shared_ptr< Vulkan::CommandPool > m_transferCommandPool; ///< Command pool for transfer queue operations.
			std::array< AsyncReadbackSlot, AsyncBufferCount > m_asyncSlots{}; ///< Double-buffered async readback slots.
			size_t m_currentAsyncSlot{0}; ///< Currently selected async slot index (round-robin).
			uint32_t m_graphicsFamilyIndex{0}; ///< Graphics queue family index for ownership transfers.
			uint32_t m_transferFamilyIndex{0}; ///< Transfer queue family index for ownership transfers.

			/* Detachable encoding sessions. */
			std::unique_ptr< EncodingSession > m_currentSession; ///< Active encoding session (null when not recording).
			std::vector< std::unique_ptr< EncodingSession > > m_finishingSessions; ///< Sessions still encoding in background.

			/* Hardware (Vulkan Video H.265) path. */
			std::unique_ptr< VideoFrameConverter > m_frameConverter; ///< GPU BGRA->NV12 converter.
			std::unique_ptr< Vulkan::VideoEncoderH265 > m_hardwareEncoder; ///< Hardware H.265 encoder.
			std::shared_ptr< Vulkan::CommandPool > m_hardwareCommandPool; ///< Graphics pool for the snapshot copies.
			std::array< HardwareSlot, HardwareSlotCount > m_hardwareSlots{}; ///< GPU snapshot slots.
			std::unique_ptr< HardwareSession > m_hardwareSession; ///< Active hardware session (null on the software path).

			/* Recording state (main thread only). */
			std::atomic< bool > m_isRecording{false}; ///< True when capture is active on the main thread.

			/* Timing and frame pacing. */
			uint32_t m_targetFramerate{30}; ///< Target recording framerate (default 30 FPS).
			uint32_t m_maxQueuedFrames{90}; ///< Grab buffer bound; captures are skipped above this depth (RAM guard).
			int m_adaptedCpuUsed{-1}; ///< Last adapted encoder speed; warm-starts the next session (-1 = none yet).
			std::chrono::steady_clock::time_point m_lastCaptureTime; ///< Last frame capture timestamp for pacing.
			std::chrono::steady_clock::time_point m_recordStartTime; ///< Wall-clock time when recording started (PTS origin).
			std::chrono::nanoseconds m_frameDuration{0}; ///< Duration between frames based on target FPS.

			/* Recording parameters (locked at start). */
			uint32_t m_recordWidth{0}; ///< Recording width in pixels (even, locked at start).
			uint32_t m_recordHeight{0}; ///< Recording height in pixels (even, locked at start).
			/* State flags. */
			bool m_useTransferQueue{false}; ///< True when dedicated transfer queue is available and in use.
			bool m_showStatistics{false}; ///< True to log periodic encoding statistics.
			
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
