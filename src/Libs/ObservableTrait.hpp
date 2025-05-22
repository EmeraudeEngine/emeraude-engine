/*
 * src/Libs/ObservableTrait.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstddef>
#include <any>
#include <set>
#include <mutex>
#if defined(DEBUG) && IS_LINUX
#include <map>
#include <memory>
#endif

/* Forward declarations. */
namespace EmEn::Libs
{
	class ObserverTrait;
}

namespace EmEn::Libs
{
	/**
	 * @brief This class is the observable interface of the observer design pattern.
	 */
	class ObservableTrait
	{
		/* NOTE: This simplifies looped-back operations between Observable/Observer classes. */
		friend class ObserverTrait;

		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ObservableTrait (const ObservableTrait & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ObservableTrait (ObservableTrait && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return ObservableTrait &
			 */
			ObservableTrait & operator= (const ObservableTrait & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return Observable &
			 */
			ObservableTrait & operator= (ObservableTrait && copy) noexcept = delete;

			/**
			 * @brief Destructs the observable.
			 * @note Takes cares of detaching observers.
			 */
			virtual ~ObservableTrait ();

			/**
			 * @brief Returns whether the object is currently observed.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isObserved () const noexcept
			{
				return !m_observers.empty();
			}

			/**
			 * @brief Returns whether the object is observed by a specific observer.
			 * @param observer The pointer of the observer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isObserved (ObserverTrait * observer) const noexcept
			{
				return m_observers.contains(observer);
			}

			/**
			 * @brief Returns the list of observers.
			 * @return const std::set< Observer * > &
			 */
			[[nodiscard]]
			const std::set< ObserverTrait * > &
			observers () const noexcept
			{
				return m_observers;
			}

			/**
			 * @brief Returns the observable unique identifier set by Observable::getClassUID().
			 * @return size_t
			 */
			[[nodiscard]]
			virtual size_t classUID () const noexcept = 0;

			/**
			 * @brief Returns whether the observable unique identifier matches.
			 * @note The ID should be asked once per class with Observable::getClassUID() and stored as a static.
			 * @param classUID A number to identify a kind of class.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool is (size_t classUID) const noexcept = 0;

#if defined(DEBUG) && IS_LINUX
			/**
			 * @brief Returns the label registered when generation the UID to use to debug the Observer::onNotification().
			 * @note This debugging method will return:
			 *  - "UNALLOCATED_MAP" if the function is called before any observable creation.
			 *  - "UNREGISTERED_ITEM" if the UID doesn't exist.
			 *  - "FEATURE_UNAVAILABLE" if the system is other than Linux.
			 * @param UID An observable UID.
			 * @return const char *
			 */
			[[nodiscard]]
			static const char * whoIs (size_t UID) noexcept;
#else
			[[nodiscard]]
			static const char *
			whoIs (size_t /*UID*/) noexcept
			{
				return "FEATURE_UNAVAILABLE";
			}
#endif

		protected:

			/**
			 * @brief Constructs an observable object.
			 */
			ObservableTrait () noexcept = default;

			/**
			 * @brief Sends a notification to every observer.
			 * @param notificationCode Code of notification.
			 * @param data A reference to any. Default no data.
			 * @return void
			 */
			void notify (int notificationCode, const std::any & data = {}) noexcept;

			/**
			 * @brief Returns a unique identifier for observable and register a label with it [Thread-safe].
			 * @param label A pointer to a C-string.
			 * @return size_t
			 */
			[[nodiscard]]
			static size_t getClassUID (const char * label) noexcept;

		private:

			/**
			 * @brief Adds an observer to this object.
			 * @param observer The pointer of the observer.
			 * @return void
			 */
			void
			addObserver (ObserverTrait * observer) noexcept
			{
				m_observers.emplace(observer);
			}

			/**
			 * @brief Removes an observer from this object.
			 * @param observer The pointer of the observer.
			 * @return void
			 */
			void
			removeObserver (ObserverTrait * observer) noexcept
			{
				m_observers.erase(observer);
			}

			static std::mutex s_UIDMutex;
			static size_t s_nextClassUID;
#if defined(DEBUG) && IS_LINUX
			static std::unique_ptr< std::map< size_t, const char * > > s_classUIDs;
#endif

			std::set< ObserverTrait * > m_observers;
			mutable std::mutex m_notificationMutex;
	};
}
