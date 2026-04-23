/*
 * src/Graphics/Geometry/RawGeometryOptions.hpp
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
#include <optional>

/* Local inclusions for usages. */
#include "Graphics/Types.hpp"
#include "Libs/Math/Space3D/Point.hpp"

namespace EmEn::Graphics::Geometry
{
	/**
	 * @brief Options for raw geometry resource loading.
	 * @note Provides optional topology override and explicit bounding box specification.
	 * When bounding box min/max are not provided, they are auto-computed from vertex positions.
	 */
	struct RawGeometryOptions
	{
		/** @brief The primitive topology for this geometry. Default: TriangleList. */
		Topology topology{Topology::TriangleList};

		/** @brief Optional explicit bounding box minimum point. */
		std::optional< Libs::Math::Space3D::Point< float > > boundingBoxMin{};

		/** @brief Optional explicit bounding box maximum point. */
		std::optional< Libs::Math::Space3D::Point< float > > boundingBoxMax{};
	};
}
