/*
 * src/Libs/PixelFactory/Margin.hpp
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
#include <cstdint>
#include <array>
#include <sstream>
#include <string>

namespace EmEn::Libs::PixelFactory
{
	enum class MarginPosition: uint8_t
	{
		Top = 0UL,
		Right = 1UL,
		Bottom = 2UL,
		Left = 3UL
	};

	/**
	 * @brief Defines a 2D area to work with finite pixels.
	 * @tparam data_t The unsigned integer precision. Default size_t.
	 */
	template< typename data_t = size_t >
	requires (std::is_integral_v< data_t >, std::is_unsigned_v< data_t >)
	class Margin
	{
		public:

			/**
			 * @brief Constructs a default margin.
			 */
			Margin () noexcept = default;

			/**
			 * @brief Constructs a margin.
			 * @param margin
			 */
			explicit Margin (data_t margin) noexcept
				: m_data{{margin, margin, margin, margin}}
			{

			}

			/**
			 * @brief Constructs a margin.
			 * @param verticalMargin
			 * @param horizontalMargin
			 */
			Margin (data_t verticalMargin, data_t horizontalMargin) noexcept
				: m_data{{verticalMargin, horizontalMargin, verticalMargin, horizontalMargin}}
			{

			}

			/**
			 * @brief Constructs a margin.
			 * @param top
			 * @param right
			 * @param bottom
			 * @param left
			 */
			Margin (data_t top, data_t right, data_t bottom, data_t left) noexcept
				: m_data{{top, right, bottom, left}}
			{

			}

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			Margin (const Margin & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			Margin (Margin && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return Margin &
			 */
			Margin & operator= (const Margin & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Margin &
			 */
			Margin & operator= (Margin && copy) noexcept = default;

			/**
			 * @brief Destructs the margin.
			 */
			virtual ~Margin () = default;

			/**
			 * @brief Sets a size for all borders.
			 * @param margin The unique size.
			 */
			void
			setMargin (data_t margin) noexcept
			{
				m_data[Top] = margin;
				m_data[Right] = margin;
				m_data[Bottom] = margin;
				m_data[Left] = margin;
			}

			/**
			 * @brief Sets the margins vertically and horizontally.
			 * @param verticalMargin The vertical size.
			 * @param horizontalMargin The horizontal size.
			 */
			void
			setMargin (data_t verticalMargin, data_t horizontalMargin) noexcept
			{
				m_data[Top] = verticalMargin;
				m_data[Right] = horizontalMargin;
				m_data[Bottom] = verticalMargin;
				m_data[Left] = horizontalMargin;
			}

			/**
			 * @brief Sets the margins for each border.
			 * @param top The top size.
			 * @param right The right size.
			 * @param bottom The bottom size.
			 * @param left The left size.
			 */
			void
			setMargin (data_t top, data_t right, data_t bottom, data_t left) noexcept
			{
				m_data[Top] = top;
				m_data[Right] = right;
				m_data[Bottom] = bottom;
				m_data[Left] = left;
			}

			/**
			 * @brief Returns the margin size of a border.
			 * @param position The border.
			 * @return data_t
			 */
			[[nodiscard]]
			data_t
			margin (MarginPosition position) const noexcept
			{
				return m_data[static_cast< size_t >(position)];
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream &
			operator<< (std::ostream & out, const Margin & obj)
			{
				return out <<
					"Margin data :\n"
					"Margin top : " << obj.m_data[Top] << "\n"
					"Margin right : " << obj.m_data[Right] << "\n"
					"Margin bottom : " << obj.m_data[Bottom] << "\n"
					"margin left : " << obj.m_data[Left] << '\n';
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string
			to_string (const Margin & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			static constexpr auto Top = 0UL;
			static constexpr auto Right = 1UL;
			static constexpr auto Bottom = 2UL;
			static constexpr auto Left = 3UL;

			std::array< data_t, 4 > m_data{0, 0, 0, 0};
	};
}
