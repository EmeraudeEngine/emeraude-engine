/*
 * src/TracerEntry.hpp
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
#include <chrono>
#include <string>
#include <thread>
#include "Libs/std_source_location.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"

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
				: m_severity(severity),
				m_tag(tag),
				m_message(std::move(message)),
				m_location(location),
				m_threadId(threadId)
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
}
