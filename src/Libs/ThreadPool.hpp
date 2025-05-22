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
			~ThreadPool ();

			/**
			 * @bried Adds a new task to the pool.
			 * @param task A reference to a function.
			 * @return bool
			 */
			bool enqueue (std::function< void () > task);

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
			bool m_stop{false};
	};
}