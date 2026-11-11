/*
 * src/Graphics/Material/DirtyFrameRegions.hpp
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
#include <algorithm>
#include <atomic>
#include <cstdint>

namespace EmEn::Graphics::Material
{
	/**
	 * @brief The frame-in-flight regions of a material's uniform block that still hold an older content, and the single
	 * registration of the material for the renderer's flush.
	 * @note A material block has one region per frame in flight (SharedUniformBuffer::frameCount()), each read through its
	 * own descriptor set. A dynamic property change raises every region; the flush of frame f (render thread, after the
	 * fence of f: Renderer::flushMaterialVideoMemoryUpdates()) rewrites region f only — the other regions may still be read
	 * by the frames in flight — and lowers it. The material stays registered until every region is lowered: a change
	 * reaches every region within frameCount() frames, and no frame ever reads a region while it is written.
	 * @note Lock-free and thread-safe: raised from any thread (logic, console, loading), lowered on the render thread.
	 * The protocol keeps exactly one registration while a region is raised: a raise during a flush is never lost (the
	 * registration is released BEFORE the region is lowered and claimed again after, both sides through one exchange).
	 */
	class DirtyFrameRegions final
	{
		public:

			/** @brief The most regions tracked, one bit each (MaxFramesInFlight is far below). */
			static constexpr uint32_t MaxRegionCount{32};

			/**
			 * @brief Arms the tracking with the block's region count. Every region is clean.
			 * @note Called by the material's creation while it holds its properties lock, BEFORE it writes every region:
			 * a change made after that write raises (armed), a change made before it is in the write.
			 * @param regionCount The number of regions, clamped to [1, MaxRegionCount].
			 */
			void
			arm (uint32_t regionCount) noexcept
			{
				m_mask.store(0, std::memory_order_relaxed);
				m_regionCount.store(std::clamp(regionCount, 1U, MaxRegionCount), std::memory_order_release);
			}

			/**
			 * @brief Disarms the tracking (the GPU side is destroyed): raiseAll() does nothing until the next arm().
			 */
			void
			disarm () noexcept
			{
				m_regionCount.store(0, std::memory_order_release);
				m_mask.store(0, std::memory_order_relaxed);
			}

			/**
			 * @brief Returns the number of regions tracked, 0 while disarmed.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			regionCount () const noexcept
			{
				return m_regionCount.load(std::memory_order_acquire);
			}

			/**
			 * @brief Raises every region, after a property change.
			 * @note Call it AFTER the change is written (the flush reads the properties after lowering).
			 * @return bool True when the caller must register the material for a flush: armed and not registered yet.
			 */
			[[nodiscard]]
			bool
			raiseAll () noexcept
			{
				const auto regionCount = m_regionCount.load(std::memory_order_acquire);

				/* Not armed: the creation's own write will carry the change. */
				if ( regionCount == 0 )
				{
					return false;
				}

				m_mask.fetch_or(allBits(regionCount), std::memory_order_acq_rel);

				return !m_registered.exchange(true, std::memory_order_acq_rel);
			}

			/**
			 * @brief Starts the flush of one region: releases the registration, then lowers the region.
			 * @pre Called once per flush entry, on the render thread, BEFORE the properties are read.
			 * @param region The region of the frame being recorded.
			 * @return bool True when the region was raised: the caller rewrites it.
			 */
			[[nodiscard]]
			bool
			lower (uint32_t region) noexcept
			{
				m_registered.store(false, std::memory_order_release);

				if ( region >= MaxRegionCount )
				{
					return false;
				}

				const auto bit = 1U << region;

				return (m_mask.fetch_and(~bit, std::memory_order_acq_rel) & bit) != 0;
			}

			/**
			 * @brief Ends the flush of one region: claims the registration again when regions remain raised.
			 * @return bool True when the caller must register the material for the next flush.
			 */
			[[nodiscard]]
			bool
			needsAnotherFlush () noexcept
			{
				if ( m_mask.load(std::memory_order_acquire) == 0 )
				{
					return false;
				}

				return !m_registered.exchange(true, std::memory_order_acq_rel);
			}

		private:

			/**
			 * @brief Returns the mask of the first regionCount regions.
			 * @param regionCount The number of regions, in [1, MaxRegionCount].
			 * @return uint32_t
			 */
			[[nodiscard]]
			static constexpr
			uint32_t
			allBits (uint32_t regionCount) noexcept
			{
				return regionCount >= MaxRegionCount ? ~0U : (1U << regionCount) - 1U;
			}

			std::atomic< uint32_t > m_mask{0};
			std::atomic< uint32_t > m_regionCount{0};
			std::atomic< bool > m_registered{false};
	};
}
