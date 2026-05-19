/*
 * src/TracerLogger.hpp
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
 */

#pragma once

/* STL inclusions. */
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <queue>
#include <source_location>
#include <string>
#include <thread>
#include <utility>

/* Local inclusions for usages. */
#include "CoreTypes.hpp"

namespace EmEn
{
	/**
	 * @class TracerEntry
	 * @brief Represents a single log entry with timestamp, severity, tag, message, and location information.
	 *
	 * TracerEntry encapsulates all the information associated with a single log message,
	 * including when it was created, where in the code it originated, which thread produced it,
	 * and what severity level it represents. This class is designed for efficient storage
	 * in the TracerLogger's queue.
	 *
	 * @note Member variables are ordered for optimal memory alignment (largest to smallest).
	 * @note This class is engine-internal: only Tracer and TracerLogger reach for it. External
	 *       code uses the public Tracer API (see Tracer.hpp).
	 * @see TracerLogger, Tracer
	 * @version 0.8.39
	 */
	class TracerEntry final
	{
		public:

			/**
			 * @brief Constructs a tracer entry with all required information.
			 *
			 * The timestamp is automatically captured at construction time using steady_clock.
			 *
			 * @param severity The severity level of the log message.
			 * @param tag A pointer to a C-string tag used for filtering and categorization.
			 * @param message The log message content (moved into the entry).
			 * @param location The source code location where the log was generated.
			 * @param threadId The ID of the thread that created this entry.
			 * @version 0.8.39
			 */
			TracerEntry (Severity severity, const char * tag, std::string message, const std::source_location & location, const std::thread::id & threadId) noexcept
				: m_tag{tag},
				m_message{std::move(message)},
				m_location{location},
				m_threadId{threadId},
				m_severity{severity}
			{

			}

			/**
			 * @brief Returns the timestamp when the entry was created.
			 * @return A const reference to the steady_clock time_point.
			 * @version 0.8.39
			 */
			[[nodiscard]]
			const std::chrono::time_point< std::chrono::steady_clock > &
			time () const noexcept
			{
				return m_time;
			}

			/**
			 * @brief Returns the severity level of the log entry.
			 * @return The severity level (Debug, Info, Success, Warning, Error, or Fatal).
			 * @version 0.8.39
			 */
			[[nodiscard]]
			Severity
			severity () const noexcept
			{
				return m_severity;
			}

			/**
			 * @brief Returns the tag associated with this entry.
			 *
			 * Tags are used for filtering and categorizing log messages.
			 *
			 * @return A pointer to a C-string representing the tag.
			 * @version 0.8.39
			 */
			[[nodiscard]]
			const char *
			tag () const noexcept
			{
				return m_tag;
			}

			/**
			 * @brief Returns the log message content.
			 * @return A const reference to the message string.
			 * @version 0.8.39
			 */
			[[nodiscard]]
			const std::string &
			message () const noexcept
			{
				return m_message;
			}

			/**
			 * @brief Returns the source code location where this entry was generated.
			 *
			 * Includes file name, line number, column number, and function name.
			 *
			 * @return A const reference to the source_location.
			 * @version 0.8.39
			 */
			[[nodiscard]]
			const std::source_location &
			location () const noexcept
			{
				return m_location;
			}

			/**
			 * @brief Returns the ID of the thread that generated this entry.
			 * @return A const reference to the thread ID.
			 * @version 0.8.39
			 */
			[[nodiscard]]
			const std::thread::id &
			threadId () const noexcept
			{
				return m_threadId;
			}

		private:

			/* NOTE: Members ordered for optimal memory alignment (largest to smallest). */
			std::chrono::time_point< std::chrono::steady_clock > m_time{std::chrono::steady_clock::now()};
			const char * m_tag{nullptr};
			std::string m_message;
			std::source_location m_location;
			std::thread::id m_threadId;
			Severity m_severity{Severity::Info};
	};

	/**
	 * @class TracerLogger
	 * @brief Asynchronous file logger that writes log entries to disk in a separate thread.
	 *
	 * TracerLogger provides thread-safe, non-blocking file I/O for log entries. It maintains
	 * an internal queue of entries and processes them in a dedicated worker thread, ensuring
	 * that logging does not block the main application threads.
	 *
	 * The logger supports multiple output formats (Text, JSON, HTML) and automatically
	 * handles file creation, flushing, and proper shutdown.
	 *
	 * @note All public methods are thread-safe.
	 * @note This class is engine-internal. Public consumers should use Tracer's
	 *       enableLogger/disableLogger API rather than instantiating it directly.
	 * @see TracerEntry, Tracer
	 * @version 0.8.39
	 */
	class TracerLogger final
	{
		public:

			/**
			 * @brief Constructs the trace logger with a file path and format.
			 *
			 * Creates or truncates the log file at the specified path. If the file cannot
			 * be opened, the logger will be marked as unusable.
			 *
			 * @param filepath The path to the log file (moved into the logger).
			 * @param logFormat The output format for log entries (Text, JSON, or HTML). Defaults to Text.
			 * @version 0.8.39
			 */
			explicit TracerLogger (std::filesystem::path filepath, LogFormat logFormat = LogFormat::Text) noexcept;

			/**
			 * @brief Destructs the trace logger and ensures clean shutdown.
			 *
			 * Stops the worker thread and waits for it to complete, ensuring all
			 * pending log entries are written to disk before destruction.
			 *
			 * @version 0.8.39
			 */
			~TracerLogger ();

			/**
			 * @brief Queues a log entry for asynchronous writing to the log file.
			 *
			 * This method is thread-safe and non-blocking. The entry is added to an internal
			 * queue and will be written to disk by the worker thread.
			 *
			 * @param severity The severity level of the log entry.
			 * @param tag A pointer to a C-string tag for categorization.
			 * @param message The log message content (moved into the entry).
			 * @param location The source code location where the log was generated.
			 * @version 0.8.39
			 */
			void push (Severity severity, const char * tag, std::string message, const std::source_location & location) noexcept;

			/**
			 * @brief Starts the worker thread that writes log entries to disk.
			 *
			 * The worker thread will process queued entries asynchronously until stop() is called.
			 *
			 * @return true if the thread was successfully started, false if the logger is unusable or already running.
			 * @version 0.8.39
			 */
			bool start () noexcept;

			/**
			 * @brief Signals the worker thread to stop processing entries.
			 *
			 * After calling this method, the worker thread will finish writing any remaining
			 * entries and then terminate. The caller should join the thread afterward.
			 *
			 * @version 0.8.39
			 */
			void stop () noexcept;

			/**
			 * @brief Discards all pending log entries in the queue.
			 *
			 * This method is thread-safe and can be used to clear the queue before shutdown
			 * or to discard unwanted entries.
			 *
			 * @version 0.8.39
			 */
			void clear () noexcept;

		private:

			/**
			 * @brief Worker thread function that processes and writes log entries to disk.
			 *
			 * This method runs in a separate thread and continuously processes queued entries,
			 * writing them to the log file. It blocks waiting for new entries and wakes up
			 * when entries are pushed or when stop() is called.
			 *
			 * @version 0.8.39
			 */
			void task () noexcept;

			std::filesystem::path m_filepath;
			std::queue< TracerEntry > m_entries;
			LogFormat m_logFormat;
			std::thread m_thread;
			std::mutex m_entriesAccess;
			std::condition_variable m_condition;
			std::atomic_bool m_isUsable{false};
			std::atomic_bool m_isRunning{false};
	};
}