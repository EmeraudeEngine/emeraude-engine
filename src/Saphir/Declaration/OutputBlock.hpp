/*
 * src/Saphir/Declaration/OutputBlock.hpp
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
#include "AbstractShaderBlock.hpp"

namespace EmEn::Saphir::Declaration
{
	/**
	 * @brief The InputBlock class
	 * @extends EmEn::Saphir::Declaration::AbstractShaderBlock
	 */
	class OutputBlock final : public AbstractShaderBlock
	{
		public:

			/**
			 * @brief Constructs an output block.
			 * @param name A C-string to set the name of the block.
			 * @param location An integer for the location of this block within the shader.
			 * @param instanceName A C-string to set the name of the instance of the block. Default nullptr.
			 * @param arraySize Set the block as an array. Default 0.
			 */
			explicit OutputBlock (Key name, uint32_t location, Key instanceName = nullptr, uint32_t arraySize = 0) noexcept;

			/** @copydoc EmEn::Saphir::Declaration::Interface::sourceCode() */
			[[nodiscard]]
			std::string sourceCode () const noexcept override;
	};
}
