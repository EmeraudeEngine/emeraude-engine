/*
 * src/Tracer.hpp
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

/* Engine configuration file. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <iostream>
#include <sstream>
#include <vector>
#include <queue>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <condition_variable>
#include <mutex>
#include <filesystem>
#include <source_location>

/* Local inclusions for usages. */
#include "Libs/BlobTrait.hpp"
#include "CoreTypes.hpp"

/* Forward declarations. */
namespace EmEn
{
	class Arguments;
	class FileSystem;
	class Settings;
}

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
	 * @see TracerLogger, Tracer
	 * @version 0.8.38
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
			 * @version 0.8.38
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
			 * @version 0.8.38
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
			 * @version 0.8.38
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
			 * @version 0.8.38
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
			 * @version 0.8.38
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
			 * @version 0.8.38
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
			 * @version 0.8.38
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
	 * @see TracerEntry, Tracer
	 * @version 0.8.38
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
			 * @version 0.8.38
			 */
			explicit TracerLogger (std::filesystem::path filepath, LogFormat logFormat = LogFormat::Text) noexcept;

			/**
			 * @brief Destructs the trace logger and ensures clean shutdown.
			 *
			 * Stops the worker thread and waits for it to complete, ensuring all
			 * pending log entries are written to disk before destruction.
			 *
			 * @version 0.8.38
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
			 * @version 0.8.38
			 */
			void push (Severity severity, const char * tag, std::string message, const std::source_location & location) noexcept;

			/**
			 * @brief Starts the worker thread that writes log entries to disk.
			 *
			 * The worker thread will process queued entries asynchronously until stop() is called.
			 *
			 * @return true if the thread was successfully started, false if the logger is unusable or already running.
			 * @version 0.8.38
			 */
			bool start () noexcept;

			/**
			 * @brief Signals the worker thread to stop processing entries.
			 *
			 * After calling this method, the worker thread will finish writing any remaining
			 * entries and then terminate. The caller should join the thread afterward.
			 *
			 * @version 0.8.38
			 */
			void stop () noexcept;

			/**
			 * @brief Discards all pending log entries in the queue.
			 *
			 * This method is thread-safe and can be used to clear the queue before shutdown
			 * or to discard unwanted entries.
			 *
			 * @version 0.8.38
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
			 * @version 0.8.38
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

	/**
	 * @class Tracer
	 * @brief Main singleton service responsible for logging messages to console and files.
	 *
	 * Tracer is the central logging facility for the Emeraude Engine. It provides thread-safe
	 * logging capabilities with support for multiple severity levels, tag-based filtering,
	 * and optional file output. The service is initialized early in the engine lifecycle
	 * and is available throughout the application's lifetime.
	 *
	 * Key features:
	 * - Thread-safe console and file output
	 * - Multiple severity levels (Debug, Info, Success, Warning, Error, Fatal)
	 * - Tag-based filtering for focused debugging
	 * - Configurable output formatting (ANSI colors for console)
	 * - Optional asynchronous file logging via TracerLogger
	 * - Source location tracking (file, line, column, function)
	 * - Process and thread ID tracking for multi-process applications
	 *
	 * The Tracer follows a two-phase initialization:
	 * 1. earlySetup() - Called during engine bootstrap, before other services
	 * 2. lateSetup() - Called after FileSystem and Settings are available
	 *
	 * @note This class implements the singleton pattern and is non-copyable, non-movable.
	 * @see TracerLogger, TracerEntry, T_TraceHelperBase
	 * @version 0.8.38
	 */
	class Tracer final
	{
		public:

			/** @brief Class identifier used in log messages. */
			static constexpr auto ClassId{"Tracer"};

			/** @brief ANSI Control Sequence Introducer for terminal formatting. */
			static constexpr auto CSI{"\033["};

			/** @brief Deleted copy constructor (singleton pattern). */
			Tracer (const Tracer & copy) noexcept = delete;

			/** @brief Deleted move constructor (singleton pattern). */
			Tracer (Tracer && copy) noexcept = delete;

			/** @brief Deleted copy assignment operator (singleton pattern). */
			Tracer & operator= (const Tracer & copy) noexcept = delete;

			/** @brief Deleted move assignment operator (singleton pattern). */
			Tracer & operator= (Tracer && copy) noexcept = delete;

			/**
			 * @brief Destructs the tracer and ensures clean shutdown.
			 *
			 * Disables the logger and waits for all pending entries to be written.
			 *
			 * @version 0.8.38
			 */
			~Tracer ();

			/**
			 * @brief Returns the singleton instance of the Tracer.
			 *
			 * This method provides thread-safe access to the global Tracer instance.
			 * The instance is created on first access and persists for the application lifetime.
			 *
			 * @return A reference to the singleton Tracer instance.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			static
			Tracer &
			getInstance () noexcept;

			/**
			 * @brief Performs early initialization of the Tracer service.
			 *
			 * This is the first setup phase, called during engine bootstrap before other services
			 * are available. It processes command-line arguments to configure basic tracing behavior
			 * and identifies the process for multi-process applications.
			 *
			 * @param arguments The command-line arguments passed to the application.
			 * @param processName A descriptive name for this process (moved into the tracer).
			 * @param childProcess True if this is a child process, false for the main process.
			 * @version 0.8.38
			 */
			void earlySetup (const Arguments & arguments, std::string processName, bool childProcess) noexcept;

			/**
			 * @brief Completes initialization after FileSystem and Settings services are available.
			 *
			 * This is the second setup phase, called after the FileSystem and Settings services
			 * have been initialized. It configures file logging, applies user preferences from
			 * settings, and finalizes the Tracer configuration.
			 *
			 * @param arguments The command-line arguments (for runtime overrides).
			 * @param fileSystem The FileSystem service (for cache directory access).
			 * @param settings The Settings service (for user preferences).
			 * @todo Clarify the behavior around logger enablement from settings vs arguments.
			 * @version 0.8.38
			 */
			void lateSetup (const Arguments & arguments, const FileSystem & fileSystem, Settings & settings) noexcept;

			/**
			 * @brief Checks if the Tracer service has completed early initialization.
			 * @return true if earlySetup() has been called, false otherwise.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			bool
			isServiceInitialized () const noexcept
			{
				return m_serviceInitialized;
			}

			/**
			 * @brief Returns the descriptive name of the current process.
			 *
			 * This is particularly useful in multi-process applications to distinguish
			 * log output from different processes.
			 *
			 * @return A const reference to the process name string.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			const std::string &
			processName () const noexcept
			{
				return m_processName;
			}

			/**
			 * @brief Adds a tag filter to show only messages with matching tags.
			 *
			 * When filters are active, only log messages with tags matching one of the
			 * registered filters will be displayed on the console. This is useful for
			 * focused debugging of specific subsystems.
			 *
			 * @param filter The tag string to filter (moved into the filter list).
			 * @note This only affects console output, not file logging.
			 * @version 0.8.38
			 */
			void
			addTagFilter (std::string filter)
			{
				m_filters.emplace_back(std::move(filter));
			}

			/**
			 * @brief Removes all tag filters, showing all log messages again.
			 * @version 0.8.38
			 */
			void
			removeAllTagFilters () noexcept
			{
				m_filters.clear();
			}

			/**
			 * @brief Controls whether only errors and warnings are printed to the console.
			 *
			 * When enabled, Info, Success, and Debug messages are suppressed on the console
			 * but are still written to the log file if logging is enabled.
			 *
			 * @param state true to show only errors/warnings, false to show all messages.
			 * @note This does not affect file logging.
			 * @version 0.8.38
			 */
			void
			enablePrintOnlyErrors (bool state) noexcept
			{
				m_printOnlyErrors = state;
			}

			/**
			 * @brief Checks if the console is in errors-only mode.
			 * @return true if only errors/warnings are shown, false if all messages are shown.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			bool
			printOnlyErrors () const noexcept
			{
				return m_printOnlyErrors;
			}

			/**
			 * @brief Controls whether source location information is shown in console output.
			 *
			 * When enabled, each console log message includes the file name, line number,
			 * column number, and function name where the log originated.
			 *
			 * @param state true to show source location, false to hide it.
			 * @note This does not affect file logging, which always includes location.
			 * @version 0.8.38
			 */
			void
			enableSourceLocation (bool state) noexcept
			{
				m_sourceLocationEnabled = state;
			}

			/**
			 * @brief Checks if source location information is enabled in console output.
			 * @return true if source location is shown, false otherwise.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			bool
			isSourceLocationEnabled () const noexcept
			{
				return m_sourceLocationEnabled;
			}

			/**
			 * @brief Controls whether thread and process information is shown in console output.
			 *
			 * When enabled, each console log message includes parent process ID (PPID),
			 * process ID (PID), and thread ID (TID).
			 *
			 * @param state true to show thread/process info, false to hide it.
			 * @note This does not affect file logging, which always includes thread ID.
			 * @version 0.8.38
			 */
			void
			enableThreadInfos (bool state) noexcept
			{
				m_threadInfosEnabled = state;
			}

			/**
			 * @brief Checks if thread and process information is enabled in console output.
			 * @return true if thread/process info is shown, false otherwise.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			bool
			isThreadInfosEnabled () const noexcept
			{
				return m_threadInfosEnabled;
			}

			/**
			 * @brief Disables all Tracer output (both console and file).
			 *
			 * When disabled, all trace() calls become no-ops, producing no output.
			 * This is useful for completely silencing the engine's logging.
			 *
			 * @param state true to disable tracing, false to enable it.
			 * @version 0.8.38
			 */
			void
			disableTracer (bool state) noexcept
			{
				m_isTracerDisabled = state;
			}

			/**
			 * @brief Checks if the Tracer is completely disabled.
			 * @return true if tracing is disabled, false if it's active.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			bool
			isTracerDisabled () const noexcept
			{
				return m_isTracerDisabled;
			}

			/**
			 * @brief Checks if file logging was requested at startup.
			 *
			 * This indicates whether the user's settings or command-line arguments
			 * requested file logging to be enabled during initialization.
			 *
			 * @return true if logger was requested at startup, false otherwise.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			bool
			isLoggerRequestedAtStartup () const noexcept
			{
				return m_loggerRequestedAtStartup;
			}

			/**
			 * @brief Enables file logging with the specified log file path.
			 *
			 * Creates a TracerLogger instance and starts the worker thread to write
			 * log entries to the specified file. If a logger is already active, this
			 * method returns immediately.
			 *
			 * @param filepath The path to the log file to create/open.
			 * @return true if logging was successfully enabled, false on failure.
			 * @version 0.8.38
			 */
			bool enableLogger (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Checks if file logging is currently active.
			 * @return true if a logger is active and writing to a file, false otherwise.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			bool
			isLoggerEnabled () const noexcept
			{
				return m_logger != nullptr;
			}

			/**
			 * @brief Disables file logging and closes the log file.
			 *
			 * Stops the TracerLogger worker thread and destroys the logger instance,
			 * ensuring all pending entries are written before shutdown.
			 *
			 * @version 0.8.38
			 */
			void disableLogger () noexcept;

			/**
			 * @brief Creates a log entry with the specified severity and message.
			 *
			 * This is the primary logging method. It outputs to both the console (with color
			 * coding and optional filtering) and the log file if enabled. The method is
			 * thread-safe and respects all configured filters and settings.
			 *
			 * @param severity The severity level of the log message.
			 * @param tag A pointer to a C-string tag for categorization and filtering.
			 * @param message The log message content.
			 * @param location The source location where the log was generated (automatically captured).
			 * @version 0.8.38
			 */
			void trace (Severity severity, const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) const noexcept;

			/**
			 * @brief Creates a specialized log entry for tracking API function calls.
			 *
			 * This method is designed for logging external API calls (e.g., Vulkan, OpenGL, OpenAL).
			 * It formats the output to highlight the function name and optional details about the call.
			 *
			 * @param tag A pointer to a C-string identifying the API (e.g., "Vulkan", "OpenAL").
			 * @param functionName A pointer to a C-string with the API function name.
			 * @param message Optional additional information about the API call.
			 * @param location The source location where the API call was made (automatically captured).
			 * @version 0.8.38
			 */
			void traceAPI (const char * tag, const char * functionName, std::string_view message = {}, const std::source_location & location = std::source_location::current()) const noexcept;

			/**
			 * @brief Generates a log file path in the cache directory with appropriate extension.
			 *
			 * Creates a file path in the format: "cache_directory/journal-{name}.{ext}"
			 * where the extension depends on the configured log format (Text, JSON, or HTML).
			 *
			 * @param name The base name for the log file (typically the process name).
			 * @return The full path to the generated log file.
			 * @version 0.8.38
			 */
			[[nodiscard]]
			std::filesystem::path generateLogFilepath (const std::string & name) const noexcept;

			/**
			 * @brief Convenience method to create an Info-level log entry.
			 *
			 * @param tag A pointer to a C-string tag for categorization.
			 * @param message The log message content.
			 * @param location The source location (automatically captured).
			 * @version 0.8.38
			 */
			static
			void
			info (const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) noexcept
			{
				Tracer::getInstance().trace(Severity::Info, tag, message, location);
			}

			/**
			 * @brief Convenience method to create a Success-level log entry.
			 *
			 * @param tag A pointer to a C-string tag for categorization.
			 * @param message The log message content.
			 * @param location The source location (automatically captured).
			 * @version 0.8.38
			 */
			static
			void
			success (const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) noexcept
			{
				Tracer::getInstance().trace(Severity::Success, tag, message, location);
			}

			/**
			 * @brief Convenience method to create a Warning-level log entry.
			 *
			 * @param tag A pointer to a C-string tag for categorization.
			 * @param message The log message content.
			 * @param location The source location (automatically captured).
			 * @version 0.8.38
			 */
			static
			void
			warning (const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) noexcept
			{
				Tracer::getInstance().trace(Severity::Warning, tag, message, location);
			}

			/**
			 * @brief Convenience method to create an Error-level log entry.
			 *
			 * @param tag A pointer to a C-string tag for categorization.
			 * @param message The log message content.
			 * @param location The source location (automatically captured).
			 * @version 0.8.38
			 */
			static
			void
			error (const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) noexcept
			{
				Tracer::getInstance().trace(Severity::Error, tag, message, location);
			}

			/**
			 * @brief Convenience method to create a Fatal-level log entry.
			 *
			 * @param tag A pointer to a C-string tag for categorization.
			 * @param message The log message content.
			 * @param location The source location (automatically captured).
			 * @version 0.8.38
			 */
			static
			void
			fatal (const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) noexcept
			{
				Tracer::getInstance().trace(Severity::Fatal, tag, message, location);
			}

			/**
			 * @brief Convenience method to create a Debug-level log entry.
			 *
			 * This method is only active in Debug builds. In Release builds, it becomes
			 * a no-op and is eliminated by the compiler.
			 *
			 * @param tag A pointer to a C-string tag for categorization.
			 * @param message The log message content.
			 * @param location The source location (automatically captured).
			 * @version 0.8.38
			 */
			static
			void
			debug (const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) noexcept
			{
				if constexpr ( IsDebug )
				{
					Tracer::getInstance().trace(Severity::Debug, tag, message, location);
				}
			}

			/**
			 * @brief Convenience method to create an API call log entry.
			 *
			 * @param tag A pointer to a C-string identifying the API.
			 * @param functionName A pointer to a C-string with the API function name.
			 * @param message Optional additional information about the API call.
			 * @param location The source location (automatically captured).
			 * @version 0.8.38
			 */
			static
			void
			API (const char * tag, const char * functionName, std::string_view message = {}, const std::source_location & location = std::source_location::current())
			{
				Tracer::getInstance().traceAPI(tag, functionName, message, location);
			}

			/**
			 * @brief Callback function for GLFW error handling integration.
			 *
			 * This static method can be registered as a GLFW error callback to redirect
			 * GLFW error messages to the Tracer system.
			 *
			 * @param error The GLFW error code.
			 * @param description The error message from GLFW.
			 * @version 0.8.38
			 */
			static
			void
			traceGLFW (int error, const char * description) noexcept
			{
				Tracer::getInstance().trace(Severity::Error, "GLFW", (std::stringstream{} << description << " (errno:" << error << ')').str(), {});
			}

		private:

			struct PrivateToken {};

		public:

			/**
			 * @brief Private constructor accessible only through PrivateToken.
			 *
			 * This enables std::make_unique while preventing external instantiation.
			 *
			 * @version 0.8.38
			 */
			explicit Tracer (PrivateToken) noexcept;

		private:

			/**
			 * @brief Applies ANSI color codes to a message based on severity level.
			 *
			 * @param stream The string stream to write the colorized message to.
			 * @param severity The severity level (determines the color).
			 * @param message The message text to colorize.
			 * @version 0.8.38
			 */
			static void colorizeMessage (std::stringstream & stream, Severity severity,  std::string_view message) noexcept;

			/**
			 * @brief Appends process and thread identification information to a stream.
			 *
			 * Adds PPID, PID, and TID information to the log message for debugging
			 * multi-process and multi-threaded applications.
			 *
			 * @param stream The string stream to append the information to.
			 * @version 0.8.38
			 */
			void injectProcessInfo (std::stringstream & stream) const noexcept;

			/**
			 * @brief Checks if a tag passes the active filters.
			 *
			 * If no filters are active, all tags pass. If filters are active, only tags
			 * matching at least one filter pass.
			 *
			 * @param tag The tag to check against active filters.
			 * @return true if the tag should be displayed, false if it should be filtered out.
			 * @version 0.8.38
			 */
			bool filterTag (const char * tag) const noexcept;

			/* NOTE: Members ordered for optimal memory alignment (largest to smallest). */
			std::filesystem::path m_cacheDirectory;
			std::string m_processName;
			std::vector< std::string > m_filters;
			std::unique_ptr< TracerLogger > m_logger;
			mutable std::mutex m_consoleAccess;
			int m_parentProcessID{-1};
			int m_processID{-1};
			LogFormat m_logFormat{LogFormat::Text};
			bool m_serviceInitialized{false};
			bool m_isChildProcess{false};
			bool m_printOnlyErrors{false};
			bool m_sourceLocationEnabled{false};
			bool m_threadInfosEnabled{false};
			bool m_isTracerDisabled{false};
			bool m_loggerRequestedAtStartup{false};
	};

	/* ==================================================================================================================== */
	/* ================================================ Tracer utilities ================================================== */
	/* ==================================================================================================================== */

	/**
	 * @class T_TraceHelperBase
	 * @brief CRTP base class for RAII-based trace helper objects with stream-like interface.
	 *
	 * This template class uses the Curiously Recurring Template Pattern (CRTP) to provide
	 * zero-overhead, type-safe trace helpers for different severity levels. It inherits from
	 * BlobTrait to support stream-like message building via operator<<.
	 *
	 * The key feature is RAII (Resource Acquisition Is Initialization): the trace message is
	 * built during the object's lifetime and automatically sent to the Tracer when the object
	 * is destroyed. This enables convenient single-line logging with stream-style formatting:
	 *
	 * @code
	 * TraceInfo{"MyTag"} << "Value: " << 42 << ", Name: " << name;
	 * @endcode
	 *
	 * The severity level is encoded as a template parameter, allowing the compiler to optimize
	 * each instantiation specifically for its severity without runtime overhead.
	 *
	 * @tparam helper_t The derived class type (CRTP pattern, e.g., TraceInfo, TraceError).
	 * @tparam severity_t The compile-time severity level for this trace type.
	 *
	 * @note This class is non-copyable and non-movable to ensure proper RAII semantics.
	 * @see BlobTrait, TraceInfo, TraceError, TraceWarning, TraceSuccess, TraceFatal
	 * @version 0.8.38
	 */
	template< typename helper_t, Severity severity_t >
	class T_TraceHelperBase : public Libs::BlobTrait
	{
		public:

			/**
			 * @brief Constructs a trace helper object with a tag.
			 *
			 * Creates an empty trace message that can be built using operator<<.
			 *
			 * @param tag A pointer to a C-string tag for categorization.
			 * @param location The source location where the trace was created (automatically captured).
			 * @version 0.8.38
			 */
			explicit
			T_TraceHelperBase (const char * tag, const std::source_location & location = std::source_location::current()) noexcept
				: m_tag{tag},
				m_location{location}
			{

			}

			/**
			 * @brief Constructs a trace helper object with a tag and initial message.
			 *
			 * Creates a trace message with initial content. Additional content can be
			 * appended using operator<<.
			 *
			 * @param tag A pointer to a C-string tag for categorization.
			 * @param initialMessage The initial message content.
			 * @param location The source location where the trace was created (automatically captured).
			 * @version 0.8.38
			 */
			T_TraceHelperBase (const char * tag, std::string_view initialMessage, const std::source_location & location = std::source_location::current()) noexcept
				: BlobTrait{initialMessage},
				m_tag{tag},
				m_location{location}
			{

			}

			/** @brief Deleted copy constructor. */
			T_TraceHelperBase (const T_TraceHelperBase &) noexcept = delete;

			/** @brief Deleted move constructor. */
			T_TraceHelperBase (T_TraceHelperBase &&) noexcept = delete;

			/** @brief Deleted copy assignment. */
			T_TraceHelperBase & operator= (const T_TraceHelperBase &) noexcept = delete;

			/** @brief Deleted move assignment. */
			T_TraceHelperBase & operator= (T_TraceHelperBase &&) noexcept = delete;

			/**
			 * @brief Destructs the trace helper and sends the built message to the Tracer.
			 *
			 * This is where the actual logging occurs. The complete message built via
			 * operator<< is sent to the Tracer with the appropriate severity level.
			 *
			 * @version 0.8.38
			 */
			~T_TraceHelperBase ()
			{
				Tracer::getInstance().trace(severity_t, m_tag, this->get(), m_location);
			}

		protected:

			const char * m_tag;
			std::source_location m_location;
	};

#ifdef DEBUG
	/**
	 * @class TraceDebug
	 * @brief RAII trace helper for Debug-level log messages.
	 *
	 * In Debug builds, this class provides full logging functionality. Supports
	 * stream-style message building via operator<<.
	 *
	 * @code
	 * TraceDebug{"MyTag"} << "Debugging value: " << value;
	 * @endcode
	 *
	 * @note Only active in Debug builds. In Release builds, this becomes a zero-overhead no-op.
	 * @see T_TraceHelperBase
	 * @version 0.8.38
	 */
	class TraceDebug final : public T_TraceHelperBase< TraceDebug, Severity::Debug >
	{
		public:

			using T_TraceHelperBase::T_TraceHelperBase;
	};
#else
	/**
	 * @class TraceDebug
	 * @brief Zero-overhead dummy class for Debug traces in Release builds.
	 *
	 * This dummy implementation is designed to be completely eliminated by the compiler
	 * in Release builds, ensuring zero runtime overhead. All methods are inline no-ops.
	 *
	 * @note This is the Release build version. In Debug builds, full functionality is provided.
	 * @version 0.8.38
	 */
	class TraceDebug final
	{
		public:

			explicit TraceDebug (const char *, const std::source_location & = {}) noexcept {}

			TraceDebug (const char *, const char *, const std::source_location & = {}) noexcept {}

			TraceDebug (const char *, std::string_view, const std::source_location & = {}) noexcept {}

			TraceDebug (const TraceDebug &) noexcept = delete;

			TraceDebug (TraceDebug &&) noexcept = delete;

			TraceDebug & operator= (const TraceDebug &) noexcept = delete;

			TraceDebug & operator= (TraceDebug &&) noexcept = delete;

			~TraceDebug () = default;

			template< typename data_t >
			TraceDebug &
			operator<< (const data_t &) noexcept
			{
				return *this;
			}
	};
#endif

	/**
	 * @class TraceSuccess
	 * @brief RAII trace helper for Success-level log messages.
	 *
	 * Used to log successful operations, typically with green console coloring.
	 * Supports stream-style message building via operator<<.
	 *
	 * @code
	 * TraceSuccess{"MyTag"} << "Operation completed successfully!";
	 * @endcode
	 *
	 * @see T_TraceHelperBase
	 * @version 0.8.38
	 */
	class TraceSuccess final : public T_TraceHelperBase< TraceSuccess, Severity::Success >
	{
		public:

			using T_TraceHelperBase::T_TraceHelperBase;
	};

	/**
	 * @class TraceInfo
	 * @brief RAII trace helper for Info-level log messages.
	 *
	 * Used for general informational messages. This is the default severity level
	 * for routine logging. Supports stream-style message building via operator<<.
	 *
	 * @code
	 * TraceInfo{"MyTag"} << "System initialized with " << count << " components";
	 * @endcode
	 *
	 * @see T_TraceHelperBase
	 * @version 0.8.38
	 */
	class TraceInfo final : public T_TraceHelperBase< TraceInfo, Severity::Info >
	{
		public:

			using T_TraceHelperBase::T_TraceHelperBase;
	};

	/**
	 * @class TraceWarning
	 * @brief RAII trace helper for Warning-level log messages.
	 *
	 * Used to log potential issues or unexpected conditions that don't prevent execution
	 * but should be investigated. Typically displayed with magenta console coloring.
	 * Supports stream-style message building via operator<<.
	 *
	 * @code
	 * TraceWarning{"MyTag"} << "Deprecated feature used: " << featureName;
	 * @endcode
	 *
	 * @see T_TraceHelperBase
	 * @version 0.8.38
	 */
	class TraceWarning final : public T_TraceHelperBase< TraceWarning, Severity::Warning >
	{
		public:

			using T_TraceHelperBase::T_TraceHelperBase;
	};

	/**
	 * @class TraceError
	 * @brief RAII trace helper for Error-level log messages.
	 *
	 * Used to log errors that affect functionality but allow the application to continue.
	 * Typically displayed with red console coloring and always shown even in errors-only mode.
	 * Supports stream-style message building via operator<<.
	 *
	 * @code
	 * TraceError{"MyTag"} << "Failed to load resource: " << resourcePath;
	 * @endcode
	 *
	 * @see T_TraceHelperBase
	 * @version 0.8.38
	 */
	class TraceError final : public T_TraceHelperBase< TraceError, Severity::Error >
	{
		public:

			using T_TraceHelperBase::T_TraceHelperBase;
	};

	/**
	 * @class TraceFatal
	 * @brief RAII trace helper for Fatal-level log messages with optional program termination.
	 *
	 * Used to log critical errors that may require immediate application termination.
	 * This class does NOT inherit from T_TraceHelperBase because it has a unique behavior:
	 * it can optionally call std::terminate() after logging.
	 *
	 * Typically displayed with red background console coloring. Supports stream-style
	 * message building via operator<< (inherited from BlobTrait).
	 *
	 * @code
	 * // Log fatal error but continue execution
	 * TraceFatal{"MyTag", false} << "Critical failure in subsystem";
	 *
	 * // Log fatal error and terminate
	 * TraceFatal{"MyTag", true} << "Unrecoverable error, terminating";
	 * @endcode
	 *
	 * @note This class is non-copyable and non-movable to ensure proper RAII semantics.
	 * @see BlobTrait, Tracer
	 * @version 0.8.38
	 */
	class TraceFatal final : public Libs::BlobTrait
	{
		public:

			/**
			 * @brief Constructs a fatal trace helper with optional termination.
			 *
			 * Creates an empty trace message that can be built using operator<<.
			 *
			 * @param tag A pointer to a C-string tag for categorization.
			 * @param terminate If true, calls std::terminate() after logging. Defaults to false.
			 * @param location The source location where the trace was created (automatically captured).
			 * @version 0.8.38
			 */
			explicit
			TraceFatal (const char * tag, bool terminate = false, const std::source_location & location = std::source_location::current()) noexcept
				: m_tag{tag},
				m_location{location},
				m_terminate{terminate}
			{

			}

			/**
			 * @brief Constructs a fatal trace helper with initial message and optional termination.
			 *
			 * Creates a trace message with initial content. Additional content can be
			 * appended using operator<<.
			 *
			 * @param tag A pointer to a C-string tag for categorization.
			 * @param initialMessage The initial message content.
			 * @param terminate If true, calls std::terminate() after logging. Defaults to false.
			 * @param location The source location where the trace was created (automatically captured).
			 * @version 0.8.38
			 */
			TraceFatal (const char * tag, std::string_view initialMessage, bool terminate = false, const std::source_location & location = std::source_location::current()) noexcept
				: BlobTrait{initialMessage},
				m_tag{tag},
				m_location{location},
				m_terminate{terminate}
			{

			}

			/** @brief Deleted copy constructor. */
			TraceFatal (const TraceFatal &) noexcept = delete;

			/** @brief Deleted move constructor. */
			TraceFatal (TraceFatal &&) noexcept = delete;

			/** @brief Deleted copy assignment. */
			TraceFatal & operator= (const TraceFatal &) noexcept = delete;

			/** @brief Deleted move assignment. */
			TraceFatal & operator= (TraceFatal &&) noexcept = delete;

			/**
			 * @brief Destructs the trace helper, logs the message, and optionally terminates.
			 *
			 * Sends the complete message to the Tracer with Fatal severity. If the terminate
			 * flag was set to true in the constructor, calls std::terminate() after logging.
			 *
			 * @version 0.8.38
			 */
			~TraceFatal ()
			{
				Tracer::getInstance().trace(Severity::Fatal, m_tag, this->get(), m_location);

				if ( m_terminate )
				{
					std::terminate();
				}
			}

		private:

			const char * m_tag;
			std::source_location m_location;
			bool m_terminate;
	};

	/**
	 * @class TraceAPI
	 * @brief RAII trace helper for tracking external API function calls.
	 *
	 * This specialized helper is designed for logging calls to external APIs (e.g., Vulkan,
	 * OpenGL, OpenAL, etc.). It formats the output to emphasize the API function name and
	 * can optionally terminate the program after logging.
	 *
	 * The logged message is formatted to highlight the function name in yellow with special
	 * formatting. Supports stream-style message building via operator<< (inherited from BlobTrait).
	 *
	 * @code
	 * // Simple API call tracking
	 * TraceAPI{"Vulkan", "vkCreateInstance"};
	 *
	 * // API call with additional information
	 * TraceAPI{"OpenAL", "alGenBuffers"} << "Generating " << count << " buffers";
	 * @endcode
	 *
	 * @note This class is non-copyable and non-movable to ensure proper RAII semantics.
	 * @see BlobTrait, Tracer
	 * @version 0.8.38
	 */
	class TraceAPI final : public Libs::BlobTrait
	{
		public:

			/**
			 * @brief Constructs an API trace helper for a function call.
			 *
			 * Creates an empty trace message that can be built using operator<<.
			 *
			 * @param tag A pointer to a C-string identifying the API (e.g., "Vulkan", "OpenAL").
			 * @param functionName A pointer to a C-string with the API function name.
			 * @param terminate If true, calls std::terminate() after logging. Defaults to false.
			 * @param location The source location where the trace was created (automatically captured).
			 * @version 0.8.38
			 */
			explicit
			TraceAPI (const char * tag, const char * functionName, bool terminate = false, const std::source_location & location = std::source_location::current()) noexcept
				: m_tag{tag},
				m_functionName{functionName},
				m_location{location},
				m_terminate{terminate}
			{

			}

			/**
			 * @brief Constructs an API trace helper with initial message.
			 *
			 * Creates a trace message with initial content. Additional content can be
			 * appended using operator<<.
			 *
			 * @param tag A pointer to a C-string identifying the API (e.g., "Vulkan", "OpenAL").
			 * @param functionName A pointer to a C-string with the API function name.
			 * @param initialMessage The initial message content.
			 * @param terminate If true, calls std::terminate() after logging. Defaults to false.
			 * @param location The source location where the trace was created (automatically captured).
			 * @version 0.8.38
			 */
			TraceAPI (const char * tag, const char * functionName, std::string_view initialMessage, bool terminate = false, const std::source_location & location = std::source_location::current()) noexcept
				: BlobTrait{initialMessage},
				m_tag{tag},
				m_functionName{functionName},
				m_location{location},
				m_terminate{terminate}
			{

			}

			/** @brief Deleted copy constructor. */
			TraceAPI (const TraceAPI &) noexcept = delete;

			/** @brief Deleted move constructor. */
			TraceAPI (TraceAPI &&) noexcept = delete;

			/** @brief Deleted copy assignment. */
			TraceAPI & operator= (const TraceAPI &) noexcept = delete;

			/** @brief Deleted move assignment. */
			TraceAPI & operator= (TraceAPI &&) noexcept = delete;

			/**
			 * @brief Destructs the trace helper, logs the API call, and optionally terminates.
			 *
			 * Sends the complete message to the Tracer's traceAPI() method with special
			 * formatting for API calls. If the terminate flag was set to true in the
			 * constructor, calls std::terminate() after logging.
			 *
			 * @version 0.8.38
			 */
			~TraceAPI ()
			{
				Tracer::getInstance().traceAPI(m_tag, m_functionName, this->get(), m_location);

				if ( m_terminate )
				{
					std::terminate();
				}
			}

		private:

			const char * m_tag;
			const char * m_functionName;
			std::source_location m_location;
			bool m_terminate;
	};
}
