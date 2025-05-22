/*
 * src/Libs/ObserverTrait.cpp
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

#include "ObserverTrait.hpp"

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <iostream>

/* Local inclusions. */
#include "Libs/ObservableTrait.hpp"

namespace EmEn::Libs
{
	ObserverTrait::~ObserverTrait ()
	{
		/* NOTE: Only erase "this" observer to every attached observable.
		 * Do not use Observable::removeObserver(); */
		for ( auto * observable : m_observables )
		{
			observable->m_observers.erase(this);
		}
	}

	void
	ObserverTrait::observe (ObservableTrait * observable) noexcept
	{
		const auto result = m_observables.emplace(observable);

		if ( !result.second )
		{
			if constexpr ( ObserverDebugEnabled )
			{
				std::cout << "observe() : (@" << this << ") already observing ('" << observable->classUID() << "')." "\n";
			}

			return;
		}

		observable->addObserver(this);
	}

	void
	ObserverTrait::forget (ObservableTrait * observable) noexcept
	{
		if ( m_observables.erase(observable) == 0 )
		{
			if constexpr ( ObserverDebugEnabled )
			{
				std::cout << "release() : (@" << this << ") wasn't observing ('" << observable->classUID() << "')." "\n";
			}

			return;
		}

		observable->removeObserver(this);
	}
}
