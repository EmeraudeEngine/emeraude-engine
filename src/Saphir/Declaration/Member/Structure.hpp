/*
 * src/Saphir/Declaration/Member/Structure.hpp
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
#include <string>

/* Local inclusions for inheritances. */
#include "Interface.hpp"

namespace EmEn::Saphir::Declaration::Member
{
	/**
	 * @brief The structure member structure.
	 * @extends EmEn::Saphir::Declaration::Member::Interface This is a structure member.
	 */
	class Structure final : public Interface
	{
		public:

			/**
			 * @brief Constructs a structure member.
			 * @param type A C-string to set the type of the member. Use one of Keys::GLSL::* keyword.
			 * @param name A C-string to set the name of the member.
			 */
			Structure (VariableType type, Key name) noexcept;

			/** @copydoc EmEn::Saphir::Declaration::Member::Interface::type() */
			[[nodiscard]]
			VariableType
			type () const noexcept override
			{
				return m_type;
			}

			/** @copydoc EmEn::Saphir::Declaration::Member::Interface::name() */
			[[nodiscard]]
			Key
			name () const noexcept override
			{
				return m_name;
			}

			/** @copydoc EmEn::Saphir::Declaration::Member::Interface::bytes() */
			[[nodiscard]]
			uint32_t
			bytes () const noexcept override
			{
				return size_bytes(m_type);
			}

			/** @copydoc EmEn::Saphir::Declaration::Member::Interface::sourceCode() */
			[[nodiscard]]
			std::string sourceCode () const noexcept override;

		private:

			VariableType m_type;
			Key m_name;
	};
}
