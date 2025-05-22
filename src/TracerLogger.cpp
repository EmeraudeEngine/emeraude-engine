/*
 * src/TracerLogger.cpp
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

#include "TracerLogger.hpp"

/* STL inclusions. */
#include <fstream>
#include <thread>
#include <utility>

namespace EmEn
{
	TracerLogger::TracerLogger (std::filesystem::path filepath, LogFormat logFormat) noexcept
		: m_filepath(std::move(filepath)),
		m_logFormat(logFormat)
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

				/* NOTE: Wait for the thread to be wake up. */
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
}
