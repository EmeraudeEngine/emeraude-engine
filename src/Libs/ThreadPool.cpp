#include "ThreadPool.hpp"

/* STL inclusions. */
#include <iostream>

namespace EmEn::Libs
{
	ThreadPool::ThreadPool (size_t threadCount)
	{
		for ( size_t index = 0; index < threadCount; ++index )
		{
			m_workers.emplace_back(&ThreadPool::worker, this);
		}
	}

	ThreadPool::~ThreadPool ()
	{
		{
			std::unique_lock< std::mutex > scopeLock{m_queueAccess};

			m_stop = true;
		}

		/* NOTE: Wake every body to make them stop. */
		m_condition.notify_all();

		for ( auto & worker : m_workers )
		{
			worker.join();
		}
	}

	bool
	ThreadPool::enqueue (std::function< void () > task)
	{
		{
			std::unique_lock< std::mutex > scopeLock{m_queueAccess};

			if ( m_stop )
			{
				std::cerr << "Enqueue on a stopped thread pool !" "\n";

				return false;
			}

			m_tasks.emplace(std::move(task));
		}

		m_condition.notify_one();

		return true;
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

				if ( m_stop && m_tasks.empty() )
				{
					return;
				}

				task = std::move(m_tasks.front());

				m_tasks.pop();
			}

			task();
		}
	}
}