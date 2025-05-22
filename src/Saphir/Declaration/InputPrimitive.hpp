/*
 * src/Saphir/Declaration/InputPrimitive.hpp
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

/* Local inclusions for inheritances. */
#include "Interface.hpp"

/* Local inclusions for usages. */
#include "Types.hpp"

namespace EmEn::Saphir::Declaration
{
	/**
	 * @brief The InputPrimitive class
	 * @extends EmEn::Saphir::Declaration::Interface This is a shader code declaration.
	 */
	class InputPrimitive final : public Interface
	{
		public:

			/**
			 * @brief Constructs an input primitive.
			 * @param primitiveType The input primitive type.
			 */
			explicit InputPrimitive (InputPrimitiveType primitiveType) noexcept;

			/** @copydoc EmEn::Saphir::Declaration::Interface::isValid() */
			[[nodiscard]]
			bool
			isValid () const noexcept override
			{
				return true;
			}

			/** @copydoc EmEn::Saphir::Declaration::Interface::name() */
			[[nodiscard]]
			Key name () const noexcept override;

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
			 * @brief Returns the input primitive type.
			 * @return InputPrimitiveType
			 */
			[[nodiscard]]
			InputPrimitiveType
			primitiveType () const noexcept
			{
				return m_primitiveType;
			}

		private:

			InputPrimitiveType m_primitiveType;
	};
}
