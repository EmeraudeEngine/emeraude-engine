/*
 * src/ServiceInterface.cpp
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

#include "ServiceInterface.hpp"

/* STL inclusions. */
#include <string>
#include <vector>

/* Local inclusions. */
#include "Libs/NameableTrait.hpp"
#include "Tracer.hpp"

namespace EmEn
{
	constexpr auto TracerTag{"ServiceInterface"};

	ServiceInterface::ServiceInterface (const std::string & serviceInstanceName) noexcept
		: NameableTrait(serviceInstanceName)
	{

	}

	bool
	ServiceInterface::initialize () noexcept
	{
		if ( this->usable() )
		{
			TraceError{TracerTag} <<
				"The service '" << this->name() << "' looks like already initialized !" "\n"
				"The method ServiceInterface::usable() must dynamically report if the service has been initialized and usable !";

			return false;
		}

		return this->onInitialize();
	}

	bool
	ServiceInterface::initialize (std::vector< ServiceInterface * > & services) noexcept
	{
		if ( !this->initialize() )
		{
			return false;
		}

		services.emplace_back(this);

		return true;
	}

	bool
	ServiceInterface::terminate () noexcept
	{
		return this->onTerminate();
	}
}
