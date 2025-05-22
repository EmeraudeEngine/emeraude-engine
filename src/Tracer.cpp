/*
 * src/Tracer.cpp
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

#include "Tracer.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstring>
#include <algorithm>
#include <exception>
#include <iostream>

/* System inclusions. */
#if IS_LINUX || IS_MACOS
#include <unistd.h>
#endif

/* Local inclusions. */
#include "Libs/String.hpp"
#include "Arguments.hpp"
#include "FileSystem.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "TracerLogger.hpp"
#include "Types.hpp"
#if IS_WINDOWS
#include "PlatformSpecific/Helpers.hpp"
#endif

namespace EmEn
{
	using namespace EmEn::Libs;

	const size_t Tracer::ClassUID{getClassUID(ClassId)};

	Tracer * Tracer::s_instance{nullptr};

	Tracer::Tracer (const Arguments & arguments, std::string processName, bool childProcess) noexcept
		: ServiceInterface(ClassId),
		m_arguments(arguments),
		m_processName(std::move(processName))
	{
		m_flags[IsChildProcess] = childProcess;

		if ( s_instance != nullptr )
		{
			std::cerr << __PRETTY_FUNCTION__ << ", constructor called twice !" "\n";

			std::terminate();
		}

		s_instance = this;
	}

	Tracer::~Tracer ()
	{
		s_instance = nullptr;
	}

	bool
	Tracer::onInitialize () noexcept
	{
		/* NOTE: Register once PPID and PID for this tracer. */
#if IS_LINUX || IS_MACOS
		m_parentProcessID = getppid();
		m_processID = getpid();
#elif IS_WINDOWS
		const auto PID = GetCurrentProcessId();

		m_parentProcessID = PlatformSpecific::getParentProcessId(PID);
		m_processID = static_cast< int >(PID);
#endif

		auto argument = m_arguments.get("--filter-tags");

		if ( argument.isPresent() )
		{
			for ( const auto & term : String::explode(argument.value(), ',', false) )
			{
				this->addTagFilter(String::trim(term));
			}
		}

		argument = m_arguments.get("-q", "--disable-tracing");

		if ( argument.isPresent() )
		{
			std::cout << "Tracer disabled on startup !" "\n";

			m_flags[IsTracerDisabled] = true;
		}

		m_flags[ServiceInitialized] = true;

		return true;
	}

	bool
	Tracer::onTerminate () noexcept
	{
		m_flags[ServiceInitialized] = false;

		this->disableLogger();

		return true;
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
	Tracer::lateInitialize (const FileSystem & fileSystem, Settings & settings) noexcept
	{
		if ( this->isTracerDisabled() )
		{
			return;
		}

		this->enablePrintOnlyErrors(settings.get< bool >(TracerPrintOnlyErrorsKey, DefaultTracerPrintOnlyErrors));
		this->enableSourceLocation(settings.get< bool >(TracerEnableSourceLocationKey, DefaultTracerEnableSourceLocation));
		this->enableThreadInfos(settings.get< bool >(TracerEnableThreadInfosKey, DefaultTracerEnableThreadInfos));

		m_cacheDirectory = fileSystem.cacheDirectory();
		m_logFormat = to_LogFormat(settings.get< std::string >(TracerLogFormatKey, DefaultTracerLogFormat));

		const auto argument = m_arguments.get("-l", "--enable-log");

		if ( settings.get< bool >(TracerEnableLoggerKey, DefaultTracerEnableLogger) || argument.isPresent() )
		{
			m_flags[LoggerRequestedAtStartup] = true;

			/* NOTE: Disable the logger creation at the startup. This is useful for multi-processes application. */
			if ( m_arguments.get("--disable-log").isPresent() )
			{
				return;
			}

			if ( argument.value().empty() )
			{
				const auto logFilepath = this->generateLogFilepath(m_processName);

				this->enableLogger(logFilepath);
			}
			else
			{
				this->enableLogger(std::filesystem::path{argument.value()});
			}
		}
	}

	void
	Tracer::trace (Severity severity, const char * tag, std::string_view message, const std::source_location & location) const noexcept
	{
		if ( this->isTracerDisabled() || !this->filterTag(tag) )
		{
			return;
		}

		std::stringstream trace;

		trace << '[' << to_string(severity) << "][" << tag << ']';

		Tracer::colorizeMessage(trace, severity, message);

		if ( this->isSourceLocationEnabled() )
		{
			trace << "\n\t" "[" << location.file_name() << ':' << location.line() << ':' << location.column() << " `" << location.function_name() << "`]";
		}

		if ( this->isThreadInfosEnabled() )
		{
			this->injectProcessInfo(trace);
		}

		switch ( severity )
		{
			case Severity::Debug :
			case Severity::Info :
			case Severity::Success :
				if ( !m_flags[PrintOnlyErrors] )
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

		if ( m_logger != nullptr )
		{
			m_logger->push(severity, tag, std::string{message}, location);
		}
	}

	void
	Tracer::traceAPI (const char * tag, const char * functionName, std::string_view message, const std::source_location & location) const noexcept
	{
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

		std::cout << trace.str() << "\n";

		if ( m_logger != nullptr )
		{
			std::stringstream logMessage{};
			logMessage << functionName << "() : " << message;

			m_logger->push(Severity::Info, tag, logMessage.str(), location);
		}
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

		stream << "[PPID:" << m_parentProcessID << "][PID:" << m_processID << "][TID:" << tid << ']';
	}

	void
	Tracer::traceGLFW (int error, const char * description) noexcept
	{
		s_instance->trace(Severity::Error, "GLFW", (std::stringstream{} << description << " (errno:" << error << ')').str(), {});
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
		return std::ranges::any_of(m_filters, [tag](const auto & filteredTag) {
			return std::strcmp(tag, filteredTag.c_str()) == 0;
		});
	}
}
