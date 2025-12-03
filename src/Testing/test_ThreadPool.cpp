/*
 * src/Testing/test_ThreadPool.cpp
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
#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

/* Local inclusions. */
#include "Libs/Math/Space2D/AARectangle.hpp"
#include "Libs/Time/Elapsed/PrintScopeRealTime.hpp"
#include "Libs/PixelFactory/Pixmap.hpp"
#include "Libs/PixelFactory/Processor.hpp"
#include "Libs/ThreadPool.hpp"

using namespace EmEn::Libs;
using namespace EmEn::Libs::Math;
using namespace EmEn::Libs::PixelFactory;
using namespace EmEn::Libs::Time::Elapsed;

/* ============================================================================
 * Task class tests
 * ============================================================================ */

TEST(ThreadPoolTask, DefaultConstructor)
{
	ThreadPool::Task task;

	EXPECT_TRUE(task.empty());
	EXPECT_FALSE(static_cast< bool >(task));
}

TEST(ThreadPoolTask, ConstructFromSmallLambda)
{
	bool executed = false;
	ThreadPool::Task task{[&executed] { executed = true; }};

	EXPECT_FALSE(task.empty());
	EXPECT_TRUE(static_cast< bool >(task));
	EXPECT_TRUE(task.isSmall());

	task();

	EXPECT_TRUE(executed);
}

TEST(ThreadPoolTask, ConstructFromLargeLambda)
{
	/* Create a lambda with a capture larger than SmallBufferSize (48 bytes). */
	std::array< int, 20 > largeCapture{};  /* 80 bytes on most platforms. */
	largeCapture.fill(42);
	bool executed = false;

	ThreadPool::Task task{[largeCapture, &executed] {
		executed = true;
		/* Use largeCapture to prevent optimization. */
		volatile int sum = 0;
		for ( const auto & val : largeCapture )
		{
			sum += val;
		}
	}};

	EXPECT_FALSE(task.empty());
	EXPECT_TRUE(static_cast< bool >(task));
	EXPECT_FALSE(task.isSmall());

	task();

	EXPECT_TRUE(executed);
}

TEST(ThreadPoolTask, MoveConstructor)
{
	int value = 0;
	ThreadPool::Task task1{[&value] { value = 42; }};

	EXPECT_FALSE(task1.empty());

	ThreadPool::Task task2{std::move(task1)};

	EXPECT_TRUE(task1.empty());
	EXPECT_FALSE(task2.empty());

	task2();

	EXPECT_EQ(value, 42);
}

TEST(ThreadPoolTask, MoveAssignment)
{
	int value = 0;
	ThreadPool::Task task1{[&value] { value = 42; }};
	ThreadPool::Task task2;

	EXPECT_FALSE(task1.empty());
	EXPECT_TRUE(task2.empty());

	task2 = std::move(task1);

	EXPECT_TRUE(task1.empty());
	EXPECT_FALSE(task2.empty());

	task2();

	EXPECT_EQ(value, 42);
}

TEST(ThreadPoolTask, MoveAssignmentSelfAssignment)
{
	int value = 0;
	ThreadPool::Task task{[&value] { value = 42; }};

	/* Self-assignment should be safe. */
	task = std::move(task);

	EXPECT_FALSE(task.empty());

	task();

	EXPECT_EQ(value, 42);
}

TEST(ThreadPoolTask, MoveOnlyCapture)
{
	auto ptr = std::make_unique< int >(42);
	int result = 0;

	ThreadPool::Task task{[p = std::move(ptr), &result] {
		result = *p;
	}};

	EXPECT_FALSE(task.empty());

	task();

	EXPECT_EQ(result, 42);
}

TEST(ThreadPoolTask, FunctionPointer)
{
	static int staticValue = 0;

	ThreadPool::Task task{[] { staticValue = 123; }};

	task();

	EXPECT_EQ(staticValue, 123);
}

TEST(ThreadPoolTask, StdFunction)
{
	int value = 0;
	std::function< void () > func = [&value] { value = 99; };

	ThreadPool::Task task{std::move(func)};

	task();

	EXPECT_EQ(value, 99);
}

/* ============================================================================
 * ThreadPool construction tests
 * ============================================================================ */

TEST(ThreadPool, DefaultConstructor)
{
	ThreadPool pool;

	EXPECT_GE(pool.threadCount(), 1U);
	EXPECT_LE(pool.threadCount(), std::thread::hardware_concurrency());
}

TEST(ThreadPool, ConstructorWithThreadCount)
{
	ThreadPool pool(4);

	EXPECT_EQ(pool.threadCount(), 4U);
}

TEST(ThreadPool, ConstructorWithZeroThreads)
{
	/* Zero threads should be adjusted to at least 1. */
	ThreadPool pool(0);

	EXPECT_GE(pool.threadCount(), 1U);
}

TEST(ThreadPool, ConstructorWithOneThread)
{
	ThreadPool pool(1);

	EXPECT_EQ(pool.threadCount(), 1U);
}

/* ============================================================================
 * enqueue tests
 * ============================================================================ */

TEST(ThreadPool, EnqueueSimpleTask)
{
	ThreadPool pool(2);
	std::atomic< bool > executed{false};

	pool.enqueue([&executed] {
		executed = true;
	});

	pool.wait();

	EXPECT_TRUE(executed.load());
}

TEST(ThreadPool, EnqueueMultipleTasks)
{
	ThreadPool pool(4);
	constexpr size_t taskCount = 100;
	std::atomic< size_t > counter{0};

	for ( size_t i = 0; i < taskCount; ++i )
	{
		pool.enqueue([&counter] {
			counter.fetch_add(1, std::memory_order_relaxed);
		});
	}

	pool.wait();

	EXPECT_EQ(counter.load(), taskCount);
}

TEST(ThreadPool, EnqueueWithCapture)
{
	ThreadPool pool(2);
	std::vector< int > results(10, 0);
	std::atomic< size_t > completed{0};

	for ( size_t i = 0; i < results.size(); ++i )
	{
		pool.enqueue([&results, &completed, i] {
			results[i] = static_cast< int >(i * 2);
			completed.fetch_add(1, std::memory_order_relaxed);
		});
	}

	pool.wait();

	EXPECT_EQ(completed.load(), results.size());

	for ( size_t i = 0; i < results.size(); ++i )
	{
		EXPECT_EQ(results[i], static_cast< int >(i * 2));
	}
}

TEST(ThreadPool, EnqueueStdFunction)
{
	ThreadPool pool(2);
	std::atomic< int > value{0};

	std::function< void () > task = [&value] {
		value.store(42, std::memory_order_relaxed);
	};

	pool.enqueue(std::move(task));
	pool.wait();

	EXPECT_EQ(value.load(), 42);
}

/* ============================================================================
 * enqueueWithResult tests (only available with exceptions enabled)
 * ============================================================================ */

#if __cpp_exceptions
TEST(ThreadPool, EnqueueWithResultInt)
{
	ThreadPool pool(2);

	auto future = pool.enqueueWithResult([] {
		return 42;
	});

	EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPool, EnqueueWithResultVoid)
{
	ThreadPool pool(2);
	std::atomic< bool > executed{false};

	auto future = pool.enqueueWithResult([&executed] {
		executed = true;
	});

	future.get();

	EXPECT_TRUE(executed.load());
}

TEST(ThreadPool, EnqueueWithResultString)
{
	ThreadPool pool(2);

	auto future = pool.enqueueWithResult([] {
		return std::string("Hello, ThreadPool!");
	});

	EXPECT_EQ(future.get(), "Hello, ThreadPool!");
}

TEST(ThreadPool, EnqueueWithResultException)
{
	ThreadPool pool(2);

	auto future = pool.enqueueWithResult([]() -> int {
		throw std::runtime_error("Test exception");
	});

	EXPECT_THROW(future.get(), std::runtime_error);
}

TEST(ThreadPool, EnqueueWithResultMultiple)
{
	ThreadPool pool(4);
	constexpr size_t taskCount = 50;
	std::vector< std::future< size_t > > futures;
	futures.reserve(taskCount);

	for ( size_t i = 0; i < taskCount; ++i )
	{
		futures.push_back(pool.enqueueWithResult([i] {
			return i * i;
		}));
	}

	for ( size_t i = 0; i < taskCount; ++i )
	{
		EXPECT_EQ(futures[i].get(), i * i);
	}
}
#endif

/* ============================================================================
 * enqueueBatch tests
 * ============================================================================ */

TEST(ThreadPool, EnqueueBatchEmpty)
{
	ThreadPool pool(2);
	std::vector< std::function< void () > > tasks;

	size_t enqueued = pool.enqueueBatch(tasks.begin(), tasks.end());

	EXPECT_EQ(enqueued, 0U);
}

TEST(ThreadPool, EnqueueBatchMultipleTasks)
{
	ThreadPool pool(4);
	constexpr size_t taskCount = 100;
	std::atomic< size_t > counter{0};
	std::vector< std::function< void () > > tasks;
	tasks.reserve(taskCount);

	for ( size_t i = 0; i < taskCount; ++i )
	{
		tasks.emplace_back([&counter] {
			counter.fetch_add(1, std::memory_order_relaxed);
		});
	}

	const size_t enqueued = pool.enqueueBatch(tasks.begin(), tasks.end());

	EXPECT_EQ(enqueued, taskCount);

	pool.wait();

	EXPECT_EQ(counter.load(), taskCount);
}

TEST(ThreadPool, EnqueueBatchDistribution)
{
	ThreadPool pool(4);
	constexpr size_t taskCount = 16;
	std::atomic< size_t > counter{0};
	std::vector< std::function< void () > > tasks;

	for ( size_t i = 0; i < taskCount; ++i )
	{
		tasks.emplace_back([&counter] {
			counter.fetch_add(1, std::memory_order_relaxed);
		});
	}

	pool.enqueueBatch(tasks.begin(), tasks.end());
	pool.wait();

	EXPECT_EQ(counter.load(), taskCount);
}

/* ============================================================================
 * parallelFor tests
 * ============================================================================ */

TEST(ThreadPool, ParallelForEmptyRange)
{
	ThreadPool pool(4);
	std::atomic< size_t > counter{0};

	/* Empty range should not execute the body. */
	pool.parallelFor(size_t{0}, size_t{0}, [&counter](size_t) {
		counter.fetch_add(1, std::memory_order_relaxed);
	});

	EXPECT_EQ(counter.load(), 0U);
}

TEST(ThreadPool, ParallelForReversedRange)
{
	ThreadPool pool(4);
	std::atomic< size_t > counter{0};

	/* Reversed range (start >= end) should not execute. */
	pool.parallelFor(size_t{10}, size_t{5}, [&counter](size_t) {
		counter.fetch_add(1, std::memory_order_relaxed);
	});

	EXPECT_EQ(counter.load(), 0U);
}

TEST(ThreadPool, ParallelForSimple)
{
	ThreadPool pool(4);
	constexpr size_t size = 1000;
	std::vector< int > data(size, 0);

	pool.parallelFor(size_t{0}, size, [&data](size_t i) {
		data[i] = static_cast< int >(i * 2);
	});

	for ( size_t i = 0; i < size; ++i )
	{
		EXPECT_EQ(data[i], static_cast< int >(i * 2));
	}
}

TEST(ThreadPool, ParallelForAllIndicesExecuted)
{
	ThreadPool pool(4);
	constexpr size_t size = 500;
	std::vector< std::atomic< int > > flags(size);

	for ( auto & flag : flags )
	{
		flag.store(0, std::memory_order_relaxed);
	}

	pool.parallelFor(size_t{0}, size, [&flags](size_t i) {
		flags[i].fetch_add(1, std::memory_order_relaxed);
	});

	/* Verify each index was executed exactly once. */
	for ( size_t i = 0; i < size; ++i )
	{
		EXPECT_EQ(flags[i].load(), 1) << "Index " << i << " was not executed exactly once";
	}
}

TEST(ThreadPool, ParallelForWithOffset)
{
	ThreadPool pool(4);
	constexpr size_t start = 10;
	constexpr size_t end = 110;
	std::vector< int > data(end, -1);

	pool.parallelFor(start, end, [&data](size_t i) {
		data[i] = static_cast< int >(i);
	});

	/* Values before start should be unchanged. */
	for ( size_t i = 0; i < start; ++i )
	{
		EXPECT_EQ(data[i], -1);
	}

	/* Values in range should be set. */
	for ( size_t i = start; i < end; ++i )
	{
		EXPECT_EQ(data[i], static_cast< int >(i));
	}
}

TEST(ThreadPool, ParallelForWithGrainSize)
{
	ThreadPool pool(4);
	constexpr size_t size = 1000;
	std::atomic< size_t > counter{0};

	pool.parallelFor(size_t{0}, size, [&counter](size_t) {
		counter.fetch_add(1, std::memory_order_relaxed);
	}, 100);

	EXPECT_EQ(counter.load(), size);
}

TEST(ThreadPool, ParallelForSmallWorkload)
{
	/* With a single thread pool, small workloads should run sequentially. */
	ThreadPool pool(1);
	constexpr size_t size = 10;
	std::vector< int > data(size, 0);

	pool.parallelFor(size_t{0}, size, [&data](size_t i) {
		data[i] = static_cast< int >(i);
	});

	for ( size_t i = 0; i < size; ++i )
	{
		EXPECT_EQ(data[i], static_cast< int >(i));
	}
}

/* NOTE: parallelFor requires unsigned integer types due to the std::unsigned_integral constraint.
 * This test verifies functionality with size_t. */
TEST(ThreadPool, ParallelForIntegerType)
{
	ThreadPool pool(4);
	constexpr size_t size = 100;
	std::vector< int > data(size, 0);

	pool.parallelFor(size_t{0}, size, [&data](size_t i) {
		data[i] = static_cast< int >(i * 3);
	});

	for ( size_t i = 0; i < size; ++i )
	{
		EXPECT_EQ(data[i], static_cast< int >(i * 3));
	}
}

TEST(ThreadPool, ParallelForAccumulation)
{
	ThreadPool pool(4);
	constexpr size_t size = 10000;
	std::vector< size_t > values(size);

	pool.parallelFor(size_t{0}, size, [&values](size_t i) {
		values[i] = i;
	});

	/* Sum should be 0 + 1 + 2 + ... + (size-1) = size * (size - 1) / 2. */
	size_t sum = std::accumulate(values.begin(), values.end(), size_t{0});
	size_t expected = size * (size - 1) / 2;

	EXPECT_EQ(sum, expected);
}

/* ============================================================================
 * wait and isIdle tests
 * ============================================================================ */

TEST(ThreadPool, WaitOnEmptyPool)
{
	ThreadPool pool(2);

	/* Wait on pool with no tasks should return immediately. */
	pool.wait();

	EXPECT_TRUE(pool.isIdle());
}

TEST(ThreadPool, WaitForTasks)
{
	ThreadPool pool(2);
	std::atomic< size_t > counter{0};

	for ( size_t i = 0; i < 50; ++i )
	{
		pool.enqueue([&counter] {
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			counter.fetch_add(1, std::memory_order_relaxed);
		});
	}

	pool.wait();

	EXPECT_EQ(counter.load(), 50U);
	EXPECT_TRUE(pool.isIdle());
}

TEST(ThreadPool, IsIdleAfterWait)
{
	ThreadPool pool(4);

	pool.enqueue([] {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	});

	EXPECT_FALSE(pool.isIdle());

	pool.wait();

	EXPECT_TRUE(pool.isIdle());
}

TEST(ThreadPool, PendingTasksCount)
{
	ThreadPool pool(1);
	std::atomic< bool > blockTask{true};
	std::atomic< bool > taskStarted{false};

	/* Enqueue a blocking task. */
	pool.enqueue([&blockTask, &taskStarted] {
		taskStarted = true;
		while ( blockTask.load(std::memory_order_acquire) )
		{
			std::this_thread::yield();
		}
	});

	/* Wait for the blocking task to start. */
	while ( !taskStarted.load(std::memory_order_acquire) )
	{
		std::this_thread::yield();
	}

	/* Enqueue more tasks. */
	for ( int i = 0; i < 5; ++i )
	{
		pool.enqueue([] {});
	}

	/* There should be pending tasks. */
	EXPECT_GT(pool.pendingTasks(), 0U);

	/* Release the blocking task. */
	blockTask.store(false, std::memory_order_release);

	pool.wait();

	EXPECT_EQ(pool.pendingTasks(), 0U);
}

TEST(ThreadPool, BusyWorkersCount)
{
	ThreadPool pool(4);

	/* Simple test: verify busyWorkers starts at 0. */
	EXPECT_EQ(pool.busyWorkers(), 0U);

	/* Enqueue tasks and verify workers become busy then idle. */
	std::atomic< size_t > counter{0};
	for ( size_t i = 0; i < 100; ++i )
	{
		pool.enqueue([&counter] {
			counter.fetch_add(1, std::memory_order_relaxed);
		});
	}

	pool.wait();

	/* After wait, all workers should be idle. */
	EXPECT_EQ(pool.busyWorkers(), 0U);
	EXPECT_EQ(counter.load(), 100U);
}

/* ============================================================================
 * Thread safety tests
 * ============================================================================ */

TEST(ThreadPool, ConcurrentEnqueue)
{
	ThreadPool pool(4);
	constexpr size_t tasksPerThread = 100;
	constexpr size_t numEnqueueThreads = 4;
	std::atomic< size_t > totalExecuted{0};

	std::vector< std::thread > enqueuers;

	for ( size_t t = 0; t < numEnqueueThreads; ++t )
	{
		enqueuers.emplace_back([&pool, &totalExecuted] {
			for ( size_t i = 0; i < tasksPerThread; ++i )
			{
				pool.enqueue([&totalExecuted] {
					totalExecuted.fetch_add(1, std::memory_order_relaxed);
				});
			}
		});
	}

	for ( auto & thread : enqueuers )
	{
		thread.join();
	}

	pool.wait();

	EXPECT_EQ(totalExecuted.load(), tasksPerThread * numEnqueueThreads);
}

#if __cpp_exceptions
TEST(ThreadPool, ConcurrentEnqueueWithResult)
{
	ThreadPool pool(4);
	constexpr size_t tasksPerThread = 50;
	constexpr size_t numEnqueueThreads = 4;

	std::vector< std::thread > enqueuers;
	std::vector< std::vector< std::future< size_t > > > allFutures(numEnqueueThreads);

	for ( size_t t = 0; t < numEnqueueThreads; ++t )
	{
		enqueuers.emplace_back([&pool, &allFutures, t] {
			for ( size_t i = 0; i < tasksPerThread; ++i )
			{
				allFutures[t].push_back(pool.enqueueWithResult([t, i] {
					return t * 1000 + i;
				}));
			}
		});
	}

	for ( auto & thread : enqueuers )
	{
		thread.join();
	}

	/* Verify all results. */
	for ( size_t threadIndex = 0; threadIndex < numEnqueueThreads; ++threadIndex )
	{
		for ( size_t taskIndex = 0; taskIndex < tasksPerThread; ++taskIndex )
		{
			EXPECT_EQ(allFutures[threadIndex][taskIndex].get(), threadIndex * 1000 + taskIndex);
		}
	}
}
#endif

TEST(ThreadPool, StressTest)
{
	constexpr size_t totalTasks = 10000;

	ThreadPool pool(8);

	std::atomic< size_t > counter{0};

	for ( size_t taskIndex = 0; taskIndex < totalTasks; ++taskIndex )
	{
		pool.enqueue([&counter] {
			counter.fetch_add(1, std::memory_order_relaxed);
		});
	}

	pool.wait();

	EXPECT_EQ(counter.load(), totalTasks);
}

TEST(ThreadPool, VaryingWorkloads)
{
	/* Test that unbalanced workloads complete correctly. */
	ThreadPool pool(4);
	constexpr size_t taskCount = 100;
	std::atomic< size_t > counter{0};

	/* Enqueue tasks with varying execution times. */
	for ( size_t i = 0; i < taskCount; ++i )
	{
		pool.enqueue([&counter, i] {
			/* Simulate varying workloads. */
			if ( i % 10 == 0 )
			{
				std::this_thread::sleep_for(std::chrono::microseconds(100));
			}
			counter.fetch_add(1, std::memory_order_relaxed);
		});
	}

	pool.wait();

	EXPECT_EQ(counter.load(), taskCount);
}

/* ============================================================================
 * Edge case tests
 * ============================================================================ */

TEST(ThreadPool, DestructorWaitsForTasks)
{
	std::atomic< size_t > counter{0};

	{
		ThreadPool pool(2);

		for ( size_t i = 0; i < 50; ++i )
		{
			pool.enqueue([&counter] {
				std::this_thread::sleep_for(std::chrono::microseconds(100));
				counter.fetch_add(1, std::memory_order_relaxed);
			});
		}

		/* Pool destructor should wait for all tasks. */
	}

	EXPECT_EQ(counter.load(), 50U);
}

TEST(ThreadPool, RapidEnqueueDequeue)
{
	ThreadPool pool(4);
	std::atomic< size_t > counter{0};

	/* Enqueue all tasks first, then wait once at the end. */
	for ( int round = 0; round < 10; ++round )
	{
		for ( size_t i = 0; i < 100; ++i )
		{
			pool.enqueue([&counter] {
				counter.fetch_add(1, std::memory_order_relaxed);
			});
		}
	}

	pool.wait();

	EXPECT_EQ(counter.load(), 1000U);
}

TEST(ThreadPool, LargeCaptureTask)
{
	ThreadPool pool(2);
	std::array< int, 1000 > largeData{};
	std::iota(largeData.begin(), largeData.end(), 0);
	std::atomic< int > result{0};

	pool.enqueue([largeData, &result] {
		int sum = 0;
		for ( int val : largeData )
		{
			sum += val;
		}
		result.store(sum, std::memory_order_relaxed);
	});

	pool.wait();

	/* Sum of 0 to 999 = 999 * 1000 / 2 = 499500. */
	EXPECT_EQ(result.load(), 499500);
}

TEST(ThreadPool, MoveOnlyCaptureInEnqueue)
{
	ThreadPool pool(2);
	auto ptr = std::make_unique< int >(42);
	std::atomic< int > result{0};

	pool.enqueue([p = std::move(ptr), &result] {
		result.store(*p, std::memory_order_relaxed);
	});

	pool.wait();

	EXPECT_EQ(result.load(), 42);
}

TEST(ThreadPool, ParallelPixmapDrawing)
{
	ThreadPool pool;

	constexpr uint32_t imageWidth = 3840;
	constexpr uint32_t imageHeight = 2160;
	constexpr size_t operationCount = 50000;
	constexpr size_t iterationCount = 100;

	/* Seed for reproducible random generation. */
	std::mt19937 rng{42}; // NOLINT(cert-msc32-c, cert-msc51-cpp)
	std::uniform_int_distribution< int32_t > distX{0, static_cast< int32_t >(imageWidth - 1)};
	std::uniform_int_distribution< int32_t > distY{0, static_cast< int32_t >(imageHeight - 1)};
	std::uniform_int_distribution< int32_t > distRadius{5, 100};
	std::uniform_real_distribution< float > distColor{0.0F, 1.0F};
	std::uniform_int_distribution< int > distOperation{0, 2}; /* 0: segment, 1: circle, 2: square */

	/* Pre-generate random drawing operations. */
	struct DrawOperation
	{
		int type;
		int32_t x1, y1, x2, y2;
		int32_t radius;
		Color< float > color;
	};

	std::vector< DrawOperation > operations;
	operations.reserve(operationCount);

	for ( size_t operationIndex = 0; operationIndex < operationCount; ++operationIndex )
	{
		DrawOperation op{};
		op.type = distOperation(rng);
		op.x1 = distX(rng);
		op.y1 = distY(rng);
		op.x2 = distX(rng);
		op.y2 = distY(rng);
		op.radius = distRadius(rng);
		op.color = Color< float >{distColor(rng), distColor(rng), distColor(rng), 1.0F};

		operations.push_back(op);
	}

	/* Lambda to execute all drawing operations on a processor. */
	auto executeOperations = [&operations] (Processor< uint8_t > & processor)
	{
		for ( const auto & op : operations )
		{
			switch ( op.type )
			{
				case 0 : /* Segment */
					processor.drawSegment(
						Vector< 2, int32_t >{op.x1, op.y1},
						Vector< 2, int32_t >{op.x2, op.y2},
						op.color
					);
					break;

				case 1 : /* Circle */
					processor.drawCircle(
						Vector< 2, int32_t >{op.x1, op.y1},
						op.radius,
						op.color
					);
					break;

				case 2 : /* Square */
					processor.drawSquare(
						Space2D::AARectangle< int32_t >{op.x1, op.y1, std::abs(op.x2 - op.x1) + 1, std::abs(op.y2 - op.y1) + 1},
						op.color
					);
					break;

				default :
					break;
			}
		}
	};

	/* ======================================================================
	 * Sequential execution: 100 iterations in a classic loop
	 * ====================================================================== */
	std::chrono::microseconds sequentialDuration{};
	{
		const auto start = std::chrono::high_resolution_clock::now();

		for ( size_t iteration = 0; iteration < iterationCount; ++iteration )
		{
			Pixmap< uint8_t > image{imageWidth, imageHeight};
			Processor< uint8_t > processor{image};

			executeOperations(processor);
		}

		const auto end = std::chrono::high_resolution_clock::now();
		sequentialDuration = std::chrono::duration_cast< std::chrono::microseconds >(end - start);
	}

	/* ======================================================================
	 * Parallel execution: 100 iterations using parallelFor
	 * ====================================================================== */
	std::chrono::microseconds parallelDuration{};
	{
		const auto start = std::chrono::high_resolution_clock::now();

		pool.parallelFor(size_t{0}, iterationCount, [&] (size_t) {
			Pixmap< uint8_t > image{imageWidth, imageHeight};
			Processor< uint8_t > processor{image};

			executeOperations(processor);
		});

		const auto end = std::chrono::high_resolution_clock::now();
		parallelDuration = std::chrono::duration_cast< std::chrono::microseconds >(end - start);
	}

	/* Print results for information. */
	const auto sequentialMs = static_cast< double >(sequentialDuration.count()) / 1000.0;
	const auto parallelMs = static_cast< double >(parallelDuration.count()) / 1000.0;
	const auto speedup = sequentialMs / parallelMs;
	const auto timeGainPercent = ((sequentialMs - parallelMs) / sequentialMs) * 100.0;

	std::cout << "[          ] Sequential: " << sequentialMs << " ms\n";
	std::cout << "[          ] Parallel:   " << parallelMs << " ms (" << pool.threadCount() << " threads)\n";
	std::cout << "[          ] Speedup:    " << speedup << "x | Time saved: " << timeGainPercent << "%\n";

	/* The parallel version should be faster than sequential. */
	EXPECT_LT(parallelDuration.count(), sequentialDuration.count());
}