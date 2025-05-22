/*
 * src/Saphir/Declaration/SpecializationConstant.hpp
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
	 * @brief The SpecializationConstant class.
	 * @extends EmEn::Saphir::Declaration::Interface This is a shader code declaration.
	 */
	class SpecializationConstant final : public Interface
	{
		public:

			/**
			 * @brief Constructs a specialization constant.
			 * @param type A C-string to set the type of the uniform. Use one of Keys::GLSL::* keyword.
			 * @param name A C-string to set the name of the uniform.
			 * @param constantId The id of constant.
			 * @param defaultValue A string for the default value [std::move].
			 */
			SpecializationConstant (Key type, Key name, uint32_t constantId, std::string defaultValue) noexcept;

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
			 * @brief Returns the variable type.
			 * @return Key
			 */
			[[nodiscard]]
			Key
			type () const noexcept
			{
				return m_type;
			}

			/**
			 * @brief Returns the constant id.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			constantId () const noexcept
			{
				return m_constantId;
			}

			/**
			 * @brief Returns the default value.
			 * @return const std::string &
			 */
			[[nodiscard]]
			const std::string &
			defaultValue () const noexcept
			{
				return m_defaultValue;
			}

		private:

			Key m_type;
			Key m_name;
			uint32_t m_constantId;
			std::string m_defaultValue;
	};
}
