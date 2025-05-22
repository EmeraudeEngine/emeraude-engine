/*
 * src/Scenes/CubicInfluenceArea.hpp
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

/* Local inclusions for inheritances. */
#include "InfluenceAreaInterface.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Vector.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class AbstractEntity;
}

namespace EmEn::Scenes
{
	/**
	 * @brief Defines a cubic area of influence for a modifier.
	 * @extends EmEn::Scenes::InfluenceAreaInterface The influence area base class.
	 */
	class CubicInfluenceArea final : public InfluenceAreaInterface
	{
		public:

			/**
			 * @brief Constructs a cubic influence area.
			 * @param parentEntity A reference to the modifier parent scene entity.
			 * @param size Unidirectional size of the area.
			 */
			explicit CubicInfluenceArea (const AbstractEntity & parentEntity, float size = 1.0F) noexcept;

			/**
			 * @brief Constructs a cubic influence area.
			 * @param parentEntity A reference to the modifier parent scene entity.
			 * @param xSize Size of the area in X axis.
			 * @param ySize Size of the area in Y axis.
			 * @param zSize Size of the area in Z axis.
			 */
			CubicInfluenceArea (const AbstractEntity & parentEntity, float xSize, float ySize, float zSize) noexcept;

			/** @copydoc EmEn::Scenes::InfluenceAreaInterface::isUnderInfluence(const Libs::Math::CartesianFrame< float > &, const Libs::Math::Space3D::Sphere< float > &) */
			[[nodiscard]]
			bool isUnderInfluence (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Space3D::Sphere< float > & worldBoundingSphere) const noexcept override;

			/** @copydoc EmEn::Scenes::InfluenceAreaInterface::influenceStrength(const Libs::Math::CartesianFrame< float > &, const Libs::Math::Space3D::Sphere< float > &) */
			[[nodiscard]]
			float influenceStrength (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Space3D::Sphere< float > & worldBoundingSphere) const noexcept override;

			/** @copydoc EmEn::Scenes::InfluenceAreaInterface::isUnderInfluence(const Libs::Math::CartesianFrame< float > &, const Libs::Math::Space3D::AACuboid< float > &) */
			[[nodiscard]]
			bool isUnderInfluence (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Space3D::AACuboid< float > & worldBoundingBox) const noexcept override;

			/** @copydoc EmEn::Scenes::InfluenceAreaInterface::influenceStrength(const Libs::Math::CartesianFrame< float > &, const Libs::Math::Space3D::AACuboid< float > &) */
			[[nodiscard]]
			float influenceStrength (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Space3D::AACuboid< float > & worldBoundingBox) const noexcept override;

			/**
			 * @brief Sets the size of the area.
			 * @param size Unidirectional size of the area.
			 * @return void
			 */
			void setSize (float size) noexcept;

			/**
			 * @brief Sets the size of the area.
			 * @param xSize Size of the area in X axis.
			 * @param ySize Size of the area in Y axis.
			 * @param zSize Size of the area in Z axis.
			 * @return void
			 */
			void setSize (float xSize, float ySize, float zSize) noexcept;

			/**
			 * @brief Returns the size in X axis.
			 * @return float
			 */
			[[nodiscard]]
			float xSize () const noexcept;

			/**
			 * @brief Returns the size in Y axis.
			 * @return float
			 */
			[[nodiscard]]
			float ySize () const noexcept;

			/**
			 * @brief Returns the size in Z axis.
			 * @return float
			 */
			[[nodiscard]]
			float zSize () const noexcept;

		private:

			/**
			 * @brief Gets position in the modifier space.
			 * @param position A reference to a vector.
			 * @return Libs::Math::Vector< 4, float >
			 */
			[[nodiscard]]
			Libs::Math::Vector< 4, float > getPositionInModifierSpace (const Libs::Math::Vector< 3, float > & position) const noexcept;

			const AbstractEntity * m_parentEntity{nullptr};
			float m_xSize;
			float m_ySize;
			float m_zSize;
	};
}
