/*
 * src/Saphir/Declaration/StageInput.hpp
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

/* Forward declarations. */
namespace EmEn::Saphir::Declaration
{
	class StageOutput;
}

namespace EmEn::Saphir::Declaration
{
	/**
	 * @brief The StageInput class
	 * @extends EmEn::Saphir::Declaration::Interface This is a shader code declaration.
	 */
	class StageInput final : public Interface
	{
		public:

			/**
			 * @brief Constructs a stage input declaration.
			 * @param location An integer to specify the location to use in the shader.
			 * @param type A C-string to set the GLSL type of the sampler. Use one of Keys::GLSL::Sampler* keyword.
			 * @param name A C-string to set the name of the sampler.
			 * @param interpolation The interpolation type between stages. Default nullptr. Use one of Keys::GLSL::* keyword.
			 * @param arraySize Set the variable as an array. If size is set to -1, the array will be defined as "[]" in the code.  Default 0.
			 */
			StageInput (uint32_t location, Key type, Key name, Key interpolation = nullptr, int32_t arraySize = 0) noexcept;

			/**
			 * @brief Constructs a stage input declaration from a stage output declaration.
			 * @param stageOutput A reference to a stage output.
			 */
			explicit StageInput (const StageOutput & stageOutput) noexcept;

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
			 * @brief Returns the variable location in the shader.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			location () const noexcept
			{
				return m_location;
			}

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
			 * @brief Returns the variable interpolation.
			 * @return Key
			 */
			[[nodiscard]]
			Key
			interpolation () const noexcept
			{
				return m_interpolation;
			}

			/**
			 * @brief Returns the array size.
			 * @return int32_t
			 */
			[[nodiscard]]
			int32_t
			arraySize () const noexcept
			{
				return m_arraySize;
			}

			/**
			 * @brief Returns whether the expression is an array.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isArray () const noexcept
			{
				return m_arraySize > 0;
			}

			/**
			 * @brief Returns whether the expression is a non-fixed size array.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isNonFixedArraySize () const noexcept
			{
				return m_arraySize == -1;
			}

		private:

			uint32_t m_location;
			Key m_type;
			Key m_name;
			Key m_interpolation;
			int32_t m_arraySize;
	};
}
