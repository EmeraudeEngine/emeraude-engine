/*
 * src/Libs/Variant.hpp
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
#include <variant>

/* Local inclusions for usage. */
#include "Libs/Math/Matrix.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/Math/CartesianFrame.hpp"

namespace EmEn::Libs
{
	/**
	 * @class Variant
	 * @brief Type-safe discriminated union capable of holding any one of 20 concrete engine types.
	 *
	 * Variant wraps a `std::variant` (the `Storage` type alias) that can hold exactly one value at
	 * a time from a fixed set of primitive and engine-math types, or nothing at all (the null state).
	 * The active type is tracked via the `Type` enum, whose integer values map 1:1 to the underlying
	 * `std::variant` index, so `type()` is a trivial index cast with no branching.
	 *
	 * Construction and mutation are performed through a pair of constrained templates — one constructor
	 * and one `set()` method — gated by the `IsValidType` concept. This replaces a previous design
	 * that required 20 individual overloads for each supported type. `std::monostate` is explicitly
	 * excluded from the constraint so callers cannot accidentally construct a null variant through the
	 * templated path; use the default constructor or `reset()` for that.
	 *
	 * All value-retrieval methods (`as*()`) operate with `std::get_if` rather than `std::get`,
	 * complying with the project-wide `-fno-exceptions` policy. On a type mismatch the method emits
	 * a diagnostic message to `std::cerr` and returns the zero-value for the requested type (0,
	 * 0.0, false, or a default-constructed object). Callers should always verify the active type
	 * with `type()` or `isNull()` before extracting a value.
	 *
	 * @note The class is marked `final`; it is not intended to serve as a base class.
	 * @note All constructors, assignments, and retrieval methods are `noexcept`, consistent with
	 *       the engine's exception-free compilation mode.
	 *
	 * @code
	 * // Storing and retrieving a value
	 * EmEn::Libs::Variant v{42.0f};
	 * if (v.type() == EmEn::Libs::Variant::Type::Float)
	 *     float x = v.asFloat();
	 *
	 * // Updating in-place
	 * v.set(Math::Vector3F{1.0f, 0.0f, 0.0f});
	 *
	 * // Resetting to null
	 * v.reset();
	 * assert(v.isNull());
	 * @endcode
	 *
	 * @see Variant::Type
	 * @see Variant::Storage
	 * @version 0.8.61
	 */
	class Variant final
	{
		public:

			static constexpr auto NullString{"Null"};                                     ///< String representation of Type::Null.
			static constexpr auto Integer8String{"Integer8"};                             ///< String representation of Type::Integer8.
			static constexpr auto UnsignedInteger8String{"UnsignedInteger8"};             ///< String representation of Type::UnsignedInteger8.
			static constexpr auto Integer16String{"Integer16"};                           ///< String representation of Type::Integer16.
			static constexpr auto UnsignedInteger16String{"UnsignedInteger16"};           ///< String representation of Type::UnsignedInteger16.
			static constexpr auto Integer32String{"Integer32"};                           ///< String representation of Type::Integer32.
			static constexpr auto UnsignedInteger32String{"UnsignedInteger32"};           ///< String representation of Type::UnsignedInteger32.
			static constexpr auto Integer64String{"Integer64"};                           ///< String representation of Type::Integer64.
			static constexpr auto UnsignedInteger64String{"UnsignedInteger64"};           ///< String representation of Type::UnsignedInteger64.
			static constexpr auto FloatString{"Float"};                                   ///< String representation of Type::Float.
			static constexpr auto DoubleString{"Double"};                                 ///< String representation of Type::Double.
			static constexpr auto LongDoubleString{"LongDouble"};                         ///< String representation of Type::LongDouble.
			static constexpr auto BooleanString{"Boolean"};                               ///< String representation of Type::Boolean.
			static constexpr auto Vector2FloatString{"Vector2Float"};                     ///< String representation of Type::Vector2Float.
			static constexpr auto Vector3FloatString{"Vector3Float"};                     ///< String representation of Type::Vector3Float.
			static constexpr auto Vector4FloatString{"Vector4Float"};                     ///< String representation of Type::Vector4Float.
			static constexpr auto Matrix2FloatString{"Matrix2Float"};                     ///< String representation of Type::Matrix2Float.
			static constexpr auto Matrix3FloatString{"Matrix3Float"};                     ///< String representation of Type::Matrix3Float.
			static constexpr auto Matrix4FloatString{"Matrix4Float"};                     ///< String representation of Type::Matrix4Float.
			static constexpr auto CartesianFrameString{"CartesianFrameFloat"};            ///< String representation of Type::CartesianFrameFloat.
			static constexpr auto ColorString{"Color"};                                   ///< String representation of Type::Color.

			/**
			 * @enum Type
			 * @brief Discriminator tag identifying which concrete type is currently held by a Variant.
			 *
			 * The integer value of each enumerator equals the index of the corresponding alternative
			 * inside the `Storage` std::variant, with `Null` mapping to `std::monostate` at index 0.
			 * This invariant is relied upon by `type()`, which casts `m_data.index()` directly.
			 *
			 * @version 0.8.61
			 */
			enum class Type
			{
				/* Null variable. */
				Null,              ///< No value is held; the variant is in its default empty state.
				/* Primitives variables. */
				Integer8,          ///< Holds a signed 8-bit integer (`int8_t`).
				UnsignedInteger8,  ///< Holds an unsigned 8-bit integer (`uint8_t`).
				Integer16,         ///< Holds a signed 16-bit integer (`int16_t`).
				UnsignedInteger16, ///< Holds an unsigned 16-bit integer (`uint16_t`).
				Integer32,         ///< Holds a signed 32-bit integer (`int32_t`).
				UnsignedInteger32, ///< Holds an unsigned 32-bit integer (`uint32_t`).
				Integer64,         ///< Holds a signed 64-bit integer (`int64_t`).
				UnsignedInteger64, ///< Holds an unsigned 64-bit integer (`uint64_t`).
				Float,             ///< Holds a single-precision floating-point value (`float`).
				Double,            ///< Holds a double-precision floating-point value (`double`).
				LongDouble,        ///< Holds an extended-precision floating-point value (`long double`).
				Boolean,           ///< Holds a boolean value (`bool`).
				/* Libs variables. */
				Vector2Float,        ///< Holds a 2-component single-precision vector (`Math::Vector2F`).
				Vector3Float,        ///< Holds a 3-component single-precision vector (`Math::Vector3F`).
				Vector4Float,        ///< Holds a 4-component single-precision vector (`Math::Vector4F`).
				Matrix2Float,        ///< Holds a 2x2 single-precision matrix (`Math::Matrix2F`).
				Matrix3Float,        ///< Holds a 3x3 single-precision matrix (`Math::Matrix3F`).
				Matrix4Float,        ///< Holds a 4x4 single-precision matrix (`Math::Matrix4F`).
				CartesianFrameFloat, ///< Holds a single-precision Cartesian frame (`Math::CartesianFrameF`).
				Color                ///< Holds a single-precision RGBA color (`PixelFactory::ColorF`).
			};

			/**
			 * @brief Internal storage type matching the Type enum order.
			 *
			 * The position of each alternative in this variant list corresponds exactly to the
			 * integer value of the matching `Type` enumerator. `std::monostate` at index 0
			 * represents the null (empty) state. This correspondence must be maintained if new
			 * types are ever added; both `Type` and `Storage` must be updated in lockstep.
			 */
			using Storage = std::variant<
				std::monostate,
				int8_t, uint8_t,
				int16_t, uint16_t,
				int32_t, uint32_t,
				int64_t, uint64_t,
				float, double, long double,
				bool,
				Math::Vector< 2, float >,
				Math::Vector< 3, float >,
				Math::Vector< 4, float >,
				Math::Matrix< 2, float >,
				Math::Matrix< 3, float >,
				Math::Matrix< 4, float >,
				Math::CartesianFrame< float >,
				PixelFactory::Color< float >
			>;

		private:

			/**
			 * @brief Trait that resolves to `std::true_type` if `T` is one of the alternatives
			 *        of a given `std::variant` specialization `V`, and `std::false_type` otherwise.
			 *
			 * The primary template handles the non-variant case. The partial specialization
			 * unpacks `std::variant<Ts...>` and checks membership via `std::disjunction`.
			 *
			 * @tparam T The type to test.
			 * @tparam V A `std::variant` specialisation to test membership against.
			 */
			template< typename T, typename V >
			struct IsStorageAlternative : std::false_type {};

			template< typename T, typename... Ts >
			struct IsStorageAlternative< T, std::variant< Ts... > > : std::disjunction< std::is_same< T, Ts >... > {};

			/**
			 * @brief Constraint variable template: `true` when `T` (after decay) is a valid
			 *        `Storage` alternative other than `std::monostate`.
			 *
			 * Used as a `requires` clause on the templated constructor and `set()` to prevent
			 * instantiation with unsupported types at compile time. `std::monostate` is explicitly
			 * excluded so callers must use `reset()` to reach the null state.
			 *
			 * @tparam T The candidate type to validate.
			 */
			template< typename T >
			static constexpr bool IsValidType = IsStorageAlternative< std::decay_t< T >, Storage >::value
											 && !std::is_same_v< std::decay_t< T >, std::monostate >;

		public:

			/**
			 * @brief Constructs a null variant.
			 *
			 * The internal storage is initialised to `std::monostate`. `isNull()` returns `true`
			 * and `type()` returns `Type::Null` after this constructor.
			 */
			Variant () noexcept = default;

			/**
			 * @brief Constructs a variant by copying another variant.
			 * @param other The source variant to copy.
			 */
			Variant (const Variant & other) noexcept = default;

			/**
			 * @brief Copies the state of another variant into this one.
			 * @param other The source variant to copy.
			 * @return A reference to this variant.
			 */
			Variant & operator= (const Variant & other) noexcept = default;

			/**
			 * @brief Constructs a variant by moving another variant.
			 * @param other The source variant to move from. Left in the null state after the move.
			 */
			Variant (Variant && other) noexcept = default;

			/**
			 * @brief Moves the state of another variant into this one.
			 * @param other The source variant to move from. Left in the null state after the move.
			 * @return A reference to this variant.
			 */
			Variant & operator= (Variant && other) noexcept = default;

			/**
			 * @brief Destructs the variant and releases the currently held value.
			 */
			~Variant () = default;

			/**
			 * @brief Constructs a variant holding the supplied value.
			 *
			 * The template parameter is constrained by `IsValidType` so the compiler rejects any
			 * type that is not a listed `Storage` alternative (and rejects `std::monostate`
			 * explicitly). The active type after construction matches the `Type` enumerator
			 * corresponding to `value_t`.
			 *
			 * @tparam value_t The concrete type to store. Must satisfy `IsValidType<value_t>`.
			 * @param variable The value to copy into the internal storage.
			 */
			template< typename value_t > requires IsValidType< value_t >
			explicit
			Variant (const value_t & variable) noexcept
				: m_data{variable}
			{

			}

			/**
			 * @brief Replaces the currently held value with the supplied value.
			 *
			 * After this call, `type()` returns the `Type` enumerator corresponding to `value_t`
			 * and any previously held value is destroyed. The template parameter is constrained
			 * by `IsValidType` identically to the constructor.
			 *
			 * @tparam value_t The concrete type to store. Must satisfy `IsValidType<value_t>`.
			 * @param variable The value to copy into the internal storage.
			 */
			template< typename value_t > requires IsValidType< value_t >
			void
			set (const value_t & variable) noexcept
			{
				m_data = variable;
			}

			/**
			 * @brief Returns whether the variant currently holds no value.
			 * @return `true` if the internal storage holds `std::monostate` (null state),
			 *         `false` if any concrete value is active.
			 */
			[[nodiscard]]
			bool
			isNull () const noexcept
			{
				return std::holds_alternative< std::monostate >(m_data);
			}

			/**
			 * @brief Returns the discriminator tag of the currently held value.
			 *
			 * The implementation casts `m_data.index()` to `Type` directly, relying on the
			 * 1:1 correspondence between `Storage` alternative indices and `Type` enumerator
			 * values. This is a branchless, constant-time operation.
			 *
			 * @return The `Type` enumerator identifying the active alternative. Returns
			 *         `Type::Null` when no value is held.
			 */
			[[nodiscard]]
			Type
			type () const noexcept
			{
				return static_cast< Type >(m_data.index());
			}

			/**
			 * @brief Returns the held value as a signed 8-bit integer.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Integer8`, emits
			 * a diagnostic to `std::cerr` and returns `0`.
			 *
			 * @pre `type() == Type::Integer8`
			 * @return The stored `int8_t` value, or `0` on type mismatch.
			 */
			[[nodiscard]]
			int8_t asInteger8 () const noexcept;

			/**
			 * @brief Returns the held value as an unsigned 8-bit integer.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::UnsignedInteger8`,
			 * emits a diagnostic to `std::cerr` and returns `0`.
			 *
			 * @pre `type() == Type::UnsignedInteger8`
			 * @return The stored `uint8_t` value, or `0` on type mismatch.
			 */
			[[nodiscard]]
			uint8_t asUnsignedInteger8 () const noexcept;

			/**
			 * @brief Returns the held value as a signed 16-bit integer.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Integer16`, emits
			 * a diagnostic to `std::cerr` and returns `0`.
			 *
			 * @pre `type() == Type::Integer16`
			 * @return The stored `int16_t` value, or `0` on type mismatch.
			 */
			[[nodiscard]]
			int16_t asInteger16 () const noexcept;

			/**
			 * @brief Returns the held value as an unsigned 16-bit integer.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::UnsignedInteger16`,
			 * emits a diagnostic to `std::cerr` and returns `0`.
			 *
			 * @pre `type() == Type::UnsignedInteger16`
			 * @return The stored `uint16_t` value, or `0` on type mismatch.
			 */
			[[nodiscard]]
			uint16_t asUnsignedInteger16 () const noexcept;

			/**
			 * @brief Returns the held value as a signed 32-bit integer.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Integer32`, emits
			 * a diagnostic to `std::cerr` and returns `0`.
			 *
			 * @pre `type() == Type::Integer32`
			 * @return The stored `int32_t` value, or `0` on type mismatch.
			 */
			[[nodiscard]]
			int32_t asInteger32 () const noexcept;

			/**
			 * @brief Returns the held value as an unsigned 32-bit integer.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::UnsignedInteger32`,
			 * emits a diagnostic to `std::cerr` and returns `0`.
			 *
			 * @pre `type() == Type::UnsignedInteger32`
			 * @return The stored `uint32_t` value, or `0` on type mismatch.
			 */
			[[nodiscard]]
			uint32_t asUnsignedInteger32 () const noexcept;

			/**
			 * @brief Returns the held value as a signed 64-bit integer.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Integer64`, emits
			 * a diagnostic to `std::cerr` and returns `0`.
			 *
			 * @pre `type() == Type::Integer64`
			 * @return The stored `int64_t` value, or `0` on type mismatch.
			 */
			[[nodiscard]]
			int64_t asInteger64 () const noexcept;

			/**
			 * @brief Returns the held value as an unsigned 64-bit integer.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::UnsignedInteger64`,
			 * emits a diagnostic to `std::cerr` and returns `0`.
			 *
			 * @pre `type() == Type::UnsignedInteger64`
			 * @return The stored `uint64_t` value, or `0` on type mismatch.
			 */
			[[nodiscard]]
			uint64_t asUnsignedInteger64 () const noexcept;

			/**
			 * @brief Returns the held value as a single-precision floating-point number.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Float`, emits a
			 * diagnostic to `std::cerr` and returns `0.0f`.
			 *
			 * @pre `type() == Type::Float`
			 * @return The stored `float` value, or `0.0f` on type mismatch.
			 */
			[[nodiscard]]
			float asFloat () const noexcept;

			/**
			 * @brief Returns the held value as a double-precision floating-point number.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Double`, emits a
			 * diagnostic to `std::cerr` and returns `0.0`.
			 *
			 * @pre `type() == Type::Double`
			 * @return The stored `double` value, or `0.0` on type mismatch.
			 */
			[[nodiscard]]
			double asDouble () const noexcept;

			/**
			 * @brief Returns the held value as an extended-precision floating-point number.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::LongDouble`, emits
			 * a diagnostic to `std::cerr` and returns `0.0L`.
			 *
			 * @pre `type() == Type::LongDouble`
			 * @return The stored `long double` value, or `0.0L` on type mismatch.
			 */
			[[nodiscard]]
			long double asLongDouble () const noexcept;

			/**
			 * @brief Returns the held value as a boolean.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Boolean`, emits a
			 * diagnostic to `std::cerr` and returns `false`.
			 *
			 * @pre `type() == Type::Boolean`
			 * @return The stored `bool` value, or `false` on type mismatch.
			 */
			[[nodiscard]]
			bool asBool () const noexcept;

			/**
			 * @brief Returns the held value as a 2-component single-precision vector.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Vector2Float`,
			 * emits a diagnostic to `std::cerr` and returns a default-constructed `Math::Vector2F`.
			 *
			 * @pre `type() == Type::Vector2Float`
			 * @return The stored `Math::Vector2F` value, or a zero-initialized vector on type mismatch.
			 */
			[[nodiscard]]
			Math::Vector2F asVector2Float () const noexcept;

			/**
			 * @brief Returns the held value as a 3-component single-precision vector.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Vector3Float`,
			 * emits a diagnostic to `std::cerr` and returns a default-constructed `Math::Vector3F`.
			 *
			 * @pre `type() == Type::Vector3Float`
			 * @return The stored `Math::Vector3F` value, or a zero-initialized vector on type mismatch.
			 */
			[[nodiscard]]
			Math::Vector3F asVector3Float () const noexcept;

			/**
			 * @brief Returns the held value as a 4-component single-precision vector.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Vector4Float`,
			 * emits a diagnostic to `std::cerr` and returns a default-constructed `Math::Vector4F`.
			 *
			 * @pre `type() == Type::Vector4Float`
			 * @return The stored `Math::Vector4F` value, or a zero-initialized vector on type mismatch.
			 */
			[[nodiscard]]
			Math::Vector4F asVector4Float () const noexcept;

			/**
			 * @brief Returns the held value as a 2x2 single-precision matrix.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Matrix2Float`,
			 * emits a diagnostic to `std::cerr` and returns a default-constructed `Math::Matrix2F`.
			 *
			 * @pre `type() == Type::Matrix2Float`
			 * @return The stored `Math::Matrix2F` value, or a default-constructed matrix on type mismatch.
			 */
			[[nodiscard]]
			Math::Matrix2F asMatrix2Float () const noexcept;

			/**
			 * @brief Returns the held value as a 3x3 single-precision matrix.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Matrix3Float`,
			 * emits a diagnostic to `std::cerr` and returns a default-constructed `Math::Matrix3F`.
			 *
			 * @pre `type() == Type::Matrix3Float`
			 * @return The stored `Math::Matrix3F` value, or a default-constructed matrix on type mismatch.
			 */
			[[nodiscard]]
			Math::Matrix3F asMatrix3Float () const noexcept;

			/**
			 * @brief Returns the held value as a 4x4 single-precision matrix.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Matrix4Float`,
			 * emits a diagnostic to `std::cerr` and returns a default-constructed `Math::Matrix4F`.
			 *
			 * @pre `type() == Type::Matrix4Float`
			 * @return The stored `Math::Matrix4F` value, or a default-constructed matrix on type mismatch.
			 */
			[[nodiscard]]
			Math::Matrix4F asMatrix4Float () const noexcept;

			/**
			 * @brief Returns the held value as a single-precision Cartesian frame.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::CartesianFrameFloat`,
			 * emits a diagnostic to `std::cerr` and returns a default-constructed
			 * `Math::CartesianFrameF`.
			 *
			 * @pre `type() == Type::CartesianFrameFloat`
			 * @return The stored `Math::CartesianFrameF` value, or a default-constructed frame on
			 *         type mismatch.
			 */
			[[nodiscard]]
			Math::CartesianFrameF asCartesianFrameFloat () const noexcept;

			/**
			 * @brief Returns the held value as a single-precision RGBA color.
			 *
			 * Uses `std::get_if` internally. If the active type is not `Type::Color`, emits a
			 * diagnostic to `std::cerr` and returns a default-constructed `PixelFactory::ColorF`.
			 *
			 * @pre `type() == Type::Color`
			 * @return The stored `PixelFactory::ColorF` value, or a default-constructed color on
			 *         type mismatch.
			 */
			[[nodiscard]]
			PixelFactory::ColorF asColor () const noexcept;

			/**
			 * @brief Resets the variant to the null state.
			 *
			 * Replaces the internal storage with `std::monostate`. After this call, `isNull()`
			 * returns `true` and `type()` returns `Type::Null`. Any previously held value is
			 * destroyed.
			 */
			void
			reset () noexcept
			{
				m_data = std::monostate{};
			}

			/**
			 * @brief Returns the name of a `Type` enumerator as a null-terminated C-string.
			 *
			 * Matches each enumerator to one of the static `*String` constants defined in this
			 * class. Falls through to `NullString` for `Type::Null` and as a safety default for
			 * any unrecognized value.
			 *
			 * @param type The enumerator whose name is requested.
			 * @return A pointer to a statically allocated, null-terminated string. The pointer
			 *         is always valid and must not be freed by the caller.
			 */
			static const char * to_cstring (Type type) noexcept;

		private:

			/**
			 * @brief Writes a human-readable representation of the variant to an output stream.
			 *
			 * Uses `std::visit` to stream the active value directly. When the variant is null,
			 * outputs the literal string `"Null"`. For all other types the value is streamed
			 * using its own `operator<<`.
			 *
			 * @param out The output stream to write to.
			 * @param variant The variant whose value is written.
			 * @return A reference to `out`, enabling chained stream operations.
			 */
			friend std::ostream & operator<< (std::ostream & out, const Variant & variant) noexcept;

			Storage m_data{}; ///< The underlying std::variant storage holding the active value, or std::monostate when null.
	};
}
