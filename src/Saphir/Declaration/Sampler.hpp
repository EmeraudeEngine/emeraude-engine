/*
 * src/Saphir/Declaration/Sampler.hpp
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

/* Local inclusions for inheritances. */
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

			/**
			 * @brief Constructs a shader uniform variable.
			 * @param set An integer to define in which set the sampler is.
			 * @param binding An integer to define at which point the sampler is bound.
			 * @param type A C-string to set the GLSL type of the sampler. Use one of Keys::GLSL::Sampler* keyword.
			 * @param name A C-string to set the name of the sampler.
			 * @param arraySize Set the variable as an array. Default 0.
			 */
			Sampler (uint32_t set, uint32_t binding, Key type, Key name, uint32_t arraySize = 0) noexcept;

			/** @copydoc EmEn::Saphir::Declaration::Interface::isValid() */
			[[nodiscard]]
			bool isValid () const noexcept override;

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

		private:

			uint32_t m_set;
			uint32_t m_binding;
			Key m_type;
			Key m_name;
			uint32_t m_arraySize;
	};
}
