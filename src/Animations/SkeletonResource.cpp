/*
 * src/Animations/SkeletonResource.cpp
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

#include "SkeletonResource.hpp"

/* Local inclusions. */
#include "Tracer.hpp"

namespace EmEn::Animations
{
	bool
	SkeletonResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* Default: an empty skeleton (no joints). */
		m_skeleton = Libs::Animation::Skeleton< float >{{}};

		return this->setLoadSuccess(true);
	}

	bool
	SkeletonResource::load (const std::filesystem::path & /*filepath*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "Loading skeletons from standalone files is not yet supported.");

		return this->setLoadSuccess(false);
	}

	bool
	SkeletonResource::load (const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "Loading skeletons from JSON is not yet supported.");

		return this->setLoadSuccess(false);
	}

	bool
	SkeletonResource::load (Libs::Animation::Skeleton< float > skeleton) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		m_skeleton = std::move(skeleton);

		return this->setLoadSuccess(true);
	}
}
