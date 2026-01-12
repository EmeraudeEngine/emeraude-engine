/*
 * src/Tracer.cpp
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
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Tracer.hpp"

/* STL inclusions. */
#include <cstring>
#include <algorithm>
#include <fstream>
#include <utility>

/* System inclusions. */
#if IS_LINUX || IS_MACOS
#include <unistd.h>
#endif

/* Local inclusions. */
#include "Libs/String.hpp"
#include "Arguments.hpp"
#include "FileSystem.hpp"
#include "Settings.hpp"
#include "SettingKeys.hpp"
#if IS_WINDOWS
#include "PlatformSpecific/Helpers.hpp"
#endif

namespace EmEn
{
	using namespace Libs;

	Tracer &
	Tracer::getInstance () noexcept
	{
		static auto instance = std::make_unique< Tracer >(PrivateToken{});

		return *instance;
	}

	TracerLogger::TracerLogger (std::filesystem::path filepath, LogFormat logFormat) noexcept
		: m_filepath{std::move(filepath)},
		m_logFormat{logFormat}
	{
		std::fstream file{m_filepath, std::ios::out | std::ios::trunc};

		m_isUsable = file.is_open();

		file.close();

		if constexpr ( IsDebug )
		{
			if ( m_isUsable )
			{
				std::cout << "TracerLogger::TracerLogger() : Log file " << m_filepath << " opened !" "\n";
			}
			else
			{
				std::cerr << "TracerLogger::TracerLogger() : Unable to open the log file " << m_filepath << " !" "\n";
			}
		}
	}

	TracerLogger::~TracerLogger ()
	{
		this->stop();

		if ( m_thread.joinable() )
		{
			m_thread.join();
		}
	}

	void
	TracerLogger::push (Severity severity, const char * tag, std::string message, const std::source_location & location) noexcept
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
	TracerLogger::start () noexcept
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

	void
	TracerLogger::stop () noexcept
	{
		m_isRunning = false;

		m_condition.notify_one();
	}

	void
	TracerLogger::clear () noexcept
	{
		const std::lock_guard< std::mutex > lock{m_entriesAccess};

		std::queue< TracerEntry > emptyQueue;

		m_entries.swap(emptyQueue);
	}

	void
	TracerLogger::task () noexcept
	{
		std::fstream file{m_filepath, std::ios::out | std::ios::app};

		/* NOTE: Write the file start. */
		switch ( m_logFormat )
		{
			case LogFormat::Text :
				file << "====== " << EngineName << " " << VersionString << " execution. Beginning at " << std::chrono::steady_clock::now().time_since_epoch().count() << " ======" "\n";
				break;

			case LogFormat::JSON :
				file << "{" "\n";
				break;

			case LogFormat::HTML :
				file <<
					"<!DOCTYPE html>" "\n"
					"<html>" "\n"
					"\t" "<head>" "\n"
					"\t\t" "<title>" << EngineName << " " << VersionString << " execution</title>" "\n"
					"\t" "</head>" "\n"
					"\t" "<body>" "\n"

					"\t\t" "<h1>" << EngineName << " " << VersionString << " execution</h1>" "\n"
					"\t\t" "<p>Beginning at " << std::chrono::steady_clock::now().time_since_epoch().count() << "</p>" "\n";
				break;
		}

		while ( m_isRunning )
		{
			std::queue< TracerEntry > localQueue;

			{
				std::unique_lock< std::mutex > lock{m_entriesAccess};

				/* NOTE: Wait for the thread to be a wake-up. */
				m_condition.wait(lock, [&] {
					return !m_entries.empty() || !m_isRunning;
				});

				if ( !m_entries.empty() )
				{
					m_entries.swap(localQueue);
				}
			}

			while ( !localQueue.empty() )
			{
				const auto & entry = localQueue.front();

				switch ( m_logFormat )
				{
					case LogFormat::Text :
						file <<
							"[" << entry.time().time_since_epoch().count() << "]"
							"[" << entry.tag() << "]"
							"[" << to_string(entry.severity()) << "]"
							"[" << entry.location().file_name() << ':' << entry.location().line() << ':' << entry.location().column() << " `" << entry.location().function_name() << "`]" "\n"
							<< entry.message() << '\n';
						break;

					case LogFormat::JSON :
						file <<
							"\t" "{" "\n"
							"\t\t" "\"tag\" : " << entry.tag() << " @ <small><i>" << entry.location().file_name() << ':' << entry.location().line() << ':' << entry.location().column() << " `" << entry.location().function_name() << '`' << "</i></small></h2>" "\n"
							"\t\t" "\"filename\" : " << entry.location().file_name() << ':' << entry.location().line() << ':' << entry.location().column() << " `" << entry.location().function_name() << '`' << "</i></small></h2>" "\n"
							"\t\t" "\"line\" : " <<  entry.location().line() << ':' << entry.location().column() << " `" << entry.location().function_name() << '`' << "</i></small></h2>" "\n"
							"\t\t" "\"column\" :" << entry.location().column() << " `" << entry.location().function_name() << '`' << "</i></small></h2>" "\n"
							"\t\t" "\"function\" : " << entry.location().function_name() << '`' << "</i></small></h2>" "\n"
							"\t\t" "<p class=\"entry-time\">Time: " << entry.time().time_since_epoch().count() << "</p>" "\n"
							"\t\t" "<p class=\"entry-thread\">Thread: " << entry.threadId() << "</p>" "\n"
							"\t\t" "<p class=\"entry-severity\">Severity: " << to_string(entry.severity()) << "<p/>" "\n"
							"\t\t" "<pre class=\"entry-message\">" "\n"
							<< entry.message() << "\n"
							"\t\t" "</pre>" "\n"
							"\t" "}" "\n";
						break;

					case LogFormat::HTML :
						file <<
							"\t\t" "<div>" "\n"
							"\t\t\t" "<h2 class=\"entry-tag\">" << entry.tag() << " @ <small><i>" << entry.location().file_name() << ':' << entry.location().line() << ':' << entry.location().column() << " `" << entry.location().function_name() << '`' << "</i></small></h2>" "\n"
							"\t\t\t" "<p class=\"entry-time\">Time: " << entry.time().time_since_epoch().count() << "</p>" "\n"
							"\t\t\t" "<p class=\"entry-thread\">Thread: " << entry.threadId() << "</p>" "\n"
							"\t\t\t" "<p class=\"entry-severity\">Severity: " << to_string(entry.severity()) << "<p/>" "\n"
							"\t\t\t" "<pre class=\"entry-message\">" "\n"
							<< entry.message() << "\n"
							"\t\t\t" "</pre>" "\n"
							"\t\t" "</div>" "\n";
						break;
				}

				localQueue.pop();
			}

			/* NOTE: Force to write into the file. */
			file.flush();
		}

		/* NOTE: Write the file end. */
		switch ( m_logFormat )
		{
			case LogFormat::Text :
				file << "====== Log file closed properly ======" "\n";
				break;

			case LogFormat::JSON :
				file << "}" "\n";
				break;

			case LogFormat::HTML :
				file <<
				   "\t\t" "<p>Ending at " << std::chrono::steady_clock::now().time_since_epoch().count() << "</p>" "\n"
				   "\t" "</body>" "\n"
				   "</html>" "\n";
				break;
		}
	}

	Tracer::Tracer (PrivateToken) noexcept
	{
		if constexpr ( IsDebug )
		{
			std::cout << "Tracer constructed!" << std::endl;
		}
	}

	Tracer::~Tracer ()
	{
		this->disableLogger();

		if constexpr ( IsDebug )
		{
			std::cout << "Tracer instance destroyed!" << std::endl;
		}
	}

	void
	Tracer::earlySetup (const Arguments & arguments, std::string processName, bool childProcess) noexcept
	{
		m_processName = std::move(processName);

		m_isChildProcess = childProcess;

		/* NOTE: Register once PPID and PID for this tracer. */
#if IS_LINUX || IS_MACOS
		m_parentProcessID = getppid();
		m_processID = getpid();
#elif IS_WINDOWS
		const auto PID = GetCurrentProcessId();

		m_parentProcessID = PlatformSpecific::getParentProcessId(PID);
		m_processID = static_cast< int >(PID);
#endif

		if ( const auto argument = arguments.get("--filter-tags") )
		{
			for ( const auto & term : String::explode(argument.value(), ',', false) )
			{
				this->addTagFilter(String::trim(term));
			}
		}

		if ( arguments.isSwitchPresent("-q", "--disable-tracing") )
		{
			std::cout << "Tracer disabled on startup !" "\n";

			m_isTracerDisabled = true;
		}

		m_serviceInitialized = true;
	}

	void
	Tracer::lateSetup (const Arguments & arguments, const FileSystem & fileSystem, Settings & settings) noexcept
	{
		if ( this->isTracerDisabled() )
		{
			return;
		}

		this->enablePrintOnlyErrors(settings.getOrSetDefault< bool >(TracerPrintOnlyErrorsKey, DefaultTracerPrintOnlyErrors));
		this->enableSourceLocation(settings.getOrSetDefault< bool >(TracerEnableSourceLocationKey, DefaultTracerEnableSourceLocation));
		this->enableThreadInfos(settings.getOrSetDefault< bool >(TracerEnableThreadInfosKey, DefaultTracerEnableThreadInfos));

		m_cacheDirectory = fileSystem.cacheDirectory();
		m_logFormat = to_LogFormat(settings.getOrSetDefault< std::string >(TracerLogFormatKey, DefaultTracerLogFormat));

		/* TODO: Clarify this behavior! */
		const auto argument = arguments.get("-l", "--enable-log");

		if ( settings.getOrSetDefault< bool >(TracerEnableLoggerKey, DefaultTracerEnableLogger) || argument.has_value() )
		{
			m_loggerRequestedAtStartup = true;

			/* NOTE: Disable the logger creation at the startup. This is useful for multi-processes application. */
			if ( arguments.isSwitchPresent("--disable-log") )
			{
				return;
			}

			if ( argument.has_value() )
			{
				this->enableLogger(std::filesystem::path{argument.value()});
			}
			else
			{
				const auto logFilepath = this->generateLogFilepath(m_processName);

				this->enableLogger(logFilepath);
			}
		}

		TraceDebug{ClassId} << "The tracer is fully configured for the process '" << m_processName << "'.";
	}

	std::filesystem::path
	Tracer::generateLogFilepath (const std::string & name) const noexcept
	{
		std::stringstream filename;
		filename << "journal-" << name;

		switch ( m_logFormat )
		{
			case LogFormat::Text :
				filename << ".log";
				break;

			case LogFormat::JSON :
				filename << ".json";
				break;

			case LogFormat::HTML :
				filename << ".html";
				break;
		}

		auto cacheDirectory = m_cacheDirectory;

		return cacheDirectory.append(filename.str());
	}

	bool
	Tracer::enableLogger (const std::filesystem::path & filepath) noexcept
	{
		if ( m_logger != nullptr )
		{
			return true;
		}

		m_logger = std::make_unique< TracerLogger >(filepath, m_logFormat);

		if ( m_logger->start() )
		{
			return true;
		}

		m_logger.reset();

		this->trace(Severity::Error, ClassId, "Unable to enable the tracer logger!");

		return false;
	}

	void
	Tracer::disableLogger () noexcept
	{
		m_logger.reset();
	}

	void
	Tracer::trace (Severity severity, const char * tag, std::string_view message, const std::source_location & location) const noexcept
	{
		if ( this->isTracerDisabled() || !this->filterTag(tag) )
		{
			return;
		}

		if ( m_logger != nullptr )
		{
			m_logger->push(severity, tag, std::string{message}, location);
		}

		std::stringstream trace;

		trace << '[' << to_string(severity) << "][" << tag << ']';

		Tracer::colorizeMessage(trace, severity, message);

		if ( this->isThreadInfosEnabled() )
		{
			this->injectProcessInfo(trace);
		}

		if ( this->isSourceLocationEnabled() )
		{
			trace << "[" << location.file_name() << ':' << location.line() << ':' << location.column() << " `" << location.function_name() << "`]";
		}

		const std::lock_guard< std::mutex > lock{m_consoleAccess};

		switch ( severity )
		{
			case Severity::Debug :
			case Severity::Info :
			case Severity::Success :
				if ( !m_printOnlyErrors )
				{
					std::cout << trace.str() << '\n';
				}
				break;

			case Severity::Warning :
			case Severity::Error :
			case Severity::Fatal :
				std::cerr << trace.str() << '\n';
				break;
		}
	}

	void
	Tracer::traceAPI (const char * tag, const char * functionName, std::string_view message, const std::source_location & location) const noexcept
	{
		if ( m_logger != nullptr )
		{
			std::stringstream logMessage{};
			logMessage << functionName << "() : " << message;

			m_logger->push(Severity::Info, tag, logMessage.str(), location);
		}

		std::stringstream trace;

		trace << "[" << tag << "] ";

		if ( message.empty() )
		{
			trace << "\033[1;93m" << functionName << "() called !" << "\033[0m ";
		}
		else
		{
			trace << "\033[1;93m" << functionName << "(), " << message << "\033[0m ";
		}

		if ( this->isSourceLocationEnabled() )
		{
			trace << "\n\t" "[" << location.file_name() << ':' << location.line() << ':' << location.column() << " `" << location.function_name() << "`]";
		}

		if ( this->isThreadInfosEnabled() )
		{
			this->injectProcessInfo(trace);
		}

		const std::lock_guard< std::mutex > lock{m_consoleAccess};

		std::cout << trace.str() << "\n";
	}

	void
	Tracer::injectProcessInfo (std::stringstream & stream) const noexcept
	{
#if IS_LINUX
		const auto tid = gettid();
#elif IS_MACOS
		const auto tid = -1;
#elif IS_WINDOWS
		const auto tid = GetCurrentThreadId();
#else
		const auto tid = -1;
#endif

		stream << "\n\t" "[PPID:" << m_parentProcessID << "][PID:" << m_processID << "][TID:" << tid << ']';
	}

	bool
	Tracer::filterTag (const char * tag) const noexcept
	{
		/* There is no tag filtering at all. */
		if ( m_filters.empty() )
		{
			return true;
		}

		/* Checks if a term matches the filter. */
		return std::ranges::any_of(m_filters, [tag] (const auto & filteredTag) {
			return std::strcmp(tag, filteredTag.c_str()) == 0;
		});
	}

	void
	Tracer::colorizeMessage (std::stringstream & stream, Severity severity,  std::string_view message) noexcept
	{
		switch ( severity )
		{
			case Severity::Debug :
				stream << " \033[1;36m" << message << "\033[0m ";
				break;

			case Severity::Success :
				stream << " \033[1;92m" << message << "\033[0m ";
				break;

			case Severity::Warning :
				stream << " \033[1;35m" << message << "\033[0m ";
				break;

			case Severity::Error :
				stream << " \033[1;91m" << message << "\033[0m ";
				break;

			case Severity::Fatal :
				stream << " \033[1;41m" << message << "\033[0m ";
				break;

			case Severity::Info :
			default :
				stream << ' ' << message << ' ';
				break;
		}
	}
}
