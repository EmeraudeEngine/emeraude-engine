/*
 * src/Scenes/InfluenceAreaInterface.hpp
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

/* Local inclusions. */
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/Space3D/AACuboid.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"

namespace EmEn::Scenes
{
	/**
	 * @brief The influence area interface.
	 * It defines en influence area around a scene entity.
	 */
	class InfluenceAreaInterface
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			InfluenceAreaInterface (const InfluenceAreaInterface & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			InfluenceAreaInterface (InfluenceAreaInterface && copy) noexcept = delete;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return InfluenceAreaInterface &
			 */
			InfluenceAreaInterface & operator= (const InfluenceAreaInterface & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return InfluenceAreaInterface &
			 */
			InfluenceAreaInterface & operator= (InfluenceAreaInterface && copy) noexcept = delete;

			/**
			 * @brief Destructs the influence area.
			 */
			virtual ~InfluenceAreaInterface () = default;

			/**
			 * @brief Returns whether a scene entity is under the influence of this area.
			 * @param worldCoordinates A reference to a cartesian frame.
			 * @param worldBoundingSphere A reference to a sphere.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isUnderInfluence (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Space3D::Sphere< float > & worldBoundingSphere) const noexcept = 0;

			/**
			 * @brief Returns the strength of influence to a scene entity from this area.
			 * @param worldCoordinates A reference to a cartesian frame.
			 * @param worldBoundingSphere A reference to a sphere.
			 * @return float
			 */
			[[nodiscard]]
			virtual float influenceStrength (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Space3D::Sphere< float > & worldBoundingSphere) const noexcept = 0;

			/**
			 * @brief Returns whether a scene entity is under the influence of this area.
			 * @param worldCoordinates A reference to a cartesian frame.
			 * @param worldBoundingBox A reference to a cuboid.
			 * @return bool
			 */
			[[nodiscard]]
			virtual bool isUnderInfluence (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Space3D::AACuboid< float > & worldBoundingBox) const noexcept = 0;

			/**
			 * @brief Returns the strength of influence to a scene entity from this area.
			 * @param worldCoordinates A reference to a cartesian frame.
			 * @param worldBoundingBox A reference to a cuboid.
			 * @return float
			 */
			[[nodiscard]]
			virtual float influenceStrength (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Space3D::AACuboid< float > & worldBoundingBox) const noexcept = 0;

		protected:

			/**
			 * @brief Constructs an influence area.
			 */
			InfluenceAreaInterface () noexcept = default;

	};
}
