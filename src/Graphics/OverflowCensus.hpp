/*
 * src/Graphics/OverflowCensus.hpp
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
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

/* Third-party inclusions. */
#include <vulkan/vulkan.h>

/* Local inclusions for usages. */
#include "FrameDiagnostics.hpp"
#include "IndirectPostProcessEffect.hpp"
#include "StaticVector.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Buffer;
	class CommandBuffer;
	class ComputePipeline;
	class DescriptorPool;
	class DescriptorSet;
	class DescriptorSetLayout;
	class Device;
	class GPUProfiler;
	class Image;
	class ImageView;
	class PipelineLayout;
	class Sampler;
	class TextureInterface;
}

namespace EmEn::Graphics
{
	class IrradianceProbeVolume;
	class Renderer;
}

namespace EmEn::Graphics
{
	/**
	 * @brief The OVERFLOW CENSUS: counts, per frame, the scene-radiance texels an RGBA16F target has
	 * turned into NaN, infinity, or the largest finite binary16 (scene-colour pre-exposure, step B1a,
	 * 2026-09-26).
	 * @details A physical luminance above 65 504 nits cannot be stored in a half float: it becomes +Inf,
	 * and the first filter that multiplies it by 0 turns it into NaN. The census is the instrument that
	 * says how often, where, and by how much, before anything is changed to prevent it.
	 *
	 * One compute dispatch per CHANNEL (one image) classifies every texel from the IEEE BITS of its
	 * worst RGB channel — integer comparisons only, no isnan()/isinf() and no float compare that a
	 * fast-math compiler (Metal) may fold away — and reduces the counts in shared memory, then with one
	 * atomic per workgroup into the channel's own region of a per-frame counter buffer. The channels:
	 *  - `SceneColour`: the grabbed scene colour the chain starts from;
	 *  - `ToneMapInput`: the chain colour the tone mapper receives (or the chain output when none ran);
	 *  - the RAW TRACES the effects declare (IndirectPostProcessEffect::radianceTargets(): `RTGI_Trace`,
	 *    `RTR_Trace`), which the chain colour never shows before a denoiser averaged them;
	 *  - `ProbeIrradiance`: the interior texels of the irradiance probe atlas;
	 *  - `SelfTest`: a 16×16 image of raw half bit patterns, on request only (runSelfTest()).
	 *
	 * ⚠️ `ToneMapInput` = 0 does NOT mean the scene colour is clean: the TAA and SSR guards scrub the
	 * non-finite values out of the chain before the tone mapper. The gap between `SceneColour` and
	 * `ToneMapInput` IS those guards at work.
	 * ⚠️ `ceiling` is not `Inf`: a GPU may round an overflowing conversion to the largest finite value
	 * instead of +Inf, and both are counted apart for that reason.
	 * ⚠️ `SceneColour` is the grab-pass COPY, made with a linear blit: at a 1:1 scale a driver may still
	 * weight a neighbour by 0, and 0 × Inf = NaN, so it may count a few more (NaN) texels around an Inf
	 * of the scene target. It is still what the chain itself reads.
	 *
	 * The whole census is ONE batch per frame, recorded by the post-processor right before the
	 * ToneMapping occupant executes (or after the last effect when no tone mapper runs): every counted
	 * image is final there. It is HARVESTED in Renderer::beginFrame() once the slot's fence has passed,
	 * whether or not the chain runs that frame. A serial header written by the batch detects a slot that
	 * was recorded but never executed.
	 *
	 * @note Always created (at the first PostProcessor::configure()), DISARMED by default: disarmed, it
	 * records nothing and costs one branch per chain call. A count read on a machine means something
	 * only once runSelfTest() answered PASS there.
	 * @note Not exported: the renderer, the post-processor and their console bindings are its only users.
	 */
	class OverflowCensus final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"OverflowCensus"};

			/** @brief The most channels one frame counts: the five of today plus spare room. */
			static constexpr uint32_t MaxChannels{OverflowCensusMaxChannels};

			/** @brief The side of the self-test image, in texels. */
			static constexpr uint32_t SelfTestExtent{16};

			/* The self-test tuple a correct census measures on the self-test image (see OverflowCensus.cpp). */
			static constexpr uint32_t SelfTestExpectedTested{SelfTestExtent * SelfTestExtent};
			static constexpr uint32_t SelfTestExpectedNaN{21};
			static constexpr uint32_t SelfTestExpectedInf{12};
			static constexpr uint32_t SelfTestExpectedCeiling{8};
			/** @brief 65 472.0 as binary32 bits: the largest finite binary16 below the ceiling. */
			static constexpr uint32_t SelfTestExpectedPeakBits{0x477FC000U};

			/**
			 * @brief Constructs the census (no GPU resource).
			 * @note Out of line with the destructor: the members hold smart pointers to forward-declared
			 * Vulkan types, and an inline constructor makes MSVC instantiate their deleters (C2027).
			 */
			OverflowCensus () noexcept;

			/**
			 * @brief Destructs the census.
			 */
			~OverflowCensus () noexcept;

			OverflowCensus (const OverflowCensus & copy) noexcept = delete;
			OverflowCensus (OverflowCensus && copy) noexcept = delete;
			OverflowCensus & operator= (const OverflowCensus & copy) noexcept = delete;
			OverflowCensus & operator= (OverflowCensus && copy) noexcept = delete;

			/**
			 * @brief Creates every GPU resource: the per-frame counter and readback buffers, the pipelines, the self-test image.
			 * @note Needs the frames in flight: call it once the renderer's frame scopes exist (the first
			 * PostProcessor::configure()). A failure is traced once and leaves the census unavailable.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool create (Renderer & renderer) noexcept;

			/**
			 * @brief Releases every GPU resource (while the device still exists).
			 * @return void
			 */
			void destroy () noexcept;

			/* ---- RENDER THREAD ---- */

			/**
			 * @brief Opens the census for a rendered frame: latches the armed state and the self-test request.
			 * @note Idempotent per serial: both halves of a cut frame call it, and the radiance targets the
			 * pre-translucency half noted survive until the closing batch.
			 * @param frameSerial The rendered-frame serial (Renderer::renderedFrameSerial()).
			 * @return void
			 */
			void prepareFrame (uint64_t frameSerial) noexcept;

			/**
			 * @brief Returns whether this frame records a batch (armed, or a self-test is pending).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			frameArmed () const noexcept
			{
				return m_frameArmed || m_frameSelfTestGeneration != 0;
			}

			/**
			 * @brief Notes the radiance targets of an effect the chain is about to record.
			 * @param targets The effect's radiance targets.
			 * @return void
			 */
			void noteRadianceTargets (std::span< const IndirectPostProcessEffect::RadianceTarget > targets) noexcept;

			/**
			 * @brief Records the frame's batch: the counter reset, one dispatch per channel, the readback copy.
			 * @note Outside any render pass, after every counted image is final (before the tone mapper).
			 * Nothing when the frame is not armed.
			 * @param commandBuffer The frame's command buffer.
			 * @param frameIndex The frame in flight index.
			 * @param sceneColour The grabbed scene colour the chain started from.
			 * @param toneMapInput The chain colour at this point.
			 * @param toneMapped Whether @a toneMapInput is the tone mapper's input (false: the chain output, no tone mapper runs).
			 * @param volume The irradiance probe volume, or nullptr.
			 * @param profiler The GPU profiler, or nullptr.
			 * @return void
			 */
			void recordBatch (const Vulkan::CommandBuffer & commandBuffer, uint32_t frameIndex, const Vulkan::TextureInterface & sceneColour, const Vulkan::TextureInterface & toneMapInput, bool toneMapped, const IrradianceProbeVolume * volume, Vulkan::GPUProfiler * profiler) noexcept;

			/**
			 * @brief Reads back the batch a frame slot recorded, once its fence has passed.
			 * @param frameIndex The frame in flight index whose fence was just waited.
			 * @return void
			 */
			void harvest (uint32_t frameIndex) noexcept;

			/* ---- ANY THREAD ---- */

			/**
			 * @brief Arms or disarms the census, from the next rendered frame.
			 * @param state The state.
			 * @return void
			 */
			void
			setArmed (bool state) noexcept
			{
				m_armed.store(state, std::memory_order_release);
			}

			/**
			 * @brief Returns whether the census is armed (the request; it lands on the next rendered frame).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			armed () const noexcept
			{
				return m_armed.load(std::memory_order_acquire);
			}

			/**
			 * @brief Returns whether the census exists and its GPU resources were created.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			available () const noexcept
			{
				return m_available.load(std::memory_order_acquire);
			}

			/**
			 * @brief Opens a new statistics window.
			 * @note Frames already recorded when this is called are left out of it, even when they are
			 * harvested after the call.
			 * @return uint64_t The rendered-frame serial the new window starts after.
			 */
			uint64_t resetWindow () noexcept;

			/**
			 * @brief Returns a copy of the last counted frame, the window and the last self-test.
			 * @return OverflowCensusSnapshot
			 */
			[[nodiscard]]
			OverflowCensusSnapshot snapshot () const noexcept;

			/**
			 * @brief Counts the self-test image in the next frame that records a batch, and waits for the result.
			 * @warning MAIN THREAD, blocking: never call it from the render thread, which has to produce the
			 * frame. Returns `ran = false` when no frame counted it within @a timeout (no scene, no HDR chain).
			 * @param timeout The longest wait.
			 * @return OverflowCensusSelfTest
			 */
			[[nodiscard]]
			OverflowCensusSelfTest runSelfTest (std::chrono::milliseconds timeout) noexcept;

		private:

			/** @brief The census shader's push constants (both variants). */
			struct PushConstants
			{
				uint32_t width;
				uint32_t height;
				/** @brief Texels per atlas tile side, border included; 0 for a plain image. */
				uint32_t tile;
				/** @brief Border texels on each side of an atlas tile. */
				uint32_t border;
			};

			static_assert(sizeof(PushConstants) == 16, "The census push constants must stay 16 bytes (GLSL mirror).");

			/** @brief What the CPU remembers of a recorded channel, to decode its region. */
			struct ChannelRecord
			{
				OverflowCensusChannelName name{};
				uint32_t expectedTexels{0};
			};

			/** @brief One radiance target noted by the executor this frame. */
			struct NotedTarget
			{
				OverflowCensusChannelName name{};
				const Vulkan::TextureInterface * texture{nullptr};
			};

			/** @brief The resources of one frame in flight. */
			struct Slot
			{
				/** @brief Device-local: region 0 is the serial header, region c + 1 is channel c. */
				std::unique_ptr< Vulkan::Buffer > counters;
				/** @brief Host-visible copy of the counters, persistently mapped. */
				std::unique_ptr< Vulkan::Buffer > readback;
				const uint8_t * mappedPointer{nullptr};
				/** @brief One set per channel, whose storage binding covers ONLY that channel's region. */
				std::array< std::unique_ptr< Vulkan::DescriptorSet >, MaxChannels > sets;
				std::array< ChannelRecord, MaxChannels > channels{};
				uint64_t expectedSerial{0};
				/** @brief The self-test request this batch serves, 0 for none. */
				uint64_t selfTestGeneration{0};
				uint32_t channelCount{0};
				/** @brief The channel index of the self-test image, MaxChannels for none. */
				uint32_t selfTestChannel{MaxChannels};
				bool toneMapped{false};
				/** @brief The batch counts the frame (the census was armed), not only the self-test. */
				bool countsFrame{false};
				bool pending{false};
			};

			/**
			 * @brief Creates the descriptor set layout, the pipeline layout, the two pipelines and the sampler.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createPipelines (Renderer & renderer) noexcept;

			/**
			 * @brief Creates the descriptor pool and, per frame in flight, the counter buffer, the readback buffer and the channel sets.
			 * @param frameCount The frames in flight.
			 * @return bool
			 */
			[[nodiscard]]
			bool createSlots (uint32_t frameCount) noexcept;

			/**
			 * @brief Creates and uploads the self-test image.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createSelfTestImage (Renderer & renderer) noexcept;

			/**
			 * @brief Folds a counted frame into the statistics window. The statistics lock must be held.
			 * @param report The frame's report.
			 * @return void
			 */
			void accumulateWindow (const OverflowCensusReport & report) noexcept;

			std::shared_ptr< Vulkan::Device > m_device;
			std::vector< Slot > m_slots;
			std::shared_ptr< Vulkan::DescriptorSetLayout > m_descriptorSetLayout;
			std::shared_ptr< Vulkan::DescriptorPool > m_descriptorPool;
			std::shared_ptr< Vulkan::PipelineLayout > m_pipelineLayout;
			std::unique_ptr< Vulkan::ComputePipeline > m_pipeline2D;
			std::unique_ptr< Vulkan::ComputePipeline > m_pipelineArray;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::shared_ptr< Vulkan::Image > m_selfTestImage;
			std::shared_ptr< Vulkan::ImageView > m_selfTestView;
			/** @brief Bytes per region of a counter buffer (the storage offset alignment). */
			VkDeviceSize m_stride{0};
			/* ---- Render thread only (the booleans are at the end). ---- */
			Base::StaticVector< NotedTarget, MaxChannels > m_notedTargets;
			uint64_t m_preparedSerial{0};
			/** @brief The self-test request this frame serves, 0 for none (latched by prepareFrame()). */
			uint64_t m_frameSelfTestGeneration{0};
			/* ---- Shared, under m_statisticsAccess. ---- */
			mutable std::mutex m_statisticsAccess;
			std::condition_variable m_selfTestCompletion;
			OverflowCensusReport m_latest{};
			OverflowCensusWindow m_window{};
			OverflowCensusSelfTest m_selfTest{};
			/* ---- Atomics. ---- */
			/** @brief The latest serial prepareFrame() saw: where a reset window starts. */
			std::atomic< uint64_t > m_lastPreparedSerial{0};
			/** @brief The self-test generation the console asked for, 0 for none. */
			std::atomic< uint64_t > m_selfTestRequested{0};
			/** @brief The last self-test generation handed out. */
			std::atomic< uint64_t > m_selfTestRequestCounter{0};
			/** @brief The latest self-test generation whose result was published (written under the lock). */
			std::atomic< uint64_t > m_selfTestPublished{0};
			std::atomic< bool > m_armed{false};
			std::atomic< bool > m_available{false};
			/* ---- Render thread only. ---- */
			/** @brief The census counts this frame (the armed state, latched by prepareFrame()). */
			bool m_frameArmed{false};
			/** @brief A channel dropped for lack of room was traced (once per session). */
			bool m_droppedChannelReported{false};
			/** @brief A channel that could not be counted (format, descriptor) was traced (once per session). */
			bool m_skippedChannelReported{false};
			/** @brief A coverage defect (tested != expected) was traced; re-armed by a clean frame. */
			bool m_coverageDefectReported{false};
	};
}
