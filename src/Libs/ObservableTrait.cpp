/*
 * src/Libs/ObservableTrait.cpp
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

#include "ObservableTrait.hpp"

/* STL inclusions. */
#include <iostream>

/* Local inclusions. */
#include "ObserverTrait.hpp"

namespace EmEn::Libs
{
	ObservableTrait::~ObservableTrait ()
	{
		/* NOTE: Wait for Observable::notify() to finish. */
		const std::lock_guard< std::mutex > lockGuard{m_notificationMutex};

		/* NOTE: Only erase "this" observable to every looking observer.
		 * Do not use ObserverTrait::release(); */
		for ( auto * observer : m_observers )
		{
			observer->m_observables.erase(this);
		}
	}

	void
	ObservableTrait::notify (int notificationCode, const std::any & data) noexcept
	{
		const std::lock_guard< std::mutex > lock{m_notificationMutex};

		if constexpr ( ObserverDebugEnabled )
		{
			if ( m_observers.empty() )
			{
				std::cout << "Observable @" << this << " tries to notify the code '" << notificationCode << "', but no one was listening !" "\n";

				return;
			}
		}

		/* [ERASE IN LOOP] */
		auto observerIt = m_observers.begin();

		while ( observerIt != m_observers.end() )
		{
			/* If onNotification() returns false, we automatically remove the observer. */
			if ( auto * observer = *observerIt; !observer->onNotification(this, notificationCode, data) )
			{
				observer->m_observables.erase(this);

				/* NOTE: set::erase() gives the next iterator. */
				observerIt = m_observers.erase(observerIt);
			}
			else
			{
				++observerIt;
			}
		}
	}
}
