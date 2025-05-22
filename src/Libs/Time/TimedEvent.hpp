/*
 * src/Libs/Time/TimedEvent.hpp
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
#include <chrono>
#include <functional>
#include <ratio>
#include <type_traits>
#include <thread>
#include <mutex>
#include <condition_variable>

/* Local Inclusions. */
#include "Types.hpp"

namespace EmEn::Libs::Time
{
	/**
	 * @brief The timed event class.
	 * @tparam rep_t The time thick precision. Default unsigned int.
	 * @tparam period_t The unity of time expressed with std::ratio<>. Default std::milli.
	 */
	template< typename rep_t = uint32_t, typename period_t = std::milli >
	requires (std::is_arithmetic_v< rep_t >)
	class TimedEvent final
	{
		public:

			/**
			 * @brief Constructs a default timed event.
			 */
			TimedEvent () noexcept = default;

			/**
			 * @brief Constructs a timed event.
			 * @param function A function object to fire after each interval.
			 * @param granularity The interval duration respecting the period_t.
			 * @param once Declare to stop after one interval elapsed. Default false.
			 */
			TimedEvent (std::function< bool (TimerID) > function, rep_t granularity, bool once = false) noexcept
				: m_granularity(granularity), m_function(std::move(function))
			{
				m_onlyOnce = once;
			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			TimedEvent (const TimedEvent & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			TimedEvent (TimedEvent && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			TimedEvent & operator= (const TimedEvent & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			TimedEvent & operator= (TimedEvent && copy) noexcept = default;

			/**
			 * @brief Destructs the timed event.
			 * @note Waits for thread to quit properly.
			 */
			~TimedEvent ()
			{
				/* NOTE: This will causes the while() loop to stop in the thread. */
				{
					const std::lock_guard< std::mutex > lock{m_mutex};

					m_isProcessCreated = false;
					//m_isProcessActive = false;
					//m_isProcessPaused = false;
				}

				m_condition.notify_one();

				/* If thread is launched, wait for the end. */
				if ( m_thread.joinable() )
				{
					m_thread.join();
				}
			}

			/**
			 * @brief Modifies the interval duration.
			 * @param granularity The interval duration respecting the period_t template argument.
			 * @return void
			 */
			void
			setGranularity (rep_t granularity) noexcept
			{
				const std::lock_guard< std::mutex > lock{m_mutex};

				m_granularity = std::chrono::duration< rep_t, period_t >{granularity};
				m_previousTime = std::chrono::steady_clock::now();
			}

			/**
			 * @brief Resets the timer.
			 * @return void
			 */
			void
			reset () noexcept
			{
				const std::lock_guard< std::mutex > lock{m_mutex};

				m_previousTime = std::chrono::steady_clock::now();
			}

			/**
			 * @brief Returns the interval duration respecting the period_t (time unit) template argument.
			 * @return rep_t
			 */
			[[nodiscard]]
			rep_t
			granularity () const noexcept
			{
				return m_granularity.count();
			}

			/**
			 * @brief Sets the timer ID to identify on callback.
			 * @param timerID A unsigned integer.
			 * @return void
			 */
			void
			setTimerID (TimerID timerID) noexcept
			{
				m_timerId = timerID;
			}

			/**
			 * @brief Returns the timer ID associated.
			 * @note This can be zero.
			 * @return TimerID
			 */
			[[nodiscard]]
			TimerID
			timerID () const noexcept
			{
				return m_timerId;
			}

			/**
			 * @brief Returns the elapsed time respecting the period_t (time unit) template argument.
			 * @return rep_t
			 */
			[[nodiscard]]
			rep_t
			elapsed () noexcept
			{
				const auto top = std::chrono::steady_clock::now();

				return std::chrono::duration_cast< std::chrono::duration< rep_t, period_t > >(top - m_previousTime).count();
			}

			/**
			 * @brief Returns whether the interval expressed by the granularity parameter is elapsed and returns the duration overflow.
			 * @return time A reference to a value to express if interval is elapsed, the time overflow, if not the remaining time (all using period_t, the time unit).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isIntervalElapsed (rep_t & time) noexcept
			{
				const auto top = std::chrono::steady_clock::now();

				const auto delta = std::chrono::duration_cast< std::chrono::duration< rep_t, period_t > >(top - m_previousTime).count();
				const auto intervalDuration = m_granularity.count();

				if ( delta < intervalDuration )
				{
					/* Report the remaining time. */
					time = intervalDuration - delta;

					return false;
				}

				/* Report the time overflow */
				time = delta - intervalDuration;

				m_previousTime = top - std::chrono::duration< rep_t, period_t >{time};

				return true;
			}

			/**
			 * @brief Starts the timer of the event.
			 * @return void
			 */
			void
			start () noexcept
			{
				const std::lock_guard< std::mutex > lock{m_mutex};

				/* If the process is already active, we do nothing. */
				if ( m_isProcessActive )
				{
					return;
				}

				m_isProcessActive = true;
				m_isProcessPaused = false;

				m_previousTime = std::chrono::steady_clock::now();

				/* Creates the process thread only once. */
				if ( m_isProcessCreated )
				{
					/* NOTE: Wake up the thread. */
					m_condition.notify_one();

					return;
				}

				m_isProcessCreated = true;

				m_thread = std::thread{&TimedEvent::process, this};
			}

			/**
			 * @brief Stops the timer of the event.
			 * @return void
			 */
			void
			stop () noexcept
			{
				const std::lock_guard< std::mutex > lock{m_mutex};

				if ( !m_isProcessCreated || !m_isProcessActive )
				{
					return;
				}

				m_isProcessActive = false;
			}

			/**
			 * @brief Pauses the timer of the event.
			 * @return void
			 */
			void
			pause () noexcept
			{
				const std::lock_guard< std::mutex > lock{m_mutex};

				if ( !m_isProcessCreated || !m_isProcessActive )
				{
					return;
				}

				m_isProcessActive = false;
				m_isProcessPaused = true;
			}

			/**
			 * @brief Resumes the timer of the event.
			 * @return void
			 */
			void
			resume () noexcept
			{
				{
					const std::lock_guard< std::mutex > lock{m_mutex};
					
					if ( !m_isProcessPaused )
					{
						return;
					}

					m_isProcessActive = true;
					m_isProcessPaused = false;
					
					m_previousTime = std::chrono::steady_clock::now();
				}
				
				/* NOTE: Wake up the thread. */
				m_condition.notify_one();
			}

			/**
			 * @brief Returns whether the event is running.
			 * @return bool.
			 */
			[[nodiscard]]
			bool
			isStarted () const noexcept
			{
				return m_isProcessActive;
			}

			/**
			 * @brief Returns whether the event is on pause.
			 * @return bool.
			 */
			[[nodiscard]]
			bool
			isPaused () const noexcept
			{
				return m_isProcessPaused;
			}

			/**
			 * @brief Returns whether the event is fired only once.
			 * @return bool.
			 */
			[[nodiscard]]
			bool
			once () const noexcept
			{
				return m_onlyOnce;
			}

		private:

			/**
			 * @brief Thread task.
			 * @return void
			 */
			void
			process ()
			{
				std::unique_lock< std::mutex > lock{m_mutex};

				while ( m_isProcessCreated )
				{
					/* NOTE: The thread will sleep until "m_isProcessActive" is true or the timer is destroyed.
					 * The mutex is automatically released during sleep and reacquired upon waking. */
					m_condition.wait(lock, [this] {
						return m_isProcessActive || !m_isProcessCreated;
					});

					/* NOTE: If we wake up because the timer is destroyed, we exit the loop. */
					if ( !m_isProcessCreated )
					{
						break;
					}

					/* NOTE: Get a copy to be thread safe. */
					const auto granularityCopy = m_granularity;
					const auto previousTimeCopy = m_previousTime;

					/* NOTE: At this point, we are certain that m_isProcessActive is true and the mutex is locked. */
					lock.unlock();

					rep_t time;
					const auto top = std::chrono::steady_clock::now();
					const auto delta = std::chrono::duration_cast<std::chrono::duration<rep_t, period_t>>(top - previousTimeCopy).count();
					const auto intervalDuration = granularityCopy.count();

					bool intervalElapsed;

					if ( delta < intervalDuration )
					{
						time = intervalDuration - delta;

						intervalElapsed = false;
					}
					else
					{
						time = delta - intervalDuration;

						intervalElapsed = true;
					}

					if ( intervalElapsed )
					{
						if ( m_function(m_timerId) || m_onlyOnce )
						{
							this->stop();
						}
						{
							const std::lock_guard< std::mutex > updateLock(m_mutex);

							m_previousTime = top - std::chrono::duration<rep_t, period_t>{time};
						}
					}
					else
					{
						std::this_thread::sleep_for(std::chrono::duration<rep_t, period_t>{time});
					}

					lock.lock();
				}
			}

			std::chrono::duration< rep_t, period_t > m_granularity{std::chrono::seconds{1}};
			std::chrono::time_point< std::chrono::steady_clock > m_previousTime{std::chrono::steady_clock::now()};
			std::function< bool (TimerID) > m_function;
			std::thread m_thread;
			mutable std::mutex m_mutex;
			std::condition_variable m_condition;
			TimerID m_timerId{0};
			std::atomic_bool m_isProcessCreated{false};
			std::atomic_bool m_isProcessActive{false};
			std::atomic_bool m_isProcessPaused{false};
			std::atomic_bool m_onlyOnce{false};
	};
}
