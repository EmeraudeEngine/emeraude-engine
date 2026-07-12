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

/* STL inclusions. */
#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

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
			 * @param framesInFlight The number of frames in flight.
			 * @return void
			 */
			void
			setFramesInFlight (uint32_t framesInFlight) noexcept
			{
				const std::scoped_lock lock{m_mutex};

				m_delayTicks = std::max(framesInFlight, 1U);
			}

			/**
			 * @brief Retires an owning pointer; the object is destroyed after the delay.
			 * @param object The last (or a keep-alive) reference to the object.
			 * @return void
			 */
			void
			retireObject (std::shared_ptr< void > object) noexcept
			{
				if ( object == nullptr )
				{
					return;
				}

				const std::scoped_lock lock{m_mutex};

				m_entries.emplace_back(Entry{
					.retiredAtTick = m_currentTick,
					.object = std::move(object),
					.action = nullptr
				});
			}

			/**
			 * @brief Retires an owning unique pointer; the object is destroyed after the delay.
			 * @tparam object_t The concrete object type.
			 * @param object The unique pointer to retire.
			 * @return void
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
			 * @return void
			 */
			void
			retireAction (std::function< void () > action) noexcept
			{
				if ( action == nullptr )
				{
					return;
				}

				const std::scoped_lock lock{m_mutex};

				m_entries.emplace_back(Entry{
					.retiredAtTick = m_currentTick,
					.object = nullptr,
					.action = std::move(action)
				});
			}

			/**
			 * @brief Advances the frame tick and destroys every entry old enough.
			 * @note Call once per frame from the render thread, right after the
			 * frame fence wait: at that point, entries retired framesInFlight
			 * ticks ago are no longer referenced by any pending command buffer.
			 * @return void
			 */
			void
			tick () noexcept
			{
				std::vector< Entry > expired;

				{
					const std::scoped_lock lock{m_mutex};

					++m_currentTick;

					while ( !m_entries.empty() && m_currentTick - m_entries.front().retiredAtTick >= m_delayTicks )
					{
						expired.emplace_back(std::move(m_entries.front()));

						m_entries.pop_front();
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
			 * @brief Destroys every pending entry immediately.
			 * @warning The caller must guarantee the device is idle (shutdown, resize).
			 * @return void
			 */
			void
			flush () noexcept
			{
				std::deque< Entry > entries;

				{
					const std::scoped_lock lock{m_mutex};

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
				std::shared_ptr< void > object;
				std::function< void () > action;
			};

			std::mutex m_mutex;
			std::deque< Entry > m_entries;
			uint64_t m_currentTick{0};
			uint32_t m_delayTicks{5};
	};
}
