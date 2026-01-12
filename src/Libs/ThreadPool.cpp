/*
 * src/Libs/ThreadPool.cpp
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

#include "ThreadPool.hpp"

/* Configurations. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>

/* Local inclusions. */
#include "Libs/Time/Elapsed/PrintScopeRealTime.hpp"

namespace EmEn::Libs
{
	ThreadPool::ThreadPool (size_t threadCount)
	{
		/* Ensure at least one worker. */
		if ( threadCount == 0 )
		{
			threadCount = 1;
		}

		m_workers.reserve(threadCount);

		for ( size_t index = 0; index < threadCount; ++index )
		{
			m_workers.emplace_back(&ThreadPool::worker, this);
		}

		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] " << threadCount << " threads spawned in the pool." "\n";
		}
	}

	ThreadPool::~ThreadPool () noexcept
	{
		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] Cleaning the thread pool..." "\n";
		}

		/* Signal all workers to stop. */
		{
			std::lock_guard< std::mutex > lock{m_mutex};
			m_stop.store(true, std::memory_order_release);
		}

		/* Wake all workers so they can see the stop flag. */
		m_condition.notify_all();

		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] Stopped and waiting for all threads to quit..." "\n";
		}

		for ( auto & worker : m_workers )
		{
			if ( worker.joinable() )
			{
				worker.join();
			}
		}

		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] Thread pool terminated." "\n";
		}
	}

	bool
	ThreadPool::enqueueTask (Task && task)
	{
		{
			std::lock_guard< std::mutex > lock{m_mutex};

			if ( m_stop.load(std::memory_order_acquire) )
			{
				if constexpr ( ThreadPoolDebugEnabled )
				{
					std::cerr << "[ThreadPool-debug] Enqueue on a stopped thread pool!" "\n";
				}

				return false;
			}

			m_tasks.emplace_back(std::move(task));
			m_pendingTasks.fetch_add(1, std::memory_order_release);
		}

		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] New task added to queue." "\n";
		}

		m_condition.notify_one();

		return true;
	}

	void
	ThreadPool::wait ()
	{
		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout <<
				"[ThreadPool-debug] Waiting for " << m_busyWorkers.load(std::memory_order_relaxed) << " workers to finish "
				"(" << m_pendingTasks.load(std::memory_order_relaxed) << " tasks left)..." "\n";
		}

		std::unique_lock< std::mutex > lock{m_mutex};

		m_completionCondition.wait(lock, [this] {
			return m_pendingTasks.load(std::memory_order_acquire) == 0 && m_busyWorkers.load(std::memory_order_acquire) == 0;
		});

		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] All tasks completed." "\n";
		}
	}

	void
	ThreadPool::worker ()
	{
		while ( true )
		{
			Task task;

			{
				std::unique_lock< std::mutex > lock{m_mutex};

				/* Wait until there's work or stop signal. */
				m_condition.wait(lock, [this] {
					return m_stop.load(std::memory_order_acquire) || !m_tasks.empty();
				});

				/* Check stop condition - but finish remaining tasks first. */
				if ( m_stop.load(std::memory_order_acquire) && m_tasks.empty() )
				{
					return;
				}

				/* Get a task from the queue. */
				if ( !m_tasks.empty() )
				{
					task = std::move(m_tasks.front());
					m_tasks.pop_front();
					m_pendingTasks.fetch_sub(1, std::memory_order_release);
					m_busyWorkers.fetch_add(1, std::memory_order_release);
				}
				else
				{
					/* Spurious wakeup or stop without tasks. */
					continue;
				}
			}

			/* Execute task outside any locks. */
			if constexpr ( ThreadPoolDebugEnabled )
			{
				std::cout <<
					"[ThreadPool-debug] Worker running task... "
					"(" << m_busyWorkers.load(std::memory_order_relaxed) << " busy, " << m_pendingTasks.load(std::memory_order_relaxed) << " pending)" "\n";

				{
					Time::Elapsed::PrintScopeRealTime stat{"[ThreadPool-debug] Task finished"};

					task();
				}

				std::cout << "[ThreadPool-debug] Worker finished task." "\n";
			}
			else
			{
				task();
			}

			/* Decrement busy count and signal completion. */
			{
				std::lock_guard< std::mutex > lock{m_mutex};

				m_busyWorkers.fetch_sub(1, std::memory_order_release);

				m_completionCondition.notify_all();
			}
		}
	}
}
