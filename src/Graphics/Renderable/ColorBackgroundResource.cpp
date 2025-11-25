/*
 * src/Graphics/Renderable/ColorBackgroundResource.cpp
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

#include "ColorBackgroundResource.hpp"

namespace EmEn::Graphics::Renderable
{
	using namespace Libs;

	bool
	ColorBackgroundResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		this->setAverageColor(PixelFactory::Magenta);

		return this->setLoadSuccess(true);
	}

	bool
	ColorBackgroundResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "Loading via JSON not done yet !");

		this->setAverageColor(PixelFactory::Black);

		return this->setLoadSuccess(true);
	}
}
