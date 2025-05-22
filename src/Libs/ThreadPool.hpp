/*
 * src/Libs/ThreadPool.hpp
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
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace EmEn::Libs
{
	/**
	 * @brief A thread pool class to manage multiple work in a fixed threads number.
	 */
	class ThreadPool final
	{
		public:

			/**
			 * @brief Constructs a thread pool.
			 * @param threadCount The desired number of workers.
			 */
			explicit ThreadPool (size_t threadCount);

			/**
			 * @brief Destructs the thread pool.
			 */
			~ThreadPool () noexcept;

			/**
			 * @brief Adds a new task to the pool.
			 * @param task A reference to a function.
			 * @return bool
			 */
			bool enqueue (std::function< void () > task);

			/**
			 * @brief Waits until all tasks in the queue and in progress are finished.
			 * @return void
			 */
			void wait ();

		private:

			/**
			 * @brief Thread work function.
			 * @return void
			 */
			void worker ();

			std::vector< std::thread > m_workers;
			std::queue< std::function< void () > > m_tasks;
			std::mutex m_queueAccess;
			std::condition_variable m_condition;
			std::condition_variable m_completion_condition;
			size_t m_busyWorkers{0};
			bool m_stop{false};
	};
}
