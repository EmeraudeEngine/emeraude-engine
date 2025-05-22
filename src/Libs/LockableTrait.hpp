/*
 * src/Libs/LockableTrait.hpp
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
#include <atomic>
#include <mutex>

namespace EmEn::Libs
{
	/**
	 * @brief Adds a thread-safe, BasicLockable locking capability to a class.
	 * @note This trait is compatible with std::lock_guard for RAII-style locking,
	 * uses an atomic flag to prevent race conditions, and provides a virtual
	 * destructor for safe polymorphism.
	 */
	class LockableTrait
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			LockableTrait (const LockableTrait & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			LockableTrait (LockableTrait && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return LockableTrait &
			 */
			LockableTrait & operator= (const LockableTrait & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return LockableTrait &
			 */
			LockableTrait & operator= (LockableTrait && copy) noexcept = delete;

			/**
			 * @brief virtual destructor.
			 */
			virtual ~LockableTrait () = default;

			/**
			 * @brief Locks the object.
			 * @note Part of the BasicLockable requirement for std::lock_guard.
			 * @return void
			 */
			void
			lock () noexcept
			{
				m_locked.store(true, std::memory_order_release);
			}

			/**
			 * @brief Unlocks the object.
			 * @note Part of the BasicLockable requirement for std::lock_guard.
			 * @return void
			 */
			void
			unlock () noexcept
			{
				m_locked.store(false, std::memory_order_release);
			}

			/**
			 * @brief Atomically checks if the object is locked.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isLocked () const noexcept
			{
				return m_locked.load(std::memory_order_acquire);
			}

		protected:

			/**
			 * @brief Default constructor.
			 */
			LockableTrait () noexcept = default;

		private:

			std::atomic_bool m_locked{false};
	};
}
