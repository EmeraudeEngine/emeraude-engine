/*
 * src/Console/TypedBinding.hpp
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
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/* Local inclusions for usages. */
#include "Argument.hpp"
#include "Command.hpp"
#include "CommandResult.hpp"
#include "CommandSignature.hpp"
#include "Parameter.hpp"
#include "Types.hpp"

/*
 * The machinery behind a typed ControllableTrait::bindCommand(): the parameter types are DEDUCED from
 * the callable's C++ signature, only the names, descriptions and defaults are written by hand, and the
 * compiler refuses a Parameter list whose length differs from the callable's arity. Same idea as
 * pybind11's `.def("f", &f, py::arg("x"), py::arg("y") = 1.0F)` (BSD-3, https://github.com/pybind/pybind11)
 * and Godot's `ClassDB::bind_method(D_METHOD("f", "x", "y"), &f, DEFVAL(1.0))`
 * (MIT, https://github.com/godotengine/godot) — the design only, no code is taken from either.
 */
namespace EmEn::Console::TypedBinding
{
	/** @brief The scalar C++ types a command parameter may have. */
	template< typename Type >
	concept ScalarParameter =
		std::same_as< Type, bool > ||
		std::same_as< Type, int32_t > ||
		std::same_as< Type, float > ||
		std::same_as< Type, std::string > ||
		std::same_as< Type, Argument >;

	/**
	 * @brief Returns the parameter type of a scalar C++ type.
	 * @tparam Type A scalar parameter type.
	 * @return ParameterType
	 */
	template< ScalarParameter Type >
	[[nodiscard]]
	consteval
	ParameterType
	scalarType () noexcept
	{
		if constexpr ( std::same_as< Type, bool > )
		{
			return ParameterType::Boolean;
		}
		else if constexpr ( std::same_as< Type, int32_t > )
		{
			return ParameterType::Integer;
		}
		else if constexpr ( std::same_as< Type, float > )
		{
			return ParameterType::Float;
		}
		else if constexpr ( std::same_as< Type, std::string > )
		{
			return ParameterType::String;
		}
		else
		{
			return ParameterType::Any;
		}
	}

	/** @brief Dependent false, for a static_assert in an unsupported specialization. */
	template< typename >
	inline constexpr bool AlwaysFalse = false;

	/**
	 * @brief How a C++ argument type becomes a command parameter. Unsupported types fail here.
	 * @tparam Type The decayed type of the callable's argument.
	 */
	template< typename Type >
	struct ParameterTraits
	{
		static_assert(AlwaysFalse< Type >, "A typed console command parameter must be bool, int32_t, float, std::string or Console::Argument, a std::optional of one of them, or (last parameter only) a std::vector of one of them.");
	};

	/** @brief A plain scalar: required, unless its Parameter declares a default value. */
	template< ScalarParameter Type >
	struct ParameterTraits< Type >
	{
		static constexpr auto Kind = scalarType< Type >();
		static constexpr auto Arity = ParameterArity::Required;

		[[nodiscard]]
		static
		bool
		extract (const Arguments & arguments, size_t index, const Parameter & parameter, Type & value, std::string & error) noexcept
		{
			const auto * argument = selectArgument(arguments, index, parameter);

			if ( argument == nullptr )
			{
				error = missingArgumentError(parameter);

				return false;
			}

			if ( !convertArgument(*argument, value) )
			{
				error = conversionError(parameter, *argument);

				return false;
			}

			return true;
		}
	};

	/** @brief std::optional< scalar >: may be omitted, the callable then receives std::nullopt. */
	template< ScalarParameter Type >
	struct ParameterTraits< std::optional< Type > >
	{
		static constexpr auto Kind = scalarType< Type >();
		static constexpr auto Arity = ParameterArity::Optional;

		[[nodiscard]]
		static
		bool
		extract (const Arguments & arguments, size_t index, const Parameter & parameter, std::optional< Type > & value, std::string & error) noexcept
		{
			if ( index >= arguments.size() )
			{
				value.reset();

				return true;
			}

			Type converted{};

			if ( !convertArgument(arguments[index], converted) )
			{
				error = conversionError(parameter, arguments[index]);

				return false;
			}

			value = std::move(converted);

			return true;
		}
	};

	/** @brief std::vector< scalar >, last parameter only: every remaining argument, possibly none. */
	template< ScalarParameter Type >
	struct ParameterTraits< std::vector< Type > >
	{
		static constexpr auto Kind = scalarType< Type >();
		static constexpr auto Arity = ParameterArity::Variadic;

		[[nodiscard]]
		static
		bool
		extract (const Arguments & arguments, size_t index, const Parameter & parameter, std::vector< Type > & values, std::string & error) noexcept
		{
			values.clear();

			for ( auto argumentIndex = index; argumentIndex < arguments.size(); ++argumentIndex )
			{
				Type converted{};

				if ( !convertArgument(arguments[argumentIndex], converted) )
				{
					error = conversionError(parameter, arguments[argumentIndex]);

					return false;
				}

				values.emplace_back(std::move(converted));
			}

			return true;
		}
	};

	/**
	 * @brief The signature of a callable's call operator (lambdas, mutable lambdas, functors).
	 * @tparam Signature The type of `&Callable::operator()`.
	 */
	template< typename Signature >
	struct CallOperatorTraits;

	template< typename Class, typename Result, typename... Args >
	struct CallOperatorTraits< Result (Class::*) (Args...) >
	{
		using ResultType = Result;
		using ArgumentTypes = std::tuple< std::remove_cvref_t< Args >... >;
		static constexpr size_t Arity = sizeof...(Args);
	};

	template< typename Class, typename Result, typename... Args >
	struct CallOperatorTraits< Result (Class::*) (Args...) const > : CallOperatorTraits< Result (Class::*) (Args...) >
	{

	};

	template< typename Class, typename Result, typename... Args >
	struct CallOperatorTraits< Result (Class::*) (Args...) noexcept > : CallOperatorTraits< Result (Class::*) (Args...) >
	{

	};

	template< typename Class, typename Result, typename... Args >
	struct CallOperatorTraits< Result (Class::*) (Args...) const noexcept > : CallOperatorTraits< Result (Class::*) (Args...) >
	{

	};

	/** @brief The traits of a callable object (a lambda, not a generic one). */
	template< typename Callable >
	using CallableTraits = CallOperatorTraits< decltype(&std::remove_cvref_t< Callable >::operator()) >;

	/** @brief A callable a typed bindCommand() accepts: a non-generic lambda returning CommandResult. */
	template< typename Callable >
	concept TypedCommandCallable =
		requires { &std::remove_cvref_t< Callable >::operator(); } &&
		std::same_as< typename CallableTraits< Callable >::ResultType, CommandResult >;

	/**
	 * @brief Returns whether a variadic parameter, if any, is the last one.
	 * @tparam ArgumentTypes The tuple of the callable's decayed argument types.
	 * @return bool
	 */
	template< typename ArgumentTypes, size_t... Index >
	[[nodiscard]]
	consteval
	bool
	variadicIsLast (std::index_sequence< Index... > /*indices*/) noexcept
	{
		constexpr size_t Count = sizeof...(Index);

		return ( ( ParameterTraits< std::tuple_element_t< Index, ArgumentTypes > >::Arity != ParameterArity::Variadic || Index + 1 == Count ) && ... );
	}

	/**
	 * @brief Completes the author's Parameter list with the types and arities deduced from the callable.
	 * @tparam ArgumentTypes The tuple of the callable's decayed argument types.
	 * @param parameters The author's declarations, in call order.
	 * @return std::vector< Parameter >
	 */
	template< typename ArgumentTypes, size_t... Index >
	[[nodiscard]]
	std::vector< Parameter >
	resolveParameters (const Parameter * parameters, std::index_sequence< Index... > /*indices*/) noexcept
	{
		return {
			parameters[Index].resolved(
				ParameterTraits< std::tuple_element_t< Index, ArgumentTypes > >::Kind,
				ParameterTraits< std::tuple_element_t< Index, ArgumentTypes > >::Arity
			)...
		};
	}

	/**
	 * @brief Converts the arguments, then calls the callable with them.
	 * @tparam Callable The author's callable.
	 * @tparam ArgumentTypes The tuple of the callable's decayed argument types.
	 * @param callable The author's callable.
	 * @param signature The resolved signature.
	 * @param arguments The supplied arguments.
	 * @param outputs The console outputs.
	 * @return bool Whether the command succeeded.
	 */
	template< typename Callable, typename ArgumentTypes, size_t... Index >
	[[nodiscard]]
	bool
	invoke (Callable & callable, const CommandSignature & signature, const Arguments & arguments, Outputs & outputs, std::index_sequence< Index... > /*indices*/) noexcept
	{
		std::string error;

		if ( !signature.checkArgumentCount(arguments, error) )
		{
			outputs.emplace_back(Severity::Error, std::move(error));

			return false;
		}

		ArgumentTypes values{};

		/* NOTE: the fold stops at the first argument that fails, so the error names that one. */
		const bool converted = ( ParameterTraits< std::tuple_element_t< Index, ArgumentTypes > >::extract(arguments, Index, signature.parameters()[Index], std::get< Index >(values), error) && ... );

		if ( !converted )
		{
			outputs.emplace_back(Severity::Error, std::move(error));

			return false;
		}

		auto result = std::apply(callable, std::move(values));

		return result.moveTo(outputs);
	}

	/**
	 * @brief Builds the validating Binding of a typed command.
	 * @tparam Callable The author's callable.
	 * @param callable The author's callable [std::move].
	 * @param signature The resolved signature, copied into the binding.
	 * @return Binding
	 */
	template< typename Callable >
	[[nodiscard]]
	Binding
	makeBinding (Callable callable, const CommandSignature & signature) noexcept
	{
		using Traits = CallableTraits< Callable >;

		return [callable = std::move(callable), signature] (const Arguments & arguments, Outputs & outputs) mutable {
			return invoke< Callable, typename Traits::ArgumentTypes >(callable, signature, arguments, outputs, std::make_index_sequence< Traits::Arity >{});
		};
	}
}
