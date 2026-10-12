/*
 * src/Console/Output.hpp
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

/* STL inclusions. */
#include <cmath>
#include <cstddef>
#include <ostream>

/* Local inclusions for usages. */
#include "Math/Vector.hpp"

namespace EmEn::Console
{
	/**
	 * @brief A float written as a JSON number for a hand-streamed console answer: 9 significant digits (the float round
	 * trip), and null when it is not finite (JSON has no NaN nor infinity: a "nan" made the whole answer invalid JSON,
	 * and the MCP server then dropped its structuredContent).
	 * @note The stream's precision is restored after the number, so the next values keep theirs.
	 */
	struct JSONNumber final
	{
		float value;

		friend
		std::ostream &
		operator<< (std::ostream & output, const JSONNumber & number) noexcept
		{
			if ( !std::isfinite(number.value) )
			{
				return output << "null";
			}

			const auto previousPrecision = output.precision(9);

			output << number.value;
			output.precision(previousPrecision);

			return output;
		}
	};

	/**
	 * @brief Wraps a float to stream it as a JSON number (see JSONNumber).
	 * @param value The value.
	 * @return JSONNumber
	 */
	[[nodiscard]]
	inline
	JSONNumber
	jsonNumber (float value) noexcept
	{
		return JSONNumber{value};
	}

	/**
	 * @brief Writes a 3D vector as a JSON array of JSON numbers (a non-finite component is null).
	 * @param output A reference to the stream.
	 * @param vector A reference to the vector.
	 */
	inline
	void
	writeJSONVector (std::ostream & output, const Base::Math::Vector< 3, float > & vector) noexcept
	{
		output << '[' << jsonNumber(vector[0]) << ',' << jsonNumber(vector[1]) << ',' << jsonNumber(vector[2]) << ']';
	}
}
