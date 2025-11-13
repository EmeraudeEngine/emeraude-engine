/*
 * src/Testing/test_ParallelizableProcess.cpp
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

#include <gtest/gtest.h>

/* STL inclusions. */
#include <iostream>
#include <cstddef>

/* Local inclusions. */
//#include "Libs/ParallelizableTrait.hpp"
#include "Libs/Randomizer.hpp"
#include "Libs/Time/Elapsed/PrintScopeRealTime.hpp"
#include "Libs/Utility.hpp"

using namespace EmEn::Libs;
using namespace EmEn::Libs::Time::Elapsed;

/*class Fibonnizzer final : public ParallelizableTrait< size_t >
{
	public:

		explicit
		Fibonnizzer (size_t threadCount, std::vector< size_t > * outsideResults = nullptr) noexcept
			: ParallelizableTrait< size_t >(threadCount), m_outsideResults(outsideResults)
		{

		}

		Fibonnizzer (const Fibonnizzer & copy) noexcept = delete;

		Fibonnizzer (Fibonnizzer && copy) noexcept = delete;

		Fibonnizzer & operator= (const Fibonnizzer & copy) noexcept = delete;

		Fibonnizzer & operator= (Fibonnizzer && copy) noexcept = delete;

		~Fibonnizzer () override
		{
			this->waitForAllJobsDone();
		}

		void
		enableOutput () noexcept
		{
			m_enableOutput = true;
		}

		void
		launch (size_t jobCount) noexcept
		{
			if ( m_enableOutput )
			{
				const std::lock_guard< std::mutex > lock{m_consoleOutputLock};

				std::cout << "[" << std::this_thread::get_id() << "] Launching the task ..." "\n";
			}

			Randomizer< size_t > randomizer;

			for ( size_t job = 0; job < jobCount; job++ )
			{
				this->createTask(randomizer.value(1, 47));

				if ( m_enableOutput )
				{
					const std::lock_guard< std::mutex > lock{m_consoleOutputLock};

					std::cout << "[" << std::this_thread::get_id() << "] Job #" << job << " created !" "\n";
				}
			}
		}

		void
		printResults () noexcept
		{
			const std::lock_guard< std::mutex > lockA{m_resultsLock};
			const std::lock_guard< std::mutex > lockB{m_consoleOutputLock};

			std::cout << "[" << std::this_thread::get_id() << "] Results :" "\n";

			for ( auto result : m_results )
			{
				std::cout << result << '\n';
			}
			
			std::cout << '\n';
		}

		[[nodiscard]]
		const std::vector< size_t > &
		results ()
		{
			return m_results;
		}

	private:

		[[nodiscard]]
		static
		size_t
		fibonacci (size_t valueX) noexcept
		{
			if ( valueX == 1 || valueX == 0 )
			{
				return valueX;
			}

			return fibonacci(valueX - 1) + fibonacci(valueX - 2);
		}

		[[nodiscard]]
		bool
		task (const size_t & data) noexcept override
		{
			if ( m_enableOutput )
			{
				const std::lock_guard< std::mutex > lock{m_consoleOutputLock};

				std::cout << "[" << std::this_thread::get_id() << "] Computing fibonnacci(" << data << ") ..." << '\n';
			}

			size_t iteration = 0;
			size_t result = 0;

			while ( iteration < data )
			{
				result = fibonacci(iteration++);
			}

			if ( m_enableOutput )
			{
				const std::lock_guard< std::mutex > lock{m_consoleOutputLock};

				std::cout << "[" << std::this_thread::get_id() << "] fibonnacci(" << data << ") = " << result << " !" << '\n';
			}

			{
				const std::lock_guard< std::mutex > lock{m_resultsLock};

				if ( m_outsideResults != nullptr )
				{
					m_outsideResults->emplace_back(result);
				}
				else
				{
					m_results.emplace_back(result);
				}
			}

			return true;
		}

		std::mutex m_resultsLock;
		std::mutex m_consoleOutputLock;
		std::vector< size_t > m_results;
		std::vector< size_t > * m_outsideResults{nullptr};
		bool m_enableOutput{false};
};*/

TEST(ParallelizableProcess, DestructorWaitForJobDone)
{
	/*constexpr size_t jobCount{48};

	PrintScopeRealTime stat{"Fibonnizzer::launch(48)"};

	std::vector< size_t > m_results{};

	Fibonnizzer fibonnizzer{std::thread::hardware_concurrency(), &m_results};
	fibonnizzer.launch(jobCount);

	ASSERT_EQ(m_results.size(), jobCount);*/
}

TEST(ParallelizableProcess, FibonnacciWorkload)
{
	/*constexpr size_t jobCountA{48};
	constexpr size_t jobCountB{96};

	Fibonnizzer fibonnizzer{std::thread::hardware_concurrency()};

	{
		PrintScopeRealTime stat{"Fibonnizzer::launch(48)"};

		fibonnizzer.launch(jobCountA);
		fibonnizzer.waitForAllJobsDone();

		ASSERT_EQ(fibonnizzer.results().size(), jobCountA);
	}

	{
		PrintScopeRealTime stat{"Fibonnizzer::launch(96)"};

		fibonnizzer.launch(jobCountB);
		fibonnizzer.waitForAllJobsDone();

		ASSERT_EQ(fibonnizzer.results().size(), jobCountA + jobCountB);
	}*/
}
