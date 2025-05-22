/*
 * src/Libs/ParallelizableTrait.hpp
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
#include <mutex>
#include <stack>
#include <thread>
#include <vector>

namespace EmEn::Libs
{
	/**
	 * @brief This trait is intended to be inherited by an object that needs threaded tasks.
	 * @tparam data_t The type of data to parallelize.
	 * @deprecated Use Libs::ThreadPool instead!
	 */
	template< typename data_t >
	class ParallelizableTrait
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ParallelizableTrait (const ParallelizableTrait & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ParallelizableTrait (ParallelizableTrait && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return ParallelizableProcess &
			 */
			ParallelizableTrait & operator= (const ParallelizableTrait & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return ParallelizableProcess &
			 */
			ParallelizableTrait & operator= (ParallelizableTrait && copy) noexcept = default;

			/**
			 * @brief Destructs the parallelizable process.
			 */
			virtual ~ParallelizableTrait () = default;

			/**
			 * @brief Creates the task.
			 * @param data A reference to data.
			 * @return void
			 */
			void
			createTask (const data_t & data) noexcept
			{
				/* Adding works to the queue. */
				{
					const std::lock_guard< std::mutex > lockGuard{m_mutex};

					m_pendingJobs.emplace(data);
				}

				/* Try to execute a worker if room there.
				 * Current worker ID to pass to process function. */
				for ( auto & worker : m_workers )
				{
					if ( worker.joinable() )
					{
						continue;
					}

					worker = std::thread{[this] () {
						data_t data{};

						while ( this->getNextJob(data) )
						{
							if ( !this->task(data) )
							{
								break;
							}
						}
					}};

					break;
				}
			}

			/**
			 * @brief Waits for all jobs to finish.
			 * @return void
			 */
			void
			waitForAllJobsDone () noexcept
			{
				for ( auto & worker : m_workers )
				{
					if ( !worker.joinable() )
					{
						continue;
					}

					worker.join();
				}
			}

		protected:

			/**
			 * @brief Constructs a parallelizable process.
			 * @param workersCount The number of workers. Default max thread.
			 */
			explicit
			ParallelizableTrait (size_t workersCount = std::thread::hardware_concurrency()) noexcept
				: m_workers(workersCount)
			{

			}

			/**
			 * @brief This method takes care to pull out from the queue the next job.
			 * @return bool
			 */
			bool
			getNextJob (data_t & data) noexcept
			{
				const std::lock_guard< std::mutex > lockGuard{m_mutex};

				if ( !m_pendingJobs.empty() )
				{
					/* Copy data outside this function. */
					data = m_pendingJobs.top();

					/* Removes it from the queue. */
					m_pendingJobs.pop();

					return true;
				}

				return false;
			}

			/**
			 * @brief The process to build in child class.
			 * @tparam data_t The data type.
			 * @param data A reference to the data.
			 * @return void
			 */
			virtual bool task (const data_t & data) noexcept = 0;

		private:

			std::vector< std::thread > m_workers;
			std::stack< data_t > m_pendingJobs{};
			std::mutex m_mutex;
	};
}
