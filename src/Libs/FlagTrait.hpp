/*
 * src/Libs/FlagTrait.hpp
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
#include <type_traits>

namespace EmEn::Libs
{
	/**
	 * @brief Adds the ability to use flags on an object by using an integer and bitwise operations.
	 * @note Only the flag reading is public. This is intended for internal options of a class.
	 * @tparam data_t The size of the flag. Default uint32_t.
	 */
	template< typename data_t = uint32_t >
	requires (std::is_integral_v< data_t >, std::is_unsigned_v< data_t >)
	class FlagTrait
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			FlagTrait (const FlagTrait & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			FlagTrait (FlagTrait && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return FlagTrait &
			 */
			FlagTrait & operator= (const FlagTrait & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return FlagTrait &
			 */
			FlagTrait & operator= (FlagTrait && copy) noexcept = default;

			/**
			 * @brief Destructs the flag trait.
			 * @note As a trait mechanism, 'virtual' keyword is useless here.
			 */
			virtual ~FlagTrait () = default;

			/**
			 * @brief Returns the raw flag value.
			 * @return data_t
			 */
			[[nodiscard]]
			data_t
			flags () const noexcept
			{
				return m_flags;
			}

			/**
			 * @brief Returns whether a flag is enabled.
			 * @param flagBit The flag bit to check.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFlagEnabled (data_t flagBit) const noexcept
			{
				return (m_flags & flagBit) != 0;
			}

			/**
			 * @brief Returns whether a flag is disabled.
			 * @param flagBit The flag bit to check.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFlagDisabled (data_t flagBit) const noexcept
			{
				return (m_flags & flagBit) == 0;
			}

			/**
			 * @brief Returns whether a flag is enabled.
			 * @param flags The flags.
			 * @param flagBit The flag bit to check.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			enabled (data_t flags, data_t flagBit) noexcept
			{
				return (flags & flagBit) != 0;
			}

			/**
			 * @brief Returns whether a flag is disabled.
			 * @param flags The flags.
			 * @param flagBit The flag bit to check.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			disabled (data_t flags, data_t flagBit) noexcept
			{
				return (flags & flagBit) == 0;
			}

		protected:

			/**
			 * @brief Constructs a default flag trait.
			 */
			FlagTrait () noexcept = default;

			/**
			 * @brief Constructs a flag trait.
			 * @param flags The initial flag value.
			 */
			explicit
			FlagTrait (data_t flags) noexcept
				: m_flags(flags)
			{

			}

			/**
			 * @brief Enables a flag.
			 * @param flagBit The flag bit value.
			 * @return void
			 */
			void
			enableFlag (data_t flagBit) noexcept
			{
				m_flags |= flagBit;
			}

			/**
			 * @brief Disables a flag.
			 * @param flagBit The flag bit value.
			 * @return void
			 */
			void
			disableFlag (data_t flagBit) noexcept
			{
				m_flags &= ~flagBit;
			}

			/**
			 * @brief Toggles a flag state.
			 * @param flagBit The flag bit value.
			 * @return void
			 */
			void
			toggleFlag (data_t flagBit) noexcept
			{
				m_flags ^= flagBit;
			}

			/**
			 * @brief Sets multiple flags in one shot.
			 * @warning This will erase all previous flags.
			 * @param flags The flag bits.
			 * @return void
			 */
			void
			setFlags (data_t flags) noexcept
			{
				m_flags = flags;
			}

			/**
			 * @brief Resets all flag to zero.
			 * @note Sames as call FlagTrait::setFlagBits(0).
			 * @return void
			 */
			void
			resetFlags () noexcept
			{
				m_flags = 0;
			}

		private:

			data_t m_flags{0};
	};
}
