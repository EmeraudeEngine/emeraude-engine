/*
 * src/Graphics/Material/Helpers.hpp
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

/* Local inclusions for usages. */
#include "Graphics/Types.hpp"
#include "Libraries/FastJSON.hpp"
#include "Libraries/PixelFactory/Color.hpp"

/* Forward declarations. */
namespace Emeraude
{
	namespace Saphir
	{
		namespace Generator
		{
			class Abstract;
		}

		class VertexShader;
	}

	namespace Graphics
	{
		namespace Material
		{
			class Interface;
		}

		namespace Geometry
		{
			class Interface;
		}
	}
}

namespace Emeraude::Graphics::Material
{
	/* JSON key. */
	static constexpr auto JKName{"Name"};
	static constexpr auto JKType{"Type"};
	static constexpr auto JKData{"Data"};
	static constexpr auto JKShininess{"Shininess"};
	static constexpr auto JKScale{"Scale"};
	static constexpr auto JKAmount{"Amount"};
	static constexpr auto JKColor{"Color"};
	static constexpr auto JKValue{"Value"};
	static constexpr auto JKBlendingMode{"BlendingMode"};
	static constexpr auto JKAutoIllumination{"AutoIllumination"};
	static constexpr auto JKOpacity{"Opacity"};
	static constexpr auto JKAlphaThreshold{"AlphaThreshold"};

	/**
	 * @brief Checks primary texture coordinates linkage between the material and the geometry.
	 * @param generator A reference to the shader manager.
	 * @param vertexShader A reference to the vertex shader.
	 * @param material A reference to the material.
	 * @param geometry A reference to the geometry.
	 * @return bool
	 */
	[[nodiscard]]
	bool checkPrimaryTextureCoordinates (Saphir::Generator::Abstract & generator, Saphir::VertexShader & vertexShader, const Interface & material, const Geometry::Interface & geometry) noexcept;

	/**
	 * @brief Checks secondary texture coordinates linkage between the material and the geometry.
	 * @param generator A reference to the shader manager.
	 * @param vertexShader A reference to the vertex shader.
	 * @param material A reference to the material.
	 * @param geometry A reference to the geometry.
	 * @return bool
	 */
	[[nodiscard]]
	bool checkSecondaryTextureCoordinates (Saphir::Generator::Abstract & generator, Saphir::VertexShader & vertexShader, const Interface & material, const Geometry::Interface & geometry) noexcept;

	/**
	 * @brief Parses the base of a component structure from JSON data.
	 * @param data A reference to the json value.
	 * @param componentName A string for the componentName.
	 * @param fillingType A reference to filling type to complete, if succeed.
	 * @param componentData A reference to componentName data to complete, if succeed.
	 * @param optional Set the component as optional and will not return error if not present. Default false.
	 * @return bool
	 */
	[[nodiscard]]
	bool parseComponentBase (const Json::Value & data, const char * componentName, FillingType & fillingType, Json::Value & componentData, bool optional = false) noexcept;

	/**
	 * @brief Parses from an identified color component JSON data.
	 * @param data A reference to the json value.
	 * @return Libraries::PixelFactory::Color< float >
	 */
	[[nodiscard]]
	Libraries::PixelFactory::Color< float > parseColorComponent (const Json::Value & data) noexcept;

	/**
	 * @brief Parses from an identified color component JSON data.
	 * @param data A reference to the json value.
	 * @return float
	 */
	[[nodiscard]]
	float parseValueComponent (const Json::Value & data) noexcept;

	/**
	 * @brief Parses a component as a value.
	 * @param data A reference to the json value.
	 * @param componentType A C-String point to the targeted component.
	 * @param value A reference to the value to complete, if succeed.
	 * @return bool
	 */
	[[nodiscard]]
	bool getComponentAsValue (const Json::Value & data, const char * componentType, float & value) noexcept;
}
