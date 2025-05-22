/*
 * src/Graphics/Geometry/Types.cpp
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

#include "Types.hpp"

/* STL inclusions. */
#include <sstream>

namespace EmEn::Graphics::Geometry
{
	uint32_t
	getElementCountFromFlags (uint32_t geometryFlagBits) noexcept
	{
		/* NOTE: Position is always present! */
		uint32_t elementCount = 3;

		if ( (geometryFlagBits & EnableTangentSpace) != 0U )
		{
			elementCount += 9;
		}
		else if ( (geometryFlagBits & EnableNormal) != 0U )
		{
			elementCount += 3;
		}

		if ( (geometryFlagBits & EnablePrimaryTextureCoordinates) != 0U )
		{
			elementCount += (geometryFlagBits & Enable3DPrimaryTextureCoordinates) != 0 ? 3 : 2;
		}

		if ( (geometryFlagBits & EnableSecondaryTextureCoordinates) != 0U )
		{
			elementCount += (geometryFlagBits & Enable3DSecondaryTextureCoordinates) != 0 ? 3 : 2;
		}

		if ( (geometryFlagBits & EnableVertexColor) != 0U )
		{
			elementCount += 4;
		}

		if ( (geometryFlagBits & EnableInfluence) != 0U )
		{
			elementCount += 4;
		}

		if ( (geometryFlagBits & EnableWeight) != 0U )
		{
			elementCount += 4;
		}

		return elementCount;
	}

	std::string
	getFlagsString (uint32_t geometryFlagBits) noexcept
	{
		std::stringstream output;

		output <<
			/* Vertex attributes presence in local data. */
			"EnableNormal : " << ( (geometryFlagBits & EnableNormal) != 0 ? "yes" : "no" ) << "\n"
			"EnableTangentSpace : " << ( (geometryFlagBits & EnableTangentSpace) != 0 ? "yes" : "no" ) << "\n"
			"EnablePrimaryTextureCoordinates : " << ( (geometryFlagBits & EnablePrimaryTextureCoordinates) != 0 ? "yes" : "no" ) << "\n"
			"EnableSecondaryTextureCoordinates : " << ( (geometryFlagBits & EnableSecondaryTextureCoordinates) != 0 ? "yes" : "no" ) << "\n"
			"EnableVertexColor : " << ( (geometryFlagBits & EnableVertexColor) != 0 ? "yes" : "no" ) << "\n"
			"EnableInfluence : " << ( (geometryFlagBits & EnableInfluence) != 0 ? "yes" : "no" ) << "\n"
			"EnableWeight : " << ( (geometryFlagBits & EnableWeight) != 0 ? "yes" : "no" ) << "\n"
			/* Vertex attribute options. */
			"Enable3DPrimaryTextureCoordinates : " << ( (geometryFlagBits & Enable3DPrimaryTextureCoordinates) != 0 ? "yes" : "no" ) << "\n"
			"Enable3DSecondaryTextureCoordinates : " << ( (geometryFlagBits & Enable3DSecondaryTextureCoordinates) != 0 ? "yes" : "no" ) << "\n"
			"EnableDynamicVertexBuffer : " << ( (geometryFlagBits & EnableDynamicVertexBuffer) != 0 ? "yes" : "no" ) << "\n"
			"EnableAbsolutePosition : " << ( (geometryFlagBits & EnableAbsolutePosition) != 0 ? "yes" : "no" ) << "\n"
			"EnablePrimitiveRestart : " << ( (geometryFlagBits & EnablePrimitiveRestart) != 0 ? "yes" : "no" ) << "\n";

		return output.str();
	}
}
