/*
 * src/Libs/ThreadPool.cpp
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
		for ( size_t index = 0; index < threadCount; ++index )
		{
			m_workers.emplace_back(&ThreadPool::worker, this);
		}

		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] " << threadCount << " thread spawned in the pool !" "\n";
		}
	}

	ThreadPool::~ThreadPool () noexcept
	{
		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] Cleaning the thread pool ..." "\n";
		}

		{
			std::unique_lock< std::mutex > scopeLock{m_queueAccess};

			m_stop = true;
		}

		/* NOTE: Wake every body to make them stop. */
		m_condition.notify_all();

		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] Stopped and wait for all threads to quit ..." "\n";
		}

		for ( auto & worker : m_workers )
		{
			if ( !worker.joinable() )
			{
				continue;
			}

			worker.join();
		}

		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] Thread pool terminated." "\n";
		}
	}

	bool
	ThreadPool::enqueue (std::function< void () > task)
	{
		{
			std::unique_lock< std::mutex > scopeLock{m_queueAccess};

			if ( m_stop )
			{
				std::cerr << "[ThreadPool-debug] Enqueue on a stopped thread pool !" "\n";

				return false;
			}

			m_tasks.emplace(std::move(task));
		}

		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] New task added." "\n";
		}

		m_condition.notify_one();

		return true;
	}

	void
	ThreadPool::wait ()
	{
		std::unique_lock< std::mutex > lock{m_queueAccess};

		if constexpr ( ThreadPoolDebugEnabled )
		{
			std::cout << "[ThreadPool-debug] Waiting for " << m_busyWorkers << " to finish (" << m_tasks.size() << " tasks left) ..." "\n";
		}

		/* NOTE: The wait condition is that the task file is empty
		 * and no worker is currently executing a task. */
		m_completion_condition.wait(lock, [this] {
			return m_tasks.empty() && m_busyWorkers == 0;
		});
	}

	void
	ThreadPool::worker ()
	{
		while ( true )
		{
			std::function< void () > task;

			{
				std::unique_lock< std::mutex > lock{m_queueAccess};

				m_condition.wait(lock, [this] {
					return m_stop || !m_tasks.empty();
				});

				/* NOTE: If the pool is stopped and there are no more tasks, the worker may stop. */
				if ( m_stop && m_tasks.empty() )
				{
					return;
				}

				task = std::move(m_tasks.front());

				m_tasks.pop();

				++m_busyWorkers;
			}

			if constexpr ( ThreadPoolDebugEnabled )
			{
				Time::Elapsed::PrintScopeRealTime stat{"[ThreadPool-debug] Task finished"};

				std::cout << "[ThreadPool-debug] Thread #" << std::this_thread::get_id() << " running ... (" << m_busyWorkers << " busy workers, " << m_tasks.size() << " tasks left)" "\n";

				task();

				std::cout << "[ThreadPool-debug] Thread #" << std::this_thread::get_id() << " finished. (" << m_busyWorkers << " busy workers, " << m_tasks.size() << " tasks left)" "\n";
			}
			else
			{
				task();
			}

			{
				std::unique_lock< std::mutex > lock{m_queueAccess};

				--m_busyWorkers;

				/* NOTE: If it was the last task in progress AND the queue is empty,
				 * we notify those who are waiting. */
				if ( m_busyWorkers == 0 && m_tasks.empty() )
				{
					m_completion_condition.notify_all();
				}
			}
		}
	}
}