/*
 * src/Saphir/Types.hpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2024 - "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Emeraude-Engine; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * https://bitbucket.org/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <string>
#include <vector>

namespace Emeraude::Saphir
{
	/**
	 * @brief The shader type enumerations.
	 */
	enum class ShaderType : uint32_t
	{
		Undefined,
		VertexShader,
		TesselationControlShader,
		TesselationEvaluationShader,
		GeometryShader,
		FragmentShader,
		ComputeShader
	};

	static constexpr auto VertexShaderString{"VertexShader"};
	static constexpr auto TesselationControlShaderString{"TesselationControlShader"};
	static constexpr auto TesselationEvaluationShaderString{"TesselationEvaluationShader"};
	static constexpr auto GeometryShaderString{"GeometryShader"};
	static constexpr auto FragmentShaderString{"FragmentShader"};
	static constexpr auto ComputeShaderString{"ComputeShader"};

	/**
	 * @brief Returns a C-String version of the enum value.
	 * @param type The enum value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (ShaderType type) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param type The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (ShaderType type) noexcept
	{
		return {to_cstring(type)};
	}

	/* Shader file extensions. */
	static constexpr auto VertexShaderFileExtension{"vert"};
	static constexpr auto TesselationControlShaderFileExtension{"tesc"};
	static constexpr auto TesselationEvaluationShaderFileExtension{"tese"};
	static constexpr auto GeometryShaderFileExtension{"geom"};
	static constexpr auto FragmentShaderFileExtension{"frag"};
	static constexpr auto ComputeShaderFileExtension{"comp"};

	[[nodiscard]]
	const char * getShaderFileExtension (ShaderType type) noexcept;

	using ShaderBinary = std::vector< uint32_t >;
}
