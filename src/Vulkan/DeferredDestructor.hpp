/*
 * src/Vulkan/DeferredDestructor.hpp
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
#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

/* Local inclusions for usages. */
#include "Mutex.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief Central deferred-destruction queue for Vulkan-backed objects.
	 * @note This is THE engine contract for destroying GPU-visible objects at runtime.
	 * With N frames in flight, a command buffer submitted at frame F may still be
	 * executing while the CPU prepares frame F+1: destroying an object as soon as the
	 * CPU stops using it can pull it from under the GPU (validation errors, device loss).
	 * Instead, retire the object (or a destruction action) here; it is destroyed once
	 * the renderer has ticked framesInFlight times, i.e. once every command buffer that
	 * could reference it has completed execution and been re-recorded.
	 * Retiring is thread-safe; tick() and flush() belong to the render thread.
	 */
	class EMEN_API DeferredDestructor final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"DeferredDestructor"};

			/**
			 * @brief Constructs the deferred destructor.
			 * @note The default retirement delay is conservative (5 ticks); the renderer
			 * lowers it to the actual frames-in-flight count during initialization.
			 */
			DeferredDestructor () noexcept = default;

			/**
			 * @brief Deleted copy constructor (owns a mutex).
			 */
			DeferredDestructor (const DeferredDestructor & copy) noexcept = delete;

			/**
			 * @brief Deleted move constructor (owns a mutex).
			 */
			DeferredDestructor (DeferredDestructor && copy) noexcept = delete;

			/**
			 * @brief Deleted copy assignment (owns a mutex).
			 * @return DeferredDestructor &
			 */
			DeferredDestructor & operator= (const DeferredDestructor & copy) noexcept = delete;

			/**
			 * @brief Deleted move assignment (owns a mutex).
			 * @return DeferredDestructor &
			 */
			DeferredDestructor & operator= (DeferredDestructor && copy) noexcept = delete;

			/**
			 * @brief Destructs the deferred destructor, destroying every pending entry.
			 */
			~DeferredDestructor ()
			{
				this->flush();
			}

			/**
			 * @brief Sets the retirement delay from the renderer frames-in-flight count.
			 * @note Also (re)opens the queue (close()): the renderer calls it at its initialization.
			 * @param framesInFlight The number of frames in flight.
			 */
			void
			setFramesInFlight (uint32_t framesInFlight) noexcept
			{
				const Base::MutexLock lock{m_mutex};

				m_delayTicks = std::max(framesInFlight, 1U);
				m_closed = false;
			}

			/**
			 * @brief Destroys every pending entry (flush()), then CLOSES the queue: from then on, a retirement is
			 * destroyed (or its action run) at once, in the caller's thread.
			 * @note The renderer closes it at its termination, its device idle and no frame in flight any more. The
			 * resources are unloaded AFTER that (Core::terminate(): unloadUnusedResources() after each secondary
			 * service), and the renderer itself is destroyed after the device: a retirement that waited for the
			 * renderer's destructor would destroy its Vulkan object with no device left.
			 * @warning The caller must guarantee the device is idle.
			 */
			void
			close () noexcept
			{
				this->flush();

				const Base::MutexLock lock{m_mutex};

				m_closed = true;
			}

			/**
			 * @brief Retires an owning pointer; the object is destroyed after the delay.
			 * @param object The last (or a keep-alive) reference to the object.
			 */
			void
			retireObject (std::shared_ptr< void > object) noexcept
			{
				if ( object == nullptr )
				{
					return;
				}

				{
					const Base::MutexLock lock{m_mutex};

					if ( !m_closed )
					{
						m_entries.emplace_back(Entry{
							.retiredAtTick = m_currentTick,
							.serial = ++m_lastSerial,
							.object = std::move(object),
							.action = nullptr,
							.isReady = nullptr
						});

						return;
					}
				}

				/* NOTE: Closed (the renderer terminated, its device idle): destroyed now, outside the lock. */
				object.reset();
			}

			/**
			 * @brief Retires an owning unique pointer; the object is destroyed after the delay.
			 * @tparam object_t The concrete object type.
			 * @param object The unique pointer to retire.
			 */
			template< typename object_t >
			void
			retireObject (std::unique_ptr< object_t > object) noexcept
			{
				this->retireObject(std::shared_ptr< void >{std::move(object)});
			}

			/**
			 * @brief Retires a destruction action, executed after the delay.
			 * @note Use this when the object needs an explicit tear-down call
			 * (e.g. RenderTarget::Abstract::destroyRenderTarget()) instead of
			 * plain destruction. The callable must be copyable: capture the
			 * object through a std::shared_ptr.
			 * @param action The destruction action.
			 */
			void
			retireAction (std::function< void () > action) noexcept
			{
				if ( action == nullptr )
				{
					return;
				}

				{
					const Base::MutexLock lock{m_mutex};

					if ( !m_closed )
					{
						m_entries.emplace_back(Entry{
							.retiredAtTick = m_currentTick,
							.serial = ++m_lastSerial,
							.object = nullptr,
							.action = std::move(action),
							.isReady = nullptr
						});

						return;
					}
				}

				/* NOTE: Closed (the renderer terminated, its device idle): run now, outside the lock. */
				action();
			}

			/**
			 * @brief Retires a destruction action executed after the delay AND once a condition holds.
			 * @note For an object still written by work the frame fences do not cover — an upload on the
			 * transfer path, whose completion is a queue timeline value (Vulkan::PendingSubmissions). The
			 * condition is polled at each tick once the delay elapsed; it must be cheap and thread-safe.
			 * flush() runs the action without asking: its caller guarantees an idle device.
			 * @param isReady The condition, true once nothing on the GPU uses the object any more.
			 * @param action The destruction action.
			 */
			void
			retireActionWhen (std::function< bool () > isReady, std::function< void () > action) noexcept
			{
				if ( action == nullptr )
				{
					return;
				}

				{
					const Base::MutexLock lock{m_mutex};

					if ( !m_closed )
					{
						m_entries.emplace_back(Entry{
							.retiredAtTick = m_currentTick,
							.serial = ++m_lastSerial,
							.object = nullptr,
							.action = std::move(action),
							.isReady = std::move(isReady)
						});

						return;
					}
				}

				/* NOTE: Closed: the device is idle, the condition holds by construction (flush() does not ask either). */
				action();
			}

			/**
			 * @brief Advances the frame tick and destroys every entry old enough.
			 * @note Call once per frame from the render thread, right after the
			 * frame fence wait: at that point, entries retired framesInFlight
			 * ticks ago are no longer referenced by any pending command buffer.
			 */
			void
			tick () noexcept
			{
				std::vector< Entry > expired;

				{
					const Base::MutexLock lock{m_mutex};

					++m_currentTick;

					/* NOTE: Entries are in retirement order, so the scan stops at the first one still inside the
					 * delay. A conditional entry whose condition does not hold yet stays, without holding back the
					 * entries behind it. */
					for ( auto entryIt = m_entries.begin(); entryIt != m_entries.end(); )
					{
						if ( m_currentTick - entryIt->retiredAtTick < m_delayTicks )
						{
							break;
						}

						if ( entryIt->isReady != nullptr && !entryIt->isReady() )
						{
							++entryIt;

							continue;
						}

						expired.emplace_back(std::move(*entryIt));

						entryIt = m_entries.erase(entryIt);
					}
				}

				/* NOTE: Destruction happens outside the lock so that a Vulkan object
				 * destructor retiring further objects cannot deadlock. */
				for ( auto & entry : expired )
				{
					if ( entry.action != nullptr )
					{
						entry.action();
					}
				}
			}

			/**
			 * @brief Returns a mark of the retirements made so far, for releaseRetiredUpTo().
			 * @note Take it BEFORE waiting for the device to be idle: what is retired after it may belong to work
			 * submitted during or after the wait.
			 * @return uint64_t
			 */
			[[nodiscard]]
			uint64_t
			retirementMark () noexcept
			{
				const Base::MutexLock lock{m_mutex};

				return m_lastSerial;
			}

			/**
			 * @brief Destroys every entry retired up to a mark, whatever its tick, once its condition holds.
			 * @note For a render thread that waited for the device to be idle AFTER taking the mark: nothing the GPU
			 * executes can reference those objects any more, and the render thread records nothing meanwhile. The frame
			 * delay of tick() cannot cover that case: a swap-chain recreated at every frame (a window resize storm) drops
			 * every frame, tick() never runs, and the retired generations piled up until the video memory ran out
			 * (measured 2026-11-19: 1 776 entries, 5.5 GiB after 48 s). A conditional entry (retireActionWhen()) still
			 * waits for its condition, which an idle device makes true in practice.
			 * @param mark The mark returned by retirementMark() before the wait.
			 */
			void
			releaseRetiredUpTo (uint64_t mark) noexcept
			{
				std::vector< Entry > released;

				{
					const Base::MutexLock lock{m_mutex};

					/* NOTE: Entries are in retirement order: the scan stops at the first one retired after the mark. */
					for ( auto entryIt = m_entries.begin(); entryIt != m_entries.end(); )
					{
						if ( entryIt->serial > mark )
						{
							break;
						}

						if ( entryIt->isReady != nullptr && !entryIt->isReady() )
						{
							++entryIt;

							continue;
						}

						released.emplace_back(std::move(*entryIt));

						entryIt = m_entries.erase(entryIt);
					}
				}

				/* NOTE: Outside the lock, as in tick(). */
				for ( auto & entry : released )
				{
					if ( entry.action != nullptr )
					{
						entry.action();
					}
				}
			}

			/**
			 * @brief Destroys every pending entry immediately.
			 * @warning The caller must guarantee the device is idle (shutdown, resize).
			 */
			void
			flush () noexcept
			{
				std::deque< Entry > entries;

				{
					const Base::MutexLock lock{m_mutex};

					entries.swap(m_entries);
				}

				for ( auto & entry : entries )
				{
					if ( entry.action != nullptr )
					{
						entry.action();
					}
				}
			}

		private:

			/** @brief A retired object or destruction action, stamped with its retirement tick. */
			struct Entry
			{
				uint64_t retiredAtTick{0};
				/* NOTE: Increasing in retirement order (retirementMark()). */
				uint64_t serial{0};
				std::shared_ptr< void > object;
				std::function< void () > action;
				/* NOTE: Null for a plain retirement (the delay alone). */
				std::function< bool () > isReady;
			};

			Base::Mutex m_mutex;
			std::deque< Entry > m_entries EMEN_GUARDED_BY(m_mutex);
			uint64_t m_currentTick EMEN_GUARDED_BY(m_mutex){0};
			uint64_t m_lastSerial EMEN_GUARDED_BY(m_mutex){0};
			uint32_t m_delayTicks EMEN_GUARDED_BY(m_mutex){5};
			/* NOTE: close(): a retirement is destroyed at once (the renderer terminated). */
			bool m_closed EMEN_GUARDED_BY(m_mutex){false};
	};
}
