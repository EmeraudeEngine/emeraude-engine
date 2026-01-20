/*
 * src/Graphics/ViewMatricesInterface.cpp
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

#include "ViewMatricesInterface.hpp"

namespace EmEn::Graphics
{
	using namespace Libs::Math;

	std::array< Vector< 3, float >, 8 >
	ViewMatricesInterface::computeFrustumCornersWorld (const Matrix< 4, float > & inverseViewProjection) noexcept
	{
		/* NDC cube corners (Vulkan clip space: x,y in [-1,1], z in [0,1]).
		 * Near plane (z=0): 4 corners, Far plane (z=1): 4 corners.
		 * Order: near-bottom-left, near-bottom-right, near-top-right, near-top-left,
		 *        far-bottom-left, far-bottom-right, far-top-right, far-top-left */
		static constexpr std::array< Vector< 4, float >, 8 > ndcCorners = {{
			/* Near plane (z=0) */
			Vector< 4, float >{-1.0F, -1.0F, 0.0F, 1.0F}, /* near-bottom-left */
			Vector< 4, float >{ 1.0F, -1.0F, 0.0F, 1.0F}, /* near-bottom-right */
			Vector< 4, float >{ 1.0F,  1.0F, 0.0F, 1.0F}, /* near-top-right */
			Vector< 4, float >{-1.0F,  1.0F, 0.0F, 1.0F}, /* near-top-left */
			/* Far plane (z=1) */
			Vector< 4, float >{-1.0F, -1.0F, 1.0F, 1.0F}, /* far-bottom-left */
			Vector< 4, float >{ 1.0F, -1.0F, 1.0F, 1.0F}, /* far-bottom-right */
			Vector< 4, float >{ 1.0F,  1.0F, 1.0F, 1.0F}, /* far-top-right */
			Vector< 4, float >{-1.0F,  1.0F, 1.0F, 1.0F}  /* far-top-left */
		}};

		std::array< Vector< 3, float >, 8 > worldCorners;

		for ( size_t i = 0; i < 8; ++i )
		{
			/* Transform from NDC to world space. */
			Vector< 4, float > worldPos = inverseViewProjection * ndcCorners[i];

			/* Perspective divide. */
			if ( worldPos.w() != 0.0F )
			{
				worldPos /= worldPos.w();
			}

			worldCorners[i] = Vector< 3, float >{worldPos.x(), worldPos.y(), worldPos.z()};
		}

		return worldCorners;
	}
}