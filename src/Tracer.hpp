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
	 * @brief A single entry for the tracer.
	 */
	class TracerEntry final
	{
		public:

			/**
			 * @brief Constructs a tracer entry.
			 * @param severity The severity of the message.
			 * @param tag The tag to sort and/or filter entries.
			 * @param message The content of the entry [std::move].
			 * @param location The location of the message in the code source.
			 * @param threadId The thread ID.
			 */
			TracerEntry (Severity severity, const char * tag, std::string message, const std::source_location & location, const std::thread::id & threadId) noexcept
				: m_severity{severity},
				m_tag{tag},
				m_message{std::move(message)},
				m_location{location},
				m_threadId{threadId}
			{

			}

			/**
			 * @brief Returns the time of the message.
			 * @return const time_point< steady_clock > &
			 */
			[[nodiscard]]
			const std::chrono::time_point< std::chrono::steady_clock > &
			time () const noexcept
			{
				return m_time;
			}

			/**
			 * @brief Returns the severity of the message.
			 * @return Severity
			 */
			[[nodiscard]]
			Severity
			severity () const noexcept
			{
				return m_severity;
			}

			/**
			 * @brief Returns the tag of the entry.
			 * @return const char *
			 */
			[[nodiscard]]
			const char *
			tag () const noexcept
			{
				return m_tag;
			}

			/**
			 * @brief Returns the message.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			message () const noexcept
			{
				return m_message;
			}

			/**
			 * @brief Returns the location where the entry comes from.
			 * @return const std::source_location &
			 */
			[[nodiscard]]
			const std::source_location &
			location () const noexcept
			{
				return m_location;
			}

			/**
			 * @brief Returns the thread ID where the entry was generated.
			 * @return const std::thread::id &
			 */
			[[nodiscard]]
			const std::thread::id &
			threadId () const noexcept
			{
				return m_threadId;
			}

		private:

			std::chrono::time_point< std::chrono::steady_clock > m_time{std::chrono::steady_clock::now()};
			Severity m_severity{Severity::Info};
			const char * m_tag{nullptr};
			std::string m_message;
			std::source_location m_location;
			std::thread::id m_threadId;
	};

	/**
	 * @brief The tracer logger class.
	 */
	class TracerLogger final
	{
		public:

			/**
			 * @brief Constructs the trace logger.
			 * @param filepath A reference to a path to the log file [std::move].
			 * @param logFormat The type of log desired. Default Text.
			 */
			explicit TracerLogger (std::filesystem::path filepath, LogFormat logFormat = LogFormat::Text) noexcept;

			/**
			 * @brief Destructs the trace logger.
			 */
			~TracerLogger ();

			/**
			 * @brief Creates a log.
			 * @param severity The type of log.
			 * @param tag A pointer to a C-string to describe a tag. This helps for sorting logs.
			 * @param message A reference to a string for the log content.
			 * @param location A reference to a source_location.
			 * @return void
			 */
			void
			push (Severity severity, const char * tag, std::string message, const std::source_location & location) noexcept
			{
				{
					/* NOTE: Lock between the writing logs task in a file and the push/pop method. */
					const std::lock_guard< std::mutex > lock{m_entriesAccess};

					m_entries.emplace(severity, tag, std::move(message), location, std::this_thread::get_id());
				}

				/* NOTE: wake up the thread. */
				m_condition.notify_one();
			}

			bool
			start () noexcept
			{
				if ( !m_isUsable || m_isRunning )
				{
					if constexpr ( IsDebug )
					{
						std::cerr << "TraceLogger::start() : Unable to enable the tracer logger !" "\n";
					}

					return false;
				}

				m_isRunning = true;

				m_thread = std::thread{&TracerLogger::task, this};

				return true;
			}

			/**
			 * @brief Stops the writing task for shutting down the tracer service.
			 * @return void
			 */
			void
			stop () noexcept
			{
				m_isRunning = false;

				m_condition.notify_one();
			}

			/**
			 * @brief Clears pending entries.
			 * @return void
			 */
			void
			clear () noexcept
			{
				const std::lock_guard< std::mutex > lock{m_entriesAccess};

				std::queue< TracerEntry > emptyQueue;

				m_entries.swap(emptyQueue);
			}

		private:

			/**
			 * @brief Runs the task of writing entries to the log file.
			 * @return void
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
	 * @brief The tracer class is responsible for logging messages, errors, warnings, etc. to the terminal or in a log file.
	 */
	class Tracer final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"Tracer"};

			/* ANSI Escape Codes */
			static constexpr auto CSI{"\033["};

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Tracer (const Tracer & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Tracer (Tracer && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Tracer &
			 */
			Tracer & operator= (const Tracer & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Tracer &
			 */
			Tracer & operator= (Tracer && copy) noexcept = delete;

			/**
			 * @brief Destructs the tracer.
			 */
			~Tracer ()
			{
				this->disableLogger();

				if constexpr ( IsDebug )
				{
					std::cout << "Tracer instance destroyed!" << std::endl;
				}
			}

			/**
			 * @brief Returns the instance of tracer.
			 * @return Tracer &
			 */
			[[nodiscard]]
			static
			Tracer &
			getInstance () noexcept
			{
				static auto instance = std::make_unique< Tracer >(PrivateToken{});

				return *instance;
			}

			/**
			 * @brief Sets up the tracer for a specific process.
			 * @param arguments A reference to the arguments.
			 * @param processName A string to identify the instance with multi-processes application [std::move].
			 * @param childProcess Declares a child process.
			 * @return void
			 */
			void earlySetup (const Arguments & arguments, std::string processName, bool childProcess) noexcept;

			/**
			 * @brief Sets up the tracer.
			 * @warning This can't be done at startup because the Tracer service is the first to be set up.
			 * @param arguments A reference to the arguments.
			 * @param fileSystem A reference to the filesystem service.
			 * @param settings A reference to the settings service.
			 * @return void
			 */
			void lateSetup (const Arguments & arguments, const FileSystem & fileSystem, Settings & settings) noexcept;

			/**
			 * @brief Returns the process name.
			 * @note This is useful for multi-processes application.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			processName () const noexcept
			{
				return m_processName;
			}

			/**
			 * @brief Adds a term to only print out a trace message containing it.
			 * @note This only affects the standard console output.
			 * @param filter The term to filter.
			 * @return void
			 */
			void
			addTagFilter (std::string filter)
			{
				m_filters.emplace_back(std::move(filter));
			}

			/**
			 * @brief Clears all filters.
			 * @return void
			 */
			void
			removeAllTagFilters () noexcept
			{
				m_filters.clear();
			}

			/**
			 * @brief Enables only the errors to be print in the standard console. By default, this option is disabled.
			 * @note This won't affect the log file.
			 * @param state The state.
			 * @return void
			 */
			void
			enablePrintOnlyErrors (bool state) noexcept
			{
				m_printOnlyErrors = state;
			}

			/**
			 * @brief Returns whether only errors are print in the standard console.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			printOnlyErrors () const noexcept
			{
				return m_printOnlyErrors;
			}

			/**
			 * @brief Enables the location "[file:number]" section in the standard console. By default, this option is enabled.
			 * @note This won't affect the log file.
			 * @param state The state.
			 * @return void
			 */
			void
			enableSourceLocation (bool state) noexcept
			{
				m_sourceLocationEnabled = state;
			}

			/**
			 * @brief Returns it the location in the standard console is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSourceLocationEnabled () const noexcept
			{
				return m_sourceLocationEnabled;
			}

			/**
			 * @brief Enables the thread information section in the standard console. By default, this option is enabled.
			 * @note This won't affect the log file.
			 * @param state The state.
			 * @return void
			 */
			void
			enableThreadInfos (bool state) noexcept
			{
				m_threadInfosEnabled = state;
			}

			/**
			 * @brief Returns whether the thread information in the standard console is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isThreadInfosEnabled () const noexcept
			{
				return m_threadInfosEnabled;
			}

			/**
			 * @brief Disables the tracer.
			 * @param state The state.
			 * @return void
			 */
			void
			disableTracer (bool state) noexcept
			{
				m_isTracerDisabled = state;
			}

			/**
			 * @brief Returns whether the tracer is disabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTracerDisabled () const noexcept
			{
				return m_isTracerDisabled;
			}

			/**
			 * @brief Returns whether the tracer is requested to enable logger at startup.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLoggerRequestedAtStartup () const noexcept
			{
				return m_loggerRequestedAtStartup;
			}

			/**
			 * @brief Prepares the logger to write into a logfile.
			 * @param filepath A reference to a path.
			 * @return bool
			 */
			bool enableLogger (const std::filesystem::path & filepath) noexcept;

			/**
			 * @brief Returns whether the tracer is writing into the logfile.
			 * @note Returns always false if the logger is disabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLoggerEnabled () const noexcept
			{
				return m_logger != nullptr;
			}

			/**
			 * @brief Removes the logger and closes the logfile.
			 * @return void
			 */
			void disableLogger () noexcept;

			/**
			 * @brief Creates a log.
			 * @param severity The type of log.
			 * @param tag A pointer on a c-string to identify and sort logs.
			 * @param message A string view.
			 * @param location A reference to a source_location. Default constructed in place.
			 * @return void
			 */
			void trace (Severity severity, const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) const noexcept;

			/**
			 * @brief Creates a log for a specific API.
			 * @param tag A pointer on a c-string to identify and sort logs.
			 * @param functionName A pointer on a c-string for the API function.
			 * @param message A string view. Default none.
			 * @param location A reference to a source_location. Default constructed in place.
			 * @return void
			 */
			void traceAPI (const char * tag, const char * functionName, std::string_view message = {}, const std::source_location & location = std::source_location::current()) const noexcept;

			/**
			 * @brief Generates the name of a log file based on a name.
			 * @param name A reference to a string.
			 * @return std::filesystem::path
			 */
			[[nodiscard]]
			std::filesystem::path generateLogFilepath (const std::string & name) const noexcept;

			/**
			 * @brief Creates a quick log with Info as severity.
			 * @param tag A pointer on a c-string to identify and sort logs.
			 * @param message A string view.
			 * @param location A reference to a source_location. Default constructed in place.
			 * @return void
			 */
			static
			void
			info (const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) noexcept
			{
				Tracer::getInstance().trace(Severity::Info, tag, message, location);
			}

			/**
			 * @brief Creates a quick log with Success as severity.
			 * @param tag A pointer on a c-string to identify and sort logs.
			 * @param message A string view.
			 * @param location A reference to a source_location. Default constructed in place.
			 * @return void
			 */
			static
			void
			success (const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) noexcept
			{
				Tracer::getInstance().trace(Severity::Success, tag, message, location);
			}

			/**
			 * @brief Creates a quick log with Warning as severity.
			 * @param tag A pointer on a c-string to identify and sort logs.
			 * @param message A string view.
			 * @param location A reference to a source_location. Default constructed in place.
			 * @return void
			 */
			static
			void
			warning (const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) noexcept
			{
				Tracer::getInstance().trace(Severity::Warning, tag, message, location);
			}

			/**
			 * @brief Creates a quick log with Error as severity.
			 * @param tag A pointer on a c-string to identify and sort logs.
			 * @param message A string view.
			 * @param location A reference to a source_location. Default constructed in place.
			 * @return void
			 */
			static
			void
			error (const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) noexcept
			{
				Tracer::getInstance().trace(Severity::Error, tag, message, location);
			}

			/**
			 * @brief Creates a quick log with Fatal as severity.
			 * @param tag A pointer on a c-string to identify and sort logs.
			 * @param message A string view.
			 * @param location A reference to a source_location. Default constructed in place.
			 * @return void
			 */
			static
			void
			fatal (const char * tag, std::string_view message, const std::source_location & location = std::source_location::current()) noexcept
			{
				Tracer::getInstance().trace(Severity::Fatal, tag, message, location);
			}

			/**
			 * @brief Creates a quick log with Debug as severity.
			 * @param tag A pointer on a c-string to identify and sort logs.
			 * @param message A string view.
			 * @param location A reference to a source_location. Default constructed in place.
			 * @return void
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
			 * @brief Creates a quick log for API usage.
			 * @param tag A pointer on a c-string to identify and sort logs.
			 * @param functionName A pointer on a c-string for the API function.
			 * @param message A string view. Default none.
			 * @param location A reference to a source_location. Default constructed in place.
			 * @return void
			 */
			static
			void
			API (const char * tag, const char * functionName, std::string_view message = {}, const std::source_location & location = std::source_location::current())
			{
				Tracer::getInstance().traceAPI(tag, functionName, message, location);
			}

			/**
			 * @brief Converts GLFW log facility to trace()
			 * @param error The error number from the GLFW library.
			 * @param description The message associated with the error code.
			 * @return void
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

			/** @brief Constructor for std::make_unique(). */
			explicit
			Tracer (PrivateToken) noexcept
			{
				if constexpr ( IsDebug )
				{
					std::cout << "Tracer constructed!" << std::endl;
				}
			}

		private:

			/**
			 * @brief Colorizes a message from the severity type.
			 * @param stream A reference to a stream.
			 * @param severity The severity.
			 * @param message A string view.
			 * @return void
			 */
			static
			void
			colorizeMessage (std::stringstream & stream, Severity severity,  std::string_view message) noexcept
			{
				switch ( severity )
				{
					case Severity::Debug :
						stream << " \033[1;36m" << message << "\033[0m ";
						break;

					case Severity::Success :
						stream << " \033[1;92m" << message << " \033[0m ";
						break;

					case Severity::Warning :
						stream << " \033[1;35m" << message << " \033[0m ";
						break;

					case Severity::Error :
						stream << " \033[1;91m" << message << " \033[0m ";
						break;

					case Severity::Fatal :
						stream << " \033[1;41m" << message << " \033[0m ";
						break;

					case Severity::Info :
					default :
						stream << ' ' << message << ' ';
						break;
				}
			}

			/**
			 * @brief Injects process and thread info.
			 * @param stream A reference to a stream.
			 * @return void
			 */
			void injectProcessInfo (std::stringstream & stream) const noexcept;

			/**
			 * @brief Filters the current tag. If the method returns true, the message with the tag is allowed to be displayed.
			 * @param tag The tag to filter.
			 * @return bool
			 */
			bool filterTag (const char * tag) const noexcept;

			std::filesystem::path m_cacheDirectory;
			std::string m_processName;
			std::vector< std::string > m_filters;
			std::unique_ptr< TracerLogger > m_logger;
			LogFormat m_logFormat{LogFormat::Text};
			int m_parentProcessID{-1};
			int m_processID{-1};
			mutable std::mutex m_consoleAccess;
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

#ifdef DEBUG
	/**
	 * @brief This utils class creates a debug trace object.
	 * @extends EmEn::Libs::BlobTrait
	 */
	class TraceDebug final : public Libs::BlobTrait
	{
		public:

			/**
			 * @brief Constructs a debug trace.
			 * @param tag A pointer to a C-string for the tag.
			 * @param location A reference to a source location structure.
			 */
			explicit
			TraceDebug (const char * tag, const std::source_location & location = std::source_location::current()) noexcept
				: m_tag(tag),
				m_location(location)
			{

			}

			/**
			 * @brief Constructs a debug trace with an initial message.
			 * @param tag A pointer to a C-string for the tag.
			 * @param initialMessage A string view.
			 * @param location A reference to a source location structure.
			 */
			TraceDebug (const char * tag, const std::string & initialMessage, const std::source_location & location = std::source_location::current()) noexcept
				: BlobTrait(initialMessage),
				m_tag(tag),
				m_location(location)
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceDebug (const TraceDebug & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceDebug (TraceDebug && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			TraceDebug & operator= (const TraceDebug & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			TraceDebug & operator= (TraceDebug && copy) noexcept = delete;

			/**
			 * @brief Destructs the trace and calls the real tracer service.
			 */
			~TraceDebug ()
			{
				Tracer::getInstance().trace(Severity::Debug, m_tag, this->get(), m_location);
			}

		private:

			const char * m_tag;
			std::source_location m_location;
	};
#else
	/**
	 * @brief This utils class creates a debug trace object.
	 * @note This dummy version is intended to be wiped out from the code at compile-time.
	 */
	class TraceDebug final
	{
		public:

			explicit TraceDebug (const char *, const std::source_location & = {}) noexcept {}

			TraceDebug (const char *, const char *, const std::source_location & = {}) noexcept {}

			TraceDebug (const char *, const std::string &, const std::source_location & = {}) noexcept {}

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
	 * @brief This utils class creates a success trace object.
	 * @extends EmEn::Libs::BlobTrait
	 */
	class TraceSuccess final : public Libs::BlobTrait
	{
		public:

			/**
			 * @brief Constructs a success trace.
			 * @param tag A pointer to a C-string for the tag.
			 * @param location A reference to a source location structure.
			 */
			explicit
			TraceSuccess (const char * tag, const std::source_location & location = std::source_location::current()) noexcept
				: m_tag(tag),
				m_location(location)
			{

			}

			/**
			 * @brief Constructs a success trace with an initial message.
			 * @param tag A pointer to a C-string for the tag.
			 * @param initialMessage A string view.
			 * @param location A reference to a source location structure.
			 */
			TraceSuccess (const char * tag, const std::string & initialMessage, const std::source_location & location = std::source_location::current()) noexcept
				: BlobTrait(initialMessage),
				m_tag(tag),
				m_location(location)
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceSuccess (const TraceSuccess & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceSuccess (TraceSuccess && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			TraceSuccess & operator= (const TraceSuccess & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			TraceSuccess & operator= (TraceSuccess && copy) noexcept = delete;

			/**
			 * @brief Destructs the trace and calls the real tracer service.
			 */
			~TraceSuccess ()
			{
				Tracer::getInstance().trace(Severity::Success, m_tag, this->get(), m_location);
			}


		private:

			const char * m_tag;
			std::source_location m_location;
	};

	/**
	 * @brief This utils class creates an info trace object.
	 * @extends EmEn::Libs::BlobTrait
	 */
	class TraceInfo final : public Libs::BlobTrait
	{
		public:

			/**
			 * @brief Constructs an info trace.
			 * @param tag A pointer to a C-string for the tag.
			 * @param location A reference to a source location structure.
			 */
			explicit
			TraceInfo (const char * tag, const std::source_location & location = std::source_location::current()) noexcept
				: m_tag(tag),
				m_location(location)
			{

			}

			/**
			 * @brief Constructs an info trace with an initial message.
			 * @param tag A pointer to a C-string for the tag.
			 * @param initialMessage A string view.
			 * @param location A reference to a source location structure.
			 */
			TraceInfo (const char * tag, const std::string & initialMessage, const std::source_location & location = std::source_location::current()) noexcept
				: BlobTrait(initialMessage),
				m_tag(tag),
				m_location(location)
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceInfo (const TraceInfo & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceInfo (TraceInfo && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			TraceInfo & operator= (const TraceInfo & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			TraceInfo & operator= (TraceInfo && copy) noexcept = delete;

			/**
			 * @brief Destructs the trace and calls the real tracer service.
			 */
			~TraceInfo ()
			{
				Tracer::getInstance().trace(Severity::Info, m_tag, this->get(), m_location);
			}

		private:

			const char * m_tag;
			std::source_location m_location;
	};

	/**
	 * @brief This utils class creates a warning trace object.
	 * @extends EmEn::Libs::BlobTrait
	 */
	class TraceWarning final : public Libs::BlobTrait
	{
		public:

			/**
			 * @brief Constructs a warning trace.
			 * @param tag A pointer to a C-string for the tag.
			 * @param location A reference to a source location structure.
			 */
			explicit
			TraceWarning (const char * tag, const std::source_location & location = std::source_location::current()) noexcept
				: m_tag(tag),
				m_location(location)
			{

			}

			/**
			 * @brief Constructs a warning trace with an initial message.
			 * @param tag A pointer to a C-string for the tag.
			 * @param initialMessage A string view.
			 * @param location A reference to a source location structure.
			 */
			TraceWarning (const char * tag, const std::string & initialMessage, const std::source_location & location = std::source_location::current()) noexcept
				: BlobTrait(initialMessage),
				m_tag(tag),
				m_location(location)
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceWarning (const TraceWarning & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceWarning (TraceWarning && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			TraceWarning & operator= (const TraceWarning & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			TraceWarning & operator= (TraceWarning && copy) noexcept = delete;

			/**
			 * @brief Destructs the trace and calls the real tracer service.
			 */
			~TraceWarning ()
			{
				Tracer::getInstance().trace(Severity::Warning, m_tag, this->get(), m_location);
			}

		private:

			const char * m_tag;
			std::source_location m_location;
	};

	/**
	 * @brief TThis utils class create an error trace object.
	 * @extends EmEn::Libs::BlobTrait
	 */
	class TraceError final : public Libs::BlobTrait
	{
		public:

			/**
			 * @brief Constructs an error trace.
			 * @param tag A pointer to a C-string for the tag.
			 * @param location A reference to a source location structure.
			 */
			explicit
			TraceError (const char * tag, const std::source_location & location = std::source_location::current()) noexcept
				: m_tag(tag),
				m_location(location)
			{

			}

			/**
			 * @brief Constructs an error trace with an initial message.
			 * @param tag A pointer to a C-string for the tag.
			 * @param initialMessage A string view.
			 * @param location A reference to a source location structure.
			 */
			TraceError (const char * tag, const std::string & initialMessage, const std::source_location & location = std::source_location::current()) noexcept
				: BlobTrait(initialMessage),
				m_tag(tag),
				m_location(location)
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceError (const TraceError & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceError (TraceError && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 */
			TraceError & operator= (const TraceError & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			TraceError & operator= (TraceError && copy) noexcept = delete;

			/**
			 * @brief Destructs the trace and calls the real tracer service.
			 */
			~TraceError ()
			{
				Tracer::getInstance().trace(Severity::Error, m_tag, this->get(), m_location);
			}

		private:

			const char * m_tag;
			std::source_location m_location;
	};

	/**
	 * @brief This utils class creates a fatal trace object.
	 * @extends EmEn::Libs::BlobTrait
	 */
	class TraceFatal final : public Libs::BlobTrait
	{
		public:

			/**
			 * @brief Constructs a fatal trace.
			 * @param tag A pointer to a C-string for the tag.
			 * @param terminate Call std::terminate() at this object destructor. Default false.
			 * @param location A reference to a source location structure.
			 */
			explicit
			TraceFatal (const char * tag, bool terminate = false, const std::source_location & location = std::source_location::current()) noexcept
				: m_tag(tag),
				m_location(location),
				m_terminate(terminate)
			{

			}

			/**
			 * @brief Constructs a fatal trace with an initial message.
			 * @param tag A pointer to a C-string for the tag.
			 * @param initialMessage A string view.
			 * @param terminate Call std::terminate() at this object destructor. Default false.
			 * @param location A reference to a source location structure.
			 */
			TraceFatal (const char * tag, const std::string & initialMessage, bool terminate = false, const std::source_location & location = std::source_location::current()) noexcept
				: BlobTrait(initialMessage),
				m_tag(tag),
				m_location(location),
				m_terminate(terminate)
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceFatal (const TraceFatal & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceFatal (TraceFatal && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return TraceFatal &
			 */
			TraceFatal & operator= (const TraceFatal & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return TraceFatal &
			 */
			TraceFatal & operator= (TraceFatal && copy) noexcept = delete;

			/**
			 * @brief Destructs the trace and calls the real tracer service.
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
	 * @brief This utils class creates an API trace object.
	 * @extends EmEn::Libs::BlobTrait
	 */
	class TraceAPI final : public Libs::BlobTrait
	{
		public:

			/**
			 * @brief Constructs an API trace.
			 * @param tag A pointer to a C-string for the tag.
			 * @param functionName A pointer on a c-string for the API function.
			 * @param terminate Call std::terminate() at this object destructor. Default false.
			 * @param location A reference to a source location structure.
			 */
			explicit
			TraceAPI (const char * tag, const char * functionName, bool terminate = false, const std::source_location & location = std::source_location::current()) noexcept
				: m_tag(tag),
				m_functionName(functionName),
				m_location(location),
				m_terminate(terminate)
			{

			}

			/**
			 * @brief Constructs a fatal trace with an initial message.
			 * @param tag A pointer to a C-string for the tag.
			 * @param functionName A pointer on a c-string for the API function.
			 * @param initialMessage A string view.
			 * @param terminate Call std::terminate() at this object destructor. Default false.
			 * @param location A reference to a source location structure.
			 */
			TraceAPI (const char * tag, const char * functionName, const std::string & initialMessage, bool terminate = false, const std::source_location & location = std::source_location::current()) noexcept
				: BlobTrait(initialMessage),
				m_tag(tag),
				m_functionName(functionName),
				m_location(location),
				m_terminate(terminate)
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceAPI (const TraceAPI & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			TraceAPI (TraceAPI && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return TraceFatal &
			 */
			TraceAPI & operator= (const TraceAPI & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return TraceFatal &
			 */
			TraceAPI & operator= (TraceAPI && copy) noexcept = delete;

			/**
			 * @brief Destructs the trace and calls the real tracer service.
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
