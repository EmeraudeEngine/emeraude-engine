/*
 * src/Libs/Debug/Dummy.hpp
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
#include <utility>

namespace EmEn::Libs::Debug
{
	/**
	 * @brief Dummy class to identify special method in use.
	 */
	class Dummy final
	{
		public:

			inline static int s_instanceCount = 0;

			/**
			 * @brief Default constructor.
			 */
			Dummy () noexcept
			{
				s_instanceCount++;

				std::cout << "[DEBUG] Default constructor called! " << *this << "\n";
			}

			/**
			 * @brief Parametric constructor.
			 * @param value A value.
			 */
			explicit
			Dummy (int value) noexcept
				: m_value{value}
			{
				s_instanceCount++;

				std::cout << "[DEBUG] Parametric constructor called! " << *this << "\n";
			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Dummy (const Dummy & copy) noexcept
				: m_value{copy.m_value}
			{
				s_instanceCount++;

				std::cout << "[DEBUG] Copy constructor called! " << *this << "\n";
			}

			/**
			 * @brief Move constructor.
			 * @param other A reference to the copied instance.
			 */
			Dummy (Dummy && other) noexcept
				: m_value{std::exchange(other.m_value, -1)}
			{
				s_instanceCount++;

				std::cout << "[DEBUG] Move constructor called! " << *this << "\n";
			}

			/**
			 * @brief Assignment operator.
			 * @param other A reference to the copied instance.
			 * @return Dummy &
			 */
			Dummy &
			operator= (const Dummy & other) noexcept
			{
				std::cout << "[DEBUG] Copy assignment called (from old value : " << other.value() << ") ! " << *this << "\n";

				if ( this != &other )
				{
					m_value = other.m_value;
				}

				return *this;
			}

			/**
			 * @brief Move operator.
			 * @param other A reference to the copied instance.
			 * @return Dummy &
			 */
			Dummy &
			operator= (Dummy && other) noexcept
			{
				const int oldValueFrom = other.m_value;
				const int oldValueTo = m_value;

				if ( this != &other )
				{
					m_value = std::exchange(other.m_value, -1);
				}

				std::cout << "[DEBUG] Move assignment (from: " << oldValueFrom << " to: " << oldValueTo << ")\n";

				return *this;
			}

			/**
			 * @brief Destructor.
			 */
			~Dummy ()
			{
				s_instanceCount--;

				std::cout << "[DEBUG] Destructor called! " << *this << "\n";
			}

			/**
			 * @brief C++ spaceship operator.
			 * @param other A reference to another dummy.
			 * @return std::strong_ordering
			 */
			auto operator<=> (const Dummy & other) const = default;

			/**
			 * @brief Sets a value.
			 * @param value The value.
			 * @return void
			 */
			void
			value (int value)
			{
				m_value = value;
			}

			/**
			 * @brief Returns the value.
			 * @return int
			 */
			[[nodiscard]]
			int
			value () const
			{
				return m_value;
			}

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<<(std::ostream & out, const Dummy & obj);

			int m_value{-1};
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const Dummy & obj)
	{
		return out << "This dummy value: " << obj.m_value << " (instance count: " << Dummy::s_instanceCount << ')';
	}
}
