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
#include <iostream>
#include <atomic>
#include <cstddef>
#include <concepts>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <new>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace EmEn::Libs
{
	/**
	 * @class ThreadPool
	 * @brief High-performance thread pool optimized for game engine workloads.
	 *
	 * ThreadPool provides efficient task-based parallelism with several optimizations
	 * designed for real-time applications and game engines. The implementation focuses
	 * on minimizing allocation overhead and simplicity for reliability.
	 *
	 * ## Key Features
	 *
	 * ### Small Buffer Optimization (SBO)
	 * Tasks up to 48 bytes are stored inline without heap allocation, covering most
	 * lambdas with small captures (approximately 6 pointers). Larger tasks automatically
	 * fall back to heap allocation. This eliminates memory allocation for the common case.
	 *
	 * ### Single Shared Queue
	 * Uses a single shared task queue protected by a mutex. This design provides
	 * simplicity and reliability while still achieving good performance for typical
	 * game engine workloads. The single-queue design eliminates complex synchronization
	 * issues that can arise with more sophisticated approaches.
	 *
	 * ### Template-Based Task Submission
	 * The template-based enqueue() avoids std::function overhead for known callable types,
	 * while still maintaining compatibility with std::function for type-erased callables.
	 * This eliminates virtual dispatch costs in performance-critical code.
	 *
	 * ### Data-Parallel Support
	 * The parallelFor() method provides an optimized fork-join pattern for data-parallel
	 * workloads. It distributes work automatically across available workers using atomic
	 * chunk distribution, and the calling thread participates in the work to avoid
	 * wasting resources. Blocks until completion, providing implicit synchronization.
	 *
	 * ### Batch Operations
	 * The enqueueBatch() method allows submitting multiple tasks under a single lock
	 * acquisition, reducing synchronization overhead for bulk task submission.
	 *
	 * ## Thread Safety
	 * All public methods are thread-safe and can be called concurrently from any thread.
	 * The pool uses condition variables for efficient waiting and notification.
	 *
	 * ## Usage Example
	 * @code
	 * // Create a thread pool with hardware thread count
	 * EmEn::Libs::ThreadPool pool;
	 *
	 * // Fire-and-forget task submission
	 * pool.enqueue([] {
	 *     std::cout << "Hello from worker thread\n";
	 * });
	 *
	 * // Task with result
	 * auto future = pool.enqueueWithResult([] {
	 *     return 42;
	 * });
	 * int result = future.get(); // Blocks until task completes
	 *
	 * // Data-parallel loop
	 * std::vector<int> data(10000);
	 * pool.parallelFor(size_t{0}, data.size(), [&](size_t i) {
	 *     data[i] = compute(i);
	 * });
	 * // parallelFor blocks until all iterations complete
	 *
	 * // Batch task submission
	 * std::vector<std::function<void()>> tasks;
	 * tasks.push_back([] { doWork1(); });
	 * tasks.push_back([] { doWork2(); });
	 * pool.enqueueBatch(tasks.begin(), tasks.end());
	 *
	 * // Wait for all tasks to finish
	 * pool.wait();
	 * @endcode
	 *
	 * ## Performance Considerations
	 * - Tasks smaller than 48 bytes avoid heap allocation entirely
	 * - For small workloads, parallelFor() falls back to sequential execution to avoid overhead
	 * - The single-queue design trades maximum throughput for reliability and simplicity
	 * - For typical game engine workloads, queue contention is negligible
	 *
	 * ## Implementation Details
	 * - Workers wait on a condition variable when the queue is empty
	 * - Task completion signals a separate condition variable for wait() callers
	 * - The destructor gracefully drains remaining tasks before joining workers
	 * - Move-only Task wrapper supports lambdas with move-only captures (e.g., std::unique_ptr)
	 *
	 * @note Thread-safe: All public methods can be called from any thread concurrently.
	 * @note Platform support: Tested on Linux (G++ 13.3+), macOS (AppleClang 17.0+), Windows (MSVC 2022+).
	 * @note Requires C++20 for concepts and std::invocable support.
	 * @see Task
	 * @version 0.8.38
	 */
	class ThreadPool final
	{
		public:

			/** @brief Small buffer size for inline task storage (48 bytes = 6 pointers). */
			static constexpr size_t SmallBufferSize{48};

			/**
			 * @class Task
			 * @brief Move-only callable wrapper with Small Buffer Optimization.
			 *
			 * Task is a type-erased callable wrapper designed to minimize heap allocations
			 * for small function objects. It uses Small Buffer Optimization (SBO) to store
			 * callables up to SmallBufferSize (48 bytes) inline, avoiding dynamic allocation
			 * entirely for the common case.
			 *
			 * This class serves as a C++20 alternative to std::move_only_function (which was
			 * introduced in C++23) and provides better performance characteristics than
			 * std::function for thread pool workloads by being move-only.
			 *
			 * ## Storage Strategy
			 * - **Small callables** (≤48 bytes, standard alignment): Stored in inline buffer
			 * - **Large callables** (>48 bytes or over-aligned): Heap-allocated via new/delete
			 *
			 * ## Type Erasure
			 * Uses function pointers for invoke, destroy, and move operations to achieve
			 * type erasure without virtual dispatch overhead.
			 *
			 * ## Move Semantics
			 * Task is move-only (non-copyable) to avoid expensive copy operations and to
			 * support move-only captured values in lambdas (e.g., std::unique_ptr).
			 *
			 * ## Usage Example
			 * @code
			 * // Small lambda (fits in SBO)
			 * ThreadPool::Task task1{[] { std::cout << "Hello\n"; }};
			 *
			 * // Lambda with small capture (fits in SBO)
			 * int x = 42;
			 * ThreadPool::Task task2{[x] { std::cout << x << "\n"; }};
			 *
			 * // Large capture (heap-allocated)
			 * std::array<int, 100> largeData{};
			 * ThreadPool::Task task3{[largeData] { ... }};
			 *
			 * // Move-only capture
			 * auto ptr = std::make_unique<int>(42);
			 * ThreadPool::Task task4{[p = std::move(ptr)] { std::cout << *p << "\n"; }};
			 *
			 * // Invoke the task
			 * task1(); // Calls the stored callable
			 * @endcode
			 *
			 * @note Move-only: Cannot be copied, only moved.
			 * @note Noexcept guarantee: Move operations are noexcept for small callables.
			 * @see ThreadPool
			 */
			class Task final
			{
				public:

					/**
					 * @brief Constructs an empty task.
					 * @post empty() returns true.
					 */
					Task () noexcept = default;

					/**
					 * @brief Constructs a task from any invocable object.
					 *
					 * Stores the callable using Small Buffer Optimization if it fits within
					 * SmallBufferSize (48 bytes) and has standard alignment. Larger or over-aligned
					 * callables are heap-allocated.
					 *
					 * @tparam function_t Callable type (lambda, function pointer, functor, etc.).
					 * @param callable The callable object to store. Forwarded to avoid unnecessary copies.
					 * @pre F must be invocable with no arguments: callable().
					 * @post empty() returns false, operator bool() returns true.
					 * @note Noexcept if F fits in SBO and is nothrow move-constructible.
					 */
					template< typename function_t >
					requires std::invocable< function_t > && (!std::is_same_v< std::decay_t< function_t >, Task >)
					explicit
					Task (function_t && callable) noexcept(sizeof(std::decay_t< function_t >) <= SmallBufferSize && std::is_nothrow_move_constructible_v< std::decay_t< function_t > >)
					{
						using DecayedF = std::decay_t< function_t >;

						if constexpr ( sizeof(DecayedF) <= SmallBufferSize && alignof(DecayedF) <= alignof(std::max_align_t) )
						{
							/* Small buffer: construct in-place. */
							::new (static_cast< void * >(m_storage)) DecayedF(std::forward< function_t >(callable));
							m_invoke = &invokeSmall< DecayedF >;
							m_destroy = &destroySmall< DecayedF >;
							m_move = &moveSmall< DecayedF >;
							m_isSmall = true;
						}
						else
						{
							/* Large buffer: heap allocate. */
							m_heapPtr = new DecayedF(std::forward< function_t >(callable));
							m_invoke = &invokeLarge< DecayedF >;
							m_destroy = &destroyLarge< DecayedF >;
							m_move = &moveLarge< DecayedF >;
							m_isSmall = false;
						}
					}

					/**
					 * @brief Deleted copy constructor.
					 * @note Task is move-only to support move-only captures and avoid expensive copies.
					 */
					Task (const Task &) = delete;

					/**
					 * @brief Deleted copy assignment operator.
					 * @note Task is move-only to support move-only captures and avoid expensive copies.
					 */
					Task & operator= (const Task &) = delete;

					/**
					 * @brief Move constructor.
					 * @param other Task to move from.
					 * @post other.empty() returns true after the move.
					 */
					Task (Task && other) noexcept
					{
						if ( other.m_invoke != nullptr )
						{
							other.m_move(&other, this);
							other.m_invoke = nullptr;
							other.m_destroy = nullptr;
							other.m_move = nullptr;
						}
					}

					/**
					 * @brief Move assignment operator.
					 *
					 * Destroys the current task (if any) and moves the contents of other
					 * into this task.
					 *
					 * @param other Task to move from.
					 * @return Reference to this task.
					 * @post other.empty() returns true after the move.
					 * @post If this == &other, no operation is performed (self-assignment safe).
					 */
					Task &
					operator= (Task && other) noexcept
					{
						if ( this != &other )
						{
							this->clear();

							if ( other.m_invoke != nullptr )
							{
								other.m_move(&other, this);
								other.m_invoke = nullptr;
								other.m_destroy = nullptr;
								other.m_move = nullptr;
							}
						}

						return *this;
					}

					/**
					 * @brief Destructor.
					 *
					 * Destroys the stored callable, invoking its destructor. For heap-allocated
					 * callables, releases the allocated memory.
					 */
					~Task () noexcept
					{
						this->clear();
					}

					/**
					 * @brief Invokes the stored callable.
					 *
					 * Executes the callable that was passed to the constructor. After invocation,
					 * the task remains valid and can be invoked again (though this is not typical
					 * usage in a thread pool context).
					 *
					 * @pre empty() must return false (task must contain a callable).
					 * @note Undefined behavior if called on an empty task.
					 * @note Any exceptions thrown by the callable propagate to the caller.
					 */
					void
					operator() ()
					{
						if ( m_invoke != nullptr )
						{
							m_invoke(this);
						}
						else
						{
							std::cerr << "[ThreadPool-Error] Attempted to invoke empty task!\n";
						}
					}

					/**
					 * @brief Checks if the task contains a callable.
					 * @return True if the task contains a callable (not empty), false otherwise.
					 * @note Equivalent to !empty().
					 */
					[[nodiscard]]
					explicit
					operator bool () const noexcept
					{
						return m_invoke != nullptr;
					}

					/**
					 * @brief Checks if the task is empty.
					 * @return True if the task does not contain a callable, false otherwise.
					 * @note Empty tasks are created by the default constructor or after being moved from.
					 */
					[[nodiscard]]
					bool
					empty () const noexcept
					{
						return m_invoke == nullptr;
					}

					/**
					 * @brief Checks if the callable is stored inline using Small Buffer Optimization.
					 * @return True if using inline storage (callable ≤48 bytes), false if heap-allocated.
					 * @note Useful for debugging and performance analysis.
					 */
					[[nodiscard]]
					bool
					isSmall () const noexcept
					{
						return m_isSmall;
					}

				private:

					/**
					 * @brief Clears the stored callable.
					 */
					void
					clear () noexcept
					{
						if ( m_destroy != nullptr )
						{
							m_destroy(this);
							m_invoke = nullptr;
							m_destroy = nullptr;
							m_move = nullptr;
						}
					}

					/* Type-erased function pointers. */
					template< typename function_t >
					static void
					invokeSmall (Task * self)
					{
						(*reinterpret_cast< function_t * >(self->m_storage))();
					}

					template< typename function_t >
					static void
					invokeLarge (Task * self)
					{
						(*static_cast< function_t * >(self->m_heapPtr))();
					}

					template< typename function_t >
					static void
					destroySmall (Task * self) noexcept
					{
						reinterpret_cast< function_t * >(self->m_storage)->~function_t();
					}

					template< typename function_t >
					static void
					destroyLarge (Task * self) noexcept
					{
						delete static_cast< function_t * >(self->m_heapPtr);
					}

					template< typename function_t >
					static void
					moveSmall (Task * from, Task * to) noexcept
					{
						::new (static_cast< void * >(to->m_storage)) function_t(std::move(*reinterpret_cast< function_t * >(from->m_storage)));
						reinterpret_cast< function_t * >(from->m_storage)->~function_t();
						to->m_invoke = from->m_invoke;
						to->m_destroy = from->m_destroy;
						to->m_move = from->m_move;
						to->m_isSmall = true;
					}

					template< typename function_t >
					static void
					moveLarge (Task * from, Task * to) noexcept
					{
						to->m_heapPtr = from->m_heapPtr;
						from->m_heapPtr = nullptr;
						to->m_invoke = from->m_invoke;
						to->m_destroy = from->m_destroy;
						to->m_move = from->m_move;
						to->m_isSmall = false;
					}

					/* Storage: either inline buffer or heap pointer. */
					union
					{
							alignas(std::max_align_t) std::byte m_storage[SmallBufferSize];
							void * m_heapPtr{nullptr};
					};

					void (*m_invoke)(Task *){nullptr};
					void (*m_destroy)(Task *) noexcept {nullptr};
					void (*m_move)(Task *, Task *) noexcept {nullptr};
					bool m_isSmall{false};
			};

			/**
			 * @brief Constructs a thread pool with the specified number of worker threads.
			 *
			 * Creates and launches worker threads immediately. Each worker thread runs
			 * until the pool is destroyed. If threadCount is 0, defaults to at least 1 worker.
			 *
			 * @param threadCount Number of worker threads to create. Defaults to the number
			 *                    of hardware threads available (std::thread::hardware_concurrency()).
			 * @post threadCount() returns the actual number of workers created.
			 * @note If threadCount is 0, it is automatically adjusted to 1.
			 * @note Worker threads are launched immediately and wait for tasks.
			 */
			explicit ThreadPool (size_t threadCount = std::thread::hardware_concurrency());

			/**
			 * @brief Deleted copy constructor.
			 * @note ThreadPool manages threads and cannot be copied.
			 */
			ThreadPool (const ThreadPool &) = delete;

			/**
			 * @brief Deleted move constructor.
			 * @note ThreadPool manages threads and cannot be moved.
			 */
			ThreadPool (ThreadPool &&) = delete;

			/**
			 * @brief Deleted copy assignment operator.
			 * @note ThreadPool manages threads and cannot be copied.
			 */
			ThreadPool & operator= (const ThreadPool &) = delete;

			/**
			 * @brief Deleted move assignment operator.
			 * @note ThreadPool manages threads and cannot be moved.
			 */
			ThreadPool & operator= (ThreadPool &&) = delete;

			/**
			 * @brief Destructs the thread pool, gracefully shutting down all workers.
			 *
			 * Signals all workers to stop, wakes them up, and waits for them to finish
			 * any in-progress tasks. Remaining queued tasks are drained by each worker
			 * before it exits.
			 *
			 * @note Blocks until all worker threads have terminated.
			 * @note Any tasks still queued when the destructor runs will be executed before shutdown.
			 */
			~ThreadPool () noexcept;

			/**
			 * @brief Returns the number of worker threads in the pool.
			 * @return The total number of worker threads created at construction.
			 * @note Thread-safe: Does not change after construction.
			 */
			[[nodiscard]]
			size_t
			threadCount () const noexcept
			{
				return m_workers.size();
			}

			/**
			 * @brief Returns the number of workers currently executing tasks.
			 *
			 * Provides a snapshot of how many worker threads are actively running tasks
			 * at the moment of the call. Useful for monitoring pool utilization.
			 *
			 * @return The number of busy workers (range: 0 to threadCount()).
			 * @note Thread-safe: Uses atomic load with relaxed ordering.
			 * @note The value may be stale by the time it is used due to concurrent operations.
			 */
			[[nodiscard]]
			size_t
			busyWorkers () const noexcept
			{
				return m_busyWorkers.load(std::memory_order_relaxed);
			}

			/**
			 * @brief Returns the approximate number of pending tasks in the queue.
			 *
			 * Counts tasks that have been enqueued but not yet started executing. This
			 * does not include tasks currently being executed by workers.
			 *
			 * @return The approximate number of queued tasks waiting to be executed.
			 * @note Thread-safe: Uses atomic load with relaxed ordering.
			 * @note Due to concurrent operations, the value is approximate
			 *       and may be stale by the time it is used.
			 */
			[[nodiscard]]
			size_t
			pendingTasks () const noexcept
			{
				return m_pendingTasks.load(std::memory_order_relaxed);
			}

			/**
			 * @brief Checks if all tasks are complete and all workers are idle.
			 *
			 * Returns true when there are no pending tasks in any queue and no workers
			 * are currently executing tasks. Useful for determining when the pool has
			 * finished all submitted work.
			 *
			 * @return True if pendingTasks() == 0 and busyWorkers() == 0, false otherwise.
			 * @note Thread-safe: Uses atomic loads with acquire ordering for memory visibility.
			 * @note The result may become stale immediately if tasks are enqueued concurrently.
			 */
			[[nodiscard]]
			bool
			isIdle () const noexcept
			{
				return m_pendingTasks.load(std::memory_order_acquire) == 0 && m_busyWorkers.load(std::memory_order_acquire) == 0;
			}

			/**
			 * @brief Adds a task to the pool for asynchronous execution (template version).
			 *
			 * This is the preferred method for submitting tasks. The template version avoids
			 * std::function overhead by storing the callable directly using the Task class
			 * with Small Buffer Optimization.
			 *
			 * @tparam function_t Callable type (lambda, function pointer, functor, etc.).
			 * @param callable The callable to execute. Must be invocable with no arguments.
			 * @return True if enqueued successfully, false if the pool is shutting down.
			 * @note Thread-safe: Can be called concurrently from multiple threads.
			 * @note No guarantee on execution order or timing.
			 * @note If enqueue returns false, the callable will not be executed.
			 *
			 * @code
			 * ThreadPool pool;
			 * bool success = pool.enqueue([] {
			 *     std::cout << "Task executed\n";
			 * });
			 * @endcode
			 */
			template< typename function_t >
			requires std::invocable< function_t >
			bool
			enqueue (function_t && callable)
			{
				return this->enqueueTask(Task{std::forward< function_t >(callable)});
			}

			/**
			 * @brief Adds a task to the pool (std::function version, for compatibility).
			 *
			 * This overload accepts std::function for compatibility with code that requires
			 * type erasure. Prefer the template version when possible for better performance.
			 *
			 * @param task A std::function<void()> to execute.
			 * @return True if enqueued successfully, false if the pool is shutting down.
			 * @note Thread-safe: Can be called concurrently from multiple threads.
			 */
			bool
			enqueue (std::function< void() > task)
			{
				return this->enqueueTask(Task{std::move(task)});
			}

			/**
			 * @brief Adds multiple tasks to the pool in a single batch operation.
			 *
			 * More efficient than multiple individual enqueue() calls. This method adds
			 * all tasks under a single lock acquisition and uses notify_all() to wake
			 * all workers, reducing synchronization overhead.
			 *
			 * @tparam Iterator Forward iterator type that dereferences to a callable object.
			 * @param begin Iterator to the first task to enqueue.
			 * @param end Iterator past the last task (standard C++ end iterator).
			 * @return The number of tasks successfully enqueued (0 if pool is shutting down).
			 * @note Thread-safe: Can be called concurrently from multiple threads.
			 * @note All tasks in the range [begin, end) must be valid callables.
			 * @note Returns 0 immediately if begin == end or if the pool is stopped.
			 *
			 * @code
			 * ThreadPool pool;
			 * std::vector<std::function<void()>> tasks;
			 * tasks.push_back([] { std::cout << "Task 1\n"; });
			 * tasks.push_back([] { std::cout << "Task 2\n"; });
			 * tasks.push_back([] { std::cout << "Task 3\n"; });
			 * size_t enqueued = pool.enqueueBatch(tasks.begin(), tasks.end());
			 * @endcode
			 */
			template< typename Iterator >
			size_t
			enqueueBatch (Iterator begin, Iterator end)
			{
				if ( begin == end )
				{
					return 0;
				}

				if ( m_stop.load(std::memory_order_acquire) )
				{
					return 0;
				}

				size_t count = 0;

				{
					std::lock_guard< std::mutex > lock{m_mutex};

					for ( auto it = begin; it != end; ++it )
					{
						m_tasks.emplace_back(std::move(*it));
						++count;
					}

					m_pendingTasks.fetch_add(count, std::memory_order_release);
				}

				m_condition.notify_all();

				return count;
			}

#if __cpp_exceptions
			/**
			 * @brief Adds a task and returns a std::future for retrieving its result.
			 *
			 * Use this method when you need to retrieve the return value of a task or
			 * detect exceptions thrown during task execution. The returned future can be
			 * used to block until the task completes and retrieve its result.
			 *
			 * The future provides synchronization: calling future.get() will block until
			 * the task completes. If the task throws an exception, it will be propagated
			 * when get() is called.
			 *
			 * @tparam function_t Callable type with any return type (void or non-void).
			 * @param callable The callable to execute. Must be invocable with no arguments.
			 * @return std::future<ReturnType> where ReturnType is the callable's return type.
			 * @note Thread-safe: Can be called concurrently from multiple threads.
			 * @note The task is wrapped in exception-safe handling: exceptions are captured
			 *       and rethrown when future.get() is called.
			 * @note Slightly more overhead than enqueue() due to std::promise/std::future.
			 * @note This method is only available when C++ exceptions are enabled.
			 *
			 * @code
			 * ThreadPool pool;
			 *
			 * // Task returning a value
			 * auto future = pool.enqueueWithResult([] {
			 *     return 42;
			 * });
			 * int result = future.get(); // Blocks until task completes, returns 42
			 *
			 * // Task returning void
			 * auto voidFuture = pool.enqueueWithResult([] {
			 *     std::cout << "Task completed\n";
			 * });
			 * voidFuture.get(); // Blocks until task completes
			 *
			 * // Exception handling
			 * auto exFuture = pool.enqueueWithResult([] {
			 *     throw std::runtime_error("Error");
			 *     return 0;
			 * });
			 * try {
			 *     exFuture.get(); // Rethrows the exception
			 * } catch (const std::runtime_error& e) {
			 *     std::cerr << e.what() << "\n";
			 * }
			 * @endcode
			 */
			template< typename function_t >
			requires std::invocable< function_t >
			[[nodiscard]]
			auto
			enqueueWithResult (function_t && callable) -> std::future< std::invoke_result_t< function_t > >
			{
				using ReturnType = std::invoke_result_t< function_t >;

				auto promise = std::make_shared< std::promise< ReturnType > >();
				auto future = promise->get_future();

				this->enqueue([p = std::move(promise), f = std::forward< function_t >(callable)] () mutable {
					try
					{
						if constexpr ( std::is_void_v< ReturnType > )
						{
							f();
							p->set_value();
						}
						else
						{
							p->set_value(f());
						}
					}
					catch ( ... )
					{
						p->set_exception(std::current_exception());
					}
				});

				return future;
			}
#endif

			/**
			 * @brief Executes a callable in parallel over a range of indices (fork-join pattern).
			 *
			 * This is the most efficient way to parallelize data-parallel workloads such as
			 * array processing, image manipulation, or batch computations. The method divides
			 * the iteration space into chunks, distributes them across worker threads,
			 * and blocks until all iterations complete.
			 *
			 * Key optimizations:
			 * - Single lambda instantiation shared across all workers
			 * - Automatic chunk sizing based on worker count and grain size
			 * - The calling thread participates in execution (no wasted resources)
			 * - Dynamic work distribution via atomic counter (natural load balancing)
			 * - Falls back to sequential execution for small workloads to avoid overhead
			 *
			 * @tparam index_t Unsigned integer type for indices (e.g., size_t, uint32_t, uint64_t).
			 * @tparam function_t Callable type that accepts a single IndexType parameter.
			 * @param start First index to process (inclusive).
			 * @param end Last index (exclusive), following
			 * @param body Callable invoked for each index: body(index). Must be thread-safe
			 *             if it accesses shared data.
			 * @param grainSize Minimum number of iterations per task chunk (default 1).
			 *                  Higher values reduce task creation overhead but may cause
			 *                  load imbalance. Automatically increased for very small workloads.
			 *
			 * @pre start < end (if start >= end, returns immediately without executing body).
			 * @post All iterations in [start, end) have been executed exactly once.
			 * @note Blocks until all iterations complete (synchronous operation with implicit wait).
			 * @note Thread-safe: body must be thread-safe for concurrent execution.
			 * @note Exceptions: If body throws, the exception propagates from parallelFor.
			 *       Other iterations may continue executing concurrently.
			 * @note Sequential fallback: If end-start <= grainSize or threadCount() <= 1,
			 *       executes sequentially to avoid parallelization overhead.
			 *
			 * @code
			 * ThreadPool pool;
			 *
			 * // Process array elements
			 * std::vector<int> data(10000);
			 * pool.parallelFor(size_t{0}, data.size(), [&data](size_t i) {
			 *     data[i] = compute(i);
			 * });
			 * // All elements processed when parallelFor returns
			 *
			 * // With custom grain size for better performance
			 * pool.parallelFor(size_t{0}, data.size(), [&data](size_t i) {
			 *     data[i] = expensiveCompute(i);
			 * }, 100); // Process at least 100 elements per task
			 *
			 * // 2D loop example
			 * const size_t width = 1920, height = 1080;
			 * std::vector<Pixel> image(width * height);
			 * pool.parallelFor(size_t{0}, height, [&](size_t y) {
			 *     for (size_t x = 0; x < width; ++x) {
			 *         image[y * width + x] = renderPixel(x, y);
			 *     }
			 * });
			 * @endcode
			 */
			template< typename index_t, typename function_t >
			requires std::unsigned_integral< index_t > && std::invocable< function_t, index_t >
			void
			parallelFor (index_t start, index_t end, function_t && body, size_t grainSize = 1)
			{
				if ( start >= end )
				{
					return;
				}

				const auto totalIterations = static_cast< size_t >(end - start);

				/* For very small workloads, just run sequentially. */
				if ( totalIterations <= grainSize || m_workers.size() <= 1 )
				{
					for ( index_t i = start; i < end; ++i )
					{
						body(i);
					}

					return;
				}

				/* Calculate number of chunks based on worker count. */
				const size_t numWorkers = m_workers.size();
				const size_t effectiveGrain = std::max(grainSize, totalIterations / (numWorkers * 4));
				const size_t numChunks = (totalIterations + effectiveGrain - 1) / effectiveGrain;

				/* Use shared_ptr to ensure state lives until all tasks complete. */
				struct SharedState
				{
					std::atomic< size_t > nextChunk{0};
					std::atomic< size_t > completedTasks{0};
					size_t totalTasks{0};
				};

				auto state = std::make_shared< SharedState >();
				state->totalTasks = numWorkers + 1; /* Workers + calling thread. */

				/* Create worker task. */
				auto workerTask = [state, &body, start, end, effectiveGrain, numChunks] ()
				{
					while ( true )
					{
						const size_t chunkIndex = state->nextChunk.fetch_add(1, std::memory_order_relaxed);

						if ( chunkIndex >= numChunks )
						{
							break;
						}

						const auto chunkStart = static_cast< index_t >(start + chunkIndex * effectiveGrain);
						const auto chunkEnd = static_cast< index_t >(std::min(static_cast< size_t >(chunkStart) + effectiveGrain, static_cast< size_t >(end)));

						for ( index_t index = chunkStart; index < chunkEnd; ++index )
						{
							body(index);
						}
					}

					/* Mark this task as fully done (including the break from loop). */
					state->completedTasks.fetch_add(1, std::memory_order_release);
				};

				/* Enqueue one task per worker. */
				for ( size_t index = 0; index < numWorkers; ++index )
				{
					this->enqueue(workerTask);
				}

				/* The calling thread also participates in the work. */
				workerTask();

				/* Spin-wait for all tasks to complete (not just chunks). */
				while ( state->completedTasks.load(std::memory_order_acquire) < state->totalTasks )
				{
					std::this_thread::yield();
				}
			}

			/**
			 * @brief Blocks until all queued and in-progress tasks are finished.
			 *
			 * Waits until pendingTasks() == 0 and busyWorkers() == 0. This ensures
			 * all enqueued work has completed before returning.
			 *
			 * @post isIdle() returns true when wait() returns.
			 * @note Thread-safe: Can be called concurrently from multiple threads.
			 * @note Does not prevent new tasks from being enqueued concurrently.
			 *       If other threads continue enqueuing, wait() may return while
			 *       new tasks are being added.
			 * @note Calling wait() from a task executed by the pool will deadlock
			 *       if the pool is saturated (all workers busy). Avoid this pattern.
			 *
			 * @code
			 * ThreadPool pool;
			 *
			 * // Enqueue some tasks
			 * for (int i = 0; i < 100; ++i) {
			 *     pool.enqueue([i] {
			 *         std::cout << "Task " << i << "\n";
			 *     });
			 * }
			 *
			 * // Wait for all tasks to complete
			 * pool.wait();
			 * std::cout << "All tasks finished\n";
			 * @endcode
			 */
			void wait ();

		private:

			/**
			 * @brief Internal: Enqueues a Task object.
			 * @param task The task to enqueue.
			 * @return True if enqueued, false if pool is stopped.
			 */
			bool enqueueTask (Task && task);

			/**
			 * @brief Worker thread main loop.
			 */
			void worker ();

			std::vector< std::thread > m_workers;
			std::deque< Task > m_tasks;
			std::mutex m_mutex;
			std::condition_variable m_condition;
			std::condition_variable m_completionCondition;
			std::atomic< size_t > m_pendingTasks{0};
			std::atomic< size_t > m_busyWorkers{0};
			std::atomic< bool > m_stop{false};
	};
}
