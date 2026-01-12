/*
 * src/Libs/ObserverTrait.hpp
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
#include <any>
#include <set>

/* Forward declarations. */
namespace EmEn::Libs
{
	class ObservableTrait;
}

namespace EmEn::Libs
{
	/**
	 * @brief This class is the observer interface of the observer design pattern.
	 */
	class ObserverTrait
	{
		/* NOTE: This simplifies looped-back operations between Observable/Observer classes. */
		friend class ObservableTrait;

		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			ObserverTrait (const ObserverTrait & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			ObserverTrait (ObserverTrait && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return ObserverTrait &
			 */
			ObserverTrait & operator= (const ObserverTrait & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return ObserverTrait &
			 */
			ObserverTrait & operator= (ObserverTrait && copy) noexcept = delete;

			/**
			 * @brief Destructs the observer.
			 * @note Takes cares of detaching observables.
			 */
			virtual ~ObserverTrait ();

			/**
			 * @brief Says if something is currently observed.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isObserving () const noexcept
			{
				return !m_observables.empty();
			}

			/**
			 * @brief Returns whether a specific object is observed.
			 * @param observable The pointer of the observable.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isObserving (ObservableTrait * observable) const noexcept
			{
				return m_observables.contains(observable);
			}

			/**
			 * @brief Observes an observable.
			 * @param observable The pointer of the observable.
			 * @return void
			 */
			void observe (ObservableTrait * observable) noexcept;

			/**
			 * @brief Removes an observable.
			 * @param observable The pointer of the observable.
			 * @return void
			 */
			void forget (ObservableTrait * observable) noexcept;

		protected:

			/**
			 * @brief Constructs an observer.
			 */
			ObserverTrait () noexcept = default;

			/**
			 * @brief Consumes the event from an observable.
			 * @note Returning false will detach the observable.
			 * @param observable The observable pointer responsible for the notification.
			 * @param notificationCode Code of notification.
			 * @param data Pointer to possible data.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool onNotification (const ObservableTrait * observable, int notificationCode, const std::any & data) noexcept = 0;

		private:

			std::set< ObservableTrait * > m_observables;
	};
}
