/*
 * src/Physics/ConstraintSolver.hpp
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
#include <vector>
#include <algorithm>

/* Local inclusions. */
#include "MovableTrait.hpp"
#include "ContactManifold.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief Sequential Impulse constraint solver for rigid body dynamics.
	 * @note Implements Erin Catto's iterative impulse-based physics solver.
	 */
	class ConstraintSolver final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ConstraintSolver"};

			/**
			 * @brief Constructs a constraint solver with custom iteration counts.
			 * @param velocityIterations Number of velocity constraint iterations. Default 8.
			 * @param positionIterations Number of position correction iterations. Default 3.
			 */
			explicit
			ConstraintSolver (uint32_t velocityIterations = 8U, uint32_t positionIterations = 3U) noexcept
				: m_velocityIterations{velocityIterations},
				  m_positionIterations{positionIterations}
			{

			}

			/**
			 * @brief Solves all contact constraints in the given manifolds.
			 * @param manifolds A vector of contact manifolds to solve.
			 * @param deltaTime The physics time step in seconds.
			 * @return void
			 */
			void solve (std::vector< ContactManifold > & manifolds, float deltaTime) noexcept;

			/**
			 * @brief Sets the number of velocity iterations.
			 * @param iterations Number of iterations (typical: 6-10).
			 * @return void
			 */
			void
			setVelocityIterations (uint32_t iterations) noexcept
			{
				m_velocityIterations = std::max(1U, iterations);
			}

			/**
			 * @brief Sets the number of position iterations.
			 * @param iterations Number of iterations (typical: 2-4).
			 * @return void
			 */
			void
			setPositionIterations (uint32_t iterations) noexcept
			{
				m_positionIterations = std::max(1U, iterations);
			}

		private:

			/**
			 * @brief Prepares contact points by computing effective mass and velocity bias.
			 * @param manifold The contact manifold to prepare.
			 * @param deltaTime The physics time step.
			 * @return void
			 */
			void prepareContacts (ContactManifold & manifold, float deltaTime) noexcept;

			/**
			 * @brief Solves velocity constraints for a manifold (applies impulses).
			 * @param manifold The contact manifold.
			 * @return void
			 */
			void solveVelocityConstraints (ContactManifold & manifold) noexcept;

			/**
			 * @brief Solves position constraints (corrects penetration directly).
			 * @param manifold The contact manifold.
			 * @return void
			 */
			void solvePositionConstraints (ContactManifold & manifold) noexcept;

			/* Number of velocity constraint solver iterations. */
			uint32_t m_velocityIterations;
			/* Number of position correction iterations. */
			uint32_t m_positionIterations;
	};
}
