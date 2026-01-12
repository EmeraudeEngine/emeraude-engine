/*
 * src/Libs/Debug/ConcurrencyDetector.hpp
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

#pragma once

/* STL inclusions. */
#include <iostream>
#include <thread>
#include <mutex>
#include <optional>
#include <string>
#include <source_location>

namespace EmEn::Libs::Debug
{
	inline std::mutex g_console_mutex;

	/**
	 * @brief Class to detect concurrent access to a scope.
	 * @note This is basically used like a std::mutex.
	 * @warning This is a development tool, it is not intended to be kept in the final code!
	 */
	class ConcurrencyDetector final
	{
		public:

			/**
			 * @brief Constructs a concurrency detector.
			 * @param contextName A string to name the context.
			 */
			explicit
			ConcurrencyDetector (std::string contextName) noexcept
				: m_contextName{std::move(contextName)},
				m_activeThreadId{std::nullopt}
			{

			}

			/**
			 * @brief Deleted copy constructor.
			 * @param other A reference to a concurrency detector.
			 */
			ConcurrencyDetector (const ConcurrencyDetector & other) = delete;

			/**
			 * @brief Deleted assignment operator.
			 * @param other A reference to a concurrency detector.
			 * @return ConcurrencyDetector &
			 */
			ConcurrencyDetector & operator= (const ConcurrencyDetector & other) = delete;

		private:

			friend class ConcurrencyDetectorGuard;

			/**
			 * @brief Enters the context and check for concurrency.
			 * @param location A reference to a source location.
			 * @return void
			 */
			void
			enter (const std::source_location & location)
			{
				const auto currentThreadId = std::this_thread::get_id();

				const std::lock_guard< std::mutex > lock{m_internalMutex};

				if ( m_activeThreadId.has_value() )
				{
					/* NOTE: Another thread is already in the section! */
					if ( m_activeThreadId.value() != currentThreadId )
					{
						/* NOTE: We lock the console for a clean display. */
						const std::lock_guard< std::mutex > consoleLock{g_console_mutex};

						std::cerr << "[CONCURRENCY DETECTED] Section '" << m_contextName << "' at " <<
							location.file_name() << ':' << location.line() << " (" << location.function_name() << ")" "\n"
							"Thread " << currentThreadId << " entered while Thread " << m_activeThreadId.value() << " was already inside!" "\n";
					}
				}
				else
				{
					/* NOTE: The section is free, we record our passage. */
					m_activeThreadId = currentThreadId;
				}
			}

			/**
			 * @brief Leavers the context and remove the thread ID from the scope detector.
			 * @return void
			 */
			void
			leave ()
			{
				const std::thread::id current_id = std::this_thread::get_id();

				const std::lock_guard< std::mutex > lock{m_internalMutex};

				/* NOTE: We only release the section if we are the ones occupying it. */
				if ( m_activeThreadId.has_value() && m_activeThreadId.value() == current_id )
				{
					m_activeThreadId = std::nullopt;
				}
			}

			std::string m_contextName;
			std::mutex m_internalMutex;
			std::optional< std::thread::id > m_activeThreadId;
	};

	/**
	 * @brief Helpers based to instantiate easily the ConcurrencyDetector object.
	 * @note This is basically used like a std::lock_guard.
	 * @warning This is a development tool, it is not intended to be kept in the final code!
	 */
	class ConcurrencyDetectorGuard final
	{
		public:

			/**
			 * @brief Constructs a concurrency detector guard.
			 * @param detector A reference to a concurrency detector.
			 * @param location A reference to a source location. Default automatic.
			 */
			explicit
			ConcurrencyDetectorGuard (ConcurrencyDetector & detector, const std::source_location & location = std::source_location::current())
				: m_detector{detector}
			{
				m_detector.enter(location);
			}

			/**
			 * @brief Deleted copy constructor.
			 * @param other A reference to a concurrency detector guard.
			 */
			ConcurrencyDetectorGuard (const ConcurrencyDetectorGuard & other) = delete;

			/**
			 * @brief Deleted assignment operator.
			 * @param other A reference to a concurrency detector guard.
			 * @return ConcurrencyDetectorGuard &
			 */
			ConcurrencyDetectorGuard & operator= (const ConcurrencyDetectorGuard & other) = delete;

			/**
			 * @brief Destructs the concurrency detector guard.
			 */
			~ConcurrencyDetectorGuard ()
			{
				m_detector.leave();
			}

		private:

			ConcurrencyDetector & m_detector;
	};
}
