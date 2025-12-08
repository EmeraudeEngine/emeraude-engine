/*
 * src/Saphir/Types.cpp
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

#include "Types.hpp"

/* STL inclusions. */
#include <iostream>

namespace EmEn::Saphir
{
	const char *
	to_cstring (ShaderType type) noexcept
	{
		switch ( type )
		{
			case ShaderType::VertexShader :
				return VertexShaderString;

			case ShaderType::TesselationControlShader :
				return TesselationControlShaderString;

			case ShaderType::TesselationEvaluationShader :
				return TesselationEvaluationShaderString;

			case ShaderType::GeometryShader :
				return GeometryShaderString;

			case ShaderType::FragmentShader :
				return FragmentShaderString;

			case ShaderType::ComputeShader :
				return ComputeShaderString;

			default:
				return nullptr;
		}
	}

	const char *
	getShaderFileExtension (ShaderType type) noexcept
	{
		switch ( type )
		{
			case ShaderType::VertexShader :
				return VertexShaderFileExtension;

			case ShaderType::TesselationControlShader :
				return TesselationControlShaderFileExtension;

			case ShaderType::TesselationEvaluationShader :
				return TesselationEvaluationShaderFileExtension;

			case ShaderType::GeometryShader :
				return GeometryShaderFileExtension;

			case ShaderType::FragmentShader :
				return FragmentShaderFileExtension;

			case ShaderType::ComputeShader :
				return ComputeShaderFileExtension;

			default:
				return nullptr;
		}
	}

	const char *
	to_cstring (ColorSpaceConversion type) noexcept
	{
		switch ( type )
		{
			case ColorSpaceConversion::None :
				return NoneString;

			case ColorSpaceConversion::ToLinear :
				return ToLinearString;

			case ColorSpaceConversion::ToSRGB :
				return ToSRGBString;

			default:
				return nullptr;
		}
	}

	ColorSpaceConversion
	to_ColorSpaceConversion (const std::string & value) noexcept
	{
		if ( value == NoneString )
		{
			return ColorSpaceConversion::None;
		}

		if ( value == ToLinearString )
		{
			return ColorSpaceConversion::ToLinear;
		}

		if ( value == ToSRGBString )
		{
			return ColorSpaceConversion::ToSRGB;
		}

		std::cerr << "to_ColorSpaceConversion() : Unknown '" << value << "' type ! Returning 'None' by default." << "\n";

		return ColorSpaceConversion::None;
	}
}
