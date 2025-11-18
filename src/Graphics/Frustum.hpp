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

			static constexpr auto Right{0};
			static constexpr auto Left{1};
			static constexpr auto Bottom{2};
			static constexpr auto Top{3};
			static constexpr auto Far{4};
			static constexpr auto Near{5};

			/** @brief Default constructor. */
			Frustum () noexcept = default;

			/**
			 * @brief Updates the frustum geometry when the camera moves.
			 * @param viewProjectionMatrix
			 */
			void update (const Libs::Math::Matrix< 4, float > & viewProjectionMatrix) noexcept;

			/**
			 * @brief Checks a point against the Frustum.
			 * @param point A reference to a vector.
			 * @return bool
			 */
			[[nodiscard]]
			bool isSeeing (const Libs::Math::Vector< 3, float > & point) const noexcept;

			/**
			 * @brief Checks a sphere against the Frustum.
			 * @param sphere A reference to a sphere.
			 * @return bool
			 */
			[[nodiscard]]
			bool isSeeing (const Libs::Math::Space3D::Sphere< float > & sphere) const noexcept;

			/**
			 * @brief Checks an axis aligned bounding box against the Frustum.
			 * @param aabb A reference to an axis aligned bounding box.
			 * @return bool
			 */
			[[nodiscard]]
			bool isSeeing (const Libs::Math::Space3D::AACuboid< float > & aabb) const noexcept;

		private:

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend std::ostream & operator<< (std::ostream & out, const Frustum & obj);

			std::array< Libs::Math::Plane< float >, 6 > m_planes{};
	};

	inline
	std::ostream &
	operator<< (std::ostream & out, const Frustum & obj)
	{
		return out << "Frustum data :" "\n"
			"Right " << obj.m_planes[Frustum::Right] <<
			"Left " << obj.m_planes[Frustum::Left] <<
			"Bottom " << obj.m_planes[Frustum::Bottom] <<
			"Top " << obj.m_planes[Frustum::Top] <<
			"Far " << obj.m_planes[Frustum::Far] <<
			"Near " << obj.m_planes[Frustum::Near];
	}

	/**
	 * @brief Stringifies the object.
	 * @param obj A reference to the object to print.
	 * @return std::string
	 */
	inline
	std::string
	to_string (const Frustum & obj)
	{
		std::stringstream output;

		output << obj;

		return output.str();
	}
}
