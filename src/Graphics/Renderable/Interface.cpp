/*
 * src/Graphics/Renderable/Interface.cpp
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

#include "Interface.hpp"

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Resources;

	constexpr auto TracerTag{"RenderableInterface"};

	bool
	Interface::onDependenciesLoaded () noexcept
	{
		/* NOTE: Check for sub-geometries and layer count coherence. */
		if ( this->geometry()->subGeometryCount() != this->layerCount() )
		{
			TraceError{TracerTag} <<
				"Resource '" << this->name() << "' (" << this->classLabel() << ") structure ill-formed ! "
				"There is " << this->geometry()->subGeometryCount() << " sub-geometries and " <<  this->layerCount() << " rendering layers !";

			return false;
		}

		if constexpr ( IsDebug )
		{
			/* NOTE: Check the geometry resource. */
			if ( !this->geometry()->isLoaded() )
			{

				TraceError{TracerTag} <<
					"Resource '" << this->name() << "' (" << this->classLabel() << ") structure ill-formed ! "
					"The geometry is not created !";

				return false;
			}

			/* NOTE: Check material resources. */
			const auto layerCount = this->layerCount();

			for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
			{
				if ( !this->material(layerIndex)->isCreated() )
				{
					TraceError{TracerTag} <<
						"Resource '" << this->name() << "' (" << this->classLabel() << ") structure ill-formed ! "
						"The material #" << layerIndex << " is not created !";

					return false;
				}
			}
		}

		this->setReadyForInstantiation(true);

		return true;
	}
}
