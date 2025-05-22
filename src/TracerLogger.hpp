/*
 * src/TracerLogger.hpp
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
#include <iostream>
#include <filesystem>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>

/* Local inclusions for usages. */
#include "TracerEntry.hpp"
#include "Types.hpp"

namespace EmEn
{
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
}
