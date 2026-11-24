/*
 * src/Scenes/NavigationInterface.hpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* Local inclusions for usages. */
#include "Math/Vector.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief Interface to ask a world where a body can go: whether the straight way between two points is free, and
	 * which point to head for now to reach another one.
	 * @note OPTIONAL (2026-11-21): a world that offers none is open ground, a body goes straight. What "free" means is
	 * the implementation's: TileWorld::Navigation answers from the open and closed cells of a tile layout.
	 * @note The points are WORLD points. An implementation says whether their height matters (TileWorld::Navigation
	 * ignores it: a grid seen from above).
	 * @note Thread: an implementation states its own rule; a consumer asks from the thread that moves its bodies.
	 */
	class EMEN_API NavigationInterface
	{
		public:

			/**
			 * @brief Destructs the navigation interface.
			 */
			virtual ~NavigationInterface () = default;

			/**
			 * @brief Copy constructor (deleted: a polymorphic interface, copying would slice).
			 * @param copy A reference to the copied instance.
			 */
			NavigationInterface (const NavigationInterface & copy) noexcept = delete;

			/**
			 * @brief Move constructor (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 */
			NavigationInterface (NavigationInterface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 * @return NavigationInterface &
			 */
			NavigationInterface & operator= (const NavigationInterface & copy) noexcept = delete;

			/**
			 * @brief Move assignment (deleted: a polymorphic interface).
			 * @param copy A reference to the copied instance.
			 * @return NavigationInterface &
			 */
			NavigationInterface & operator= (NavigationInterface && copy) noexcept = delete;

			/**
			 * @brief Returns whether a body can go STRAIGHT from a point to another: nothing the navigation knows stands
			 * on the segment, nor within the clearance on either side of it.
			 * @note It is a line of sight as well, for what the navigation knows: a consumer aims along a clear segment.
			 * @param from The start, in world space.
			 * @param to The end, in world space.
			 * @param clearance Half the width the body needs, in metres (0 = a line).
			 * @return bool False as well for a point that is not finite or a negative clearance.
			 */
			[[nodiscard]]
			virtual bool isClear (const Base::Math::Vector< 3, float > & from, const Base::Math::Vector< 3, float > & to, float clearance) const noexcept = 0;

			/**
			 * @brief Returns the point a body heads for NOW to reach a target: the target itself when the straight way
			 * is clear, else a point on a way round what stands between them.
			 * @note To ask again as the body moves (every logic cycle): the answer is the next point only, never a whole
			 * path. When the navigation knows no way, the answer is the target: the body goes straight, as it would in
			 * a world without navigation.
			 * @param from Where the body is, in world space.
			 * @param to Where it goes, in world space.
			 * @param clearance Half the width the body needs, in metres (0 = a line).
			 * @return Base::Math::Vector< 3, float > A world point; `to` itself for a point that is not finite.
			 */
			[[nodiscard]]
			virtual Base::Math::Vector< 3, float > waypoint (const Base::Math::Vector< 3, float > & from, const Base::Math::Vector< 3, float > & to, float clearance) const noexcept = 0;

		protected:

			/**
			 * @brief Constructs a navigation interface.
			 */
			NavigationInterface () noexcept = default;
	};
}
