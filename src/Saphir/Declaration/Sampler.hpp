/*
 * src/Saphir/Declaration/Sampler.hpp
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

/* Local inclusions for inheritances. */
#include <limits>

#include "Interface.hpp"

namespace EmEn::Saphir::Declaration
{
	/**
	 * @brief The Sampler class
	 * @extends EmEn::Saphir::Declaration::Interface This is a shader code declaration.
	 */
	class Sampler final : public Interface
	{
		public:

			/** @brief Special value for unbounded arrays (generates [] in GLSL). */
			static constexpr uint32_t UnboundedArray{std::numeric_limits< uint32_t >::max()};

			/**
			 * @brief Constructs a shader uniform variable.
			 * @param set An integer to define in which set the sampler is.
			 * @param binding An integer to define at which point the sampler is bound.
			 * @param type A C-string to set the GLSL type of the sampler. Use one of Keys::GLSL::Sampler* keyword.
			 * @param name A C-string to set the name of the sampler.
			 * @param arraySize Set the variable as an array. Default 0. Use UnboundedArray for runtime-sized arrays.
			 */
			Sampler (uint32_t set, uint32_t binding, Key type, Key name, uint32_t arraySize = 0) noexcept
				: m_set{set},
				m_binding{binding},
				m_type{type},
				m_name{name},
				m_arraySize{arraySize}
			{

			}

			/** @copydoc EmEn::Saphir::Declaration::Interface::isValid() */
			[[nodiscard]]
			bool
			isValid () const noexcept override
			{
				if ( m_type == nullptr )
				{
					return false;
				}

				if ( m_name == nullptr )
				{
					return false;
				}

				return true;
			}

			/** @copydoc EmEn::Saphir::Declaration::Interface::name() */
			[[nodiscard]]
			Key
			name () const noexcept override
			{
				return m_name;
			}

			/** @copydoc EmEn::Saphir::Declaration::Interface::bytes() */
			[[nodiscard]]
			uint32_t
			bytes () const noexcept override
			{
				return 0;
			}

			/** @copydoc EmEn::Saphir::Declaration::Interface::sourceCode() */
			[[nodiscard]]
			std::string sourceCode () const noexcept override;

			/**
			 * @brief Gets the set index.
			 * @return uint32_t.
			 */
			[[nodiscard]]
			uint32_t
			set () const noexcept
			{
				return m_set;
			}

			/**
			 * @brief Gets the binding point.
			 * @return uint32_t.
			 */
			[[nodiscard]]
			uint32_t
			binding () const noexcept
			{
				return m_binding;
			}

			/**
			 * @brief Returns the variable type.
			 * @return Key
			 */
			[[nodiscard]]
			Key type () const noexcept
			{
				return m_type;
			}

			/**
			 * @brief Returns the array size.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			arraySize () const noexcept
			{
				return m_arraySize;
			}

			/**
			 * @brief Returns whether this sampler is an unbounded array.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUnbounded () const noexcept
			{
				return m_arraySize == UnboundedArray;
			}

			/**
			 * @brief Returns whether this sampler is an array (bounded or unbounded).
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isArray () const noexcept
			{
				return m_arraySize > 0;
			}

		private:

			uint32_t m_set;
			uint32_t m_binding;
			Key m_type;
			Key m_name;
			uint32_t m_arraySize;
	};
}
