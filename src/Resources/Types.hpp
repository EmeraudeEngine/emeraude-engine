/*
 * src/Resources/Types.hpp
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

namespace EmEn::Resources
{
	/** @brief Name of a default resource. */
	constexpr auto Default{"Default"};

	/** @brief Name of the data store base directory. */
	static constexpr auto DataStores{"data-stores"};

	/**
	 * @brief The resource source type.
	 */
	enum class SourceType
	{
		Undefined,
		/* Data key will hold the path to a local file. */
		LocalData,
		/* Data key will hold the URL to an external file. */
		ExternalData,
		/* Data key will hold the JSON definition of the resource. */
		DirectData
	};

	static constexpr auto UndefinedString{"Undefined"};
	static constexpr auto LocalDataString{"LocalData"};
	static constexpr auto ExternalDataString{"ExternalData"};
	static constexpr auto DirectDataString{"DirectData"};

	/**
	 * @brief Converts a source type enumeration value to the corresponding string.
	 * @param value The enumeration value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (SourceType value) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (SourceType value) noexcept
	{
		return {to_cstring(value)};
	}

	/**
	 * @brief Converts a string to a source type enumeration value.
	 * @param value A reference to a string.
	 * @return ComponentType
	 */
	[[nodiscard]]
	SourceType to_SourceType (const std::string & value) noexcept;

	/** @brief This enum defines every stage of resource loading. */
	enum class Status : uint8_t
	{
		/* This is the status of a new resource instantiation. */
		Unloaded = 0,
		/* Define a resource being attached with dependencies. */
		Enqueuing = 1,
		/* Define a resource being manually attached with dependencies. */
		ManualEnqueuing = 2,
		/* Define a resource being loaded.
		 * NOTE: In this stage, this is no more possible to add new dependency. */
		Loading = 3,
		/* Define a resource fully loaded with all dependencies. */
		Loaded = 4,
		/* Define a resource impossible to load. */
		Failed = 5
	};

	static constexpr auto UnloadedString{"Unloaded"};
	static constexpr auto EnqueuingString{"Enqueuing"};
	static constexpr auto ManualEnqueuingString{"ManualEnqueuing"};
	static constexpr auto LoadingString{"Loading"};
	static constexpr auto LoadedString{"Loaded"};
	static constexpr auto FailedString{"Failed"};

	/**
	 * @brief Converts a light pass type enumeration value to the corresponding string.
	 * @param value The enumeration value.
	 * @return const char *
	 */
	[[nodiscard]]
	const char * to_cstring (Status value) noexcept;

	/**
	 * @brief Returns a string version of the enum value.
	 * @param value The enum value.
	 * @return std::string
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (Status value) noexcept
	{
		return {to_cstring(value)};
	}

	/** @brief Describes the depth of dependencies requested for a resource. */
	enum class DepComplexity: uint8_t
	{
		None = 0,
		One = 1,
		Few = 2,
		Complex = 3
	};
}
