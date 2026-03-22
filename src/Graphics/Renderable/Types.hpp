/*
 * src/Graphics/Renderable/Types.hpp
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
#include <cstdint>

namespace EmEn::Graphics::Renderable
{
	/** @brief Maximum number of LOD levels. */
	static constexpr uint32_t MaxLODLevels{4};
	/** @brief Minimum triangle count a LOD level must produce to be generated. */
	static constexpr size_t MinTrianglesPerLOD{250};
	static constexpr float LODReductionRatio{0.33F};

	/* JSON keys. */
	static constexpr auto JKLayers{"Layers"};
	static constexpr auto JKGeometryType{"GeometryType"};
	static constexpr auto JKGeometryName{"GeometryName"};
	static constexpr auto JKMaterialType{"MaterialType"};
	static constexpr auto JKMaterialName{"MaterialName"};
	static constexpr auto JKEnableDoubleSidedFace{"EnableDoubleSidedFace"};
	static constexpr auto JKDrawingMode{"DrawingMode"};
	static constexpr auto JKUniformScale{"UniformScale"};
	static constexpr auto JKScale{"Scale"};
	static constexpr auto JKCenterAtBottom{"CenterAtBottom"};
	static constexpr auto JKFlip{"Flip"};
	static constexpr auto JKGridSize{"GridSize"};
	static constexpr auto JKGridDivision{"GridDivision"};
	static constexpr auto JKGridVisibleSize{"GridVisibleSize"};
	static constexpr auto JKHeightMap{"HeightMap"};
	static constexpr auto JKImageName{"ImageName"};
	static constexpr auto JKInverse{"Inverse"};
	static constexpr auto JKPerlinNoise{"PerlinNoise"};
	static constexpr auto JKSquareDiamondNoise{"SquareDiamond"};
	static constexpr auto JKVertexColor{"VertexColor"};
	static constexpr auto JKTexture{"Texture"};
	static constexpr auto JKLightPosition{"LightPosition"};
	static constexpr auto JKLightAmbientColor{"LightAmbientColor"};
	static constexpr auto JKLightDiffuseColor{"LightDiffuseColor"};
	static constexpr auto JKLightSpecularColor{"LightSpecularColor"};
	static constexpr auto JKUVMultiplier{"UVMultiplier"};
}
