/*
 * src/Graphics/Frustum.hpp
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

/* Project configurations. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <array>
#include <string>

/* Local inclusions for usages. */
#include "Math/Matrix.hpp"
#include "Math/Plane.hpp"
#include "Math/Space3D/AACuboid.hpp"
#include "Math/Space3D/Sphere.hpp"
#include "Math/Vector.hpp"

namespace EmEn::Graphics
{
	/**
	 * @brief The Frustum class
	 */
	class EMEN_API Frustum final
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
			void update (const Base::Math::Matrix< 4, float > & viewProjectionMatrix) noexcept;

			/**
			 * @brief Checks a point against the Frustum.
			 * @param point A reference to a vector.
			 * @return bool
			 */
			[[nodiscard]]
			bool isSeeing (const Base::Math::Vector< 3, float > & point) const noexcept;

			/**
			 * @brief Checks a sphere against the Frustum.
			 * @param sphere A reference to a sphere.
			 * @return bool
			 */
			[[nodiscard]]
			bool isSeeing (const Base::Math::Space3D::Sphere< float > & sphere) const noexcept;

			/**
			 * @brief Checks an axis aligned bounding box against the Frustum.
			 * @param aabb A reference to an axis aligned bounding box.
			 * @return bool
			 */
			[[nodiscard]]
			bool isSeeing (const Base::Math::Space3D::AACuboid< float > & aabb) const noexcept;

			/**
			 * @brief Returns the volume that can CAST a shadow into this (light) frustum: the same frustum
			 * with its near plane dropped, i.e. extruded toward the light without limit.
			 * @note ⚠️ The shadow cast pass clamps depth (ShadowCasting.cpp, depthClampEnable): a caster
			 * standing between the light and the near plane is flattened onto it and still occludes. Culling
			 * casters with the full frustum drops exactly those — the tallest ones, between the sun and the
			 * slice — and leaves a shadow-shaped hole. Test casters against this volume, never the frustum.
			 * @return Frustum
			 */
			[[nodiscard]]
			Frustum shadowCasterVolume () const noexcept;

			/**
			 * @brief Returns one plane of the frustum (normal pointing INSIDE).
			 * @param index Right, Left, Bottom, Top, Far or Near.
			 * @return const Base::Math::Plane< float > &
			 */
			[[nodiscard]]
			const Base::Math::Plane< float > &
			plane (size_t index) const noexcept
			{
				return m_planes[index < m_planes.size() ? index : Near];
			}

		private:

			/**
			 * @brief How many planes the tests walk: all six, or the first five (every plane but Near,
			 * which is the LAST index) for a shadow caster volume.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			testedPlaneCount () const noexcept
			{
				return m_nearPlaneIgnored ? Near : m_planes.size();
			}


			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend EMEN_API std::ostream & operator<< (std::ostream & out, const Frustum & obj);

			std::array< Base::Math::Plane< float >, 6 > m_planes{};
			bool m_nearPlaneIgnored{false};
	};

	EMEN_API std::string to_string (const Frustum & obj);
}
