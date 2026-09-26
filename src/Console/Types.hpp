/*
 * src/Console/Types.hpp
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

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>

namespace EmEn::Console
{
	/**	@brief Type of console argument. */
	enum class EMEN_API ArgumentType : uint8_t
	{
		Undefined,
		Boolean,
		Integer,
		Float,
		String
	};

	/**
	 * @brief Type of a declared command parameter.
	 * @note Deduced from the C++ type of the callable's argument by a typed bindCommand().
	 * 'Any' is a parameter declared as a raw Console::Argument: it accepts any scalar.
	 */
	enum class EMEN_API ParameterType : uint8_t
	{
		Any,
		Boolean,
		Integer,
		Float,
		String
	};

	/**
	 * @brief How many values a declared command parameter takes.
	 * @note Deduced from the C++ type: T = Required (unless the Parameter declares a default),
	 * std::optional< T > = Optional, a trailing std::vector< T > = Variadic (zero or more).
	 */
	enum class EMEN_API ParameterArity : uint8_t
	{
		Required,
		Optional,
		Variadic
	};

	/**
	 * @brief Behaviour hints of a typed command, combinable with operator|.
	 * @note They describe the command to a machine client (the MCP tool annotations
	 * readOnlyHint / destructiveHint / idempotentHint). They are hints, never enforced.
	 */
	enum class EMEN_API CommandHint : uint8_t
	{
		None = 0,
		/** @brief The command only reads state. */
		ReadOnly = 1 << 0,
		/** @brief The command may destroy state (delete a scene, quit, overwrite a file). */
		Destructive = 1 << 1,
		/** @brief Calling the command twice with the same arguments has no further effect. */
		Idempotent = 1 << 2
	};

	/**
	 * @brief Combines two command hints.
	 * @param left The first hint set.
	 * @param right The second hint set.
	 * @return CommandHint
	 */
	[[nodiscard]]
	constexpr
	CommandHint
	operator| (CommandHint left, CommandHint right) noexcept
	{
		return static_cast< CommandHint >(static_cast< uint8_t >(left) | static_cast< uint8_t >(right));
	}

	/**
	 * @brief Returns whether a hint set contains a hint.
	 * @param hints The hint set.
	 * @param hint The hint to look for.
	 * @return bool
	 */
	[[nodiscard]]
	constexpr
	bool
	hasHint (CommandHint hints, CommandHint hint) noexcept
	{
		return ( static_cast< uint8_t >(hints) & static_cast< uint8_t >(hint) ) != 0;
	}
}
