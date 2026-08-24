/*
 * src/Vulkan/GPUProfiler.hpp
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
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "Vulkan/AbstractDeviceDependentObject.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class CommandBuffer;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief Per-pass GPU timing service based on Vulkan timestamp queries.
	 * @note One query pool per frame in flight: while frame N records its timestamps,
	 * the results of frame N - framesInFlight are harvested on the CPU right after the
	 * frame fence wait — the read is therefore always available and never stalls.
	 * Timestamps use the classic TOP_OF_PIPE (scope begin) / BOTTOM_OF_PIPE (scope end)
	 * pair, the only combination giving meaningful approximate timings on most GPUs
	 * (intermediate stages do not: passes overlap on the hardware).
	 * References:
	 *  - https://docs.vulkan.org/samples/latest/samples/api/timestamp_queries/README.html
	 *  - https://nikitablack.github.io/post/how_to_use_vulkan_timestamp_queries/
	 * @warning V1 scope: only the MAIN frame command buffer is instrumented. The shadow
	 * map and render-to-texture passes are submitted through separate command buffers
	 * BEFORE the main one; resetting the shared pool from the main command buffer would
	 * wipe their queries on the GPU timeline. Widening the coverage needs one pool range
	 * per submission — deferred until the need is proven.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 */
	class EMEN_API GPUProfiler final : public AbstractDeviceDependentObject
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanGPUProfiler"};

			/** @brief Maximum number of timing scopes recorded per frame. */
			static constexpr uint32_t MaxScopesPerFrame{64};

			/** @brief Maximum label length, terminator included. */
			static constexpr size_t LabelCapacity{48};

			/** @brief Blend weight of the newest sample in the rolling average (~60 frame window). */
			static constexpr float AverageAlpha{0.05F};

			/**
			 * @brief A harvested timing entry, ready for display.
			 */
			struct Timing
			{
				std::string label;
				float lastMS{0.0F};
				float averageMS{0.0F};
				float maximumMS{0.0F};
				uint64_t sampleCount{0};
				uint32_t depth{0};
			};

			/**
			 * @brief Constructs a GPU profiler.
			 * @param device A reference to a smart pointer of the device.
			 * @param frameCount The number of frames in flight (one query pool each).
			 */
			GPUProfiler (const std::shared_ptr< Device > & device, uint32_t frameCount) noexcept;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			GPUProfiler (const GPUProfiler & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			GPUProfiler (GPUProfiler && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return GPUProfiler &
			 */
			GPUProfiler & operator= (const GPUProfiler & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return GPUProfiler &
			 */
			GPUProfiler & operator= (GPUProfiler && copy) noexcept = delete;

			/**
			 * @brief Destructs the GPU profiler.
			 */
			~GPUProfiler () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool createOnHardware () noexcept override;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool destroyFromHardware () noexcept override;

			/**
			 * @brief Returns whether the device supports timestamps and the pools exist.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			usable () const noexcept
			{
				return m_usable;
			}

			/**
			 * @brief Reads back the timings of the frame slot's PREVIOUS submission.
			 * @warning Must be called after the frame slot's in-flight fence wait and
			 * before beginFrame() re-records the slot: the fence is what guarantees the
			 * query results availability without a stalling wait flag.
			 * @param frameSlot The frame-in-flight slot index.
			 * @return void
			 */
			void harvest (uint32_t frameSlot) noexcept;

			/**
			 * @brief Starts the frame: resets the slot's query pool and opens the root scope.
			 * @warning Must be recorded OUTSIDE a render pass, right after the command
			 * buffer recording begins.
			 * @param commandBuffer A reference to the main frame command buffer.
			 * @param frameSlot The frame-in-flight slot index.
			 * @return void
			 */
			void beginFrame (const CommandBuffer & commandBuffer, uint32_t frameSlot) noexcept;

			/**
			 * @brief Closes the root scope. Records the frame-end timestamp.
			 * @param commandBuffer A reference to the main frame command buffer.
			 * @return void
			 */
			void endFrame (const CommandBuffer & commandBuffer) noexcept;

			/**
			 * @brief Opens a named timing scope. Scopes nest; the depth is kept for display.
			 * @note Silently ignored when the per-frame scope budget is exhausted (a warning
			 * is traced once). Legal inside or outside a render pass.
			 * @param commandBuffer A reference to the main frame command buffer.
			 * @param label The scope label (static string preferred).
			 * @param subLabel An optional sub-label appended as "label/subLabel". Default nullptr.
			 * @return void
			 */
			void beginScope (const CommandBuffer & commandBuffer, const char * label, const char * subLabel = nullptr) noexcept;

			/**
			 * @brief Closes the innermost open scope.
			 * @param commandBuffer A reference to the main frame command buffer.
			 * @return void
			 */
			void endScope (const CommandBuffer & commandBuffer) noexcept;

			/**
			 * @brief Returns a display-ready copy of the accumulated timings.
			 * @note Thread-safe: this is the console-facing read.
			 * @return std::vector< Timing >
			 */
			[[nodiscard]]
			std::vector< Timing > snapshot () const noexcept;

			/**
			 * @brief Clears the accumulated statistics (averages, maxima).
			 * @return void
			 */
			void resetStatistics () noexcept;

			/**
			 * @brief RAII helper opening a scope for the current C++ block.
			 */
			class ScopedZone final
			{
				public:

					/**
					 * @brief Opens a profiling scope, or does nothing when the profiler is null.
					 * @param profiler A pointer to the profiler. nullptr disables the zone.
					 * @param commandBuffer A reference to the main frame command buffer.
					 * @param label The scope label.
					 * @param subLabel An optional sub-label. Default nullptr.
					 */
					ScopedZone (GPUProfiler * profiler, const CommandBuffer & commandBuffer, const char * label, const char * subLabel = nullptr) noexcept
						: m_profiler{profiler},
						m_commandBuffer{&commandBuffer}
					{
						if ( m_profiler != nullptr )
						{
							m_profiler->beginScope(*m_commandBuffer, label, subLabel);
						}
					}

					/** @brief No copy, no move: strictly block-scoped. */
					ScopedZone (const ScopedZone & copy) noexcept = delete;
					ScopedZone (ScopedZone && copy) noexcept = delete;
					ScopedZone & operator= (const ScopedZone & copy) noexcept = delete;
					ScopedZone & operator= (ScopedZone && copy) noexcept = delete;

					/**
					 * @brief Closes the profiling scope.
					 */
					~ScopedZone ()
					{
						if ( m_profiler != nullptr )
						{
							m_profiler->endScope(*m_commandBuffer);
						}
					}

				private:

					GPUProfiler * m_profiler;
					const CommandBuffer * m_commandBuffer;
			};

		private:

			/**
			 * @brief A scope recorded during the current frame (CPU-side bookkeeping).
			 */
			struct ScopeRecord
			{
				char label[LabelCapacity]{};
				uint32_t beginQuery{0};
				uint32_t endQuery{0};
				uint32_t depth{0};
			};

			/**
			 * @brief Per frame-in-flight slot data.
			 */
			struct FrameData
			{
				VkQueryPool pool{VK_NULL_HANDLE};
				std::vector< ScopeRecord > records;
				uint32_t queryCount{0};
				bool submitted{false};
			};

			/**
			 * @brief Rolling statistics of one label (display order = first-seen order).
			 */
			struct Accumulated
			{
				float lastMS{0.0F};
				float averageMS{0.0F};
				float maximumMS{0.0F};
				uint64_t sampleCount{0};
				uint32_t depth{0};
			};

			/**
			 * @brief Folds one harvested duration into the accumulator.
			 * @param label The scope label.
			 * @param depth The scope nesting depth.
			 * @param milliseconds The measured duration.
			 * @return void
			 */
			void accumulate (const char * label, uint32_t depth, float milliseconds) noexcept;

			std::vector< FrameData > m_frames;
			std::vector< uint32_t > m_openScopes;
			std::vector< std::pair< std::string, Accumulated > > m_accumulated;
			mutable std::mutex m_accumulatedAccess;
			double m_timestampPeriodNS{0.0};
			uint64_t m_timestampMask{~0ULL};
			uint32_t m_currentSlot{0};
			bool m_scopeOverflowTraced{false};
			bool m_usable{false};
	};
}
