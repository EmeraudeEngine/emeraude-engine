/*
 * src/Graphics/Frustum.hpp
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

#pragma once

/* STL inclusions. */
#include <array>
#include <cstdint>
#include <string>

/* Local inclusions for usages. */
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"
#include "Libs/Math/Matrix.hpp"
#include "Libs/Math/Plane.hpp"
#include "Libs/Math/Vector.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The Frustum class
	 */
	class Frustum final
	{
		public:

			enum class Result : uint8_t
			{
				Outside,
				Intersect,
				Inside
			};

			static constexpr auto Right{0};
			static constexpr auto Left{1};
			static constexpr auto Bottom{2};
			static constexpr auto Top{3};
			static constexpr auto Far{4};
			static constexpr auto Near{5};

			/** @brief Default constructor. */
			Frustum () noexcept = default;

			/**
			 * @brief Declares the far distance of the frustum.
			 * @note Lazy way of keeping the far distance.
			 * @param farDistance The distance of the camera.
			 * @return void
			 */
			void updateProperties (float farDistance) noexcept;

			/**
			 * @brief Updates the frustum geometry when the camera moves.
			 * @param viewProjectionMatrix
			 */
			void update (const Libs::Math::Matrix< 4, float > & viewProjectionMatrix) noexcept;

			/**
			 * @brief Updates the frustum geometry.
			 * @param projection A reference to a projection matrix.
			 * @param view A reference to a view matrix.
			 * @return void
			 */
			void
			update (const Libs::Math::Matrix< 4, float > & projection, const Libs::Math::Matrix< 4, float > & view) noexcept
			{
				this->update(projection * view);
			}

			/**
			 * @brief Checks a point against the Frustum.
			 * @param point
			 * @return Result
			 */
			[[nodiscard]]
			Result isCollidingWith (const Libs::Math::Vector< 3, float > & point) const noexcept;

			/**
			 * @brief Checks a sphere against the Frustum.
			 * @param sphere
			 * @return Result
			 */
			[[nodiscard]]
			Result isCollidingWith (const Libs::Math::Space3D::Sphere< float > & sphere) const noexcept;

			/**
			 * @brief Checks an axis aligned bounding box against the Frustum.
			 * @FIXME May be it could be useful to separate Intersect and Inside case ! ² Not perfect...
			 * @param box A reference to an axis aligned bounding box.
			 * @return Result
			 */
			[[nodiscard]]
			Result isCollidingWith (const Libs::Math::Space3D::AACuboid< float > & box) const noexcept;

			/**
			 * @brief Checks a ??? against the Frustum.
			 * @param coordX
			 * @param coordY
			 * @param coordZ
			 * @param size
			 * @return Result
			 */
			[[nodiscard]]
			Result isCollidingWith (float coordX, float coordY, float coordZ, float size) const noexcept;

			/**
			 * @brief setTestState
			 * @param state
			 */
			static
			void
			setTestState (bool state) noexcept
			{
				s_enableFrustumTest = state;
			}

			/**
			 * @brief isTestEnabled
			 * @return bool
			 */
			static
			bool
			isTestEnabled () noexcept
			{
				return s_enableFrustumTest;
			}

			/**
			 * @brief toggleTestState
			 * @return bool
			 */
			static
			bool
			toggleTestState () noexcept
			{
				return s_enableFrustumTest = !s_enableFrustumTest;
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Frustum & obj);

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend std::string to_string (const Frustum & obj);

		private:

			static constexpr auto PlaneCount{6UL};

			static bool s_enableFrustumTest;

			std::array< Libs::Math::Plane< float >, PlaneCount > m_planes{};
	};
}
