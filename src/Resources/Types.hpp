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

/**
 * @file Types.hpp
 * @brief Core type definitions and enumerations for the Emeraude Engine resource management system.
 *
 * This file defines fundamental types, enumerations, and utility functions used throughout
 * the resource management subsystem. It includes:
 * - Resource source type identification (local, external, direct data)
 * - Resource loading status tracking
 * - Dependency complexity indicators
 * - String conversion utilities for all enumerations
 *
 * All functions in this file are header-only inline implementations for optimal performance.
 *
 * @see EmEn::Resources::Manager
 * @see EmEn::Resources::ResourceTrait
 * @version 0.8.35
 */

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <string>
#include <string_view>

/**
 * @namespace EmEn::Resources
 * @brief Resource management subsystem for the Emeraude Engine.
 *
 * This namespace contains all classes, types, and utilities related to resource
 * management including loading, caching, dependency resolution, and lifecycle management.
 *
 * @version 0.8.35
 */
namespace EmEn::Resources
{
	/**
	 * @brief Name of a default resource.
	 *
	 * This constant is used to identify the default resource instance when no specific
	 * resource name is provided. Default resources are typically used as fallbacks or
	 * placeholder values during initialization.
	 *
	 * @version 0.8.35
	 */
	constexpr auto Default{"Default"};

	/**
	 * @brief Name of the data store base directory.
	 *
	 * This constant defines the base directory name where resource data stores are located.
	 * Data stores organize resources by type and provide hierarchical storage for game assets.
	 *
	 * @version 0.8.35
	 */
	constexpr auto DataStores{"data-stores"};

	/**
	 * @enum SourceType
	 * @brief Defines the origin and storage method of resource data.
	 *
	 * This enumeration specifies where and how resource data is stored and accessed.
	 * It is used by the resource management system to determine the appropriate loading
	 * strategy and data access pattern for each resource.
	 *
	 * @see to_cstring(SourceType)
	 * @see to_string(SourceType)
	 * @see to_SourceType(std::string_view)
	 * @version 0.8.35
	 */
	enum class SourceType
	{
		Undefined,     ///< Uninitialized or unknown source type. Default state for new resources.
		LocalData,     ///< Data key holds the path to a local file on the filesystem.
		ExternalData,  ///< Data key holds the URL to an external file (network resource).
		DirectData     ///< Data key holds the JSON definition of the resource inline.
	};

	/**
	 * @brief String representation for SourceType::Undefined.
	 * @version 0.8.35
	 */
	constexpr auto UndefinedString{"Undefined"};

	/**
	 * @brief String representation for SourceType::LocalData.
	 * @version 0.8.35
	 */
	constexpr auto LocalDataString{"LocalData"};

	/**
	 * @brief String representation for SourceType::ExternalData.
	 * @version 0.8.35
	 */
	constexpr auto ExternalDataString{"ExternalData"};

	/**
	 * @brief String representation for SourceType::DirectData.
	 * @version 0.8.35
	 */
	constexpr auto DirectDataString{"DirectData"};

	/**
	 * @brief Converts a SourceType enumeration value to its C-string representation.
	 *
	 * This function provides a compile-time constant string representation of the
	 * SourceType enumeration. It is guaranteed not to throw exceptions and always
	 * returns a valid string pointer.
	 *
	 * @param value The SourceType enumeration value to convert.
	 * @return A pointer to a null-terminated string containing the enumeration name.
	 *         Returns "Undefined" for any invalid or unrecognized value.
	 *
	 * @note This function is noexcept and always returns a valid pointer.
	 * @see to_string(SourceType)
	 * @see to_SourceType(std::string_view)
	 * @version 0.8.35
	 */
	[[nodiscard]]
	inline
	const char *
	to_cstring (SourceType value) noexcept
	{
		switch ( value )
		{
			case SourceType::Undefined :
				return UndefinedString;

			case SourceType::LocalData :
				return LocalDataString;

			case SourceType::ExternalData :
				return ExternalDataString;

			case SourceType::DirectData :
				return DirectDataString;
		}

		return UndefinedString;
	}

	/**
	 * @brief Converts a SourceType enumeration value to a std::string.
	 *
	 * This function creates a std::string object from the SourceType enumeration
	 * by delegating to to_cstring(). Use this when you need a std::string object
	 * rather than a C-string pointer.
	 *
	 * @param value The SourceType enumeration value to convert.
	 * @return A std::string containing the enumeration name.
	 *
	 * @see to_cstring(SourceType)
	 * @see to_SourceType(std::string_view)
	 * @version 0.8.35
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (SourceType value)
	{
		return {to_cstring(value)};
	}

	/**
	 * @brief Converts a string to a SourceType enumeration value.
	 *
	 * This function parses a string representation and returns the corresponding
	 * SourceType enumeration value. It performs exact string matching against known
	 * SourceType names.
	 *
	 * @param value A string_view containing the SourceType name to convert.
	 *              Valid values are: "LocalData", "ExternalData", "DirectData".
	 * @return The corresponding SourceType enumeration value.
	 *         Returns SourceType::Undefined if the input string does not match any known value.
	 *
	 * @note This function is case-sensitive and requires exact matches.
	 * @note This function is noexcept and will never throw exceptions.
	 * @see to_cstring(SourceType)
	 * @see to_string(SourceType)
	 * @version 0.8.35
	 */
	[[nodiscard]]
	inline
	SourceType
	to_SourceType (std::string_view value) noexcept
	{
		if ( value == LocalDataString )
		{
			return SourceType::LocalData;
		}

		if ( value == ExternalDataString )
		{
			return SourceType::ExternalData;
		}

		if ( value == DirectDataString )
		{
			return SourceType::DirectData;
		}

		return SourceType::Undefined;
	}

	/**
	 * @enum Status
	 * @brief Defines every stage of the resource loading lifecycle.
	 *
	 * This enumeration tracks the current state of a resource through its loading process,
	 * from initial instantiation through dependency resolution, loading, and final completion
	 * or failure. The status transitions are generally sequential, moving from Unloaded toward
	 * either Loaded or Failed states.
	 *
	 * The loading pipeline follows this typical flow:
	 * Unloaded -> Enqueuing/ManualEnqueuing -> Loading -> Loaded/Failed
	 *
	 * @note Once a resource reaches the Loading state, no additional dependencies can be added.
	 * @see to_cstring(Status)
	 * @see to_string(Status)
	 * @version 0.8.35
	 */
	enum class Status : uint8_t
	{
		Unloaded = 0,        ///< Initial status of a new resource instantiation. Resource has not been queued for loading.
		Enqueuing = 1,       ///< Resource is being attached with dependencies automatically by the system.
		ManualEnqueuing = 2, ///< Resource is being manually attached with dependencies by user code.
		Loading = 3,         ///< Resource is actively being loaded. No new dependencies can be added at this stage.
		Loaded = 4,          ///< Resource has been fully loaded along with all its dependencies.
		Failed = 5           ///< Resource loading has failed and cannot be loaded.
	};

	/**
	 * @brief String representation for Status::Unloaded.
	 * @version 0.8.35
	 */
	constexpr auto UnloadedString{"Unloaded"};

	/**
	 * @brief String representation for Status::Enqueuing.
	 * @version 0.8.35
	 */
	constexpr auto EnqueuingString{"Enqueuing"};

	/**
	 * @brief String representation for Status::ManualEnqueuing.
	 * @version 0.8.35
	 */
	constexpr auto ManualEnqueuingString{"ManualEnqueuing"};

	/**
	 * @brief String representation for Status::Loading.
	 * @version 0.8.35
	 */
	constexpr auto LoadingString{"Loading"};

	/**
	 * @brief String representation for Status::Loaded.
	 * @version 0.8.35
	 */
	constexpr auto LoadedString{"Loaded"};

	/**
	 * @brief String representation for Status::Failed.
	 * @version 0.8.35
	 */
	constexpr auto FailedString{"Failed"};

	/**
	 * @brief Converts a Status enumeration value to its C-string representation.
	 *
	 * This function provides a compile-time constant string representation of the
	 * Status enumeration. It is guaranteed not to throw exceptions and always
	 * returns a valid string pointer.
	 *
	 * @param value The Status enumeration value to convert.
	 * @return A pointer to a null-terminated string containing the status name.
	 *         Returns "Unloaded" for any invalid or unrecognized value.
	 *
	 * @note This function is noexcept and always returns a valid pointer.
	 * @see to_string(Status)
	 * @version 0.8.35
	 */
	[[nodiscard]]
	inline
	const char *
	to_cstring (Status value) noexcept
	{
		switch ( value )
		{
			case Status::Unloaded :
				return UnloadedString;

			case Status::Enqueuing :
				return EnqueuingString;

			case Status::ManualEnqueuing :
				return ManualEnqueuingString;

			case Status::Loading :
				return LoadingString;

			case Status::Loaded :
				return LoadedString;

			case Status::Failed :
				return FailedString;
		}

		return UnloadedString;
	}

	/**
	 * @brief Converts a Status enumeration value to a std::string.
	 *
	 * This function creates a std::string object from the Status enumeration
	 * by delegating to to_cstring(). Use this when you need a std::string object
	 * rather than a C-string pointer.
	 *
	 * @param value The Status enumeration value to convert.
	 * @return A std::string containing the status name.
	 *
	 * @see to_cstring(Status)
	 * @version 0.8.35
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (Status value)
	{
		return {to_cstring(value)};
	}

	/**
	 * @enum DepComplexity
	 * @brief Describes the depth and complexity of dependencies for a resource.
	 *
	 * This enumeration categorizes resources based on how many dependencies they have
	 * and how complex their dependency tree is. This information can be used by the
	 * resource management system to optimize loading strategies and prioritize resources.
	 *
	 * The complexity levels provide hints about resource loading time and memory requirements:
	 * - None: Instant loading, minimal memory
	 * - One: Quick loading, small memory footprint
	 * - Few: Moderate loading time, reasonable memory usage
	 * - Complex: Extended loading time, significant memory requirements
	 *
	 * @see to_cstring(DepComplexity)
	 * @see to_string(DepComplexity)
	 * @version 0.8.35
	 */
	enum class DepComplexity : uint8_t
	{
		None = 0,    ///< No dependencies. Resource is self-contained and can be loaded independently.
		One = 1,     ///< Single dependency. Resource depends on exactly one other resource.
		Few = 2,     ///< Few dependencies (2-5 typically). Resource has a small, manageable dependency tree.
		Complex = 3  ///< Complex dependency tree (6+ typically). Resource has many dependencies or nested dependencies.
	};

	/**
	 * @brief String representation for DepComplexity::None.
	 * @version 0.8.35
	 */
	constexpr auto NoneString{"None"};

	/**
	 * @brief String representation for DepComplexity::One.
	 * @version 0.8.35
	 */
	constexpr auto OneString{"One"};

	/**
	 * @brief String representation for DepComplexity::Few.
	 * @version 0.8.35
	 */
	constexpr auto FewString{"Few"};

	/**
	 * @brief String representation for DepComplexity::Complex.
	 * @version 0.8.35
	 */
	constexpr auto ComplexString{"Complex"};

	/**
	 * @brief Converts a DepComplexity enumeration value to its C-string representation.
	 *
	 * This function provides a compile-time constant string representation of the
	 * DepComplexity enumeration. It is guaranteed not to throw exceptions and always
	 * returns a valid string pointer.
	 *
	 * @param value The DepComplexity enumeration value to convert.
	 * @return A pointer to a null-terminated string containing the complexity level name.
	 *         Returns "None" for any invalid or unrecognized value.
	 *
	 * @note This function is noexcept and always returns a valid pointer.
	 * @see to_string(DepComplexity)
	 * @version 0.8.35
	 */
	[[nodiscard]]
	inline
	const char *
	to_cstring (DepComplexity value) noexcept
	{
		switch ( value )
		{
			case DepComplexity::None :
				return NoneString;

			case DepComplexity::One :
				return OneString;

			case DepComplexity::Few :
				return FewString;

			case DepComplexity::Complex :
				return ComplexString;
		}

		return NoneString;
	}

	/**
	 * @brief Converts a DepComplexity enumeration value to a std::string.
	 *
	 * This function creates a std::string object from the DepComplexity enumeration
	 * by delegating to to_cstring(). Use this when you need a std::string object
	 * rather than a C-string pointer.
	 *
	 * @param value The DepComplexity enumeration value to convert.
	 * @return A std::string containing the complexity level name.
	 *
	 * @see to_cstring(DepComplexity)
	 * @version 0.8.35
	 */
	[[nodiscard]]
	inline
	std::string
	to_string (DepComplexity value)
	{
		return {to_cstring(value)};
	}
}
