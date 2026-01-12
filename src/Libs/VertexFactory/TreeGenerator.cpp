/*
 * src/Libs/VertexFactory/TreeGenerator.cpp
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

#include "TreeGenerator.hpp"

/* Local inclusions. */
#include "ShapeGenerator.hpp"

namespace EmEn::Libs::VertexFactory
{
	using namespace Math;

	void
	TreeGenerator::generateBranch (ShapeAssembler< float > & /*assembler*/, float /*size*/, float /*height*/, const CartesianFrame< float > & /*origin*/, size_t currentDepth, bool /*generateSubBranches*/) noexcept
	{
		std::cout << "Level #" << currentDepth << " generation." "\n";

		/*Shape branch{};
		ShapeAssembler branchAssembler{branch};

		CartesianFrame< float > chunkCoords{};

		for ( auto thisDepth = currentDepth; thisDepth < m_depth; thisDepth++ )
		{
			chunkCoords.clearRotation();

			if ( generateSubBranches && thisDepth > currentDepth )
			{
				//for ( auto branchDepth = depth - 1; branchDepth < m_depth; branchDepth++ )
				{
					std::cout << "Level #" << currentDepth << " sub-generation." "\n";

					auto branchOrigin = chunkCoords;
					branchOrigin.yaw(Math::Radian(Utility::random(0.0F, 360.0F)));
					branchOrigin.pitch(Math::Radian(m_maxSpreadingAngle * 2.0F));

					this->generateBranch(assembler, size * 0.5F, height * 0.75F, branchOrigin, thisDepth, true);
				}
			}

			chunkCoords.roll(Math::Radian(Utility::random(-m_maxSpreadingAngle, m_maxSpreadingAngle)));
			chunkCoords.pitch(Math::Radian(Utility::random(-m_maxSpreadingAngle, m_maxSpreadingAngle)));

			const auto nextSize = size * m_reduceFactor;

			auto chunk = ShapeGenerator::generateCylinder(size, nextSize, height, 8, 4);

			branchAssembler.merge(chunk, chunkCoords, false);

			chunkCoords.translateAlongLocalYAxis(height);
			size = nextSize;
			height *= m_reduceFactor;
		}

		{
			auto branchOrigin = ShapeGenerator::generateSphere(size * 5.0F, 8, 8);

			assembler.merge(branchOrigin, origin);
		}

		if ( branch.empty() )
			return;

		assembler.merge(branch, origin);*/

		std::cout << "Level #" << currentDepth << " assembled to the parent." "\n";
	}

	Shape< float >
	TreeGenerator::generate (unsigned int seed) noexcept
	{
		Shape< float > treeShape{};

		ShapeAssembler treeAssembler{treeShape};

		std::srand(seed);

		this->generateBranch(treeAssembler, m_baseSize, m_baseHeight, {}, 0U, true);

		return treeShape;
	}
}
