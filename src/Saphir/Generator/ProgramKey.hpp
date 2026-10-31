/*
 * src/Saphir/Generator/ProgramKey.hpp
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
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

namespace EmEn::Saphir::Generator
{
	/**
	 * @brief The EXACT identity of a generated shader program, the key of the renderer's program cache
	 * (Graphics::Renderer::findCachedProgram()).
	 * @note ⚠️ Until 2026-10-31 that cache was keyed by a bare size_t, a hash of these same inputs with no equality
	 * behind it: two programs whose hashes met shared one entry, and the first one generated was served for both — a
	 * spot-shadow program (sampler2D) drawn for the sun's cascaded pass (a 2D-array view), intermittently, since the hash
	 * folds in a render pass handle (an address, different every run). Engine item
	 * shadow-sampler-cube-vs-array-at-load, docs/caution-points.md.
	 * @note The key is the byte sequence of every input, in the order the generator adds them: the generator's class
	 * identifier first, then each value in its own fixed width and each text with its length in front, so two different
	 * input lists never give the same bytes. The equality compares those bytes; the hash only places them in the map.
	 * Built once per program generation (a cold path), never per frame.
	 */
	class ProgramKey final
	{
		public:

			/**
			 * @brief Starts the key of a generator.
			 * @param generatorClassId The generator's class identifier (its ClassId).
			 */
			explicit
			ProgramKey (std::string_view generatorClassId) noexcept
			{
				this->add(generatorClassId);
			}

			/**
			 * @brief Adds an integer, a boolean or an enumeration value, in a fixed 64-bit width.
			 * @tparam value_t An integral or enumeration type.
			 * @param value The value.
			 * @return ProgramKey &
			 */
			template< typename value_t >
			requires std::integral< value_t > || std::is_enum_v< value_t >
			ProgramKey &
			add (value_t value) noexcept
			{
				uint64_t widened = 0;

				if constexpr ( std::is_enum_v< value_t > )
				{
					widened = static_cast< uint64_t >(static_cast< std::underlying_type_t< value_t > >(value));
				}
				else
				{
					widened = static_cast< uint64_t >(value);
				}

				const auto bytes = std::bit_cast< std::array< char, sizeof(uint64_t) > >(widened);

				m_bytes.append(bytes.data(), bytes.size());

				return *this;
			}

			/**
			 * @brief Adds an object's identity (a Vulkan handle, an effect): its address.
			 * @param pointer The pointer (null is a valid value).
			 * @return ProgramKey &
			 */
			ProgramKey &
			add (const void * pointer) noexcept
			{
				return this->add(static_cast< uint64_t >(std::bit_cast< uintptr_t >(pointer)));
			}

			/**
			 * @brief Adds a text, its length first.
			 * @param text The text (a renderable's name).
			 * @return ProgramKey &
			 */
			ProgramKey &
			add (std::string_view text) noexcept
			{
				this->add(static_cast< uint64_t >(text.size()));

				m_bytes.append(text);

				return *this;
			}

			/**
			 * @brief Compares two keys byte for byte.
			 * @param other A reference to the other key.
			 * @return bool
			 */
			[[nodiscard]]
			bool operator== (const ProgramKey & other) const noexcept = default;

			/**
			 * @brief Returns the hash of the key, to place it in a hash map.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			hash () const noexcept
			{
				return std::hash< std::string >{}(m_bytes);
			}

		private:

			std::string m_bytes;
	};
}

/**
 * @brief The hash of a ProgramKey, for std::unordered_map.
 */
template<>
struct std::hash< EmEn::Saphir::Generator::ProgramKey >
{
	[[nodiscard]]
	size_t
	operator() (const EmEn::Saphir::Generator::ProgramKey & key) const noexcept
	{
		return key.hash();
	}
};
