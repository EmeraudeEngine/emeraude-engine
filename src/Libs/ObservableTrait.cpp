/*
 * src/Libs/ObservableTrait.cpp
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

#include "ObservableTrait.hpp"

/* STL inclusions. */
#include <iostream>
#include <vector>

/* Local inclusions. */
#include "ObserverTrait.hpp"

namespace EmEn::Libs
{
	ObservableTrait::~ObservableTrait ()
	{
		/* NOTE: Snapshot our observers under our own lock, then detach ourselves
		 * from each one. Taking the observer lock per-entry serialises with any
		 * observe()/forget() or parallel destruction on those observers. */
		std::set< ObserverTrait * > snapshot;
		{
			const std::lock_guard< std::mutex > selfLock{m_notificationMutex};

			snapshot.swap(m_observers);
		}

		for ( auto * observer : snapshot )
		{
			const std::lock_guard< std::mutex > observerLock{observer->m_observationMutex};

			observer->m_observables.erase(this);
		}
	}

	void
	ObservableTrait::notify (int notificationCode, const std::any & data) noexcept
	{
		/* Phase 1: snapshot the observer set under our own lock. We deliberately
		 * release the lock before invoking onNotification() so that callbacks
		 * may safely call observe()/forget()/notify() without deadlocking,
		 * and so that concurrent notify() on this same observable can proceed. */
		std::vector< ObserverTrait * > observers;
		{
			const std::lock_guard< std::mutex > selfLock{m_notificationMutex};

			if constexpr ( ObserverDebugEnabled )
			{
				if ( m_observers.empty() )
				{
					std::cout << "Observable @" << this << " tries to notify the code '" << notificationCode << "', but no one was listening !" "\n";

					return;
				}
			}

			observers.assign(m_observers.begin(), m_observers.end());
		}

		/* Phase 2: invoke callbacks without any lock held and collect the
		 * observers that asked to be detached (by returning false). */
		std::vector< ObserverTrait * > toDetach;
		toDetach.reserve(observers.size());

		for ( auto * observer : observers )
		{
			if ( !observer->onNotification(this, notificationCode, data) )
			{
				toDetach.push_back(observer);
			}
		}

		/* Phase 3: apply the detachments atomically per observer. std::scoped_lock
		 * uses a deadlock-avoidance algorithm to acquire both mutexes safely,
		 * regardless of the order used by concurrent callers. The double-check on
		 * m_observers.erase() tolerates observers that were already removed via
		 * forget() or a concurrent notify(). */
		for ( auto * observer : toDetach )
		{
			const std::scoped_lock cross{m_notificationMutex, observer->m_observationMutex};

			if ( m_observers.erase(observer) > 0 )
			{
				observer->m_observables.erase(this);
			}
		}
	}
}
