/*
 * src/Libs/FlagArrayTrait.hpp
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
#include <array>

namespace EmEn::Libs
{
	/**
	 * @brief Adds the ability to use flags on an object by using an array of booleans.
	 * @note Only the flag reading is public. This is intended for internal options of a class.
	 * @warning The EmEn::FlagTrait is a better alternative to use.
	 * @tparam count_t The number of possible flags. Default 8.
	 */
	template< size_t count_t = 8 >
	class FlagArrayTrait
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			FlagArrayTrait (const FlagArrayTrait & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			FlagArrayTrait (FlagArrayTrait && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return FlagArrayTrait &
			 */
			FlagArrayTrait & operator= (const FlagArrayTrait & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return FlagArrayTrait &
			 */
			FlagArrayTrait & operator= (FlagArrayTrait && copy) noexcept = default;

			/**
			 * @brief Destructs the flag array trait.
			 * @note As a trait mechanism, 'virtual' keyword is useless here.
			 */
			virtual ~FlagArrayTrait () = default;

			/**
			 * @brief Returns the flag array.
			 * @return data_t
			 */
			[[nodiscard]]
			const std::array< bool, count_t > &
			flags () const noexcept
			{
				return m_flags;
			}

			/**
			 * @brief Returns whether a flag is enabled.
			 * @warning Out of bound flag index will return silently 'false'.
			 * @param flag The flag index.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFlagEnabled (size_t flag) const noexcept
			{
				if ( flag >= count_t )
				{
					return false;
				}

				return m_flags[flag];
			}

			/**
			 * @brief Returns whether a flag is disabled.
			 * @warning Out of bound flag index will return silently 'false'.
			 * @param flag The flag index.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFlagDisabled (size_t flag) const noexcept
			{
				if ( flag >= count_t )
				{
					return true;
				}

				return !m_flags[flag];
			}

		protected:

			/**
			 * @brief Constructs a flag array trait.
			 */
			FlagArrayTrait () noexcept = default;

			/**
			 * @brief Enables a flag.
			 * @warning Out of bound flag index will silently do nothing.
			 * @param flag The flag index.
			 * @return void
			 */
			void
			enableFlag (size_t flag) noexcept
			{
				if ( flag < count_t )
				{
					m_flags[flag] = true;
				}
			}

			/**
			 * @brief Toggles a flag state.
			 * @warning Out of bound flag index will silently do nothing.
			 * @param flag The flag index.
			 * @return void
			 */
			void
			toggleFlag (size_t flag) noexcept
			{
				if ( flag < count_t )
				{
					m_flags[flag] = !m_flags[flag];
				}
			}

			/**
			 * @brief Disables a flag.
			 * @warning Out of bound flag index will silently do nothing.
			 * @param flag The flag index.
			 * @return void
			 */
			void
			disableFlag (size_t flag) noexcept
			{
				if ( flag < count_t )
				{
					m_flags[flag] = false;
				}
			}

			/**
			 * @brief Sets a flag state.
			 * @warning Out of bound flag index will silently do nothing.
			 * @param flag The flag index.
			 * @param state The flag state.
			 * @return void
			 */
			void
			setFlag (size_t flag, bool state) noexcept
			{
				if ( flag < count_t )
				{
					m_flags[flag] = state;
				}
			}

			/**
			 * @brief Resets all flag to false.
			 * @return void
			 */
			void
			resetFlags () noexcept
			{
				m_flags.fill(false);
			}

		private:

			std::array< bool, count_t > m_flags{};
	};
}
