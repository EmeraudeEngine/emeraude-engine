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

/* STL inclusions. */
#include <cstddef>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <array>
#include <vector>
#include "Libs/std_source_location.hpp"

/* Local inclusions for inheritances. */
#include "ServiceInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/BlobTrait.hpp"
#include "TracerLogger.hpp"
#include "Types.hpp"

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
	 * @brief The tracer service class responsible for logging messages, errors, warnings, etc. to the terminal or in a log file.
	 * @extends EmEn::ServiceInterface This is a service.
	 */
	class Tracer final : public ServiceInterface
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"TracerService"};

			/** @brief Observable class unique identifier. */
			static const size_t ClassUID;

			/* ANSI Escape Codes */
			static constexpr auto CSI{"\033["};

			enum class ANSIColorCode: uint8_t
			{
				Black = 30,
				Red = 31,
				Green = 32,
				Yellow = 33,
				Blue = 34,
				Magenta = 35,
				Cyan = 36,
				White = 37,
				BrightBlack = 90,
				BrightRed = 91,
				BrightGreen = 92,
				BrightYellow = 93,
				BrightBlue = 94,
				BrightMagenta = 95,
				BrightCyan = 96,
				BrightWhite = 97
			};

			/**
			 * @brief Constructs the tracer.
			 * @param arguments A reference to the argument service.
			 * @param processName A string to identify the instance with multi-processes application [std::move].
			 * @param childProcess Declares a child process.
			 */
			Tracer (const Arguments & arguments, std::string processName, bool childProcess) noexcept;

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
			 */
			Tracer & operator= (const Tracer & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 */
			Tracer & operator= (Tracer && copy) noexcept = delete;

			/**
			 * @brief Destructs the tracer.
			 */
			~Tracer () override;

			/** @copydoc EmEn::Libs::ObservableTrait::classUID() const */
			[[nodiscard]]
			size_t
			classUID () const noexcept override
			{
				return ClassUID;
			}

			/** @copydoc EmEn::Libs::ObservableTrait::is() const */
			[[nodiscard]]
			bool
			is (size_t classUID) const noexcept override
			{
				return classUID == ClassUID;
			}

			/** @copydoc EmEn::ServiceInterface::usable() */
			[[nodiscard]]
			bool
			usable () const noexcept override
			{
				return m_flags[ServiceInitialized];
			}

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
				m_flags[PrintOnlyErrors] = state;
			}

			/**
			 * @brief Returns whether only errors are print in the standard console.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			printOnlyErrors () const noexcept
			{
				return m_flags[PrintOnlyErrors];
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
				m_flags[SourceLocationEnabled] = state;
			}

			/**
			 * @brief Returns it the location in the standard console is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSourceLocationEnabled () const noexcept
			{
				return m_flags[SourceLocationEnabled];
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
				m_flags[ThreadInfosEnabled] = state;
			}

			/**
			 * @brief Returns whether the thread information in the standard console is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isThreadInfosEnabled () const noexcept
			{
				return m_flags[ThreadInfosEnabled];
			}

			/**
			 * @brief Disables the tracer.
			 * @param state The state.
			 * @return void
			 */
			void
			disableTracer (bool state) noexcept
			{
				m_flags[IsTracerDisabled] = state;
			}

			/**
			 * @brief Returns whether the tracer is disabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isTracerDisabled () const noexcept
			{
				return m_flags[IsTracerDisabled];
			}

			/**
			 * @brief Returns whether the tracer is requested to enable logger at startup.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLoggerRequestedAtStartup () const noexcept
			{
				return m_flags[LoggerRequestedAtStartup];
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
			 * @brief Late stage of initialization for the Tracer service as it needs some infos.
			 * @warning This can't be done at startup because the Tracer service is the first to be set up.
			 * @param fileSystem A reference to the filesystem service.
			 * @param settings A reference to the settings service.
			 * @return void
			 */
			void lateInitialize (const FileSystem & fileSystem, Settings & settings) noexcept;

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
			 * @brief Returns the instance of tracer.
			 * @return Tracer *
			 */
			[[nodiscard]]
			static
			Tracer *
			instance () noexcept
			{
				return s_instance;
			}

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
				s_instance->trace(Severity::Info, tag, message, location);
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
				s_instance->trace(Severity::Success, tag, message, location);
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
				s_instance->trace(Severity::Warning, tag, message, location);
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
				s_instance->trace(Severity::Error, tag, message, location);
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
				s_instance->trace(Severity::Fatal, tag, message, location);
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
					s_instance->trace(Severity::Debug, tag, message, location);
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
				s_instance->traceAPI(tag, functionName, message, location);
			}

			/**
			 * @brief Converts GLFW log facility to trace()
			 * @param error The error number from the GLFW library.
			 * @param description The message associated with the error code.
			 * @return void
			 */
			static void traceGLFW (int error, const char * description) noexcept;

		private:

			/** @copydoc EmEn::ServiceInterface::onInitialize() */
			bool onInitialize () noexcept override;

			/** @copydoc EmEn::ServiceInterface::onTerminate() */
			bool onTerminate () noexcept override;

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

			/* Flag names. */
			static constexpr auto ServiceInitialized{0UL};
			static constexpr auto IsChildProcess{1UL};
			static constexpr auto PrintOnlyErrors{2UL};
			static constexpr auto SourceLocationEnabled{3UL};
			static constexpr auto ThreadInfosEnabled{4UL};
			static constexpr auto IsTracerDisabled{5UL};
			static constexpr auto LoggerRequestedAtStartup{6UL};

			static Tracer * s_instance;

			const Arguments & m_arguments;
			std::filesystem::path m_cacheDirectory;
			std::string m_processName;
			std::vector< std::string > m_filters;
			std::unique_ptr< TracerLogger > m_logger;
			LogFormat m_logFormat{LogFormat::Text};
			int m_parentProcessID{-1};
			int m_processID{-1};
			std::array< bool, 8 > m_flags{
				false/*ServiceInitialized*/,
				false/*IsChildProcess*/,
				false/*PrintOnlyErrors*/,
				false/*SourceLocationEnabled*/,
				false/*ThreadInfosEnabled*/,
				false/*IsTracerDisabled*/,
				false/*LoggerRequestedAtStartup*/,
				false/*UNUSED*/
			};
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
				Tracer::instance()->trace(Severity::Debug, m_tag, this->get(), m_location);
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
				Tracer::instance()->trace(Severity::Success, m_tag, this->get(), m_location);
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
				Tracer::instance()->trace(Severity::Info, m_tag, this->get(), m_location);
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
				Tracer::instance()->trace(Severity::Warning, m_tag, this->get(), m_location);
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
				Tracer::instance()->trace(Severity::Error, m_tag, this->get(), m_location);
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
				Tracer::instance()->trace(Severity::Fatal, m_tag, this->get(), m_location);

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
				Tracer::instance()->traceAPI(m_tag, m_functionName, this->get(), m_location);

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
