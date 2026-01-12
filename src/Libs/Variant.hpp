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

/* Local inclusions for usage. */
#include "Libs/Math/Matrix.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/Math/CartesianFrame.hpp"

namespace EmEn::Libs
{
	/**
	 * @brief Simple class to store any variable in a container.
	 * @deprecated Use std::any instead !
	 */
	class Variant final
	{
		public:

			static constexpr auto NullString = "Null";
			static constexpr auto Integer8String = "Integer8";
			static constexpr auto UnsignedInteger8String = "UnsignedInteger8";
			static constexpr auto Integer16String = "Integer16";
			static constexpr auto UnsignedInteger16String = "UnsignedInteger16";
			static constexpr auto Integer32String = "Integer32";
			static constexpr auto UnsignedInteger32String = "UnsignedInteger32";
			static constexpr auto Integer64String = "Integer64";
			static constexpr auto UnsignedInteger64String = "UnsignedInteger64";
			static constexpr auto FloatString = "Float";
			static constexpr auto DoubleString = "Double";
			static constexpr auto LongDoubleString = "LongDouble";
			static constexpr auto BooleanString = "Boolean";
			static constexpr auto Vector2FloatString = "Vector2Float";
			static constexpr auto Vector3FloatString = "Vector3Float";
			static constexpr auto Vector4FloatString = "Vector4Float";
			static constexpr auto Matrix2FloatString = "Matrix2Float";
			static constexpr auto Matrix3FloatString = "Matrix3Float";
			static constexpr auto Matrix4FloatString = "Matrix4Float";
			static constexpr auto CartesianFrameString = "CartesianFrameFloat";
			static constexpr auto ColorString = "Color";

			/**
			 * @brief The type of data.
			 */
			enum class Type
			{
				/* Null variable. */
				Null,
				/* Primitives variables. */
				Integer8,
				UnsignedInteger8,
				Integer16,
				UnsignedInteger16,
				Integer32,
				UnsignedInteger32,
				Integer64,
				UnsignedInteger64,
				Float,
				Double,
				LongDouble,
				Boolean,
				/* Libs variables. */
				Vector2Float,
				Vector3Float,
				Vector4Float,
				Matrix2Float,
				Matrix3Float,
				Matrix4Float,
				CartesianFrameFloat,
				Color
			};

			/**
			 * @brief Construct a variant.
			 */
			Variant () noexcept = default;

			/**
			 * @brief Copy constructor.
			 * @param other A reference to a variant.
			 */
			Variant (const Variant & other) noexcept;

			/**
			 * @brief Assignment operator.
			 * @param other A reference to a variant.
			 */
			Variant & operator= (const Variant & other) noexcept;

			/**
			 * @brief Move constructor.
			 * @param other A reference to a variant.
			 */
			Variant (Variant && other) noexcept;

			/**
			 * @brief Move assignment operator.
			 * @param other A reference to a variant.
			 */
			Variant & operator= (Variant && other) noexcept;

			explicit Variant (int8_t variable) noexcept;

			explicit Variant (uint8_t variable) noexcept;

			explicit Variant (int16_t variable) noexcept;

			explicit Variant (uint16_t variable) noexcept;

			explicit Variant (int32_t variable) noexcept;

			explicit Variant (uint32_t variable) noexcept;

			explicit Variant (int64_t variable) noexcept;

			explicit Variant (uint64_t variable) noexcept;

			explicit Variant (float variable) noexcept;

			explicit Variant (double variable) noexcept;

			explicit Variant (long double variable) noexcept;

			explicit Variant (bool variable) noexcept;

			explicit Variant (const Math::Vector2F & variable) noexcept;

			explicit Variant (const Math::Vector3F & variable) noexcept;

			explicit Variant (const Math::Vector4F & variable) noexcept;

			explicit Variant (const Math::Matrix2F & variable) noexcept;

			explicit Variant (const Math::Matrix3F & variable) noexcept;

			explicit Variant (const Math::Matrix4F & variable) noexcept;

			explicit Variant (const Math::CartesianFrameF & variable) noexcept;

			explicit Variant (const PixelFactory::ColorF & variable) noexcept;

			/**
			 * @brief Destructs a variant.
			 */
			~Variant ();

			void set (int8_t variable) noexcept;

			void set (uint8_t variable) noexcept;

			void set (int16_t variable) noexcept;

			void set (uint16_t variable) noexcept;

			void set (int32_t variable) noexcept;

			void set (uint32_t variable) noexcept;

			void set (int64_t variable) noexcept;

			void set (uint64_t variable) noexcept;

			void set (float variable) noexcept;

			void set (double variable) noexcept;

			void set (long double variable) noexcept;

			void set (bool variable) noexcept;

			void set (const Math::Vector2F & variable) noexcept;

			void set (const Math::Vector3F & variable) noexcept;

			void set (const Math::Vector4F & variable) noexcept;

			void set (const Math::Matrix2F & variable) noexcept;

			void set (const Math::Matrix3F & variable) noexcept;

			void set (const Math::Matrix4F & variable) noexcept;

			void set (const Math::CartesianFrameF & variable) noexcept;

			void set (const PixelFactory::ColorF & variable) noexcept;

			[[nodiscard]]
			bool isNull () const noexcept;

			[[nodiscard]]
			Type type () const noexcept;

			[[nodiscard]]
			void * getData () const noexcept;

			[[nodiscard]]
			int8_t asInteger8 () const noexcept;

			[[nodiscard]]
			uint8_t asUnsignedInteger8 () const noexcept;

			[[nodiscard]]
			int16_t asInteger16 () const noexcept;

			[[nodiscard]]
			uint16_t asUnsignedInteger16 () const noexcept;

			[[nodiscard]]
			int32_t asInteger32 () const noexcept;

			[[nodiscard]]
			uint32_t asUnsignedInteger32 () const noexcept;

			[[nodiscard]]
			int64_t asInteger64 () const noexcept;

			[[nodiscard]]
			uint64_t asUnsignedInteger64 () const noexcept;

			[[nodiscard]]
			float asFloat () const noexcept;

			[[nodiscard]]
			double asDouble () const noexcept;

			[[nodiscard]]
			long double asLongDouble () const noexcept;

			[[nodiscard]]
			bool asBool () const noexcept;

			[[nodiscard]]
			Math::Vector2F asVector2Float () const noexcept;

			[[nodiscard]]
			Math::Vector3F asVector3Float () const noexcept;

			[[nodiscard]]
			Math::Vector4F asVector4Float () const noexcept;

			[[nodiscard]]
			Math::Matrix2F asMatrix2Float () const noexcept;

			[[nodiscard]]
			Math::Matrix3F asMatrix3Float () const noexcept;

			[[nodiscard]]
			Math::Matrix4F asMatrix4Float () const noexcept;

			[[nodiscard]]
			Math::CartesianFrameF asCartesianFrameFloat () const noexcept;

			[[nodiscard]]
			PixelFactory::ColorF asColor () const noexcept;

			/**
			 * @brief Reset internal value to null.
			 * @return void
			 */
			void reset () noexcept;

			/**
			 *
			 * @param out
			 * @param variant
			 * @return
			 */
			friend std::ostream & operator<< (std::ostream & out, const Variant & variant) noexcept;

			/**
			 * @brief Returns the type into a C-String format.
			 * @param type The type.
			 * @return const char *
			 */
			static const char * to_cstring (Type type) noexcept;

		private:

			void * m_data{nullptr};
			Type m_type{Type::Null};
	};
}
