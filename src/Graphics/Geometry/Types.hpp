/*
 * src/Graphics/Geometry/Types.hpp
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
#include <cstdint>
#include <string>

namespace EmEn::Graphics::Geometry
{
	/** @brief Geometry interface flag bits. */
	enum GeometryFlagBits : uint32_t
	{
		/* Vertex attributes present in local data. */
		/** @brief This flags tells the geometry to have normals in vertex attributes. */
		EnableNormal = 1 << 0,
		/**
		 * @brief This flags tells the geometry to have tangents, bi-normals and normals (TBN) in vertex attributes.
		 * @note Overrides GeometryFlagBits::EnableNormal flag.
		 */
		EnableTangentSpace = 1 << 1,
		/** @brief This flags tells the geometry to have primary texture coordinates in vertex attributes. */
		EnablePrimaryTextureCoordinates = 1 << 2,
		/** @brief This flags tells the geometry to have secondary texture coordinates in vertex attributes. */
		EnableSecondaryTextureCoordinates = 1 << 3,
		/** @brief This flags tells the geometry to have vertex colors in vertex attributes. */
		EnableVertexColor = 1 << 4,
		/** @brief This flags tells the geometry uses influence indexes to a uniform buffer for skeletal animations. */
		EnableInfluence = 1 << 5,
		/** @brief This flags tells the geometry to have weights in vertex attributes. */
		EnableWeight = 1 << 6,
		/** @brief This flags tells the primary texture coordinates are expressed in 3D (UVW) instead of 2D (UV). */
		Enable3DPrimaryTextureCoordinates = 1 << 7,
		/** @brief This flags tells the secondary texture coordinates are expressed in 3D (UVW) instead of 2D (UV). */
		Enable3DSecondaryTextureCoordinates = 1 << 8,
		/** @brief This flags tells the VBO uses dynamic updates. */
		EnableDynamicVertexBuffer = 1 << 9,
		/** @brief This flags tells the vertex positions are expressed in world coordinates. */
		EnableAbsolutePosition = 1 << 10,
		/** @brief This flags tells the index buffer uses the special index to restart primitives. */
		EnablePrimitiveRestart = 1 << 11
	};

	/** @brief SubGeometry flag bits. */
	enum SubGeometryFlagBits : uint32_t
	{
		EnableDoubleSidedFaces = 1
	};

	/**
	 * @brief Returns the number of element for a vertex definition using geometry flags.
	 * @param geometryFlagBits The geometry flags.
	 * @return uint32_t
	 */
	[[nodiscard]]
	uint32_t getElementCountFromFlags (uint32_t geometryFlagBits) noexcept;

	/**
	 * @brief Returns a printable debug flags string.
	 * @param geometryFlagBits The geometry flags.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string getFlagsString (uint32_t geometryFlagBits) noexcept;
}
