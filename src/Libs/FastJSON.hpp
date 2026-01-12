/*
 * src/Libs/FastJSON.hpp
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

/* Application configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <filesystem>
#include <optional>
#include <utility>

/* Third-party inclusions. */
#ifndef JSON_USE_EXCEPTION
#define JSON_USE_EXCEPTION 0
#endif
#include "json/json.h"

/* Local inclusions. */
#include "Libs/Math/Matrix.hpp"
#include "Libs/PixelFactory/Color.hpp"

namespace EmEn::Libs::FastJSON
{
	/* NOTE: Common JSON key. */
	constexpr auto TypeKey{"Type"};
	constexpr auto NameKey{"Name"};
	constexpr auto PositionKey{"Position"};
	constexpr auto OrientationKey{"Orientation"};
	constexpr auto ColorKey{"Color"};
	constexpr auto IntensityKey{"Intensity"};
	constexpr auto DataKey{"Data"};
	constexpr auto PropertiesKey{"Properties"};
	constexpr auto ScaleKey{"Scale"};
	constexpr auto SizeKey{"Size"};
	constexpr auto DivisionKey{"Division"};
	constexpr auto UVMultiplierKey{"UVMultiplier"};
	constexpr auto ModeKey{"Mode"};
	
	/**
	 * @brief Gets the root JSON node from a filepath.
	 * @param filepath A reference to a filesystem path.
	 * @param stackLimit The depth of JSON parsing. Default 16.
	 * @param quiet Do not print console message. Default false.
	 * @return std::optional< Json::Value >
	 */
	[[nodiscard]]
	std::optional< Json::Value > getRootFromFile (const std::filesystem::path & filepath, int stackLimit = 16, bool quiet = false);

	/**
	 * @brief Gets the root JSON node from a string.
	 * @param json A reference to a string.
	 * @param stackLimit The depth of JSON parsing. Default 16.
	 * @param quiet Do not print console message. Default false.
	 * @return std::optional< Json::Value >
	 */
	[[nodiscard]]
	std::optional< Json::Value > getRootFromString (const std::string & json, int stackLimit = 16, bool quiet = false);

	/**
	 * @brief Creates a compact standard string from a JSON node.
	 * @param root A reference to a JSON value.
	 * @return std::string
	 */
	[[nodiscard]]
	std::string stringify (const Json::Value & root);

	/**
	 * @brief Gets a JSON array from a JSON node.
	 * @param parentNode A reference to a JSON node.
	 * @param key The JSON key name.
	 * @return std::optional< Json::Value >
	 */
	[[nodiscard]]
	inline
	std::optional< Json::Value >
	getArray (const Json::Value & parentNode, const char * key) noexcept
	{
		if ( !parentNode.isMember(key) )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is missing !" << std::endl;
			}

			return std::nullopt;
		}

		const auto & node = parentNode[key];

		if ( !node.isArray() )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is not an array !" << std::endl;
			}

			return std::nullopt;
		}

		return node;
	}

	/**
	 * @brief Gets a JSON object from a JSON node.
	 * @param parentNode A reference to a JSON node.
	 * @param key The JSON key name.
	 * @return std::optional< Json::Value >
	 */
	[[nodiscard]]
	inline
	std::optional< Json::Value >
	getObject (const Json::Value & parentNode, const char * key) noexcept
	{
		if ( !parentNode.isMember(key) )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is missing !" << std::endl;
			}

			return std::nullopt;
		}

		const auto & node = parentNode[key];

		if ( !node.isObject() )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is not an object !" << std::endl;
			}

			return std::nullopt;
		}

		return node;
	}

	/**
	 * @brief Returns a number from a JSON node.
	 * @tparam value_t The type of the number.
	 * @param parentNode A reference to a JSON node.
	 * @param key The JSON key name to look for.
	 * @return std::optional< value_t >
	 */
	template< typename value_t >
	[[nodiscard]]
	std::optional< value_t >
	getValue (const Json::Value & parentNode, const char * key) noexcept requires (std::is_arithmetic_v< value_t >)
	{
		if ( !parentNode.isMember(key) )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is missing !" << std::endl;
			}

			return std::nullopt;
		}

		const auto & node = parentNode[key];

		if constexpr ( std::is_same_v< value_t, bool > )
		{
			if ( node.isBool() )
			{
				return parentNode[key].asBool();
			}
		}
		else
		{
			if ( node.isNumeric() )
			{
				if constexpr ( std::is_same_v< value_t, int8_t > || std::is_same_v< value_t, int16_t> || std::is_same_v< value_t, int32_t > )
				{
					return static_cast< value_t >(node.asInt());
				}

				if constexpr ( std::is_same_v< value_t, int64_t > )
				{
					return node.asInt64();
				}

				if constexpr ( std::is_same_v< value_t, uint8_t > || std::is_same_v< value_t, uint16_t> || std::is_same_v< value_t, uint32_t > )
				{
					return static_cast< value_t >(node.asUInt());
				}

				if constexpr ( std::is_same_v< value_t, uint64_t > )
				{
					return node.asUInt64();
				}

				if constexpr ( std::is_same_v< value_t, float_t > )
				{
					return node.asFloat();
				}

				if constexpr ( std::is_same_v< value_t, double_t > )
				{
					return node.asDouble();
				}
			}
		}

		if constexpr ( IsDebug )
		{
			std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is not convertible to a numeric value !" << std::endl;
		}

		return std::nullopt;
	}

	/**
	 * @brief Returns a string from a JSON node.
	 * @tparam value_t The type of the variable. For overloading, this must be 'std::string'.
	 * @param parentNode A reference to a JSON node.
	 * @param key The JSON key name to look for.
	 * @return std::optional< value_t >
	 */
	template< typename value_t >
	[[nodiscard]]
	std::optional< value_t >
	getValue (const Json::Value & parentNode, const char * key) requires (std::is_same_v< value_t, std::string >)
	{
		if ( !parentNode.isMember(key) )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is missing !" << std::endl;
			}

			return std::nullopt;
		}

		const auto & node = parentNode[key];

		if ( !node.isString() )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is not a string !" << std::endl;
			}

			return std::nullopt;
		}

		return node.asString();
	}

	/**
	 * @brief Helper to choose the correct function to cast a number.
	 * @tparam precision_t The type of precision to cast
	 */
	template< typename precision_t >
	struct JsonValueCaster;

	/**
	 * @brief Helper specialization for float casting.
	 */
	template<>
	struct JsonValueCaster< float >
	{
		static
		float
		cast (const Json::Value & node) noexcept
		{
			return node.asFloat();
		}
	};

	/**
	 * @brief Helper specialization for double casting.
	 */
	template<>
	struct JsonValueCaster< double >
	{
		static
		double
		cast (const Json::Value & node) noexcept
		{
			return node.asDouble();
		}
	};

	/**
	 * @brief Helpers to construct multiple same parameters objects like vectors or matrices.
	 * @tparam object_t The type of object to instantiate.
	 * @tparam parameter_count The number of parameters.
	 * @tparam Is
	 * @param node A reference to a JSON node.
	 * @return object_t
	 */
	template< typename object_t, typename parameter_count, std::size_t... Is >
	object_t
	createFromJsonImpl (const Json::Value & node, std::index_sequence< Is... >)
	{
		return object_t{ JsonValueCaster< parameter_count >::cast(node[static_cast< Json::Value::ArrayIndex >(Is)])... };
	}

	/**
	 * @brief Returns a vector from a JSON node.
	 * @tparam value_t The type of vector.
	 * @param parentNode A reference to a JSON node.
	 * @param key The JSON key name to look for.
	 * @return std::optional< value_t >
	 */
	template< Math::VectorConcept value_t >
	[[nodiscard]]
	std::optional< value_t >
	getValue (const Json::Value & parentNode, const char * key) noexcept
	{
		using Traits = Math::VectorTraits< value_t >;

		if ( !parentNode.isMember(key) )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is missing !" << std::endl;
			}

			return std::nullopt;
		}

		const auto & node = parentNode[key];

		if ( !node.isArray() || node.size() < Traits::dim )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is not an array of " << Traits::dim << " items !" << std::endl;
			}

			return std::nullopt;
		}

		return createFromJsonImpl< value_t, typename Traits::precision >(node, std::make_index_sequence< Traits::dim >());
	}

	/**
	 * @brief Returns a matrix from a JSON node.
	 * @tparam value_t The type of vector.
	 * @param parentNode A reference to a JSON node.
	 * @param key The JSON key name to look for.
	 * @return std::optional< value_t >
	 */
	template< Math::MatrixConcept value_t >
	[[nodiscard]]
	std::optional< value_t >
	getValue (const Json::Value & parentNode, const char * key) noexcept
	{
		using Traits = Math::MatrixTraits< value_t >;

		constexpr size_t element_count = Traits::dim * Traits::dim;

		if ( !parentNode.isMember(key) )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is missing !" << std::endl;
			}

			return std::nullopt;
		}

		const auto & node = parentNode[key];

		if ( !node.isArray() || node.size() < element_count )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is not an array of " << element_count << " items !" << std::endl;
			}

			return std::nullopt;
		}

		return createFromJsonImpl< value_t, typename Traits::precision >(node, std::make_index_sequence<element_count>());
	}

	/**
	 * @brief Returns a color from a JSON node.
	 * @tparam value_t The type of vector.
	 * @param parentNode A reference to a JSON node.
	 * @param key The JSON key name to look for.
	 * @return std::optional< value_t >
	 */
	template< PixelFactory::ColorConcept value_t >
	[[nodiscard]]
	std::optional< value_t >
	getValue (const Json::Value & parentNode, const char * key) noexcept
	{
		using precision_t = typename PixelFactory::ColorTraits< value_t >::precision;

		if ( !parentNode.isMember(key) )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is missing !" << std::endl;
			}

			return std::nullopt;
		}

		const auto & node = parentNode[key];

		if ( !node.isArray() )
		{
			if constexpr ( IsDebug )
			{
				std::cerr << "[FastJSON-DEBUG] Key '" << key << "' is not an array !" << std::endl;
			}

			return std::nullopt;
		}

		switch ( node.size() )
		{
			case 3:
				return createFromJsonImpl< value_t, precision_t >(node, std::make_index_sequence< 3 >());

			case 4:
				return createFromJsonImpl< value_t, precision_t >(node, std::make_index_sequence< 4 >());

			default:
				if constexpr ( IsDebug )
				{
					std::cerr << "[FastJSON-DEBUG] Key '" << key << "' cannot be converted to a color !" << std::endl;
				}

				return std::nullopt;
		}
	}

	/**
	 * @brief Gets a string from a JSON node using a list of valid terms.
	 * @param data A reference to a v value.
	 * @param key The JSON key name.
	 * @param possibleValues An array of allowed string values.
	 * @return std::optional< std::string >, The string value if it's found and valid, otherwise std::nullopt.
	 */
	template< size_t dim_t >
	[[nodiscard]]
	std::optional< std::string >
	getValidatedStringValue (const Json::Value & data, std::string_view key, const std::array< std::string_view, dim_t > & possibleValues) requires (dim_t > 0)
	{
		if ( const std::string keyString{key}; data.isMember(keyString) )
		{
			if ( const auto & node = data[keyString]; node.isString() )
			{
				auto foundValue = node.asString();

				if ( std::ranges::any_of(possibleValues, [&] (std::string_view value) {return value == foundValue;}) )
				{
					return foundValue;
				}
			}
		}

		return std::nullopt;
	}
}
